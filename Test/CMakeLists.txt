cmake_minimum_required(VERSION 3.0)
project(AdStringTest)

enable_testing()
if(NOT TARGET AdString)
    find_package(AdString CONFIG REQUIRED)
endif()

add_executable(TestBasic "")

target_link_libraries(
    TestBasic
    PRIVATE
    AdString
)

target_sources(
    TestBasic
    PRIVATE
    Basic/main.c
    Basic/random.c
)

add_test(Basic TestBasic)
