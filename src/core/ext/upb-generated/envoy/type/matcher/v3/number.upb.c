/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     envoy/type/matcher/v3/number.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include <stddef.h>
#include "upb/generated_code_support.h"
#include "envoy/type/matcher/v3/number.upb.h"
#include "envoy/type/v3/range.upb.h"
#include "udpa/annotations/status.upb.h"
#include "udpa/annotations/versioning.upb.h"
#include "validate/validate.upb.h"

// Must be last.
#include "upb/port/def.inc"

static const upb_MiniTableSub envoy_type_matcher_v3_DoubleMatcher_submsgs[1] = {
  {.submsg = &envoy_type_v3_DoubleRange_msg_init},
};

static const upb_MiniTableField envoy_type_matcher_v3_DoubleMatcher__fields[2] = {
  {1, 8, -1, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {2, 8, -1, kUpb_NoSub, 1, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable envoy_type_matcher_v3_DoubleMatcher_msg_init = {
  &envoy_type_matcher_v3_DoubleMatcher_submsgs[0],
  &envoy_type_matcher_v3_DoubleMatcher__fields[0],
  16, 2, kUpb_ExtMode_NonExtendable, 2, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pom_1bt_maxmaxb},
    {0x0008000002000011, &upb_pof8_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTable *messages_layout[1] = {
  &envoy_type_matcher_v3_DoubleMatcher_msg_init,
};

const upb_MiniTableFile envoy_type_matcher_v3_number_proto_upb_file_layout = {
  messages_layout,
  NULL,
  NULL,
  1,
  0,
  0,
};

#include "upb/port/undef.inc"

