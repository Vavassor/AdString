add_executable(TestBasic "")

target_sources(
    TestBasic
    PRIVATE
    Basic/main.c
    Basic/random.c
    ../../Source/ad_string.c
)

add_test(Basic TestBasic)

