cc_test(
    name = "hello-test",
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