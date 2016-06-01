//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


// <string_view>

// void swap(basic_string_view& _other) noexcept

#include <experimental/string_view>
#include <cassert>

template<typename CharT>
void test ( const CharT *s, size_t len ) {
    typedef std::experimental::basic_string_view<CharT> SV;
    {
    SV sv1(s);
    SV sv2;

    assert ( sv1.size() == len );
    assert ( sv1.data() == s );
    assert ( sv2.size() == 0 );

    sv1.swap ( sv2 );
    assert ( sv1.size() == 0 );
    assert ( sv2.size() == len );
    assert ( sv2.data() == s );
    }

}

#if _LIBCPP_STD_VER > 11
constexpr size_t test_ce ( size_t n, size_t k ) {
    typedef std::experimental::basic_string_view<char> SV;
    SV sv1{ "ABCDEFGHIJKL", n };
    SV sv2 { sv1.data(), k };
    sv1.swap ( sv2 );
    return sv1.size();
}
#endif


int main () {
    test ( "ABCDE", 5 );
    test ( "a", 1 );
    test ( "", 0 );

    test ( L"ABCDE", 5 );
    test ( L"a", 1 );
    test ( L"", 0 );

#if __cplusplus >= 201103L
    test ( u"ABCDE", 5 );
    test ( u"a", 1 );
    test ( u"", 0 );

    test ( U"ABCDE", 5 );
    test ( U"a", 1 );
    test ( U"", 0 );
#endif

#if _LIBCPP_STD_VER > 11
    {
    static_assert ( test_ce (2, 3) == 3, "" );
    static_assert ( test_ce (5, 3) == 3, "" );
    static_assert ( test_ce (0, 1) == 1, "" );
    }
#endif
}
