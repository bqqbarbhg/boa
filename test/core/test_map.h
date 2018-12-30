
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL
uint32_t g_hash_factor;
int g_do_reserve;
int g_insert_reversed;

uint32_t int_hash(int i) { return i % 10000 * g_hash_factor; }
int int_cmp(const void *a, const void *b, void *user) { return *(int*)a == *(int*)b; }

typedef struct { int key, val; } kv_int_int;

void insert_int(boa_map *map, int k, int v)
{
	boa_map_insert_result ires = boa_map_insert_inline(map, &k, int_hash(k), &int_cmp, NULL);
	boa_assert(ires.inserted);
	kv_int_int *kv = (kv_int_int*)ires.entry;
	kv->key = k;
	kv->val = v;
}

int find_int(boa_map *map, int k)
{
	void *entry = boa_map_find_inline(map, &k, int_hash(k), &int_cmp, NULL);
	kv_int_int *kv = (kv_int_int*)entry;
	if (kv != NULL) {
		boa_assert(kv->key == k);
		return kv->val;
	} else {
		return -1;
	}
}

void erase_int(boa_map *map, int k)
{
	void *entry = boa_map_find_inline(map, &k, int_hash(k), &int_cmp, NULL);
	kv_int_int *kv = (kv_int_int*)entry;
	if (kv != NULL) {
		boa_assert(kv->key == k);
		boa_map_remove(map, entry);
	}
}

void init_square_map_ator(boa_map *map, uint32_t count, boa_allocator *ator)
{
	boa_map_init_ator(map, sizeof(kv_int_int), ator);

	if (g_do_reserve) {
		boa_map_reserve(map, count);
		boa_assert(map->capacity >= count);
	}

	if (g_insert_reversed) {
		for (int i = count - 1; i >= 0; i--)
			insert_int(map, i, i * i);
	} else {
		for (uint32_t i = 0; i < count; i++)
			insert_int(map, i, i * i);
	}

	boa_assert(map->count == count);
}

void init_square_map(boa_map *map, uint32_t count)
{
	init_square_map_ator(map, count, NULL);
}

typedef struct string_entry {
	uint32_t length;
	char *string;
	int value;
} string_entry;

uint32_t string_hash(const char *str)
{
	uint32_t hash = 2166136261u;
	for (; *str != 0; str++) {
		hash = (hash ^ (uint32_t)*str) * 16777619u;
	}
	return hash;
}

int string_cmp(const void *a, const void *b, void *user)
{
	const char *str = (const char *)a;
	const string_entry *entry = (const string_entry*)b;
	return !strcmp(str, entry->string);
}

void insert_string(boa_map *map, const char *str, int value)
{
	uint32_t hash = string_hash(str);
	boa_map_insert_result ires = boa_map_insert_inline(map, str, hash, &string_cmp, NULL);
	string_entry *entry = (string_entry*)ires.entry;
	if (ires.inserted) {
		entry->length = (uint32_t)strlen(str);
		entry->string = (char*)boa_alloc(entry->length + 1);
		memcpy(entry->string, str, entry->length + 1);
	}

	entry->value = value;
}

int find_string(boa_map *map, const char *str)
{
	uint32_t hash = string_hash(str);
	string_entry *entry = (string_entry*)boa_map_find_inline(map, str, hash, &string_cmp, NULL);
	if (entry != NULL) {
		boa_assert(!strcmp(entry->string, str));
		return entry->value;
	} else {
		return -1;
	}
}

void erase_string(boa_map *map, const char *str)
{
	uint32_t hash = string_hash(str);
	string_entry *entry = (string_entry*)boa_map_find_inline(map, str, hash, &string_cmp, NULL);
	if (entry != NULL) {
		boa_assert(!strcmp(entry->string, str));
		boa_free(entry->string);
		boa_map_remove(map, entry);
	}
}

void string_map_reset(boa_map *map)
{
	boa_map_for (string_entry, entry, map) {
		boa_free(entry->string);
	}

	boa_map_reset(map);
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
	boa_map_init(map, sizeof(kv_int_int));

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
	boa_map_init(map, sizeof(kv_int_int));

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
		boa_assert(find_int(map, i) == i * i);
	}

	boa_map_reset(map);
}

BOA_TEST(map_medium_remove_half, "Remove half of the keys of a map and find the remaining ones")
{
	boa_map mapv = { 0 }, *map = &mapv;
	uint32_t count = 1000;
	init_square_map(map, count);

	for (uint32_t i = 0; i < count; i += 2) {
		erase_int(map, i);
	}

	boa_assert(map->count == count / 2);

	for (uint32_t i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		int val = find_int(map, i);
		if (i % 2 == 0) {
			boa_assert(val == -1);
		} else {
			boa_assert(val == i * i);
		}
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
		boa_map_insert_result ires = boa_map_insert(map, &key, int_hash(key), &int_cmp, NULL);
		kv_int_int *kv = (kv_int_int*)ires.entry;
		boa_assert(!ires.inserted);
		kv->val = key * key * key;
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
	boa_map_init(map, sizeof(kv_int_int));

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
	boa_assert(it.entry);
	boa_map_advance(map, &it);
	boa_assert(it.entry);
	boa_map_advance(map, &it);
	boa_assert(it.entry == NULL);

	boa_map_reset(map);
}

BOA_TEST(map_iterate, "Iterating a small map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_map_init(map, sizeof(kv_int_int));

	if (g_do_reserve) {
		boa_map_reserve(map, 64);
	}

	insert_int(map, 1, 10);
	insert_int(map, 2, 20);
	insert_int(map, 4, 40);

	int visited_value[6] = { 0 };

	boa_map_iterator it = boa_map_begin(map);
	for (; it.entry; boa_map_advance(map, &it)) {
		kv_int_int *kv = (kv_int_int*)it.entry;
		boa_assertf(kv->key >= 0 && kv->key < 6, "Key out of range: %d", kv->key);
		visited_value[kv->key] = kv->val;
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
	for (; it.entry; boa_map_advance(map, &it)) {
		kv_int_int *kv = (kv_int_int*)it.entry;
		visited_value[kv->key] = kv->val;
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
		void *value = boa_map_find(map, &key, hash, &int_cmp, NULL);
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
	while (it.entry) {
		iter++;
		boa_test_hint_u32(iter);
		it = boa_map_remove_iter(map, it.entry);
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
	while (it.entry) {
		iter++;
		boa_test_hint_u32(iter);

		int key = ((kv_int_int*)it.entry)->key;
		if (key % 2 == 1) {
			it = boa_map_remove_iter(map, it.entry);
		} else {
			boa_map_advance(map, &it);
		}
	}

	boa_assert(iter == count);
	boa_assert(map->count == count / 2);

	iter = 0;
	it = boa_map_begin(map);
	for (; it.entry; boa_map_advance(map, &it)) {
		iter++;
		boa_test_hint_u32(iter);
		int key = ((kv_int_int*)it.entry)->key;
		boa_assert(key % 2 == 0);
	}

	boa_assert(iter == map->count);

	boa_map_reset(map);
}

BOA_TEST(map_allocator, "Set allocator for map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_test_allocator ator = boa_test_allocator_make();
	uint32_t count = 1000;
	init_square_map_ator(map, count, &ator.ator);

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
	boa_map_init_ator(map, sizeof(kv_int_int), &ator.ator);

	boa_map_reserve(map, 16);

	uint32_t capacity = map->capacity;
	uint32_t allocs = ator.allocs;
	uint32_t reallocs = ator.reallocs;

	for (uint32_t i = 0; i < 10000; i++) {
		int key = (int)i;
		uint32_t hash = int_hash(key);
		boa_map_insert_result ires = boa_map_insert(map, &key, hash, &int_cmp, NULL);
		boa_assert(ires.entry);
		boa_assert(ires.inserted);
		boa_map_remove(map, ires.entry);
	}

	boa_assert(capacity == map->capacity);
	boa_assert(allocs == ator.allocs);
	boa_assert(reallocs == ator.reallocs);

	boa_map_reset(map);
}

BOA_TEST(map_insert_alloc_fail, "Insert should fail gracefully")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_map_init(map, sizeof(kv_int_int));

	boa_map_reserve(map, 16);

	boa_test_fail_next_allocation();
	uint32_t inserted_until = 0;

	for (uint32_t i = 0; i < 64; i++) {
		int had_space = (map->count < map->capacity) ? 1 : 0;
		int key = (int)i;
		uint32_t hash = int_hash(key);
		boa_map_insert_result ires = boa_map_insert(map, &key, hash, &int_cmp, NULL);
		kv_int_int *kv = (kv_int_int*)ires.entry;
		if (had_space) {
			boa_assert(ires.inserted);
			boa_assert(ires.entry != NULL);
			kv->key = i;
			kv->val = i * i;
		} else {
			boa_assert(ires.entry == NULL);
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
	uint32_t count = 10000;
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
	boa_map_init(map, sizeof(kv_int_int));

	boa_map_reserve(map, 1024);

	boa_test_fail_next_allocation();
	uint32_t inserted_until = 0;

	for (uint32_t i = 0; i < 1024; i++) {
		boa_assert(map->count < map->capacity);
		int key = (int)i;
		boa_map_insert_result ires = boa_map_insert(map, &key, 0, &int_cmp, NULL);
		kv_int_int *kv = (kv_int_int*)ires.entry;
		if (kv) {
			boa_assert(ires.inserted);
			kv->key = i;
			kv->val = i * i;
		} else {
			inserted_until = i;
			break;
		}
	}

	boa_assert(inserted_until > 0);
	boa_assert(map->count == inserted_until);

	for (uint32_t i = 0; i < inserted_until; i++) {
		int key = (int)i;
		kv_int_int *kv = (kv_int_int*)boa_map_find(map, &key, 0, &int_cmp, NULL);
		boa_assert(kv->key == i);
		boa_assert(kv->val == i * i);
	}

	boa_map_reset(map);
}

BOA_TEST(string_map_simple, "Simple manual string map test")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_map_init(map, sizeof(string_entry));

	if (g_do_reserve)
		boa_map_reserve(map, 32);

	insert_string(map, "Hello", 10);
	insert_string(map, "World", 20);

	boa_assert(map->count == 2);

	boa_assert(find_string(map, "Hello") == 10);
	boa_assert(find_string(map, "World") == 20);
	boa_assert(find_string(map, "What") == -1);

	erase_string(map, "Hello");
	boa_assert(map->count == 1);

	boa_assert(find_string(map, "Hello") == -1);
	boa_assert(find_string(map, "World") == 20);

	string_map_reset(map);
}

BOA_TEST(string_map_medium, "String map test with medium amount of keys")
{
	boa_map mapv = { 0 }, *map = &mapv;
	boa_map_init(map, sizeof(string_entry));

	uint32_t count = 1000;

	if (g_do_reserve)
		boa_map_reserve(map, count);

	char fmtarr[128];
	boa_buf fmtbuf = boa_array_buf(fmtarr);
	for (uint32_t i = 0; i < count; i++) {
		char *key = boa_format(boa_clear(&fmtbuf), "%u", i);
		insert_string(map, key, i);
	}

	boa_assert(map->count == count);

	for (uint32_t i = 0; i < count; i++) {
		char *key = boa_format(boa_clear(&fmtbuf), "%u", i);
		int val = find_string(map, key);
		boa_assert(val == i);
	}

	boa_reset(&fmtbuf);

	string_map_reset(map);
}
