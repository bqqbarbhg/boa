
#include <boa_test.h>
#include <boa_core.h>

BOA_TEST(boa_arraycount, "boa_arraycount() should count the number of elements in an array")
{
	int a[4], b[1];
	char c[3];
	boa_assert(boa_arraycount(a) == 4);
	boa_assert(boa_arraycount(b) == 1);
	boa_assert(boa_arraycount(c) == 3);
}

BOA_TEST(boa_arrayend, "boa_arrayend() should count the number of elements in an array")
{
	int a[4], b[1];
	char c[3];
	boa_assert(boa_arrayend(a) == a + 4);
	boa_assert(boa_arrayend(b) == b + 1);
	boa_assert(boa_arrayend(c) == c + 3);
}

BOA_TEST(check_ptr_valid, "Check valid pointer")
{
	int value = 0, *ptr = &value;
	boa_assert(boa_check_ptr(ptr) == ptr);
}

BOA_TEST(check_ptr_null, "Check NULL pointer")
{
	boa_expect_assert( boa_check_ptr(NULL) );
}

BOA_TEST(test_align_up, "boa_align_up() should work for powers of two")
{
	boa_assert(boa_align_up(0, 4) == 0);
	boa_assert(boa_align_up(1, 4) == 4);
	boa_assert(boa_align_up(2, 4) == 4);
	boa_assert(boa_align_up(3, 4) == 4);
	boa_assert(boa_align_up(4, 4) == 4);
	boa_assert(boa_align_up(5, 4) == 8);
	boa_assert(boa_align_up(16, 8) == 16);
	boa_assert(boa_align_up(17, 8) == 24);
}

BOA_TEST(test_align_fuzz, "Test boa_align_up() results for sanity")
{
	for (uint32_t align = 1; align <= 4096; align <<= 1) {
		boa_test_hint_u32(align);
		for (uint32_t i = 0; i < 10000; i++) {
			boa_test_hint_u32(i);
			uint32_t val = boa_align_up(i, align);
			boa_assert(val >= i && val < i + align);
			boa_assert(val % align == 0);
		}
	}
}

BOA_TEST(test_align_assert, "boa_align_up() should assert for non power of two alignments")
{
	boa_expect_assert( boa_align_up(4, 0) );
	boa_expect_assert( boa_align_up(4, 3) );
	boa_expect_assert( boa_align_up(4, 5) );
	boa_expect_assert( boa_align_up(4, 17) );
}

BOA_TEST(test_pow2_round, "Test boa_round_pow2_up() results for sanity")
{
	boa_assert(boa_round_pow2_up(0) == 0);
	boa_assert(boa_round_pow2_up(1) == 1);
	boa_assert(boa_round_pow2_up(2) == 2);
	boa_assert(boa_round_pow2_up(3) == 4);
	boa_assert(boa_round_pow2_up(4) == 4);
	boa_assert(boa_round_pow2_up(5) == 8);
	boa_assert(boa_round_pow2_up(100) == 128);
}

BOA_TEST(test_pow2_round_fuzz, "boa_round_pow2_up() should round to the next power of two")
{
	for (uint32_t i = 1; i < 100000; i++) {
		boa_test_hint_u32(i);
		uint32_t val = boa_round_pow2_up(i);
		boa_assert((val & val - 1) == 0);
		boa_assert(val >= i && val >> 1 < i);
	}
}

BOA_TEST(test_highest_bit, "boa_highest_bit() should find highest bit set")
{
	boa_assert(boa_highest_bit(1) == 0);
	boa_assert(boa_highest_bit(3) == 1);
	boa_assert(boa_highest_bit(6) == 2);
	boa_assert(boa_highest_bit(9) == 3);
	boa_assert(boa_highest_bit(20) == 4);
	boa_assert(boa_highest_bit(40) == 5);
	boa_assert(boa_highest_bit(100) == 6);
	boa_assert(boa_highest_bit(2000) == 10);
	boa_assert(boa_highest_bit(UINT32_MAX) == 31);
}

BOA_TEST(test_highest_bit_fuzz, "Test boa_highest_bit() results for sanity")
{
	for (uint32_t i = 1; i < 100000; i++) {
		boa_test_hint_u32(i);
		uint32_t val = boa_highest_bit(i);
		boa_assert((i & (1 << val)) != 0);
		boa_assert((i & (2 << val) - 1) == i);
	}
}

BOA_TEST(test_highest_bit_assert, "boa_highest_bit() should assert on zero")
{
	boa_expect_assert( boa_highest_bit(0) );
}
