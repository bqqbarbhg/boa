
#include <boa_test.h>
#include <boa_core.h>
#include <boa_core_cpp.h>

#if BOA_TEST_IMPL

namespace {
	struct Point {
		Point(int x, int y) : x(x), y(y) { }
		int x, y;
		bool operator==(const Point &rhs) const { return x == rhs.x && y == rhs.y; }
		bool operator!=(const Point &rhs) const { return !(*this == rhs); }
	};

	struct IntGreater {
		bool operator()(int a, int b) { return a > b; }
	};
}

#endif

BOA_TEST(cpp_alloc, "C++ allocation functions")
{
	void *ptr = boa::alloc(16);
	boa_assert(ptr != NULL);
	ptr = boa_realloc(ptr, 64);
	boa_assert(ptr != NULL);
	boa::free(ptr);
}

BOA_TEST(cpp_buf_basic, "Simple boa::buf<int> test")
{
	boa::buf<int> buf;
	buf.push(10);
	buf.push(30);
	buf.push(40);
	buf.insert(1, 20);

	boa_assert(buf[0] == 10);
	boa_assert(buf[1] == 20);
	boa_assert(buf[2] == 30);
	boa_assert(buf[3] == 40);
	boa_assert(buf.count() == 4);

	buf.remove(1);
	buf.erase(2);

	boa_assert(buf[0] == 10);
	boa_assert(buf[1] == 40);

	boa_assert(buf.count() == 2);
}

BOA_TEST(cpp_buf_array, "boa::buf<int> from array")
{
	int arr[4];
	boa::buf<int> buf = boa::array_buf(arr);
	buf.push(10);
	buf.push(20);

	boa_assert(arr[0] == 10);
	boa_assert(arr[1] == 20);
}

BOA_TEST(cpp_buf_range_for, "boa::buf<int> range for")
{
	boa::buf<int> buf;
	buf.push(10);
	buf.push(20);
	buf.push(30);
	buf.push(40);

	int num = 10;
	for (int val : buf) {
		boa_assert(val == num);
		num += 10;
	}
	boa_assert(num == 50);
}

BOA_TEST(cpp_buf_points, "Buffer of more complex types boa::buf<Point>")
{
	boa::buf<Point> buf;
	buf.push({ 1, 2 });
	buf.push({ 3, 4 });

	boa_assert(buf[0] == Point(1, 2));
	boa_assert(buf[1] == Point(3, 4));
}

BOA_TEST(cpp_format_simple, "boa::format() simple overload")
{
	char *str = boa::format("Hello %s", "World");
	boa_assert(strcmp(str, "Hello World") == 0);
	boa::free(str);
}

BOA_TEST(cpp_format_buf, "boa::format() simple overload")
{
	boa::buf<char> buf;
	char *str = boa::format(buf, "Hello %s", "World");
	boa_assert(strcmp(str, "Hello World") == 0);
}

BOA_TEST(cpp_set_u32, "C++ U32 set")
{
	boa::u32_set<uint32_t> set;
	set.insert(10);
	set.insert(20);
	set.insert(30);

	boa_assert(set.find(10) && *set.find(10) == 10);
	boa_assert(set.find(20) && *set.find(20) == 20);
	boa_assert(set.find(30) && *set.find(30) == 30);
	boa_assert(set.find(40) == nullptr);

	const boa::u32_set<uint32_t> &ref = set;
	boa_assert(ref.find(10) && *ref.find(10) == 10);
	boa_assert(ref.find(20) && *ref.find(20) == 20);
	boa_assert(ref.find(30) && *ref.find(30) == 30);
	boa_assert(ref.find(40) == nullptr);
}

BOA_TEST(cpp_map_u32, "C++ U32 map")
{
	boa::u32_map<uint32_t, uint32_t> map;
	map.insert(1, 10);
	map.insert(2, 20);
	map.insert(3, 30);

	boa_assert(map.find(1) && map.find(1)->val == 10);
	boa_assert(map.find(2) && map.find(2)->val == 20);
	boa_assert(map.find(3) && map.find(3)->val == 30);
	boa_assert(map.find(4) == nullptr);

	const boa::u32_map<uint32_t, uint32_t> &ref = map;
	boa_assert(ref.find(1) && ref.find(1)->val == 10);
	boa_assert(ref.find(2) && ref.find(2)->val == 20);
	boa_assert(ref.find(3) && ref.find(3)->val == 30);
	boa_assert(ref.find(4) == nullptr);
}

BOA_TEST(cpp_pqueue, "C++ priority queue")
{
	boa::pqueue<int> pq;
	for (int i : { 5, 1, 3, 2, 4 })
		pq.enqueue(i);

	boa_assert(pq.count() == 5);
	boa_assert(pq.non_empty());

	for (int i : { 1, 2, 3, 4, 5 })
		boa_assert(pq.dequeue() == i);

	boa_assert(pq.count() == 0);
	boa_assert(pq.is_empty());
	boa_expect_assert( pq.dequeue() );
}

BOA_TEST(cpp_pqueue_functor, "C++ priority queue with custom functor")
{
	boa::pqueue<int, IntGreater> pq;

	for (int i : { 5, 1, 3, 2, 4 })
		pq.enqueue(i);

	boa_assert(pq.count() == 5);
	boa_assert(pq.non_empty());

	for (int i : { 5, 4, 3, 2, 1 })
		boa_assert(pq.dequeue() == i);

	boa_assert(pq.count() == 0);
	boa_assert(pq.is_empty());
	boa_expect_assert( pq.dequeue() );
}

BOA_TEST(cpp_pqueue_array, "C++ Array backed priority queue")
{
	int array[16];
	boa::pqueue<int> pq{ boa::array_view(array) };

	for (int i : { 5, 1, 3, 2, 4 })
		pq.enqueue(i);

	boa_assert(pq.count() == 5);
	boa_assert(pq.non_empty());

	for (int i : { 1, 2, 3, 4, 5 })
		boa_assert(pq.dequeue() == i);

	boa_assert(pq.count() == 0);
	boa_assert(pq.is_empty());
	boa_expect_assert( pq.dequeue() );
}

BOA_TEST(cpp_arena_simple, "Simple C++ arena test")
{
	boa::arena arena;

	int *a = arena.push<int>();
	int *b = arena.push<int>();

	boa_assert(a != NULL);
	boa_assert(b != NULL);
	boa_assert(a != b);
}
