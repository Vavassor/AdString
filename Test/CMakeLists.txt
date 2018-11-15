add_executable(TestBasic "")

target_sources(
    TestBasic
    PRIVATE
    ../Source/ad_string.c
    Basic/main.c
)

add_test(Basic TestBasic)
