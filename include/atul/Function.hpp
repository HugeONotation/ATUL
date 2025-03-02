#ifndef ATUL_FUNCTION_HPP
#define ATUL_FUNCTION_HPP

#include <tuple>
#include <memory_resource>

namespace atul {

    template<class Alloc, class C>
    class AA_function;

    ///
    /// An allocator-aware alternative to std::function which uses an instance
    /// of std::pmr::allocator<byte> internally.
    ///
    /// @tparam Alloc STL compatible allocator type
    /// @tparam Ret Callable return type
    /// @tparam Args Callable argument types
    template<class Alloc, class Ret, class...Args>
    class AA_function<Alloc, Ret (Args...)> {
    public:

        class Callable_interface {
        public:

            virtual ~Callable_interface() = default;

            virtual Ret call(Args&&...args) = 0;

            virtual const std::type_info& target_type() = 0;

            virtual void* target() = 0;
        };

        template<class Callable>
        class Callable_wrapper : public Callable_interface {
        public:

            explicit Callable_wrapper(const Callable& c):
                callable(c) {}

            Ret call(Args&&...args) final {
                return callable(args...);
            }

            const std::type_info& target_type() final {
                return typeid(Callable);
            }

            void* target() final {
                return reinterpret_cast<void*>(&callable);
            }

            Callable callable;
        };

        //=================================================
        // Type aliases
        //=================================================

        using return_type = Ret;

        //=================================================
        // -ctors
        //=================================================

        AA_function()= default;

        explicit AA_function(std::nullptr_t):
            allocator(),
            callable() {}

        AA_function(const AA_function& other):
            allocator(other.allocator),
            callable(acquire_callable(other.callable)) {}

        AA_function(AA_function&& other) noexcept:
            allocator(std::exchange(other.allocator, {})),
            callable(std::exchange(other.callable, nullptr)) {}

        template<class Callable>
        AA_function(const Alloc& a, Callable&& callable):
            allocator(a),
            callable(acquire_callable<Callable>(callable)) {}

        template<class Callable>
        AA_function(const Alloc& a, Callable* callable):
            allocator(a),
            callable(acquire_callable<Callable*>(callable)) {}

        template<class Callable>
        explicit AA_function(Callable&& callable):
            AA_function(Alloc{}, std::forward<Callable>(callable)) {}

        ~AA_function() {
            release_callable();
        }

        //=================================================
        // Assignment operators
        //=================================================

        AA_function& operator=(const AA_function& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }

            release_callable();

            allocator = rhs.allocator;
            callable = acquire_callable(rhs.callable);

            return *this;
        }

        AA_function& operator=(AA_function&& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }

            release_callable();

            allocator = std::exchange(rhs.allocator, {});
            callable = std::exchange(rhs.callable, nullptr);

            return *this;
        }

        template<class C>
        AA_function& operator=(C&& callable) {
            release_callable();
            this->callable = acquire_callable(callable);
            return *this;
        }

        template<class C>
        AA_function& operator=(std::reference_wrapper<C> callable) noexcept {
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

        void swap(AA_function& other) noexcept {
            if (allocator != other.allocator) {
                throw std::runtime_error("Cannot swap callable allocations if allocators do not compare equal.");
            }
            std::swap(callable, other.callable);
        }

        Ret operator()(Args...args) {
            if (!callable) {
                throw std::bad_function_call();
            }

            return callable->call(args...);
        }

    private:

        //=================================================
        // Instance members
        //=================================================

        std::pmr::polymorphic_allocator<std::byte> allocator{};

        Callable_interface* callable = nullptr;

        //=================================================
        // Helpers
        //=================================================

        template<class Callable>
        Callable_interface* acquire_callable(const Callable& callable) {
            std::byte* allocation = allocator.allocate(sizeof(Callable_wrapper<Callable>));
            if (allocation == nullptr) {
                throw std::bad_alloc();
            }

            auto* alloc = reinterpret_cast<Callable_wrapper<Callable>*>(allocation);
            new (alloc) Callable_wrapper<Callable>(callable);
            //allocator.construct(alloc, callable);

            return alloc;
        }

        void release_callable() {
            if (callable) {
                callable->~Callable_interface();
                allocator.deallocate(reinterpret_cast<std::byte*>(callable), sizeof(*callable));
            }
        }

    };

    template<class C>
    using Function = AA_function<std::pmr::polymorphic_allocator<std::byte>, C>;

}

#endif //ATUL_FUNCTION_HPP
