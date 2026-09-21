cmake_minimum_required(VERSION 3.16)
project(TTaskSchedulerProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(lib)
include_directories(app)

# --- основное приложение ---
add_executable(TaskSchedulerApp
    bin/main.cpp
)

target_link_libraries(TaskSchedulerApp PRIVATE
    Threads::Threads
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


# тесты
enable_testing()

add_executable(ttask_tests
    tests/ts_test.cpp
    tests/cl_test.cpp
    tests/mt_test.cpp
)

target_link_libraries(ttask_tests PRIVATE
    Threads::Threads
    GTest::gtest_main
)

include(GoogleTest)    
gtest_discover_tests(ttask_tests) 

#target_compile_options(TaskSchedulerApp PRIVATE -Wall -Wextra)