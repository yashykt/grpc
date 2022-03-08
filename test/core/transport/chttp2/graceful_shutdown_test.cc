//
//
// Copyright 2022 gRPC authors.
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
//
//

#include <grpc/support/port_platform.h>

#include <stdlib.h>
#include <string.h>

#include <thread>

#include <gmock/gmock.h>

#include "absl/synchronization/mutex.h"
#include "absl/synchronization/notification.h"

#include <grpc/grpc.h>
#include <grpc/grpc_posix.h>
#include <grpc/grpc_security.h>

#include "src/core/ext/transport/chttp2/transport/chttp2_transport.h"
#include "src/core/ext/transport/chttp2/transport/frame_goaway.h"
#include "src/core/lib/channel/channel_stack_builder.h"
#include "src/core/lib/config/core_configuration.h"
#include "src/core/lib/gprpp/host_port.h"
#include "src/core/lib/iomgr/endpoint_pair.h"
#include "src/core/lib/slice/slice.h"
#include "src/core/lib/slice/slice_string_helpers.h"
#include "src/core/lib/surface/channel.h"
#include "src/core/lib/surface/server.h"
#include "test/core/end2end/cq_verifier.h"
#include "test/core/util/port.h"
#include "test/core/util/test_config.h"
#include "test/core/util/test_tcp_server.h"

namespace grpc_core {
namespace {

void* Tag(intptr_t t) { return reinterpret_cast<void*>(t); }

class GracefulShutdownTest : public ::testing::Test {
 protected:
  GracefulShutdownTest() { SetupAndStart(); }

  ~GracefulShutdownTest() override { ShutdownAndDestroy(); }

  // Sets up the client and server
  void SetupAndStart() {
    ExecCtx exec_ctx;
    cq_ = grpc_completion_queue_create_for_next(nullptr);
    grpc_arg server_args[] = {
        grpc_channel_arg_integer_create(
            const_cast<char*>(GRPC_ARG_HTTP2_BDP_PROBE), 0),
        grpc_channel_arg_integer_create(
            const_cast<char*>(GRPC_ARG_KEEPALIVE_TIME_MS), INT_MAX)};
    grpc_channel_args server_channel_args = {GPR_ARRAY_SIZE(server_args),
                                             server_args};
    // Create server
    server_ = grpc_server_create(&server_channel_args, nullptr);
    auto* core_server = grpc_core::Server::FromC(server_);
    grpc_server_register_completion_queue(server_, cq_, nullptr);
    grpc_server_start(server_);
    fds_ = grpc_iomgr_create_endpoint_pair("fixture", nullptr);
    auto* transport = grpc_create_chttp2_transport(core_server->channel_args(),
                                                   fds_.server, false);
    grpc_endpoint_add_to_pollset(fds_.server, grpc_cq_pollset(cq_));
    GPR_ASSERT(core_server->SetupTransport(transport, nullptr,
                                           core_server->channel_args(),
                                           nullptr) == GRPC_ERROR_NONE);
    // Start polling on the client
    client_poll_thread_ = absl::make_unique<std::thread>([this]() {
      grpc_completion_queue* client_cq =
          grpc_completion_queue_create_for_next(nullptr);
      {
        ExecCtx exec_ctx;
        grpc_endpoint_add_to_pollset(fds_.client, grpc_cq_pollset(client_cq));
      }
      while (!shutdown_) {
        GPR_ASSERT(
            grpc_completion_queue_next(
                client_cq, grpc_timeout_milliseconds_to_deadline(10), nullptr)
                .type == GRPC_QUEUE_TIMEOUT);
      }
    });
    // Write connection prefix and settings frame
    constexpr char kPrefix[] =
        "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n\x00\x00\x00\x04\x00\x00\x00\x00\x00";
    Write(absl::string_view(kPrefix, sizeof(kPrefix) - 1));
    // Start reading on the client
    grpc_slice_buffer_init(&read_buffer_);
    GRPC_CLOSURE_INIT(&on_read_done_, OnReadDone, this, nullptr);
    grpc_endpoint_read(fds_.client, &read_buffer_, &on_read_done_, false);
  }

  // Shuts down and destroys the client and server.
  void ShutdownAndDestroy() {
    // Shutdown and destroy server
    grpc_server_shutdown_and_notify(server_, cq_, Tag(1000));
    while (grpc_completion_queue_next(cq_, gpr_inf_future(GPR_CLOCK_REALTIME),
                                      nullptr)
               .tag != Tag(1000)) {
    }
    grpc_server_destroy(server_);
    shutdown_ = true;
    client_poll_thread_->join();
    ExecCtx exec_ctx;
    grpc_endpoint_shutdown(
        fds_.client, GRPC_ERROR_CREATE_FROM_STATIC_STRING("Client shutdown"));
    ExecCtx::Get()->Flush();
    GPR_ASSERT(read_end_notification_.WaitForNotificationWithTimeout(
        absl::Seconds(5)));
    grpc_endpoint_destroy(fds_.client);
  }

  static void OnReadDone(void* arg, grpc_error_handle error) {
    GracefulShutdownTest* self = static_cast<GracefulShutdownTest*>(arg);
    if (error == GRPC_ERROR_NONE) {
      {
        absl::MutexLock lock(&self->mu_);
        for (size_t i = 0; i < self->read_buffer_.count; ++i) {
          absl::StrAppend(&self->read_bytes_,
                          StringViewFromSlice(self->read_buffer_.slices[i]));
        }
        self->read_cv_.SignalAll();
      }
      grpc_slice_buffer_reset_and_unref(&self->read_buffer_);
      grpc_endpoint_read(self->fds_.client, &self->read_buffer_,
                         &self->on_read_done_, false);
    } else {
      grpc_slice_buffer_destroy(&self->read_buffer_);
      self->read_end_notification_.Notify();
    }
  }

  // Waits for \a bytes to show up in read_bytes_
  void WaitForReadBytes(absl::string_view bytes) {
    std::atomic<bool> done{false};
    std::thread cq_driver([&]() {
      while (!done) {
        grpc_event ev = grpc_completion_queue_next(
            cq_, grpc_timeout_milliseconds_to_deadline(10), nullptr);
        GPR_ASSERT(ev.type == GRPC_QUEUE_TIMEOUT);
      }
    });
    {
      absl::MutexLock lock(&mu_);
      while (!absl::StrContains(read_bytes_, bytes)) {
        read_cv_.WaitWithTimeout(&mu_, absl::Seconds(5));
      }
    }
    done = true;
    cq_driver.join();
  }

  void WaitForGoaway(uint32_t last_stream_id) {
    grpc_slice_buffer buffer;
    grpc_slice_buffer_init(&buffer);
    grpc_chttp2_goaway_append(last_stream_id, 0, grpc_empty_slice(), &buffer);
    std::string expected_bytes;
    for (size_t i = 0; i < buffer.count; ++i) {
      absl::StrAppend(&expected_bytes, StringViewFromSlice(buffer.slices[i]));
      char* dump =
          grpc_dump_slice(buffer.slices[i], GPR_DUMP_HEX | GPR_DUMP_ASCII);
      gpr_log(GPR_DEBUG, "DATA: %s", dump);
      gpr_free(dump);
    }
    grpc_slice_buffer_destroy(&buffer);
    WaitForReadBytes(expected_bytes);
  }

  // This is a blocking call. It waits for the write callback to be invoked
  // before returning. (In other words, do not call this from a thread that
  // should not be blocked, for example, a polling thread.)
  void Write(absl::string_view bytes) {
    grpc_slice slice =
        StaticSlice::FromStaticBuffer(bytes.data(), bytes.size()).TakeCSlice();
    grpc_slice_buffer buffer;
    grpc_slice_buffer_init(&buffer);
    grpc_slice_buffer_add(&buffer, slice);
    WriteBuffer(&buffer);
    grpc_slice_buffer_destroy(&buffer);
  }

  void WriteBuffer(grpc_slice_buffer* buffer) {
    absl::Notification on_write_done_notification_;
    GRPC_CLOSURE_INIT(&on_write_done_, OnWriteDone,
                      &on_write_done_notification_, nullptr);
    grpc_endpoint_write(fds_.client, buffer, &on_write_done_, nullptr);
    ExecCtx::Get()->Flush();
    GPR_ASSERT(on_write_done_notification_.WaitForNotificationWithTimeout(
        absl::Seconds(5)));
  }

  static void OnWriteDone(void* arg, grpc_error_handle error) {
    GPR_ASSERT(error == GRPC_ERROR_NONE);
    absl::Notification* on_write_done_notification_ =
        static_cast<absl::Notification*>(arg);
    on_write_done_notification_->Notify();
  }

  grpc_endpoint_pair fds_;
  grpc_server* server_;
  grpc_completion_queue* cq_;
  std::unique_ptr<std::thread> client_poll_thread_;
  std::atomic<bool> shutdown_{false};
  grpc_closure on_read_done_;
  absl::Mutex mu_;
  absl::CondVar read_cv_;
  absl::Notification read_end_notification_;
  grpc_slice_buffer read_buffer_;
  std::string read_bytes_ ABSL_GUARDED_BY(mu_);
  grpc_closure on_write_done_;
};

// TEST_F(GracefulShutdownTest, GracefulGoaway) {
//   // Initiate shutdown on the server
//   grpc_server_shutdown_and_notify(server_, cq_, Tag(1));
//   // Wait for first goaway
//   WaitForGoaway((1u << 31) - 1);
// }

}  // namespace
}  // namespace grpc_core

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  grpc::testing::TestEnvironment env(argc, argv);
  grpc_init();
  int result = RUN_ALL_TESTS();
  grpc_shutdown();
  return result;
}
