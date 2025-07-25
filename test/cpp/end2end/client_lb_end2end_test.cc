// Copyright 2016 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <grpc/event_engine/endpoint_config.h>
#include <grpc/grpc.h>
#include <grpc/support/alloc.h>
#include <grpc/support/atm.h>
#include <grpc/support/time.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/ext/call_metric_recorder.h>
#include <grpcpp/ext/orca_service.h>
#include <grpcpp/ext/server_metric_recorder.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/impl/sync.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <thread>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/core/client_channel/backup_poller.h"
#include "src/core/client_channel/config_selector.h"
#include "src/core/client_channel/global_subchannel_pool.h"
#include "src/core/config/config_vars.h"
#include "src/core/credentials/transport/fake/fake_credentials.h"
#include "src/core/lib/address_utils/parse_address.h"
#include "src/core/lib/address_utils/sockaddr_utils.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/iomgr/tcp_client.h"
#include "src/core/lib/transport/connectivity_state.h"
#include "src/core/resolver/endpoint_addresses.h"
#include "src/core/resolver/fake/fake_resolver.h"
#include "src/core/server/server.h"
#include "src/core/service_config/service_config.h"
#include "src/core/service_config/service_config_impl.h"
#include "src/core/util/backoff.h"
#include "src/core/util/crash.h"
#include "src/core/util/debug_location.h"
#include "src/core/util/env.h"
#include "src/core/util/notification.h"
#include "src/core/util/ref_counted_ptr.h"
#include "src/core/util/time.h"
#include "src/cpp/server/secure_server_credentials.h"
#include "src/proto/grpc/health/v1/health.grpc.pb.h"
#include "src/proto/grpc/testing/echo.grpc.pb.h"
#include "test/core/test_util/port.h"
#include "test/core/test_util/postmortem.h"
#include "test/core/test_util/resolve_localhost_ip46.h"
#include "test/core/test_util/test_config.h"
#include "test/core/test_util/test_lb_policies.h"
#include "test/cpp/end2end/connection_attempt_injector.h"
#include "test/cpp/end2end/test_service_impl.h"
#include "test/cpp/util/credentials.h"
#include "xds/data/orca/v3/orca_load_report.pb.h"

namespace grpc {
namespace testing {
namespace {

using xds::data::orca::v3::OrcaLoadReport;
constexpr char kRequestMessage[] = "Live long and prosper.";

// A noop health check service that just terminates the call and returns OK
// status in its methods. This is used to test the retry mechanism in
// SubchannelStreamClient.
class NoopHealthCheckServiceImpl : public health::v1::Health::Service {
 public:
  ~NoopHealthCheckServiceImpl() override = default;
  Status Check(ServerContext*, const health::v1::HealthCheckRequest*,
               health::v1::HealthCheckResponse*) override {
    return Status::OK;
  }
  Status Watch(ServerContext*, const health::v1::HealthCheckRequest*,
               ServerWriter<health::v1::HealthCheckResponse>*) override {
    grpc_core::MutexLock lock(&mu_);
    request_count_++;
    return Status::OK;
  }
  int request_count() {
    grpc_core::MutexLock lock(&mu_);
    return request_count_;
  }

 private:
  grpc_core::Mutex mu_;
  int request_count_ ABSL_GUARDED_BY(&mu_) = 0;
};

// Subclass of TestServiceImpl that increments a request counter for
// every call to the Echo RPC.
class MyTestServiceImpl : public TestServiceImpl {
 public:
  Status Echo(ServerContext* context, const EchoRequest* request,
              EchoResponse* response) override {
    {
      grpc_core::MutexLock lock(&mu_);
      ++request_count_;
    }
    AddClient(context->peer());
    if (request->has_param() && request->param().has_backend_metrics()) {
      const auto& request_metrics = request->param().backend_metrics();
      auto* recorder = context->ExperimentalGetCallMetricRecorder();
      EXPECT_NE(recorder, nullptr);
      // Do not record when zero since it indicates no test per-call report.
      if (request_metrics.application_utilization() > 0) {
        recorder->RecordApplicationUtilizationMetric(
            request_metrics.application_utilization());
      }
      if (request_metrics.cpu_utilization() > 0) {
        recorder->RecordCpuUtilizationMetric(request_metrics.cpu_utilization());
      }
      if (request_metrics.mem_utilization() > 0) {
        recorder->RecordMemoryUtilizationMetric(
            request_metrics.mem_utilization());
      }
      if (request_metrics.rps_fractional() > 0) {
        recorder->RecordQpsMetric(request_metrics.rps_fractional());
      }
      if (request_metrics.eps() > 0) {
        recorder->RecordEpsMetric(request_metrics.eps());
      }
      for (const auto& [key, value] : request_metrics.request_cost()) {
        char* key_copy = static_cast<char*>(
            grpc_call_arena_alloc(context->c_call(), key.size() + 1));
        strncpy(key_copy, key.data(), key.size());
        key_copy[key.size()] = '\0';
        recorder->RecordRequestCostMetric(key_copy, value);
      }
      for (const auto& [key, value] : request_metrics.utilization()) {
        char* key_copy = static_cast<char*>(
            grpc_call_arena_alloc(context->c_call(), key.size() + 1));
        strncpy(key_copy, key.data(), key.size());
        key_copy[key.size()] = '\0';
        recorder->RecordUtilizationMetric(key_copy, value);
      }
      for (const auto& [key, value] : request_metrics.named_metrics()) {
        char* key_copy = static_cast<char*>(
            grpc_call_arena_alloc(context->c_call(), key.size() + 1));
        strncpy(key_copy, key.data(), key.size());
        key_copy[key.size()] = '\0';
        recorder->RecordNamedMetric(key_copy, value);
      }
    }
    return TestServiceImpl::Echo(context, request, response);
  }

  size_t request_count() {
    grpc_core::MutexLock lock(&mu_);
    return request_count_;
  }

  void ResetCounters() {
    grpc_core::MutexLock lock(&mu_);
    request_count_ = 0;
  }

  std::set<std::string> clients() {
    grpc_core::MutexLock lock(&clients_mu_);
    return clients_;
  }

 private:
  void AddClient(const std::string& client) {
    grpc_core::MutexLock lock(&clients_mu_);
    clients_.insert(client);
  }

  grpc_core::Mutex mu_;
  size_t request_count_ ABSL_GUARDED_BY(&mu_) = 0;

  grpc_core::Mutex clients_mu_;
  std::set<std::string> clients_ ABSL_GUARDED_BY(&clients_mu_);
};

class FakeResolverResponseGeneratorWrapper {
 public:
  FakeResolverResponseGeneratorWrapper()
      : response_generator_(grpc_core::MakeRefCounted<
                            grpc_core::FakeResolverResponseGenerator>()) {}

  FakeResolverResponseGeneratorWrapper(
      FakeResolverResponseGeneratorWrapper&& other) noexcept {
    response_generator_ = std::move(other.response_generator_);
  }

  void SetResponse(grpc_core::Resolver::Result result) {
    grpc_core::ExecCtx exec_ctx;
    response_generator_->SetResponseSynchronously(std::move(result));
  }

  void SetNextResolution(const std::vector<int>& ports,
                         const char* service_config_json = nullptr,
                         const grpc_core::ChannelArgs& per_address_args =
                             grpc_core::ChannelArgs()) {
    SetResponse(BuildFakeResults(ports, service_config_json, per_address_args));
  }

  grpc_core::FakeResolverResponseGenerator* Get() const {
    return response_generator_.get();
  }

 private:
  static grpc_core::Resolver::Result BuildFakeResults(
      const std::vector<int>& ports, const char* service_config_json = nullptr,
      const grpc_core::ChannelArgs& per_address_args =
          grpc_core::ChannelArgs()) {
    grpc_core::Resolver::Result result;
    result.addresses = grpc_core::EndpointAddressesList();
    for (const int& port : ports) {
      absl::StatusOr<grpc_core::URI> lb_uri =
          grpc_core::URI::Parse(grpc_core::LocalIpUri(port));
      CHECK_OK(lb_uri);
      grpc_resolved_address address;
      CHECK(grpc_parse_uri(*lb_uri, &address));
      result.addresses->emplace_back(address, per_address_args);
    }
    if (result.addresses->empty()) {
      result.resolution_note = "fake resolver empty address list";
    }
    if (service_config_json != nullptr) {
      result.service_config = grpc_core::ServiceConfigImpl::Create(
          grpc_core::ChannelArgs(), service_config_json);
      EXPECT_TRUE(result.service_config.ok()) << result.service_config.status();
    }
    return result;
  }

  grpc_core::RefCountedPtr<grpc_core::FakeResolverResponseGenerator>
      response_generator_;
};

constexpr absl::string_view kDefaultAuthority = "default.example.com";

class ClientLbEnd2endTest : public ::testing::Test {
 protected:
  void SetUp() override { grpc_init(); }

  void TearDown() override {
    for (size_t i = 0; i < servers_.size(); ++i) {
      servers_[i]->Shutdown();
    }
    servers_.clear();
    grpc_shutdown();
  }

  void CreateServers(
      size_t num_servers, std::vector<int> ports = {},
      std::shared_ptr<ServerCredentials> server_creds = nullptr) {
    servers_.clear();
    for (size_t i = 0; i < num_servers; ++i) {
      int port = 0;
      if (ports.size() == num_servers) port = ports[i];
      servers_.emplace_back(new ServerData(port, server_creds));
    }
  }

  void StartServer(size_t index) { servers_[index]->Start(); }

  void StartServers(size_t num_servers, std::vector<int> ports = {},
                    std::shared_ptr<ServerCredentials> server_creds = nullptr) {
    CreateServers(num_servers, std::move(ports), std::move(server_creds));
    for (size_t i = 0; i < num_servers; ++i) {
      StartServer(i);
    }
  }

  std::vector<int> GetServersPorts(size_t start_index = 0,
                                   size_t stop_index = 0) {
    if (stop_index == 0) stop_index = servers_.size();
    std::vector<int> ports;
    for (size_t i = start_index; i < stop_index; ++i) {
      ports.push_back(servers_[i]->port_);
    }
    return ports;
  }

  std::unique_ptr<grpc::testing::EchoTestService::Stub> BuildStub(
      const std::shared_ptr<Channel>& channel) {
    return grpc::testing::EchoTestService::NewStub(channel);
  }

  std::shared_ptr<Channel> BuildChannel(
      const std::string& lb_policy_name,
      const FakeResolverResponseGeneratorWrapper& response_generator,
      ChannelArguments args = ChannelArguments(),
      std::shared_ptr<ChannelCredentials> channel_creds = nullptr) {
    if (!lb_policy_name.empty()) {
      args.SetLoadBalancingPolicyName(lb_policy_name);
    }  // else, default to pick first
    args.SetPointer(GRPC_ARG_FAKE_RESOLVER_RESPONSE_GENERATOR,
                    response_generator.Get());
    if (channel_creds == nullptr) {
      channel_creds =
          std::make_shared<FakeTransportSecurityChannelCredentials>();
    }
    return grpc::CreateCustomChannel(absl::StrCat("fake:", kDefaultAuthority),
                                     channel_creds, args);
  }

  Status SendRpc(
      const std::unique_ptr<grpc::testing::EchoTestService::Stub>& stub,
      EchoResponse* response = nullptr, int timeout_ms = 1000,
      bool wait_for_ready = false, EchoRequest* request = nullptr,
      std::string authority_override = "") {
    EchoResponse local_response;
    if (response == nullptr) response = &local_response;
    EchoRequest local_request;
    if (request == nullptr) request = &local_request;
    request->set_message(kRequestMessage);
    request->mutable_param()->set_echo_metadata(true);
    ClientContext context;
    context.set_deadline(grpc_timeout_milliseconds_to_deadline(timeout_ms));
    if (!authority_override.empty()) context.set_authority(authority_override);
    if (wait_for_ready) context.set_wait_for_ready(true);
    context.AddMetadata("foo", "1");
    context.AddMetadata("bar", "2");
    context.AddMetadata("baz", "3");
    return stub->Echo(&context, *request, response);
  }

  void CheckRpcSendOk(
      const grpc_core::DebugLocation& location,
      const std::unique_ptr<grpc::testing::EchoTestService::Stub>& stub,
      bool wait_for_ready = false, const OrcaLoadReport* load_report = nullptr,
      int timeout_ms = 2000) {
    EchoResponse response;
    EchoRequest request;
    EchoRequest* request_ptr = nullptr;
    if (load_report != nullptr) {
      request_ptr = &request;
      auto params = request.mutable_param();
      auto backend_metrics = params->mutable_backend_metrics();
      *backend_metrics = *load_report;
    }
    Status status =
        SendRpc(stub, &response, timeout_ms, wait_for_ready, request_ptr);
    ASSERT_TRUE(status.ok())
        << "From " << location.file() << ":" << location.line()
        << "\nError: " << status.error_message() << " "
        << status.error_details();
    ASSERT_EQ(response.message(), kRequestMessage)
        << "From " << location.file() << ":" << location.line();
  }

  void CheckRpcSendFailure(
      const grpc_core::DebugLocation& location,
      const std::unique_ptr<grpc::testing::EchoTestService::Stub>& stub,
      StatusCode expected_status, absl::string_view expected_message_regex) {
    Status status = SendRpc(stub);
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(expected_status, status.error_code())
        << location.file() << ":" << location.line();
    EXPECT_THAT(status.error_message(),
                ::testing::MatchesRegex(expected_message_regex))
        << location.file() << ":" << location.line();
  }

  void SendRpcsUntil(
      const grpc_core::DebugLocation& debug_location,
      const std::unique_ptr<grpc::testing::EchoTestService::Stub>& stub,
      absl::AnyInvocable<bool(const Status&)> continue_predicate,
      EchoRequest* request_ptr = nullptr, int timeout_ms = 15000) {
    absl::Time deadline = absl::InfiniteFuture();
    if (timeout_ms != 0) {
      deadline = absl::Now() +
                 (absl::Milliseconds(timeout_ms) * grpc_test_slowdown_factor());
    }
    while (true) {
      Status status =
          SendRpc(stub, /*response=*/nullptr, /*timeout_ms=*/1000,
                  /*wait_for_ready=*/false, /*request=*/request_ptr);
      if (!continue_predicate(status)) return;
      EXPECT_LE(absl::Now(), deadline)
          << debug_location.file() << ":" << debug_location.line();
      if (absl::Now() >= deadline) break;
    }
  }

  struct ServerData {
    const int port_;
    const std::shared_ptr<ServerCredentials> server_creds_;
    std::unique_ptr<Server> server_;
    MyTestServiceImpl service_;
    std::unique_ptr<experimental::ServerMetricRecorder> server_metric_recorder_;
    experimental::OrcaService orca_service_;
    std::unique_ptr<std::thread> thread_;
    bool enable_noop_health_check_service_ = false;
    NoopHealthCheckServiceImpl noop_health_check_service_impl_;

    grpc_core::Mutex mu_;
    grpc_core::CondVar cond_;
    bool server_ready_ ABSL_GUARDED_BY(mu_) = false;
    bool started_ ABSL_GUARDED_BY(mu_) = false;

    explicit ServerData(
        int port = 0, std::shared_ptr<ServerCredentials> server_creds = nullptr)
        : port_(port > 0 ? port : grpc_pick_unused_port_or_die()),
          server_creds_(
              server_creds == nullptr
                  ? std::shared_ptr<
                        ServerCredentials>(new SecureServerCredentials(
                        grpc_fake_transport_security_server_credentials_create()))
                  : std::move(server_creds)),
          server_metric_recorder_(experimental::ServerMetricRecorder::Create()),
          orca_service_(
              server_metric_recorder_.get(),
              experimental::OrcaService::Options().set_min_report_duration(
                  absl::Seconds(0.1))) {}

    void Start() {
      LOG(INFO) << "starting server on port " << port_;
      grpc_core::MutexLock lock(&mu_);
      started_ = true;
      thread_ =
          std::make_unique<std::thread>(std::bind(&ServerData::Serve, this));
      while (!server_ready_) {
        cond_.Wait(&mu_);
      }
      server_ready_ = false;
      LOG(INFO) << "server startup complete";
    }

    void Serve() {
      ServerBuilder builder;
      builder.AddListeningPort(absl::StrCat("localhost:", port_),
                               server_creds_);
      builder.RegisterService(&service_);
      builder.RegisterService(&orca_service_);
      if (enable_noop_health_check_service_) {
        builder.RegisterService(&noop_health_check_service_impl_);
      }
      grpc::ServerBuilder::experimental_type(&builder)
          .EnableCallMetricRecording(server_metric_recorder_.get());
      server_ = builder.BuildAndStart();
      grpc_core::MutexLock lock(&mu_);
      server_ready_ = true;
      cond_.Signal();
    }

    void Shutdown() {
      grpc_core::MutexLock lock(&mu_);
      if (!started_) return;
      server_->Shutdown(grpc_timeout_milliseconds_to_deadline(0));
      thread_->join();
      started_ = false;
    }

    void StopListeningAndSendGoaways() {
      grpc_core::ExecCtx exec_ctx;
      auto* server = grpc_core::Server::FromC(server_->c_server());
      server->StopListening();
      server->SendGoaways();
    }

    void SetServingStatus(const std::string& service, bool serving) {
      server_->GetHealthCheckService()->SetServingStatus(service, serving);
    }
  };

  void ResetCounters() {
    for (const auto& server : servers_) server->service_.ResetCounters();
  }

  bool SeenAllServers(size_t start_index = 0, size_t stop_index = 0) {
    if (stop_index == 0) stop_index = servers_.size();
    for (size_t i = start_index; i < stop_index; ++i) {
      if (servers_[i]->service_.request_count() == 0) return false;
    }
    return true;
  }

  // If status_check is null, all RPCs must succeed.
  // If status_check is non-null, it will be called for all non-OK RPCs.
  void WaitForServers(
      const grpc_core::DebugLocation& location,
      const std::unique_ptr<grpc::testing::EchoTestService::Stub>& stub,
      size_t start_index = 0, size_t stop_index = 0,
      absl::AnyInvocable<void(const Status&)> status_check = nullptr,
      absl::Duration timeout = absl::Seconds(30)) {
    if (stop_index == 0) stop_index = servers_.size();
    auto deadline = absl::Now() + (timeout * grpc_test_slowdown_factor());
    LOG(INFO) << "========= WAITING FOR BACKENDS [" << start_index << ", "
              << stop_index << ") ==========";
    while (!SeenAllServers(start_index, stop_index)) {
      Status status = SendRpc(stub);
      if (status_check != nullptr) {
        if (!status.ok()) status_check(status);
      } else {
        EXPECT_TRUE(status.ok())
            << " code=" << status.error_code() << " message=\""
            << status.error_message() << "\" at " << location.file() << ":"
            << location.line();
      }
      EXPECT_LE(absl::Now(), deadline)
          << " at " << location.file() << ":" << location.line();
      if (absl::Now() >= deadline) break;
    }
    ResetCounters();
  }

  void WaitForServer(
      const grpc_core::DebugLocation& location,
      const std::unique_ptr<grpc::testing::EchoTestService::Stub>& stub,
      size_t server_index,
      absl::AnyInvocable<void(const Status&)> status_check = nullptr) {
    WaitForServers(location, stub, server_index, server_index + 1,
                   std::move(status_check));
  }

  bool WaitForChannelState(
      Channel* channel,
      absl::AnyInvocable<bool(grpc_connectivity_state)> predicate,
      bool try_to_connect = false, int timeout_seconds = 5) {
    const gpr_timespec deadline =
        grpc_timeout_seconds_to_deadline(timeout_seconds);
    while (true) {
      grpc_connectivity_state state = channel->GetState(try_to_connect);
      if (predicate(state)) break;
      if (!channel->WaitForStateChange(state, deadline)) return false;
    }
    return true;
  }

  bool WaitForChannelNotReady(Channel* channel, int timeout_seconds = 5) {
    auto predicate = [](grpc_connectivity_state state) {
      return state != GRPC_CHANNEL_READY;
    };
    return WaitForChannelState(channel, predicate, false, timeout_seconds);
  }

  bool WaitForChannelReady(Channel* channel, int timeout_seconds = 5) {
    auto predicate = [](grpc_connectivity_state state) {
      return state == GRPC_CHANNEL_READY;
    };
    return WaitForChannelState(channel, predicate, true, timeout_seconds);
  }

  // Updates \a connection_order by appending to it the index of the newly
  // connected server. Must be called after every single RPC.
  void UpdateConnectionOrder(
      const std::vector<std::unique_ptr<ServerData>>& servers,
      std::vector<int>* connection_order) {
    for (size_t i = 0; i < servers.size(); ++i) {
      if (servers[i]->service_.request_count() == 1) {
        // Was the server index known? If not, update connection_order.
        const auto it =
            std::find(connection_order->begin(), connection_order->end(), i);
        if (it == connection_order->end()) {
          connection_order->push_back(i);
          return;
        }
      }
    }
  }

  void EnableNoopHealthCheckService() {
    for (auto& server : servers_) {
      server->enable_noop_health_check_service_ = true;
    }
  }

  static std::string MakeConnectionFailureRegex(absl::string_view prefix) {
    return absl::StrCat(
        prefix,
        "; last error: (UNKNOWN|UNAVAILABLE): "
        // IP address
        "(ipv6:%5B::1%5D|ipv4:127.0.0.1):[0-9]+: "
        // Prefixes added for context
        "(Failed to connect to remote host: )?"
        "(Timeout occurred: )?"
        // Parenthetical wrappers
        "( ?\\(*("
        "Secure read failed|"
        "Handshake (read|write) failed|"
        "Delayed close due to in-progress write|"
        // Syscall
        "((connect|sendmsg|recvmsg|getsockopt\\(SO\\_ERROR\\)): ?)?"
        // strerror() output or other message
        "(Connection refused"
        "|Connection reset by peer"
        "|Socket closed"
        "|FD shutdown"
        "|Endpoint closing)"
        // errno value
        "( \\([0-9]+\\))?"
        // close paren from wrappers above
        ")\\)*)+");
  }

  std::vector<std::unique_ptr<ServerData>> servers_;
  grpc_core::PostMortem port_mortem_;
};

TEST_F(ClientLbEnd2endTest, ChannelStateConnectingWhenResolving) {
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator);
  auto stub = BuildStub(channel);
  // Initial state should be IDLE.
  EXPECT_EQ(channel->GetState(false /* try_to_connect */), GRPC_CHANNEL_IDLE);
  // Tell the channel to try to connect.
  // Note that this call also returns IDLE, since the state change has
  // not yet occurred; it just gets triggered by this call.
  EXPECT_EQ(channel->GetState(true /* try_to_connect */), GRPC_CHANNEL_IDLE);
  // Now that the channel is trying to connect, we should get to state
  // CONNECTING.
  ASSERT_TRUE(
      WaitForChannelState(channel.get(), [&](grpc_connectivity_state state) {
        if (state == GRPC_CHANNEL_IDLE) return false;
        EXPECT_EQ(state, GRPC_CHANNEL_CONNECTING);
        return true;
      }));
  // Return a resolver result, which allows the connection attempt to proceed.
  response_generator.SetNextResolution(GetServersPorts());
  // We should eventually transition into state READY.
  EXPECT_TRUE(WaitForChannelReady(channel.get()));
}

TEST_F(ClientLbEnd2endTest, ChannelIdleness) {
  // Start server.
  const int kNumServers = 1;
  StartServers(kNumServers);
  // Set max idle time and build the channel.
  ChannelArguments args;
  args.SetInt(GRPC_ARG_CLIENT_IDLE_TIMEOUT_MS,
              1000 * grpc_test_slowdown_factor());
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator, args);
  auto stub = BuildStub(channel);
  // The initial channel state should be IDLE.
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_IDLE);
  // After sending RPC, channel state should be READY.
  LOG(INFO) << "*** SENDING RPC, CHANNEL SHOULD CONNECT ***";
  response_generator.SetNextResolution(GetServersPorts());
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_READY);
  // After a period time not using the channel, the channel state should switch
  // to IDLE.
  LOG(INFO) << "*** WAITING FOR CHANNEL TO GO IDLE ***";
  gpr_sleep_until(grpc_timeout_milliseconds_to_deadline(1200));
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_IDLE);
  // Sending a new RPC should awake the IDLE channel.
  LOG(INFO) << "*** SENDING ANOTHER RPC, CHANNEL SHOULD RECONNECT ***";
  response_generator.SetNextResolution(GetServersPorts());
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_READY);
}

//
// authority override tests
//

class AuthorityOverrideTest : public ClientLbEnd2endTest {
 protected:
  static void SetUpTestSuite() {
    grpc_core::CoreConfiguration::Reset();
    grpc_core::CoreConfiguration::RegisterEphemeralBuilder(
        [](grpc_core::CoreConfiguration::Builder* builder) {
          grpc_core::RegisterAuthorityOverrideLoadBalancingPolicy(builder);
        });
    grpc_init();
  }

  static void TearDownTestSuite() {
    grpc_shutdown();
    grpc_core::CoreConfiguration::Reset();
  }
};

TEST_F(AuthorityOverrideTest, NoOverride) {
  StartServers(1);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request);
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ(kDefaultAuthority, response.param().host());
}

TEST_F(AuthorityOverrideTest, OverrideFromResolver) {
  StartServers(1);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator);
  auto stub = BuildStub(channel);
  // Inject resolver result that sets the per-address authority to a
  // different value.
  response_generator.SetNextResolution(
      GetServersPorts(), /*service_config_json=*/nullptr,
      grpc_core::ChannelArgs().Set(GRPC_ARG_DEFAULT_AUTHORITY,
                                   "from-resolver.example.com"));
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request);
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ("from-resolver.example.com", response.param().host());
}

TEST_F(AuthorityOverrideTest, OverrideOnChannel) {
  StartServers(1);
  // Set authority via channel arg.
  FakeResolverResponseGeneratorWrapper response_generator;
  ChannelArguments args;
  args.SetString(GRPC_ARG_DEFAULT_AUTHORITY, "from-channel.example.com");
  auto channel = BuildChannel("", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request);
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ("from-channel.example.com", response.param().host());
}

TEST_F(AuthorityOverrideTest, OverrideFromLbPolicy) {
  // We use InsecureCreds here to avoid the authority check in the fake
  // security connector.
  StartServers(1, {}, InsecureServerCredentials());
  FakeResolverResponseGeneratorWrapper response_generator;
  ChannelArguments args;
  args.SetString(GRPC_ARG_TEST_LB_AUTHORITY_OVERRIDE, "from-lb.example.com");
  auto channel = BuildChannel("authority_override_lb", response_generator, args,
                              InsecureChannelCredentials());
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request);
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ("from-lb.example.com", response.param().host());
}

TEST_F(AuthorityOverrideTest, PerRpcOverride) {
  // We use InsecureCreds here to avoid the authority check in the fake
  // security connector.
  StartServers(1, {}, InsecureServerCredentials());
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator, ChannelArguments(),
                              InsecureChannelCredentials());
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request,
                          /*authority_override=*/"per-rpc.example.com");
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ("per-rpc.example.com", response.param().host());
}

TEST_F(AuthorityOverrideTest,
       ChannelOverrideTakesPrecedenceOverResolverOverride) {
  StartServers(1);
  // Set authority via channel arg.
  FakeResolverResponseGeneratorWrapper response_generator;
  ChannelArguments args;
  args.SetString(GRPC_ARG_DEFAULT_AUTHORITY, "from-channel.example.com");
  auto channel = BuildChannel("", response_generator, args);
  auto stub = BuildStub(channel);
  // Inject resolver result that sets the per-address authority to a
  // different value.
  response_generator.SetNextResolution(
      GetServersPorts(), /*service_config_json=*/nullptr,
      grpc_core::ChannelArgs().Set(GRPC_ARG_DEFAULT_AUTHORITY,
                                   "from-resolver.example.com"));
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request);
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ("from-channel.example.com", response.param().host());
}

TEST_F(AuthorityOverrideTest,
       LbPolicyOverrideTakesPrecedenceOverChannelOverride) {
  // We use InsecureCreds here to avoid the authority check in the fake
  // security connector.
  StartServers(1, {}, InsecureServerCredentials());
  FakeResolverResponseGeneratorWrapper response_generator;
  ChannelArguments args;
  args.SetString(GRPC_ARG_DEFAULT_AUTHORITY, "from-channel.example.com");
  args.SetString(GRPC_ARG_TEST_LB_AUTHORITY_OVERRIDE, "from-lb.example.com");
  auto channel = BuildChannel("authority_override_lb", response_generator, args,
                              InsecureChannelCredentials());
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request);
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ("from-lb.example.com", response.param().host());
}

TEST_F(AuthorityOverrideTest,
       PerRpcOverrideTakesPrecedenceOverLbPolicyOverride) {
  // We use InsecureCreds here to avoid the authority check in the fake
  // security connector.
  StartServers(1, {}, InsecureServerCredentials());
  FakeResolverResponseGeneratorWrapper response_generator;
  ChannelArguments args;
  args.SetString(GRPC_ARG_TEST_LB_AUTHORITY_OVERRIDE, "from-lb.example.com");
  auto channel = BuildChannel("authority_override_lb", response_generator, args,
                              InsecureChannelCredentials());
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an RPC.
  EchoRequest request;
  request.mutable_param()->set_echo_host_from_authority_header(true);
  EchoResponse response;
  Status status = SendRpc(stub, &response, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request,
                          /*authority_override=*/"per-rpc.example.com");
  EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                           << " message=" << status.error_message();
  // Check that the right authority was seen by the server.
  EXPECT_EQ("per-rpc.example.com", response.param().host());
}

//
// pick_first tests
//

using PickFirstTest = ClientLbEnd2endTest;

TEST_F(PickFirstTest, Basic) {
  // Start servers and send one RPC per server.
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel(
      "", response_generator);  // test that pick first is the default.
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  for (size_t i = 0; i < servers_.size(); ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  // All requests should have gone to a single server.
  bool found = false;
  for (size_t i = 0; i < servers_.size(); ++i) {
    const int request_count = servers_[i]->service_.request_count();
    if (request_count == kNumServers) {
      found = true;
    } else {
      EXPECT_EQ(0, request_count);
    }
  }
  EXPECT_TRUE(found);
  // Check LB policy name for the channel.
  EXPECT_EQ("pick_first", channel->GetLoadBalancingPolicyName());
}

TEST_F(PickFirstTest, ProcessPending) {
  StartServers(1);  // Single server
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel(
      "", response_generator);  // test that pick first is the default.
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution({servers_[0]->port_});
  WaitForServer(DEBUG_LOCATION, stub, 0);
  // Create a new channel and its corresponding PF LB policy, which will pick
  // the subchannels in READY state from the previous RPC against the same
  // target (even if it happened over a different channel, because subchannels
  // are globally reused). Progress should happen without any transition from
  // this READY state.
  FakeResolverResponseGeneratorWrapper second_response_generator;
  auto second_channel = BuildChannel("", second_response_generator);
  auto second_stub = BuildStub(second_channel);
  second_response_generator.SetNextResolution({servers_[0]->port_});
  CheckRpcSendOk(DEBUG_LOCATION, second_stub);
}

TEST_F(PickFirstTest, SelectsReadyAtStartup) {
  ChannelArguments args;
  constexpr int kInitialBackOffMs = 5000;
  args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS,
              kInitialBackOffMs * grpc_test_slowdown_factor());
  // Create 2 servers, but start only the second one.
  std::vector<int> ports = {grpc_pick_unused_port_or_die(),
                            grpc_pick_unused_port_or_die()};
  CreateServers(2, ports);
  StartServer(1);
  FakeResolverResponseGeneratorWrapper response_generator1;
  auto channel1 = BuildChannel("pick_first", response_generator1, args);
  auto stub1 = BuildStub(channel1);
  response_generator1.SetNextResolution(ports);
  // Wait for second server to be ready.
  WaitForServer(DEBUG_LOCATION, stub1, 1);
  // Create a second channel with the same addresses.  Its PF instance
  // should immediately pick the second subchannel, since it's already
  // in READY state.
  FakeResolverResponseGeneratorWrapper response_generator2;
  auto channel2 = BuildChannel("pick_first", response_generator2, args);
  response_generator2.SetNextResolution(ports);
  // Check that the channel reports READY without waiting for the
  // initial backoff.
  EXPECT_TRUE(WaitForChannelReady(channel2.get(), 1 /* timeout_seconds */));
}

TEST_F(PickFirstTest, BackOffInitialReconnect) {
  StartServers(1);
  ChannelArguments args;
  constexpr int kInitialBackOffMs = 100;
  args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS,
              kInitialBackOffMs * grpc_test_slowdown_factor());
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution({servers_[0]->port_});
  // Intercept the first two connection attempts.
  ConnectionAttemptInjector injector;
  auto hold1 = injector.AddHold(servers_[0]->port_);
  auto hold2 = injector.AddHold(servers_[0]->port_);
  // Start trying to connect.
  EXPECT_EQ(channel->GetState(/*try_to_connect=*/true), GRPC_CHANNEL_IDLE);
  // When the first connection attempt starts, record the time, then fail the
  // attempt.
  hold1->Wait();
  const grpc_core::Timestamp first_attempt_time = grpc_core::Timestamp::Now();
  hold1->Fail(absl::UnavailableError("nope"));
  // Wait for the second attempt and see how long it took.
  hold2->Wait();
  const grpc_core::Duration waited =
      grpc_core::Timestamp::Now() - first_attempt_time;
  // The channel will transition to TRANSIENT_FAILURE.
  EXPECT_TRUE(
      WaitForChannelState(channel.get(), [&](grpc_connectivity_state state) {
        if (state == GRPC_CHANNEL_TRANSIENT_FAILURE) return true;
        EXPECT_THAT(state, ::testing::AnyOf(GRPC_CHANNEL_IDLE,
                                            GRPC_CHANNEL_CONNECTING));
        return false;
      }));
  // Now let the second attempt complete.
  hold2->Resume();
  // The channel will transition to READY.
  EXPECT_TRUE(WaitForChannelReady(channel.get()));
  // Check how long it took.
  VLOG(2) << "Waited " << waited.millis() << " milliseconds";
  // We should have waited at least kInitialBackOffMs, plus or minus
  // jitter.  Jitter is 0.2, but we give extra leeway to account for
  // measurement skew, thread hops, etc.
  EXPECT_GE(waited.millis(),
            (kInitialBackOffMs * grpc_test_slowdown_factor()) * 0.7);
  EXPECT_LE(waited.millis(),
            (kInitialBackOffMs * grpc_test_slowdown_factor()) * 1.5);
}

TEST_F(PickFirstTest, BackOffMinReconnect) {
  ChannelArguments args;
  constexpr int kMinReconnectBackOffMs = 1000;
  args.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS,
              kMinReconnectBackOffMs * grpc_test_slowdown_factor());
  const std::vector<int> ports = {grpc_pick_unused_port_or_die()};
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(ports);
  // Make connection delay a 10% longer than it's willing to in order to make
  // sure we are hitting the codepath that waits for the min reconnect backoff.
  ConnectionAttemptInjector injector;
  injector.SetDelay(grpc_core::Duration::Milliseconds(
      kMinReconnectBackOffMs * grpc_test_slowdown_factor() * 1.10));
  const gpr_timespec t0 = gpr_now(GPR_CLOCK_MONOTONIC);
  channel->WaitForConnected(
      grpc_timeout_milliseconds_to_deadline(kMinReconnectBackOffMs * 2));
  const gpr_timespec t1 = gpr_now(GPR_CLOCK_MONOTONIC);
  const grpc_core::Duration waited =
      grpc_core::Duration::FromTimespec(gpr_time_sub(t1, t0));
  VLOG(2) << "Waited " << waited.millis() << " milliseconds";
  // We should have waited at least kMinReconnectBackOffMs. We subtract one to
  // account for test and precision accuracy drift.
  EXPECT_GE(waited.millis(),
            (kMinReconnectBackOffMs * grpc_test_slowdown_factor()) - 1);
}

TEST_F(PickFirstTest, ResetConnectionBackoff) {
  ChannelArguments args;
  constexpr int kInitialBackOffMs = 1000;
  args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS,
              kInitialBackOffMs * grpc_test_slowdown_factor());
  const std::vector<int> ports = {grpc_pick_unused_port_or_die()};
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(ports);
  // The channel won't become connected (there's no server).
  EXPECT_FALSE(
      channel->WaitForConnected(grpc_timeout_milliseconds_to_deadline(10)));
  // Bring up a server on the chosen port.
  StartServers(1, ports);
  const gpr_timespec t0 = gpr_now(GPR_CLOCK_MONOTONIC);
  // Wait for connect, but not long enough.  This proves that we're
  // being throttled by initial backoff.
  EXPECT_FALSE(
      channel->WaitForConnected(grpc_timeout_milliseconds_to_deadline(10)));
  // Reset connection backoff.
  experimental::ChannelResetConnectionBackoff(channel.get());
  // Wait for connect.  Should happen as soon as the client connects to
  // the newly started server, which should be before the initial
  // backoff timeout elapses.
  EXPECT_TRUE(channel->WaitForConnected(
      grpc_timeout_milliseconds_to_deadline(kInitialBackOffMs)));
  const gpr_timespec t1 = gpr_now(GPR_CLOCK_MONOTONIC);
  const grpc_core::Duration waited =
      grpc_core::Duration::FromTimespec(gpr_time_sub(t1, t0));
  VLOG(2) << "Waited " << waited.millis() << " milliseconds";
  // We should have waited less than kInitialBackOffMs.
  EXPECT_LT(waited.millis(), kInitialBackOffMs * grpc_test_slowdown_factor());
}

TEST_F(ClientLbEnd2endTest,
       ResetConnectionBackoffNextAttemptStartsImmediately) {
  // Start connection injector.
  ConnectionAttemptInjector injector;
  // Create client.
  const int port = grpc_pick_unused_port_or_die();
  ChannelArguments args;
  const int kInitialBackOffMs = 5000;
  args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS,
              kInitialBackOffMs * grpc_test_slowdown_factor());
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution({port});
  // Intercept initial connection attempt.
  auto hold1 = injector.AddHold(port);
  LOG(INFO) << "=== TRIGGERING INITIAL CONNECTION ATTEMPT";
  EXPECT_EQ(GRPC_CHANNEL_IDLE, channel->GetState(/*try_to_connect=*/true));
  hold1->Wait();
  EXPECT_EQ(GRPC_CHANNEL_CONNECTING,
            channel->GetState(/*try_to_connect=*/false));
  // Reset backoff.
  LOG(INFO) << "=== RESETTING BACKOFF";
  experimental::ChannelResetConnectionBackoff(channel.get());
  // Intercept next attempt.  Do this before resuming the first attempt,
  // just in case the client makes progress faster than this thread.
  auto hold2 = injector.AddHold(port);
  // Fail current attempt and wait for next one to start.
  LOG(INFO) << "=== RESUMING INITIAL ATTEMPT";
  const gpr_timespec t0 = gpr_now(GPR_CLOCK_MONOTONIC);
  hold1->Resume();
  LOG(INFO) << "=== WAITING FOR SECOND ATTEMPT";
  // This WaitForStateChange() call just makes sure we're doing some polling.
  EXPECT_TRUE(channel->WaitForStateChange(GRPC_CHANNEL_CONNECTING,
                                          grpc_timeout_seconds_to_deadline(1)));
  hold2->Wait();
  const gpr_timespec t1 = gpr_now(GPR_CLOCK_MONOTONIC);
  LOG(INFO) << "=== RESUMING SECOND ATTEMPT";
  hold2->Resume();
  // Elapsed time should be very short, much less than kInitialBackOffMs.
  const grpc_core::Duration waited =
      grpc_core::Duration::FromTimespec(gpr_time_sub(t1, t0));
  VLOG(2) << "Waited " << waited.millis() << " milliseconds";
  EXPECT_LT(waited.millis(), 1000 * grpc_test_slowdown_factor());
}

TEST_F(PickFirstTest, Updates) {
  // Start servers and send one RPC per server.
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  // Perform one RPC against the first server.
  response_generator.SetNextResolution(GetServersPorts(0, 1));
  LOG(INFO) << "****** SET [0] *******";
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(servers_[0]->service_.request_count(), 1);
  // An empty update will result in the channel going into TRANSIENT_FAILURE.
  response_generator.SetNextResolution({});
  LOG(INFO) << "****** SET none *******";
  WaitForChannelNotReady(channel.get());
  // Next update introduces servers_[1], making the channel recover.
  response_generator.SetNextResolution(GetServersPorts(1, 2));
  LOG(INFO) << "****** SET [1] *******";
  WaitForChannelReady(channel.get());
  WaitForServer(DEBUG_LOCATION, stub, 1);
  // And again for servers_[2]
  response_generator.SetNextResolution(GetServersPorts(2, 3));
  LOG(INFO) << "****** SET [2] *******";
  WaitForServer(DEBUG_LOCATION, stub, 2);
  // Check LB policy name for the channel.
  EXPECT_EQ("pick_first", channel->GetLoadBalancingPolicyName());
}

TEST_F(PickFirstTest, UpdateSuperset) {
  // Start servers and send one RPC per server.
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);

  std::vector<int> ports;

  // Perform one RPC against the first server.
  ports.emplace_back(servers_[0]->port_);
  response_generator.SetNextResolution(ports);
  LOG(INFO) << "****** SET [0] *******";
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(servers_[0]->service_.request_count(), 1);
  servers_[0]->service_.ResetCounters();

  // Send and superset update
  ports.clear();
  ports.emplace_back(servers_[1]->port_);
  ports.emplace_back(servers_[0]->port_);
  response_generator.SetNextResolution(ports);
  LOG(INFO) << "****** SET superset *******";
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  // We stick to the previously connected server.
  WaitForServer(DEBUG_LOCATION, stub, 0);
  EXPECT_EQ(0, servers_[1]->service_.request_count());

  // Check LB policy name for the channel.
  EXPECT_EQ("pick_first", channel->GetLoadBalancingPolicyName());
}

TEST_F(PickFirstTest, UpdateToUnconnected) {
  const int kNumServers = 2;
  CreateServers(kNumServers);
  StartServer(0);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);

  std::vector<int> ports;

  // Try to send rpcs against a list where the server is available.
  ports.emplace_back(servers_[0]->port_);
  response_generator.SetNextResolution(ports);
  LOG(INFO) << "****** SET [0] *******";
  CheckRpcSendOk(DEBUG_LOCATION, stub);

  // Send resolution for which all servers are currently unavailable. Eventually
  // this triggers replacing the existing working subchannel_list with the new
  // currently unresponsive list.
  ports.clear();
  ports.emplace_back(grpc_pick_unused_port_or_die());
  ports.emplace_back(servers_[1]->port_);
  response_generator.SetNextResolution(ports);
  LOG(INFO) << "****** SET [unavailable] *******";
  EXPECT_TRUE(WaitForChannelNotReady(channel.get()));

  // Ensure that the last resolution was installed correctly by verifying that
  // the channel becomes ready once one of if its endpoints becomes available.
  LOG(INFO) << "****** StartServer(1) *******";
  StartServer(1);
  EXPECT_TRUE(WaitForChannelReady(channel.get()));
}

TEST_F(PickFirstTest, GlobalSubchannelPool) {
  // Start one server.
  const int kNumServers = 1;
  StartServers(kNumServers);
  std::vector<int> ports = GetServersPorts();
  // Create two channels that (by default) use the global subchannel pool.
  // Use the same channel creds for both, so that they have the same
  // subchannel keys.
  auto channel_creds =
      std::make_shared<FakeTransportSecurityChannelCredentials>();
  FakeResolverResponseGeneratorWrapper response_generator1;
  auto channel1 = BuildChannel("pick_first", response_generator1,
                               ChannelArguments(), channel_creds);
  auto stub1 = BuildStub(channel1);
  response_generator1.SetNextResolution(ports);
  FakeResolverResponseGeneratorWrapper response_generator2;
  auto channel2 = BuildChannel("pick_first", response_generator2,
                               ChannelArguments(), channel_creds);
  auto stub2 = BuildStub(channel2);
  response_generator2.SetNextResolution(ports);
  WaitForServer(DEBUG_LOCATION, stub1, 0);
  // Send one RPC on each channel.
  CheckRpcSendOk(DEBUG_LOCATION, stub1);
  CheckRpcSendOk(DEBUG_LOCATION, stub2);
  // The server receives two requests.
  EXPECT_EQ(2, servers_[0]->service_.request_count());
  // The two requests are from the same client port, because the two channels
  // share subchannels via the global subchannel pool.
  EXPECT_EQ(1UL, servers_[0]->service_.clients().size());
}

TEST_F(PickFirstTest, LocalSubchannelPool) {
  // Start one server.
  const int kNumServers = 1;
  StartServers(kNumServers);
  std::vector<int> ports = GetServersPorts();
  // Create two channels that use local subchannel pool.
  ChannelArguments args;
  args.SetInt(GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL, 1);
  FakeResolverResponseGeneratorWrapper response_generator1;
  auto channel1 = BuildChannel("pick_first", response_generator1, args);
  auto stub1 = BuildStub(channel1);
  response_generator1.SetNextResolution(ports);
  FakeResolverResponseGeneratorWrapper response_generator2;
  auto channel2 = BuildChannel("pick_first", response_generator2, args);
  auto stub2 = BuildStub(channel2);
  response_generator2.SetNextResolution(ports);
  WaitForServer(DEBUG_LOCATION, stub1, 0);
  // Send one RPC on each channel.
  CheckRpcSendOk(DEBUG_LOCATION, stub1);
  CheckRpcSendOk(DEBUG_LOCATION, stub2);
  // The server receives two requests.
  EXPECT_EQ(2, servers_[0]->service_.request_count());
  // The two requests are from two client ports, because the two channels didn't
  // share subchannels with each other.
  EXPECT_EQ(2UL, servers_[0]->service_.clients().size());
}

TEST_F(PickFirstTest, ManyUpdates) {
  const int kNumUpdates = 1000;
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  std::vector<int> ports = GetServersPorts();
  for (size_t i = 0; i < kNumUpdates; ++i) {
    std::shuffle(ports.begin(), ports.end(),
                 std::mt19937(std::random_device()()));
    response_generator.SetNextResolution(ports);
    // We should re-enter core at the end of the loop to give the resolution
    // setting closure a chance to run.
    if ((i + 1) % 10 == 0) CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  // Check LB policy name for the channel.
  EXPECT_EQ("pick_first", channel->GetLoadBalancingPolicyName());
}

TEST_F(PickFirstTest, ReresolutionNoSelected) {
  // Prepare the ports for up servers and down servers.
  const int kNumServers = 3;
  const int kNumAliveServers = 1;
  StartServers(kNumAliveServers);
  std::vector<int> alive_ports, dead_ports;
  for (size_t i = 0; i < kNumServers; ++i) {
    if (i < kNumAliveServers) {
      alive_ports.emplace_back(servers_[i]->port_);
    } else {
      dead_ports.emplace_back(grpc_pick_unused_port_or_die());
    }
  }
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  // The initial resolution only contains dead ports. There won't be any
  // selected subchannel. Re-resolution will return the same result.
  response_generator.SetNextResolution(dead_ports);
  LOG(INFO) << "****** INITIAL RESOLUTION SET *******";
  for (size_t i = 0; i < 10; ++i) {
    CheckRpcSendFailure(
        DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
        MakeConnectionFailureRegex("failed to connect to all addresses"));
  }
  // PF should request re-resolution.
  LOG(INFO) << "****** WAITING FOR RE-RESOLUTION *******";
  EXPECT_TRUE(response_generator.Get()->WaitForReresolutionRequest(
      absl::Seconds(5 * grpc_test_slowdown_factor())));
  LOG(INFO) << "****** RE-RESOLUTION SEEN *******";
  // Send a resolver result that contains reachable ports, so that the
  // pick_first LB policy can recover soon.
  response_generator.SetNextResolution(alive_ports);
  LOG(INFO) << "****** RE-RESOLUTION SENT *******";
  WaitForServer(DEBUG_LOCATION, stub, 0, [](const Status& status) {
    EXPECT_EQ(StatusCode::UNAVAILABLE, status.error_code());
    EXPECT_THAT(status.error_message(),
                ::testing::ContainsRegex(MakeConnectionFailureRegex(
                    "failed to connect to all addresses")));
  });
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(servers_[0]->service_.request_count(), 1);
  // Check LB policy name for the channel.
  EXPECT_EQ("pick_first", channel->GetLoadBalancingPolicyName());
}

TEST_F(PickFirstTest, ReconnectWithoutNewResolverResult) {
  std::vector<int> ports = {grpc_pick_unused_port_or_die()};
  StartServers(1, ports);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(ports);
  LOG(INFO) << "****** INITIAL CONNECTION *******";
  WaitForServer(DEBUG_LOCATION, stub, 0);
  LOG(INFO) << "****** STOPPING SERVER ******";
  servers_[0]->Shutdown();
  EXPECT_TRUE(WaitForChannelNotReady(channel.get()));
  LOG(INFO) << "****** RESTARTING SERVER ******";
  StartServers(1, ports);
  WaitForServer(DEBUG_LOCATION, stub, 0);
}

TEST_F(PickFirstTest, ReconnectWithoutNewResolverResultStartsFromTopOfList) {
  std::vector<int> ports = {grpc_pick_unused_port_or_die(),
                            grpc_pick_unused_port_or_die()};
  CreateServers(2, ports);
  StartServer(1);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(ports);
  LOG(INFO) << "****** INITIAL CONNECTION *******";
  WaitForServer(DEBUG_LOCATION, stub, 1);
  LOG(INFO) << "****** STOPPING SERVER ******";
  servers_[1]->Shutdown();
  EXPECT_TRUE(WaitForChannelNotReady(channel.get()));
  LOG(INFO) << "****** STARTING BOTH SERVERS ******";
  StartServers(2, ports);
  WaitForServer(DEBUG_LOCATION, stub, 0);
}

TEST_F(PickFirstTest, FailsEmptyResolverUpdate) {
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  LOG(INFO) << "****** SENDING INITIAL RESOLVER RESULT *******";
  // Send a resolver result with an empty address list and a callback
  // that triggers a notification.
  grpc_core::Notification notification;
  grpc_core::Resolver::Result result;
  result.addresses.emplace();
  result.result_health_callback = [&](absl::Status status) {
    LOG(INFO) << "****** RESULT HEALTH CALLBACK *******";
    EXPECT_EQ(absl::StatusCode::kUnavailable, status.code());
    EXPECT_EQ("address list must not be empty", status.message()) << status;
    notification.Notify();
  };
  response_generator.SetResponse(std::move(result));
  // Wait for channel to report TRANSIENT_FAILURE.
  LOG(INFO) << "****** TELLING CHANNEL TO CONNECT *******";
  auto predicate = [](grpc_connectivity_state state) {
    return state == GRPC_CHANNEL_TRANSIENT_FAILURE;
  };
  EXPECT_TRUE(
      WaitForChannelState(channel.get(), predicate, /*try_to_connect=*/true));
  // Callback should run.
  notification.WaitForNotification();
  // Return a valid address.
  LOG(INFO) << "****** SENDING NEXT RESOLVER RESULT *******";
  StartServers(1);
  response_generator.SetNextResolution(GetServersPorts());
  LOG(INFO) << "****** SENDING WAIT_FOR_READY RPC *******";
  CheckRpcSendOk(DEBUG_LOCATION, stub, /*wait_for_ready=*/true);
}

TEST_F(PickFirstTest, CheckStateBeforeStartWatch) {
  std::vector<int> ports = {grpc_pick_unused_port_or_die()};
  StartServers(1, ports);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel_1 = BuildChannel("pick_first", response_generator);
  auto stub_1 = BuildStub(channel_1);
  response_generator.SetNextResolution(ports);
  LOG(INFO) << "****** RESOLUTION SET FOR CHANNEL 1 *******";
  WaitForServer(DEBUG_LOCATION, stub_1, 0);
  LOG(INFO) << "****** CHANNEL 1 CONNECTED *******";
  servers_[0]->Shutdown();
  EXPECT_TRUE(WaitForChannelNotReady(channel_1.get()));
  // Channel 1 will receive a re-resolution containing the same server. It will
  // create a new subchannel and hold a ref to it.
  StartServers(1, ports);
  LOG(INFO) << "****** SERVER RESTARTED *******";
  FakeResolverResponseGeneratorWrapper response_generator_2;
  auto channel_2 = BuildChannel("pick_first", response_generator_2);
  auto stub_2 = BuildStub(channel_2);
  response_generator_2.SetNextResolution(ports);
  LOG(INFO) << "****** RESOLUTION SET FOR CHANNEL 2 *******";
  WaitForServer(DEBUG_LOCATION, stub_2, 0);
  LOG(INFO) << "****** CHANNEL 2 CONNECTED *******";
  servers_[0]->Shutdown();
  // Wait until the disconnection has triggered the connectivity notification.
  // Otherwise, the subchannel may be picked for next call but will fail soon.
  EXPECT_TRUE(WaitForChannelNotReady(channel_2.get()));
  // Channel 2 will also receive a re-resolution containing the same server.
  // Both channels will ref the same subchannel that failed.
  StartServers(1, ports);
  LOG(INFO) << "****** SERVER RESTARTED AGAIN *******";
  LOG(INFO) << "****** CHANNEL 2 STARTING A CALL *******";
  // The first call after the server restart will succeed.
  CheckRpcSendOk(DEBUG_LOCATION, stub_2);
  LOG(INFO) << "****** CHANNEL 2 FINISHED A CALL *******";
  // Check LB policy name for the channel.
  EXPECT_EQ("pick_first", channel_1->GetLoadBalancingPolicyName());
  // Check LB policy name for the channel.
  EXPECT_EQ("pick_first", channel_2->GetLoadBalancingPolicyName());
}

TEST_F(PickFirstTest, IdleOnDisconnect) {
  // Start server, send RPC, and make sure channel is READY.
  const int kNumServers = 1;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel =
      BuildChannel("", response_generator);  // pick_first is the default.
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_READY);
  // Stop server.  Channel should go into state IDLE.
  servers_[0]->Shutdown();
  EXPECT_TRUE(WaitForChannelNotReady(channel.get()));
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_IDLE);
  servers_.clear();
}

TEST_F(PickFirstTest, StaysIdleUponEmptyUpdate) {
  // Start server, send RPC, and make sure channel is READY.
  const int kNumServers = 1;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel =
      BuildChannel("", response_generator);  // pick_first is the default.
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_READY);
  // Stop server.  Channel should go into state IDLE.
  servers_[0]->Shutdown();
  EXPECT_TRUE(WaitForChannelNotReady(channel.get()));
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_IDLE);
  // Now send resolver update that includes no addresses.  Channel
  // should stay in state IDLE.
  response_generator.SetNextResolution({});
  EXPECT_FALSE(channel->WaitForStateChange(
      GRPC_CHANNEL_IDLE, grpc_timeout_seconds_to_deadline(3)));
  // Now bring the backend back up and send a non-empty resolver update,
  // and then try to send an RPC.  Channel should go back into state READY.
  StartServer(0);
  response_generator.SetNextResolution(GetServersPorts());
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(channel->GetState(false), GRPC_CHANNEL_READY);
}

TEST_F(PickFirstTest,
       StaysTransientFailureOnFailedConnectionAttemptUntilReady) {
  // Allocate 3 ports, with no servers running.
  std::vector<int> ports = {grpc_pick_unused_port_or_die(),
                            grpc_pick_unused_port_or_die(),
                            grpc_pick_unused_port_or_die()};
  // Create channel with a 1-second backoff.
  ChannelArguments args;
  args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS,
              1000 * grpc_test_slowdown_factor());
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(ports);
  EXPECT_EQ(GRPC_CHANNEL_IDLE, channel->GetState(false));
  // Send an RPC, which should fail.
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
      MakeConnectionFailureRegex("failed to connect to all addresses"));
  // Channel should be in TRANSIENT_FAILURE.
  EXPECT_EQ(GRPC_CHANNEL_TRANSIENT_FAILURE, channel->GetState(false));
  // Now start a server on the last port.
  StartServers(1, {ports.back()});
  // Channel should remain in TRANSIENT_FAILURE until it transitions to READY.
  EXPECT_TRUE(channel->WaitForStateChange(GRPC_CHANNEL_TRANSIENT_FAILURE,
                                          grpc_timeout_seconds_to_deadline(4)));
  EXPECT_EQ(GRPC_CHANNEL_READY, channel->GetState(false));
  CheckRpcSendOk(DEBUG_LOCATION, stub);
}

//
// round_robin tests
//

using RoundRobinTest = ClientLbEnd2endTest;

TEST_F(RoundRobinTest, Basic) {
  // Start servers and send one RPC per server.
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Wait until all backends are ready.
  WaitForServers(DEBUG_LOCATION, stub);
  // "Sync" to the end of the list. Next sequence of picks will start at the
  // first server (index 0).
  WaitForServer(DEBUG_LOCATION, stub, servers_.size() - 1);
  // Backends should be iterated over in the order in which the addresses were
  // given.
  std::vector<int> connection_order;
  for (size_t i = 0; i < servers_.size(); ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
    UpdateConnectionOrder(servers_, &connection_order);
  }
  EXPECT_THAT(connection_order, ::testing::ElementsAre(0, 1, 2));
  // Check LB policy name for the channel.
  EXPECT_EQ("round_robin", channel->GetLoadBalancingPolicyName());
}

TEST_F(RoundRobinTest, ProcessPending) {
  StartServers(1);  // Single server
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution({servers_[0]->port_});
  WaitForServer(DEBUG_LOCATION, stub, 0);
  // Create a new channel and its corresponding RR LB policy, which will pick
  // the subchannels in READY state from the previous RPC against the same
  // target (even if it happened over a different channel, because subchannels
  // are globally reused). Progress should happen without any transition from
  // this READY state.
  FakeResolverResponseGeneratorWrapper second_response_generator;
  auto second_channel = BuildChannel("round_robin", second_response_generator);
  auto second_stub = BuildStub(second_channel);
  second_response_generator.SetNextResolution({servers_[0]->port_});
  CheckRpcSendOk(DEBUG_LOCATION, second_stub);
}

TEST_F(RoundRobinTest, Updates) {
  // Start servers.
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  // Start with a single server.
  LOG(INFO) << "*** FIRST BACKEND ***";
  std::vector<int> ports = {servers_[0]->port_};
  response_generator.SetNextResolution(ports);
  WaitForServer(DEBUG_LOCATION, stub, 0);
  // Send RPCs. They should all go servers_[0]
  for (size_t i = 0; i < 10; ++i) CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(10, servers_[0]->service_.request_count());
  EXPECT_EQ(0, servers_[1]->service_.request_count());
  EXPECT_EQ(0, servers_[2]->service_.request_count());
  ResetCounters();
  // And now for the second server.
  LOG(INFO) << "*** SECOND BACKEND ***";
  ports.clear();
  ports.emplace_back(servers_[1]->port_);
  response_generator.SetNextResolution(ports);
  // Wait until update has been processed, as signaled by the second backend
  // receiving a request.
  EXPECT_EQ(0, servers_[1]->service_.request_count());
  WaitForServer(DEBUG_LOCATION, stub, 1);
  for (size_t i = 0; i < 10; ++i) CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(0, servers_[0]->service_.request_count());
  EXPECT_EQ(10, servers_[1]->service_.request_count());
  EXPECT_EQ(0, servers_[2]->service_.request_count());
  ResetCounters();
  // ... and for the last server.
  LOG(INFO) << "*** THIRD BACKEND ***";
  ports.clear();
  ports.emplace_back(servers_[2]->port_);
  response_generator.SetNextResolution(ports);
  WaitForServer(DEBUG_LOCATION, stub, 2);
  for (size_t i = 0; i < 10; ++i) CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(0, servers_[0]->service_.request_count());
  EXPECT_EQ(0, servers_[1]->service_.request_count());
  EXPECT_EQ(10, servers_[2]->service_.request_count());
  ResetCounters();
  // Back to all servers.
  LOG(INFO) << "*** ALL BACKENDS ***";
  ports.clear();
  ports.emplace_back(servers_[0]->port_);
  ports.emplace_back(servers_[1]->port_);
  ports.emplace_back(servers_[2]->port_);
  response_generator.SetNextResolution(ports);
  WaitForServers(DEBUG_LOCATION, stub);
  // Send three RPCs, one per server.
  for (size_t i = 0; i < 3; ++i) CheckRpcSendOk(DEBUG_LOCATION, stub);
  EXPECT_EQ(1, servers_[0]->service_.request_count());
  EXPECT_EQ(1, servers_[1]->service_.request_count());
  EXPECT_EQ(1, servers_[2]->service_.request_count());
  ResetCounters();
  // An empty update will result in the channel going into TRANSIENT_FAILURE.
  LOG(INFO) << "*** NO BACKENDS ***";
  ports.clear();
  response_generator.SetNextResolution(ports);
  WaitForChannelNotReady(channel.get());
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
      "empty address list \\(fake resolver empty address list\\)");
  servers_[0]->service_.ResetCounters();
  // Next update introduces servers_[1], making the channel recover.
  LOG(INFO) << "*** BACK TO SECOND BACKEND ***";
  ports.clear();
  ports.emplace_back(servers_[1]->port_);
  response_generator.SetNextResolution(ports);
  WaitForChannelReady(channel.get());
  WaitForServer(DEBUG_LOCATION, stub, 1);
  EXPECT_EQ(GRPC_CHANNEL_READY, channel->GetState(/*try_to_connect=*/false));
  // Check LB policy name for the channel.
  EXPECT_EQ("round_robin", channel->GetLoadBalancingPolicyName());
}

TEST_F(RoundRobinTest, UpdateInError) {
  StartServers(2);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  // Start with a single server.
  response_generator.SetNextResolution(GetServersPorts(0, 1));
  // Send RPCs. They should all go to server 0.
  for (size_t i = 0; i < 10; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub, /*wait_for_ready=*/false,
                   /*load_report=*/nullptr, /*timeout_ms=*/4000);
  }
  EXPECT_EQ(10, servers_[0]->service_.request_count());
  EXPECT_EQ(0, servers_[1]->service_.request_count());
  servers_[0]->service_.ResetCounters();
  // Send an update adding an unreachable server and server 1.
  std::vector<int> ports = {servers_[0]->port_, grpc_pick_unused_port_or_die(),
                            servers_[1]->port_};
  response_generator.SetNextResolution(ports);
  WaitForServers(DEBUG_LOCATION, stub, 0, 2, /*status_check=*/nullptr,
                 /*timeout=*/absl::Seconds(60));
  // Send a bunch more RPCs.  They should all succeed and should be
  // split evenly between the two servers.
  // Note: The split may be slightly uneven because of an extra picker
  // update that can happen if the subchannels for servers 0 and 1
  // report READY before the subchannel for the unreachable server
  // transitions from CONNECTING to TRANSIENT_FAILURE.
  for (size_t i = 0; i < 10; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub, /*wait_for_ready=*/false,
                   /*load_report=*/nullptr, /*timeout_ms=*/4000);
  }
  EXPECT_THAT(servers_[0]->service_.request_count(),
              ::testing::AllOf(::testing::Ge(4), ::testing::Le(6)));
  EXPECT_THAT(servers_[1]->service_.request_count(),
              ::testing::AllOf(::testing::Ge(4), ::testing::Le(6)));
  EXPECT_EQ(10, servers_[0]->service_.request_count() +
                    servers_[1]->service_.request_count());
}

TEST_F(RoundRobinTest, ManyUpdates) {
  // Start servers and send one RPC per server.
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  std::vector<int> ports = GetServersPorts();
  for (size_t i = 0; i < 1000; ++i) {
    std::shuffle(ports.begin(), ports.end(),
                 std::mt19937(std::random_device()()));
    response_generator.SetNextResolution(ports);
    if (i % 10 == 0) CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  // Check LB policy name for the channel.
  EXPECT_EQ("round_robin", channel->GetLoadBalancingPolicyName());
}

TEST_F(RoundRobinTest, ReresolveOnSubchannelConnectionFailure) {
  // Start 3 servers.
  StartServers(3);
  // Create channel.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  // Initially, tell the channel about only the first two servers.
  std::vector<int> ports = {servers_[0]->port_, servers_[1]->port_};
  response_generator.SetNextResolution(ports);
  // Wait for both servers to be seen.
  WaitForServers(DEBUG_LOCATION, stub, 0, 2);
  // Have server 0 send a GOAWAY.  This should trigger a re-resolution.
  LOG(INFO) << "****** SENDING GOAWAY FROM SERVER 0 *******";
  {
    grpc_core::ExecCtx exec_ctx;
    grpc_core::Server::FromC(servers_[0]->server_->c_server())->SendGoaways();
  }
  LOG(INFO) << "****** WAITING FOR RE-RESOLUTION REQUEST *******";
  EXPECT_TRUE(response_generator.Get()->WaitForReresolutionRequest(
      absl::Seconds(5 * grpc_test_slowdown_factor())));
  LOG(INFO) << "****** RE-RESOLUTION REQUEST SEEN *******";
  // Tell the fake resolver to send an update that adds the last server, but
  // only when the LB policy requests re-resolution.
  ports.push_back(servers_[2]->port_);
  response_generator.SetNextResolution(ports);
  // Wait for the client to see server 2.
  WaitForServer(DEBUG_LOCATION, stub, 2);
}

TEST_F(RoundRobinTest, FailsEmptyResolverUpdate) {
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  LOG(INFO) << "****** SENDING INITIAL RESOLVER RESULT *******";
  // Send a resolver result with an empty address list and a callback
  // that triggers a notification.
  grpc_core::Notification notification;
  grpc_core::Resolver::Result result;
  result.addresses.emplace();
  result.resolution_note = "injected error";
  result.result_health_callback = [&](absl::Status status) {
    EXPECT_EQ(status, absl::UnavailableError("empty address list"));
    notification.Notify();
  };
  response_generator.SetResponse(std::move(result));
  // Wait for channel to report TRANSIENT_FAILURE.
  LOG(INFO) << "****** TELLING CHANNEL TO CONNECT *******";
  auto predicate = [](grpc_connectivity_state state) {
    return state == GRPC_CHANNEL_TRANSIENT_FAILURE;
  };
  EXPECT_TRUE(
      WaitForChannelState(channel.get(), predicate, /*try_to_connect=*/true));
  // Callback should have been run.
  notification.WaitForNotification();
  // Make sure RPCs fail with the right status.
  CheckRpcSendFailure(DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
                      "empty address list \\(injected error\\)");
  // Return a valid address.
  LOG(INFO) << "****** SENDING NEXT RESOLVER RESULT *******";
  StartServers(1);
  response_generator.SetNextResolution(GetServersPorts());
  LOG(INFO) << "****** SENDING WAIT_FOR_READY RPC *******";
  CheckRpcSendOk(DEBUG_LOCATION, stub, /*wait_for_ready=*/true);
}

TEST_F(RoundRobinTest, TransientFailure) {
  // Start servers and create channel.  Channel should go to READY state.
  const int kNumServers = 3;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  EXPECT_TRUE(WaitForChannelReady(channel.get()));
  // Now kill the servers.  The channel should transition to TRANSIENT_FAILURE.
  for (size_t i = 0; i < servers_.size(); ++i) {
    servers_[i]->Shutdown();
  }
  auto predicate = [](grpc_connectivity_state state) {
    return state == GRPC_CHANNEL_TRANSIENT_FAILURE;
  };
  EXPECT_TRUE(WaitForChannelState(channel.get(), predicate));
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
      MakeConnectionFailureRegex("connections to all backends failing"));
}

TEST_F(RoundRobinTest, TransientFailureAtStartup) {
  // Create channel and return servers that don't exist.  Channel should
  // quickly transition into TRANSIENT_FAILURE.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution({
      grpc_pick_unused_port_or_die(),
      grpc_pick_unused_port_or_die(),
      grpc_pick_unused_port_or_die(),
  });
  for (size_t i = 0; i < servers_.size(); ++i) {
    servers_[i]->Shutdown();
  }
  auto predicate = [](grpc_connectivity_state state) {
    return state == GRPC_CHANNEL_TRANSIENT_FAILURE;
  };
  EXPECT_TRUE(WaitForChannelState(channel.get(), predicate, true));
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
      MakeConnectionFailureRegex("connections to all backends failing"));
}

TEST_F(RoundRobinTest, StaysInTransientFailureInSubsequentConnecting) {
  // Start connection injector.
  ConnectionAttemptInjector injector;
  // Get port.
  const int port = grpc_pick_unused_port_or_die();
  // Create channel.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution({port});
  // Allow first connection attempt to fail normally, and wait for
  // channel to report TRANSIENT_FAILURE.
  LOG(INFO) << "=== WAITING FOR CHANNEL TO REPORT TF ===";
  auto predicate = [](grpc_connectivity_state state) {
    return state == GRPC_CHANNEL_TRANSIENT_FAILURE;
  };
  EXPECT_TRUE(
      WaitForChannelState(channel.get(), predicate, /*try_to_connect=*/true));
  // Wait for next connection attempt to start.
  auto hold = injector.AddHold(port);
  hold->Wait();
  // Now the subchannel should be reporting CONNECTING.  Make sure the
  // channel is still in TRANSIENT_FAILURE and is still reporting the
  // right status.
  EXPECT_EQ(GRPC_CHANNEL_TRANSIENT_FAILURE, channel->GetState(false));
  // Send a few RPCs, just to give the channel a chance to propagate a
  // new picker, in case it was going to incorrectly do so.
  LOG(INFO) << "=== EXPECTING RPCs TO FAIL ===";
  for (size_t i = 0; i < 5; ++i) {
    CheckRpcSendFailure(
        DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
        MakeConnectionFailureRegex("connections to all backends failing"));
  }
  // Clean up.
  hold->Resume();
}

TEST_F(RoundRobinTest, ReportsLatestStatusInTransientFailure) {
  // Start connection injector.
  ConnectionAttemptInjector injector;
  // Get ports.
  const std::vector<int> ports = {grpc_pick_unused_port_or_die(),
                                  grpc_pick_unused_port_or_die()};
  // Create channel.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(ports);
  // Allow first connection attempts to fail normally, and check that
  // the RPC fails with the right status message.
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::UNAVAILABLE,
      MakeConnectionFailureRegex("connections to all backends failing"));
  // Now intercept the next connection attempt for each port.
  auto hold1 = injector.AddHold(ports[0]);
  auto hold2 = injector.AddHold(ports[1]);
  hold1->Wait();
  hold2->Wait();
  // Inject a custom failure message.
  hold1->Fail(GRPC_ERROR_CREATE("Survey says... Bzzzzt!"));
  // Wait until RPC fails with the right message.
  absl::Time deadline =
      absl::Now() + (absl::Seconds(5) * grpc_test_slowdown_factor());
  while (true) {
    Status status = SendRpc(stub);
    EXPECT_EQ(StatusCode::UNAVAILABLE, status.error_code());
    if (::testing::Matches(::testing::MatchesRegex(
            "connections to all backends failing; last error: "
            "UNKNOWN: (ipv6:%5B::1%5D|ipv4:127.0.0.1):[0-9]+: "
            "Survey says... Bzzzzt!"))(status.error_message())) {
      break;
    }
    LOG(INFO) << "STATUS MESSAGE: " << status.error_message();
    EXPECT_THAT(status.error_message(),
                ::testing::MatchesRegex(MakeConnectionFailureRegex(
                    "connections to all backends failing")));
    EXPECT_LT(absl::Now(), deadline);
    if (absl::Now() >= deadline) break;
  }
  // Clean up.
  hold2->Resume();
}

TEST_F(RoundRobinTest, DoesNotFailRpcsUponDisconnection) {
  // Start connection injector.
  ConnectionAttemptInjector injector;
  // Start server.
  StartServers(1);
  // Create channel.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Start a thread constantly sending RPCs in a loop.
  LOG(INFO) << "=== STARTING CLIENT THREAD ===";
  std::atomic<bool> shutdown{false};
  gpr_event ev;
  gpr_event_init(&ev);
  std::thread thd([&]() {
    LOG(INFO) << "sending first RPC";
    CheckRpcSendOk(DEBUG_LOCATION, stub);
    gpr_event_set(&ev, reinterpret_cast<void*>(1));
    while (!shutdown.load()) {
      LOG(INFO) << "sending RPC";
      CheckRpcSendOk(DEBUG_LOCATION, stub);
    }
  });
  // Wait for first RPC to complete.
  LOG(INFO) << "=== WAITING FOR FIRST RPC TO COMPLETE ===";
  ASSERT_EQ(reinterpret_cast<void*>(1),
            gpr_event_wait(&ev, grpc_timeout_seconds_to_deadline(1)));
  // Channel should now be READY.
  ASSERT_EQ(GRPC_CHANNEL_READY, channel->GetState(false));
  // Tell injector to intercept the next connection attempt.
  auto hold1 =
      injector.AddHold(servers_[0]->port_, /*intercept_completion=*/true);
  // Now kill the server.  The subchannel should report IDLE and be
  // immediately reconnected to, but this should not cause any test
  // failures.
  LOG(INFO) << "=== SHUTTING DOWN SERVER ===";
  {
    grpc_core::ExecCtx exec_ctx;
    grpc_core::Server::FromC(servers_[0]->server_->c_server())->SendGoaways();
  }
  gpr_sleep_until(grpc_timeout_seconds_to_deadline(1));
  servers_[0]->Shutdown();
  // Wait for next attempt to start.
  LOG(INFO) << "=== WAITING FOR RECONNECTION ATTEMPT ===";
  hold1->Wait();
  // Start server and allow attempt to continue.
  LOG(INFO) << "=== RESTARTING SERVER ===";
  StartServer(0);
  hold1->Resume();
  // Wait for next attempt to complete.
  LOG(INFO) << "=== WAITING FOR RECONNECTION ATTEMPT TO COMPLETE ===";
  hold1->WaitForCompletion();
  // Now shut down the thread.
  LOG(INFO) << "=== SHUTTING DOWN CLIENT THREAD ===";
  shutdown.store(true);
  thd.join();
}

TEST_F(RoundRobinTest, SingleReconnect) {
  const int kNumServers = 3;
  StartServers(kNumServers);
  const auto ports = GetServersPorts();
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(ports);
  WaitForServers(DEBUG_LOCATION, stub);
  // Sync to end of list.
  WaitForServer(DEBUG_LOCATION, stub, servers_.size() - 1);
  for (size_t i = 0; i < servers_.size(); ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
    EXPECT_EQ(1, servers_[i]->service_.request_count()) << "for backend #" << i;
  }
  // One request should have gone to each server.
  for (size_t i = 0; i < servers_.size(); ++i) {
    EXPECT_EQ(1, servers_[i]->service_.request_count());
  }
  // Kill the first server.
  servers_[0]->StopListeningAndSendGoaways();
  // Wait for client to notice that the backend is down.  We know that's
  // happened when we see kNumServers RPCs that do not go to backend 0.
  ResetCounters();
  SendRpcsUntil(
      DEBUG_LOCATION, stub,
      [&, num_rpcs_not_on_backend_0 = 0](const Status& status) mutable {
        EXPECT_TRUE(status.ok()) << "code=" << status.error_code()
                                 << " message=" << status.error_message();
        if (servers_[0]->service_.request_count() == 1) {
          num_rpcs_not_on_backend_0 = 0;
        } else {
          ++num_rpcs_not_on_backend_0;
        }
        ResetCounters();
        return num_rpcs_not_on_backend_0 < kNumServers;
      });
  // Send a bunch of RPCs.
  for (int i = 0; i < 10 * kNumServers; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  // No requests have gone to the deceased server.
  EXPECT_EQ(0UL, servers_[0]->service_.request_count());
  // Bring the first server back up.
  servers_[0]->Shutdown();
  StartServer(0);
  // Requests should start arriving at the first server either right away (if
  // the server managed to start before the RR policy retried the subchannel) or
  // after the subchannel retry delay otherwise (RR's subchannel retried before
  // the server was fully back up).
  WaitForServer(DEBUG_LOCATION, stub, 0);
}

// If health checking is required by client but health checking service
// is not running on the server, the channel should be treated as healthy.
TEST_F(RoundRobinTest, ServersHealthCheckingUnimplementedTreatedAsHealthy) {
  StartServers(1);  // Single server
  ChannelArguments args;
  args.SetServiceConfigJSON(
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name\"}}");
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution({servers_[0]->port_});
  EXPECT_TRUE(WaitForChannelReady(channel.get()));
  CheckRpcSendOk(DEBUG_LOCATION, stub);
}

TEST_F(RoundRobinTest, HealthChecking) {
  EnableDefaultHealthCheckService(true);
  // Start servers.
  const int kNumServers = 3;
  StartServers(kNumServers);
  ChannelArguments args;
  args.SetServiceConfigJSON(
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name\"}}");
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Channel should not become READY, because health checks should be failing.
  LOG(INFO)
      << "*** initial state: unknown health check service name for all servers";
  EXPECT_FALSE(WaitForChannelReady(channel.get(), 1));
  // Now set one of the servers to be healthy.
  // The channel should become healthy and all requests should go to
  // the healthy server.
  LOG(INFO) << "*** server 0 healthy";
  servers_[0]->SetServingStatus("health_check_service_name", true);
  EXPECT_TRUE(WaitForChannelReady(channel.get()));
  // New channel state may be reported before the picker is updated, so
  // wait for the server before proceeding.
  WaitForServer(DEBUG_LOCATION, stub, 0);
  for (int i = 0; i < 10; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  EXPECT_EQ(10, servers_[0]->service_.request_count());
  EXPECT_EQ(0, servers_[1]->service_.request_count());
  EXPECT_EQ(0, servers_[2]->service_.request_count());
  // Now set a second server to be healthy.
  LOG(INFO) << "*** server 2 healthy";
  servers_[2]->SetServingStatus("health_check_service_name", true);
  WaitForServer(DEBUG_LOCATION, stub, 2);
  for (int i = 0; i < 10; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  EXPECT_EQ(5, servers_[0]->service_.request_count());
  EXPECT_EQ(0, servers_[1]->service_.request_count());
  EXPECT_EQ(5, servers_[2]->service_.request_count());
  // Now set the remaining server to be healthy.
  LOG(INFO) << "*** server 1 healthy";
  servers_[1]->SetServingStatus("health_check_service_name", true);
  WaitForServer(DEBUG_LOCATION, stub, 1);
  for (int i = 0; i < 9; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  EXPECT_EQ(3, servers_[0]->service_.request_count());
  EXPECT_EQ(3, servers_[1]->service_.request_count());
  EXPECT_EQ(3, servers_[2]->service_.request_count());
  // Now set one server to be unhealthy again.  Then wait until the
  // unhealthiness has hit the client.  We know that the client will see
  // this when we send kNumServers requests and one of the remaining servers
  // sees two of the requests.
  LOG(INFO) << "*** server 0 unhealthy";
  servers_[0]->SetServingStatus("health_check_service_name", false);
  do {
    ResetCounters();
    for (int i = 0; i < kNumServers; ++i) {
      CheckRpcSendOk(DEBUG_LOCATION, stub);
    }
  } while (servers_[1]->service_.request_count() != 2 &&
           servers_[2]->service_.request_count() != 2);
  // Now set the remaining two servers to be unhealthy.  Make sure the
  // channel leaves READY state and that RPCs fail.
  LOG(INFO) << "*** all servers unhealthy";
  servers_[1]->SetServingStatus("health_check_service_name", false);
  servers_[2]->SetServingStatus("health_check_service_name", false);
  EXPECT_TRUE(WaitForChannelNotReady(channel.get()));
  // New channel state may be reported before the picker is updated, so
  // one or two more RPCs may succeed before we see a failure.
  SendRpcsUntil(DEBUG_LOCATION, stub, [&](const Status& status) {
    if (status.ok()) return true;
    EXPECT_EQ(status.error_code(), StatusCode::UNAVAILABLE);
    EXPECT_THAT(
        status.error_message(),
        ::testing::MatchesRegex(
            "connections to all backends failing; last error: "
            "(ipv6:%5B::1%5D|ipv4:127.0.0.1):[0-9]+: backend unhealthy"));
    return false;
  });
  // Clean up.
  EnableDefaultHealthCheckService(false);
}

TEST_F(RoundRobinTest, HealthCheckingHandlesSubchannelFailure) {
  EnableDefaultHealthCheckService(true);
  // Start servers.
  const int kNumServers = 3;
  StartServers(kNumServers);
  servers_[0]->SetServingStatus("health_check_service_name", true);
  servers_[1]->SetServingStatus("health_check_service_name", true);
  servers_[2]->SetServingStatus("health_check_service_name", true);
  ChannelArguments args;
  args.SetServiceConfigJSON(
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name\"}}");
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  WaitForServer(DEBUG_LOCATION, stub, 0);
  // Stop server 0 and send a new resolver result to ensure that RR
  // checks each subchannel's state.
  servers_[0]->StopListeningAndSendGoaways();
  response_generator.SetNextResolution(GetServersPorts());
  // Send a bunch more RPCs.
  for (size_t i = 0; i < 100; i++) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
}

TEST_F(RoundRobinTest, WithHealthCheckingInhibitPerChannel) {
  EnableDefaultHealthCheckService(true);
  // Start server.
  const int kNumServers = 1;
  StartServers(kNumServers);
  // Use the same channel creds for both channels, so that they have the same
  // subchannel keys.
  auto channel_creds =
      std::make_shared<FakeTransportSecurityChannelCredentials>();
  // Create a channel with health-checking enabled.
  ChannelArguments args;
  args.SetServiceConfigJSON(
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name\"}}");
  FakeResolverResponseGeneratorWrapper response_generator1;
  auto channel1 =
      BuildChannel("round_robin", response_generator1, args, channel_creds);
  auto stub1 = BuildStub(channel1);
  std::vector<int> ports = GetServersPorts();
  response_generator1.SetNextResolution(ports);
  // Create a channel with health checking enabled but inhibited.
  args.SetInt(GRPC_ARG_INHIBIT_HEALTH_CHECKING, 1);
  FakeResolverResponseGeneratorWrapper response_generator2;
  auto channel2 =
      BuildChannel("round_robin", response_generator2, args, channel_creds);
  auto stub2 = BuildStub(channel2);
  response_generator2.SetNextResolution(ports);
  // First channel should not become READY, because health checks should be
  // failing.
  EXPECT_FALSE(WaitForChannelReady(channel1.get(), 1));
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub1, StatusCode::UNAVAILABLE,
      "connections to all backends failing; last error: "
      "(ipv6:%5B::1%5D|ipv4:127.0.0.1):[0-9]+: backend unhealthy");
  // Second channel should be READY.
  EXPECT_TRUE(WaitForChannelReady(channel2.get(), 1));
  CheckRpcSendOk(DEBUG_LOCATION, stub2);
  // Enable health checks on the backend and wait for channel 1 to succeed.
  servers_[0]->SetServingStatus("health_check_service_name", true);
  CheckRpcSendOk(DEBUG_LOCATION, stub1, true /* wait_for_ready */);
  // Check that we created only one subchannel to the backend.
  EXPECT_EQ(1UL, servers_[0]->service_.clients().size());
  // Clean up.
  EnableDefaultHealthCheckService(false);
}

TEST_F(RoundRobinTest, HealthCheckingServiceNamePerChannel) {
  EnableDefaultHealthCheckService(true);
  // Start server.
  const int kNumServers = 1;
  StartServers(kNumServers);
  // Use the same channel creds for both channels, so that they have the same
  // subchannel keys.
  auto channel_creds =
      std::make_shared<FakeTransportSecurityChannelCredentials>();
  // Create a channel with health-checking enabled.
  ChannelArguments args;
  args.SetServiceConfigJSON(
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name\"}}");
  FakeResolverResponseGeneratorWrapper response_generator1;
  auto channel1 =
      BuildChannel("round_robin", response_generator1, args, channel_creds);
  auto stub1 = BuildStub(channel1);
  std::vector<int> ports = GetServersPorts();
  response_generator1.SetNextResolution(ports);
  // Create a channel with health-checking enabled with a different
  // service name.
  ChannelArguments args2;
  args2.SetServiceConfigJSON(
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name2\"}}");
  FakeResolverResponseGeneratorWrapper response_generator2;
  auto channel2 =
      BuildChannel("round_robin", response_generator2, args2, channel_creds);
  auto stub2 = BuildStub(channel2);
  response_generator2.SetNextResolution(ports);
  // Allow health checks from channel 2 to succeed.
  servers_[0]->SetServingStatus("health_check_service_name2", true);
  // First channel should not become READY, because health checks should be
  // failing.
  EXPECT_FALSE(WaitForChannelReady(channel1.get(), 1));
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub1, StatusCode::UNAVAILABLE,
      "connections to all backends failing; last error: "
      "(ipv6:%5B::1%5D|ipv4:127.0.0.1):[0-9]+: backend unhealthy");
  // Second channel should be READY.
  EXPECT_TRUE(WaitForChannelReady(channel2.get(), 1));
  CheckRpcSendOk(DEBUG_LOCATION, stub2);
  // Enable health checks for channel 1 and wait for it to succeed.
  servers_[0]->SetServingStatus("health_check_service_name", true);
  CheckRpcSendOk(DEBUG_LOCATION, stub1, true /* wait_for_ready */);
  // Check that we created only one subchannel to the backend.
  EXPECT_EQ(1UL, servers_[0]->service_.clients().size());
  // Clean up.
  EnableDefaultHealthCheckService(false);
}

TEST_F(RoundRobinTest,
       HealthCheckingServiceNameChangesAfterSubchannelsCreated) {
  EnableDefaultHealthCheckService(true);
  // Start server.
  const int kNumServers = 1;
  StartServers(kNumServers);
  // Create a channel with health-checking enabled.
  const char* kServiceConfigJson =
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name\"}}";
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator);
  auto stub = BuildStub(channel);
  std::vector<int> ports = GetServersPorts();
  response_generator.SetNextResolution(ports, kServiceConfigJson);
  servers_[0]->SetServingStatus("health_check_service_name", true);
  EXPECT_TRUE(WaitForChannelReady(channel.get(), 1 /* timeout_seconds */));
  // Send an update on the channel to change it to use a health checking
  // service name that is not being reported as healthy.
  const char* kServiceConfigJson2 =
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name2\"}}";
  response_generator.SetNextResolution(ports, kServiceConfigJson2);
  EXPECT_TRUE(WaitForChannelNotReady(channel.get()));
  // Clean up.
  EnableDefaultHealthCheckService(false);
}

TEST_F(RoundRobinTest, HealthCheckingRetryOnStreamEnd) {
  // Start servers.
  const int kNumServers = 2;
  CreateServers(kNumServers);
  EnableNoopHealthCheckService();
  StartServer(0);
  StartServer(1);
  ChannelArguments args;
  // Create a channel with health-checking enabled.
  args.SetServiceConfigJSON(
      "{\"healthCheckConfig\": "
      "{\"serviceName\": \"health_check_service_name\"}}");
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("round_robin", response_generator, args);
  response_generator.SetNextResolution(GetServersPorts());
  EXPECT_FALSE(WaitForChannelReady(channel.get()));
  EXPECT_GT(servers_[0]->noop_health_check_service_impl_.request_count(), 1);
  EXPECT_GT(servers_[1]->noop_health_check_service_impl_.request_count(), 1);
}

//
// LB policy pick args
//

class ClientLbPickArgsTest : public ClientLbEnd2endTest {
 protected:
  void SetUp() override {
    ClientLbEnd2endTest::SetUp();
    current_test_instance_ = this;
  }

  static void SetUpTestSuite() {
    grpc_core::CoreConfiguration::Reset();
    grpc_core::CoreConfiguration::RegisterEphemeralBuilder(
        [](grpc_core::CoreConfiguration::Builder* builder) {
          grpc_core::RegisterTestPickArgsLoadBalancingPolicy(builder,
                                                             SavePickArgs);
        });
    grpc_init();
  }

  static void TearDownTestSuite() {
    grpc_shutdown();
    grpc_core::CoreConfiguration::Reset();
  }

  std::vector<grpc_core::PickArgsSeen> args_seen_list() {
    grpc_core::MutexLock lock(&mu_);
    return args_seen_list_;
  }

  static std::string ArgsSeenListString(
      const std::vector<grpc_core::PickArgsSeen>& args_seen_list) {
    std::vector<std::string> entries;
    for (const auto& args_seen : args_seen_list) {
      std::vector<std::string> metadata;
      for (const auto& [key, value] : args_seen.metadata) {
        metadata.push_back(absl::StrCat(key, "=", value));
      }
      entries.push_back(absl::StrFormat("{path=\"%s\", metadata=[%s]}",
                                        args_seen.path,
                                        absl::StrJoin(metadata, ", ")));
    }
    return absl::StrCat("[", absl::StrJoin(entries, ", "), "]");
  }

 private:
  static void SavePickArgs(const grpc_core::PickArgsSeen& args_seen) {
    ClientLbPickArgsTest* self = current_test_instance_;
    grpc_core::MutexLock lock(&self->mu_);
    self->args_seen_list_.emplace_back(args_seen);
  }

  static ClientLbPickArgsTest* current_test_instance_;
  grpc_core::Mutex mu_;
  std::vector<grpc_core::PickArgsSeen> args_seen_list_;
};

ClientLbPickArgsTest* ClientLbPickArgsTest::current_test_instance_ = nullptr;

TEST_F(ClientLbPickArgsTest, Basic) {
  const int kNumServers = 1;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("test_pick_args_lb", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Proactively connect the channel, so that the LB policy will always
  // be connected before it sees the pick.  Otherwise, the test would be
  // flaky because sometimes the pick would be seen twice (once in
  // CONNECTING and again in READY) and other times only once (in READY).
  ASSERT_TRUE(channel->WaitForConnected(gpr_inf_future(GPR_CLOCK_MONOTONIC)));
  // Check LB policy name for the channel.
  EXPECT_EQ("test_pick_args_lb", channel->GetLoadBalancingPolicyName());
  // Now send an RPC and check that the picker sees the expected data.
  CheckRpcSendOk(DEBUG_LOCATION, stub, /*wait_for_ready=*/true);
  auto pick_args_seen_list = args_seen_list();
  EXPECT_THAT(pick_args_seen_list,
              ::testing::ElementsAre(::testing::AllOf(
                  ::testing::Field(&grpc_core::PickArgsSeen::path,
                                   "/grpc.testing.EchoTestService/Echo"),
                  ::testing::Field(&grpc_core::PickArgsSeen::metadata,
                                   ::testing::UnorderedElementsAre(
                                       ::testing::Pair("foo", "1"),
                                       ::testing::Pair("bar", "2"),
                                       ::testing::Pair("baz", "3"))))))
      << ArgsSeenListString(pick_args_seen_list);
}

//
// tests that LB policies can get the call's trailing metadata
//

class OrcaLoadReportBuilder {
 public:
  OrcaLoadReportBuilder() = default;
  explicit OrcaLoadReportBuilder(const OrcaLoadReport& report)
      : report_(report) {}
  OrcaLoadReportBuilder& SetApplicationUtilization(double v) {
    report_.set_application_utilization(v);
    return *this;
  }
  OrcaLoadReportBuilder& SetCpuUtilization(double v) {
    report_.set_cpu_utilization(v);
    return *this;
  }
  OrcaLoadReportBuilder& SetMemUtilization(double v) {
    report_.set_mem_utilization(v);
    return *this;
  }
  OrcaLoadReportBuilder& SetQps(double v) {
    report_.set_rps_fractional(v);
    return *this;
  }
  OrcaLoadReportBuilder& SetEps(double v) {
    report_.set_eps(v);
    return *this;
  }
  OrcaLoadReportBuilder& SetRequestCost(absl::string_view n, double v) {
    (*report_.mutable_request_cost())[n] = v;
    return *this;
  }
  OrcaLoadReportBuilder& SetUtilization(absl::string_view n, double v) {
    (*report_.mutable_utilization())[n] = v;
    return *this;
  }
  OrcaLoadReportBuilder& SetNamedMetrics(absl::string_view n, double v) {
    (*report_.mutable_named_metrics())[n] = v;
    return *this;
  }
  OrcaLoadReport Build() { return std::move(report_); }

 private:
  OrcaLoadReport report_;
};

OrcaLoadReport BackendMetricDataToOrcaLoadReport(
    const grpc_core::BackendMetricData& backend_metric_data) {
  auto builder = OrcaLoadReportBuilder()
                     .SetApplicationUtilization(
                         backend_metric_data.application_utilization)
                     .SetCpuUtilization(backend_metric_data.cpu_utilization)
                     .SetMemUtilization(backend_metric_data.mem_utilization)
                     .SetQps(backend_metric_data.qps)
                     .SetEps(backend_metric_data.eps);
  for (const auto& [key, value] : backend_metric_data.request_cost) {
    builder.SetRequestCost(std::string(key), value);
  }
  for (const auto& [key, value] : backend_metric_data.utilization) {
    builder.SetUtilization(std::string(key), value);
  }
  for (const auto& [key, value] : backend_metric_data.named_metrics) {
    builder.SetNamedMetrics(std::string(key), value);
  }
  return builder.Build();
}

// TODO(roth): Change this to use EqualsProto() once that becomes available in
// OSS.
void CheckLoadReportAsExpected(const OrcaLoadReport& actual,
                               const OrcaLoadReport& expected) {
  EXPECT_EQ(actual.application_utilization(),
            expected.application_utilization());
  EXPECT_EQ(actual.cpu_utilization(), expected.cpu_utilization());
  EXPECT_EQ(actual.mem_utilization(), expected.mem_utilization());
  EXPECT_EQ(actual.rps_fractional(), expected.rps_fractional());
  EXPECT_EQ(actual.eps(), expected.eps());
  EXPECT_EQ(actual.request_cost().size(), expected.request_cost().size());
  for (const auto& [key, value] : actual.request_cost()) {
    auto it = expected.request_cost().find(key);
    ASSERT_NE(it, expected.request_cost().end());
    EXPECT_EQ(it->second, value);
  }
  EXPECT_EQ(actual.utilization().size(), expected.utilization().size());
  for (const auto& [key, value] : actual.utilization()) {
    auto it = expected.utilization().find(key);
    ASSERT_NE(it, expected.utilization().end());
    EXPECT_EQ(it->second, value);
  }
  EXPECT_EQ(actual.named_metrics().size(), expected.named_metrics().size());
  for (const auto& [key, value] : actual.named_metrics()) {
    auto it = expected.named_metrics().find(key);
    ASSERT_NE(it, expected.named_metrics().end());
    EXPECT_EQ(it->second, value);
  }
}

class ClientLbInterceptTrailingMetadataTest : public ClientLbEnd2endTest {
 protected:
  void SetUp() override {
    ClientLbEnd2endTest::SetUp();
    current_test_instance_ = this;
  }

  static void SetUpTestSuite() {
    grpc_core::CoreConfiguration::Reset();
    grpc_core::CoreConfiguration::RegisterEphemeralBuilder(
        [](grpc_core::CoreConfiguration::Builder* builder) {
          grpc_core::RegisterInterceptRecvTrailingMetadataLoadBalancingPolicy(
              builder, ReportTrailerIntercepted);
        });
    grpc_init();
  }

  static void TearDownTestSuite() {
    grpc_shutdown();
    grpc_core::CoreConfiguration::Reset();
  }

  int num_trailers_intercepted() {
    grpc_core::MutexLock lock(&mu_);
    return num_trailers_intercepted_;
  }

  absl::Status last_status() {
    grpc_core::MutexLock lock(&mu_);
    return last_status_;
  }

  grpc_core::MetadataVector trailing_metadata() {
    grpc_core::MutexLock lock(&mu_);
    return std::move(trailing_metadata_);
  }

  std::optional<OrcaLoadReport> backend_load_report() {
    grpc_core::MutexLock lock(&mu_);
    return std::move(load_report_);
  }

  // Returns true if received callback within deadline.
  bool WaitForLbCallback() {
    grpc_core::MutexLock lock(&mu_);
    while (!trailer_intercepted_) {
      if (cond_.WaitWithTimeout(&mu_, absl::Seconds(3))) return false;
    }
    trailer_intercepted_ = false;
    return true;
  }

  void RunPerRpcMetricReportingTest(const OrcaLoadReport& reported,
                                    const OrcaLoadReport& expected) {
    const int kNumServers = 1;
    const int kNumRpcs = 10;
    StartServers(kNumServers);
    FakeResolverResponseGeneratorWrapper response_generator;
    auto channel =
        BuildChannel("intercept_trailing_metadata_lb", response_generator);
    auto stub = BuildStub(channel);
    response_generator.SetNextResolution(GetServersPorts());
    for (size_t i = 0; i < kNumRpcs; ++i) {
      CheckRpcSendOk(DEBUG_LOCATION, stub, false, &reported);
      auto actual = backend_load_report();
      ASSERT_TRUE(actual.has_value());
      CheckLoadReportAsExpected(*actual, expected);
    }
    // Check LB policy name for the channel.
    EXPECT_EQ("intercept_trailing_metadata_lb",
              channel->GetLoadBalancingPolicyName());
    EXPECT_EQ(kNumRpcs, num_trailers_intercepted());
  }

 private:
  static void ReportTrailerIntercepted(
      const grpc_core::TrailingMetadataArgsSeen& args_seen) {
    const auto* backend_metric_data = args_seen.backend_metric_data;
    ClientLbInterceptTrailingMetadataTest* self = current_test_instance_;
    grpc_core::MutexLock lock(&self->mu_);
    self->last_status_ = args_seen.status;
    self->num_trailers_intercepted_++;
    self->trailer_intercepted_ = true;
    self->trailing_metadata_ = args_seen.metadata;
    if (backend_metric_data != nullptr) {
      self->load_report_ =
          BackendMetricDataToOrcaLoadReport(*backend_metric_data);
    }
    self->cond_.Signal();
  }

  static ClientLbInterceptTrailingMetadataTest* current_test_instance_;
  int num_trailers_intercepted_ = 0;
  bool trailer_intercepted_ = false;
  grpc_core::Mutex mu_;
  grpc_core::CondVar cond_;
  absl::Status last_status_;
  grpc_core::MetadataVector trailing_metadata_;
  std::optional<OrcaLoadReport> load_report_;
};

ClientLbInterceptTrailingMetadataTest*
    ClientLbInterceptTrailingMetadataTest::current_test_instance_ = nullptr;

TEST_F(ClientLbInterceptTrailingMetadataTest, StatusOk) {
  StartServers(1);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel =
      BuildChannel("intercept_trailing_metadata_lb", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an OK RPC.
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  // Check LB policy name for the channel.
  EXPECT_EQ("intercept_trailing_metadata_lb",
            channel->GetLoadBalancingPolicyName());
  EXPECT_EQ(1, num_trailers_intercepted());
  EXPECT_EQ(absl::OkStatus(), last_status());
}

TEST_F(ClientLbInterceptTrailingMetadataTest, StatusFailed) {
  StartServers(1);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel =
      BuildChannel("intercept_trailing_metadata_lb", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  EchoRequest request;
  auto* expected_error = request.mutable_param()->mutable_expected_error();
  expected_error->set_code(GRPC_STATUS_PERMISSION_DENIED);
  expected_error->set_error_message("bummer, man");
  Status status = SendRpc(stub, /*response=*/nullptr, /*timeout_ms=*/1000,
                          /*wait_for_ready=*/false, &request);
  EXPECT_EQ(status.error_code(), StatusCode::PERMISSION_DENIED);
  EXPECT_EQ(status.error_message(), "bummer, man");
  absl::Status status_seen_by_lb = last_status();
  EXPECT_EQ(status_seen_by_lb.code(), absl::StatusCode::kPermissionDenied);
  EXPECT_EQ(status_seen_by_lb.message(), "bummer, man");
}

TEST_F(ClientLbInterceptTrailingMetadataTest,
       StatusCancelledWithoutStartingRecvTrailingMetadata) {
  StartServers(1);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel =
      BuildChannel("intercept_trailing_metadata_lb", response_generator);
  response_generator.SetNextResolution(GetServersPorts());
  auto stub = BuildStub(channel);
  {
    // Start a stream (sends initial metadata) and then cancel without
    // calling Finish().
    ClientContext ctx;
    auto stream = stub->BidiStream(&ctx);
    ctx.TryCancel();
  }
  // Wait for stream to be cancelled.
  ASSERT_TRUE(WaitForLbCallback());
  // Check status seen by LB policy.
  EXPECT_EQ(1, num_trailers_intercepted());
  absl::Status status_seen_by_lb = last_status();
  EXPECT_EQ(status_seen_by_lb.code(), absl::StatusCode::kCancelled);
  EXPECT_EQ(status_seen_by_lb.message(), "call cancelled");
}

TEST_F(ClientLbInterceptTrailingMetadataTest, InterceptsRetriesDisabled) {
  const int kNumServers = 1;
  const int kNumRpcs = 10;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  ChannelArguments channel_args;
  channel_args.SetInt(GRPC_ARG_ENABLE_RETRIES, 0);
  auto channel = BuildChannel("intercept_trailing_metadata_lb",
                              response_generator, channel_args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  for (size_t i = 0; i < kNumRpcs; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  // Check LB policy name for the channel.
  EXPECT_EQ("intercept_trailing_metadata_lb",
            channel->GetLoadBalancingPolicyName());
  EXPECT_EQ(kNumRpcs, num_trailers_intercepted());
  EXPECT_THAT(trailing_metadata(),
              ::testing::UnorderedElementsAre(
                  // TODO(roth): Should grpc-status be visible here?
                  ::testing::Pair("grpc-status", "0"),
                  ::testing::Pair("user-agent", ::testing::_),
                  ::testing::Pair("foo", "1"), ::testing::Pair("bar", "2"),
                  ::testing::Pair("baz", "3")));
  EXPECT_FALSE(backend_load_report().has_value());
}

TEST_F(ClientLbInterceptTrailingMetadataTest, InterceptsRetriesEnabled) {
  const int kNumServers = 1;
  const int kNumRpcs = 10;
  StartServers(kNumServers);
  ChannelArguments args;
  args.SetServiceConfigJSON(
      "{\n"
      "  \"methodConfig\": [ {\n"
      "    \"name\": [\n"
      "      { \"service\": \"grpc.testing.EchoTestService\" }\n"
      "    ],\n"
      "    \"retryPolicy\": {\n"
      "      \"maxAttempts\": 3,\n"
      "      \"initialBackoff\": \"1s\",\n"
      "      \"maxBackoff\": \"120s\",\n"
      "      \"backoffMultiplier\": 1.6,\n"
      "      \"retryableStatusCodes\": [ \"ABORTED\" ]\n"
      "    }\n"
      "  } ]\n"
      "}");
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel =
      BuildChannel("intercept_trailing_metadata_lb", response_generator, args);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  for (size_t i = 0; i < kNumRpcs; ++i) {
    CheckRpcSendOk(DEBUG_LOCATION, stub);
  }
  // Check LB policy name for the channel.
  EXPECT_EQ("intercept_trailing_metadata_lb",
            channel->GetLoadBalancingPolicyName());
  EXPECT_EQ(kNumRpcs, num_trailers_intercepted());
  EXPECT_THAT(trailing_metadata(),
              ::testing::UnorderedElementsAre(
                  // TODO(roth): Should grpc-status be visible here?
                  ::testing::Pair("grpc-status", "0"),
                  ::testing::Pair("user-agent", ::testing::_),
                  ::testing::Pair("foo", "1"), ::testing::Pair("bar", "2"),
                  ::testing::Pair("baz", "3")));
  EXPECT_FALSE(backend_load_report().has_value());
}

TEST_F(ClientLbInterceptTrailingMetadataTest, Valid) {
  RunPerRpcMetricReportingTest(OrcaLoadReportBuilder()
                                   .SetApplicationUtilization(0.25)
                                   .SetCpuUtilization(0.5)
                                   .SetMemUtilization(0.75)
                                   .SetQps(0.25)
                                   .SetEps(0.1)
                                   .SetRequestCost("foo", -0.8)
                                   .SetRequestCost("bar", 1.4)
                                   .SetUtilization("baz", 1.0)
                                   .SetUtilization("quux", 0.9)
                                   .SetNamedMetrics("metric0", 3.0)
                                   .SetNamedMetrics("metric1", -1.0)
                                   .Build(),
                               OrcaLoadReportBuilder()
                                   .SetApplicationUtilization(0.25)
                                   .SetCpuUtilization(0.5)
                                   .SetMemUtilization(0.75)
                                   .SetQps(0.25)
                                   .SetEps(0.1)
                                   .SetRequestCost("foo", -0.8)
                                   .SetRequestCost("bar", 1.4)
                                   .SetUtilization("baz", 1.0)
                                   .SetUtilization("quux", 0.9)
                                   .SetNamedMetrics("metric0", 3.0)
                                   .SetNamedMetrics("metric1", -1.0)
                                   .Build());
}

TEST_F(ClientLbInterceptTrailingMetadataTest, NegativeValues) {
  RunPerRpcMetricReportingTest(OrcaLoadReportBuilder()
                                   .SetApplicationUtilization(-0.3)
                                   .SetCpuUtilization(-0.1)
                                   .SetMemUtilization(-0.2)
                                   .SetQps(-3)
                                   .SetEps(-4)
                                   .SetRequestCost("foo", -5)
                                   .SetUtilization("bar", -0.6)
                                   .SetNamedMetrics("baz", -0.7)
                                   .Build(),
                               OrcaLoadReportBuilder()
                                   .SetRequestCost("foo", -5)
                                   .SetNamedMetrics("baz", -0.7)
                                   .Build());
}

TEST_F(ClientLbInterceptTrailingMetadataTest, AboveOneUtilization) {
  RunPerRpcMetricReportingTest(OrcaLoadReportBuilder()
                                   .SetApplicationUtilization(1.9)
                                   .SetCpuUtilization(1.1)
                                   .SetMemUtilization(2)
                                   .SetQps(3)
                                   .SetEps(4)
                                   .SetUtilization("foo", 5)
                                   .Build(),
                               OrcaLoadReportBuilder()
                                   .SetApplicationUtilization(1.9)
                                   .SetCpuUtilization(1.1)
                                   .SetQps(3)
                                   .SetEps(4)
                                   .Build());
}

TEST_F(ClientLbInterceptTrailingMetadataTest, BackendMetricDataMerge) {
  const int kNumServers = 1;
  const int kNumRpcs = 10;
  StartServers(kNumServers);
  servers_[0]->server_metric_recorder_->SetApplicationUtilization(0.99);
  servers_[0]->server_metric_recorder_->SetCpuUtilization(0.99);
  servers_[0]->server_metric_recorder_->SetMemoryUtilization(0.99);
  servers_[0]->server_metric_recorder_->SetQps(0.99);
  servers_[0]->server_metric_recorder_->SetEps(0.99);
  servers_[0]->server_metric_recorder_->SetNamedUtilization("foo", 0.99);
  servers_[0]->server_metric_recorder_->SetNamedUtilization("bar", 0.1);
  OrcaLoadReport per_server_load = OrcaLoadReportBuilder()
                                       .SetApplicationUtilization(0.99)
                                       .SetCpuUtilization(0.99)
                                       .SetMemUtilization(0.99)
                                       .SetQps(0.99)
                                       .SetEps(0.99)
                                       .SetUtilization("foo", 0.99)
                                       .SetUtilization("bar", 0.1)
                                       .Build();
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel =
      BuildChannel("intercept_trailing_metadata_lb", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  size_t total_num_rpcs = 0;
  {
    OrcaLoadReport load_report =
        OrcaLoadReportBuilder().SetApplicationUtilization(0.5).Build();
    OrcaLoadReport expected = OrcaLoadReportBuilder(per_server_load)
                                  .SetApplicationUtilization(0.5)
                                  .Build();
    for (size_t i = 0; i < kNumRpcs; ++i) {
      CheckRpcSendOk(DEBUG_LOCATION, stub, false, &load_report);
      auto actual = backend_load_report();
      ASSERT_TRUE(actual.has_value());
      CheckLoadReportAsExpected(*actual, expected);
      ++total_num_rpcs;
    }
  }
  {
    OrcaLoadReport load_report =
        OrcaLoadReportBuilder().SetMemUtilization(0.5).Build();
    OrcaLoadReport expected =
        OrcaLoadReportBuilder(per_server_load).SetMemUtilization(0.5).Build();
    for (size_t i = 0; i < kNumRpcs; ++i) {
      CheckRpcSendOk(DEBUG_LOCATION, stub, false, &load_report);
      auto actual = backend_load_report();
      ASSERT_TRUE(actual.has_value());
      CheckLoadReportAsExpected(*actual, expected);
      ++total_num_rpcs;
    }
  }
  {
    OrcaLoadReport load_report = OrcaLoadReportBuilder().SetQps(0.5).Build();
    OrcaLoadReport expected =
        OrcaLoadReportBuilder(per_server_load).SetQps(0.5).Build();
    for (size_t i = 0; i < kNumRpcs; ++i) {
      CheckRpcSendOk(DEBUG_LOCATION, stub, false, &load_report);
      auto actual = backend_load_report();
      ASSERT_TRUE(actual.has_value());
      CheckLoadReportAsExpected(*actual, expected);
      ++total_num_rpcs;
    }
  }
  {
    OrcaLoadReport load_report = OrcaLoadReportBuilder().SetEps(0.5).Build();
    OrcaLoadReport expected =
        OrcaLoadReportBuilder(per_server_load).SetEps(0.5).Build();
    for (size_t i = 0; i < kNumRpcs; ++i) {
      CheckRpcSendOk(DEBUG_LOCATION, stub, false, &load_report);
      auto actual = backend_load_report();
      ASSERT_TRUE(actual.has_value());
      CheckLoadReportAsExpected(*actual, expected);
      ++total_num_rpcs;
    }
  }
  {
    OrcaLoadReport load_report =
        OrcaLoadReportBuilder()
            .SetUtilization("foo", 0.5)
            .SetUtilization("bar", 1.1)  // Out of range.
            .SetUtilization("baz", 1.0)
            .Build();
    auto expected = OrcaLoadReportBuilder(per_server_load)
                        .SetUtilization("foo", 0.5)
                        .SetUtilization("baz", 1.0)
                        .Build();
    for (size_t i = 0; i < kNumRpcs; ++i) {
      CheckRpcSendOk(DEBUG_LOCATION, stub, false, &load_report);
      auto actual = backend_load_report();
      ASSERT_TRUE(actual.has_value());
      CheckLoadReportAsExpected(*actual, expected);
      ++total_num_rpcs;
    }
  }
  // Check LB policy name for the channel.
  EXPECT_EQ("intercept_trailing_metadata_lb",
            channel->GetLoadBalancingPolicyName());
  EXPECT_EQ(total_num_rpcs, num_trailers_intercepted());
}

//
// tests that per-address args from the resolver are visible to the LB policy
//

class ClientLbAddressTest : public ClientLbEnd2endTest {
 protected:
  void SetUp() override {
    ClientLbEnd2endTest::SetUp();
    current_test_instance_ = this;
  }

  static void SetUpTestSuite() {
    grpc_core::CoreConfiguration::Reset();
    grpc_core::CoreConfiguration::RegisterEphemeralBuilder(
        [](grpc_core::CoreConfiguration::Builder* builder) {
          grpc_core::RegisterAddressTestLoadBalancingPolicy(builder,
                                                            SaveAddress);
        });
    grpc_init();
  }

  static void TearDownTestSuite() {
    grpc_shutdown();
    grpc_core::CoreConfiguration::Reset();
  }

  std::vector<std::string> addresses_seen() {
    grpc_core::MutexLock lock(&mu_);
    return addresses_seen_;
  }

 private:
  static void SaveAddress(const grpc_core::EndpointAddresses& address) {
    ClientLbAddressTest* self = current_test_instance_;
    grpc_core::MutexLock lock(&self->mu_);
    self->addresses_seen_.emplace_back(address.ToString());
  }

  static ClientLbAddressTest* current_test_instance_;
  grpc_core::Mutex mu_;
  std::vector<std::string> addresses_seen_ ABSL_GUARDED_BY(&mu_);
};

ClientLbAddressTest* ClientLbAddressTest::current_test_instance_ = nullptr;

TEST_F(ClientLbAddressTest, Basic) {
  const int kNumServers = 1;
  StartServers(kNumServers);
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("address_test_lb", response_generator);
  auto stub = BuildStub(channel);
  // Addresses returned by the resolver will have attached args.
  response_generator.SetNextResolution(
      GetServersPorts(), nullptr,
      grpc_core::ChannelArgs().Set("test_key", "test_value"));
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  // Check LB policy name for the channel.
  EXPECT_EQ("address_test_lb", channel->GetLoadBalancingPolicyName());
  // Make sure that the attributes wind up on the subchannels.
  std::vector<std::string> expected;
  for (const int port : GetServersPorts()) {
    expected.emplace_back(absl::StrCat("addrs=[", grpc_core::LocalIp(), ":",
                                       port, "] args={test_key=test_value}"));
  }
  EXPECT_EQ(addresses_seen(), expected);
}

//
// tests OOB backend metric API
//

class OobBackendMetricTest : public ClientLbEnd2endTest {
 protected:
  using BackendMetricReport = std::pair<int /*port*/, OrcaLoadReport>;

  void SetUp() override {
    ClientLbEnd2endTest::SetUp();
    current_test_instance_ = this;
  }

  static void SetUpTestSuite() {
    grpc_core::CoreConfiguration::Reset();
    grpc_core::CoreConfiguration::RegisterEphemeralBuilder(
        [](grpc_core::CoreConfiguration::Builder* builder) {
          grpc_core::RegisterOobBackendMetricTestLoadBalancingPolicy(
              builder, BackendMetricCallback);
        });
    grpc_init();
  }

  static void TearDownTestSuite() {
    grpc_shutdown();
    grpc_core::CoreConfiguration::Reset();
  }

  std::optional<BackendMetricReport> GetBackendMetricReport() {
    grpc_core::MutexLock lock(&mu_);
    if (backend_metric_reports_.empty()) return std::nullopt;
    auto result = std::move(backend_metric_reports_.front());
    backend_metric_reports_.pop_front();
    return result;
  }

 private:
  static void BackendMetricCallback(
      const grpc_core::EndpointAddresses& address,
      const grpc_core::BackendMetricData& backend_metric_data) {
    auto load_report = BackendMetricDataToOrcaLoadReport(backend_metric_data);
    int port = grpc_sockaddr_get_port(&address.address());
    grpc_core::MutexLock lock(&current_test_instance_->mu_);
    current_test_instance_->backend_metric_reports_.push_back(
        {port, std::move(load_report)});
  }

  static OobBackendMetricTest* current_test_instance_;
  grpc_core::Mutex mu_;
  std::deque<BackendMetricReport> backend_metric_reports_ ABSL_GUARDED_BY(&mu_);
};

OobBackendMetricTest* OobBackendMetricTest::current_test_instance_ = nullptr;

TEST_F(OobBackendMetricTest, Basic) {
  StartServers(1);
  // Set initial backend metric data on server.
  constexpr char kMetricName[] = "foo";
  servers_[0]->server_metric_recorder_->SetApplicationUtilization(0.5);
  servers_[0]->server_metric_recorder_->SetCpuUtilization(0.1);
  servers_[0]->server_metric_recorder_->SetMemoryUtilization(0.2);
  servers_[0]->server_metric_recorder_->SetEps(0.3);
  servers_[0]->server_metric_recorder_->SetQps(0.4);
  servers_[0]->server_metric_recorder_->SetNamedUtilization(kMetricName, 0.4);
  // Start client.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("oob_backend_metric_test_lb", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an OK RPC.
  CheckRpcSendOk(DEBUG_LOCATION, stub);
  // Check LB policy name for the channel.
  EXPECT_EQ("oob_backend_metric_test_lb",
            channel->GetLoadBalancingPolicyName());
  // Check report seen by client.
  bool report_seen = false;
  for (size_t i = 0; i < 5; ++i) {
    auto report = GetBackendMetricReport();
    if (report.has_value()) {
      EXPECT_EQ(report->first, servers_[0]->port_);
      EXPECT_EQ(report->second.application_utilization(), 0.5);
      EXPECT_EQ(report->second.cpu_utilization(), 0.1);
      EXPECT_EQ(report->second.mem_utilization(), 0.2);
      EXPECT_EQ(report->second.eps(), 0.3);
      EXPECT_EQ(report->second.rps_fractional(), 0.4);
      EXPECT_THAT(
          report->second.utilization(),
          ::testing::UnorderedElementsAre(::testing::Pair(kMetricName, 0.4)));
      report_seen = true;
      break;
    }
    gpr_sleep_until(grpc_timeout_seconds_to_deadline(1));
  }
  ASSERT_TRUE(report_seen);
  // Now update the utilization data on the server.
  // Note that the server may send a new report while we're updating these,
  // so we set them in reverse order, so that we know we'll get all new
  // data once we see a report with the new app utilization value.
  servers_[0]->server_metric_recorder_->SetNamedUtilization(kMetricName, 0.7);
  servers_[0]->server_metric_recorder_->SetQps(0.8);
  servers_[0]->server_metric_recorder_->SetEps(0.6);
  servers_[0]->server_metric_recorder_->SetMemoryUtilization(0.5);
  servers_[0]->server_metric_recorder_->SetCpuUtilization(2.4);
  servers_[0]->server_metric_recorder_->SetApplicationUtilization(1.2);
  // Wait for client to see new report.
  report_seen = false;
  for (size_t i = 0; i < 5; ++i) {
    auto report = GetBackendMetricReport();
    if (report.has_value()) {
      EXPECT_EQ(report->first, servers_[0]->port_);
      if (report->second.application_utilization() != 0.5) {
        EXPECT_EQ(report->second.application_utilization(), 1.2);
        EXPECT_EQ(report->second.cpu_utilization(), 2.4);
        EXPECT_EQ(report->second.mem_utilization(), 0.5);
        EXPECT_EQ(report->second.eps(), 0.6);
        EXPECT_EQ(report->second.rps_fractional(), 0.8);
        EXPECT_THAT(
            report->second.utilization(),
            ::testing::UnorderedElementsAre(::testing::Pair(kMetricName, 0.7)));
        report_seen = true;
        break;
      }
    }
    gpr_sleep_until(grpc_timeout_seconds_to_deadline(1));
  }
  ASSERT_TRUE(report_seen);
}

//
// tests rewriting of control plane status codes
//

class ControlPlaneStatusRewritingTest : public ClientLbEnd2endTest {
 protected:
  static void SetUpTestSuite() {
    grpc_core::CoreConfiguration::Reset();
    grpc_core::CoreConfiguration::RegisterEphemeralBuilder(
        [](grpc_core::CoreConfiguration::Builder* builder) {
          grpc_core::RegisterFailLoadBalancingPolicy(
              builder, absl::AbortedError("nope"));
        });
    grpc_init();
  }

  static void TearDownTestSuite() {
    grpc_shutdown();
    grpc_core::CoreConfiguration::Reset();
  }
};

TEST_F(ControlPlaneStatusRewritingTest, RewritesFromLb) {
  // Start client.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("fail_lb", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts());
  // Send an RPC, verify that status was rewritten.
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::INTERNAL,
      "Illegal status code from LB pick; original status: ABORTED: nope");
}

TEST_F(ControlPlaneStatusRewritingTest, RewritesFromResolver) {
  // Start client.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  grpc_core::Resolver::Result result;
  result.service_config = absl::AbortedError("nope");
  result.addresses.emplace();
  response_generator.SetResponse(std::move(result));
  // Send an RPC, verify that status was rewritten.
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::INTERNAL,
      "Illegal status code from resolver; original status: ABORTED: nope");
}

TEST_F(ControlPlaneStatusRewritingTest, RewritesFromConfigSelector) {
  class FailConfigSelector : public grpc_core::ConfigSelector {
   public:
    explicit FailConfigSelector(absl::Status status)
        : status_(std::move(status)) {}
    grpc_core::UniqueTypeName name() const override {
      static grpc_core::UniqueTypeName::Factory kFactory("FailConfigSelector");
      return kFactory.Create();
    }
    bool Equals(const ConfigSelector* other) const override {
      return status_ == static_cast<const FailConfigSelector*>(other)->status_;
    }
    absl::Status GetCallConfig(GetCallConfigArgs /*args*/) override {
      return status_;
    }

   private:
    absl::Status status_;
  };
  // Start client.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("pick_first", response_generator);
  auto stub = BuildStub(channel);
  auto config_selector =
      grpc_core::MakeRefCounted<FailConfigSelector>(absl::AbortedError("nope"));
  grpc_core::Resolver::Result result;
  result.addresses.emplace();
  result.service_config =
      grpc_core::ServiceConfigImpl::Create(grpc_core::ChannelArgs(), "{}");
  ASSERT_TRUE(result.service_config.ok()) << result.service_config.status();
  result.args = grpc_core::ChannelArgs().SetObject(config_selector);
  response_generator.SetResponse(std::move(result));
  // Send an RPC, verify that status was rewritten.
  CheckRpcSendFailure(
      DEBUG_LOCATION, stub, StatusCode::INTERNAL,
      "Illegal status code from ConfigSelector; original status: "
      "ABORTED: nope");
}

//
// WeightedRoundRobinTest
//

const char kServiceConfigPerCall[] =
    "{\n"
    "  \"loadBalancingConfig\": [\n"
    "    {\"weighted_round_robin\": {\n"
    "      \"blackoutPeriod\": \"0s\",\n"
    "      \"weightUpdatePeriod\": \"0.1s\"\n"
    "    }}\n"
    "  ]\n"
    "}";

const char kServiceConfigOob[] =
    "{\n"
    "  \"loadBalancingConfig\": [\n"
    "    {\"weighted_round_robin\": {\n"
    "      \"blackoutPeriod\": \"0s\",\n"
    "      \"weightUpdatePeriod\": \"0.1s\",\n"
    "      \"enableOobLoadReport\": true\n"
    "    }}\n"
    "  ]\n"
    "}";

const char kServiceConfigWithOutlierDetection[] =
    "{\n"
    "  \"loadBalancingConfig\": [\n"
    "    {\"outlier_detection_experimental\": {\n"
    "      \"childPolicy\": [\n"
    "        {\"weighted_round_robin\": {\n"
    "          \"blackoutPeriod\": \"%ds\",\n"
    "          \"weightUpdatePeriod\": \"0.1s\"\n"
    "        }}\n"
    "      ]\n"
    "    }}\n"
    "  ]\n"
    "}";

class WeightedRoundRobinTest : public ClientLbEnd2endTest {
 protected:
  void ExpectWeightedRoundRobinPicks(
      const grpc_core::DebugLocation& location,
      const std::unique_ptr<grpc::testing::EchoTestService::Stub>& stub,
      const std::vector<size_t>& expected_weights, size_t total_passes = 3,
      EchoRequest* request_ptr = nullptr, int timeout_ms = 15000) {
    CHECK_EQ(expected_weights.size(), servers_.size());
    size_t total_picks_per_pass = 0;
    for (size_t picks : expected_weights) {
      total_picks_per_pass += picks;
    }
    size_t num_picks = 0;
    size_t num_passes = 0;
    SendRpcsUntil(
        location, stub,
        [&](const Status&) {
          if (++num_picks == total_picks_per_pass) {
            bool match = true;
            for (size_t i = 0; i < expected_weights.size(); ++i) {
              if (servers_[i]->service_.request_count() !=
                  expected_weights[i]) {
                match = false;
                break;
              }
            }
            if (match) {
              if (++num_passes == total_passes) return false;
            } else {
              num_passes = 0;
            }
            num_picks = 0;
            ResetCounters();
          }
          return true;
        },
        request_ptr, timeout_ms);
  }
};

TEST_F(WeightedRoundRobinTest, CallAndServerMetric) {
  const int kNumServers = 3;
  StartServers(kNumServers);
  // Report server metrics that should give 6:4:3 WRR picks.
  // weights = qps / (util + (eps/qps)) =
  //   1/(0.2+0.2) : 1/(0.3+0.3) : 2/(1.5+0.1) = 6:4:3
  // where util is app_util if set, or cpu_util.
  servers_[0]->server_metric_recorder_->SetApplicationUtilization(0.2);
  servers_[0]->server_metric_recorder_->SetEps(20);
  servers_[0]->server_metric_recorder_->SetQps(100);
  servers_[1]->server_metric_recorder_->SetApplicationUtilization(0.3);
  servers_[1]->server_metric_recorder_->SetEps(30);
  servers_[1]->server_metric_recorder_->SetQps(100);
  servers_[2]->server_metric_recorder_->SetApplicationUtilization(1.5);
  servers_[2]->server_metric_recorder_->SetEps(20);
  servers_[2]->server_metric_recorder_->SetQps(200);
  // Create channel.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts(),
                                       kServiceConfigPerCall);
  // Send requests with per-call reported EPS/QPS set to 0/100.
  // This should give 1/2:1/3:1/15 = 15:10:2 WRR picks.
  EchoRequest request;
  // We cannot override with 0 with proto3, so setting it to almost 0.
  request.mutable_param()->mutable_backend_metrics()->set_eps(
      std::numeric_limits<double>::min());
  request.mutable_param()->mutable_backend_metrics()->set_rps_fractional(100);
  ExpectWeightedRoundRobinPicks(DEBUG_LOCATION, stub,
                                /*expected_weights=*/{15, 10, 2},
                                /*total_passes=*/3, &request);
  // Now send requests without per-call reported QPS.
  // This should change WRR picks back to 6:4:3.
  ExpectWeightedRoundRobinPicks(DEBUG_LOCATION, stub,
                                /*expected_weights=*/{6, 4, 3});
  // Check LB policy name for the channel.
  EXPECT_EQ("weighted_round_robin", channel->GetLoadBalancingPolicyName());
}

// This tests a bug seen in production where the outlier_detection
// policy would incorrectly generate a duplicate READY notification on
// all of its subchannels every time it saw an update, thus causing the
// WRR policy to re-enter the blackout period for that address.
TEST_F(WeightedRoundRobinTest, WithOutlierDetection) {
  const int kBlackoutPeriodSeconds = 10;
  const int kNumServers = 3;
  StartServers(kNumServers);
  // Report server metrics that should give 6:4:3 WRR picks.
  // weights = qps / (util + (eps/qps)) =
  //   1/(0.2+0.2) : 1/(0.3+0.3) : 2/(1.5+0.1) = 6:4:3
  // where util is app_util if set, or cpu_util.
  servers_[0]->server_metric_recorder_->SetApplicationUtilization(0.2);
  servers_[0]->server_metric_recorder_->SetEps(20);
  servers_[0]->server_metric_recorder_->SetQps(100);
  servers_[1]->server_metric_recorder_->SetApplicationUtilization(0.3);
  servers_[1]->server_metric_recorder_->SetEps(30);
  servers_[1]->server_metric_recorder_->SetQps(100);
  servers_[2]->server_metric_recorder_->SetApplicationUtilization(1.5);
  servers_[2]->server_metric_recorder_->SetEps(20);
  servers_[2]->server_metric_recorder_->SetQps(200);
  // Create channel.
  // Initial blackout period is 0, so that we start seeing traffic in
  // the right proportions right away.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(
      GetServersPorts(),
      absl::StrFormat(kServiceConfigWithOutlierDetection, 0).c_str());
  // Send requests with per-call reported EPS/QPS set to 0/100.
  // This should give 1/2:1/3:1/15 = 15:10:2 WRR picks.
  // Keep sending RPCs long enough to go past the new blackout period
  // that we're going to add later.
  absl::Time deadline =
      absl::Now() +
      absl::Seconds(kBlackoutPeriodSeconds * grpc_test_slowdown_factor());
  EchoRequest request;
  // We cannot override with 0 with proto3, so setting it to almost 0.
  request.mutable_param()->mutable_backend_metrics()->set_eps(
      std::numeric_limits<double>::min());
  request.mutable_param()->mutable_backend_metrics()->set_rps_fractional(100);
  do {
    ExpectWeightedRoundRobinPicks(DEBUG_LOCATION, stub,
                                  /*expected_weights=*/{15, 10, 2},
                                  /*total_passes=*/3, &request);
  } while (absl::Now() < deadline);
  // Send a new resolver response that increases blackout period.
  response_generator.SetNextResolution(
      GetServersPorts(),
      absl::StrFormat(kServiceConfigWithOutlierDetection,
                      kBlackoutPeriodSeconds * grpc_test_slowdown_factor())
          .c_str());
  // Weights should be the same before the blackout period expires.
  ExpectWeightedRoundRobinPicks(
      DEBUG_LOCATION, stub, /*expected_weights=*/{15, 10, 2},
      /*total_passes=*/3, &request,
      /*timeout_ms=*/(kBlackoutPeriodSeconds - 1) * 1000);
}

class WeightedRoundRobinParamTest
    : public WeightedRoundRobinTest,
      public ::testing::WithParamInterface<const char*> {};

INSTANTIATE_TEST_SUITE_P(WeightedRoundRobin, WeightedRoundRobinParamTest,
                         ::testing::Values(kServiceConfigPerCall,
                                           kServiceConfigOob));

TEST_P(WeightedRoundRobinParamTest, Basic) {
  const int kNumServers = 3;
  StartServers(kNumServers);
  // Report server metrics that should give 1:2:4 WRR picks.
  // weights = qps / (util + (eps/qps)) =
  //   1/(0.4+0.4) : 1/(0.2+0.2) : 2/(0.3+0.1) = 1:2:4
  // where util is app_util if set, or cpu_util.
  servers_[0]->server_metric_recorder_->SetApplicationUtilization(0.4);
  servers_[0]->server_metric_recorder_->SetEps(40);
  servers_[0]->server_metric_recorder_->SetQps(100);
  servers_[1]->server_metric_recorder_->SetApplicationUtilization(0.2);
  servers_[1]->server_metric_recorder_->SetEps(20);
  servers_[1]->server_metric_recorder_->SetQps(100);
  servers_[2]->server_metric_recorder_->SetApplicationUtilization(0.3);
  servers_[2]->server_metric_recorder_->SetEps(5);
  servers_[2]->server_metric_recorder_->SetQps(200);
  // Create channel.
  FakeResolverResponseGeneratorWrapper response_generator;
  auto channel = BuildChannel("", response_generator);
  auto stub = BuildStub(channel);
  response_generator.SetNextResolution(GetServersPorts(), GetParam());
  // Wait for the right set of WRR picks.
  ExpectWeightedRoundRobinPicks(DEBUG_LOCATION, stub,
                                /*expected_weights=*/{1, 2, 4});
  // Check LB policy name for the channel.
  EXPECT_EQ("weighted_round_robin", channel->GetLoadBalancingPolicyName());
}

}  // namespace
}  // namespace testing
}  // namespace grpc

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  grpc::testing::TestEnvironment env(&argc, argv);
  // Make the backup poller poll very frequently in order to pick up
  // updates from all the subchannels's FDs.
  grpc_core::ConfigVars::Overrides overrides;
  overrides.client_channel_backup_poll_interval_ms = 1;
  grpc_core::ConfigVars::SetOverrides(overrides);
#if TARGET_OS_IPHONE
  // Workaround Apple CFStream bug
  grpc_core::SetEnv("grpc_cfstream", "0");
#endif
  grpc_init();
  grpc::testing::ConnectionAttemptInjector::Init();
  const auto result = RUN_ALL_TESTS();
  grpc_shutdown();
  return result;
}
