cmake_minimum_required(VERSION 2.8)
project(AdString)
set(AdString_VERSION_MAJOR 1)
set(AdString_VERSION_MINOR 0)
set(AdString_VERSION_PATCH 0)

include(CTest)

add_library(AdString)

set_target_properties(
    AdString
    PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY bin
)

target_sources(
    AdString
    PRIVATE
    Source/ad_number_format.c
    Source/ad_string.c
)

add_subdirectory(Test)
