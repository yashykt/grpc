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

#ifndef GRPC_CORE_LIB_CHANNEL_SERVER_CONFIG_SELECTOR_H
#define GRPC_CORE_LIB_CHANNEL_SERVER_CONFIG_SELECTOR_H

// Similar to ServiceConfigCallData on the client-side. This will be used by the
// filters in the filter stack. In the future, this might be expanded so as to
// be able to configure other filters too similar to clients.
class ServerConfigCallData {
  ServiceConfigParser::ParsedConfig* GetMethodParsedConfig(size_t index);
};

// Similar to ConfigSelector on the client-side. Invoked by the config selector
// filter per call.
class ServerConfigSelector {
  struct CallConfig {
    grpc_error_handle error;
    ServerConfigParser::ParsedConfigVector* method_configs = nullptr;
    RefCountedPtr<ServiceConfig> service_config;
  };
  CallConfig GetCallConfig(grpc_slice* path, grpc_metadata_batch* metadata,
                           Arena* arena) = 0;
};

#endif  // GRPC_CORE_LIB_CHANNEL_SERVER_CONFIG_SELECTOR_H
