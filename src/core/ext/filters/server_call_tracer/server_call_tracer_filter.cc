//
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
//

#include <grpc/support/port_platform.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "absl/status/statusor.h"

#include "src/core/lib/channel/call_finalization.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/channel/channel_fwd.h"
#include "src/core/lib/channel/channel_stack.h"
#include "src/core/lib/channel/channel_stack_builder.h"
#include "src/core/lib/channel/context.h"
#include "src/core/lib/channel/promise_based_filter.h"
#include "src/core/lib/channel/server_call_tracer.h"
#include "src/core/lib/config/core_configuration.h"
#include "src/core/lib/promise/arena_promise.h"
#include "src/core/lib/promise/cancel_callback.h"
#include "src/core/lib/promise/context.h"
#include "src/core/lib/promise/map.h"
#include "src/core/lib/promise/pipe.h"
#include "src/core/lib/promise/poll.h"
#include "src/core/lib/promise/promise_notification.h"
#include "src/core/lib/promise/seq.h"
#include "src/core/lib/surface/channel_init.h"
#include "src/core/lib/surface/channel_stack_type.h"
#include "src/core/lib/transport/transport.h"

namespace grpc_core {

namespace {

class ServerCallTracerFilter : public grpc_core::ChannelFilter {
 public:
  static const grpc_channel_filter kFilter;

  static absl::StatusOr<ServerCallTracerFilter> Create(
      const grpc_core::ChannelArgs& /*args*/,
      grpc_core::ChannelFilter::Args /*filter_args*/);

  grpc_core::ArenaPromise<grpc_core::ServerMetadataHandle> MakeCallPromise(
      grpc_core::CallArgs call_args,
      grpc_core::NextPromiseFactory next_promise_factory) override;
};

const grpc_channel_filter ServerCallTracerFilter::kFilter =
    grpc_core::MakePromiseBasedFilter<
        ServerCallTracerFilter, grpc_core::FilterEndpoint::kServer,
        grpc_core::kFilterExaminesServerInitialMetadata |
            grpc_core::kFilterExaminesInboundMessages |
            grpc_core::kFilterExaminesOutboundMessages>("server_call_tracer");

absl::StatusOr<ServerCallTracerFilter> ServerCallTracerFilter::Create(
    const grpc_core::ChannelArgs& /*args*/,
    grpc_core::ChannelFilter::Args /*filter_args*/) {
  return ServerCallTracerFilter();
}

grpc_core::ArenaPromise<grpc_core::ServerMetadataHandle>
ServerCallTracerFilter::MakeCallPromise(
    grpc_core::CallArgs call_args,
    grpc_core::NextPromiseFactory next_promise_factory) {
  auto* call_context = grpc_core::GetContext<grpc_call_context_element>();
  auto* call_tracer = static_cast<grpc_core::ServerCallTracer*>(
      call_context[GRPC_CONTEXT_CALL_TRACER].value);
  if (call_tracer == nullptr) {
    return next_promise_factory(std::move(call_args));
  }
  auto continue_making_call_promise =
      [next_promise_factory = std::move(next_promise_factory),
       call_args = std::move(call_args), call_tracer]() mutable {
        call_tracer->RecordReceivedInitialMetadata(
            call_args.client_initial_metadata.get());
        call_args.server_initial_metadata->InterceptAndMap(
            [call_tracer](ServerMetadataHandle metadata) {
              call_tracer->RecordSendInitialMetadata(metadata.get());
              return metadata;
            });
        call_args.client_to_server_messages->InterceptAndMap(
            [call_tracer](grpc_core::MessageHandle message) {
              call_tracer->RecordReceivedMessage(*message->payload());
              return message;
            });
        call_args.server_to_client_messages->InterceptAndMap(
            [call_tracer](grpc_core::MessageHandle message) {
              call_tracer->RecordSendMessage(*message->payload());
              return message;
            });
        grpc_core::GetContext<grpc_core::CallFinalization>()->Add(
            [call_tracer](const grpc_call_final_info* final_info) {
              call_tracer->RecordEnd(final_info);
            });
        return grpc_core::OnCancel(
            Map(next_promise_factory(std::move(call_args)),
                [call_tracer](grpc_core::ServerMetadataHandle md) {
                  call_tracer->RecordReceivedTrailingMetadata(md.get());
                  return md;
                }),
            [call_tracer]() { call_tracer->RecordCancel(); });
      };
  if (!call_tracer->Ready()) {
    auto notification = std::make_shared<PromiseNotification>();
    call_tracer->NotifyOnReady([notification]() { notification->Notify(); });
    return Seq([notification]() { return notification->Wait(); },
               std::move(continue_making_call_promise));
  }
  return continue_making_call_promise();
}

}  // namespace

void RegisterServerCallTracerFilter(CoreConfiguration::Builder* builder) {
  builder->channel_init()->RegisterStage(
      GRPC_SERVER_CHANNEL, GRPC_CHANNEL_INIT_BUILTIN_PRIORITY,
      [](ChannelStackBuilder* builder) {
        if (!builder->channel_args().WantMinimalStack()) {
          builder->PrependFilter(&ServerCallTracerFilter::kFilter);
        }
        return true;
      });
}

}  // namespace grpc_core