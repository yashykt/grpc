/*
 *
 * Copyright 2018 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <grpc/support/port_platform.h>

#include <string>
#include <utility>
#include <vector>

#include "src/cpp/ext/filters/census/client_filter.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "opencensus/stats/stats.h"
#include "opencensus/tags/tag_key.h"
#include "opencensus/tags/tag_map.h"
#include "src/core/lib/surface/call.h"
#include "src/cpp/ext/filters/census/grpc_plugin.h"
#include "src/cpp/ext/filters/census/measures.h"

namespace grpc {

constexpr uint32_t
    OpenCensusCallTracer::OpenCensusCallAttemptTracer::kMaxTraceContextLen;
constexpr uint32_t
    OpenCensusCallTracer::OpenCensusCallAttemptTracer::kMaxTagsLen;

namespace {

void FilterTrailingMetadata(grpc_metadata_batch* b, uint64_t* elapsed_time) {
  if (b->idx.named.grpc_server_stats_bin != nullptr) {
    ServerStatsDeserialize(
        reinterpret_cast<const char*>(GRPC_SLICE_START_PTR(
            GRPC_MDVALUE(b->idx.named.grpc_server_stats_bin->md))),
        GRPC_SLICE_LENGTH(GRPC_MDVALUE(b->idx.named.grpc_server_stats_bin->md)),
        elapsed_time);
    grpc_metadata_batch_remove(b, b->idx.named.grpc_server_stats_bin);
  }
}

}  // namespace

void CensusClientCallData::StartTransportStreamOpBatch(
    grpc_call_element* elem, TransportStreamOpBatch* op) {
  grpc_call_next_op(elem, op->op());
}

grpc_error_handle CensusClientCallData::Init(
    grpc_call_element* elem, const grpc_call_element_args* args) {
  tracer_.Init(args);
  return GRPC_ERROR_NONE;
}

void CensusClientCallData::Destroy(grpc_call_element* /*elem*/,
                                   const grpc_call_final_info* /* final_info */,
                                   grpc_closure* /*then_call_closure*/) {}

//
// OpenCensusCallTracer::OpenCensusCallAttemptTracer
//

void OpenCensusCallTracer::OpenCensusCallAttemptTracer::
    RecordSendInitialMetadata(grpc_metadata_batch* send_initial_metadata,
                              uint32_t /* flags */) {
  GenerateClientContext(
      parent_->qualified_method_, &context_,
      (parent_->context_ == nullptr) ? nullptr : parent_->context_);
  size_t tracing_len = TraceContextSerialize(parent_->context_->Context(),
                                             tracing_buf_, kMaxTraceContextLen);
  if (tracing_len > 0) {
    GRPC_LOG_IF_ERROR(
        "census grpc_filter",
        grpc_metadata_batch_add_tail(
            send_initial_metadata, &tracing_bin_,
            grpc_mdelem_from_slices(
                GRPC_MDSTR_GRPC_TRACE_BIN,
                grpc_core::UnmanagedMemorySlice(tracing_buf_, tracing_len)),
            GRPC_BATCH_GRPC_TRACE_BIN));
  }
  grpc_slice tags = grpc_empty_slice();
  // TODO(unknown): Add in tagging serialization.
  size_t encoded_tags_len = StatsContextSerialize(kMaxTagsLen, &tags);
  if (encoded_tags_len > 0) {
    GRPC_LOG_IF_ERROR(
        "census grpc_filter",
        grpc_metadata_batch_add_tail(
            send_initial_metadata, &stats_bin_,
            grpc_mdelem_from_slices(GRPC_MDSTR_GRPC_TAGS_BIN, tags),
            GRPC_BATCH_GRPC_TAGS_BIN));
  }
}

void OpenCensusCallTracer::OpenCensusCallAttemptTracer::RecordSendMessage(
    const grpc_core::ByteStream& /* send_message */) {
  ++sent_message_count_;
}

void OpenCensusCallTracer::OpenCensusCallAttemptTracer::RecordReceivedMessage(
    const grpc_core::ByteStream& /* recv_message */) {
  recv_message_count_++;
}
void OpenCensusCallTracer::OpenCensusCallAttemptTracer::
    RecordReceivedTrailingMetadata(
        grpc_metadata_batch* recv_trailing_metadata) {
  FilterTrailingMetadata(recv_trailing_metadata, &elapsed_time_);
}

void OpenCensusCallTracer::OpenCensusCallAttemptTracer::RecordEnd(
    const grpc_call_final_info& final_info) {
  const uint64_t request_size = GetOutgoingDataSize(&final_info);
  const uint64_t response_size = GetIncomingDataSize(&final_info);
  double latency_ms = absl::ToDoubleMilliseconds(absl::Now() - start_time_);
  std::vector<std::pair<opencensus::tags::TagKey, std::string>> tags =
      context_.tags().tags();
  std::string method = absl::StrCat(parent_->method_);
  tags.emplace_back(ClientMethodTagKey(), method);
  std::string final_status =
      absl::StrCat(StatusCodeToString(final_info.final_status));
  tags.emplace_back(ClientStatusTagKey(), final_status);
  ::opencensus::stats::Record(
      {{RpcClientSentBytesPerRpc(), static_cast<double>(request_size)},
       {RpcClientReceivedBytesPerRpc(), static_cast<double>(response_size)},
       {RpcClientRoundtripLatency(), latency_ms},
       {RpcClientServerLatency(),
        ToDoubleMilliseconds(absl::Nanoseconds(elapsed_time_))},
       {RpcClientSentMessagesPerRpc(), sent_message_count_},
       {RpcClientReceivedMessagesPerRpc(), recv_message_count_}},
      tags);
  grpc_slice_unref_internal(parent_->path_);
  if (final_info.final_status != GRPC_STATUS_OK) {
    // TODO(unknown): Map grpc_status_code to trace::StatusCode.
    context_.Span().SetStatus(opencensus::trace::StatusCode::UNKNOWN,
                              StatusCodeToString(final_info.final_status));
  }
  context_.EndSpan();
  delete this;
}

//
// OpenCensusCallTracer
//

void OpenCensusCallTracer::Init(const grpc_call_element_args* args) {
  path_ = grpc_slice_ref_internal(args->path);
  method_ = GetMethod(&path_);
  qualified_method_ = absl::StrCat("Sent.", method_);
  context_ = reinterpret_cast<CensusContext*>(args->context);
}

OpenCensusCallTracer::OpenCensusCallAttemptTracer*
OpenCensusCallTracer::RecordNewAttempt(bool /* is_transparent_retry */) {
  // This can probably go on the call arena
  return new OpenCensusCallAttemptTracer(this);
}

}  // namespace grpc
