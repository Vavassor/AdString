cmake_minimum_required(VERSION 3.0)
project(AdString VERSION 1.0.0)

add_library(AdString "")

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
    ad_number_format.c
    ad_string.c
)
