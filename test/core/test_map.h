
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL
uint32_t int_hash(int i){ return i % 1000; }
int int_cmp(const void *a, const void *b){ return *(int*)a == *(int*)b; }

void insert_int(boa_map *map, int k, int v)
{
	uint32_t elem = boa_map_insert(map, &k, int_hash(k), &int_cmp);
	*boa_key(int, map, elem) = k;
	*boa_val(int, map, elem) = v;
}

int find_int(boa_map *map, int k)
{
	uint32_t elem = boa_map_find(map, &k, int_hash(k), &int_cmp);
	return elem != ~0u ? *boa_val(int, map, elem) : -1;
}

#endif


BOA_TEST(map_simple, "Simple manual map test")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

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

	insert_int(map, 1000, 1);
	insert_int(map, 2000, 2);
	insert_int(map, 3000, 3);
	insert_int(map, 4000, 4);

	boa_assert(map->count == 4);

	boa_assert(find_int(map, 1000) == 1);
	boa_assert(find_int(map, 2000) == 2);
	boa_assert(find_int(map, 3000) == 3);
	boa_assert(find_int(map, 4000) == 4);
	boa_assert(find_int(map, 5000) == -1);

	boa_map_reset(map);
}

BOA_TEST(map_large, "Insert a large amount of keys")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	for (int i = 0; i < 10000; i++)
		insert_int(map, i, i * i);

	boa_assert(map->count == 10000);

	for (int i = 0; i < 10000; i++)
		boa_assert(find_int(map, i) == i * i);

	boa_map_reset(map);
}

