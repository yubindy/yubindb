
workspace(name = "yubindb")

workspace(name = "yubindb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

new_local_repository(
    name = "skiplist",
    path = "/home/yubtin/Desktop/codes/skiplist",
    build_file = "thirdparties/skiplist.BUILD",
)

new_local_repository(
    name = "spdlog",
    path = "/usr/src/spdlog",
    build_file = "thirdparties/spdlog.BUILD",
)

new_local_repository(
    name = "crc32",
    path = "/usr/src/crc32c",
    build_file = "thirdparties/crc32.BUILD",
)