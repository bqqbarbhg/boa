#if BOA_BENCHMARK_IMPL
#include <unordered_map>

size_t std_hash_combine(size_t hash, size_t value)
{
	return hash ^ (value + 0x9e3779b9 + (hash << 6) + (hash >> 2));
}

bool operator==(const int_pair &a, const int_pair &b)
{
	return pair_cmp(&a, &b);
}

namespace std {
	template <>
	struct hash<int_pair> {
		size_t operator()(const int_pair& k) const {
			size_t x = std::hash<int>()(k.x);
			size_t y = std::hash<int>()(k.y);
			return std_hash_combine(x, y);
		}
	};
}
#endif

BOA_BENCHMARK_BEGIN_COUNT(map_sizes);
BOA_BENCHMARK_BEGIN_PERMUTATION_U32(g_do_reserve, reserve_values);

BOA_BENCHMARK(std_int_map_insert_consecutive, "Insert consecutive integers into a map")
{
	std::unordered_map<int, int> map;

	if (g_do_reserve)
		map.reserve(g_map_size);

	boa_benchmark_for() {
		if (g_do_reserve) {
			map.clear();
		} else {
			map = std::unordered_map<int, int>();
		}

		uint32_t size = boa_benchmark_count();
		for (uint32_t i = 0; i < size; i++) {
			map[i] = i;
		}
	}
}

BOA_BENCHMARK(std_pair_map_insert_consecutive, "Insert consecutive pairs into a map")
{
	std::unordered_map<int_pair, int> map;

	if (g_do_reserve)
		map.reserve(g_map_size);

	boa_benchmark_for() {
		if (g_do_reserve) {
			map.clear();
		} else {
			map = std::unordered_map<int_pair, int>();
		}

		uint32_t size = boa_benchmark_count();
		for (uint32_t i = 0; i < size; i++) {
			int_pair pair;
			pair.x = i;
			pair.y = i;
			map[pair] = i;
		}
	}
}

BOA_BENCHMARK_P(std_int_pair_map_insert_interleaved, "Insert interleaved consecutive integers and pairs into maps")
{
	std::unordered_map<int, int> mapi;
	std::unordered_map<int_pair, int> mapp;

	if (g_do_reserve) {
		mapi.reserve(g_map_size);
		mapp.reserve(g_map_size);
	}

	boa_benchmark_for() {
		if (g_do_reserve) {
			mapi.clear();
			mapp.clear();
		} else {
			mapi = std::unordered_map<int, int>();
			mapp = std::unordered_map<int_pair, int>();
		}

		uint32_t size = boa_benchmark_count();
		for (uint32_t i = 0; i < size; i++) {
			int_pair pair;
			pair.x = i;
			pair.y = i;
			mapi[i] = i;
			mapp[pair] = i;
		}
	}
}

BOA_BENCHMARK_END_PERMUTATION(g_do_reserve);

BOA_BENCHMARK(std_int_map_find_consecutive, "Find consecutive integers from an int_map")
{
	std::unordered_map<int, int> map;

	if (g_do_reserve)
		map.reserve(g_map_size);

	uint32_t size = boa_benchmark_count();

	for (uint32_t i = 0; i < size; i++) {
		map[i] = i;
	}

	boa_benchmark_for() {
		for (uint32_t i = 0; i < size; i++) {
			auto it = map.find(i);
			boa_benchmark_assert(it->second == i);
		}
	}
}

BOA_BENCHMARK(std_int_map_find_consecutive_missing, "Find consecutive missing integers from an int_map")
{
	std::unordered_map<int, int> map;

	if (g_do_reserve)
		map.reserve(g_map_size);

	uint32_t size = boa_benchmark_count();

	for (uint32_t i = 0; i < size; i++) {
		map[i] = i;
	}

	boa_benchmark_for() {
		for (uint32_t i = size; i < size * 2; i++) {
			auto it = map.find(i);
			boa_benchmark_assert(it == map.end());
		}
	}
}

BOA_BENCHMARK_END_COUNT();


