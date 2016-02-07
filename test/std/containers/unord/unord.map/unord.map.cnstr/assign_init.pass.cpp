//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <unordered_map>

// template <class Key, class T, class Hash = hash<Key>, class Pred = equal_to<Key>,
//           class Alloc = allocator<pair<const Key, T>>>
// class unordered_map

// unordered_map& operator=(initializer_list<value_type> il);

#include <unordered_map>
#include <string>
#include <cassert>
#include <cfloat>

#include "../../../test_compare.h"
#include "../../../test_hash.h"
#include "min_allocator.h"
#include "test_macros.h"
#include "test_types.h"


int main()
{
#ifndef _LIBCPP_HAS_NO_GENERALIZED_INITIALIZERS
    {
        typedef std::allocator<std::pair<const int, std::string> > A;
        typedef std::unordered_map<int, std::string,
                                   test_hash<std::hash<int> >,
                                   test_compare<std::equal_to<int> >,
                                   A
                                   > C;
        typedef std::pair<int, std::string> P;
        C c =   {
                    P(4, "four"),
                    P(1, "four"),
                    P(2, "four"),
                };
        c =     {
                    P(1, "one"),
                    P(2, "two"),
                    P(3, "three"),
                    P(4, "four"),
                    P(1, "four"),
                    P(2, "four"),
                };
        assert(c.bucket_count() >= 5);
        assert(c.size() == 4);
        assert(c.at(1) == "one");
        assert(c.at(2) == "two");
        assert(c.at(3) == "three");
        assert(c.at(4) == "four");
        assert(std::distance(c.begin(), c.end()) == c.size());
        assert(std::distance(c.cbegin(), c.cend()) == c.size());
        assert(fabs(c.load_factor() - (float)c.size()/c.bucket_count()) < FLT_EPSILON);
        assert(c.max_load_factor() == 1);
    }
#endif // _LIBCPP_HAS_NO_GENERALIZED_INITIALIZERS
#if TEST_STD_VER >= 11
    {
        typedef min_allocator<std::pair<const int, std::string> > A;
        typedef std::unordered_map<int, std::string,
                                   test_hash<std::hash<int> >,
                                   test_compare<std::equal_to<int> >,
                                   A
                                   > C;
        typedef std::pair<int, std::string> P;
        C c =   {
                    P(4, "four"),
                    P(1, "four"),
                    P(2, "four"),
                };
        c =     {
                    P(1, "one"),
                    P(2, "two"),
                    P(3, "three"),
                    P(4, "four"),
                    P(1, "four"),
                    P(2, "four"),
                };
        assert(c.bucket_count() >= 5);
        assert(c.size() == 4);
        assert(c.at(1) == "one");
        assert(c.at(2) == "two");
        assert(c.at(3) == "three");
        assert(c.at(4) == "four");
        assert(std::distance(c.begin(), c.end()) == c.size());
        assert(std::distance(c.cbegin(), c.cend()) == c.size());
        assert(fabs(c.load_factor() - (float)c.size()/c.bucket_count()) < FLT_EPSILON);
        assert(c.max_load_factor() == 1);
    }
    {
        typedef CountingType<0> Key;
        typedef CountingType<1> Value;
        typedef std::pair<const Key, Value> ValueTp;
        typedef std::allocator<std::pair<const int, std::string> > A;
        typedef std::unordered_map<Key, Value> C;

        ValueTp p1;
        p1.first.allow_copies = 2;
        p1.second.allow_copies = 2;
        C c = { p1 };

        ValueTp p;
        // one copy into the initializer list, one copy into the container.
        p.first.allow_copies = 2;
        p.second.allow_copies = 2;
        c = { p };
    }
#endif

}
