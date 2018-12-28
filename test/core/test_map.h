
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL
uint32_t g_hash_factor;
int g_do_reserve;
int g_insert_reversed;

uint32_t int_hash(int i) { return i % 10000 * g_hash_factor; }
int int_cmp(const void *a, const void *b, boa_map *m) { return *(int*)a == *(int*)b; }

void insert_int(boa_map *map, int k, int v)
{
	boa_map_insert_result ires = boa_map_insert_inline(map, &k, int_hash(k), &int_cmp);
	boa_assert(ires.inserted);
	boa_key(int, map, ires.value) = k;
	boa_val(int, map, ires.value) = v;
}

int find_int(boa_map *map, int k)
{
	void *value = boa_map_find_inline(map, &k, int_hash(k), &int_cmp);
	if (value != NULL) {
		boa_assert(boa_key(int, map, value) == k);
		return boa_val(int, map, value);
	} else {
		return -1;
	}
}

void erase_int(boa_map *map, int k)
{
	void *value = boa_map_find_inline(map, &k, int_hash(k), &int_cmp);
	if (value != NULL) {
		boa_assert(boa_key(int, map, value) == k);
		boa_map_remove(map, value);
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

	if (g_insert_reversed) {
		for (uint32_t i = 0; i < count; i++)
			insert_int(map, i, i * i);
	} else {
		for (int i = count - 1; i >= 0; i--)
			insert_int(map, i, i * i);
	}

	boa_assert(map->count == count);
}

#else

extern uint32_t g_hash_factor;
extern int g_do_reserve;
extern int g_insert_reversed;

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

static int insert_reversed_values[] = {
	0, 1,
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

BOA_TEST_BEGIN_PERMUTATION_U32(g_insert_reversed, insert_reversed_values)

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

BOA_TEST(map_medium_insert, "Insert over existing items")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 1000;
	init_square_map(map, count);

	for (uint32_t i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		int key = (int)i;
		boa_map_insert_result ires = boa_map_insert(map, &key, int_hash(key), &int_cmp);
		boa_assert(!ires.inserted);
		boa_val(int, map, ires.value) = key * key * key;
	}

	for (uint32_t i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		boa_assert(find_int(map, i) == i * i * i);
	}

	boa_map_reset(map);
}

BOA_TEST(map_remove_simple, "Remove values from a hash map")
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

	boa_map_iterator it = boa_map_begin(map);
	boa_assert(it.value);
	boa_map_advance(map, &it);
	boa_assert(it.value);
	boa_map_advance(map, &it);
	boa_assert(it.value == NULL);

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

	boa_map_iterator it = boa_map_begin(map);
	for (; it.value; boa_map_advance(map, &it)) {
		int key = boa_key(int, map, it.value);
		int val = boa_val(int, map, it.value);
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

	boa_map_iterator it = boa_map_begin(map);
	for (; it.value; boa_map_advance(map, &it)) {
		int key = boa_key(int, map, it.value);
		int val = boa_val(int, map, it.value);
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

BOA_TEST(map_remove_medium_find, "Remove a medium amount of keys by find")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 1000;
	init_square_map(map, count);

	for (uint32_t i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		int key = (int)i;
		uint32_t hash = int_hash(key);
		void *value = boa_map_find(map, &key, hash, &int_cmp);
		boa_assert(value != NULL);
		boa_map_remove(map, value);
		boa_assert(map->count == count - i - 1);
	}

	boa_map_reset(map);
}

BOA_TEST(map_remove_medium_iterate, "Remove a medium amount of keys by iteration")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 1000;
	init_square_map(map, count);

	boa_map_iterator it = boa_map_begin(map);
	uint32_t iter = 0;
	while (it.value) {
		iter++;
		boa_test_hint_u32(iter);
		it = boa_map_remove_iter(map, it.value);
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

	boa_map_iterator it = boa_map_begin(map);
	uint32_t iter = 0;
	while (it.value) {
		iter++;
		boa_test_hint_u32(iter);

		int key = boa_key(int, map, it.value);
		if (key % 2 == 1) {
			it = boa_map_remove_iter(map, it.value);
		} else {
			boa_map_advance(map, &it);
		}
	}

	boa_assert(iter == count);
	boa_assert(map->count == count / 2);

	iter = 0;
	it = boa_map_begin(map);
	for (; it.value; boa_map_advance(map, &it)) {
		iter++;
		boa_test_hint_u32(iter);
		int key = boa_key(int, map, it.value);
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
		boa_map_insert_result ires = boa_map_insert(map, &key, hash, &int_cmp);
		boa_assert(ires.value);
		boa_assert(ires.inserted);
		boa_map_remove(map, ires.value);
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
		boa_map_insert_result ires = boa_map_insert(map, &key, hash, &int_cmp);
		if (had_space) {
			boa_assert(ires.inserted);
			boa_assert(ires.value != NULL);
			boa_key(int, map, ires.value) = i;
			boa_val(int, map, ires.value) = i * i;
		} else {
			boa_assert(ires.value == NULL);
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
BOA_TEST_END_PERMUTATION(g_insert_reversed)

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
		boa_map_insert_result ires = boa_map_insert(map, &key, 0, &int_cmp);
		if (ires.value) {
			boa_assert(ires.inserted);
			boa_key(int, map, ires.value) = i;
			boa_val(int, map, ires.value) = i * i;
		} else {
			inserted_until = i;
			break;
		}
	}

	boa_assert(inserted_until > 0);
	boa_assert(map->count == inserted_until);

	for (uint32_t i = 0; i < inserted_until; i++) {
		int key = (int)i;
		void *value = boa_map_find(map, &key, 0, &int_cmp);
		boa_assert(boa_key(int, map, value) == i);
		boa_assert(boa_val(int, map, value) == i * i);
	}

	boa_map_reset(map);
}


