//
// Copyright 2021 gRPC authors.
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

#include "src/core/ext/xds/xds_http_rbac_filter.h"

#include "envoy/config/core/v3/address.upb.h"
#include "envoy/config/rbac/v3/rbac.upb.h"
#include "envoy/config/route/v3/route_components.upb.h"
#include "envoy/extensions/filters/http/rbac/v3/rbac.upb.h"
#include "envoy/extensions/filters/http/rbac/v3/rbac.upbdefs.h"
#include "envoy/type/matcher/v3/path.upb.h"
#include "envoy/type/matcher/v3/regex.upb.h"
#include "envoy/type/matcher/v3/string.upb.h"
#include "envoy/type/v3/range.upb.h"
#include "google/protobuf/wrappers.upb.h"

#include "src/core/ext/filters/rbac/rbac_filter.h"
#include "src/core/ext/filters/rbac/rbac_service_config_parser.h"
#include "src/core/lib/channel/channel_args.h"

namespace grpc_core {

const char* kXdsHttpRbacFilterConfigName =
    "envoy.extensions.filters.http.rbac.v3.RBAC";

namespace {

inline std::string UpbStringToStdString(const upb_strview& str) {
  return std::string(str.data, str.size);
}

Json ParseRegexMatcherToJson(
    const envoy_type_matcher_v3_RegexMatcher* regex_matcher) {
  return Json::Object(
      {{"regex", UpbStringToStdString(envoy_type_matcher_v3_RegexMatcher_regex(
                     regex_matcher))}});
}

Json ParseInt64RangeToJson(const envoy_type_v3_Int64Range* range) {
  return Json::Object{{"start", envoy_type_v3_Int64Range_start(range)},
                      {"end", envoy_type_v3_Int64Range_end(range)}};
}

absl::StatusOr<Json> ParseHeaderMatcherToJson(
    const envoy_config_route_v3_HeaderMatcher* header) {
  Json::Object header_json;
  std::string name =
      UpbStringToStdString(envoy_config_route_v3_HeaderMatcher_name(header));
  if (name == ":scheme") {
    return absl::InvalidArgumentError("':scheme' not allowed in header");
  }
  if (absl::StartsWith(name, "grpc-")) {
    return absl::InvalidArgumentError("'grpc-' prefixes not allowed in header");
  }
  header_json.emplace("name", std::move(name));
  if (envoy_config_route_v3_HeaderMatcher_has_exact_match(header)) {
    header_json.emplace(
        "exactMatch",
        UpbStringToStdString(
            envoy_config_route_v3_HeaderMatcher_exact_match(header)));
  } else if (envoy_config_route_v3_HeaderMatcher_has_safe_regex_match(header)) {
    header_json.emplace(
        "safeRegexMatch",
        ParseRegexMatcherToJson(
            envoy_config_route_v3_HeaderMatcher_safe_regex_match(header)));
  } else if (envoy_config_route_v3_HeaderMatcher_has_range_match(header)) {
    header_json.emplace(
        "rangeMatch",
        ParseInt64RangeToJson(
            envoy_config_route_v3_HeaderMatcher_range_match(header)));
  } else if (envoy_config_route_v3_HeaderMatcher_has_present_match(header)) {
    header_json.emplace(
        "presentMatch",
        envoy_config_route_v3_HeaderMatcher_present_match(header));
  } else if (envoy_config_route_v3_HeaderMatcher_has_prefix_match(header)) {
    header_json.emplace(
        "prefixMatch",
        UpbStringToStdString(
            envoy_config_route_v3_HeaderMatcher_prefix_match(header)));
  } else if (envoy_config_route_v3_HeaderMatcher_has_suffix_match(header)) {
    header_json.emplace(
        "suffixMatch",
        UpbStringToStdString(
            envoy_config_route_v3_HeaderMatcher_suffix_match(header)));
  } else if (envoy_config_route_v3_HeaderMatcher_has_contains_match(header)) {
    header_json.emplace(
        "containsMatch",
        UpbStringToStdString(
            envoy_config_route_v3_HeaderMatcher_contains_match(header)));
  } else {
    return absl::InvalidArgumentError(
        "Invalid route header matcher specified.");
  }
  header_json.emplace("invertMatch",
                      envoy_config_route_v3_HeaderMatcher_invert_match(header));
  return header_json;
}

absl::StatusOr<Json> ParseStringMatcherToJson(
    const envoy_type_matcher_v3_StringMatcher* matcher) {
  Json::Object json;
  if (envoy_type_matcher_v3_StringMatcher_has_exact(matcher)) {
    json.emplace("exact",
                 UpbStringToStdString(
                     envoy_type_matcher_v3_StringMatcher_exact(matcher)));
  } else if (envoy_type_matcher_v3_StringMatcher_has_prefix(matcher)) {
    json.emplace("prefix",
                 UpbStringToStdString(
                     envoy_type_matcher_v3_StringMatcher_prefix(matcher)));
  } else if (envoy_type_matcher_v3_StringMatcher_has_suffix(matcher)) {
    json.emplace("suffix",
                 UpbStringToStdString(
                     envoy_type_matcher_v3_StringMatcher_suffix(matcher)));
  } else if (envoy_type_matcher_v3_StringMatcher_has_safe_regex(matcher)) {
    json.emplace("safeRegex",
                 ParseRegexMatcherToJson(
                     envoy_type_matcher_v3_StringMatcher_safe_regex(matcher)));
  } else if (envoy_type_matcher_v3_StringMatcher_has_contains(matcher)) {
    json.emplace("contains",
                 UpbStringToStdString(
                     envoy_type_matcher_v3_StringMatcher_contains(matcher)));
  } else {
    return absl::InvalidArgumentError("StringMatcher: Invalid match pattern");
  }
  json.emplace("ignoreCase",
               envoy_type_matcher_v3_StringMatcher_ignore_case(matcher));
  return json;
}

absl::StatusOr<Json> ParsePathMatcherToJson(
    const envoy_type_matcher_v3_PathMatcher* matcher) {
  const auto* path = envoy_type_matcher_v3_PathMatcher_path(matcher);
  if (path == nullptr) {
    return absl::InvalidArgumentError("PathMatcher has empty path");
  }
  Json::Object json;
  auto path_json = ParseStringMatcherToJson(path);
  if (!path_json.ok()) {
    return path_json;
  }
  json.emplace("path", std::move(*path_json));
  return json;
}

Json ParseUInt32ValueToJson(const google_protobuf_UInt32Value* value) {
  return Json::Object{{"value", google_protobuf_UInt32Value_value(value)}};
}

Json ParseCidrRangeToJson(const envoy_config_core_v3_CidrRange* range) {
  Json::Object json;
  json.emplace("addressPrefix",
               UpbStringToStdString(
                   envoy_config_core_v3_CidrRange_address_prefix(range)));
  const auto* prefix_len = envoy_config_core_v3_CidrRange_prefix_len(range);
  if (prefix_len != nullptr) {
    json.emplace("prefixLen", ParseUInt32ValueToJson(prefix_len));
  }
  return json;
}

absl::StatusOr<Json> ParsePermissionToJson(
    const envoy_config_rbac_v3_Permission* permission) {
  Json::Object permission_json;
  // Helper function to parse Permission::Set to JSON. Used by `and_rules` and
  // `or_rules`.
  auto parse_permission_set_to_json =
      [](const envoy_config_rbac_v3_Permission_Set* set)
      -> absl::StatusOr<Json> {
    Json::Object json;
    auto* rules_json =
        json.emplace("rules", Json::Array()).first->second.mutable_array();
    size_t size;
    const envoy_config_rbac_v3_Permission* const* rules =
        envoy_config_rbac_v3_Permission_Set_rules(set, &size);
    for (size_t i = 0; i < size; ++i) {
      auto permission_json = ParsePermissionToJson(rules[i]);
      if (!permission_json.ok()) {
        return permission_json;
      }
      rules_json->emplace_back(std::move(*permission_json));
    }
    return json;
  };
  if (envoy_config_rbac_v3_Permission_has_and_rules(permission)) {
    const auto* and_rules =
        envoy_config_rbac_v3_Permission_and_rules(permission);
    auto permission_set_json = parse_permission_set_to_json(and_rules);
    if (!permission_set_json.ok()) {
      return permission_set_json;
    }
    permission_json.emplace("andRules", std::move(*permission_set_json));
  } else if (envoy_config_rbac_v3_Permission_has_or_rules(permission)) {
    const auto* or_rules = envoy_config_rbac_v3_Permission_or_rules(permission);
    auto permission_set_json = parse_permission_set_to_json(or_rules);
    if (!permission_set_json.ok()) {
      return permission_set_json;
    }
    permission_json.emplace("orRules", std::move(*permission_set_json));
  } else if (envoy_config_rbac_v3_Permission_has_any(permission)) {
    permission_json.emplace("any",
                            envoy_config_rbac_v3_Permission_any(permission));
  } else if (envoy_config_rbac_v3_Permission_has_header(permission)) {
    auto header_json = ParseHeaderMatcherToJson(
        envoy_config_rbac_v3_Permission_header(permission));
    if (!header_json.ok()) {
      return header_json;
    }
    permission_json.emplace("header", std::move(*header_json));
  } else if (envoy_config_rbac_v3_Permission_has_url_path(permission)) {
    auto url_path_json = ParsePathMatcherToJson(
        envoy_config_rbac_v3_Permission_url_path(permission));
    if (!url_path_json.ok()) {
      return url_path_json;
    }
    permission_json.emplace("urlPath", std::move(*url_path_json));
  } else if (envoy_config_rbac_v3_Permission_has_destination_ip(permission)) {
    permission_json.emplace(
        "destinationIp",
        ParseCidrRangeToJson(
            envoy_config_rbac_v3_Permission_destination_ip(permission)));
  } else if (envoy_config_rbac_v3_Permission_has_destination_port(permission)) {
    permission_json.emplace(
        "destinationPort",
        envoy_config_rbac_v3_Permission_destination_port(permission));
  } else if (envoy_config_rbac_v3_Permission_has_metadata(permission)) {
    // Not parsing metadata even if its present since it is not relevant to
    // gRPC.
    permission_json.emplace("metadata", Json::Object());
  } else if (envoy_config_rbac_v3_Permission_has_not_rule(permission)) {
    auto not_rule_json = ParsePermissionToJson(
        envoy_config_rbac_v3_Permission_not_rule(permission));
    if (!not_rule_json.ok()) {
      return not_rule_json;
    }
    permission_json.emplace("notRule", std::move(*not_rule_json));
  } else if (envoy_config_rbac_v3_Permission_has_requested_server_name(
                 permission)) {
    auto requested_server_name_json = ParseStringMatcherToJson(
        envoy_config_rbac_v3_Permission_requested_server_name(permission));
    if (!requested_server_name_json.ok()) {
      return requested_server_name_json;
    }
    permission_json.emplace("requestedServerNameJson",
                            std::move(*requested_server_name_json));
  } else {
    return absl::InvalidArgumentError("Permission: Invalid rule");
  }
  return permission_json;
}

absl::StatusOr<Json> ParsePrincipalToJson(
    const envoy_config_rbac_v3_Principal* principal) {
  Json::Object principal_json;
  // Helper function to parse Principal::Set to JSON. Used by `and_ids` and
  // `or_ids`.
  auto parse_principal_set_to_json =
      [](const envoy_config_rbac_v3_Principal_Set* set)
      -> absl::StatusOr<Json> {
    Json::Object json;
    auto* ids_json =
        json.emplace("ids", Json::Array()).first->second.mutable_array();
    size_t size;
    const envoy_config_rbac_v3_Principal* const* ids =
        envoy_config_rbac_v3_Principal_Set_ids(set, &size);
    for (size_t i = 0; i < size; ++i) {
      auto principal_json = ParsePrincipalToJson(ids[i]);
      if (!principal_json.ok()) {
        return principal_json;
      }
      ids_json->emplace_back(std::move(*principal_json));
    }
    return json;
  };
  if (envoy_config_rbac_v3_Principal_has_and_ids(principal)) {
    const auto* and_rules = envoy_config_rbac_v3_Principal_and_ids(principal);
    auto principal_set_json = parse_principal_set_to_json(and_rules);
    if (!principal_set_json.ok()) {
      return principal_set_json;
    }
    principal_json.emplace("andIds", std::move(*principal_set_json));
  } else if (envoy_config_rbac_v3_Principal_has_or_ids(principal)) {
    const auto* or_rules = envoy_config_rbac_v3_Principal_or_ids(principal);
    auto principal_set_json = parse_principal_set_to_json(or_rules);
    if (!principal_set_json.ok()) {
      return principal_set_json;
    }
    principal_json.emplace("orIds", std::move(*principal_set_json));
  } else if (envoy_config_rbac_v3_Principal_has_any(principal)) {
    principal_json.emplace("any",
                           envoy_config_rbac_v3_Principal_any(principal));
  } else if (envoy_config_rbac_v3_Principal_has_authenticated(principal)) {
    auto* authenticated_json =
        principal_json.emplace("authenticated", Json::Object())
            .first->second.mutable_object();
    const auto* principal_name =
        envoy_config_rbac_v3_Principal_Authenticated_principal_name(
            envoy_config_rbac_v3_Principal_authenticated(principal));
    if (principal_name != nullptr) {
      auto principal_name_json = ParseStringMatcherToJson(principal_name);
      if (!principal_name_json.ok()) {
        return principal_name_json;
      }
      authenticated_json->emplace("principalName",
                                  std::move(*principal_name_json));
    }
  } else if (envoy_config_rbac_v3_Principal_has_source_ip(principal)) {
    principal_json.emplace(
        "sourceIp", ParseCidrRangeToJson(
                        envoy_config_rbac_v3_Principal_source_ip(principal)));
  } else if (envoy_config_rbac_v3_Principal_has_direct_remote_ip(principal)) {
    principal_json.emplace(
        "directRemoteIp",
        ParseCidrRangeToJson(
            envoy_config_rbac_v3_Principal_direct_remote_ip(principal)));
  } else if (envoy_config_rbac_v3_Principal_has_remote_ip(principal)) {
    principal_json.emplace(
        "remoteIp", ParseCidrRangeToJson(
                        envoy_config_rbac_v3_Principal_remote_ip(principal)));
  } else if (envoy_config_rbac_v3_Principal_has_header(principal)) {
    auto header_json = ParseHeaderMatcherToJson(
        envoy_config_rbac_v3_Principal_header(principal));
    if (!header_json.ok()) {
      return header_json;
    }
    principal_json.emplace("header", std::move(*header_json));
  } else if (envoy_config_rbac_v3_Principal_has_url_path(principal)) {
    auto url_path_json = ParsePathMatcherToJson(
        envoy_config_rbac_v3_Principal_url_path(principal));
    if (!url_path_json.ok()) {
      return url_path_json;
    }
    principal_json.emplace("urlPath", std::move(*url_path_json));
  } else if (envoy_config_rbac_v3_Principal_has_metadata(principal)) {
    // Not parsing metadata even if its present since it is not relevant to
    // gRPC.
    principal_json.emplace("metadata", Json::Object());
  } else if (envoy_config_rbac_v3_Principal_has_not_id(principal)) {
    auto not_id_json =
        ParsePrincipalToJson(envoy_config_rbac_v3_Principal_not_id(principal));
    if (!not_id_json.ok()) {
      return not_id_json;
    }
    principal_json.emplace("notId", std::move(*not_id_json));
  } else {
    return absl::InvalidArgumentError("Principal: Invalid rule");
  }
  return principal_json;
}

absl::StatusOr<Json> ParsePolicyToJson(
    const envoy_config_rbac_v3_Policy* policy) {
  Json::Object policy_json;
  size_t size;
  auto* permissions_json = policy_json.emplace("permissions", Json::Array())
                               .first->second.mutable_array();
  const envoy_config_rbac_v3_Permission* const* permissions =
      envoy_config_rbac_v3_Policy_permissions(policy, &size);
  for (size_t i = 0; i < size; ++i) {
    auto permission_json = ParsePermissionToJson(permissions[i]);
    if (!permission_json.ok()) {
      return permission_json;
    }
    permissions_json->emplace_back(std::move(*permission_json));
  }
  auto* principals_json = policy_json.emplace("principals", Json::Array())
                              .first->second.mutable_array();
  const envoy_config_rbac_v3_Principal* const* principals =
      envoy_config_rbac_v3_Policy_principals(policy, &size);
  for (size_t i = 0; i < size; ++i) {
    auto principal_json = ParsePrincipalToJson(principals[i]);
    if (!principal_json.ok()) {
      return principal_json;
    }
    principals_json->emplace_back(std::move(*principal_json));
  }
  if (envoy_config_rbac_v3_Policy_has_condition(policy)) {
    return absl::InvalidArgumentError("Policy: condition not supported");
  }
  if (envoy_config_rbac_v3_Policy_has_checked_condition(policy)) {
    return absl::InvalidArgumentError(
        "Policy: checked condition not supported");
  }
  return policy_json;
}

absl::StatusOr<Json> ParseHttpRbacIntoJson(upb_strview serialized_rbac,
                                           upb_arena* arena) {
  auto* rbac = envoy_extensions_filters_http_rbac_v3_RBAC_parse(
      serialized_rbac.data, serialized_rbac.size, arena);
  if (rbac == nullptr) {
    return absl::InvalidArgumentError(
        "could not parse HTTP RBAC filter config");
  }
  Json::Object rbac_json;
  const auto* rules = envoy_extensions_filters_http_rbac_v3_RBAC_rules(rbac);
  if (rules != nullptr) {
    int action = envoy_config_rbac_v3_RBAC_action(rules);
    // Treat Log action as RBAC being absent
    if (action == envoy_config_rbac_v3_RBAC_LOG) {
      return rbac_json;
    }
    auto* inner_rbac_json = rbac_json.emplace("rules", Json::Object())
                                .first->second.mutable_object();
    inner_rbac_json->emplace("action", envoy_config_rbac_v3_RBAC_action(rules));
    if (envoy_config_rbac_v3_RBAC_has_policies(rules)) {
      auto* policies_object =
          inner_rbac_json->emplace("policies", Json::Object())
              .first->second.mutable_object();
      size_t iter = UPB_MAP_BEGIN;
      auto* entry = envoy_config_rbac_v3_RBAC_policies_next(rules, &iter);
      while (entry != nullptr) {
        auto policy = ParsePolicyToJson(
            envoy_config_rbac_v3_RBAC_PoliciesEntry_value(entry));
        if (!policy.ok()) {
          return policy;
        }
        policies_object->emplace(
            UpbStringToStdString(
                envoy_config_rbac_v3_RBAC_PoliciesEntry_key(entry)),
            std::move(*policy));
        entry = envoy_config_rbac_v3_RBAC_policies_next(rules, &iter);
      }
    }
  }
  return rbac_json;
}

}  // namespace

void XdsHttpRbacFilter::PopulateSymtab(upb_symtab* symtab) const {
  envoy_extensions_filters_http_rbac_v3_RBAC_getmsgdef(symtab);
}

absl::StatusOr<XdsHttpFilterImpl::FilterConfig>
XdsHttpRbacFilter::GenerateFilterConfig(upb_strview serialized_filter_config,
                                        upb_arena* arena) const {
  absl::StatusOr<Json> parse_result =
      ParseHttpRbacIntoJson(serialized_filter_config, arena);
  if (!parse_result.ok()) {
    return parse_result.status();
  }
  return FilterConfig{kXdsHttpRbacFilterConfigName, std::move(*parse_result)};
}

absl::StatusOr<XdsHttpFilterImpl::FilterConfig>
XdsHttpRbacFilter::GenerateFilterConfigOverride(
    upb_strview serialized_filter_config, upb_arena* arena) const {
  return GenerateFilterConfig(serialized_filter_config, arena);
}

const grpc_channel_filter* XdsHttpRbacFilter::channel_filter() const {
  return &kRbacFilter;
}

grpc_channel_args* XdsHttpRbacFilter::ModifyChannelArgs(
    grpc_channel_args* args) const {
  grpc_arg arg_to_add = grpc_channel_arg_integer_create(
      const_cast<char*>(GRPC_ARG_PARSE_RBAC_METHOD_CONFIG), 1);
  grpc_channel_args* new_args =
      grpc_channel_args_copy_and_add(args, &arg_to_add, 1);
  grpc_channel_args_destroy(args);
  return new_args;
}

absl::StatusOr<XdsHttpFilterImpl::ServiceConfigJsonEntry>
XdsHttpRbacFilter::GenerateServiceConfig(
    const FilterConfig& hcm_filter_config,
    const FilterConfig* filter_config_override) const {
  Json policy_json = filter_config_override != nullptr
                         ? filter_config_override->config
                         : hcm_filter_config.config;
  // The policy JSON may be empty, that's allowed.
  return ServiceConfigJsonEntry{"rbacPolicy", policy_json.Dump()};
}

}  // namespace grpc_core
