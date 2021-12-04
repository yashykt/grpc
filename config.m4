PHP_ARG_ENABLE(grpc, whether to enable grpc support,
[  --enable-grpc           Enable grpc support])

if test "$PHP_GRPC" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-grpc -> add include path
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/include)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/src/core/ext/upb-generated)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/src/core/ext/upbdefs-generated)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/src/php/ext/grpc)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/third_party/abseil-cpp)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/third_party/address_sorting/include)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/third_party/boringssl-with-bazel/src/include)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/third_party/re2)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/third_party/upb)
  PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/third_party/xxhash)

  LIBS="-lpthread $LIBS"

  CFLAGS="-Wall -Werror -Wno-parentheses-equality -Wno-unused-value -std=c11 -g -O2"
  CXXFLAGS="-std=c++11 -fno-exceptions -fno-rtti -g -O2"
  GRPC_SHARED_LIBADD="-lpthread $GRPC_SHARED_LIBADD"
  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY(pthread)
  PHP_ADD_LIBRARY(dl,,GRPC_SHARED_LIBADD)
  PHP_ADD_LIBRARY(dl)

  case $host in
    *darwin*)
      PHP_ADD_LIBRARY(c++,1,GRPC_SHARED_LIBADD)
      ;;
    *)
      PHP_ADD_LIBRARY(stdc++,1,GRPC_SHARED_LIBADD)
      PHP_ADD_LIBRARY(rt,,GRPC_SHARED_LIBADD)
      PHP_ADD_LIBRARY(rt)
      ;;
  esac

  PHP_SUBST(GRPC_SHARED_LIBADD)

  PHP_NEW_EXTENSION(grpc,
    src/core/ext/filters/census/grpc_context.cc \
    src/core/ext/filters/client_channel/backend_metric.cc \
    src/core/ext/filters/client_channel/backup_poller.cc \
    src/core/ext/filters/client_channel/channel_connectivity.cc \
    src/core/ext/filters/client_channel/client_channel.cc \
    src/core/ext/filters/client_channel/client_channel_channelz.cc \
    src/core/ext/filters/client_channel/client_channel_factory.cc \
    src/core/ext/filters/client_channel/client_channel_plugin.cc \
    src/core/ext/filters/client_channel/config_selector.cc \
    src/core/ext/filters/client_channel/dynamic_filters.cc \
    src/core/ext/filters/client_channel/global_subchannel_pool.cc \
    src/core/ext/filters/client_channel/health/health_check_client.cc \
    src/core/ext/filters/client_channel/http_connect_handshaker.cc \
    src/core/ext/filters/client_channel/http_proxy.cc \
    src/core/ext/filters/client_channel/lb_policy.cc \
    src/core/ext/filters/client_channel/lb_policy/address_filtering.cc \
    src/core/ext/filters/client_channel/lb_policy/child_policy_handler.cc \
    src/core/ext/filters/client_channel/lb_policy/grpclb/client_load_reporting_filter.cc \
    src/core/ext/filters/client_channel/lb_policy/grpclb/grpclb.cc \
    src/core/ext/filters/client_channel/lb_policy/grpclb/grpclb_balancer_addresses.cc \
    src/core/ext/filters/client_channel/lb_policy/grpclb/grpclb_channel_secure.cc \
    src/core/ext/filters/client_channel/lb_policy/grpclb/grpclb_client_stats.cc \
    src/core/ext/filters/client_channel/lb_policy/grpclb/load_balancer_api.cc \
    src/core/ext/filters/client_channel/lb_policy/pick_first/pick_first.cc \
    src/core/ext/filters/client_channel/lb_policy/priority/priority.cc \
    src/core/ext/filters/client_channel/lb_policy/ring_hash/ring_hash.cc \
    src/core/ext/filters/client_channel/lb_policy/rls/rls.cc \
    src/core/ext/filters/client_channel/lb_policy/round_robin/round_robin.cc \
    src/core/ext/filters/client_channel/lb_policy/weighted_target/weighted_target.cc \
    src/core/ext/filters/client_channel/lb_policy/xds/cds.cc \
    src/core/ext/filters/client_channel/lb_policy/xds/xds_cluster_impl.cc \
    src/core/ext/filters/client_channel/lb_policy/xds/xds_cluster_manager.cc \
    src/core/ext/filters/client_channel/lb_policy/xds/xds_cluster_resolver.cc \
    src/core/ext/filters/client_channel/lb_policy_registry.cc \
    src/core/ext/filters/client_channel/local_subchannel_pool.cc \
    src/core/ext/filters/client_channel/proxy_mapper_registry.cc \
    src/core/ext/filters/client_channel/resolver.cc \
    src/core/ext/filters/client_channel/resolver/binder/binder_resolver.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/dns_resolver_ares.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/grpc_ares_ev_driver_event_engine.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/grpc_ares_ev_driver_posix.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/grpc_ares_ev_driver_windows.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/grpc_ares_wrapper.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/grpc_ares_wrapper_event_engine.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/grpc_ares_wrapper_posix.cc \
    src/core/ext/filters/client_channel/resolver/dns/c_ares/grpc_ares_wrapper_windows.cc \
    src/core/ext/filters/client_channel/resolver/dns/dns_resolver_selection.cc \
    src/core/ext/filters/client_channel/resolver/dns/native/dns_resolver.cc \
    src/core/ext/filters/client_channel/resolver/fake/fake_resolver.cc \
    src/core/ext/filters/client_channel/resolver/google_c2p/google_c2p_resolver.cc \
    src/core/ext/filters/client_channel/resolver/sockaddr/sockaddr_resolver.cc \
    src/core/ext/filters/client_channel/resolver/xds/xds_resolver.cc \
    src/core/ext/filters/client_channel/resolver_registry.cc \
    src/core/ext/filters/client_channel/resolver_result_parsing.cc \
    src/core/ext/filters/client_channel/retry_filter.cc \
    src/core/ext/filters/client_channel/retry_service_config.cc \
    src/core/ext/filters/client_channel/retry_throttle.cc \
    src/core/ext/filters/client_channel/server_address.cc \
    src/core/ext/filters/client_channel/service_config_channel_arg_filter.cc \
    src/core/ext/filters/client_channel/subchannel.cc \
    src/core/ext/filters/client_channel/subchannel_pool_interface.cc \
    src/core/ext/filters/client_idle/client_idle_filter.cc \
    src/core/ext/filters/client_idle/idle_filter_state.cc \
    src/core/ext/filters/deadline/deadline_filter.cc \
    src/core/ext/filters/fault_injection/fault_injection_filter.cc \
    src/core/ext/filters/fault_injection/service_config_parser.cc \
    src/core/ext/filters/http/client/http_client_filter.cc \
    src/core/ext/filters/http/client_authority_filter.cc \
    src/core/ext/filters/http/http_filters_plugin.cc \
    src/core/ext/filters/http/message_compress/message_compress_filter.cc \
    src/core/ext/filters/http/message_compress/message_decompress_filter.cc \
    src/core/ext/filters/http/server/http_server_filter.cc \
    src/core/ext/filters/max_age/max_age_filter.cc \
    src/core/ext/filters/message_size/message_size_filter.cc \
    src/core/ext/filters/rbac/rbac_filter.cc \
    src/core/ext/filters/server_config_selector/server_config_selector.cc \
    src/core/ext/filters/server_config_selector/server_config_selector_filter.cc \
    src/core/ext/service_config/service_config.cc \
    src/core/ext/service_config/service_config_parser.cc \
    src/core/ext/transport/chttp2/alpn/alpn.cc \
    src/core/ext/transport/chttp2/client/chttp2_connector.cc \
    src/core/ext/transport/chttp2/client/insecure/channel_create.cc \
    src/core/ext/transport/chttp2/client/insecure/channel_create_posix.cc \
    src/core/ext/transport/chttp2/client/secure/secure_channel_create.cc \
    src/core/ext/transport/chttp2/server/chttp2_server.cc \
    src/core/ext/transport/chttp2/server/insecure/server_chttp2.cc \
    src/core/ext/transport/chttp2/server/insecure/server_chttp2_posix.cc \
    src/core/ext/transport/chttp2/server/secure/server_secure_chttp2.cc \
    src/core/ext/transport/chttp2/transport/bin_decoder.cc \
    src/core/ext/transport/chttp2/transport/bin_encoder.cc \
    src/core/ext/transport/chttp2/transport/chttp2_plugin.cc \
    src/core/ext/transport/chttp2/transport/chttp2_transport.cc \
    src/core/ext/transport/chttp2/transport/context_list.cc \
    src/core/ext/transport/chttp2/transport/flow_control.cc \
    src/core/ext/transport/chttp2/transport/frame_data.cc \
    src/core/ext/transport/chttp2/transport/frame_goaway.cc \
    src/core/ext/transport/chttp2/transport/frame_ping.cc \
    src/core/ext/transport/chttp2/transport/frame_rst_stream.cc \
    src/core/ext/transport/chttp2/transport/frame_settings.cc \
    src/core/ext/transport/chttp2/transport/frame_window_update.cc \
    src/core/ext/transport/chttp2/transport/hpack_encoder.cc \
    src/core/ext/transport/chttp2/transport/hpack_encoder_table.cc \
    src/core/ext/transport/chttp2/transport/hpack_parser.cc \
    src/core/ext/transport/chttp2/transport/hpack_parser_table.cc \
    src/core/ext/transport/chttp2/transport/hpack_utils.cc \
    src/core/ext/transport/chttp2/transport/http2_settings.cc \
    src/core/ext/transport/chttp2/transport/huffsyms.cc \
    src/core/ext/transport/chttp2/transport/parsing.cc \
    src/core/ext/transport/chttp2/transport/stream_lists.cc \
    src/core/ext/transport/chttp2/transport/stream_map.cc \
    src/core/ext/transport/chttp2/transport/varint.cc \
    src/core/ext/transport/chttp2/transport/writing.cc \
    src/core/ext/transport/inproc/inproc_plugin.cc \
    src/core/ext/transport/inproc/inproc_transport.cc \
    src/core/ext/upb-generated/envoy/admin/v3/config_dump.upb.c \
    src/core/ext/upb-generated/envoy/annotations/deprecation.upb.c \
    src/core/ext/upb-generated/envoy/annotations/resource.upb.c \
    src/core/ext/upb-generated/envoy/config/accesslog/v3/accesslog.upb.c \
    src/core/ext/upb-generated/envoy/config/bootstrap/v3/bootstrap.upb.c \
    src/core/ext/upb-generated/envoy/config/cluster/v3/circuit_breaker.upb.c \
    src/core/ext/upb-generated/envoy/config/cluster/v3/cluster.upb.c \
    src/core/ext/upb-generated/envoy/config/cluster/v3/filter.upb.c \
    src/core/ext/upb-generated/envoy/config/cluster/v3/outlier_detection.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/address.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/backoff.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/base.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/config_source.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/event_service_config.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/extension.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/grpc_service.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/health_check.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/http_uri.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/protocol.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/proxy_protocol.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/resolver.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/socket_option.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/substitution_format_string.upb.c \
    src/core/ext/upb-generated/envoy/config/core/v3/udp_socket_config.upb.c \
    src/core/ext/upb-generated/envoy/config/endpoint/v3/endpoint.upb.c \
    src/core/ext/upb-generated/envoy/config/endpoint/v3/endpoint_components.upb.c \
    src/core/ext/upb-generated/envoy/config/endpoint/v3/load_report.upb.c \
    src/core/ext/upb-generated/envoy/config/listener/v3/api_listener.upb.c \
    src/core/ext/upb-generated/envoy/config/listener/v3/listener.upb.c \
    src/core/ext/upb-generated/envoy/config/listener/v3/listener_components.upb.c \
    src/core/ext/upb-generated/envoy/config/listener/v3/quic_config.upb.c \
    src/core/ext/upb-generated/envoy/config/listener/v3/udp_listener_config.upb.c \
    src/core/ext/upb-generated/envoy/config/metrics/v3/stats.upb.c \
    src/core/ext/upb-generated/envoy/config/overload/v3/overload.upb.c \
    src/core/ext/upb-generated/envoy/config/rbac/v3/rbac.upb.c \
    src/core/ext/upb-generated/envoy/config/route/v3/route.upb.c \
    src/core/ext/upb-generated/envoy/config/route/v3/route_components.upb.c \
    src/core/ext/upb-generated/envoy/config/route/v3/scoped_route.upb.c \
    src/core/ext/upb-generated/envoy/config/trace/v3/http_tracer.upb.c \
    src/core/ext/upb-generated/envoy/extensions/clusters/aggregate/v3/cluster.upb.c \
    src/core/ext/upb-generated/envoy/extensions/filters/common/fault/v3/fault.upb.c \
    src/core/ext/upb-generated/envoy/extensions/filters/http/fault/v3/fault.upb.c \
    src/core/ext/upb-generated/envoy/extensions/filters/http/router/v3/router.upb.c \
    src/core/ext/upb-generated/envoy/extensions/filters/network/http_connection_manager/v3/http_connection_manager.upb.c \
    src/core/ext/upb-generated/envoy/extensions/transport_sockets/tls/v3/cert.upb.c \
    src/core/ext/upb-generated/envoy/extensions/transport_sockets/tls/v3/common.upb.c \
    src/core/ext/upb-generated/envoy/extensions/transport_sockets/tls/v3/secret.upb.c \
    src/core/ext/upb-generated/envoy/extensions/transport_sockets/tls/v3/tls.upb.c \
    src/core/ext/upb-generated/envoy/service/cluster/v3/cds.upb.c \
    src/core/ext/upb-generated/envoy/service/discovery/v3/ads.upb.c \
    src/core/ext/upb-generated/envoy/service/discovery/v3/discovery.upb.c \
    src/core/ext/upb-generated/envoy/service/endpoint/v3/eds.upb.c \
    src/core/ext/upb-generated/envoy/service/listener/v3/lds.upb.c \
    src/core/ext/upb-generated/envoy/service/load_stats/v3/lrs.upb.c \
    src/core/ext/upb-generated/envoy/service/route/v3/rds.upb.c \
    src/core/ext/upb-generated/envoy/service/route/v3/srds.upb.c \
    src/core/ext/upb-generated/envoy/service/status/v3/csds.upb.c \
    src/core/ext/upb-generated/envoy/type/http/v3/path_transformation.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/metadata.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/node.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/number.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/path.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/regex.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/string.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/struct.upb.c \
    src/core/ext/upb-generated/envoy/type/matcher/v3/value.upb.c \
    src/core/ext/upb-generated/envoy/type/metadata/v3/metadata.upb.c \
    src/core/ext/upb-generated/envoy/type/tracing/v3/custom_tag.upb.c \
    src/core/ext/upb-generated/envoy/type/v3/http.upb.c \
    src/core/ext/upb-generated/envoy/type/v3/percent.upb.c \
    src/core/ext/upb-generated/envoy/type/v3/range.upb.c \
    src/core/ext/upb-generated/envoy/type/v3/semantic_version.upb.c \
    src/core/ext/upb-generated/google/api/annotations.upb.c \
    src/core/ext/upb-generated/google/api/expr/v1alpha1/checked.upb.c \
    src/core/ext/upb-generated/google/api/expr/v1alpha1/eval.upb.c \
    src/core/ext/upb-generated/google/api/expr/v1alpha1/explain.upb.c \
    src/core/ext/upb-generated/google/api/expr/v1alpha1/syntax.upb.c \
    src/core/ext/upb-generated/google/api/expr/v1alpha1/value.upb.c \
    src/core/ext/upb-generated/google/api/http.upb.c \
    src/core/ext/upb-generated/google/protobuf/any.upb.c \
    src/core/ext/upb-generated/google/protobuf/descriptor.upb.c \
    src/core/ext/upb-generated/google/protobuf/duration.upb.c \
    src/core/ext/upb-generated/google/protobuf/empty.upb.c \
    src/core/ext/upb-generated/google/protobuf/struct.upb.c \
    src/core/ext/upb-generated/google/protobuf/timestamp.upb.c \
    src/core/ext/upb-generated/google/protobuf/wrappers.upb.c \
    src/core/ext/upb-generated/google/rpc/status.upb.c \
    src/core/ext/upb-generated/src/proto/grpc/gcp/altscontext.upb.c \
    src/core/ext/upb-generated/src/proto/grpc/gcp/handshaker.upb.c \
    src/core/ext/upb-generated/src/proto/grpc/gcp/transport_security_common.upb.c \
    src/core/ext/upb-generated/src/proto/grpc/health/v1/health.upb.c \
    src/core/ext/upb-generated/src/proto/grpc/lb/v1/load_balancer.upb.c \
    src/core/ext/upb-generated/src/proto/grpc/lookup/v1/rls.upb.c \
    src/core/ext/upb-generated/udpa/annotations/migrate.upb.c \
    src/core/ext/upb-generated/udpa/annotations/security.upb.c \
    src/core/ext/upb-generated/udpa/annotations/sensitive.upb.c \
    src/core/ext/upb-generated/udpa/annotations/status.upb.c \
    src/core/ext/upb-generated/udpa/annotations/versioning.upb.c \
    src/core/ext/upb-generated/validate/validate.upb.c \
    src/core/ext/upb-generated/xds/annotations/v3/status.upb.c \
    src/core/ext/upb-generated/xds/core/v3/authority.upb.c \
    src/core/ext/upb-generated/xds/core/v3/collection_entry.upb.c \
    src/core/ext/upb-generated/xds/core/v3/context_params.upb.c \
    src/core/ext/upb-generated/xds/core/v3/resource.upb.c \
    src/core/ext/upb-generated/xds/core/v3/resource_locator.upb.c \
    src/core/ext/upb-generated/xds/core/v3/resource_name.upb.c \
    src/core/ext/upb-generated/xds/data/orca/v3/orca_load_report.upb.c \
    src/core/ext/upb-generated/xds/type/v3/typed_struct.upb.c \
    src/core/ext/upbdefs-generated/envoy/admin/v3/config_dump.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/annotations/deprecation.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/annotations/resource.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/accesslog/v3/accesslog.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/bootstrap/v3/bootstrap.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/cluster/v3/circuit_breaker.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/cluster/v3/cluster.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/cluster/v3/filter.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/cluster/v3/outlier_detection.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/address.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/backoff.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/base.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/config_source.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/event_service_config.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/extension.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/grpc_service.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/health_check.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/http_uri.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/protocol.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/proxy_protocol.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/resolver.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/socket_option.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/substitution_format_string.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/core/v3/udp_socket_config.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/endpoint/v3/endpoint.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/endpoint/v3/endpoint_components.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/endpoint/v3/load_report.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/listener/v3/api_listener.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/listener/v3/listener.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/listener/v3/listener_components.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/listener/v3/quic_config.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/listener/v3/udp_listener_config.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/metrics/v3/stats.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/overload/v3/overload.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/route/v3/route.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/route/v3/route_components.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/route/v3/scoped_route.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/config/trace/v3/http_tracer.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/clusters/aggregate/v3/cluster.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/filters/common/fault/v3/fault.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/filters/http/fault/v3/fault.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/filters/http/router/v3/router.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/filters/network/http_connection_manager/v3/http_connection_manager.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/transport_sockets/tls/v3/cert.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/transport_sockets/tls/v3/common.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/transport_sockets/tls/v3/secret.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/extensions/transport_sockets/tls/v3/tls.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/cluster/v3/cds.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/discovery/v3/ads.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/discovery/v3/discovery.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/endpoint/v3/eds.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/listener/v3/lds.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/load_stats/v3/lrs.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/route/v3/rds.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/route/v3/srds.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/service/status/v3/csds.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/http/v3/path_transformation.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/metadata.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/node.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/number.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/path.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/regex.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/string.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/struct.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/matcher/v3/value.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/metadata/v3/metadata.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/tracing/v3/custom_tag.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/v3/http.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/v3/percent.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/v3/range.upbdefs.c \
    src/core/ext/upbdefs-generated/envoy/type/v3/semantic_version.upbdefs.c \
    src/core/ext/upbdefs-generated/google/api/annotations.upbdefs.c \
    src/core/ext/upbdefs-generated/google/api/http.upbdefs.c \
    src/core/ext/upbdefs-generated/google/protobuf/any.upbdefs.c \
    src/core/ext/upbdefs-generated/google/protobuf/descriptor.upbdefs.c \
    src/core/ext/upbdefs-generated/google/protobuf/duration.upbdefs.c \
    src/core/ext/upbdefs-generated/google/protobuf/empty.upbdefs.c \
    src/core/ext/upbdefs-generated/google/protobuf/struct.upbdefs.c \
    src/core/ext/upbdefs-generated/google/protobuf/timestamp.upbdefs.c \
    src/core/ext/upbdefs-generated/google/protobuf/wrappers.upbdefs.c \
    src/core/ext/upbdefs-generated/google/rpc/status.upbdefs.c \
    src/core/ext/upbdefs-generated/udpa/annotations/migrate.upbdefs.c \
    src/core/ext/upbdefs-generated/udpa/annotations/security.upbdefs.c \
    src/core/ext/upbdefs-generated/udpa/annotations/sensitive.upbdefs.c \
    src/core/ext/upbdefs-generated/udpa/annotations/status.upbdefs.c \
    src/core/ext/upbdefs-generated/udpa/annotations/versioning.upbdefs.c \
    src/core/ext/upbdefs-generated/validate/validate.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/annotations/v3/status.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/core/v3/authority.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/core/v3/collection_entry.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/core/v3/context_params.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/core/v3/resource.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/core/v3/resource_locator.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/core/v3/resource_name.upbdefs.c \
    src/core/ext/upbdefs-generated/xds/type/v3/typed_struct.upbdefs.c \
    src/core/ext/xds/certificate_provider_registry.cc \
    src/core/ext/xds/certificate_provider_store.cc \
    src/core/ext/xds/file_watcher_certificate_provider_factory.cc \
    src/core/ext/xds/xds_api.cc \
    src/core/ext/xds/xds_bootstrap.cc \
    src/core/ext/xds/xds_certificate_provider.cc \
    src/core/ext/xds/xds_channel_stack_modifier.cc \
    src/core/ext/xds/xds_client.cc \
    src/core/ext/xds/xds_client_stats.cc \
    src/core/ext/xds/xds_http_fault_filter.cc \
    src/core/ext/xds/xds_http_filters.cc \
    src/core/ext/xds/xds_http_rbac_filter.cc \
    src/core/ext/xds/xds_routing.cc \
    src/core/ext/xds/xds_server_config_fetcher.cc \
    src/core/lib/address_utils/parse_address.cc \
    src/core/lib/address_utils/sockaddr_utils.cc \
    src/core/lib/backoff/backoff.cc \
    src/core/lib/channel/channel_args.cc \
    src/core/lib/channel/channel_args_preconditioning.cc \
    src/core/lib/channel/channel_stack.cc \
    src/core/lib/channel/channel_stack_builder.cc \
    src/core/lib/channel/channel_trace.cc \
    src/core/lib/channel/channelz.cc \
    src/core/lib/channel/channelz_registry.cc \
    src/core/lib/channel/connected_channel.cc \
    src/core/lib/channel/handshaker.cc \
    src/core/lib/channel/handshaker_registry.cc \
    src/core/lib/channel/status_util.cc \
    src/core/lib/compression/compression.cc \
    src/core/lib/compression/compression_args.cc \
    src/core/lib/compression/compression_internal.cc \
    src/core/lib/compression/message_compress.cc \
    src/core/lib/compression/stream_compression.cc \
    src/core/lib/compression/stream_compression_gzip.cc \
    src/core/lib/compression/stream_compression_identity.cc \
    src/core/lib/config/core_configuration.cc \
    src/core/lib/debug/stats.cc \
    src/core/lib/debug/stats_data.cc \
    src/core/lib/debug/trace.cc \
    src/core/lib/event_engine/channel_args_endpoint_config.cc \
    src/core/lib/event_engine/event_engine.cc \
    src/core/lib/event_engine/event_engine_factory.cc \
    src/core/lib/event_engine/memory_allocator.cc \
    src/core/lib/event_engine/sockaddr.cc \
    src/core/lib/gpr/alloc.cc \
    src/core/lib/gpr/atm.cc \
    src/core/lib/gpr/cpu_iphone.cc \
    src/core/lib/gpr/cpu_linux.cc \
    src/core/lib/gpr/cpu_posix.cc \
    src/core/lib/gpr/cpu_windows.cc \
    src/core/lib/gpr/env_linux.cc \
    src/core/lib/gpr/env_posix.cc \
    src/core/lib/gpr/env_windows.cc \
    src/core/lib/gpr/log.cc \
    src/core/lib/gpr/log_android.cc \
    src/core/lib/gpr/log_linux.cc \
    src/core/lib/gpr/log_posix.cc \
    src/core/lib/gpr/log_windows.cc \
    src/core/lib/gpr/murmur_hash.cc \
    src/core/lib/gpr/string.cc \
    src/core/lib/gpr/string_posix.cc \
    src/core/lib/gpr/string_util_windows.cc \
    src/core/lib/gpr/string_windows.cc \
    src/core/lib/gpr/sync.cc \
    src/core/lib/gpr/sync_abseil.cc \
    src/core/lib/gpr/sync_posix.cc \
    src/core/lib/gpr/sync_windows.cc \
    src/core/lib/gpr/time.cc \
    src/core/lib/gpr/time_posix.cc \
    src/core/lib/gpr/time_precise.cc \
    src/core/lib/gpr/time_windows.cc \
    src/core/lib/gpr/tmpfile_msys.cc \
    src/core/lib/gpr/tmpfile_posix.cc \
    src/core/lib/gpr/tmpfile_windows.cc \
    src/core/lib/gpr/wrap_memcpy.cc \
    src/core/lib/gprpp/arena.cc \
    src/core/lib/gprpp/examine_stack.cc \
    src/core/lib/gprpp/fork.cc \
    src/core/lib/gprpp/global_config_env.cc \
    src/core/lib/gprpp/host_port.cc \
    src/core/lib/gprpp/mpscq.cc \
    src/core/lib/gprpp/stat_posix.cc \
    src/core/lib/gprpp/stat_windows.cc \
    src/core/lib/gprpp/status_helper.cc \
    src/core/lib/gprpp/thd_posix.cc \
    src/core/lib/gprpp/thd_windows.cc \
    src/core/lib/gprpp/time_util.cc \
    src/core/lib/http/format_request.cc \
    src/core/lib/http/httpcli.cc \
    src/core/lib/http/httpcli_security_connector.cc \
    src/core/lib/http/parser.cc \
    src/core/lib/iomgr/buffer_list.cc \
    src/core/lib/iomgr/call_combiner.cc \
    src/core/lib/iomgr/cfstream_handle.cc \
    src/core/lib/iomgr/combiner.cc \
    src/core/lib/iomgr/dualstack_socket_posix.cc \
    src/core/lib/iomgr/endpoint.cc \
    src/core/lib/iomgr/endpoint_cfstream.cc \
    src/core/lib/iomgr/endpoint_pair_event_engine.cc \
    src/core/lib/iomgr/endpoint_pair_posix.cc \
    src/core/lib/iomgr/endpoint_pair_windows.cc \
    src/core/lib/iomgr/error.cc \
    src/core/lib/iomgr/error_cfstream.cc \
    src/core/lib/iomgr/ev_apple.cc \
    src/core/lib/iomgr/ev_epoll1_linux.cc \
    src/core/lib/iomgr/ev_epollex_linux.cc \
    src/core/lib/iomgr/ev_poll_posix.cc \
    src/core/lib/iomgr/ev_posix.cc \
    src/core/lib/iomgr/ev_windows.cc \
    src/core/lib/iomgr/event_engine/closure.cc \
    src/core/lib/iomgr/event_engine/endpoint.cc \
    src/core/lib/iomgr/event_engine/iomgr.cc \
    src/core/lib/iomgr/event_engine/pollset.cc \
    src/core/lib/iomgr/event_engine/resolved_address_internal.cc \
    src/core/lib/iomgr/event_engine/resolver.cc \
    src/core/lib/iomgr/event_engine/tcp.cc \
    src/core/lib/iomgr/event_engine/timer.cc \
    src/core/lib/iomgr/exec_ctx.cc \
    src/core/lib/iomgr/executor.cc \
    src/core/lib/iomgr/executor/mpmcqueue.cc \
    src/core/lib/iomgr/executor/threadpool.cc \
    src/core/lib/iomgr/fork_posix.cc \
    src/core/lib/iomgr/fork_windows.cc \
    src/core/lib/iomgr/gethostname_fallback.cc \
    src/core/lib/iomgr/gethostname_host_name_max.cc \
    src/core/lib/iomgr/gethostname_sysconf.cc \
    src/core/lib/iomgr/grpc_if_nametoindex_posix.cc \
    src/core/lib/iomgr/grpc_if_nametoindex_unsupported.cc \
    src/core/lib/iomgr/internal_errqueue.cc \
    src/core/lib/iomgr/iocp_windows.cc \
    src/core/lib/iomgr/iomgr.cc \
    src/core/lib/iomgr/iomgr_custom.cc \
    src/core/lib/iomgr/iomgr_internal.cc \
    src/core/lib/iomgr/iomgr_posix.cc \
    src/core/lib/iomgr/iomgr_posix_cfstream.cc \
    src/core/lib/iomgr/iomgr_windows.cc \
    src/core/lib/iomgr/is_epollexclusive_available.cc \
    src/core/lib/iomgr/load_file.cc \
    src/core/lib/iomgr/lockfree_event.cc \
    src/core/lib/iomgr/polling_entity.cc \
    src/core/lib/iomgr/pollset.cc \
    src/core/lib/iomgr/pollset_custom.cc \
    src/core/lib/iomgr/pollset_set.cc \
    src/core/lib/iomgr/pollset_set_custom.cc \
    src/core/lib/iomgr/pollset_set_windows.cc \
    src/core/lib/iomgr/pollset_windows.cc \
    src/core/lib/iomgr/resolve_address.cc \
    src/core/lib/iomgr/resolve_address_custom.cc \
    src/core/lib/iomgr/resolve_address_posix.cc \
    src/core/lib/iomgr/resolve_address_windows.cc \
    src/core/lib/iomgr/socket_factory_posix.cc \
    src/core/lib/iomgr/socket_mutator.cc \
    src/core/lib/iomgr/socket_utils_common_posix.cc \
    src/core/lib/iomgr/socket_utils_linux.cc \
    src/core/lib/iomgr/socket_utils_posix.cc \
    src/core/lib/iomgr/socket_utils_windows.cc \
    src/core/lib/iomgr/socket_windows.cc \
    src/core/lib/iomgr/tcp_client.cc \
    src/core/lib/iomgr/tcp_client_cfstream.cc \
    src/core/lib/iomgr/tcp_client_custom.cc \
    src/core/lib/iomgr/tcp_client_posix.cc \
    src/core/lib/iomgr/tcp_client_windows.cc \
    src/core/lib/iomgr/tcp_custom.cc \
    src/core/lib/iomgr/tcp_posix.cc \
    src/core/lib/iomgr/tcp_server.cc \
    src/core/lib/iomgr/tcp_server_custom.cc \
    src/core/lib/iomgr/tcp_server_posix.cc \
    src/core/lib/iomgr/tcp_server_utils_posix_common.cc \
    src/core/lib/iomgr/tcp_server_utils_posix_ifaddrs.cc \
    src/core/lib/iomgr/tcp_server_utils_posix_noifaddrs.cc \
    src/core/lib/iomgr/tcp_server_windows.cc \
    src/core/lib/iomgr/tcp_windows.cc \
    src/core/lib/iomgr/time_averaged_stats.cc \
    src/core/lib/iomgr/timer.cc \
    src/core/lib/iomgr/timer_custom.cc \
    src/core/lib/iomgr/timer_generic.cc \
    src/core/lib/iomgr/timer_heap.cc \
    src/core/lib/iomgr/timer_manager.cc \
    src/core/lib/iomgr/unix_sockets_posix.cc \
    src/core/lib/iomgr/unix_sockets_posix_noop.cc \
    src/core/lib/iomgr/wakeup_fd_eventfd.cc \
    src/core/lib/iomgr/wakeup_fd_nospecial.cc \
    src/core/lib/iomgr/wakeup_fd_pipe.cc \
    src/core/lib/iomgr/wakeup_fd_posix.cc \
    src/core/lib/iomgr/work_serializer.cc \
    src/core/lib/json/json_reader.cc \
    src/core/lib/json/json_util.cc \
    src/core/lib/json/json_writer.cc \
    src/core/lib/matchers/matchers.cc \
    src/core/lib/profiling/basic_timers.cc \
    src/core/lib/profiling/stap_timers.cc \
    src/core/lib/promise/activity.cc \
    src/core/lib/resource_quota/api.cc \
    src/core/lib/resource_quota/memory_quota.cc \
    src/core/lib/resource_quota/resource_quota.cc \
    src/core/lib/resource_quota/thread_quota.cc \
    src/core/lib/resource_quota/trace.cc \
    src/core/lib/security/authorization/authorization_policy_provider_vtable.cc \
    src/core/lib/security/authorization/evaluate_args.cc \
    src/core/lib/security/authorization/sdk_server_authz_filter.cc \
    src/core/lib/security/context/security_context.cc \
    src/core/lib/security/credentials/alts/alts_credentials.cc \
    src/core/lib/security/credentials/alts/check_gcp_environment.cc \
    src/core/lib/security/credentials/alts/check_gcp_environment_linux.cc \
    src/core/lib/security/credentials/alts/check_gcp_environment_no_op.cc \
    src/core/lib/security/credentials/alts/check_gcp_environment_windows.cc \
    src/core/lib/security/credentials/alts/grpc_alts_credentials_client_options.cc \
    src/core/lib/security/credentials/alts/grpc_alts_credentials_options.cc \
    src/core/lib/security/credentials/alts/grpc_alts_credentials_server_options.cc \
    src/core/lib/security/credentials/composite/composite_credentials.cc \
    src/core/lib/security/credentials/credentials.cc \
    src/core/lib/security/credentials/credentials_metadata.cc \
    src/core/lib/security/credentials/external/aws_external_account_credentials.cc \
    src/core/lib/security/credentials/external/aws_request_signer.cc \
    src/core/lib/security/credentials/external/external_account_credentials.cc \
    src/core/lib/security/credentials/external/file_external_account_credentials.cc \
    src/core/lib/security/credentials/external/url_external_account_credentials.cc \
    src/core/lib/security/credentials/fake/fake_credentials.cc \
    src/core/lib/security/credentials/google_default/credentials_generic.cc \
    src/core/lib/security/credentials/google_default/google_default_credentials.cc \
    src/core/lib/security/credentials/iam/iam_credentials.cc \
    src/core/lib/security/credentials/insecure/insecure_credentials.cc \
    src/core/lib/security/credentials/jwt/json_token.cc \
    src/core/lib/security/credentials/jwt/jwt_credentials.cc \
    src/core/lib/security/credentials/jwt/jwt_verifier.cc \
    src/core/lib/security/credentials/local/local_credentials.cc \
    src/core/lib/security/credentials/oauth2/oauth2_credentials.cc \
    src/core/lib/security/credentials/plugin/plugin_credentials.cc \
    src/core/lib/security/credentials/ssl/ssl_credentials.cc \
    src/core/lib/security/credentials/tls/grpc_tls_certificate_distributor.cc \
    src/core/lib/security/credentials/tls/grpc_tls_certificate_provider.cc \
    src/core/lib/security/credentials/tls/grpc_tls_certificate_verifier.cc \
    src/core/lib/security/credentials/tls/grpc_tls_credentials_options.cc \
    src/core/lib/security/credentials/tls/tls_credentials.cc \
    src/core/lib/security/credentials/tls/tls_utils.cc \
    src/core/lib/security/credentials/xds/xds_credentials.cc \
    src/core/lib/security/security_connector/alts/alts_security_connector.cc \
    src/core/lib/security/security_connector/fake/fake_security_connector.cc \
    src/core/lib/security/security_connector/insecure/insecure_security_connector.cc \
    src/core/lib/security/security_connector/load_system_roots_fallback.cc \
    src/core/lib/security/security_connector/load_system_roots_linux.cc \
    src/core/lib/security/security_connector/local/local_security_connector.cc \
    src/core/lib/security/security_connector/security_connector.cc \
    src/core/lib/security/security_connector/ssl/ssl_security_connector.cc \
    src/core/lib/security/security_connector/ssl_utils.cc \
    src/core/lib/security/security_connector/ssl_utils_config.cc \
    src/core/lib/security/security_connector/tls/tls_security_connector.cc \
    src/core/lib/security/transport/client_auth_filter.cc \
    src/core/lib/security/transport/secure_endpoint.cc \
    src/core/lib/security/transport/security_handshaker.cc \
    src/core/lib/security/transport/server_auth_filter.cc \
    src/core/lib/security/transport/tsi_error.cc \
    src/core/lib/security/util/json_util.cc \
    src/core/lib/slice/b64.cc \
    src/core/lib/slice/percent_encoding.cc \
    src/core/lib/slice/slice.cc \
    src/core/lib/slice/slice_api.cc \
    src/core/lib/slice/slice_buffer.cc \
    src/core/lib/slice/slice_intern.cc \
    src/core/lib/slice/slice_refcount.cc \
    src/core/lib/slice/slice_split.cc \
    src/core/lib/slice/slice_string_helpers.cc \
    src/core/lib/slice/static_slice.cc \
    src/core/lib/surface/api_trace.cc \
    src/core/lib/surface/builtins.cc \
    src/core/lib/surface/byte_buffer.cc \
    src/core/lib/surface/byte_buffer_reader.cc \
    src/core/lib/surface/call.cc \
    src/core/lib/surface/call_details.cc \
    src/core/lib/surface/call_log_batch.cc \
    src/core/lib/surface/channel.cc \
    src/core/lib/surface/channel_init.cc \
    src/core/lib/surface/channel_ping.cc \
    src/core/lib/surface/channel_stack_type.cc \
    src/core/lib/surface/completion_queue.cc \
    src/core/lib/surface/completion_queue_factory.cc \
    src/core/lib/surface/event_string.cc \
    src/core/lib/surface/init.cc \
    src/core/lib/surface/init_secure.cc \
    src/core/lib/surface/lame_client.cc \
    src/core/lib/surface/metadata_array.cc \
    src/core/lib/surface/server.cc \
    src/core/lib/surface/validate_metadata.cc \
    src/core/lib/surface/version.cc \
    src/core/lib/transport/bdp_estimator.cc \
    src/core/lib/transport/byte_stream.cc \
    src/core/lib/transport/connectivity_state.cc \
    src/core/lib/transport/error_utils.cc \
    src/core/lib/transport/metadata.cc \
    src/core/lib/transport/metadata_batch.cc \
    src/core/lib/transport/parsed_metadata.cc \
    src/core/lib/transport/pid_controller.cc \
    src/core/lib/transport/static_metadata.cc \
    src/core/lib/transport/status_conversion.cc \
    src/core/lib/transport/status_metadata.cc \
    src/core/lib/transport/timeout_encoding.cc \
    src/core/lib/transport/transport.cc \
    src/core/lib/transport/transport_op_string.cc \
    src/core/lib/uri/uri_parser.cc \
    src/core/plugin_registry/grpc_plugin_registry.cc \
    src/core/tsi/alts/crypt/aes_gcm.cc \
    src/core/tsi/alts/crypt/gsec.cc \
    src/core/tsi/alts/frame_protector/alts_counter.cc \
    src/core/tsi/alts/frame_protector/alts_crypter.cc \
    src/core/tsi/alts/frame_protector/alts_frame_protector.cc \
    src/core/tsi/alts/frame_protector/alts_record_protocol_crypter_common.cc \
    src/core/tsi/alts/frame_protector/alts_seal_privacy_integrity_crypter.cc \
    src/core/tsi/alts/frame_protector/alts_unseal_privacy_integrity_crypter.cc \
    src/core/tsi/alts/frame_protector/frame_handler.cc \
    src/core/tsi/alts/handshaker/alts_handshaker_client.cc \
    src/core/tsi/alts/handshaker/alts_shared_resource.cc \
    src/core/tsi/alts/handshaker/alts_tsi_handshaker.cc \
    src/core/tsi/alts/handshaker/alts_tsi_utils.cc \
    src/core/tsi/alts/handshaker/transport_security_common_api.cc \
    src/core/tsi/alts/zero_copy_frame_protector/alts_grpc_integrity_only_record_protocol.cc \
    src/core/tsi/alts/zero_copy_frame_protector/alts_grpc_privacy_integrity_record_protocol.cc \
    src/core/tsi/alts/zero_copy_frame_protector/alts_grpc_record_protocol_common.cc \
    src/core/tsi/alts/zero_copy_frame_protector/alts_iovec_record_protocol.cc \
    src/core/tsi/alts/zero_copy_frame_protector/alts_zero_copy_grpc_protector.cc \
    src/core/tsi/fake_transport_security.cc \
    src/core/tsi/local_transport_security.cc \
    src/core/tsi/ssl/session_cache/ssl_session_boringssl.cc \
    src/core/tsi/ssl/session_cache/ssl_session_cache.cc \
    src/core/tsi/ssl/session_cache/ssl_session_openssl.cc \
    src/core/tsi/ssl_transport_security.cc \
    src/core/tsi/transport_security.cc \
    src/core/tsi/transport_security_grpc.cc \
    src/php/ext/grpc/byte_buffer.c \
    src/php/ext/grpc/call.c \
    src/php/ext/grpc/call_credentials.c \
    src/php/ext/grpc/channel.c \
    src/php/ext/grpc/channel_credentials.c \
    src/php/ext/grpc/completion_queue.c \
    src/php/ext/grpc/php_grpc.c \
    src/php/ext/grpc/server.c \
    src/php/ext/grpc/server_credentials.c \
    src/php/ext/grpc/timeval.c \
    third_party/abseil-cpp/absl/base/internal/cycleclock.cc \
    third_party/abseil-cpp/absl/base/internal/low_level_alloc.cc \
    third_party/abseil-cpp/absl/base/internal/raw_logging.cc \
    third_party/abseil-cpp/absl/base/internal/spinlock.cc \
    third_party/abseil-cpp/absl/base/internal/spinlock_wait.cc \
    third_party/abseil-cpp/absl/base/internal/sysinfo.cc \
    third_party/abseil-cpp/absl/base/internal/thread_identity.cc \
    third_party/abseil-cpp/absl/base/internal/throw_delegate.cc \
    third_party/abseil-cpp/absl/base/internal/unscaledcycleclock.cc \
    third_party/abseil-cpp/absl/base/log_severity.cc \
    third_party/abseil-cpp/absl/container/internal/hashtablez_sampler.cc \
    third_party/abseil-cpp/absl/container/internal/hashtablez_sampler_force_weak_definition.cc \
    third_party/abseil-cpp/absl/container/internal/raw_hash_set.cc \
    third_party/abseil-cpp/absl/debugging/internal/address_is_readable.cc \
    third_party/abseil-cpp/absl/debugging/internal/demangle.cc \
    third_party/abseil-cpp/absl/debugging/internal/elf_mem_image.cc \
    third_party/abseil-cpp/absl/debugging/internal/vdso_support.cc \
    third_party/abseil-cpp/absl/debugging/stacktrace.cc \
    third_party/abseil-cpp/absl/debugging/symbolize.cc \
    third_party/abseil-cpp/absl/hash/internal/city.cc \
    third_party/abseil-cpp/absl/hash/internal/hash.cc \
    third_party/abseil-cpp/absl/hash/internal/low_level_hash.cc \
    third_party/abseil-cpp/absl/numeric/int128.cc \
    third_party/abseil-cpp/absl/profiling/internal/exponential_biased.cc \
    third_party/abseil-cpp/absl/random/discrete_distribution.cc \
    third_party/abseil-cpp/absl/random/gaussian_distribution.cc \
    third_party/abseil-cpp/absl/random/internal/pool_urbg.cc \
    third_party/abseil-cpp/absl/random/internal/randen.cc \
    third_party/abseil-cpp/absl/random/internal/randen_detect.cc \
    third_party/abseil-cpp/absl/random/internal/randen_hwaes.cc \
    third_party/abseil-cpp/absl/random/internal/randen_round_keys.cc \
    third_party/abseil-cpp/absl/random/internal/randen_slow.cc \
    third_party/abseil-cpp/absl/random/internal/seed_material.cc \
    third_party/abseil-cpp/absl/random/seed_gen_exception.cc \
    third_party/abseil-cpp/absl/random/seed_sequences.cc \
    third_party/abseil-cpp/absl/status/status.cc \
    third_party/abseil-cpp/absl/status/status_payload_printer.cc \
    third_party/abseil-cpp/absl/status/statusor.cc \
    third_party/abseil-cpp/absl/strings/ascii.cc \
    third_party/abseil-cpp/absl/strings/charconv.cc \
    third_party/abseil-cpp/absl/strings/cord.cc \
    third_party/abseil-cpp/absl/strings/escaping.cc \
    third_party/abseil-cpp/absl/strings/internal/charconv_bigint.cc \
    third_party/abseil-cpp/absl/strings/internal/charconv_parse.cc \
    third_party/abseil-cpp/absl/strings/internal/cord_internal.cc \
    third_party/abseil-cpp/absl/strings/internal/cord_rep_btree.cc \
    third_party/abseil-cpp/absl/strings/internal/cord_rep_btree_navigator.cc \
    third_party/abseil-cpp/absl/strings/internal/cord_rep_btree_reader.cc \
    third_party/abseil-cpp/absl/strings/internal/cord_rep_consume.cc \
    third_party/abseil-cpp/absl/strings/internal/cord_rep_ring.cc \
    third_party/abseil-cpp/absl/strings/internal/cordz_functions.cc \
    third_party/abseil-cpp/absl/strings/internal/cordz_handle.cc \
    third_party/abseil-cpp/absl/strings/internal/cordz_info.cc \
    third_party/abseil-cpp/absl/strings/internal/escaping.cc \
    third_party/abseil-cpp/absl/strings/internal/memutil.cc \
    third_party/abseil-cpp/absl/strings/internal/ostringstream.cc \
    third_party/abseil-cpp/absl/strings/internal/str_format/arg.cc \
    third_party/abseil-cpp/absl/strings/internal/str_format/bind.cc \
    third_party/abseil-cpp/absl/strings/internal/str_format/extension.cc \
    third_party/abseil-cpp/absl/strings/internal/str_format/float_conversion.cc \
    third_party/abseil-cpp/absl/strings/internal/str_format/output.cc \
    third_party/abseil-cpp/absl/strings/internal/str_format/parser.cc \
    third_party/abseil-cpp/absl/strings/internal/utf8.cc \
    third_party/abseil-cpp/absl/strings/match.cc \
    third_party/abseil-cpp/absl/strings/numbers.cc \
    third_party/abseil-cpp/absl/strings/str_cat.cc \
    third_party/abseil-cpp/absl/strings/str_replace.cc \
    third_party/abseil-cpp/absl/strings/str_split.cc \
    third_party/abseil-cpp/absl/strings/string_view.cc \
    third_party/abseil-cpp/absl/strings/substitute.cc \
    third_party/abseil-cpp/absl/synchronization/barrier.cc \
    third_party/abseil-cpp/absl/synchronization/blocking_counter.cc \
    third_party/abseil-cpp/absl/synchronization/internal/create_thread_identity.cc \
    third_party/abseil-cpp/absl/synchronization/internal/graphcycles.cc \
    third_party/abseil-cpp/absl/synchronization/internal/per_thread_sem.cc \
    third_party/abseil-cpp/absl/synchronization/internal/waiter.cc \
    third_party/abseil-cpp/absl/synchronization/mutex.cc \
    third_party/abseil-cpp/absl/synchronization/notification.cc \
    third_party/abseil-cpp/absl/time/civil_time.cc \
    third_party/abseil-cpp/absl/time/clock.cc \
    third_party/abseil-cpp/absl/time/duration.cc \
    third_party/abseil-cpp/absl/time/format.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/civil_time_detail.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_fixed.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_format.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_if.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_impl.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_info.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_libc.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_lookup.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/time_zone_posix.cc \
    third_party/abseil-cpp/absl/time/internal/cctz/src/zone_info_source.cc \
    third_party/abseil-cpp/absl/time/time.cc \
    third_party/abseil-cpp/absl/types/bad_optional_access.cc \
    third_party/abseil-cpp/absl/types/bad_variant_access.cc \
    third_party/address_sorting/address_sorting.c \
    third_party/address_sorting/address_sorting_posix.c \
    third_party/address_sorting/address_sorting_windows.c \
    third_party/boringssl-with-bazel/err_data.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_bitstr.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_bool.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_d2i_fp.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_dup.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_enum.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_gentm.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_i2d_fp.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_int.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_mbstr.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_object.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_octet.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_print.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_strex.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_strnid.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_time.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_type.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_utctm.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/a_utf8.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/asn1_lib.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/asn1_par.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/asn_pack.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/f_int.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/f_string.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/tasn_dec.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/tasn_enc.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/tasn_fre.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/tasn_new.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/tasn_typ.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/tasn_utl.c \
    third_party/boringssl-with-bazel/src/crypto/asn1/time_support.c \
    third_party/boringssl-with-bazel/src/crypto/base64/base64.c \
    third_party/boringssl-with-bazel/src/crypto/bio/bio.c \
    third_party/boringssl-with-bazel/src/crypto/bio/bio_mem.c \
    third_party/boringssl-with-bazel/src/crypto/bio/connect.c \
    third_party/boringssl-with-bazel/src/crypto/bio/fd.c \
    third_party/boringssl-with-bazel/src/crypto/bio/file.c \
    third_party/boringssl-with-bazel/src/crypto/bio/hexdump.c \
    third_party/boringssl-with-bazel/src/crypto/bio/pair.c \
    third_party/boringssl-with-bazel/src/crypto/bio/printf.c \
    third_party/boringssl-with-bazel/src/crypto/bio/socket.c \
    third_party/boringssl-with-bazel/src/crypto/bio/socket_helper.c \
    third_party/boringssl-with-bazel/src/crypto/blake2/blake2.c \
    third_party/boringssl-with-bazel/src/crypto/bn_extra/bn_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/bn_extra/convert.c \
    third_party/boringssl-with-bazel/src/crypto/buf/buf.c \
    third_party/boringssl-with-bazel/src/crypto/bytestring/asn1_compat.c \
    third_party/boringssl-with-bazel/src/crypto/bytestring/ber.c \
    third_party/boringssl-with-bazel/src/crypto/bytestring/cbb.c \
    third_party/boringssl-with-bazel/src/crypto/bytestring/cbs.c \
    third_party/boringssl-with-bazel/src/crypto/bytestring/unicode.c \
    third_party/boringssl-with-bazel/src/crypto/chacha/chacha.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/cipher_extra.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/derive_key.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_aesccm.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_aesctrhmac.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_aesgcmsiv.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_chacha20poly1305.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_null.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_rc2.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_rc4.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/e_tls.c \
    third_party/boringssl-with-bazel/src/crypto/cipher_extra/tls_cbc.c \
    third_party/boringssl-with-bazel/src/crypto/cmac/cmac.c \
    third_party/boringssl-with-bazel/src/crypto/conf/conf.c \
    third_party/boringssl-with-bazel/src/crypto/cpu-aarch64-fuchsia.c \
    third_party/boringssl-with-bazel/src/crypto/cpu-aarch64-linux.c \
    third_party/boringssl-with-bazel/src/crypto/cpu-aarch64-win.c \
    third_party/boringssl-with-bazel/src/crypto/cpu-arm-linux.c \
    third_party/boringssl-with-bazel/src/crypto/cpu-arm.c \
    third_party/boringssl-with-bazel/src/crypto/cpu-intel.c \
    third_party/boringssl-with-bazel/src/crypto/cpu-ppc64le.c \
    third_party/boringssl-with-bazel/src/crypto/crypto.c \
    third_party/boringssl-with-bazel/src/crypto/curve25519/curve25519.c \
    third_party/boringssl-with-bazel/src/crypto/curve25519/spake25519.c \
    third_party/boringssl-with-bazel/src/crypto/dh_extra/dh_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/dh_extra/params.c \
    third_party/boringssl-with-bazel/src/crypto/digest_extra/digest_extra.c \
    third_party/boringssl-with-bazel/src/crypto/dsa/dsa.c \
    third_party/boringssl-with-bazel/src/crypto/dsa/dsa_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/ec_extra/ec_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/ec_extra/ec_derive.c \
    third_party/boringssl-with-bazel/src/crypto/ec_extra/hash_to_curve.c \
    third_party/boringssl-with-bazel/src/crypto/ecdh_extra/ecdh_extra.c \
    third_party/boringssl-with-bazel/src/crypto/ecdsa_extra/ecdsa_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/engine/engine.c \
    third_party/boringssl-with-bazel/src/crypto/err/err.c \
    third_party/boringssl-with-bazel/src/crypto/evp/digestsign.c \
    third_party/boringssl-with-bazel/src/crypto/evp/evp.c \
    third_party/boringssl-with-bazel/src/crypto/evp/evp_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/evp/evp_ctx.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_dsa_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_ec.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_ec_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_ed25519.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_ed25519_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_rsa.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_rsa_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_x25519.c \
    third_party/boringssl-with-bazel/src/crypto/evp/p_x25519_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/evp/pbkdf.c \
    third_party/boringssl-with-bazel/src/crypto/evp/print.c \
    third_party/boringssl-with-bazel/src/crypto/evp/scrypt.c \
    third_party/boringssl-with-bazel/src/crypto/evp/sign.c \
    third_party/boringssl-with-bazel/src/crypto/ex_data.c \
    third_party/boringssl-with-bazel/src/crypto/fipsmodule/bcm.c \
    third_party/boringssl-with-bazel/src/crypto/fipsmodule/fips_shared_support.c \
    third_party/boringssl-with-bazel/src/crypto/hkdf/hkdf.c \
    third_party/boringssl-with-bazel/src/crypto/hpke/hpke.c \
    third_party/boringssl-with-bazel/src/crypto/hrss/hrss.c \
    third_party/boringssl-with-bazel/src/crypto/lhash/lhash.c \
    third_party/boringssl-with-bazel/src/crypto/mem.c \
    third_party/boringssl-with-bazel/src/crypto/obj/obj.c \
    third_party/boringssl-with-bazel/src/crypto/obj/obj_xref.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_all.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_info.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_lib.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_oth.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_pk8.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_pkey.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_x509.c \
    third_party/boringssl-with-bazel/src/crypto/pem/pem_xaux.c \
    third_party/boringssl-with-bazel/src/crypto/pkcs7/pkcs7.c \
    third_party/boringssl-with-bazel/src/crypto/pkcs7/pkcs7_x509.c \
    third_party/boringssl-with-bazel/src/crypto/pkcs8/p5_pbev2.c \
    third_party/boringssl-with-bazel/src/crypto/pkcs8/pkcs8.c \
    third_party/boringssl-with-bazel/src/crypto/pkcs8/pkcs8_x509.c \
    third_party/boringssl-with-bazel/src/crypto/poly1305/poly1305.c \
    third_party/boringssl-with-bazel/src/crypto/poly1305/poly1305_arm.c \
    third_party/boringssl-with-bazel/src/crypto/poly1305/poly1305_vec.c \
    third_party/boringssl-with-bazel/src/crypto/pool/pool.c \
    third_party/boringssl-with-bazel/src/crypto/rand_extra/deterministic.c \
    third_party/boringssl-with-bazel/src/crypto/rand_extra/forkunsafe.c \
    third_party/boringssl-with-bazel/src/crypto/rand_extra/fuchsia.c \
    third_party/boringssl-with-bazel/src/crypto/rand_extra/passive.c \
    third_party/boringssl-with-bazel/src/crypto/rand_extra/rand_extra.c \
    third_party/boringssl-with-bazel/src/crypto/rand_extra/windows.c \
    third_party/boringssl-with-bazel/src/crypto/rc4/rc4.c \
    third_party/boringssl-with-bazel/src/crypto/refcount_c11.c \
    third_party/boringssl-with-bazel/src/crypto/refcount_lock.c \
    third_party/boringssl-with-bazel/src/crypto/rsa_extra/rsa_asn1.c \
    third_party/boringssl-with-bazel/src/crypto/rsa_extra/rsa_print.c \
    third_party/boringssl-with-bazel/src/crypto/siphash/siphash.c \
    third_party/boringssl-with-bazel/src/crypto/stack/stack.c \
    third_party/boringssl-with-bazel/src/crypto/thread.c \
    third_party/boringssl-with-bazel/src/crypto/thread_none.c \
    third_party/boringssl-with-bazel/src/crypto/thread_pthread.c \
    third_party/boringssl-with-bazel/src/crypto/thread_win.c \
    third_party/boringssl-with-bazel/src/crypto/trust_token/pmbtoken.c \
    third_party/boringssl-with-bazel/src/crypto/trust_token/trust_token.c \
    third_party/boringssl-with-bazel/src/crypto/trust_token/voprf.c \
    third_party/boringssl-with-bazel/src/crypto/x509/a_digest.c \
    third_party/boringssl-with-bazel/src/crypto/x509/a_sign.c \
    third_party/boringssl-with-bazel/src/crypto/x509/a_verify.c \
    third_party/boringssl-with-bazel/src/crypto/x509/algorithm.c \
    third_party/boringssl-with-bazel/src/crypto/x509/asn1_gen.c \
    third_party/boringssl-with-bazel/src/crypto/x509/by_dir.c \
    third_party/boringssl-with-bazel/src/crypto/x509/by_file.c \
    third_party/boringssl-with-bazel/src/crypto/x509/i2d_pr.c \
    third_party/boringssl-with-bazel/src/crypto/x509/name_print.c \
    third_party/boringssl-with-bazel/src/crypto/x509/rsa_pss.c \
    third_party/boringssl-with-bazel/src/crypto/x509/t_crl.c \
    third_party/boringssl-with-bazel/src/crypto/x509/t_req.c \
    third_party/boringssl-with-bazel/src/crypto/x509/t_x509.c \
    third_party/boringssl-with-bazel/src/crypto/x509/t_x509a.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_att.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_cmp.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_d2.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_def.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_ext.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_lu.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_obj.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_req.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_set.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_trs.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_txt.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_v3.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_vfy.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509_vpm.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509cset.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509name.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509rset.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x509spki.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_algor.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_all.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_attrib.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_crl.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_exten.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_info.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_name.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_pkey.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_pubkey.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_req.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_sig.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_spki.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_val.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_x509.c \
    third_party/boringssl-with-bazel/src/crypto/x509/x_x509a.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/pcy_cache.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/pcy_data.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/pcy_lib.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/pcy_map.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/pcy_node.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/pcy_tree.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_akey.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_akeya.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_alt.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_bcons.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_bitst.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_conf.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_cpols.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_crld.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_enum.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_extku.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_genn.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_ia5.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_info.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_int.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_lib.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_ncons.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_ocsp.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_pci.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_pcia.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_pcons.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_pmaps.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_prn.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_purp.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_skey.c \
    third_party/boringssl-with-bazel/src/crypto/x509v3/v3_utl.c \
    third_party/boringssl-with-bazel/src/ssl/bio_ssl.cc \
    third_party/boringssl-with-bazel/src/ssl/d1_both.cc \
    third_party/boringssl-with-bazel/src/ssl/d1_lib.cc \
    third_party/boringssl-with-bazel/src/ssl/d1_pkt.cc \
    third_party/boringssl-with-bazel/src/ssl/d1_srtp.cc \
    third_party/boringssl-with-bazel/src/ssl/dtls_method.cc \
    third_party/boringssl-with-bazel/src/ssl/dtls_record.cc \
    third_party/boringssl-with-bazel/src/ssl/encrypted_client_hello.cc \
    third_party/boringssl-with-bazel/src/ssl/extensions.cc \
    third_party/boringssl-with-bazel/src/ssl/handoff.cc \
    third_party/boringssl-with-bazel/src/ssl/handshake.cc \
    third_party/boringssl-with-bazel/src/ssl/handshake_client.cc \
    third_party/boringssl-with-bazel/src/ssl/handshake_server.cc \
    third_party/boringssl-with-bazel/src/ssl/s3_both.cc \
    third_party/boringssl-with-bazel/src/ssl/s3_lib.cc \
    third_party/boringssl-with-bazel/src/ssl/s3_pkt.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_aead_ctx.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_asn1.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_buffer.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_cert.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_cipher.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_file.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_key_share.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_lib.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_privkey.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_session.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_stat.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_transcript.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_versions.cc \
    third_party/boringssl-with-bazel/src/ssl/ssl_x509.cc \
    third_party/boringssl-with-bazel/src/ssl/t1_enc.cc \
    third_party/boringssl-with-bazel/src/ssl/tls13_both.cc \
    third_party/boringssl-with-bazel/src/ssl/tls13_client.cc \
    third_party/boringssl-with-bazel/src/ssl/tls13_enc.cc \
    third_party/boringssl-with-bazel/src/ssl/tls13_server.cc \
    third_party/boringssl-with-bazel/src/ssl/tls_method.cc \
    third_party/boringssl-with-bazel/src/ssl/tls_record.cc \
    third_party/re2/re2/bitstate.cc \
    third_party/re2/re2/compile.cc \
    third_party/re2/re2/dfa.cc \
    third_party/re2/re2/filtered_re2.cc \
    third_party/re2/re2/mimics_pcre.cc \
    third_party/re2/re2/nfa.cc \
    third_party/re2/re2/onepass.cc \
    third_party/re2/re2/parse.cc \
    third_party/re2/re2/perl_groups.cc \
    third_party/re2/re2/prefilter.cc \
    third_party/re2/re2/prefilter_tree.cc \
    third_party/re2/re2/prog.cc \
    third_party/re2/re2/re2.cc \
    third_party/re2/re2/regexp.cc \
    third_party/re2/re2/set.cc \
    third_party/re2/re2/simplify.cc \
    third_party/re2/re2/stringpiece.cc \
    third_party/re2/re2/tostring.cc \
    third_party/re2/re2/unicode_casefold.cc \
    third_party/re2/re2/unicode_groups.cc \
    third_party/re2/util/pcre.cc \
    third_party/re2/util/rune.cc \
    third_party/re2/util/strutil.cc \
    third_party/upb/upb/decode.c \
    third_party/upb/upb/decode_fast.c \
    third_party/upb/upb/def.c \
    third_party/upb/upb/encode.c \
    third_party/upb/upb/msg.c \
    third_party/upb/upb/reflection.c \
    third_party/upb/upb/table.c \
    third_party/upb/upb/text_encode.c \
    third_party/upb/upb/upb.c \
    , $ext_shared, , -fvisibility=hidden \
    -DOPENSSL_NO_ASM -D_GNU_SOURCE -DWIN32_LEAN_AND_MEAN \
    -D_HAS_EXCEPTIONS=0 -DNOMINMAX -DGRPC_ARES=0 \
    -DGRPC_POSIX_FORK_ALLOW_PTHREAD_ATFORK=1 \
    -DGRPC_XDS_USER_AGENT_NAME_SUFFIX='"\"PHP\""' \
    -DGRPC_XDS_USER_AGENT_VERSION_SUFFIX='"\"1.43.0dev\""')

  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/census)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/health)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/grpclb)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/pick_first)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/priority)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/ring_hash)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/rls)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/round_robin)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/weighted_target)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/lb_policy/xds)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/binder)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/dns)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/dns/c_ares)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/dns/native)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/fake)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/google_c2p)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/sockaddr)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_channel/resolver/xds)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/client_idle)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/deadline)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/fault_injection)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/http)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/http/client)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/http/message_compress)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/http/server)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/max_age)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/message_size)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/rbac)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/filters/server_config_selector)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/service_config)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/alpn)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/client)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/client/insecure)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/client/secure)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/server)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/server/insecure)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/server/secure)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/chttp2/transport)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/transport/inproc)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/admin/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/annotations)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/accesslog/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/bootstrap/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/cluster/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/core/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/endpoint/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/listener/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/metrics/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/overload/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/rbac/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/route/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/config/trace/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/extensions/clusters/aggregate/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/extensions/filters/common/fault/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/extensions/filters/http/fault/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/extensions/filters/http/router/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/extensions/filters/network/http_connection_manager/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/extensions/transport_sockets/tls/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/service/cluster/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/service/discovery/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/service/endpoint/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/service/listener/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/service/load_stats/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/service/route/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/service/status/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/type/http/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/type/matcher/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/type/metadata/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/type/tracing/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/envoy/type/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/google/api)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/google/api/expr/v1alpha1)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/google/protobuf)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/google/rpc)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/src/proto/grpc/gcp)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/src/proto/grpc/health/v1)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/src/proto/grpc/lb/v1)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/src/proto/grpc/lookup/v1)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/udpa/annotations)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/validate)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/xds/annotations/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/xds/core/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/xds/data/orca/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upb-generated/xds/type/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/admin/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/annotations)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/accesslog/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/bootstrap/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/cluster/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/core/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/endpoint/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/listener/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/metrics/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/overload/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/route/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/config/trace/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/extensions/clusters/aggregate/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/extensions/filters/common/fault/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/extensions/filters/http/fault/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/extensions/filters/http/router/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/extensions/filters/network/http_connection_manager/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/extensions/transport_sockets/tls/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/service/cluster/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/service/discovery/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/service/endpoint/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/service/listener/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/service/load_stats/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/service/route/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/service/status/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/type/http/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/type/matcher/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/type/metadata/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/type/tracing/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/envoy/type/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/google/api)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/google/protobuf)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/google/rpc)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/udpa/annotations)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/validate)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/xds/annotations/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/xds/core/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/upbdefs-generated/xds/type/v3)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/ext/xds)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/address_utils)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/backoff)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/channel)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/compression)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/config)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/debug)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/event_engine)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/gpr)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/gprpp)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/http)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/iomgr)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/iomgr/event_engine)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/iomgr/executor)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/json)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/matchers)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/profiling)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/promise)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/resource_quota)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/authorization)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/context)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/alts)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/composite)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/external)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/fake)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/google_default)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/iam)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/insecure)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/jwt)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/local)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/oauth2)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/plugin)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/ssl)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/tls)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/credentials/xds)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/security_connector)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/security_connector/alts)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/security_connector/fake)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/security_connector/insecure)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/security_connector/local)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/security_connector/ssl)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/security_connector/tls)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/transport)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/security/util)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/slice)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/surface)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/transport)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/lib/uri)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/plugin_registry)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/tsi)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/tsi/alts/crypt)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/tsi/alts/frame_protector)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/tsi/alts/handshaker)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/tsi/alts/zero_copy_frame_protector)
  PHP_ADD_BUILD_DIR($ext_builddir/src/core/tsi/ssl/session_cache)
  PHP_ADD_BUILD_DIR($ext_builddir/src/php/ext/grpc)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/base)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/base/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/container/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/debugging)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/debugging/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/hash/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/numeric)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/profiling/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/random)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/random/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/status)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/strings)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/strings/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/strings/internal/str_format)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/synchronization)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/synchronization/internal)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/time)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/time/internal/cctz/src)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/abseil-cpp/absl/types)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/address_sorting)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/asn1)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/base64)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/bio)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/blake2)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/bn_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/buf)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/bytestring)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/chacha)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/cipher_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/cmac)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/conf)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/curve25519)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/dh_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/digest_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/dsa)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/ec_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/ecdh_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/ecdsa_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/engine)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/err)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/evp)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/fipsmodule)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/hkdf)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/hpke)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/hrss)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/lhash)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/obj)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/pem)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/pkcs7)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/pkcs8)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/poly1305)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/pool)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/rand_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/rc4)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/rsa_extra)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/siphash)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/stack)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/trust_token)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/x509)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/crypto/x509v3)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/boringssl-with-bazel/src/ssl)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/re2/re2)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/re2/util)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/upb/upb)
fi
