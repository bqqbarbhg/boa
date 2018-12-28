
#include <boa_benchmark.h>

#define BENCH_MAP_INLINE 0
#include "core/bench_map.h"
#undef BENCH_MAP_INLINE

#define BENCH_MAP_INLINE 1
#include "core/bench_map.h"
#undef BENCH_MAP_INLINE

#include "core/bench_std_map.h"

