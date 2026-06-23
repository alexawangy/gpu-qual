cmake_minimum_required(VERSION 3.20...4.3)

project(
  gpuqual
  VERSION 0.1.0
  DESCRIPTION "GPU qualification utilities"
  LANGUAGES CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds are disabled. Configure with: cmake --preset dev"
  )
endif()

include(GNUInstallDirs)
include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GPUQUAL_ENABLE_WARNINGS "Enable compiler warnings for gpuqual targets" ON)
option(GPUQUAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(gpuqual STATIC
  src/verdict.cpp
  src/io_json.cpp
  src/observed.cpp
  src/spec.cpp
)

target_compile_features(gpuqual PUBLIC cxx_std_20)

target_link_libraries(gpuqual PUBLIC nlohmann_json::nlohmann_json)

target_include_directories(
  gpuqual
  PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

add_executable(gpu-qual src/main.cpp)

target_link_libraries(gpu-qual PRIVATE gpuqual)

if(GPUQUAL_ENABLE_WARNINGS)
  foreach(target gpuqual gpu-qual)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W4)
      if(GPUQUAL_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE /WX)
      endif()
    else()
      target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
      if(GPUQUAL_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE -Werror)
      endif()
    endif()
  endforeach()
endif()

install(
  TARGETS gpu-qual
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  include(CTest)
endif()

if(BUILD_TESTING AND EXISTS "${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
