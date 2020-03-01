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

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})
if (MSVC)
	include_directories(.)
endif()
target_link_libraries(ltoex ${OPENSSL_LIBRARIES})
