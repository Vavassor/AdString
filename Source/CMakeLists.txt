cmake_minimum_required(VERSION 3.0)
project(AftString VERSION 1.0.0)

add_library(AftString "")

set_target_properties(
    AftString
    PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY bin
)

target_sources(
    AftString
    PRIVATE
    aft_number_format.c
    aft_string.c
)
