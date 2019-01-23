
#include <boa_test.h>
#include <boa_os.h>

#if BOA_TEST_IMPL

void simple_thread_entry(void *user)
{
	*(int*)user = 10;
}

#endif

BOA_TEST(thread_simple, "Create a thread to set a value")
{
	int value = 0;
	boa_thread_opts opts = { 0 };
	opts.entry = &simple_thread_entry;
	opts.user = &value;
	boa_thread *thread = boa_create_thread(&opts);
	boa_assert(thread != NULL);
	boa_join_thread(thread);
	boa_assert(value == 10);
}

BOA_TEST(thread_simple_named, "Create a thread with a debug name")
{
	int value = 0;
	boa_thread_opts opts = { 0 };
	opts.entry = &simple_thread_entry;
	opts.user = &value;
	opts.debug_name = "Simple thread";
	boa_thread *thread = boa_create_thread(&opts);
	boa_assert(thread != NULL);
	boa_join_thread(thread);
	boa_assert(value == 10);
}

