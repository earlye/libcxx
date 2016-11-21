// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++98, c++03, c++11, c++14

// <variant>

//  template <class T, class... Types>
//  constexpr add_pointer_t<T> get_if(variant<Types...>* v) noexcept;
// template <class T, class... Types>
//  constexpr add_pointer_t<const T> get_if(const variant<Types...>* v) noexcept;

#include <variant>
#include <cassert>
#include "test_macros.h"
#include "variant_test_helpers.hpp"

void test_const_get_if() {
    {
        using V = std::variant<int>;
        constexpr const V* v = nullptr;
        static_assert(std::get_if<int>(v) == nullptr, "");
    }
    {
        using V = std::variant<int, long>;
        constexpr V v(42);
        ASSERT_NOEXCEPT(std::get_if<int>(&v));
        ASSERT_SAME_TYPE(decltype(std::get_if<int>(&v)), int const*);
        static_assert(*std::get_if<int>(&v) == 42, "");
        static_assert(std::get_if<long>(&v) == nullptr, "");
    }
    {
        using V = std::variant<int, long>;
        constexpr V v(42l);
        ASSERT_SAME_TYPE(decltype(std::get_if<long>(&v)), long const*);
        static_assert(*std::get_if<long>(&v) == 42, "");
        static_assert(std::get_if<int>(&v) == nullptr, "");
    }
// FIXME: Remove these once reference support is reinstated
#if !defined(TEST_VARIANT_HAS_NO_REFERENCES)
    {
        using V = std::variant<int&>;
        int x = 42;
        const V v(x);
        ASSERT_SAME_TYPE(decltype(std::get_if<int&>(std::addressof(v))), int*);
        assert(std::get_if<int&>(std::addressof(v)) == &x);
    }
    {
        using V = std::variant<int&&>;
        int x = 42;
        const V v(std::move(x));
        ASSERT_SAME_TYPE(decltype(std::get_if<int&&>(std::addressof(v))), int*);
        assert(std::get_if<int&&>(std::addressof(v)) == &x);
    }
    {
        using V = std::variant<const int&&>;
        int x = 42;
        const V v(std::move(x));
        ASSERT_SAME_TYPE(decltype(std::get_if<const int&&>(std::addressof(v))), const int*);
        assert(std::get_if<const int&&>(std::addressof(v)) == &x);
    }
#endif
}

void test_get_if()
{
    {
        using V = std::variant<int>;
        V* v = nullptr;
        assert(std::get_if<int>(v) == nullptr);
    }
    {
        using V = std::variant<int, long>;
        V v(42);
        ASSERT_NOEXCEPT(std::get_if<int>(std::addressof(v)));
        ASSERT_SAME_TYPE(decltype(std::get_if<int>(std::addressof(v))), int*);
        assert(*std::get_if<int>(std::addressof(v)) == 42);
        assert(std::get_if<long>(std::addressof(v)) == nullptr);
    }
    {
        using V = std::variant<int, long>;
        V v(42l);
        ASSERT_SAME_TYPE(decltype(std::get_if<long>(std::addressof(v))), long*);
        assert(*std::get_if<long>(std::addressof(v)) == 42);
        assert(std::get_if<int>(std::addressof(v)) == nullptr);
    }
// FIXME: Remove these once reference support is reinstated
#if !defined(TEST_VARIANT_HAS_NO_REFERENCES)
    {
        using V = std::variant<int&>;
        int x = 42;
        V v(x);
        ASSERT_SAME_TYPE(decltype(std::get_if<int&>(std::addressof(v))), int*);
        assert(std::get_if<int&>(std::addressof(v)) == &x);
    }
    {
        using V = std::variant<const int&>;
        int x = 42;
        V v(x);
        ASSERT_SAME_TYPE(decltype(std::get_if<const int&>(std::addressof(v))), const int*);
        assert(std::get_if<const int&>(std::addressof(v)) == &x);
    }
    {
        using V = std::variant<int&&>;
        int x = 42;
        V v(std::move(x));
        ASSERT_SAME_TYPE(decltype(std::get_if<int&&>(std::addressof(v))), int*);
        assert(std::get_if<int&&>(std::addressof(v)) == &x);
    }
    {
        using V = std::variant<const int&&>;
        int x = 42;
        V v(std::move(x));
        ASSERT_SAME_TYPE(decltype(std::get_if<const int&&>(std::addressof(v))), const int*);
        assert(std::get_if<const int&&>(std::addressof(v)) == &x);
    }
#endif
}

int main()
{
    test_const_get_if();
    test_get_if();
}
