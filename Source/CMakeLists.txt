cmake_minimum_required(VERSION 3.0)
project(AftString VERSION 1.0.0)

include(GNUInstallDirs)

add_library(AftString "")

set_target_properties(
    AftString
    PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

target_include_directories(
    AftString
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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

