cc_binary(
    name = "memtable",
    srcs = ["testmemtable.cpp"],
    deps = [
        "//src/db:db",
        "//src/util:util",
    ],
)
cc_binary(
    name = "log",
    srcs = ["testlog.cpp"],
    deps = [
        "//src/util:loginit",
    ],
)
cc_test(
    name = "bloom",
    srcs = ["testbloom.cpp"],
    deps = [
        "//src/util:util",
        "@com_google_googletest//:gtest_main",
    ],
)
cc_test(
    name = "table",
    srcs = ["testbloom.cpp"],
    deps = [
        "//src/util:db",
        "@com_google_googletest//:gtest_main",
    ],
)