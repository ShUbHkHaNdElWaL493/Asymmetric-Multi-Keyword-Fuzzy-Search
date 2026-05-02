cmake_minimum_required(VERSION 3.10)
project(FYP2026 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(${PROJECT_NAME}_MAIN ${CMAKE_SOURCE_DIR}/src/main.cpp)

# set(L 32 CACHE STRING "LSH functions")
# set(M 256 CACHE STRING "Security parameter")
# set(W 0.01 CACHE STRING "Window size")

target_include_directories(${PROJECT_NAME}_MAIN PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
)

# target_compile_definitions(${PROJECT_NAME}_MAIN PRIVATE 
#     L_VAL=${L}
#     M_VAL=${M}
#     W_VAL=${W}
# )