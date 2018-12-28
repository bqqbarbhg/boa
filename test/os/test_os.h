
#include <boa_test.h>
#include <boa_os.h>

BOA_TEST(timestamp_monotonic, "boa_cycle_timestamp() should increment monotonically")
{
	uint64_t prev = boa_cycle_timestamp();
	for (uint32_t i = 0; i < 10000; i++) {
		uint64_t cur = boa_cycle_timestamp();
		boa_assert(cur >= prev);
		boa_yield_cpu();
		prev = cur;
	}
}

