#pragma once

struct uvec2 {
	unsigned int x;
	unsigned int y;

	uvec2(unsigned int x, unsigned int y) : x(x), y(y) {}
	uvec2() {
		x = 0; y = 0;
	}
};

struct ivec2 {
	int x;
	int y;

	ivec2(int x, int y) : x(x), y(y) {}
	ivec2() {
		x = 0; y = 0;
	}
};

struct Entity {
	uvec2 pos;

	bool isGoal = false;

	Entity() {
		pos.x = 0;
		pos.y = 0;
		isGoal = false;
	}
	Entity(uint32_t tx, uint32_t ty, bool _isGoal = false) {
		pos.x = tx;
		pos.y = ty;
		isGoal = _isGoal;
	}

	void move(ivec2 mv) {
		pos.x = (unsigned int)(int(pos.x) + mv.x);
		pos.y = (unsigned int)(int(pos.y) + mv.y);
	}
	void place(uvec2 pos) {
		pos.x = pos.x;
		pos.y = pos.y;
	}
};