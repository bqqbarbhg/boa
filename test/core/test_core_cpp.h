
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
