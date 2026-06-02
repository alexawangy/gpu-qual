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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GPUQUAL_ENABLE_WARNINGS "Enable compiler warnings for gpuqual targets" ON)
option(GPUQUAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_executable(gpuqual)

set_target_properties(gpuqual PROPERTIES OUTPUT_NAME gpu-qual)

target_sources(gpuqual PRIVATE src/main.cpp)

target_compile_features(gpuqual PRIVATE cxx_std_20)

target_include_directories(
  gpuqual
  PRIVATE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

if(GPUQUAL_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(gpuqual PRIVATE /W4)
    if(GPUQUAL_WARNINGS_AS_ERRORS)
      target_compile_options(gpuqual PRIVATE /WX)
    endif()
  else()
    target_compile_options(gpuqual PRIVATE -Wall -Wextra -Wpedantic)
    if(GPUQUAL_WARNINGS_AS_ERRORS)
      target_compile_options(gpuqual PRIVATE -Werror)
    endif()
  endif()
endif()

install(
  TARGETS gpuqual
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  include(CTest)
endif()

if(BUILD_TESTING AND EXISTS "${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
