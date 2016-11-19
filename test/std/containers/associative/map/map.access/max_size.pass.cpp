//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <map>

// class map

// size_type max_size() const;

#include <cassert>
#include <limits>
#include <map>
#include <type_traits>

#include "test_allocator.h"
#include "test_macros.h"

template <class Alloc> size_t alloc_max_size(Alloc const &a) {
  typedef std::allocator_traits<Alloc> AT;
  return AT::max_size(a);
}

int main()
{
  typedef std::pair<const int, int> KV;
  {
    typedef limited_allocator<KV, 10> A;
    typedef std::map<int, int, std::less<int>, A> C;
    C c;
    assert(c.max_size() <= 10);
    LIBCPP_ASSERT(c.max_size() == 10);
  }
  {
    typedef limited_allocator<KV, (size_t)-1> A;
    typedef std::map<int, int, std::less<int>, A> C;
    const C::difference_type max_dist =
        std::numeric_limits<C::difference_type>::max();
    C c;
    assert(c.max_size() <= max_dist);
    LIBCPP_ASSERT(c.max_size() == max_dist);
    }
    {
      typedef std::map<char, int> C;
      const C::difference_type max_dist =
          std::numeric_limits<C::difference_type>::max();
      C c;
      assert(c.max_size() <= max_dist);
      assert(c.max_size() <= alloc_max_size(c.get_allocator()));
    }
}
