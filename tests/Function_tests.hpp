#ifndef ATUL_FUNCTION_TESTS
#define ATUL_FUNCTION_TESTS

#include <atul/Function.hpp>

#include <memory_resource>

namespace atul::tests {

    //=====================================================
    // Function tests
    //=====================================================

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

    //=====================================================
    // AA_function Tests
    //=====================================================

    TEST(AA_function_tests, Construct_from_nullptr) {
        using allocator_type = std::allocator<int>;
        allocator_type allocator;

        AA_function<allocator_type, void(int)> function{nullptr};
    }

    TEST(AA_function_tests, Empty_void_function_pointer) {
        using allocator_type = std::allocator<int>;
        allocator_type allocator;

        AA_function<allocator_type, void(int)> function{nullptr};

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(void));
    }


    int x1_0 = 0;

    void foo1_0() {
        x1_0 = 5;
    }

    TEST(AA_function_tests, Void_function_pointer) {
        using allocator_type = std::allocator<int>;
        allocator_type allocator;

        x1_0 = 9;

        AA_function<allocator_type, void()> function{foo1_0};
        function();

        EXPECT_EQ(x1_0, 5);

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(void(*)()));
    }

    int x1_1 = 0;

    TEST(AA_function_tests, Void_lambda) {
        using allocator_type = std::allocator<int>;
        allocator_type allocator;

        auto lambda = [] () {
            x1_1 = 10;
        };

        x1_1 = 5;
        AA_function<allocator_type, void()> function{lambda};
        function();

        EXPECT_EQ(x1_1, 10);

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(decltype(lambda)));
    }

    //=====================================================
    // SBO_function Tests
    //=====================================================

    TEST(SBO_function_tests, Construct_from_nullptr) {
        SBO_function<24, void()> function{nullptr};
        EXPECT_THROW(function(), std::bad_function_call);
    }

    TEST(SBO_function_tests, Empty_void_function_pointer) {
        SBO_function<24, void()> function{};

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(void));
    }

    int x2_0 = 0;

    void foo2_0() {
        x2_0 = 5;
    }

    TEST(SBO_function_tests, Void_function_pointer) {
        x2_0 = 9;
        Function<void()> function{foo2_0};
        function();

        EXPECT_EQ(x2_0, 5);

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(void(*)()));
    }

    int x2_1 = 0;

    TEST(SBO_function_tests, Void_lambda) {
        auto lambda = [] () {
            x2_1 = 10;
        };

        x2_1 = 5;
        Function<void()> function{lambda};
        function();

        EXPECT_EQ(x2_1, 10);

        const auto& id = function.target_type();
        EXPECT_EQ(id, typeid(decltype(lambda)));
    }

    int x2_2 = 0;

    void foo2_2(int arg) {
        x2_2 = arg;
    }

    TEST(SBO_function_tests, Function_with_argument) {
        Function<void(int)> function{foo2_2};
        function(10);

        EXPECT_EQ(x2_2, 10);
    }

    int x2_3 = 0;

    TEST(SBO_function_tests, Lambda_with_argument) {
        auto lambda = [] (int arg) {
            x2_3 = arg;
        };

        Function<void(int)> function{lambda};
        function(154);

        EXPECT_EQ(x2_3, 154);
    }

    int x2_4 = 0;

    void foo2_4(int arg) {
        x2_4 = arg;
    }

    TEST(SBO_function_tests, Copy_function_pointer) {
        Function<void(int)> function_original{foo2_4};
        Function<void(int)> function_copy{function_original};
        function_copy(67435);

        EXPECT_EQ(x2_4, 67435);
    }

    int x2_5 = 0;

    TEST(SBO_function_tests, Copy_lambda) {
        auto lambda = [] (int arg) {
            x2_5 = arg;
        };

        Function<void(int)> function_original{lambda};
        Function<void(int)> function_copy{function_original};
        function_copy(545);

        EXPECT_EQ(x2_5, 545);
    }

}

#endif
