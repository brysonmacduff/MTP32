cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
set(PICO_SDK_PATH "../pico-sdk")

# Import Pico SDK (must be before project())
include(pico_sdk_import.cmake)

project(mtp32
    VERSION 0.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

file(GLOB MTP32_SOURCES
    "../../*.cpp"
)

file(GLOB MTP32_HEADERS
    "../../*.h"
)

add_library(${PROJECT_NAME} STATIC ${MTP32_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

# ------------------------------------------------------------
# Installation layout (correct + platform-safe)
# ------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(MTP32_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/mtp32")
set(MTP32_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(MTP32_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/mtp32")

install(TARGETS ${PROJECT_NAME}
    EXPORT mtp32Targets
    ARCHIVE DESTINATION ${MTP32_INSTALL_LIBDIR}
)

# Install only the public headers
install(FILES ${MTP32_HEADERS}
    DESTINATION ${MTP32_INSTALL_INCLUDEDIR}
)

# ------------------------------------------------------------
# Generate CMake package config
# ------------------------------------------------------------
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/mtp32Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mtp32Config.cmake
    INSTALL_DESTINATION ${MTP32_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mtp32ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mtp32Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/mtp32ConfigVersion.cmake
    DESTINATION ${MTP32_INSTALL_CMAKEDIR}
)

install(EXPORT mtp32Targets
    FILE mtp32Targets.cmake
    NAMESPACE mtp32::
    DESTINATION ${MTP32_INSTALL_CMAKEDIR}
)
