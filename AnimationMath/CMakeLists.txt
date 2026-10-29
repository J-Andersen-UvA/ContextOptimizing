add_library(context_optimizing_animation_math
    src/geometry.cpp
    src/pose.cpp
)

add_library(ContextOptimizing::AnimationMath
    ALIAS context_optimizing_animation_math)

target_compile_features(context_optimizing_animation_math PUBLIC cxx_std_20)

target_include_directories(context_optimizing_animation_math
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(context_optimizing_animation_math PRIVATE /W4 /permissive-)
else()
    target_compile_options(context_optimizing_animation_math PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CONTEXT_OPTIMIZING_BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

