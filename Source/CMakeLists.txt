cmake_minimum_required(VERSION 3.0)
project(AftString VERSION 1.0.0)

if(CONAN_EXPORTED)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

include(GNUInstallDirs)


option(USE_CUSTOM_ALLOCATOR "Use custom memory allocation with static linking." OFF)
if(USE_CUSTOM_ALLOCATOR)
    set(AFT_STRING_USE_CUSTOM_ALLOCATOR 1)
else()
    set(AFT_STRING_USE_CUSTOM_ALLOCATOR 0)
endif()


configure_file(
    "${PROJECT_SOURCE_DIR}/aft_string_config.h.in"
    "${PROJECT_BINARY_DIR}/aft_string_config.h"
)

# solely for finding aft_string_config.h
include_directories("${PROJECT_BINARY_DIR}")


add_library(AftString "")

set_target_properties(
    AftString
    PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

target_compile_options(
	AftString
	PUBLIC
	$<$<C_COMPILER_ID:MSVC>:/utf-8>
)

target_include_directories(
    AftString
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
    AftString
    PRIVATE
    $<$<C_COMPILER_ID:GNU>:m>
)

target_sources(
    AftString
    PRIVATE
    aft_number_format.c
    aft_string.c
    big_int.c
    floating_point_format.c
)


install(
    TARGETS AftString
    EXPORT AftStringTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    EXPORT AftStringTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AftString
    FILE AftStringTargets.cmake
)
install(
    DIRECTORY Include/AftString
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AftString
)

