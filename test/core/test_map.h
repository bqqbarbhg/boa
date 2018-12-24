
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
	if (elem != ~0u) {
		boa_assert(*boa_key(int, map, elem) == k);
		return *boa_val(int, map, elem);
	} else {
		return -1;
	}
}

void erase_int(boa_map *map, int k)
{
	uint32_t elem = boa_map_find_inline(map, &k, int_hash(k), &int_cmp);
	if (elem != ~0u) {
		boa_assert(*boa_key(int, map, elem) == k);
		boa_map_erase(map, elem);
	}
}

void init_square_map(boa_map *map, uint32_t count)
{
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve) {
		boa_map_reserve(map, count);
		boa_assert(map->capacity >= count);
	}

	for (uint32_t i = 0; i < count; i++)
		insert_int(map, i, i * i);

	boa_assert(map->count == count);
}

#else

extern uint32_t g_hash_factor;
extern int g_do_reserve;

static uint32_t hash_factors[] = {
	1051, // < Provides reasonable hashing
	13,   // < Provides ok hashing
	1,    // < Degrades to linear
	0,    // < Degrades to linked list
};

static uint32_t hash_factors_no_zero[] = {
	1051, // < Provides reasonable hashing
	13,   // < Provides ok hashing
	1,    // < Degrades to linear
};

static int reserve_values[] = {
	1, 0,
};

#endif

BOA_TEST_BEGIN_PERMUTATION_U32(g_hash_factor, hash_factors)
BOA_TEST_BEGIN_PERMUTATION_U32(g_do_reserve, reserve_values)

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
	uint32_t count = 1000;
	init_square_map(map, count);

	for (uint32_t i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		boa_assertf(find_int(map, i) == i * i, "Index: %i", i);
	}

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

	boa_assert(map->count == 2);

	boa_assert(find_int(map, 1) == 10);
	boa_assert(find_int(map, 2) == -1);
	boa_assert(find_int(map, 3) == 30);
	boa_assert(find_int(map, 4) == -1);
	boa_assert(find_int(map, 5) == -1);

	uint32_t element = boa_map_begin(map);
	boa_assert(element != ~0u);
	element = boa_map_next(map, element);
	boa_assert(element != ~0u);
	element = boa_map_next(map, element);
	boa_assert(element == ~0u);

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
		boa_test_hint_u32(element);
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
	uint32_t count = 1000;
	init_square_map(map, count);

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
		boa_test_hint_u32(i);
		boa_assert(visited_value[i] == i * i);
	}

	boa_map_reset(map);
}

BOA_TEST(map_erase_medium_find, "Erase a medium amount of keys by find")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 1000;
	init_square_map(map, count);

	for (uint32_t i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		int key = (int)i;
		uint32_t hash = int_hash(key);
		uint32_t element = boa_map_find(map, &key, hash, &int_cmp);
		boa_assert(element != ~0u);
		boa_map_erase(map, element);
		boa_assert(map->count == count - i - 1);
	}

	boa_map_reset(map);
}

BOA_TEST(map_erase_medium_iterate, "Erase a medium amount of keys by iteration")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 1000;
	init_square_map(map, count);

	uint32_t element = boa_map_begin(map);
	uint32_t iter = 0;
	while (element != ~0u) {
		iter++;
		boa_test_hint_u32(iter);
		boa_test_hint_u32(element);
		boa_assert(element % map->impl.block_num_elements == 0);
		element = boa_map_erase(map, element);
		boa_assert(map->count == count - iter);
	}

	boa_assert(iter == count);

	boa_map_reset(map);
}

BOA_TEST(map_erase_erase_odd, "Erase odd keys from a map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 1000;
	init_square_map(map, count);

	uint32_t element = boa_map_begin(map);
	uint32_t iter = 0;
	while (element != ~0u) {
		iter++;
		boa_test_hint_u32(iter);
		boa_test_hint_u32(element);

		int key = *boa_key(int, map, element);
		if (key % 2 == 1) {
			element = boa_map_erase(map, element);
		} else {
			element = boa_map_next(map, element);
		}
	}

	boa_assert(iter == count);
	boa_assert(map->count == count / 2);

	iter = 0;
	element = boa_map_begin(map);
	for (; element != ~0u; element = boa_map_next(map, element)) {
		iter++;
		boa_test_hint_u32(iter);
		boa_test_hint_u32(element);
		int key = *boa_key(int, map, element);
		boa_assert(key % 2 == 0);
	}

	boa_assert(iter == map->count);

	boa_map_reset(map);
}

BOA_TEST(map_allocator, "Set allocator for map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_test_allocator ator = boa_test_allocator_make();
	map->ator = &ator.ator;
	uint32_t count = 1000;
	init_square_map(map, count);

	boa_assert(ator.allocs >= 1);
	boa_assert(ator.reallocs == 0);
	boa_assert(ator.frees == ator.allocs - 1);

	boa_map_reset(map);
	boa_assert(ator.frees == ator.allocs);
}

BOA_TEST(map_erase_insert_loop, "Insert + erase loop should not reallocate")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_test_allocator ator = boa_test_allocator_make();
	map->ator = &ator.ator;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	boa_map_reserve(map, 16);

	uint32_t capacity = map->capacity;
	uint32_t allocs = ator.allocs;
	uint32_t reallocs = ator.reallocs;

	for (uint32_t i = 0; i < 10000; i++) {
		int key = (int)i;
		uint32_t hash = int_hash(key);
		uint32_t element = boa_map_insert(map, &key, hash, &int_cmp);
		boa_assert(element != ~0u);
		boa_map_erase(map, element);
	}

	boa_assert(capacity == map->capacity);
	boa_assert(allocs == ator.allocs);
	boa_assert(reallocs == ator.reallocs);

	boa_map_reset(map);
}

BOA_TEST(map_insert_alloc_fail, "Insert should fail gracefully")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_test_allocator ator = boa_test_allocator_make();
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	boa_map_reserve(map, 16);

	boa_test_fail_next_allocation();
	uint32_t inserted_until = 0;

	for (uint32_t i = 0; i < 64; i++) {
		int had_space = (map->count < map->capacity) ? 1 : 0;
		int key = (int)i;
		uint32_t hash = int_hash(key);
		uint32_t element = boa_map_insert(map, &key, hash, &int_cmp);
		if (had_space) {
			*boa_key(int, map, element) = i;
			*boa_val(int, map, element) = i * i;
			boa_assert(element != ~0u);
		} else {
			boa_assert(element == ~0u);
			inserted_until = i;
			break;
		}
	}

	boa_assert(inserted_until > 0);
	boa_assert(map->count == inserted_until);

	for (uint32_t i = 0; i < inserted_until; i++) {
		boa_assert(find_int(map, i) == i * i);
	}

	boa_map_reset(map);
}

BOA_TEST_BEGIN_PERMUTATION_U32(g_hash_factor, hash_factors_no_zero)

BOA_TEST(map_large, "Insert a large amount of keys")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 20000;
	init_square_map(map, count);

	for (uint32_t i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		boa_assert(find_int(map, i) == i * i);
	}

	boa_map_reset(map);
}

BOA_TEST_END_PERMUTATION(g_hash_factor)
BOA_TEST_END_PERMUTATION(g_do_reserve)

BOA_TEST(map_insert_alloc_fail_fallback, "Insert should fail gracefully on aux block allocation")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_test_allocator ator = boa_test_allocator_make();
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	boa_map_reserve(map, 1024);

	boa_test_fail_next_allocation();
	uint32_t inserted_until = 0;

	for (uint32_t i = 0; i < 1024; i++) {
		boa_assert(map->count < map->capacity);
		int key = (int)i;
		uint32_t element = boa_map_insert(map, &key, 0, &int_cmp);
		if (element != ~0u) {
			*boa_key(int, map, element) = i;
			*boa_val(int, map, element) = i * i;
		} else {
			inserted_until = i;
			break;
		}
	}

	boa_assert(inserted_until > 0);
	boa_assert(map->count == inserted_until);

	for (uint32_t i = 0; i < inserted_until; i++) {
		int key = (int)i;
		uint32_t elem = boa_map_find_inline(map, &key, 0, &int_cmp);
		boa_assert(*boa_key(int, map, elem) == i);
		boa_assert(*boa_val(int, map, elem) == i * i);
	}

	boa_map_reset(map);
}


