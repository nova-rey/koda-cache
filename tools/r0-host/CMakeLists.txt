cmake_minimum_required(VERSION 3.20)
project(koda-r0-host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
add_library(koda_r0 STATIC
    src/device_identity.cpp
    src/evidence.cpp
    src/volume_ownership.cpp
    src/libkrun_worker.cpp
    src/recovery.cpp)
target_include_directories(koda_r0 PUBLIC include)
if (WIN32)
  target_compile_definitions(koda_r0 PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(koda_r0 PRIVATE advapi32 setupapi)
endif()

add_executable(koda-r0 src/main.cpp)
target_link_libraries(koda-r0 PRIVATE koda_r0)

if (BUILD_TESTING)
  add_executable(koda-r0-tests tests/r0_tests.cpp)
  target_link_libraries(koda-r0-tests PRIVATE koda_r0)
  add_test(NAME koda-r0-tests COMMAND koda-r0-tests)
endif()
