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

target_sources(gpuqual_tests
  PRIVATE
    ${GPUQUAL_TEST_SOURCES}
    "${PROJECT_SOURCE_DIR}/src/verdict.cpp"
)

target_compile_features(gpuqual_tests PRIVATE cxx_std_20)

target_link_libraries(gpuqual_tests
  PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

if(TARGET gpuqual)
  target_include_directories(gpuqual_tests
    PRIVATE
      "$<TARGET_PROPERTY:gpuqual,INCLUDE_DIRECTORIES>"
  )
endif()

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(gpuqual_tests)
