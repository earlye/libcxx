#include "benchmark/benchmark_api.h"

#include "Generators.hpp"
#include "TestArray.hpp"

#include <algorithm>

using benchmark::DoNotOptimize;

void BM_find_end(benchmark::State& st) {
    StrideGenerator<int> g(0, st.range_y());
    StrideGenerator<int> g2(0, st.range_y());
    auto test_arr = generate_test_array<int>(st.range_x(), g);
    auto test_arr1 = generate_test_array<int>(st.range_y(), g2);
    auto test_arr2 = make_test_array<int>(1, -1);
    while (st.KeepRunning()) {
        DoNotOptimize(std::find_end(test_arr.begin(), test_arr.end(), test_arr1.begin(), test_arr1.end()));
        DoNotOptimize(std::find_end(test_arr.begin(), test_arr.end(), test_arr2.begin(), test_arr2.end()));
        DoNotOptimize(test_arr);
        DoNotOptimize(test_arr1);
        DoNotOptimize(test_arr2);
    }
}
BENCHMARK(BM_find_end)->ArgPair(1<<14, 1<<4);
BENCHMARK(BM_find_end)->ArgPair(1<<14, 1<<14);


BENCHMARK_MAIN()
