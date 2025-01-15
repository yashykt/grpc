# Copyright 2023 The gRPC Authors
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

"""
This file is generated by generate_dockerimage_current_versions_bzl.sh
It makes the info from testing docker image *.current_version files
accessible to bazel builds.
"""

DOCKERIMAGE_CURRENT_VERSIONS = {
    "third_party/rake-compiler-dock/rake_aarch64-linux.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_aarch64-linux@sha256:9ecc73e45f0a8d034f924227ccb84e0f8a1f05baec912f563400792a5cc8eb08",
    "third_party/rake-compiler-dock/rake_arm64-darwin.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_arm64-darwin@sha256:0f816ad0b08b0826fe64a43750e3e56663d8a697f753e98292994aa988fc9ccc",
    "third_party/rake-compiler-dock/rake_x64-mingw-ucrt.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_x64-mingw-ucrt@sha256:5a7704d78bb2cd53e96bc8bf8615a85f00d472d02d6cd1b29128308f933c2100",
    "third_party/rake-compiler-dock/rake_x64-mingw32.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_x64-mingw32@sha256:118ace4490d745d2dcf32ee4408406c7cc83264472bcbd278d79aa41930add35",
    "third_party/rake-compiler-dock/rake_x86-linux.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_x86-linux@sha256:370102e9eb43ff48da46743f8ff134fa2b4e5041a118958ca42cd316b52c41a8",
    "third_party/rake-compiler-dock/rake_x86-mingw32.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_x86-mingw32@sha256:52b08ed6ceb7e0ac005e5cf5b9d04289ca095f8517c24957741675df45ef4617",
    "third_party/rake-compiler-dock/rake_x86_64-darwin.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_x86_64-darwin@sha256:c5af54f9f41da6d55f1b8f802923469d870cc9d28918086eaa8a79a18ba2337c",
    "third_party/rake-compiler-dock/rake_x86_64-linux.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rake_x86_64-linux@sha256:c46de9a416ab2e32cbfb135e70e5f6b453da2779ebaf24e57976ec0573f83c51",
    "tools/dockerfile/distribtest/cpp_debian11_aarch64_cross_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cpp_debian11_aarch64_cross_x64@sha256:97ff55660fb93f4b31c029f2935bd18edf82e55d4df65376e55ed97b228a49c7",
    "tools/dockerfile/distribtest/cpp_debian11_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cpp_debian11_x64@sha256:beb3a8bedc067f4c8f54c8efe31f3d7149056c39c9751aaa395c788dd54e0a7e",
    "tools/dockerfile/distribtest/csharp_alpine_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_alpine_x64@sha256:65083a6c7b1e6f18374ac6218a924409dfc6ab526ec627b9a33d81e21c93e025",
    "tools/dockerfile/distribtest/csharp_centos7_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_centos7_x64@sha256:ec715dd5fbd621789e7598c8d4ac346a7b4037b0cc83fbb29990dc8e4c1f1a13",
    "tools/dockerfile/distribtest/csharp_debian11_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_debian11_x64@sha256:56db96cbb341ee2336a492b86f479814f8e819fd73499dca1f7c62e52c5c953a",
    "tools/dockerfile/distribtest/csharp_dotnet31_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_dotnet31_x64@sha256:b7afb4aa2f4ac65df757eb42fa78ee105fdde856feda0a9deeb5afedf87a5410",
    "tools/dockerfile/distribtest/csharp_dotnet5_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_dotnet5_x64@sha256:004e02902825b54c7f0d60eaed0819acf6f10c24853bf8f793001114e9969abd",
    "tools/dockerfile/distribtest/csharp_ubuntu2204_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_ubuntu2204_x64@sha256:a439f2ccbc666f231e511a8e58eb7f66a3de4820a4d5aded1e62275cf8ac49f0",
    "tools/dockerfile/distribtest/php8_debian12_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/php8_debian12_x64@sha256:42399d061f234ee0ef79b333555db0e11ca4f106bb1d49276abf99c459c104f0",
    "tools/dockerfile/distribtest/php8_debian12_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_php7_kannanj-testing:latest",
    "tools/dockerfile/distribtest/python_alpine_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_alpine_x64@sha256:1bbc6fa5b4b650d3037d089e164d364e05a6daf1ed6dd1025ba07cc127f73d7d",
    "tools/dockerfile/distribtest/python_arch_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_arch_x64@sha256:2c1adadeb010e107132cf5137f32a2d18727796631245b110cc74f69c07502e1",
    "tools/dockerfile/distribtest/python_bullseye_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_bullseye_x64@sha256:80553398f0c59c1dc186052f4f2deaf18fea582f6d1d166eec6ea298639031fb",
    "tools/dockerfile/distribtest/python_bullseye_x86.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_bullseye_x86@sha256:81300aed2bb5e6b3f9bf9f23ba751e884e0317a9cd79659d37cdef4f1cfd24ca",
    "tools/dockerfile/distribtest/python_centos7_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_centos7_x64@sha256:13d8f3ae273ac7dab9b5d486ca08045f6ded866c405ed7bb9d2c87574c1b911b",
    "tools/dockerfile/distribtest/python_dev_alpine3.18_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_alpine3.18_x64@sha256:3ac23840cea2a761ac104ef749b43fa652966eba96c0cbe4c8cbac58513a8099",
    "tools/dockerfile/distribtest/python_dev_arch_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_arch_x64@sha256:29f179ef2083ee6addd57e90f58781fdc1cb5dc3dd3e228da1af38785b921f35",
    "tools/dockerfile/distribtest/python_dev_bullseye_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_bullseye_x64@sha256:6c15bd5c350dbc206f45fb512418db51db9a3b73cc304d1d28f4df38bad5a702",
    "tools/dockerfile/distribtest/python_dev_bullseye_x86.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_bullseye_x86@sha256:0cbb08f31a9d45ccdc41a88e409b215cac69cc1a9b05f456ebd96b398fd8e9ee",
    "tools/dockerfile/distribtest/python_dev_centos7_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_centos7_x64@sha256:e32238c1c46a752881c6c165b918cb24bd182e3957dc72c514fdbd3d5c3312ab",
    "tools/dockerfile/distribtest/python_dev_fedora39_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_fedora39_x64@sha256:19e636a63b7d812a0a70b3334ab45ad1593ace4303268eef42c2155770ef08d3",
    "tools/dockerfile/distribtest/python_dev_ubuntu2004_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_ubuntu2004_x64@sha256:cdae114cff8a07ea5fe8738a5cf786075c78ae2bda886e3cc59bcd4890eb78ee",
    "tools/dockerfile/distribtest/python_dev_ubuntu2204_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_dev_ubuntu2204_x64@sha256:4fc253b24afa128d6295c4e1ac71ea4f8602c7153f4e2e0f38433c14236ec643",
    "tools/dockerfile/distribtest/python_fedora39_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_fedora39_x64@sha256:f2ad099b0d3553f37077705aaf6d221906fba88f0502e7131024a936ec40f58e",
    "tools/dockerfile/distribtest/python_opensuse_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_opensuse_x64@sha256:962e835125423300b800e0448b698de0ff8397819b90c2a8c15fef281619f1b6",
    "tools/dockerfile/distribtest/python_python38_buster_aarch64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_python38_buster_aarch64@sha256:0a93bf2a0303aebe1280bafad69df228b9444af9144c767d8169ecc70fb383f6",
    "tools/dockerfile/distribtest/python_ubuntu2004_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_ubuntu2004_x64@sha256:288cf72bc98fc384b9352d1f6d258b3513925ffe5746dda7e2e343723dd5f733",
    "tools/dockerfile/distribtest/python_ubuntu2204_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_ubuntu2204_x64@sha256:6054d639247a93af2b496f3c1ce48f63b2e07f5ba54e025f69bb232a747c644e",
    "tools/dockerfile/distribtest/ruby_debian11_x64_ruby_3_0.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_debian11_x64_ruby_3_0@sha256:05c579d93764f12db1a60fa78a26e0f4d6179e54187a3a531c8ff955001731ec",
    "tools/dockerfile/distribtest/ruby_debian11_x64_ruby_3_1.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_debian11_x64_ruby_3_1@sha256:a48bb08275a588fbcea21b6b6056514b69454f6844bd7db9fd72c796892d02e1",
    "tools/dockerfile/distribtest/ruby_debian11_x64_ruby_3_2.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_debian11_x64_ruby_3_2@sha256:9604f8d07c3ea330cdc1ebe394f67828710bbfef52f0dc144e513e3627279b5a",
    "tools/dockerfile/distribtest/ruby_debian11_x64_ruby_3_3.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_debian11_x64_ruby_3_3@sha256:38f27eb75a55df6e9a7d7a01bca40448d0971faee94d0332324ad85ee7200bac",
    "tools/dockerfile/distribtest/ruby_debian11_x64_ruby_3_4.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_debian11_x64_ruby_3_4@sha256:d37c32dc640c9d6c7c47fb11d0acd31a2877ad6ce21e1a4f17a90a963193b776",
    "tools/dockerfile/distribtest/ruby_ubuntu2004_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_ubuntu2004_x64@sha256:d754f48a8119b725613bc3c31548ee8770d9bfddd9f49f00f14651aba8e7f760",
    "tools/dockerfile/distribtest/ruby_ubuntu2204_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_ubuntu2204_x64@sha256:3f0558a0164bd7d41dfdb3d5635951503181bd619fce9ce8d390bdc675444eb3",
    "tools/dockerfile/grpc_artifact_centos6_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_centos6_x64@sha256:3285047265ea2b7c5d4df4c769b2d05f56288d947c75e16d27ae2dee693f791b",
    "tools/dockerfile/grpc_artifact_centos6_x86.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_centos6_x86@sha256:19783239da92208f0f39cf563529cd02e889920497ef81c60d20391fa998af62",
    "tools/dockerfile/grpc_artifact_protoc_aarch64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_protoc_aarch64@sha256:c3a6556d0e9153574279e998b0ff03eb8b49221a3b87704c23409287fae5131a",
    "tools/dockerfile/grpc_artifact_python_linux_armv7.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_python_linux_armv7@sha256:137616275fe59bf89849f7503779f9a5d9668dbf395fcc79a8221a8c38fa5bac",
    "tools/dockerfile/grpc_artifact_python_manylinux2014_aarch64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_python_manylinux2014_aarch64@sha256:263ea79d940c905233624b02751194474408338b0f5ae2fb822966b2f20f47ce",
    "tools/dockerfile/grpc_artifact_python_manylinux2014_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_python_manylinux2014_x64@sha256:0e102df67f31aeb6afe68250603288c2f1c98ccf360d1c42d751b8451da94b48",
    "tools/dockerfile/grpc_artifact_python_manylinux2014_x86.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_python_manylinux2014_x86@sha256:527e2e9ec4db0c52a53b50abfd59907a1b7e221168dc401686f6a48d33bddc5c",
    "tools/dockerfile/grpc_artifact_python_musllinux_1_1_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_python_musllinux_1_1_x64@sha256:94b57e5ea31ebc29af734474bcaff3074770778e5d27557cdc06d755ee8bc7ed",
    "tools/dockerfile/grpc_artifact_python_musllinux_1_1_x86.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_artifact_python_musllinux_1_1_x86@sha256:edf4a0c8333c9309e52f323aa7315bbc0e5643216613cab4ecd2bce3d1ec26c0",
    "tools/dockerfile/interoptest/grpc_interop_aspnetcore.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_aspnetcore@sha256:8e2e732e78724a8382c340dca72e7653c5f82c251a3110fa2874cc00ba538878",
    "tools/dockerfile/interoptest/grpc_interop_cxx.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_cxx@sha256:e5a474d33773d52ec6a8abbe2d61ee0c2a9c2b5f48793a5ea3b82c4445becf3f",
    "tools/dockerfile/interoptest/grpc_interop_dart.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_dart@sha256:673b7fdf8f225f622d190fd5d2b499a210ce45ada13c919091733be1e18ea067",
    "tools/dockerfile/interoptest/grpc_interop_go.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_go@sha256:7affc40dd8c229a12aff76c44b173ce786fced97e563777640b5d696f1d8cd89",
    "tools/dockerfile/interoptest/grpc_interop_go1.11.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_go1.11@sha256:f2fe3a0a581c687ee4217bf58fd42b18bb1f63d3d006f1b67379ff553b0e23c6",
    "tools/dockerfile/interoptest/grpc_interop_go1.16.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_go1.16@sha256:3767f47c9d06584c6c07b7ab536e13f3e87550330e6c2652ad288d3a72b0de23",
    "tools/dockerfile/interoptest/grpc_interop_go1.19.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_go1.19@sha256:889e7ff34399a5e16af87940d1eaa239e56da307f7faca3f8f1d28379c2e3df3",
    "tools/dockerfile/interoptest/grpc_interop_go1.8.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_go1.8@sha256:7830a301b37539252c592b9cd7fa30a6142d0afc717a05fc8d2b82c74fb45efe",
    "tools/dockerfile/interoptest/grpc_interop_go1.x.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_go1.x@sha256:7affc40dd8c229a12aff76c44b173ce786fced97e563777640b5d696f1d8cd89",
    "tools/dockerfile/interoptest/grpc_interop_http2.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_http2@sha256:e3f247d8038374848fadf7215b841e3575c0b2a4217feb503a79b8004b164c5a",
    "tools/dockerfile/interoptest/grpc_interop_java.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_java@sha256:6c0319bbbf77d6c198f61adf92d205efd33cbd2cccaf914a1af8f2dcb61b16a1",
    "tools/dockerfile/interoptest/grpc_interop_node.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_node@sha256:549a7683cc024fb7ffee807eaa8acb6c2d5a5dd0184a2590f91852804eea39a4",
    "tools/dockerfile/interoptest/grpc_interop_nodepurejs.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_nodepurejs@sha256:62f148c4e8b031d02d445e993a840c53eb61566ad0bc007f1d89a3387d1f73a7",
    "tools/dockerfile/interoptest/grpc_interop_php7.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_php7@sha256:eea8738f0aa1b6bbd56b847cbdfd0e0b98a46306d86db2494557f25c334abe4d",
    "tools/dockerfile/interoptest/grpc_interop_python.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_python@sha256:b3e62554f360f18672711affb79c509a80083a1ddb8e5221bd3b9823a596d0fb",
    "tools/dockerfile/interoptest/grpc_interop_pythonasyncio.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_pythonasyncio@sha256:c349c6e92d32f42e619134ef34d6d33e33e0aa106ea66388b2c22e4231a7baa6",
    "tools/dockerfile/interoptest/grpc_interop_ruby.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/grpc_interop_ruby@sha256:ce9539e9068b2597d5d43dc2bb7df17b84fd8192759cf7a02132b676ca6fa4ed",
    "tools/dockerfile/interoptest/lb_interop_fake_servers.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/lb_interop_fake_servers@sha256:b89a51dd9147e1293f50ee64dd719fce5929ca7894d3770a3d80dbdecb99fd52",
    "tools/dockerfile/test/android_ndk.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/android_ndk@sha256:e4fd2e0048c4ffd2a04e4a41154ee469ed61e058cb704ee95071f7a3bdad507a",
    "tools/dockerfile/test/bazel.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/bazel@sha256:a0ca2c4f113c4d9e62e67b36d1734c0ffc3baff851cc48f23616df82484f905d",
    "tools/dockerfile/test/bazel_arm64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/bazel_arm64@sha256:2728d010d87cb1845d002efbc3d08dadc283e8d0b7e0b582c2623960af61d2c5",
    "tools/dockerfile/test/csharp_debian11_arm64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_debian11_arm64@sha256:4d4bc5f15e03f3d3d8fd889670ecde2c66a2e4d2dd9db80733c05c1d90c8a248",
    "tools/dockerfile/test/csharp_debian11_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/csharp_debian11_x64@sha256:0763d919b17b4cfe5b65aff3bf911c04e9e4d46d11649858742033facd9f534f",
    "tools/dockerfile/test/cxx_alpine_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_alpine_x64@sha256:f49643b001fde1559e1607fa2735230c0a2acf97fb854e3bd56fb9e1c419883d",
    "tools/dockerfile/test/cxx_clang_19_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_clang_19_x64@sha256:2b5e396dbb9ef751471eb5c5433cbf3e694616daf0e8f9df53f0933e7796de5d",
    "tools/dockerfile/test/cxx_clang_7_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_clang_7_x64@sha256:b35f94648ada85aab9110dae0ebca594cd278672d4bbce38312cefa372952cae",
    "tools/dockerfile/test/cxx_debian11_openssl102_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_debian11_openssl102_x64@sha256:477ae0da7ff7faa9cf195c0d32472fec4cf8b7325505c63e00b5c794c9a4b1a7",
    "tools/dockerfile/test/cxx_debian11_openssl111_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_debian11_openssl111_x64@sha256:d383e66d4a089f9305768e3037faa2a887ff91565b0f3ddd96985dca94e9754f",
    "tools/dockerfile/test/cxx_debian11_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_debian11_x64@sha256:9f9285da21c1053ac715027e0cee66c20c70ebf016053328a4cee61ffd37e59b",
    "tools/dockerfile/test/cxx_debian11_x86.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_debian11_x86@sha256:3f505ad99e52a4b3337fedb413e883bc8e5c1d9c089299c34002b89e01254d3b",
    "tools/dockerfile/test/cxx_debian12_openssl309_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_debian12_openssl309_x64@sha256:f75bb715c4f9464526f9affb410f7965a0b8894516d7d98cd89a4e165ae065b7",
    "tools/dockerfile/test/cxx_gcc_14_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_gcc_14_x64@sha256:54685fc729952b689318057a9769edc92247a40d607d01c3517d2644d361cc73",
    "tools/dockerfile/test/cxx_gcc_7_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_gcc_7_x64@sha256:e1925d7f08a7f167c6aab2f8284a3f13b7f3830ba38876e6de1dee2ffd3c7d2f",
    "tools/dockerfile/test/cxx_gcc_8_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/cxx_gcc_8_x64@sha256:19511527fc3da78344a7ece5f1cf8ba06d928202ba5b3ca039dccb86d1a417fc",
    "tools/dockerfile/test/php8_debian12_arm64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/php8_debian12_arm64@sha256:c220e72b415ad194a41cf5640fe360b9358df1dd34553c8d8e6bfd4a82e032b3",
    "tools/dockerfile/test/php8_debian12_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/php8_debian12_x64@sha256:18bf37f80415ce3ddd48261b0ef97b421a85646cc15bd21348e19b5fd7b191d6",
    "tools/dockerfile/test/php8_zts_debian12_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/php8_zts_debian12_x64@sha256:911241a4e38046b603245d56a1ca9491e81990ca9dedb0671e41b15387e3d2ee",
    "tools/dockerfile/test/python_alpine_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_alpine_x64@sha256:01e99e5dcee30255a4683ebd308b6484b5aa4624dd514fc7d83df5fd1a0144be",
    "tools/dockerfile/test/python_debian11_default_arm64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_debian11_default_arm64@sha256:fccca33a655c7aa89dd7ebd9492cbcc1f636bd2a004cd939d1982cfce3d68326",
    "tools/dockerfile/test/python_debian11_default_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/python_debian11_default_x64@sha256:8beca964d6bbb80a8cb4c4628c25e9702b007c97113e74e589fa50c15000738d",
    "tools/dockerfile/test/rbe_ubuntu2004.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/rbe_ubuntu2004@sha256:b3eb1a17b7b091e3c5648a803076b2c40601242ff91c04d55997af6641305f68",
    "tools/dockerfile/test/ruby_debian11_arm64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_debian11_arm64@sha256:d2e79919b2e2d4cc36a29682ecb5170641df4fb506cfb453978ffdeb8a841bd9",
    "tools/dockerfile/test/ruby_debian11_x64.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/ruby_debian11_x64@sha256:6e8b4696ba0661f11a31ed0992a94d2efcd889a018f57160f0e2fb62963f3593",
    "tools/dockerfile/test/sanity.current_version": "docker://us-docker.pkg.dev/grpc-testing/testing-images-public/sanity@sha256:8b8e0b83725dfc5f81c901b3a737872410ee04fc1cb8b9ec5b2e4a5670fbf23c",
}
