cc_test(
    name = "tests",
    srcs = ["main.cpp"],
    copts = [
        "-std=c++14",
        "-Imain",
        "-Iexternal/gtest/include",
    ],
    deps = [
        "@gtest//:main",
        "//main:gc",
    ],
)