#include "benchmark/minimal_benchmark.h"
#include "ContainerBenchmarks.hpp"

#include <set>

BENCHMARK_TEMPLATE(container_range_constructor, std::set<int>, ConstantGenerator<int, 42>)->Arg(8<<12);
BENCHMARK_TEMPLATE(container_range_constructor, std::set<int>, StrideGenerator<int>)->Arg(8<<12);

BENCHMARK_TEMPLATE(container_range_constructor, std::set<int>, ConstantStrideGenerator<int, 0, 512>)->Arg(64);
BENCHMARK_TEMPLATE(container_range_constructor, std::set<int>, ConstantStrideGenerator<int, 0, 512>)->Arg(512);
BENCHMARK_TEMPLATE(container_range_constructor, std::set<int>, ConstantStrideGenerator<int, 0, 512>)->Arg(8<<12);

BENCHMARK_TEMPLATE(container_copy_constructor, std::set<int>, ConstantGenerator<int, 42>)->Arg(8<<12);
BENCHMARK_TEMPLATE(container_copy_constructor, std::set<int>, StrideGenerator<int>)->Arg(8<<12);
BENCHMARK_TEMPLATE(container_copy_constructor, std::set<int>, ConstantStrideGenerator<int, 0, 512>)->Arg(8<<12);

BENCHMARK_TEMPLATE(container_copy_assignment, std::set<int>, ConstantGenerator<int, 42>)->Arg(8<<12);
BENCHMARK_TEMPLATE(container_copy_assignment, std::set<int>, StrideGenerator<int>)->Arg(8<<12);
BENCHMARK_TEMPLATE(container_copy_assignment, std::set<int>, ConstantStrideGenerator<int, 0, 512>)->Arg(8<<12);

BENCHMARK_TEMPLATE(container_equal, std::set<int>, StrideGenerator<int>, StrideGenerator<int>)->ArgPair(8<<12, 8<<12);
BENCHMARK_TEMPLATE(container_equal, std::set<int>, StrideGenerator<int>, StrideGenerator<int>)->ArgPair(8<<12, (8<<12) + 1);

BENCHMARK_TEMPLATE(container_not_equal, std::set<int>, StrideGenerator<int>, StrideGenerator<int>)->ArgPair(8<<12, 8<<12);
BENCHMARK_TEMPLATE(container_not_equal, std::set<int>, StrideGenerator<int>, StrideGenerator<int>)->ArgPair(8<<12, (8<<12) + 1);

BENCHMARK_TEMPLATE(container_less, std::set<int>, StrideGenerator<int>, StrideGenerator<int>)->ArgPair(8<<12, 8<<12);
BENCHMARK_TEMPLATE(container_less, std::set<int>, StrideGenerator<int>, StrideGenerator<int>)->ArgPair(8<<12, (8<<12) + 1);

BENCHMARK_TEMPLATE(container_iterate, std::set<int>, StrideGenerator<int>)->Arg(8<<12);


BENCHMARK_MAIN()
