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

#include <grpc/grpc.h>
#include <grpc/support/port_platform.h>

#include "src/core/config/core_configuration.h"

namespace grpc_core {
#ifndef GRPC_NO_XDS
extern void RbacFilterRegister(CoreConfiguration::Builder* builder);
extern void StatefulSessionFilterRegister(CoreConfiguration::Builder* builder);
extern void GcpAuthenticationFilterRegister(
    CoreConfiguration::Builder* builder);
extern void RegisterXdsChannelStackModifier(
    CoreConfiguration::Builder* builder);
extern void RegisterChannelDefaultCreds(CoreConfiguration::Builder* builder);
extern void RegisterDefaultCallCreds(CoreConfiguration::Builder* builder);
extern void RegisterXdsResolver(CoreConfiguration::Builder* builder);
extern void RegisterCloud2ProdResolver(CoreConfiguration::Builder* builder);
extern void RegisterXdsClusterManagerLbPolicy(
    CoreConfiguration::Builder* builder);
extern void RegisterXdsClusterImplLbPolicy(CoreConfiguration::Builder* builder);
extern void RegisterCdsLbPolicy(CoreConfiguration::Builder* builder);
extern void RegisterXdsOverrideHostLbPolicy(
    CoreConfiguration::Builder* builder);
extern void RegisterXdsWrrLocalityLbPolicy(CoreConfiguration::Builder* builder);
extern void RegisterFileWatcherCertificateProvider(
    CoreConfiguration::Builder* builder);
extern void RegisterXdsHttpProxyMapper(CoreConfiguration::Builder* builder);
#endif
void RegisterExtraFilters(CoreConfiguration::Builder* builder) {
  // Use builder to avoid unused-parameter warning.
  (void)builder;
#ifndef GRPC_NO_XDS
  // rbac_filter is being guarded with GRPC_NO_XDS to avoid a dependency on the
  // re2 library by default
  RbacFilterRegister(builder);
  StatefulSessionFilterRegister(builder);
  GcpAuthenticationFilterRegister(builder);
  RegisterXdsChannelStackModifier(builder);
  RegisterChannelDefaultCreds(builder);
  RegisterDefaultCallCreds(builder);
  RegisterXdsResolver(builder);
  RegisterCloud2ProdResolver(builder);
  RegisterXdsClusterManagerLbPolicy(builder);
  RegisterXdsClusterImplLbPolicy(builder);
  RegisterCdsLbPolicy(builder);
  RegisterXdsOverrideHostLbPolicy(builder);
  RegisterXdsWrrLocalityLbPolicy(builder);
  RegisterFileWatcherCertificateProvider(builder);
  RegisterXdsHttpProxyMapper(builder);
#endif
}
}  // namespace grpc_core
