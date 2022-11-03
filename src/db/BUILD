cc_library(
    name = "util",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    visibility = ["//visibility:public"],
    deps = [
         "@spdlog",
    ],
)

cc_binary(
    name = "db",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    deps = [
        "@folly//:folly",
    ],
)