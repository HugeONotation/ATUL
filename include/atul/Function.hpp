#ifndef ATUL_FUNCTION_HPP
#define ATUL_FUNCTION_HPP

#include "Misc.hpp"

#include <aul/containers/Allocator_aware_base.hpp>

#include <tuple>
#include <memory_resource>

namespace atul {

    template<std::size_t SB_size>
    class SBO_function_base {
    protected:

        alignas(atul::bit_floor(SB_size)) std::byte sbo_buffer[SB_size];

    };

    template<>
    class SBO_function_base<0> {
    protected:

        alignas(1) std::byte sbo_buffer[0];

    };

    template<class Alloc, std::size_t SB_size, class C>
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
    class AA_SBO_function<A, SB_size, Ret (Args...)> :
        public SBO_function_base<SB_size>,
        public aul::Allocator_aware_base<A> {

        using sbo_base = SBO_function_base<SB_size>;
        using a_base = aul::Allocator_aware_base<A>;

        class Callable_interface {
        public:

            Callable_interface(Callable_interface&&);

            virtual ~Callable_interface() = default;

            virtual Ret call(Args&&...args) = 0;

            virtual void move_constructor_delegate(const std::byte* ptr) = 0;

            virtual void copy_constructor_delegate(const std::byte* ptr) = 0;

            virtual const std::type_info& target_type() = 0;

            virtual void* target() = 0;
        };

        template<class Callable>
        class Callable_wrapper : public Callable_interface {
        public:

            explicit Callable_wrapper(const Callable& c):
                callable(c) {}

            Ret call(Args&&...args) final {
                return callable(std::forward<Args>(args)...);
            }

            void move_constructor_delegate(const std::byte* ptr) final {
                new (ptr) Callable_wrapper{std::move(*this)};
            }

            void copy_constructor_delegate(const std::byte* ptr) final {
                new (ptr) Callable_wrapper{*this};
            }

            const std::type_info& target_type() final {
                return typeid(Callable);
            }

            void* target() final {
                return reinterpret_cast<void*>(&callable);
            }

            Callable callable;
        };

    public:

        //=================================================
        // Type aliases
        //=================================================

        using return_type = Ret;

        //=================================================
        // -ctors
        //=================================================

        AA_SBO_function()= default;

        explicit AA_SBO_function(std::nullptr_t):

            callable() {}

        AA_SBO_function(const AA_SBO_function& other):
            a_base(other)
        {
            if (!other.callable) {
                return;
            }

            if (other.is_sbo_in_use()) {
                other.callable->copy_constructor_delegate(sbo_base::sbo_buffer);
                callable = reinterpret_cast<Callable_interface*>(sbo_base::sbo_buffer);
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
                other.callable->move_constructor_delegate(sbo_base::sbo_buffer);
                callable = reinterpret_cast<Callable_interface*>(sbo_base::sbo_buffer);
            } else {
                acquire_callable(other.callable);
            }
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
        AA_SBO_function(const A& a, Callable* callable):
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

            //TODO: Revise

            release_callable();
            a_base::operator=(rhs);
            callable = acquire_callable(rhs.callable);

            return *this;
        }

        AA_SBO_function& operator=(AA_SBO_function&& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }
            //TODO: Revise

            release_callable();
            a_base::operator=(std::move(rhs));
            callable = std::exchange(rhs.callable, nullptr);

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

        //=================================================
        // Target access
        //=================================================

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
            //TODO: Properly use swap function from allocator-aware base
            if (is_sbo_is_use()) {
                AA_SBO_function tmp = std::move(*this);
                *this = std::move(other);
                other = std::move(tmp);
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

        Callable_interface* callable = nullptr;

        //=================================================
        // Helpers
        //=================================================

        [[nodiscard]]
        bool is_sbo_is_use() {
            return reinterpret_cast<std::byte*>(callable) == sbo_base::sbo_buffer;
        }

        template<class Callable>
        void acquire_callable(const Callable& c) {
            using callable_type = Callable_wrapper<Callable>;
            constexpr std::size_t required_size = sizeof(callable_type);
            constexpr std::size_t required_alignment = alignof(callable_type);

            const bool use_sb =
                (std::extent_v<decltype(sbo_base::sbo_buffer)> <= required_size) &&
                required_alignment <= alignof(sbo_base::sbo_buffer);

            if (use_sb) {
                auto* alloc = reinterpret_cast<Callable_wrapper<Callable>*>(sbo_base::sbo_buffer);
                new (alloc) Callable_wrapper<Callable>(c);
                callable = alloc;
            } else {
                auto allocator = a_base::get_allocator();
                std::byte* allocation = allocator.allocate(sizeof(Callable_wrapper<Callable>));
                if (allocation == nullptr) {
                    throw std::bad_alloc();
                }

                auto* alloc = reinterpret_cast<Callable_wrapper<Callable>*>(allocation);
                new (alloc) Callable_wrapper<Callable>(c);

                callable = alloc;
            }
        }

        template<class Callable>
        void acquire_callable(Callable&& c) {
            using callable_type = Callable_wrapper<Callable>;
            constexpr std::size_t required_size = sizeof(callable_type);
            constexpr std::size_t required_alignment = alignof(callable_type);

            const bool use_sb =
                (std::extent_v<decltype(sbo_base::sbo_buffer)> <= required_size) &&
                required_alignment <= alignof(sbo_base::sbo_buffer);

            if (use_sb) {
                auto* alloc = reinterpret_cast<Callable_wrapper<Callable>*>(sbo_base::sbo_buffer);
                new (alloc) Callable_wrapper<Callable>(std::forward<Callable>(c));
                callable = alloc;
            } else {
                auto allocator = a_base::get_allocator();
                std::byte* allocation = allocator.allocate(sizeof(Callable_wrapper<Callable>));
                if (allocation == nullptr) {
                    throw std::bad_alloc();
                }

                auto* alloc = reinterpret_cast<Callable_wrapper<Callable>*>(allocation);
                new (alloc) Callable_wrapper<Callable>(std::forward<Callable>(c));

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

    template<class C>
    using Function = AA_SBO_function<std::allocator<std::byte>, 0, C>;

    template<class A, class C>
    using AA_Function = AA_SBO_function<A, 0, C>;

    template<std::size_t SB_size, class C>
    using SBO_function = AA_SBO_function<std::allocator<std::byte>, SB_size, C>;

}

#endif //ATUL_FUNCTION_HPP
