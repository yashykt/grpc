/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     xds/data/orca/v3/orca_load_report.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include "upb/def.h"
#include "xds/data/orca/v3/orca_load_report.upbdefs.h"
#include "xds/data/orca/v3/orca_load_report.upb.h"

extern upb_def_init validate_validate_proto_upbdefinit;
static const char descriptor[705] = {'\n', '\'', 'x', 'd', 's', '/', 'd', 'a', 't', 'a', '/', 'o', 'r', 'c', 'a', '/', 'v', '3', '/', 'o', 'r', 'c', 'a', '_', 'l', 
'o', 'a', 'd', '_', 'r', 'e', 'p', 'o', 'r', 't', '.', 'p', 'r', 'o', 't', 'o', '\022', '\020', 'x', 'd', 's', '.', 'd', 'a', 't', 
'a', '.', 'o', 'r', 'c', 'a', '.', 'v', '3', '\032', '\027', 'v', 'a', 'l', 'i', 'd', 'a', 't', 'e', '/', 'v', 'a', 'l', 'i', 'd', 
'a', 't', 'e', '.', 'p', 'r', 'o', 't', 'o', '\"', '\203', '\004', '\n', '\016', 'O', 'r', 'c', 'a', 'L', 'o', 'a', 'd', 'R', 'e', 'p', 
'o', 'r', 't', '\022', 'E', '\n', '\017', 'c', 'p', 'u', '_', 'u', 't', 'i', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', '\030', '\001', ' ', 
'\001', '(', '\001', 'B', '\034', '\372', 'B', '\013', '\022', '\t', ')', '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\372', 'B', '\013', '\022', '\t', '\031', 
'\000', '\000', '\000', '\000', '\000', '\000', '\360', '?', 'R', '\016', 'c', 'p', 'u', 'U', 't', 'i', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', '\022', 
'E', '\n', '\017', 'm', 'e', 'm', '_', 'u', 't', 'i', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', '\030', '\002', ' ', '\001', '(', '\001', 'B', 
'\034', '\372', 'B', '\013', '\022', '\t', ')', '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\372', 'B', '\013', '\022', '\t', '\031', '\000', '\000', '\000', '\000', 
'\000', '\000', '\360', '?', 'R', '\016', 'm', 'e', 'm', 'U', 't', 'i', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', '\022', '\020', '\n', '\003', 'r', 
'p', 's', '\030', '\003', ' ', '\001', '(', '\004', 'R', '\003', 'r', 'p', 's', '\022', 'T', '\n', '\014', 'r', 'e', 'q', 'u', 'e', 's', 't', '_', 
'c', 'o', 's', 't', '\030', '\004', ' ', '\003', '(', '\013', '2', '1', '.', 'x', 'd', 's', '.', 'd', 'a', 't', 'a', '.', 'o', 'r', 'c', 
'a', '.', 'v', '3', '.', 'O', 'r', 'c', 'a', 'L', 'o', 'a', 'd', 'R', 'e', 'p', 'o', 'r', 't', '.', 'R', 'e', 'q', 'u', 'e', 
's', 't', 'C', 'o', 's', 't', 'E', 'n', 't', 'r', 'y', 'R', '\013', 'r', 'e', 'q', 'u', 'e', 's', 't', 'C', 'o', 's', 't', '\022', 
'{', '\n', '\013', 'u', 't', 'i', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', '\030', '\005', ' ', '\003', '(', '\013', '2', '1', '.', 'x', 'd', 
's', '.', 'd', 'a', 't', 'a', '.', 'o', 'r', 'c', 'a', '.', 'v', '3', '.', 'O', 'r', 'c', 'a', 'L', 'o', 'a', 'd', 'R', 'e', 
'p', 'o', 'r', 't', '.', 'U', 't', 'i', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', 'E', 'n', 't', 'r', 'y', 'B', '&', '\372', 'B', 
'\020', '\232', '\001', '\r', '*', '\013', '\022', '\t', ')', '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\372', 'B', '\020', '\232', '\001', '\r', '*', '\013', 
'\022', '\t', '\031', '\000', '\000', '\000', '\000', '\000', '\000', '\360', '?', 'R', '\013', 'u', 't', 'i', 'l', 'i', 'z', 'a', 't', 'i', 'o', 'n', '\032', 
'>', '\n', '\020', 'R', 'e', 'q', 'u', 'e', 's', 't', 'C', 'o', 's', 't', 'E', 'n', 't', 'r', 'y', '\022', '\020', '\n', '\003', 'k', 'e', 
'y', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\003', 'k', 'e', 'y', '\022', '\024', '\n', '\005', 'v', 'a', 'l', 'u', 'e', '\030', '\002', ' ', '\001', 
'(', '\001', 'R', '\005', 'v', 'a', 'l', 'u', 'e', ':', '\002', '8', '\001', '\032', '>', '\n', '\020', 'U', 't', 'i', 'l', 'i', 'z', 'a', 't', 
'i', 'o', 'n', 'E', 'n', 't', 'r', 'y', '\022', '\020', '\n', '\003', 'k', 'e', 'y', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\003', 'k', 'e', 
'y', '\022', '\024', '\n', '\005', 'v', 'a', 'l', 'u', 'e', '\030', '\002', ' ', '\001', '(', '\001', 'R', '\005', 'v', 'a', 'l', 'u', 'e', ':', '\002', 
'8', '\001', 'B', ']', '\n', '\033', 'c', 'o', 'm', '.', 'g', 'i', 't', 'h', 'u', 'b', '.', 'x', 'd', 's', '.', 'd', 'a', 't', 'a', 
'.', 'o', 'r', 'c', 'a', '.', 'v', '3', 'B', '\023', 'O', 'r', 'c', 'a', 'L', 'o', 'a', 'd', 'R', 'e', 'p', 'o', 'r', 't', 'P', 
'r', 'o', 't', 'o', 'P', '\001', 'Z', '\'', 'g', 'i', 't', 'h', 'u', 'b', '.', 'c', 'o', 'm', '/', 'c', 'n', 'c', 'f', '/', 'x', 
'd', 's', '/', 'g', 'o', '/', 'x', 'd', 's', '/', 'd', 'a', 't', 'a', '/', 'o', 'r', 'c', 'a', '/', 'v', '3', 'b', '\006', 'p', 
'r', 'o', 't', 'o', '3', 
};

static upb_def_init *deps[2] = {
  &validate_validate_proto_upbdefinit,
  NULL
};

upb_def_init xds_data_orca_v3_orca_load_report_proto_upbdefinit = {
  deps,
  &xds_data_orca_v3_orca_load_report_proto_upb_file_layout,
  "xds/data/orca/v3/orca_load_report.proto",
  UPB_STRVIEW_INIT(descriptor, 705)
};
