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

target_include_directories(
    AftString
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:include>
)

target_sources(
    AftString
    PRIVATE
    aft_number_format.c
    aft_string.c
)

install(
    TARGETS AftString
    EXPORT AftStringTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(
    EXPORT AftStringTargets
    DESTINATION lib/cmake/AftString
    FILE AftStringTargets.cmake
)
install(
    DIRECTORY Include/AftString
    DESTINATION include
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    AftStringConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
install(
    FILES
    AftStringConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AftStringConfigVersion.cmake
    DESTINATION lib/cmake/AftString
)

