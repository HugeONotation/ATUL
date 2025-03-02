#ifndef ATUL_FUNCTION_TESTS
#define ATUL_FUNCTION_TESTS

#include <atul/Function.hpp>

#include <memory_resource>

namespace atul::tests {

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

}

#endif
