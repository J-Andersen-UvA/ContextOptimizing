cmake_minimum_required(VERSION 3.20)

project(ContextOptimizing LANGUAGES CXX)

option(CONTEXT_OPTIMIZING_BUILD_TESTS "Build project unit tests" ON)

if(CONTEXT_OPTIMIZING_BUILD_TESTS)
    include(CTest)
endif()

add_subdirectory(Optimization)
add_subdirectory(AnimationMath)

