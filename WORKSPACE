
workspace(name = "yubindb")


load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "com_google_googletest",
  urls = ["https://github.com/google/googletest/archive/release-1.10.0.zip"],
  strip_prefix = "googletest-release-1.10.0",
  sha256 = "94c634d499558a76fa649edb13721dce6e98fb1e7018dfaeba3cd7a083945e91",
)

http_archive(
    name = "hedron_compile_commands",

    # Replace the commit hash in both places (below) with the latest, rather than using the stale one here.
    # Even better, set up Renovate and let it do the work for you (see "Suggestion: Updates" in the README).
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/c200ce8b3e0baa04fa5cc3fb222260c9ea06541f.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-c200ce8b3e0baa04fa5cc3fb222260c9ea06541f",
    # When you first run this tool, it'll recommend a sha256 hash to put here with a message like: "DEBUG: Rule 'hedron_compile_commands' indicated that a canonical reproducible form can be obtained by modifying arguments sha256 = ..."
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()

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

new_local_repository(
    name = "snappy",
    path = "/usr/src/snappy",
    build_file = "thirdparties/snappy.BUILD"
)
