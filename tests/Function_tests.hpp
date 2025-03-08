#ifndef ATUL_FUNCTION_TESTS
#define ATUL_FUNCTION_TESTS

#include <atul/Function.hpp>

#include <memory_resource>

namespace atul::tests {

    TEST(Function_tests, Construct_from_nullptr) {
        Function<void()> function{nullptr};
        EXPECT_THROW(function(), std::bad_function_call);
    }

    TEST(Function_tests, Empty_void_function_pointer) {
        Function<void()> function;

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(void));
    }

    int x0 = 0;

    void foo0() {
        x0 = 5;
    }

    TEST(Function_tests, Void_function_pointer) {
        x0 = 9;
        Function<void()> function{foo0};
        function();

        EXPECT_EQ(x0, 5);

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(void(*)()));
    }

    int x1 = 0;

    TEST(Function_tests, Void_lambda) {
        auto lambda = [] () {
            x1 = 10;
        };

        x1 = 5;
        Function<void()> function{lambda};
        function();

        EXPECT_EQ(x1, 10);

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(decltype(lambda)));
    }

    int x2 = 0;

    void foo2(int arg) {
        x2 = arg;
    }

    TEST(Function_tests, Function_with_argument) {
        Function<void(int)> function{foo2};
        function(10);

        EXPECT_EQ(x2, 10);
    }

    int x3 = 0;

    TEST(Function_tests, Lambda_with_argument) {
        auto lambda = [] (int arg) {
            x3 = arg;
        };

        Function<void(int)> function{lambda};
        function(154);

        EXPECT_EQ(x3, 154);
    }

    int x4 = 0;

    void foo4(int arg) {
        x4 = arg;
    }

    TEST(Function_tests, Copy_function_pointer) {
        Function<void(int)> function_original{foo4};
        Function<void(int)> function_copy{function_original};
        function_copy(67435);

        EXPECT_EQ(x4, 67435);
    }

    int x5 = 0;

    TEST(Function_tests, Copy_lambda) {
        auto lambda = [] (int arg) {
            x5 = arg;
        };

        Function<void(int)> function_original{lambda};
        Function<void(int)> function_copy{function_original};
        function_copy(545);

        EXPECT_EQ(x5, 545);
    }

}

#endif
