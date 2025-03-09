#ifndef ATUL_FUNCTION_HPP
#define ATUL_FUNCTION_HPP

#include "Misc.hpp"

#include <aul/containers/Allocator_aware_base.hpp>

#include <memory_resource>

namespace atul {

    [[nodiscard]]
    constexpr std::size_t compute_sbo_size(
        std::size_t size_request,
        std::size_t alignment_request
    ) {
        // Round up to next multiple of alignment
        return (size_request / alignment_request + static_cast<bool>(size_request % alignment_request)) * alignment_request;
    }

    //=====================================================
    // Callable wrappers
    //=====================================================

    ///
    /// Virtual interface which presents a uniform interface for calling
    /// functions wrapped by instance of Callable_wrapper template classes.
    ///
    template<class Ret, class...Args>
    struct Callable_interface {

        virtual ~Callable_interface() = default;

        virtual Ret call(Args&&...args) = 0;

        virtual void move_constructor_delegate(std::byte* ptr) = 0;

        virtual void copy_constructor_delegate(std::byte* ptr) = 0;

        virtual std::size_t size_of() = 0;

        virtual const std::type_info& target_type() = 0;

        virtual void* target() = 0;
    };

    ///
    /// Templated wrapper around callable types which implement a common
    /// interface.
    ///
    /// @tparam Callable A callable type
    template<class Callable, class Ret, class...Args>
    struct Callable_wrapper final : Callable_interface<Ret, Args...> {

        explicit Callable_wrapper(const Callable& c):
            callable(c)
        {
            static_assert(std::is_copy_constructible_v<Callable>);
        }

        Callable_wrapper(const Callable_wrapper& other):
            callable(other.callable)
        {
            static_assert(std::is_copy_constructible_v<Callable>);
        }

        Callable_wrapper(Callable_wrapper&& other) :
            callable(std::forward<Callable>(other.callable))
        {
            static_assert(std::is_move_constructible_v<Callable>);
        }

        Ret call(Args&&...args) override {
            return callable(std::forward<Args>(args)...);
        }

        void move_constructor_delegate(std::byte* ptr) override {
            static_assert(std::is_move_constructible_v<Callable>);
            new (ptr) Callable_wrapper{std::move(*this)};
        }

        void copy_constructor_delegate(std::byte* ptr) override {
            static_assert(std::is_copy_constructible_v<Callable>);
            new (ptr) Callable_wrapper{*this};
        }

        std::size_t size_of() override {
            return sizeof(Callable_wrapper);
        }

        const std::type_info& target_type() override {
            return typeid(Callable);
        }

        void* target() override {
            return reinterpret_cast<void*>(&callable);
        }

        Callable callable;
    };

    template<class Ret, class...Args>
    struct Callable_wrapper<Ret(Args...), Ret, Args...> final : Callable_interface<Ret, Args...> {

        using Callable = Ret(Args...);

        explicit Callable_wrapper(const Callable&) {}

        Callable_wrapper(const Callable_wrapper&) {}

        Callable_wrapper(Callable_wrapper&&) noexcept {}

        Ret call(Args&&...args) override {
            return callable(std::forward<Args>(args)...);
        }

        void move_constructor_delegate(std::byte* ptr) override {
            new (ptr) Callable_wrapper{std::move(*this)};
        }

        void copy_constructor_delegate(std::byte* ptr) override {
            new (ptr) Callable_wrapper{*this};
        }

        std::size_t size_of() override {
            return sizeof(Callable_wrapper);
        }

        const std::type_info& target_type() override {
            return typeid(Callable);
        }

        void* target() override {
            return reinterpret_cast<void*>(&callable);
        }

        Callable callable;
    };

    //=====================================================
    // AA_SBO_function
    //=====================================================

    template<class A, std::size_t SB_size, class C>
    class AA_SBO_function;

    ///
    /// An allocator-aware alternative to std::function which also optionally
    /// uses a small buffer optimization.
    ///
    ///
    ///
    /// @tparam A STL compatible allocator type
    /// @tparam SB_size Target size of internal small buffer. Will be rounded up
    /// if it can be done without increasing size of struct.
    /// @tparam Ret Callable return type
    /// @tparam Args Callable argument types
    template<class A, std::size_t SB_size, class Ret, class...Args>
    class AA_SBO_function<A, SB_size, Ret (Args...)> : public aul::Allocator_aware_base<typename std::allocator_traits<A>::template rebind_alloc<std::byte>> {
        using a_base = aul::Allocator_aware_base<typename std::allocator_traits<A>::template rebind_alloc<std::byte>>;

    public:

        //=================================================
        // Constants
        //=================================================

        static constexpr std::size_t small_buffer_size =
            compute_sbo_size(
                SB_size,
                std::max(
                    alignof(void*),
                    alignof(a_base)
                )
            );

        //=================================================
        // Type aliases
        //=================================================

        using return_type = Ret;

        using allocator_type = typename std::allocator_traits<A>::template rebind_alloc<std::byte>;

        using pointer = typename std::allocator_traits<A>::pointer;

        //=================================================
        // -ctors
        //=================================================

        AA_SBO_function() = default;

        explicit AA_SBO_function(std::nullptr_t):
            callable() {}

        AA_SBO_function(const AA_SBO_function& other):
            a_base(other)
        {
            if (!other.callable) {
                return;
            }

            if (other.is_sbo_in_use()) {
                other.callable->copy_constructor_delegate(sbo_buffer);
                callable = reinterpret_cast<Callable_interface<Ret, Args...>*>(sbo_buffer);
            } else {
                acquire_callable(other.callable);
            }
        }

        AA_SBO_function(AA_SBO_function&& other) noexcept:
            a_base(std::move(other)),
            callable(std::exchange(other.callable, nullptr))
        {
            if (!other.callable) {
                return;
            }

            if (other.is_sbo_in_use()) {
                other.callable->move_constructor_delegate(sbo_buffer);
                callable = reinterpret_cast<Callable_interface<Ret, Args...>*>(sbo_buffer);
            } else {
                acquire_callable(other.callable);
            }
        }

        template<class Callable>
        AA_SBO_function(const allocator_type& a, Callable&& callable):
            a_base(a),
            callable()
        {
            acquire_callable<Callable>(std::forward<Callable>(callable));
        }

        template<class Callable>
        explicit AA_SBO_function(Callable&& callable):
            AA_SBO_function(allocator_type{}, std::forward<Callable>(callable)) {}

        template<class Callable>
        AA_SBO_function(const allocator_type& a, Callable* callable):
            a_base(a)
        {
            acquire_callable<Callable*>(callable);
        }

        template<class Callable>
        explicit AA_SBO_function(Callable* callable):
            AA_SBO_function(A{}, callable) {}

        ~AA_SBO_function() {
            release_callable();
        }

        //=================================================
        // Assignment operators
        //=================================================

        AA_SBO_function& operator=(const AA_SBO_function& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }

            release_callable();
            a_base::operator=(rhs);

            if (!rhs.callable) {
                return *this;
            }

            std::byte* target = nullptr;
            if (rhs.is_sbo_in_use()) {
                target = sbo_buffer;
            } else {
                auto allocator = a_base::get_allocator();
                target = allocator.allocate(rhs.callable->size_of());
            }

            rhs.callable->copy_constructor_delegate(target);
            callable = reinterpret_cast<Callable_interface<Ret, Args...>*>(target);

            return *this;
        }

        AA_SBO_function& operator=(AA_SBO_function&& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }

            release_callable();
            a_base::operator=(rhs);

            if (!rhs.callable) {
                return *this;
            }

            std::byte* target = nullptr;
            if (rhs.is_sbo_in_use()) {
                target = sbo_buffer;
            } else {
                auto allocator = a_base::get_allocator();
                target = allocator.allocate(rhs.callable->size_of());
            }

            rhs.callable->move_constructor_delegate(target);
            callable = reinterpret_cast<Callable_interface<Ret, Args...>*>(target);

            return *this;
        }

        template<class C>
        AA_SBO_function& operator=(C&& callable) {
            release_callable();
            this->callable = acquire_callable(callable);
            return *this;
        }

        template<class C>
        AA_SBO_function& operator=(std::reference_wrapper<C> callable) noexcept {
            release_callable();
            this->callable = acquire_callable(callable);
            return *this;
        }

        //=================================================
        // Accessors
        //=================================================

        [[nodiscard]]
        explicit operator bool() const {
            return callable == nullptr;
        }

        [[nodiscard]]
        const std::type_info& target_type() const noexcept {
            if (callable) {
                return callable->target_type();
            } else {
                return typeid(void);
            }
        }

        template<class T>
        [[nodiscard]]
        T* target() noexcept {
            if  (typeid(T) == target_type() && callable) {
                return reinterpret_cast<T*>(callable->target());
            } else {
                return nullptr;
            }
        }

        //=================================================
        // Misc.
        //=================================================

        void swap(AA_SBO_function& other) noexcept {
            //TODO: Consider using swap function from allocator-aware base
            if (is_sbo_is_use()) {
                AA_SBO_function tmp = std::move(*this);
                *this = std::move(other);
                other = std::move(tmp);
            } else {
                //TODO: Complete
            }
        }

        Ret operator()(Args&&...args) {
            if (!callable) {
                throw std::bad_function_call();
            }

            return callable->call(std::forward<Args>(args)...);
        }

    private:

        //=================================================
        // Instance members
        //=================================================

        Callable_interface<Ret, Args...>* callable = nullptr;

        std::byte sbo_buffer[small_buffer_size] {};

        //=================================================
        // Helper functions
        //=================================================

        [[nodiscard]]
        bool is_sbo_is_use() {
            return reinterpret_cast<std::byte*>(callable) == sbo_buffer;
        }

        template<class Callable>
        void acquire_callable(const Callable& c) {
            using callable_type = Callable_wrapper<Callable, Ret, Args...>;
            constexpr std::size_t required_size = sizeof(callable_type);
            constexpr std::size_t required_alignment = alignof(callable_type);

            const bool use_sb =
                (std::extent_v<decltype(sbo_buffer)> <= required_size) &&
                required_alignment <= alignof(decltype(sbo_buffer));

            if (use_sb) {
                auto* alloc = reinterpret_cast<Callable_wrapper<Callable, Ret, Args...>*>(sbo_buffer);
                new (alloc) Callable_wrapper<Callable, Ret, Args...>(c);
                callable = alloc;
            } else {
                auto allocator = a_base::get_allocator();
                std::byte* allocation = allocator.allocate(sizeof(Callable_wrapper<Callable, Ret, Args...>));
                if (allocation == nullptr) {
                    throw std::bad_alloc();
                }

                auto* alloc = reinterpret_cast<Callable_wrapper<Callable, Ret, Args...>*>(allocation);
                new (alloc) Callable_wrapper<Callable, Ret, Args...>(c);

                callable = alloc;
            }
        }

        template<class Callable>
        void acquire_callable(Callable&& c) {
            using callable_type = Callable_wrapper<Callable, Ret, Args...>;
            constexpr std::size_t required_size = sizeof(callable_type);
            constexpr std::size_t required_alignment = alignof(callable_type);

            const bool use_sb =
                (std::extent_v<decltype(sbo_buffer)> <= required_size) &&
                required_alignment <= alignof(decltype(sbo_buffer));

            if (use_sb) {
                auto* alloc = reinterpret_cast<Callable_wrapper<Callable, Ret, Args...>*>(sbo_buffer);
                new (alloc) Callable_wrapper<Callable, Ret, Args...>(std::forward<Callable>(c));
                callable = alloc;
            } else {
                auto allocator = a_base::get_allocator();
                std::byte* allocation = allocator.allocate(sizeof(Callable_wrapper<Callable, Ret, Args...>));
                if (allocation == nullptr) {
                    throw std::bad_alloc();
                }

                auto* alloc = reinterpret_cast<Callable_wrapper<Callable, Ret, Args...>*>(allocation);
                new (alloc) Callable_wrapper<Callable, Ret, Args...>(std::forward<Callable>(c));

                callable = alloc;
            }
        }

        void release_callable() {
            if (is_sbo_is_use()) {
                callable->~Callable_interface();
            } else {
                if (callable) {
                    callable->~Callable_interface();
                    auto allocator = a_base::get_allocator();
                    allocator.deallocate(reinterpret_cast<std::byte*>(callable), sizeof(*callable));
                }
            }
        }

    };

    template<class A, class Ret, class...Args>
    class AA_SBO_function<A, 0, Ret(Args...)> : public aul::Allocator_aware_base<typename std::allocator_traits<A>::template rebind_alloc<std::byte>> {
        using a_base = aul::Allocator_aware_base<typename std::allocator_traits<A>::template rebind_alloc<std::byte>>;

    public:

        //=================================================
        // Type aliases
        //=================================================

        using return_type = Ret;

        using allocator_type = typename std::allocator_traits<A>::template rebind_alloc<std::byte>;

        using pointer = typename std::allocator_traits<A>::pointer;

        //=================================================
        // -ctors
        //=================================================

        AA_SBO_function() = default;

        explicit AA_SBO_function(std::nullptr_t):
            callable() {}

        AA_SBO_function(const AA_SBO_function& other):
            a_base(other)
        {
            if (!other.callable) {
                return;
            }
            acquire_callable(other.callable);
        }

        AA_SBO_function(AA_SBO_function&& other) noexcept:
            a_base(std::move(other)),
            callable(std::exchange(other.callable, nullptr))
        {
            if (!other.callable) {
                return;
            }
            acquire_callable(other.callable);
        }

        template<class Callable>
        AA_SBO_function(const A& a, Callable&& callable):
            a_base(a),
            callable()
        {
            acquire_callable<Callable>(std::forward<Callable>(callable));
        }

        template<class Callable>
        explicit AA_SBO_function(Callable&& callable):
            AA_SBO_function(A{}, std::forward<Callable>(callable)) {}

        template<class Callable>
        AA_SBO_function(const allocator_type& a, Callable* callable):
            a_base(a)
        {
            acquire_callable<Callable*>(callable);
        }

        template<class Callable>
        explicit AA_SBO_function(Callable* callable):
            AA_SBO_function(allocator_type{}, callable) {}

        ~AA_SBO_function() {
            release_callable();
        }

        //=================================================
        // Assignment operators
        //=================================================

        AA_SBO_function& operator=(const AA_SBO_function& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }

            release_callable();
            a_base::operator=(rhs);

            if (!rhs.callable) {
                return *this;
            }

            auto allocator = a_base::get_allocator();
            std::byte* target = allocator.allocate(rhs.callable->size_of());

            rhs.callable->copy_constructor_delegate(target);
            callable = reinterpret_cast<Callable_interface<Ret, Args...>*>(target);

            return *this;
        }

        AA_SBO_function& operator=(AA_SBO_function&& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }

            release_callable();
            a_base::operator=(rhs);

            if (!rhs.callable) {
                return *this;
            }

            auto allocator = a_base::get_allocator();
            std::byte* target = allocator.allocate(rhs.callable->size_of());

            rhs.callable->move_constructor_delegate(target);
            callable = reinterpret_cast<Callable_interface<Ret, Args...>*>(target);

            return *this;
        }

        template<class C>
        AA_SBO_function& operator=(C&& callable) {
            release_callable();
            this->callable = acquire_callable(callable);
            return *this;
        }

        template<class C>
        AA_SBO_function& operator=(std::reference_wrapper<C> callable) noexcept {
            release_callable();
            this->callable = acquire_callable(callable);
            return *this;
        }

        //=================================================
        // Accessors
        //=================================================

        [[nodiscard]]
        explicit operator bool() const {
            return callable == nullptr;
        }

        [[nodiscard]]
        const std::type_info& target_type() const noexcept {
            if (callable) {
                return callable->target_type();
            } else {
                return typeid(void);
            }
        }

        template<class T>
        [[nodiscard]]
        T* target() noexcept {
            if  (typeid(T) == target_type() && callable) {
                return reinterpret_cast<T*>(callable->target());
            } else {
                return nullptr;
            }
        }

        //=================================================
        // Misc.
        //=================================================

        void swap(AA_SBO_function& other) noexcept {
            a_base::swap(other);
            std::swap(callable, other.callable);
        }

        Ret operator()(Args&&...args) {
            if (!callable) {
                throw std::bad_function_call();
            }

            return callable->call(std::forward<Args>(args)...);
        }

    private:

        //=================================================
        // Instance members
        //=================================================

        Callable_interface<Ret, Args...>* callable = nullptr;

        //=================================================
        // Helper functions
        //=================================================

        template<class Callable>
        void acquire_callable(const Callable& c) {
            auto allocator = a_base::get_allocator();
            std::byte* allocation = allocator.allocate(sizeof(Callable_wrapper<Callable, Ret, Args...>));
            if (allocation == nullptr) {
                throw std::bad_alloc();
            }

            auto* callable_ptr = reinterpret_cast<Callable_wrapper<Callable, Ret, Args...>*>(allocation);
            new (callable_ptr) Callable_wrapper<Callable, Ret, Args...>{c};
            callable = callable_ptr;
        }

        template<class Callable>
        void acquire_callable(Callable&& c) {
            auto allocator = a_base::get_allocator();
            std::byte* allocation = allocator.allocate(sizeof(Callable_wrapper<Callable, Ret, Args...>));
            if (allocation == nullptr) {
                throw std::bad_alloc();
            }

            auto* callable_ptr = reinterpret_cast<Callable_wrapper<Callable, Ret, Args...>*>(allocation);
            new (callable_ptr) Callable_wrapper<Callable, Ret, Args...>{std::forward<Callable>(c)};
            callable = callable_ptr;
        }

        void release_callable() {
            if (callable) {
                callable->~Callable_interface();
                auto allocator = a_base::get_allocator();
                allocator.deallocate(reinterpret_cast<std::byte*>(callable), sizeof(*callable));
            }
        }

    };

    //=====================================================
    // Convenience type aliases
    //=====================================================

    template<class C>
    using Function = AA_SBO_function<std::allocator<std::byte>, 0, C>;

    template<class A, class C>
    using AA_function = AA_SBO_function<A, 0, C>;

    template<std::size_t SB_size, class C>
    using SBO_function = AA_SBO_function<std::allocator<std::byte>, SB_size, C>;

}

#endif //ATUL_FUNCTION_HPP
