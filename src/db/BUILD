
cc_library(
    name = "db",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    visibility = ["//visibility:public"],
    deps = [
         "//src/util:util",
    ],
)


cc_binary(
    name = "comtest",
    srcs = ["main.cpp"],
    deps = [
        ":db",
        "//src/util:util",
    ],
)