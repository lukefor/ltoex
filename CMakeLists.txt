cmake_minimum_required (VERSION 3.10)
project("ltoex")

add_executable(
	ltoex
	"ltoex.cpp"
	"SLDC.cpp"
	"AES.cpp"
)

set_property(TARGET ltoex PROPERTY CXX_STANDARD 17)


# Dependencies

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(.)
target_link_libraries(ltoex ${OPENSSL_LIBRARIES})


# Optimisation flags

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif()

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funroll-loops")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipoSupported OUTPUT error)

if(ipoSupported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET ltoex PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()