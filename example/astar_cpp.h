#pragma once

#include <boa_core_cpp.h>
#include <math.h>

namespace astar {

struct point { int x, y; };
point operator+(point a, point b) { return { a.x + b.x, a.y + b.y }; }
bool operator==(point a, point b) { return a.x == b.x && a.y == b.y; }

struct map {
	int width, height;
	boa::buf<float> data;

	map(int width, int height)
		: width(width)
		, height(height)
	{
		float *ptr = data.push_n(width * height);
		if (ptr) memset(ptr, 0, sizeof(float) * width * height);
	}

	void set(int x, int y, float weight) {
		boa_assert(!(x < 0 || y < 0 || x >= width || y >= height));
		data[y * width + x] = weight;
	}

	float weight(int x, int y) const {
		if (x < 0 || y < 0 || x >= width || y >= height) return INFINITY;
		return data[y * width + x];
	}

	float weight(point p) const {
		return weight(p.x, p.y);
	}
};

struct state {
	point point;
	float distance;
	uint32_t parent_pos;
};

struct work_item {
	float score;
	uint32_t state_pos;

	bool operator<(const work_item &w) const {
		return score < w.score;
	}
};

point offsets[4] = {
	{ +1, 0 },
	{ -1, 0 },
	{ 0, +1 },
	{ 0, -1 },
};

float heuristic(point a, point b)
{
	float dx = (float)(b.x - a.x);
	float dy = (float)(b.y - a.y);
	return sqrtf(dx*dx + dy*dy);
}

bool pathfind(boa::buf<point> &path, const map &map, point begin, point end, boa::allocator *ator = NULL)
{
	// Edge case: Null path
	if (begin == end) {
		point *res = path.push();
		if (res) {
			*res = begin;
			return true;
		} else {
			return false;
		}
	}

	state stack_states[64];
	work_item stack_work[64];

	boa::bset<point> closed;
	boa::buf<state> states{ boa::array_buf_ator(stack_states, ator) };
	boa::pqueue<work_item> work{ boa::array_buf_ator(stack_work, ator) };

	// TODO: Allocator + stack constructor
	closed.ator = ator;

	state initial;
	initial.point = begin;
	initial.distance = 0.0f;
	initial.parent_pos  = ~0u;
	states.push(initial);
	work.enqueue({ 0.0f, 0 });

	while (work.non_empty()) {
		int num = 0;
		for (auto &it : work.buf) {
			boa_assert(it.score >= 0.0f);
			boa_assert(it.state_pos % sizeof(state) == 0);
			boa_assert(it.state_pos < states.end_pos);
			num++;
		}

		work_item cur_work = work.dequeue();

		state cur = states.from_pos(cur_work.state_pos);

		if (cur.point == end) {
			uint32_t pos = cur_work.state_pos;
			uint32_t num = 0;
			while (pos != ~0u) {
				state &s = states.from_pos(pos);
				pos = s.parent_pos;
				num++;
			}
			
			point *points = path.push_n(num);
			if (points == NULL) return false;
			points += num;

			pos = cur_work.state_pos;
			for (uint32_t i = 0; i < num; i++) {
				state &s = states.from_pos(pos);
				pos = s.parent_pos;
				*--points = s.point;
			}

			return true;
		}

		for (point dir : offsets) {
			point pt = cur.point + dir;
			float weight = map.weight(pt);
			if (weight == INFINITY) continue;
			if (!closed.insert(pt).inserted) continue;

			uint32_t next_pos = states.end_pos;
			boa_assert(next_pos % sizeof(state) == 0);

			state *next = states.push();
			if (!next) return false;
			next->point = pt;
			next->distance = cur.distance + weight;
			next->parent_pos = cur_work.state_pos;

			float score = next->distance + heuristic(pt, end);
			if (!work.enqueue({ score, next_pos })) return false;
			boa_assert(work.buf.end_pos % sizeof(work_item) == 0);
		}
	}

	return false;
}

}

