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

#include "src/core/ext/filters/rbac/rbac_service_config_parser.h"

#include "absl/strings/str_format.h"

#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/json/json_util.h"

namespace grpc_core {

namespace {

size_t g_rbac_parser_index;

std::string ParseRegexMatcher(const Json::Object& regex_matcher_json,
                              std::vector<grpc_error_handle>* error_list) {
  std::string regex;
  ParseJsonObjectField(regex_matcher_json, "regex", &regex, error_list);
  return regex;
}

absl::StatusOr<HeaderMatcher> ParseHeaderMatcher(
    const Json::Object& header_matcher_json,
    std::vector<grpc_error_handle>* error_list) {
  std::string name;
  ParseJsonObjectField(header_matcher_json, "name", &name, error_list);
  std::string match;
  HeaderMatcher::Type type;
  const Json::Object* inner_json;
  int64_t start = 0;
  int64_t end = 0;
  bool present_match = false;
  bool invert_match;
  ParseJsonObjectField(header_matcher_json, "invertMatch", &invert_match,
                       error_list);
  if (ParseJsonObjectField(header_matcher_json, "exactMatch", &match,
                           error_list)) {
    type = HeaderMatcher::Type::kExact;
  } else if (ParseJsonObjectField(header_matcher_json, "safeRegexMatch",
                                  &inner_json, error_list)) {
    type = HeaderMatcher::Type::kSafeRegex;
    std::vector<grpc_error_handle> safe_regex_matcher_error_list;
    match = ParseRegexMatcher(*inner_json, &safe_regex_matcher_error_list);
    if (!safe_regex_matcher_error_list.empty()) {
      error_list->push_back(GRPC_ERROR_CREATE_FROM_VECTOR(
          "safeRegexMatch", &safe_regex_matcher_error_list));
    }
  } else if (ParseJsonObjectField(header_matcher_json, "rangeMatch",
                                  &inner_json, error_list)) {
    type = HeaderMatcher::Type::kRange;
    std::vector<grpc_error_handle> range_error_list;
    ParseJsonObjectField(*inner_json, "start", &start, &range_error_list);
    ParseJsonObjectField(*inner_json, "end", &end, &range_error_list);
    if (!range_error_list.empty()) {
      error_list->push_back(
          GRPC_ERROR_CREATE_FROM_VECTOR("rangeMatch", &range_error_list));
    }
  } else if (ParseJsonObjectField(header_matcher_json, "presentMatch",
                                  &present_match, error_list)) {
    type = HeaderMatcher::Type::kPresent;
  } else if (ParseJsonObjectField(header_matcher_json, "prefixMatch", &match,
                                  error_list)) {
    type = HeaderMatcher::Type::kPrefix;
  } else if (ParseJsonObjectField(header_matcher_json, "suffixMatch", &match,
                                  error_list)) {
    type = HeaderMatcher::Type::kSuffix;
  } else if (ParseJsonObjectField(header_matcher_json, "containsMatch", &match,
                                  error_list)) {
    type = HeaderMatcher::Type::kContains;
  } else {
    return absl::InvalidArgumentError("No valid matcher found");
  }
  return HeaderMatcher::Create(name, type, match, start, end, present_match,
                               invert_match);
}

std::unique_ptr<Rbac::Permission> ParsePermission(
    const Json::Object& permission_json,
    std::vector<grpc_error_handle>* error_list) {
  auto parse_permission_set = [](const Json::Object& permission_set_json,
                                 std::vector<grpc_error_handle>* error_list) {
    const Json::Array* rules_json;
    std::vector<std::unique_ptr<Rbac::Permission>> permissions;
    if (ParseJsonObjectField(permission_set_json, "rules", &rules_json,
                             error_list)) {
      for (size_t i = 0; i < rules_json->size(); ++i) {
        const Json::Object* permission_json;
        if (!ExtractJsonType(rules_json[i],
                             absl::StrFormat("rules[%d]", i).c_str(),
                             &permission_json, error_list)) {
          continue;
        }
        std::vector<grpc_error_handle> permission_error_list;
        permissions.emplace_back(
            ParsePermission(*permission_json, &permission_error_list));
        if (!permission_error_list.empty()) {
          error_list->push_back(GRPC_ERROR_CREATE_FROM_VECTOR(
              absl::StrFormat("rules[%d]", i).c_str(), &permission_error_list));
        }
      }
    }
    return permissions;
  };
  const Json::Object* and_rules_json;
  if (ParseJsonObjectField(permission_json, "andRules", &and_rules_json,
                           error_list, /*required=*/false)) {
    std::vector<grpc_error_handle> and_rules_error_list;
    auto permission = absl::make_unique<Rbac::Permission>(
        Rbac::Permission::RuleType::kAnd,
        parse_permission_set(*and_rules_json, &and_rules_error_list));
    if (!and_rules_error_list.empty()) {
      error_list->push_back(
          GRPC_ERROR_CREATE_FROM_VECTOR("and_rules", &and_rules_error_list));
    }
    return permission;
  }
  const Json::Object* or_rules_json;
  if (ParseJsonObjectField(permission_json, "orRules", &or_rules_json,
                           error_list, /*required=*/false)) {
    std::vector<grpc_error_handle> or_rules_error_list;
    auto permission = absl::make_unique<Rbac::Permission>(
        Rbac::Permission::RuleType::kOr,
        parse_permission_set(*or_rules_json, &or_rules_error_list));
    if (!or_rules_error_list.empty()) {
      error_list->push_back(
          GRPC_ERROR_CREATE_FROM_VECTOR("and_rules", &or_rules_error_list));
    }
    return permission;
  }
  bool any;
  if (ParseJsonObjectField(permission_json, "any", &any, error_list,
                           /*required=*/false) &&
      any) {
    return absl::make_unique<Rbac::Permission>(
        Rbac::Permission::RuleType::kAny);
  }
  const Json::Object* header_json;
  if (ParseJsonObjectField(permission_json, "header", &header_json, error_list,
                           /*required=*/false)) {
    std::vector<grpc_error_handle> header_error_list;
    auto permission = absl::make_unique<Rbac::Permission>(
        Rbac::Permission::RuleType::kHeader,
        ParseHeaderMatcher(*header_json, &header_error_list));
    if (!header_error_list.empty()) {
      error_list->push_back(
          GRPC_ERROR_CREATE_FROM_VECTOR("header", &header_error_list));
    }
  }
}

Rbac::Policy ParsePolicy(const Json::Object& policy_json,
                         std::vector<grpc_error_handle>* error_list) {
  Rbac::Policy policy;
  const Json::Array* permissions_json_array;
  if (ParseJsonObjectField(policy_json, "permissions", &permissions_json_array,
                           error_list)) {
    std::vector<std::unique_ptr<Rbac::Permission>> permissions;
    for (size_t i = 0; i < permissions_json_array->size(); ++i) {
      const Json::Object* permission_json;
      if (!ExtractJsonType(permissions_json_array[i],
                           absl::StrFormat("permissions[%d]", i),
                           &permission_json, error_list)) {
        continue;
      }
      std::vector<grpc_error_handle> permission_error_list;
      permissions.emplace_back(
          ParsePermission(*permission_json, &permission_error_list));
      if (!permission_error_list.empty()) {
        error_list->push_back(GRPC_ERROR_CREATE_FROM_VECTOR(
            absl::StrFormat("permissions[%d]", i).c_str(),
            &permission_error_list));
      }
    }
    policy.permissions = Rbac::Permission(Rbac::Permission::RuleType::kOr,
                                          std::move(permissions));
  }
}

Rbac ParseRbac(const Json::Object& rbac_json,
               std::vector<grpc_error_handle>* error_list) {
  Rbac rbac;
  const Json::Object* rules_json;
  if (!ParseJsonObjectField(rbac_json, "rules", &rules_json, error_list,
                            /*required=*/false)) {
    // No enforcing to be applied
    // TODO(review): Verify that an empty allow policy is fine here
    return Rbac(Rbac::Action::kAllow, {});
  }
  int action;
  if (ParseJsonObjectField(*rules_json, "action", &action, error_list)) {
    if (action > 1) {
      error_list->push_back(
          GRPC_ERROR_CREATE_FROM_STATIC_STRING("Unknown action"));
    }
  }
  const Json::Object* policies_json;
  if (ParseJsonObjectField(*rules_json, "policies", &policies_json, error_list,
                           /*required=*/false)) {
    for (const auto& entry : *policies_json) {
      std::vector<grpc_error_handle> policy_error_list;
      rbac.policies.emplace(
          entry.first,
          ParsePolicy(entry.second.object_value(), &policy_error_list));
      if (!policy_error_list.empty()) {
        error_list->push_back(GRPC_ERROR_CREATE_FROM_VECTOR(
            absl::StrFormat("policies key:'%s'", entry.first.c_str()).c_str(),
            &policy_error_list));
      }
    }
  }
  return rbac;
}

std::vector<Rbac> ParseRbacArray(const Json::Array& policies_json_array,
                                 std::vector<grpc_error_handle>* error_list) {
  std::vector<Rbac> policies;
  for (size_t i = 0; i < policies_json_array.size(); ++i) {
    const Json::Object* rbac_json;
    if (!ExtractJsonType(policies_json_array[0],
                         absl::StrFormat("rbacPolicy[%d]", i), &rbac_json,
                         error_list)) {
      continue;
    }
    std::vector<grpc_error_handle> rbac_policy_error_list;
    Rbac rbac = ParseRbac(*rbac_json, &rbac_policy_error_list);
    if (!rbac_policy_error_list.empty()) {
      error_list->push_back(GRPC_ERROR_CREATE_FROM_VECTOR(
          absl::StrFormat("rbacPolicy[%d]", i).c_str(),
          &rbac_policy_error_list));
    } else {
      policies.emplace_back(std::move(rbac));
    }
  }
  return policies;
}

}  // namespace

std::unique_ptr<ServiceConfigParser::ParsedConfig>
RbacServiceConfigParser::ParsePerMethodParams(const grpc_channel_args* args,
                                              const Json& json,
                                              grpc_error_handle* error) {
  GPR_DEBUG_ASSERT(error != nullptr && *error == GRPC_ERROR_NONE);
  // Only parse rbac policy if the channel arg is present
  if (!grpc_channel_args_find_bool(args, GRPC_ARG_PARSE_RBAC_METHOD_CONFIG,
                                   false)) {
    return nullptr;
  }
  std::vector<Rbac> rbac_policies;
  std::vector<grpc_error_handle> error_list;
  const Json::Array* policies_json_array;
  if (ParseJsonObjectField(json.object_value(), "rbacPolicy",
                           &policies_json_array, &error_list)) {
    rbac_policies = ParseRbacArray(*policies_json_array, &error_list);
  }
  *error = GRPC_ERROR_CREATE_FROM_VECTOR("Rbac parser", &error_list);
  if (*error != GRPC_ERROR_NONE || rbac_policies.empty()) {
    return nullptr;
  }
  return absl::make_unique<RbacMethodParsedConfig>(std::move(rbac_policies));
}

void RbacServiceConfigParser::Register() {
  g_rbac_parser_index = ServiceConfigParser::RegisterParser(
      absl::make_unique<RbacServiceConfigParser>());
}

size_t RbacServiceConfigParser::ParserIndex() { return g_rbac_parser_index; }

}  // namespace grpc_core
