cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

project(ExtractCrash)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

### Externals

## zlib

set(ZLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib")
set(ZLIB_LIBRARY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/lib/zlib.lib")
if(MSVC)
  set(ZLIB_LIBRARY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/lib/zlibd.lib")
else()
    set(ZLIB_LIBRARY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/lib/zlib.lib")
endif()
set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib" "${CMAKE_CURRENT_BINARY_DIR}/external/zlib")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/zlib/" EXCLUDE_FROM_ALL)

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} zlib)
target_sources(${PROJECT_NAME} PRIVATE
  src/main.cpp
)
