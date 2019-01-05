
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL
void test_hash(uint32_t hash)
{
	uint32_t canonical = boa__map_hash_canonicalize(hash);
	boa_test_hint_u32(hash);
	boa_test_hint_u32(canonical);
	// Bits 0:LOWBITS are be non-zero
	boa_assert((canonical & BOA__MAP_LOWMASK) != 0);
	// Bits LOWBITS:32 are same as the original hash
	boa_assert((canonical & BOA__MAP_HIGHMASK) == (hash & BOA__MAP_HIGHMASK));
	// Should be unchanged if 0:LOWBITS are non-zero originally
	if (hash & BOA__MAP_LOWMASK) {
		boa_assert(canonical == hash);
	}
}
#endif

BOA_TEST(hash_canonicalize, "Hash canonicalization should satisfy postconditions")
{
	for (uint32_t i = 0; i < 1 << 16; i++) {
		test_hash(i);
	}
	for (uint32_t i = 0; i < 31; i++) {
		test_hash(1 << i);
		test_hash((1 << i) - 1);
	}
	test_hash(~0u);
}

