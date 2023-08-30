/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     envoy/service/status/v3/csds.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include <stddef.h>
#include "upb/generated_code_support.h"
#include "envoy/service/status/v3/csds.upb.h"
#include "envoy/admin/v3/config_dump_shared.upb.h"
#include "envoy/config/core/v3/base.upb.h"
#include "envoy/type/matcher/v3/node.upb.h"
#include "google/api/annotations.upb.h"
#include "google/protobuf/any.upb.h"
#include "google/protobuf/timestamp.upb.h"
#include "envoy/annotations/deprecation.upb.h"
#include "udpa/annotations/status.upb.h"
#include "udpa/annotations/versioning.upb.h"

// Must be last.
#include "upb/port/def.inc"

static const upb_MiniTableSub envoy_service_status_v3_ClientStatusRequest_submsgs[2] = {
  {.submsg = &envoy_type_matcher_v3_NodeMatcher_msg_init},
  {.submsg = &envoy_config_core_v3_Node_msg_init},
};

static const upb_MiniTableField envoy_service_status_v3_ClientStatusRequest__fields[2] = {
  {1, UPB_SIZE(4, 8), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 16), 1, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable envoy_service_status_v3_ClientStatusRequest_msg_init = {
  &envoy_service_status_v3_ClientStatusRequest_submsgs[0],
  &envoy_service_status_v3_ClientStatusRequest__fields[0],
  UPB_SIZE(16, 24), 2, kUpb_ExtMode_NonExtendable, 2, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800003f00000a, &upb_prm_1bt_maxmaxb},
    {0x0010000001010012, &upb_psm_1bt_maxmaxb},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub envoy_service_status_v3_PerXdsConfig_submsgs[5] = {
  {.submsg = &envoy_admin_v3_ListenersConfigDump_msg_init},
  {.submsg = &envoy_admin_v3_ClustersConfigDump_msg_init},
  {.submsg = &envoy_admin_v3_RoutesConfigDump_msg_init},
  {.submsg = &envoy_admin_v3_ScopedRoutesConfigDump_msg_init},
  {.submsg = &envoy_admin_v3_EndpointsConfigDump_msg_init},
};

static const upb_MiniTableField envoy_service_status_v3_PerXdsConfig__fields[7] = {
  {1, 0, 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 16), -5, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(12, 16), -5, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(12, 16), -5, 2, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(12, 16), -5, 3, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(12, 16), -5, 4, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {7, 8, 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable envoy_service_status_v3_PerXdsConfig_msg_init = {
  &envoy_service_status_v3_PerXdsConfig_submsgs[0],
  &envoy_service_status_v3_PerXdsConfig__fields[0],
  UPB_SIZE(16, 24), 7, kUpb_ExtMode_NonExtendable, 7, UPB_FASTTABLE_MASK(56), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000000003f000008, &upb_psv4_1bt},
    {0x0010000402000012, &upb_pom_1bt_maxmaxb},
    {0x001000040301001a, &upb_pom_1bt_maxmaxb},
    {0x0010000404020022, &upb_pom_1bt_maxmaxb},
    {0x001000040503002a, &upb_pom_1bt_maxmaxb},
    {0x0010000406040032, &upb_pom_1bt_maxmaxb},
    {0x000800003f000038, &upb_psv4_1bt},
  })
};

static const upb_MiniTableSub envoy_service_status_v3_ClientConfig_submsgs[3] = {
  {.submsg = &envoy_config_core_v3_Node_msg_init},
  {.submsg = &envoy_service_status_v3_PerXdsConfig_msg_init},
  {.submsg = &envoy_service_status_v3_ClientConfig_GenericXdsConfig_msg_init},
};

static const upb_MiniTableField envoy_service_status_v3_ClientConfig__fields[3] = {
  {1, UPB_SIZE(4, 8), 1, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 16), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(12, 24), 0, 2, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable envoy_service_status_v3_ClientConfig_msg_init = {
  &envoy_service_status_v3_ClientConfig_submsgs[0],
  &envoy_service_status_v3_ClientConfig__fields[0],
  UPB_SIZE(16, 32), 3, kUpb_ExtMode_NonExtendable, 3, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_psm_1bt_maxmaxb},
    {0x001000003f010012, &upb_prm_1bt_max64b},
    {0x001800003f02001a, &upb_prm_1bt_max128b},
  })
};

static const upb_MiniTableSub envoy_service_status_v3_ClientConfig_GenericXdsConfig_submsgs[3] = {
  {.submsg = &google_protobuf_Any_msg_init},
  {.submsg = &google_protobuf_Timestamp_msg_init},
  {.submsg = &envoy_admin_v3_UpdateFailureState_msg_init},
};

static const upb_MiniTableField envoy_service_status_v3_ClientConfig_GenericXdsConfig__fields[9] = {
  {1, UPB_SIZE(28, 16), 0, kUpb_NoSub, 9, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(36, 32), 0, kUpb_NoSub, 9, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(44, 48), 0, kUpb_NoSub, 9, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(4, 64), 1, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(8, 72), 2, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(12, 4), 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(16, 8), 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(20, 80), 3, 2, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(24, 12), 0, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable envoy_service_status_v3_ClientConfig_GenericXdsConfig_msg_init = {
  &envoy_service_status_v3_ClientConfig_GenericXdsConfig_submsgs[0],
  &envoy_service_status_v3_ClientConfig_GenericXdsConfig__fields[0],
  UPB_SIZE(56, 88), 9, kUpb_ExtMode_NonExtendable, 9, UPB_FASTTABLE_MASK(120), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f00000a, &upb_pss_1bt},
    {0x002000003f000012, &upb_pss_1bt},
    {0x003000003f00001a, &upb_pss_1bt},
    {0x0040000001000022, &upb_psm_1bt_maxmaxb},
    {0x004800000201002a, &upb_psm_1bt_maxmaxb},
    {0x000400003f000030, &upb_psv4_1bt},
    {0x000800003f000038, &upb_psv4_1bt},
    {0x0050000003020042, &upb_psm_1bt_maxmaxb},
    {0x000c00003f000048, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub envoy_service_status_v3_ClientStatusResponse_submsgs[1] = {
  {.submsg = &envoy_service_status_v3_ClientConfig_msg_init},
};

static const upb_MiniTableField envoy_service_status_v3_ClientStatusResponse__fields[1] = {
  {1, 0, 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable envoy_service_status_v3_ClientStatusResponse_msg_init = {
  &envoy_service_status_v3_ClientStatusResponse_submsgs[0],
  &envoy_service_status_v3_ClientStatusResponse__fields[0],
  8, 1, kUpb_ExtMode_NonExtendable, 1, UPB_FASTTABLE_MASK(8), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000000003f00000a, &upb_prm_1bt_max64b},
  })
};

static const upb_MiniTable *messages_layout[5] = {
  &envoy_service_status_v3_ClientStatusRequest_msg_init,
  &envoy_service_status_v3_PerXdsConfig_msg_init,
  &envoy_service_status_v3_ClientConfig_msg_init,
  &envoy_service_status_v3_ClientConfig_GenericXdsConfig_msg_init,
  &envoy_service_status_v3_ClientStatusResponse_msg_init,
};

const upb_MiniTableFile envoy_service_status_v3_csds_proto_upb_file_layout = {
  messages_layout,
  NULL,
  NULL,
  5,
  0,
  0,
};

#include "upb/port/undef.inc"

