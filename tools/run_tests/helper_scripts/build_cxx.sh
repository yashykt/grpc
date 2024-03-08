#!/bin/bash
# Copyright 2022 The gRPC Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -ex

cd "$(dirname "$0")/../../.."

cd third_party/abseil-cpp
mkdir build
cd build
cmake -D ABSL_BUILD_TESTING=OFF ..
make -j"${GRPC_RUN_TESTS_JOBS}"
make install

cd ../../..
cd third_party/opentelemetry-cpp
mkdir build
cd build
cmake -DWITH_ABSEIL=ON -DBUILD_TESTING=OFF ..
make -j"${GRPC_RUN_TESTS_JOBS}"
make install

cd ../../..
mkdir -p cmake/build
cd cmake/build

# MSBUILD_CONFIG's values are suitable for cmake as well
# TODO(yashkt) - maybe pass absl provider
cmake -DgRPC_BUILD_GRPCPP_OTEL_PLUGIN=ON -DgRPC_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE="${MSBUILD_CONFIG}" "$@" ../..

# GRPC_RUN_TESTS_CXX_LANGUAGE_SUFFIX will be set to either "c" or "cxx"
make -j"${GRPC_RUN_TESTS_JOBS}" "buildtests_${GRPC_RUN_TESTS_CXX_LANGUAGE_SUFFIX}" "tools_${GRPC_RUN_TESTS_CXX_LANGUAGE_SUFFIX}"
