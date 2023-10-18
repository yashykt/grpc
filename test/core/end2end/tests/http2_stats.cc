// Copyright 2023 gRPC authors.
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

#include <limits.h>

#include <functional>
#include <string>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "gtest/gtest.h"

#include <grpc/status.h>
#include <grpc/support/log.h>
#include <grpc/support/time.h>

#include "src/core/lib/channel/call_tracer.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/channel/channel_fwd.h"
#include "src/core/lib/channel/channel_stack.h"
#include "src/core/lib/channel/channel_stack_builder.h"
#include "src/core/lib/channel/context.h"
#include "src/core/lib/channel/promise_based_filter.h"
#include "src/core/lib/config/core_configuration.h"
#include "src/core/lib/gprpp/time.h"
#include "src/core/lib/iomgr/error.h"
#include "src/core/lib/promise/arena_promise.h"
#include "src/core/lib/promise/context.h"
#include "src/core/lib/resource_quota/arena.h"
#include "src/core/lib/slice/slice.h"
#include "src/core/lib/slice/slice_buffer.h"
#include "src/core/lib/surface/channel_stack_type.h"
#include "src/core/lib/transport/metadata_batch.h"
#include "src/core/lib/transport/transport.h"
#include "test/core/end2end/end2end_tests.h"

namespace grpc_core {
namespace {

class FakeCallTracer : public ClientCallTracer {
 public:
  class FakeCallAttemptTracer : public CallAttemptTracer {
   public:
    std::string TraceId() override { return ""; }
    std::string SpanId() override { return ""; }
    bool IsSampled() override { return false; }
    void RecordSendInitialMetadata(
        grpc_metadata_batch* send_initial_metadata) override {}
    void RecordSendTrailingMetadata(
        grpc_metadata_batch* /*send_trailing_metadata*/) override {}
    void RecordSendMessage(const SliceBuffer& /*send_message*/) override {}
    void RecordSendCompressedMessage(
        const SliceBuffer& /*send_compressed_message*/) override {}
    void RecordReceivedInitialMetadata(
        grpc_metadata_batch* /*recv_initial_metadata*/) override {}
    void RecordReceivedMessage(const SliceBuffer& /*recv_message*/) override {}
    void RecordReceivedDecompressedMessage(
        const SliceBuffer& /*recv_decompressed_message*/) override {}

    void RecordReceivedTrailingMetadata(
        absl::Status status, grpc_metadata_batch* /*recv_trailing_metadata*/,
        const grpc_transport_stream_stats* transport_stream_stats) override {
      transport_stream_stats_ = *transport_stream_stats;
    }

    void RecordCancel(grpc_error_handle cancel_error) override {}
    void RecordEnd(const gpr_timespec& /*latency*/) override { delete this; }
    void RecordAnnotation(absl::string_view /*annotation*/) override {}
    void RecordAnnotation(const Annotation& /*annotation*/) override {}

    static grpc_transport_stream_stats transport_stream_stats() {
      return transport_stream_stats_;
    }

   private:
    static grpc_transport_stream_stats transport_stream_stats_;
  };

  explicit FakeCallTracer() {}
  ~FakeCallTracer() override {}
  std::string TraceId() override { return ""; }
  std::string SpanId() override { return ""; }
  bool IsSampled() override { return false; }

  FakeCallAttemptTracer* StartNewAttempt(bool is_transparent_retry) override {
    return new FakeCallAttemptTracer;
  }

  void RecordAnnotation(absl::string_view /*annotation*/) override {}
  void RecordAnnotation(const Annotation& /*annotation*/) override {}
};

grpc_transport_stream_stats
    FakeCallTracer::FakeCallAttemptTracer::transport_stream_stats_;

class FakeClientFilter : public ChannelFilter {
 public:
  static const grpc_channel_filter kFilter;

  static absl::StatusOr<FakeClientFilter> Create(
      const ChannelArgs& /*args*/, ChannelFilter::Args /*filter_args*/) {
    return FakeClientFilter();
  }

  ArenaPromise<ServerMetadataHandle> MakeCallPromise(
      CallArgs call_args, NextPromiseFactory next_promise_factory) override {
    auto* call_context = GetContext<grpc_call_context_element>();
    auto* tracer = GetContext<Arena>()->ManagedNew<FakeCallTracer>();
    GPR_DEBUG_ASSERT(
        call_context[GRPC_CONTEXT_CALL_TRACER_ANNOTATION_INTERFACE].value ==
        nullptr);
    call_context[GRPC_CONTEXT_CALL_TRACER_ANNOTATION_INTERFACE].value = tracer;
    call_context[GRPC_CONTEXT_CALL_TRACER_ANNOTATION_INTERFACE].destroy =
        nullptr;
    return next_promise_factory(std::move(call_args));
  }
};

const grpc_channel_filter FakeClientFilter::kFilter =
    MakePromiseBasedFilter<FakeClientFilter, FilterEndpoint::kClient>(
        "fake_client");

class FakeServerCallTracer : public ServerCallTracer {
 public:
  ~FakeServerCallTracer() override { gpr_log(GPR_ERROR, "here"); }
  void RecordSendInitialMetadata(
      grpc_metadata_batch* /*send_initial_metadata*/) override {}
  void RecordSendTrailingMetadata(
      grpc_metadata_batch* /*send_trailing_metadata*/) override {}
  void RecordSendMessage(const SliceBuffer& /*send_message*/) override {}
  void RecordSendCompressedMessage(
      const SliceBuffer& /*send_compressed_message*/) override {}
  void RecordReceivedInitialMetadata(
      grpc_metadata_batch* /*recv_initial_metadata*/) override {}
  void RecordReceivedMessage(const SliceBuffer& /*recv_message*/) override {}
  void RecordReceivedDecompressedMessage(
      const SliceBuffer& /*recv_decompressed_message*/) override {}
  void RecordCancel(grpc_error_handle /*cancel_error*/) override {}
  void RecordReceivedTrailingMetadata(
      grpc_metadata_batch* /*recv_trailing_metadata*/) override {}

  void RecordEnd(const grpc_call_final_info* final_info) override {
    transport_stream_stats_ = final_info->stats.transport_stream_stats;
  }

  void RecordAnnotation(absl::string_view /*annotation*/) override {}
  void RecordAnnotation(const Annotation& /*annotation*/) override {}
  std::string TraceId() override { return ""; }
  std::string SpanId() override { return ""; }
  bool IsSampled() override { return false; }

  static grpc_transport_stream_stats transport_stream_stats() {
    return transport_stream_stats_;
  }

 private:
  static grpc_transport_stream_stats transport_stream_stats_;
};

grpc_transport_stream_stats FakeServerCallTracer::transport_stream_stats_;

class FakeServerCallTracerFactory : public ServerCallTracerFactory {
 public:
  ServerCallTracer* CreateNewServerCallTracer(Arena* arena) override {
    return arena->ManagedNew<FakeServerCallTracer>();
  }
};

// This test verifies the HTTP2 stats for data bytes on a stream
CORE_END2END_TEST(Http2FullstackTest, StreamStats) {
  CoreConfiguration::RegisterBuilder([](CoreConfiguration::Builder* builder) {
    builder->channel_init()->RegisterStage(
        GRPC_CLIENT_CHANNEL, /*priority=*/INT_MAX,
        [](ChannelStackBuilder* builder) {
          builder->PrependFilter(&FakeClientFilter::kFilter);
          return true;
        });
  });
  ServerCallTracerFactory::RegisterGlobal(new FakeServerCallTracerFactory);

  auto send_from_client = RandomSlice(10);
  auto send_from_server = RandomSlice(20);
  CoreEnd2endTest::IncomingStatusOnClient server_status;
  CoreEnd2endTest::IncomingMetadata server_initial_metadata;
  CoreEnd2endTest::IncomingMessage server_message;
  CoreEnd2endTest::IncomingMessage client_message;
  CoreEnd2endTest::IncomingCloseOnServer client_close;
  {
    auto c = NewClientCall("/foo").Timeout(Duration::Minutes(5)).Create();
    c.NewBatch(1)
        .SendInitialMetadata({})
        .SendMessage(send_from_client.Ref())
        .SendCloseFromClient()
        .RecvInitialMetadata(server_initial_metadata)
        .RecvMessage(server_message)
        .RecvStatusOnClient(server_status);
    auto s = RequestCall(101);
    Expect(101, true);
    Step(Duration::Minutes(1));
    s.NewBatch(102).SendInitialMetadata({}).RecvMessage(client_message);
    Expect(102, true);
    Step(Duration::Minutes(1));
    s.NewBatch(103)
        .SendStatusFromServer(GRPC_STATUS_UNIMPLEMENTED, "xyz", {})
        .SendMessage(send_from_server.Ref())
        .RecvCloseOnServer(client_close);
    Expect(103, true);
    Expect(1, true);
    Step(Duration::Minutes(1));
    EXPECT_EQ(s.method(), "/foo");
  }
  EXPECT_EQ(server_status.status(), GRPC_STATUS_UNIMPLEMENTED);
  EXPECT_EQ(server_status.message(), "xyz");
  EXPECT_FALSE(client_close.was_cancelled());
  EXPECT_EQ(client_message.payload(), send_from_client);
  EXPECT_EQ(server_message.payload(), send_from_server);

  auto client_transport_stats =
      FakeCallTracer::FakeCallAttemptTracer::transport_stream_stats();
  EXPECT_EQ(client_transport_stats.outgoing.data_bytes,
            send_from_client.size());
  EXPECT_EQ(client_transport_stats.incoming.data_bytes,
            send_from_server.size());
  auto server_transport_stats = FakeServerCallTracer::transport_stream_stats();
  EXPECT_EQ(server_transport_stats.outgoing.data_bytes,
            send_from_server.size());
  EXPECT_EQ(server_transport_stats.incoming.data_bytes,
            send_from_client.size());
  // TODO(yashykt): Add tests for framing bytes as well

  delete ServerCallTracerFactory::Get(ChannelArgs());
  ServerCallTracerFactory::RegisterGlobal(nullptr);
}

}  // namespace
}  // namespace grpc_core
