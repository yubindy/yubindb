
workspace(name = "yubindb")


load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


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


-- http_archive(
--     name = "com_github_cschuet_snappy",
--     strip_prefix = "snappy-7b7f8fc8e162bbf24ad31fa046d995703179a3be",
--     sha256 = "a62d0fa46f3efb3cec7779d95bbf3320195d2f106d63db5a029eeebf0e7ec67f",
--     urls = [
--         "https://github.com/cschuet/snappy/archive/7b7f8fc8e162bbf24ad31fa046d995703179a3be.tar.gz",
--     ],
-- )

-- load("@com_github_cschuet_snappy//:bazel/repositories.bzl", "repositories")

-- repositories()