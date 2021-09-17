//
//
// Copyright 2020 gRPC authors.
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

#include "src/core/ext/xds/xds_server_config_selector.h"

#include "src/core/lib/channel/server_config_selector.h"

namespace grpc_core {
namespace {

void* XdsServerConfigSelectorArgCopy(void* p) {
  XdsServerConfigSelectorArg* arg = static_cast<XdsServerConfigSelectorArg*>(p);
  return arg->Ref().release();
}

void XdsServerConfigSelectorArgDestroy(void* p) {
  XdsServerConfigSelectorArg* arg = static_cast<XdsServerConfigSelectorArg*>(p);
  arg->Unref();
}

int XdsServerConfigSelectorArgCmp(void* p, void* q) { return GPR_ICMP(p, q); }

const grpc_arg_pointer_vtable kChannelArgVtable = {
    XdsServerConfigSelectorArgCopy, XdsServerConfigSelectorArgDestroy,
    XdsServerConfigSelectorArgCmp};

}  // namespace

const char* XdsServerConfigSelectorArg::kChannelArgName =
    "grpc.internal.xds_server_config_selector";

grpc_arg XdsServerConfigSelectorArg::MakeChannelArg() const {
  return grpc_channel_arg_pointer_create(
      const_cast<char*>(kChannelArgName),
      const_cast<XdsServerConfigSelectorArg*>(this), &kChannelArgVtable);
}

RefCountedPtr<XdsServerConfigSelectorArg> XdsServerConfigSelectorArg::GetFromChannelArgs(const grpc_channel_args& args) {
   XdsServerConfigSelectorArg* config_selector_arg =
      grpc_channel_args_find_pointer<XdsServerConfigSelectorArg>(&args,
                                                     kChannelArgName);
  return config_selector_arg != nullptr ? config_selector_arg->Ref() : nullptr;
}

namespace {

class XdsServerConfigSelector : public ServerConfigSelector {
  explicit XdsServerConfigSelector(XdsApi::RdsUpdate rds_update, grpc_error_handle* error) {

  }
};

class ChannelData {
 public:
  static grpc_error_handle Init(grpc_channel_element* elem,
                                grpc_channel_element_args* args);
  static void Destroy(grpc_channel_element* elem);

 private:
  ChannelData(grpc_channel_element* elem, grpc_channel_element_args* args);

  RefCountedPtr<XdsServerConfigSelectorArg> config_selector_arg;
};

class CallData {
 public:
  static grpc_error_handle Init(grpc_call_element* elem,
                                const grpc_call_element_args* args);
  static void Destroy(grpc_call_element* elem,
                      const grpc_call_final_info* /* final_info */,
                      grpc_closure* /* then_schedule_closure */);
  static void StartTransportStreamOpBatch(
      grpc_call_element* elem, grpc_transport_stream_op_batch* batch);

 private:
  CallData(grpc_call_element* elem, const grpc_call_element_args& args);
  static void RecvInitialMetadataReady(void* user_data,
                                       grpc_error_handle error);
  static void RecvTrailingMetadataReady(void* user_data,
                                        grpc_error_handle error);
  void MaybeResumeRecvTrailingMetadataReady();

  grpc_core::CallCombiner* call_combiner_;
  // Overall error for the call
  grpc_error_handle error_ = GRPC_ERROR_NONE;
  // State for keeping track of recv_initial_metadata
  grpc_metadata_batch* recv_initial_metadata_ = nullptr;
  grpc_closure* original_recv_initial_metadata_ready_ = nullptr;
  grpc_closure recv_initial_metadata_ready_;
  grpc_error_handle recv_initial_metadata_error_ = GRPC_ERROR_NONE;
  // State for keeping of track of recv_trailing_metadata
  grpc_closure* original_recv_trailing_metadata_ready_;
  grpc_closure recv_trailing_metadata_ready_;
  grpc_error_handle recv_trailing_metadata_ready_error_;
  bool seen_recv_trailing_metadata_ready_ = false;
};

// ChannelData

grpc_error_handle ChannelData::Init(grpc_channel_element* elem,
                                    grpc_channel_element_args* args) {
  GPR_ASSERT(elem->filter = &kXdsServerConfigSelectorFilter);
  new (elem->channel_data) ChannelData(elem, args);
  return GRPC_ERROR_NONE;
}

void ChannelData::Destroy(grpc_channel_element* elem) {
  auto* chand = static_cast<ChannelData*>(elem->channel_data);
  chand->~ChannelData();
}

ChannelData::ChannelData(grpc_channel_element* elem,
                         grpc_channel_element_args* args) {
  
}

// CallData

grpc_error_handle CallData::Init(grpc_call_element* elem,
                                 const grpc_call_element_args* args) {
  auto* calld = new (elem->call_data) CallData(elem, *args);
  return GRPC_ERROR_NONE;
}

void CallData::Destroy(grpc_call_element* elem,
                       const grpc_call_final_info* /*final_info*/,
                       grpc_closure* /*then_schedule_closure*/) {
  auto* calld = static_cast<CallData*>(elem->call_data);
  calld->~CallData();
}

void CallData::StartTransportStreamOpBatch(grpc_call_element* elem,
                                           grpc_transport_stream_op_batch* op) {
  CallData* calld = static_cast<CallData*>(elem->call_data);
  if (op->recv_initial_metadata) {
    calld->recv_initial_metadata_ =
        op->payload->recv_initial_metadata.recv_initial_metadata;
    calld->original_recv_initial_metadata_ready_ =
        op->payload->recv_initial_metadata.recv_initial_metadata_ready;
    op->payload->recv_initial_metadata.recv_initial_metadata_ready =
        &calld->recv_initial_metadata_ready_;
  }
  if (op->recv_trailing_metadata) {
    // We might generate errors on receiving initial metadata which we need to
    // bubble up through recv_trailing_metadata_ready
    calld->original_recv_trailing_metadata_ready_ =
        op->payload->recv_trailing_metadata.recv_trailing_metadata_ready;
    op->payload->recv_trailing_metadata.recv_trailing_metadata_ready =
        &calld->recv_trailing_metadata_ready_;
  }
  // Chain to the next filter.
  grpc_call_next_op(elem, op);
}

CallData::CallData(grpc_call_element* elem, const grpc_call_element_args& args)
    : call_combiner_(args.call_combiner) {
  GRPC_CLOSURE_INIT(&recv_initial_metadata_ready_, RecvInitialMetadataReady,
                    elem, grpc_schedule_on_exec_ctx);
  GRPC_CLOSURE_INIT(&recv_trailing_metadata_ready_, RecvTrailingMetadataReady,
                    elem, grpc_schedule_on_exec_ctx);
}

void CallData::RecvInitialMetadataReady(void* user_data,
                                        grpc_error_handle error) {
  grpc_call_element* elem = static_cast<grpc_call_element*>(user_data);
  CallData* calld = static_cast<CallData*>(elem->call_data);
  if (error == GRPC_ERROR_NONE) {
  }
  calld->MaybeResumeRecvTrailingMetadataReady();
  grpc_closure* closure = calld->original_recv_initial_metadata_ready_;
  calld->original_recv_initial_metadata_ready_ = nullptr;
  Closure::Run(DEBUG_LOCATION, closure, GRPC_ERROR_REF(error));
}

void CallData::RecvTrailingMetadataReady(void* user_data,
                                         grpc_error_handle error) {
  grpc_call_element* elem = static_cast<grpc_call_element*>(user_data);
  CallData* calld = static_cast<CallData*>(elem->call_data);
  if (calld->original_recv_initial_metadata_ready_ != nullptr) {
    calld->seen_recv_trailing_metadata_ready_ = true;
    calld->recv_trailing_metadata_ready_error_ = GRPC_ERROR_REF(error);
    GRPC_CALL_COMBINER_STOP(calld->call_combiner_,
                            "Deferring RecvTrailingMetadataReady until after "
                            "RecvInitialMetadataReady");
    return;
  }
  error = grpc_error_add_child(GRPC_ERROR_REF(error), calld->error_);
  calld->error_ = GRPC_ERROR_NONE;
  grpc_closure* closure = calld->original_recv_trailing_metadata_ready_;
  calld->original_recv_trailing_metadata_ready_ = nullptr;
  Closure::Run(DEBUG_LOCATION, closure, error);
}

void CallData::MaybeResumeRecvTrailingMetadataReady() {
  if (seen_recv_trailing_metadata_ready_) {
    seen_recv_trailing_metadata_ready_ = false;
    grpc_error_handle error = recv_trailing_metadata_ready_error_;
    recv_trailing_metadata_ready_error_ = GRPC_ERROR_NONE;
    GRPC_CALL_COMBINER_START(call_combiner_, &recv_trailing_metadata_ready_,
                             error, "Continuing RecvTrailingMetadataReady");
  }
}

}  // namespace

const grpc_channel_filter kXdsServerConfigSelectorFilter = {
    CallData::StartTransportStreamOpBatch,
    grpc_channel_next_op,
    sizeof(CallData),
    CallData::Init,
    grpc_call_stack_ignore_set_pollset_or_pollset_set,
    CallData::Destroy,
    sizeof(ChannelData),
    ChannelData::Init,
    ChannelData::Destroy,
    grpc_channel_next_get_info,
    "xds_server_config_selector_filter",
};

}  // namespace grpc_core
