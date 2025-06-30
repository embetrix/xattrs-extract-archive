cmake_minimum_required(VERSION 3.10)

# Project name
project(xattrs-extract-archive)

# Find pkgconfig
find_package(PkgConfig REQUIRED)

# Find libarchive
find_package(LibArchive REQUIRED)
if (LibArchive_FOUND)
    include_directories(${LIBARCHIVE_INCLUDE_DIR})
else()
    message(FATAL_ERROR "libarchive not found")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})


# Add the executable
add_executable(xattrs-extract-archive xattrs-extract-archive.c)
target_compile_options(xattrs-extract-archive PUBLIC -Wformat -Wformat-security -Wconversion -Wsign-conversion -pedantic -Werror)

# Link the libraries
target_link_libraries(xattrs-extract-archive ${LibArchive_LIBRARIES})

# Specify the install rules
install(TARGETS xattrs-extract-archive DESTINATION bin)

