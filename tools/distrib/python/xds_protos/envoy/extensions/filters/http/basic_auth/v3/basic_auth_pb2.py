# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: envoy/extensions/filters/http/basic_auth/v3/basic_auth.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
from google.protobuf.internal import builder as _builder
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from envoy.config.core.v3 import base_pb2 as envoy_dot_config_dot_core_dot_v3_dot_base__pb2
from udpa.annotations import sensitive_pb2 as udpa_dot_annotations_dot_sensitive__pb2
from udpa.annotations import status_pb2 as udpa_dot_annotations_dot_status__pb2
from validate import validate_pb2 as validate_dot_validate__pb2


DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n<envoy/extensions/filters/http/basic_auth/v3/basic_auth.proto\x12+envoy.extensions.filters.http.basic_auth.v3\x1a\x1f\x65nvoy/config/core/v3/base.proto\x1a udpa/annotations/sensitive.proto\x1a\x1dudpa/annotations/status.proto\x1a\x17validate/validate.proto\"r\n\tBasicAuth\x12\x37\n\x05users\x18\x01 \x01(\x0b\x32 .envoy.config.core.v3.DataSourceB\x06\xb8\xb7\x8b\xa4\x02\x01\x12,\n\x17\x66orward_username_header\x18\x02 \x01(\tB\x0b\xfa\x42\x08r\x06\xc0\x01\x01\xc8\x01\x00\"T\n\x11\x42\x61sicAuthPerRoute\x12?\n\x05users\x18\x01 \x01(\x0b\x32 .envoy.config.core.v3.DataSourceB\x0e\xfa\x42\x05\x8a\x01\x02\x10\x01\xb8\xb7\x8b\xa4\x02\x01\x42\xb6\x01\n9io.envoyproxy.envoy.extensions.filters.http.basic_auth.v3B\x0e\x42\x61sicAuthProtoP\x01Z_github.com/envoyproxy/go-control-plane/envoy/extensions/filters/http/basic_auth/v3;basic_authv3\xba\x80\xc8\xd1\x06\x02\x10\x02\x62\x06proto3')

_globals = globals()
_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'envoy.extensions.filters.http.basic_auth.v3.basic_auth_pb2', _globals)
if _descriptor._USE_C_DESCRIPTORS == False:
  DESCRIPTOR._options = None
  DESCRIPTOR._serialized_options = b'\n9io.envoyproxy.envoy.extensions.filters.http.basic_auth.v3B\016BasicAuthProtoP\001Z_github.com/envoyproxy/go-control-plane/envoy/extensions/filters/http/basic_auth/v3;basic_authv3\272\200\310\321\006\002\020\002'
  _BASICAUTH.fields_by_name['users']._options = None
  _BASICAUTH.fields_by_name['users']._serialized_options = b'\270\267\213\244\002\001'
  _BASICAUTH.fields_by_name['forward_username_header']._options = None
  _BASICAUTH.fields_by_name['forward_username_header']._serialized_options = b'\372B\010r\006\300\001\001\310\001\000'
  _BASICAUTHPERROUTE.fields_by_name['users']._options = None
  _BASICAUTHPERROUTE.fields_by_name['users']._serialized_options = b'\372B\005\212\001\002\020\001\270\267\213\244\002\001'
  _globals['_BASICAUTH']._serialized_start=232
  _globals['_BASICAUTH']._serialized_end=346
  _globals['_BASICAUTHPERROUTE']._serialized_start=348
  _globals['_BASICAUTHPERROUTE']._serialized_end=432
# @@protoc_insertion_point(module_scope)