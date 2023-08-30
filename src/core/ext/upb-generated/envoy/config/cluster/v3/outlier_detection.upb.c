/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     envoy/config/cluster/v3/outlier_detection.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include <stddef.h>
#include "upb/generated_code_support.h"
#include "envoy/config/cluster/v3/outlier_detection.upb.h"
#include "google/protobuf/duration.upb.h"
#include "google/protobuf/wrappers.upb.h"
#include "udpa/annotations/status.upb.h"
#include "udpa/annotations/versioning.upb.h"
#include "validate/validate.upb.h"

// Must be last.
#include "upb/port/def.inc"

static const upb_MiniTableSub envoy_config_cluster_v3_OutlierDetection_submsgs[21] = {
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_Duration_msg_init},
  {.submsg = &google_protobuf_Duration_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_UInt32Value_msg_init},
  {.submsg = &google_protobuf_Duration_msg_init},
  {.submsg = &google_protobuf_Duration_msg_init},
};

static const upb_MiniTableField envoy_config_cluster_v3_OutlierDetection__fields[22] = {
  {1, UPB_SIZE(4, 8), 1, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 16), 2, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(12, 24), 3, 2, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(16, 32), 4, 3, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(20, 40), 5, 4, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(24, 48), 6, 5, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(28, 56), 7, 6, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(32, 64), 8, 7, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(36, 72), 9, 8, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(40, 80), 10, 9, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {11, UPB_SIZE(44, 88), 11, 10, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {12, UPB_SIZE(48, 3), 0, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {13, UPB_SIZE(52, 96), 12, 11, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {14, UPB_SIZE(56, 104), 13, 12, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {15, UPB_SIZE(60, 112), 14, 13, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {16, UPB_SIZE(64, 120), 15, 14, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {17, UPB_SIZE(68, 128), 16, 15, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {18, UPB_SIZE(72, 136), 17, 16, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {19, UPB_SIZE(76, 144), 18, 17, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {20, UPB_SIZE(80, 152), 19, 18, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {21, UPB_SIZE(84, 160), 20, 19, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {22, UPB_SIZE(88, 168), 21, 20, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_cluster_v3_OutlierDetection_msg_init = {
  &envoy_config_cluster_v3_OutlierDetection_submsgs[0],
  &envoy_config_cluster_v3_OutlierDetection__fields[0],
  UPB_SIZE(96, 176), 22, kUpb_ExtMode_NonExtendable, 22, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_psm_1bt_maxmaxb},
    {0x0010000002010012, &upb_psm_1bt_maxmaxb},
    {0x001800000302001a, &upb_psm_1bt_maxmaxb},
    {0x0020000004030022, &upb_psm_1bt_maxmaxb},
    {0x002800000504002a, &upb_psm_1bt_maxmaxb},
    {0x0030000006050032, &upb_psm_1bt_maxmaxb},
    {0x003800000706003a, &upb_psm_1bt_maxmaxb},
    {0x0040000008070042, &upb_psm_1bt_maxmaxb},
    {0x004800000908004a, &upb_psm_1bt_maxmaxb},
    {0x005000000a090052, &upb_psm_1bt_maxmaxb},
    {0x005800000b0a005a, &upb_psm_1bt_maxmaxb},
    {0x000300003f000060, &upb_psb1_1bt},
    {0x006000000c0b006a, &upb_psm_1bt_maxmaxb},
    {0x006800000d0c0072, &upb_psm_1bt_maxmaxb},
    {0x007000000e0d007a, &upb_psm_1bt_maxmaxb},
    {0x007800000f0e0182, &upb_psm_2bt_maxmaxb},
    {0x00800000100f018a, &upb_psm_2bt_maxmaxb},
    {0x0088000011100192, &upb_psm_2bt_maxmaxb},
    {0x009000001211019a, &upb_psm_2bt_maxmaxb},
    {0x00980000131201a2, &upb_psm_2bt_maxmaxb},
    {0x00a00000141301aa, &upb_psm_2bt_maxmaxb},
    {0x00a80000151401b2, &upb_psm_2bt_maxmaxb},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTable *messages_layout[1] = {
  &envoy_config_cluster_v3_OutlierDetection_msg_init,
};

const upb_MiniTableFile envoy_config_cluster_v3_outlier_detection_proto_upb_file_layout = {
  messages_layout,
  NULL,
  NULL,
  1,
  0,
  0,
};

#include "upb/port/undef.inc"

