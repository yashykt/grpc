/*
 *
 * Copyright 2023 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"
#include "absl/random/random.h"
#include "absl/strings/str_cat.h"
#include "examples/cpp/otel/util.h"
#include "opentelemetry/exporters/prometheus/exporter_factory.h"
#include "opentelemetry/exporters/prometheus/exporter_options.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"

#include <grpcpp/ext/admin_services.h>
#include <grpcpp/ext/csm_observability.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/xds_server_builder.h>

ABSL_FLAG(int32_t, port, 50051, "Server port for service.");
ABSL_FLAG(std::string, prometheus_endpoint, "localhost:9464",
          "Prometheus exporter endpoint");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  opentelemetry::exporter::metrics::PrometheusExporterOptions opts;
  // default was "localhost:9464" which causes connection issue across GKE pods
  opts.url = "0.0.0.0:9464";
  auto prometheus_exporter =
      opentelemetry::exporter::metrics::PrometheusExporterFactory::Create(opts);
  auto meter_provider =
      std::make_shared<opentelemetry::sdk::metrics::MeterProvider>();
  // The default histogram boundaries are not granular enough for RPCs. Override
  // the "grpc.server.call.duration" view as recommended by
  // https://github.com/grpc/proposal/blob/master/A66-otel-stats.md.
  AddLatencyView(meter_provider.get(), "grpc.server.call.duration", "s");
  meter_provider->AddMetricReader(std::move(prometheus_exporter));
  auto observability = grpc::CsmObservabilityBuilder()
                           .SetMeterProvider(std::move(meter_provider))
                           .BuildAndRegister();
  if (!observability.ok()) {
    std::cerr << "CsmObservability::Init() failed: "
              << observability.status().ToString() << std::endl;
    return static_cast<int>(observability.status().code());
  }
  RunServer(absl::GetFlag(FLAGS_port), []() {
    // We shouldn't create a new bitgen each time for proper usage, but this is
    // probably fine for an example.
    // Sleep between 5 and 15 seconds.
    // We shouldn't be sleeping in this thread either, but it's fine for a low
    // QPS example.
    absl::BitGen bit_gen;
    absl::SleepFor(absl::Seconds(absl::Uniform<uint32_t>(bit_gen, 5, 15)));
    // Sleep an additional 60 seconds for the first minute of an hour.
    if (absl::LocalTimeZone().At(absl::Now()).cs.minute() == 0) {
      absl::SleepFor(absl::Seconds(60));
    }
    return absl::LocalTimeZone().At(absl::Now()).cs.hour() == 0
               ? grpc::Status(grpc::StatusCode::UNAVAILABLE,
                              "Server maintenance")
               : grpc::Status::OK;
  });
  return 0;
}
