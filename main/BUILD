
cc_library(
    name = "gc",
    copts = [
        "-std=c++14",
        "-I."
    ],
    srcs = [
        "main.cpp",
        "gc.cpp",
    ],
    hdrs = [
        "gc.h",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "bad-gc",
    copts = [
        "-std=c++14"
    ],
    srcs = [
        "main.cpp",
    ],
    deps = [
        "gc",
    ]
)