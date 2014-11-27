﻿#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <cassert>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <new>
#include <utility>
#include <array>


#define TRACE() std::cout << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << std::endl


// Helper traits
template<typename T> using Invoke = typename T::type;
template <typename Condition, typename T = void> using EnableIf = Invoke<std::enable_if<Condition::value, T>>;
template <typename Condition, typename T = void> using DisableIf = Invoke<std::enable_if<!Condition::value, T>>;
template <typename T> using Decay = Invoke<std::remove_const<Invoke<std::remove_reference<T>>>>;
template <typename T, typename U> struct IsRelated : std::is_same<Decay<T>, Decay<U>> {};


template<typename Signature>
struct Function;


template<typename R, typename ...Args>
struct Function<R(Args...)>
{
    template<typename F, typename = DisableIf<IsRelated<F, Function>>>
    Function(F f) : mImpl(std::make_shared<Impl<F>>(std::move(f))) {}

    template<typename Alloc, typename F, typename = DisableIf<IsRelated<F, Function>>>
    Function(std::allocator_arg_t, Alloc alloc, F f)
    {
        typedef typename Alloc::template rebind<Impl<F>>::other Other;
        mImpl = std::allocate_shared<Impl<F>>(Other(alloc), std::move(f));
    }

    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    Function(const Function&) noexcept = default;
    Function& operator=(const Function&) noexcept = default;

    R operator()(Args ...args) const
    { return (*mImpl)(args...); }

private:
    struct ImplBase
    {
        virtual ~ImplBase() {}
        virtual R operator()(Args ...args) = 0;
    };

    template<typename F>
    struct Impl : ImplBase
    {
        template<typename FArg>
        Impl(FArg&& f) : f(std::forward<FArg>(f)) {  }

        ~Impl() {}

        virtual R operator()(Args ...args)
        { return f(args...); }

        F f;
    };

    std::shared_ptr<ImplBase> mImpl;
};

template <std::size_t N>
class StackStorage
{
public:
    StackStorage() = default;

    StackStorage(StackStorage const&) = delete;

    StackStorage& operator=(StackStorage const&) = delete;

    char* allocate(std::size_t n)
    {
        assert(pointer_in_buffer(ptr_) &&
               "StackAllocator has outlived stack_store");

        n = align(n);

        if (buf_ + N >= ptr_ + n)
        {
            auto r(ptr_);

            ptr_ += n;

            return r;
        }
        else
        {
            return static_cast<char*>(::operator new(n));
        }
    }

    void deallocate(char* const p, std::size_t n) noexcept
    {
        assert(pointer_in_buffer(ptr_)&&
        "StackAllocator has outlived stack_store");

        if (pointer_in_buffer(p))
        {
            n = align(n);

            if (p + n == ptr_)
            {
                ptr_ = p;
            }
            // else do nothing
        }
        else
        {
            ::operator delete(p);
        }
    }

    void reset() noexcept { ptr_ = buf_; }

    static constexpr ::std::size_t size() noexcept { return N; }

    ::std::size_t used() const {
        return ::std::size_t(ptr_ - buf_);
    }

private:
    static constexpr ::std::size_t align(::std::size_t const n) noexcept
    {
        return (n + (alignment - 1)) & -alignment;
    }

    bool pointer_in_buffer(char* const p) noexcept
    {
        return (buf_ <= p) && (p <= buf_ + N);
    }

private:
    static constexpr auto const alignment = alignof(std::max_align_t);

    char* ptr_ {buf_};

    alignas(std::max_align_t) char buf_[N];
};


template <class T, std::size_t N>
class StackAllocator
{
public:
    using store_type = StackStorage<N>;
    using size_type = ::std::size_t;
    using difference_type = ::std::ptrdiff_t;
    using pointer = T* ;
    using const_pointer = T const* ;
    using reference = T& ;
    using const_reference = T const& ;
    using value_type = T;
    template <class U> struct rebind { using other = StackAllocator<U, N>; };

    StackAllocator() = default;
    StackAllocator(StackStorage<N>& s) noexcept : store_(&s) { }
    template <class U>
    StackAllocator(StackAllocator<U, N> const& other) noexcept : store_(other.store_) { }

    StackAllocator& operator=(StackAllocator const&) = delete;

    T* allocate(::std::size_t const n)
    {
        std::cout << "allocate " << n << " " << typeid(T).name() <<  " size=" << sizeof(T) << std::endl;
        return static_cast<T*>(static_cast<void*>(
                                   store_->allocate(n * sizeof(T))));
    }

    void deallocate(T* const p, ::std::size_t const n) noexcept
    {
        std::cout << "Deallocate " << n << " " << typeid(T).name() <<  " size=" << sizeof(T) << std::endl;
        store_->deallocate(static_cast<char*>(static_cast<void*>(p)),
        n * sizeof(T));
    }

    template <class U, class ...A>
    void construct(U* const p, A&& ...args)
    {
        new(p) U(::std::forward<A>(args)...);
    }

    template <class U>
    void destroy(U* const p)
    {
        p->~U();
    }

    template <class U, std::size_t M>
    inline bool operator==(StackAllocator<U, M> const& rhs) const noexcept
    {
        return store_ == rhs.store_;
    }

    template <class U, std::size_t M>
    inline bool operator!=(StackAllocator<U, M> const& rhs) const noexcept
    {
        return !(*this == rhs);
    }

private:
    template <class U, std::size_t M> friend class StackAllocator;
    store_type* store_ {};
};


template<typename T = char, std::size_t N>
StackAllocator<T, N> MakeStackAllocator(StackStorage<N>& store)
{
    return StackAllocator<T, N>(store);
}


int main()
{
    StackStorage<1024> storage;
    auto stack_allocator = MakeStackAllocator(storage);

    Function<int(int)> inc(std::allocator_arg, stack_allocator, [=](int n) { return n + 1; });
    auto inc2 = inc;
    inc = inc2;

    std::cout << inc(3) << std::endl;
}