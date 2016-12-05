//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++98, c++03

// <experimental/memory_resource>

// struct pool_options

#include <cassert>
#include <experimental/memory_resource>

int main() {
  using std::experimental::pmr::pool_options;
  {
    const pool_options p;
    assert(p.max_blocks_per_chunk == 0);
    assert(p.largest_required_pool_block == 0);
  }
}
