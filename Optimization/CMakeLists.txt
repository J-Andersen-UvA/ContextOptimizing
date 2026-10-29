cmake_minimum_required(VERSION 3.20)

project(ContextOptimizingOptimization LANGUAGES CXX)

option(CONTEXT_RETARGETING_OPTIMIZATION_BUILD_TESTS
       "Build the Optimization unit tests"
       ON)

add_library(context_optimizing_optimization
    src/adam.cpp
    src/minimize.cpp
    src/numerical_gradient.cpp
)

add_library(ContextOptimizing::Optimization
    ALIAS context_optimizing_optimization)

target_compile_features(context_optimizing_optimization PUBLIC cxx_std_20)

target_include_directories(context_optimizing_optimization
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(context_optimizing_optimization PRIVATE /W4 /permissive-)
else()
    target_compile_options(context_optimizing_optimization PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CONTEXT_RETARGETING_OPTIMIZATION_BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()
