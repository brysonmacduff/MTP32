# ============================
# lib/CMakeLists.txt
# ============================

set(COMPONENT "mtp32")

# Collect all source files in this directory
file(GLOB MTP32_SOURCES
    "*.cpp"
    "*.h"
)

# Create the library target
add_library(${COMPONENT} STATIC ${MTP32_SOURCES})

# Public include directory for consumers of the library
target_include_directories(${COMPONENT}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/mtp32>
)

# Use standard install directories
include(GNUInstallDirs)

# ----------------------------------------
# Install the library
# ----------------------------------------
install(
    TARGETS ${COMPONENT}
    EXPORT ${COMPONENT}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install only headers in this directory
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mtp32
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "test" EXCLUDE
        PATTERN "platforms" EXCLUDE
)

# ----------------------------------------
# Export CMake package configuration
# ----------------------------------------
install(
    EXPORT ${COMPONENT}Targets
    FILE ${COMPONENT}Targets.cmake
    NAMESPACE ${COMPONENT}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${COMPONENT}
)

# Generate a basic package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/mtp32Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${COMPONENT}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${COMPONENT}
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${COMPONENT}Config.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/${COMPONENT}
)

# ----------------------------------------
# Add tests
# ----------------------------------------
add_subdirectory(test)
