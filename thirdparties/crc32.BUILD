CRC32_HEADERS = glob([
        "include/*.h",
        "src/*.h",
        "*.h"
])
CRC32_SOURCES = glob([
        "src/*.cc",
])
cc_library(
    name = "crc32",
    hdrs = CRC32_HEADERS,
    srcs = CRC32_SOURCES,
    visibility = ["//visibility:public"],
)