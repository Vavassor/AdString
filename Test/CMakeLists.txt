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


add_executable(TestJson "")

target_link_libraries(
    TestJson
    PRIVATE
    AftString
)

target_sources(
    TestJson
    PRIVATE
    Json/filesystem.c
    $<$<PLATFORM_ID:Linux>:Json/filesystem_posix.c>
	$<$<PLATFORM_ID:Windows>:Json/filesystem_windows.c>
    Json/json.c
    Json/json_deserialize.c
    Json/json_serialize.c
    Json/main.c
    Utility/random.c
    Utility/test.c
)

add_test(
    NAME Json
    COMMAND TestJson
)

configure_file(Json/handmade.json ${CMAKE_CURRENT_BINARY_DIR}/handmade.json COPYONLY)
configure_file(Json/omdb.json ${CMAKE_CURRENT_BINARY_DIR}/omdb.json COPYONLY)


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

