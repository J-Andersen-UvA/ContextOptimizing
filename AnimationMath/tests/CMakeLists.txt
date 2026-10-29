add_executable(context_optimizing_animation_math_tests
    geometry_tests.cpp
    test_main.cpp
)

target_link_libraries(context_optimizing_animation_math_tests
    PRIVATE ContextOptimizing::AnimationMath)

target_compile_features(context_optimizing_animation_math_tests PRIVATE cxx_std_20)

add_test(
    NAME context_optimizing_animation_math_tests
    COMMAND context_optimizing_animation_math_tests)

