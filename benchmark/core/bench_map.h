
#undef bench_map_insert
#undef bench_map_find
#undef P

#if !defined(BENCH_MAP_INLINE)
	#error "BENCH_MAP_INLINE not defined!"
#elif BENCH_MAP_INLINE
	#define bench_map_insert boa_map_insert_inline
	#define bench_map_find boa_map_find_inline
	#define P(x) x##_inline
#else
	#define bench_map_insert boa_map_insert
	#define bench_map_find boa_map_find
	#define P(x) x##_generic
#endif

#if BENCH_MAP_INLINE == 0
#if BOA_BENCHMARK_IMPL
uint32_t g_map_size;
uint32_t g_do_reserve;

boa_inline uint32_t int_hash(int i)
{
	uint32_t x = (uint32_t)i;
#if 0
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
#else
	x *= 2654435761u;
#endif
	return x;
}
boa_inline int int_cmp(const void *a, const void *b) { return *(int*)a == *(int*)b; }

typedef struct int_pair { int x, y; } int_pair;

boa_inline uint32_t pair_hash(int_pair pair)
{
	uint32_t x = int_hash(pair.x);
	uint32_t y = int_hash(pair.y);
	return boa_hash_combine(x, y);
}
boa_inline int pair_cmp(const void *a, const void *b) {
	const int_pair *pa = (const int_pair*)a, *pb = (const int_pair*)b;
	if (pa->x != pb->x) return 0;
	if (pa->y != pb->y) return 0;
	return 1;
}

#else

extern uint32_t g_map_size;
extern uint32_t g_do_reserve;

static uint32_t map_sizes[] = {
	10, 100, 1000, 10000, 100000, 1000000,
};

static int reserve_values[] = {
	1, 0,
};

#endif
#endif // BENCH_MAP_INLINE == 0

#if BOA_BENCHMARK_IMPL

boa_noinline void P(insert_int_p)(boa_map *map, int k, int v)
{
	boa_map_insert_result ires = bench_map_insert(map, &k, int_hash(k), &int_cmp);
	boa_benchmark_assert(ires.value);
	boa_key(int, map, ires.value) = k;
	boa_val(int, map, ires.value) = v;
}

boa_noinline int P(find_int_p)(boa_map *map, int k)
{
	void *value = bench_map_find(map, &k, int_hash(k), &int_cmp);
	if (value != NULL) {
		return boa_val(int, map, value);
	} else {
		return -1;
	}
}

boa_noinline void P(insert_pair_p)(boa_map *map, int_pair k, int v)
{
	boa_map_insert_result ires = bench_map_insert(map, &k, pair_hash(k), &pair_cmp);
	boa_benchmark_assert(ires.value);
	boa_key(int_pair, map, ires.value) = k;
	boa_val(int, map, ires.value) = v;
}

boa_noinline int P(find_pair_p)(boa_map *map, int_pair k)
{
	void *value = bench_map_find(map, &k, pair_hash(k), &pair_cmp);
	if (value != NULL) {
		return boa_val(int, map, value);
	} else {
		return -1;
	}
}


#endif

#define insert_int P(insert_int_p)
#define find_int P(find_int_p)
#define insert_pair P(insert_pair_p)
#define find_pair P(find_pair_p)

BOA_BENCHMARK_BEGIN_COUNT(map_sizes);
BOA_BENCHMARK_BEGIN_PERMUTATION_U32(g_do_reserve, reserve_values);
BOA_BENCHMARK_BEGIN_COMPILE_PERMUTATION(BENCH_MAP_INLINE);

BOA_BENCHMARK_P(int_map_insert_consecutive, "Insert consecutive integers into a map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve)
		boa_map_reserve(map, g_map_size);

	boa_benchmark_for() {
		if (g_do_reserve) {
			boa_map_clear(map);
		} else {
			boa_map_reset(map);
		}

		uint32_t size = boa_benchmark_count();
		for (uint32_t i = 0; i < size; i++) {
			insert_int(map, i, i);
		}
	}
}

BOA_BENCHMARK_P(pair_map_insert_consecutive, "Insert consecutive pairs into a map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int_pair);
	map->val_size = sizeof(int);

	if (g_do_reserve)
		boa_map_reserve(map, g_map_size);

	boa_benchmark_for() {
		if (g_do_reserve) {
			boa_map_clear(map);
		} else {
			boa_map_reset(map);
		}

		uint32_t size = boa_benchmark_count();
		for (uint32_t i = 0; i < size; i++) {
			int_pair pair;
			pair.x = i;
			pair.y = i;
			insert_pair(map, pair, i);
		}
	}
}

BOA_BENCHMARK_P(int_pair_map_insert_interleaved, "Insert interleaved consecutive integers and pairs into maps")
{
	boa_map mapiv = { 0 }, *mapi = &mapiv;
	mapi->key_size = sizeof(int);
	mapi->val_size = sizeof(int);
	boa_map mappv = { 0 }, *mapp = &mappv;
	mapp->key_size = sizeof(int_pair);
	mapp->val_size = sizeof(int);

	if (g_do_reserve) {
		boa_map_reserve(mapi, g_map_size);
		boa_map_reserve(mapp, g_map_size);
	}

	boa_benchmark_for() {
		if (g_do_reserve) {
			boa_map_clear(mapi);
			boa_map_clear(mapp);
		} else {
			boa_map_reset(mapi);
			boa_map_reset(mapp);
		}

		uint32_t size = boa_benchmark_count();
		for (uint32_t i = 0; i < size; i++) {
			int_pair pair;
			pair.x = i;
			pair.y = i;
			insert_int(mapi, i, i);
			insert_pair(mapp, pair, i);
		}
	}
}

BOA_BENCHMARK_END_PERMUTATION(g_do_reserve);

BOA_BENCHMARK_P(int_map_find_consecutive, "Find consecutive integers from an int_map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve)
		boa_map_reserve(map, g_map_size);

	uint32_t size = boa_benchmark_count();

	for (uint32_t i = 0; i < size; i++) {
		insert_int(map, i, i);
	}

	boa_benchmark_for() {
		for (uint32_t i = 0; i < size; i++) {
			int val = find_int(map, i);
			boa_benchmark_assert(val == i);
		}
	}
}

BOA_BENCHMARK_P(int_map_find_consecutive_missing, "Find consecutive missing integers from an int_map")
{
	boa_map mapv = { 0 }, *map = &mapv;
	map->key_size = sizeof(int);
	map->val_size = sizeof(int);

	if (g_do_reserve)
		boa_map_reserve(map, g_map_size);

	uint32_t size = boa_benchmark_count();

	for (uint32_t i = 0; i < size; i++) {
		insert_int(map, i, i);
	}

	boa_benchmark_for() {
		for (uint32_t i = size; i < size * 2; i++) {
			int val = find_int(map, i);
			boa_benchmark_assert(val == -1);
		}
	}
}

BOA_BENCHMARK_END_COUNT();
BOA_BENCHMARK_END_COMPILE_PERMUTATION(BENCH_MAP_INLINE);

#undef insert_int
#undef find_int

