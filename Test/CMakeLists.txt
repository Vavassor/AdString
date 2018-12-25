cmake_minimum_required(VERSION 3.0)
project(AftStringTest)

enable_testing()
if(NOT TARGET AftString)
    find_package(AftString CONFIG REQUIRED)
endif()


add_executable(TestBasic "")

target_link_libraries(
    TestBasic
    PRIVATE
    AftString
)

target_sources(
    TestBasic
    PRIVATE
    Basic/main.c
    Utility/random.c
    Utility/test.c
)

add_test(
    NAME Basic
    COMMAND TestBasic
)


add_executable(TestBigInt "")

target_link_libraries(
    TestBigInt
    PRIVATE
    AftString
)

target_sources(
    TestBigInt
    PRIVATE
    "Big Int/main.c"
    Utility/random.c
    Utility/test.c
)

add_test(
    NAME BigInt
    COMMAND TestBigInt
)


add_executable(TestNumberFormat "")

target_link_libraries(
    TestNumberFormat
    PRIVATE
    AftString
)

target_sources(
    TestNumberFormat
    PRIVATE
    "Number Format/main.c"
    Utility/random.c
    Utility/test.c
)

add_test(
    NAME NumberFormat
    COMMAND TestNumberFormat
)

