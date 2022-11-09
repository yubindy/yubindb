cc_library(
    name = "util",
    srcs = glob([
        "*.cpp",
        "*.h",
        "src/db/snapshot.h",
        "src/db/snapshot.cpp",
    ]),
    visibility = ["//visibility:public"],
    deps = [
         "@spdlog",
         "@crc32",
         "@skiplist",
    ],
)