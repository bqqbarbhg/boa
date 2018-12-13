
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

