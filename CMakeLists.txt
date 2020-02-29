# CMakeList.txt : CMake project for ltoex, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ltoex")


# Add source to this project's executable.
add_executable (
ltoex
"ltoex.cpp"
"SLDC.cpp"
"AES.cpp"
)

set_property(TARGET ltoex PROPERTY CXX_STANDARD 17)

# deps
#if (NOT MSVC)
#	set(SYSTEM_OPENSSL ON)
#endif()
#add_subdirectory(external/openssl-cmake)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(.)
target_link_libraries(ltoex ${OPENSSL_LIBRARIES})
