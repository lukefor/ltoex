# CMakeList.txt : CMake project for ltoex, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ltoex")


# Add source to this project's executable.
add_executable (
ltoex
"ltoex.cpp"
"ltoex.h"
"SLDC.cpp"
"SLDC.h"
"AES.cpp"
"AES.h" 
)

# deps
#if (NOT MSVC)
#	set(SYSTEM_OPENSSL ON)
#endif()
#add_subdirectory(external/openssl-cmake)

find_package(OpenSSL REQUIRED) 
include_directories(${OPENSSL_INCLUDE_DIRS})
link_directories(${OPENSSL_LIBRARIES})

target_link_libraries(ltoex crypto ssl)
