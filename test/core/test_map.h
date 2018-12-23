
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL
uint32_t g_hash_factor;
int g_do_reserve;

uint32_t int_hash(int i){ return i % 10000 * g_hash_factor; }
int int_cmp(const void *a, const void *b){ return *(int*)a == *(int*)b; }

void insert_int(boa_map *map, int k, int v)
{
	uint32_t elem = boa_map_insert_inline(map, &k, int_hash(k), &int_cmp);
	*boa_key(int, map, elem) = k;
	*boa_val(int, map, elem) = v;
}

int find_int(boa_map *map, int k)
{
	uint32_t elem = boa_map_find_inline(map, &k, int_hash(k), &int_cmp);
	return elem != ~0u ? *boa_val(int, map, elem) : -1;
}

void erase_int(boa_map *map, int k)
{
	uint32_t elem = boa_map_find_inline(map, &k, int_hash(k), &int_cmp);
	if (elem != ~0u) {
		boa_map_erase(map, elem);
	}
}

#else

extern uint32_t g_hash_factor;
extern int g_do_reserve;

static uint32_t hash_factors[] = {
	13, // < Provides reasonable hashing
	1,  // < Degrades to linear
	0,  // < Degrades to linked list
};

static uint32_t hash_factors_no_zero[] = {
	13, // < Provides reasonable hashing
	1,  // < Degrades to linear
};

static int reserve_values[] = {
	1, 0,
};

#endif

BOA_TEST_BEGIN_PERMUTATION(g_hash_factor, hash_factors)
BOA_TEST_BEGIN_PERMUTATION(g_do_reserve, reserve_values)

BOA_TEST(map_simple, "Simple manual map test")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve)
		boa_map_reserve(map, 32);

	insert_int(map, 1, 10);
	insert_int(map, 2, 20);
	insert_int(map, 3, 30);
	insert_int(map, 4, 40);

	boa_assert(map->count == 4);

	boa_assert(find_int(map, 1) == 10);
	boa_assert(find_int(map, 2) == 20);
	boa_assert(find_int(map, 3) == 30);
	boa_assert(find_int(map, 4) == 40);
	boa_assert(find_int(map, 5) == -1);

	boa_map_reset(map);
}

BOA_TEST(map_simple_collision, "Simple manual hash collision test")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve)
		boa_map_reserve(map, 32);

	insert_int(map, 10000, 1);
	insert_int(map, 20000, 2);
	insert_int(map, 30000, 3);
	insert_int(map, 40000, 4);

	boa_assert(map->count == 4);

	boa_assert(find_int(map, 10000) == 1);
	boa_assert(find_int(map, 20000) == 2);
	boa_assert(find_int(map, 30000) == 3);
	boa_assert(find_int(map, 40000) == 4);
	boa_assert(find_int(map, 50000) == -1);

	boa_map_reset(map);
}

BOA_TEST(map_medium, "Insert a medium amount of keys")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	uint32_t count = 1000;

	if (g_do_reserve) {
		boa_map_reserve(map, count);
		boa_assert(map->capacity >= count);
	}

	for (uint32_t i = 0; i < count; i++)
		insert_int(map, i, i * i);

	boa_assert(map->count == count);

	for (uint32_t i = 0; i < count; i++)
		boa_assertf(find_int(map, i) == i * i, "Index: %i", i);

	boa_map_reset(map);
}

BOA_TEST(map_erase_simple, "Erase values from a hash map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve) {
		boa_map_reserve(map, 64);
	}

	insert_int(map, 1, 10);
	insert_int(map, 2, 20);
	insert_int(map, 3, 30);
	insert_int(map, 4, 40);

	erase_int(map, 2);
	erase_int(map, 4);

	boa_assert(find_int(map, 1) == 10);
	boa_assert(find_int(map, 2) == -1);
	boa_assert(find_int(map, 3) == 30);
	boa_assert(find_int(map, 4) == -1);
	boa_assert(find_int(map, 5) == -1);

	boa_map_reset(map);
}

BOA_TEST(map_iterate, "Iterating a small map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve) {
		boa_map_reserve(map, 64);
	}

	insert_int(map, 1, 10);
	insert_int(map, 2, 20);
	insert_int(map, 4, 40);

	int visited_value[6] = { 0 };

	uint32_t element = boa_map_begin(map);
	for (; element != ~0u; element = boa_map_next(map, element)) {
		int key = *boa_key(int, map, element);
		int val = *boa_val(int, map, element);
		boa_assertf(key >= 0 && key < 6, "Key out of range: %d", key);
		visited_value[key] = val;
	}

	boa_assert(visited_value[0] == 0);
	boa_assert(visited_value[1] == 10);
	boa_assert(visited_value[2] == 20);
	boa_assert(visited_value[3] == 0);
	boa_assert(visited_value[4] == 40);
	boa_assert(visited_value[5] == 0);

	boa_map_reset(map);
}

BOA_TEST(map_iterate_medium, "Iterate a medium amount of keys")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	uint32_t count = 1000;

	if (g_do_reserve) {
		boa_map_reserve(map, count);
		boa_assert(map->capacity >= count);
	}

	for (uint32_t i = 0; i < count; i++)
		insert_int(map, i, i * i);

	boa_assert(map->count == count);

	int visited_value[1000] = { 0 };
	uint32_t num_visited = 0;

	uint32_t element = boa_map_begin(map);
	for (; element != ~0u; element = boa_map_next(map, element)) {
		int key = *boa_key(int, map, element);
		int val = *boa_val(int, map, element);
		visited_value[key] = val;
		num_visited++;
	}

	boa_assertf(num_visited == count, "num_visited == %u", num_visited);

	for (uint32_t i = 0; i < count; i++) {
		boa_assertf(visited_value[i] == i * i, "Index %u", i);
	}

	boa_map_reset(map);
}

BOA_TEST_BEGIN_PERMUTATION(g_hash_factor, hash_factors_no_zero)

BOA_TEST(map_large, "Insert a large amount of keys")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	uint32_t count = 10000;

	if (g_do_reserve) {
		boa_map_reserve(map, count);
		boa_assert(map->capacity >= count);
	}

	for (uint32_t i = 0; i < count; i++)
		insert_int(map, i, i * i);

	boa_assert(map->count == count);

	for (uint32_t i = 0; i < count; i++)
		boa_assertf(find_int(map, i) == i * i, "Index: %i", i);

	boa_map_reset(map);
}

BOA_TEST_END_PERMUTATION(g_hash_factor)
BOA_TEST_END_PERMUTATION(g_do_reserve)

