
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL
uint32_t int_hash(int i){ return i % 10; }
int int_cmp(const void *a, const void *b){ return *(int*)a == *(int*)b; }

void insert_int(boa_map *map, int i)
{
	void *kv = boa_map_insert(map, &i, int_hash(i), &int_cmp);
	*(int*)kv = i;
}
#endif


BOA_TEST(map_simple, "Simple manual map test")
{
	char data[1024] = { 0 };
	boa_map map;
	map.data = data;
	map.size = 8;
	map.kv_size = sizeof(int) * 2;
	map.chunk_size = 8 + map.kv_size * 8;
	map.aux_chunk_begin = (map.size - 1) * map.chunk_size;
	map.num_aux = 0;

	insert_int(&map, 0);
	insert_int(&map, 10);
	insert_int(&map, 1);
}

