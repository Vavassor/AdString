add_executable(TestBasic "")

target_sources(
    TestBasic
    PRIVATE
    Basic/main.c
)

target_link_libraries(TestBasic AdString)

add_test(Basic TestBasic)

