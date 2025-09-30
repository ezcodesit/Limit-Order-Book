cmake_minimum_required(VERSION 3.10)
project(Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# core engine library
add_library(engine_lib STATIC
    src/Engine.cpp
)
target_include_directories(engine_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# main engine executable
add_executable(engine
    src/main.cpp
)
target_link_libraries(engine PRIVATE engine_lib)

enable_testing()
add_subdirectory(tests)
