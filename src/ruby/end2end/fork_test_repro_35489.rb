#!/usr/bin/env ruby
#
# Copyright 2016 gRPC authors.
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

ENV['GRPC_ENABLE_FORK_SUPPORT'] = "1"
fail "forking only supported on linux" unless RUBY_PLATFORM =~ /linux/
# TODO(apolcyn): remove after this experiment is on by default
ENV['GRPC_EXPERIMENTS'] = "event_engine_fork"

this_dir = File.expand_path(File.dirname(__FILE__))
protos_lib_dir = File.join(this_dir, 'lib')
grpc_lib_dir = File.join(File.dirname(this_dir), 'lib')
$LOAD_PATH.unshift(grpc_lib_dir) unless $LOAD_PATH.include?(grpc_lib_dir)
$LOAD_PATH.unshift(protos_lib_dir) unless $LOAD_PATH.include?(protos_lib_dir)
$LOAD_PATH.unshift(this_dir) unless $LOAD_PATH.include?(this_dir)

require 'grpc'
require 'end2end_common'

def do_rpc(stub)
  stub.echo(Echo::EchoRequest.new(request: 'hello'), deadline: Time.now + 300)
end

def main
  this_dir = File.expand_path(File.dirname(__FILE__))
  echo_server_path = File.join(this_dir, 'echo_server.rb')
  to_child_r, _to_child_w = IO.pipe
  to_parent_r, to_parent_w = IO.pipe
  # Note gRPC has not yet been initialized, otherwise we would need to call prefork
  # before spawn and postfork_parent after.
  # TODO(apolcyn): consider redirecting server's stderr to a file
  Process.spawn(RbConfig.ruby, echo_server_path, in: to_child_r, out: to_parent_w, err: "server_log")
  to_child_r.close
  to_parent_w.close
  child_port = to_parent_r.gets.strip
  STDERR.puts "server running on port: #{child_port}"
  key = "grpc." * 100_000 # large enough to malloc backing storage
  value = "testvalue" * 100_000
  stub = Echo::EchoServer::Stub.new("localhost:#{child_port}", :this_channel_is_insecure, channel_args: { key => value })
  with_logging("parent: GRPC.prefork") { GRPC.prefork }
  pid = fork do
    with_logging("child: GRPC.postfork_child") { GRPC.postfork_child }
    key.clear
    value.clear
    GC.compact
    with_logging("child: post-fork RPC") { do_rpc(stub) }
    STDERR.puts "child: done"
  end
  with_logging("parent: GRPC.postfork_parent") { GRPC.postfork_parent }
  Process.wait pid
  STDERR.puts "parent: done"
end

main
