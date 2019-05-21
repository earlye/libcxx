//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: gcc && c++98
// UNSUPPORTED: gcc && c++03

// test forward

#include <utility>
#include <type_traits>
#include <cassert>

#include "test_macros.h"

struct A
{
};

A source() TEST_NOEXCEPT {return A();}
const A csource() TEST_NOEXCEPT {return A();}


TEST_CONSTEXPR bool test_constexpr_forward() {
#if TEST_STD_VER > 11
    int x = 42;
    const int cx = 101;
    return std::forward<int&>(x)        == 42
        && std::forward<int>(x)         == 42
        && std::forward<const int&>(x)  == 42
        && std::forward<const int>(x)   == 42
        && std::forward<int&&>(x)       == 42
        && std::forward<const int&&>(x) == 42
        && std::forward<const int&>(cx) == 101
        && std::forward<const int>(cx)  == 101;
#else
    return true;
#endif
}

int main(int, char**)
{
    A a;
    const A ca = A();

    ((void)a); // Prevent unused warning
    ((void)ca); // Prevent unused warning

    ASSERT_SAME_TYPE(decltype(std::forward<A&>(a)), A&);
    ASSERT_SAME_TYPE(decltype(std::forward<A>(a)), A&&);
    ASSERT_SAME_TYPE(decltype(std::forward<A>(source())), A&&);
    ASSERT_NOEXCEPT(std::forward<A&>(a))
    ASSERT_NOEXCEPT(std::forward<A>(a));
    ASSERT_NOEXCEPT(std::forward<A>(source()));

    ASSERT_SAME_TYPE(decltype(std::forward<const A&>(a)), const A&);
    ASSERT_SAME_TYPE(decltype(std::forward<const A>(a)), const A&&);
    ASSERT_SAME_TYPE(decltype(std::forward<const A>(source())), const A&&);
    ASSERT_NOEXCEPT(std::forward<const A&>(a));
    ASSERT_NOEXCEPT(std::forward<const A>(a));
    ASSERT_NOEXCEPT(std::forward<const A>(source()));

    ASSERT_SAME_TYPE(decltype(std::forward<const A&>(ca)), const A&);
    ASSERT_SAME_TYPE(decltype(std::forward<const A>(ca)), const A&&);
    ASSERT_SAME_TYPE(decltype(std::forward<const A>(csource())), const A&&);
    ASSERT_NOEXCEPT(std::forward<const A&>(ca));
    ASSERT_NOEXCEPT(std::forward<const A>(ca));
    ASSERT_NOEXCEPT(std::forward<const A>(csource()));

#if TEST_STD_VER > 11
    {
    constexpr int i2 = std::forward<int>(42);
    static_assert(std::forward<int>(42) == 42, "");
    static_assert(std::forward<const int&>(i2) == 42, "");
    static_assert(test_constexpr_forward(), "");
    }
#endif
#if TEST_STD_VER == 11 && defined(_LIBCPP_VERSION)
    // Test that std::forward is constexpr in C++11. This is an extension
    // provided by both libc++ and libstdc++.
    {
    constexpr int i2 = std::forward<int>(42);
    static_assert(std::forward<int>(42) == 42, "" );
    static_assert(std::forward<const int&>(i2) == 42, "");
    }
#endif

  return 0;
}
