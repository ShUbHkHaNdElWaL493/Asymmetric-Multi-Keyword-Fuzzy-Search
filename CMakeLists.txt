cmake_minimum_required(VERSION 3.10)
project(FYP2026 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

option(BENCHMARK_MODE "Enable benchmarking" OFF)

if(BENCHMARK_MODE)

    add_executable(${PROJECT_NAME}_2 ${CMAKE_SOURCE_DIR}/test/main.cpp)
    add_executable(${PROJECT_NAME}_3 ${CMAKE_SOURCE_DIR}/test/main.cpp)

    target_include_directories(${PROJECT_NAME}_2 PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
    )
    target_include_directories(${PROJECT_NAME}_3 PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
    )

    target_compile_definitions(${PROJECT_NAME}_2 PRIVATE N=2)
    target_compile_definitions(${PROJECT_NAME}_3 PRIVATE N=3)
    
else()
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.cpp)
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
    )
endif()