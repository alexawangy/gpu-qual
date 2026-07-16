file(GLOB GPUQUAL_TEST_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp"
)

if(NOT GPUQUAL_TEST_SOURCES)
  message(STATUS "No gpuqual test sources found in ${CMAKE_CURRENT_SOURCE_DIR}")
  return()
endif()

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.15.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(gpuqual_tests)

gpuqual_configure_warnings(gpuqual_tests)

target_sources(gpuqual_tests
  PRIVATE
    ${GPUQUAL_TEST_SOURCES}
)

target_link_libraries(gpuqual_tests
  PRIVATE
    gpuqual
    Catch2::Catch2WithMain
)

target_include_directories(gpuqual_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(gpuqual_tests)
