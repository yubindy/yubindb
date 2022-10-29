
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

http_archive(
    name = "com_github_cschuet_crc32c",
    sha256 = "81adcc96018fe226ae9b32eba74ffea266409891f220ad85fdd043d80edae598",
    strip_prefix = "crc32c-858907d2ed420b0738941df751c85a7c7588a7b6",
    urls = [
        "https://github.com/cschuet/crc32c/archive/858907d2ed420b0738941df751c85a7c7588a7b6.tar.gz",
    ],
)

load("@com_github_cschuet_crc32c//:bazel/repositories.bzl", "repositories")

repositories()