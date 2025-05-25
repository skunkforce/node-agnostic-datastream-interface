cmake_minimum_required(VERSION 3.28) # Modern CMake version
project(nadi VERSION 1.0.0 LANGUAGES CXX)

# Define the INTERFACE library
add_library(nadi INTERFACE)
add_library(nadi::nadi ALIAS nadi)

# Specify include directories
target_include_directories(nadi
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set C++ standard (optional, customize as needed)
target_compile_features(nadi INTERFACE cxx_std_20)

# Installation rules
include(GNUInstallDirs)
install(TARGETS nadi
    EXPORT nadiTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install header files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets for find_package
install(EXPORT nadiTargets
    FILE nadiTargets.cmake
    NAMESPACE nadi::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nadi
)

# Generate and install package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/nadiConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nadiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/nadiConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/nadiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/nadiConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nadi
)

# Support FetchContent
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(NADI_IS_TOP_LEVEL TRUE)
else()
    set(NADI_IS_TOP_LEVEL FALSE)
endif()