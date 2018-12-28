
#include <boa_test.h>

#if BOA_TEST_IMPL
#include <../example/astar_cpp.h>
#endif

BOA_TEST(astar_line, "Simple line path in an empty map")
{
	astar::map map(4, 4);

	boa::buf<astar::point> path;
	bool result = astar::pathfind(path, map, { 0, 1 }, { 3, 1 });

	boa_assert(result);
	boa_assert(path.count() == 4);
	boa_assert(path[0] == (astar::point{ 0, 1 }));
	boa_assert(path[1] == (astar::point{ 1, 1 }));
	boa_assert(path[2] == (astar::point{ 2, 1 }));
	boa_assert(path[3] == (astar::point{ 3, 1 }));
}

BOA_TEST(astar_obstacle, "Path around an simple obstacle")
{
	astar::map map(4, 4);    // ######
	map.set(2, 0, INFINITY); // #  # #
	map.set(1, 1, INFINITY); // #A##B#
	map.set(2, 1, INFINITY); // #.##.#
	map.set(1, 2, INFINITY); // #....#
	map.set(2, 2, INFINITY); // ######

	boa::buf<astar::point> path;
	bool result = astar::pathfind(path, map, { 0, 1 }, { 3, 1 });

	boa_assert(result);
	boa_assert(path.count() == 8);
	boa_assert(path[0] == (astar::point{ 0, 1 }));
	boa_assert(path[1] == (astar::point{ 0, 2 }));
	boa_assert(path[2] == (astar::point{ 0, 3 }));
	boa_assert(path[3] == (astar::point{ 1, 3 }));
	boa_assert(path[4] == (astar::point{ 2, 3 }));
	boa_assert(path[5] == (astar::point{ 3, 3 }));
	boa_assert(path[6] == (astar::point{ 3, 2 }));
	boa_assert(path[7] == (astar::point{ 3, 1 }));
}

BOA_TEST(astar_impossible, "Astar should fail on impossible map")
{
	astar::map map(4, 4);    // ######
	map.set(2, 0, INFINITY); // #  # #
	map.set(2, 1, INFINITY); // #A #B#
	map.set(2, 2, INFINITY); // # ## #
	map.set(1, 2, INFINITY); // # #  #
	map.set(1, 3, INFINITY); // ######

	boa::buf<astar::point> path;
	bool result = astar::pathfind(path, map, { 0, 1 }, { 3, 1 });
	boa_assert(!result);
	boa_assert(path.is_empty());
}

BOA_TEST(astar_big, "Astar should handle larger maps")
{
	astar::map map(1024, 1024);

	boa::buf<astar::point> path;
	bool result = astar::pathfind(path, map, { 0, 0 }, { 1023, 1023 });
	boa_assert(result);
}
