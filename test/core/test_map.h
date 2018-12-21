
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL
uint32_t int_hash(int i){ return i % 10; }
int int_cmp(const void *a, const void *b){ return *(int*)a == *(int*)b; }

void insert_int(boa_map *map, int k, int v)
{
	uint32_t kv = boa_map_insert(map, &k, int_hash(k), &int_cmp);
	*boa_key(int, map, kv) = k;
	*boa_val(int, map, kv) = v;
}
#endif


BOA_TEST(map_simple, "Simple manual map test")
{
	boa_map map = { 0 };
	map.key_size = sizeof(int);
	map.val_size = sizeof(int);

	insert_int(&map, 1, 10);
	insert_int(&map, 2, 20);
	insert_int(&map, 3, 30);
	insert_int(&map, 4, 40);
}

