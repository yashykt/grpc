// Copyright 2024 gRPC authors.
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

#ifndef GRPC_SRC_CPP_EXT_CHAOTIC_GOOD_H
#define GRPC_SRC_CPP_EXT_CHAOTIC_GOOD_H

#include <grpcpp/security/credentials.h>
#include <grpcpp/security/server_credentials.h>

#include <memory>

namespace grpc {

std::shared_ptr<ChannelCredentials> ChaoticGoodInsecureChannelCredentials();
std::shared_ptr<ServerCredentials> ChaoticGoodInsecureServerCredentials();

}  // namespace grpc

#endif  // GRPC_SRC_CPP_EXT_CHAOTIC_GOOD_H
