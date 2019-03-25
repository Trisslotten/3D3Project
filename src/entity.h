#pragma once

struct vec2 {
	uint32_t x;
	uint32_t y;

	vec2(uint32_t x, uint32_t y) : x(x), y(y) {}
	vec2() {
		x = 0; y = 0;
	}
};

struct Entity {
	vec2 pos;

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

	void move(vec2 mv) {
		pos.x += mv.x;
		pos.y += mv.y;
	}
	void place(vec2 pos) {
		pos.x = pos.x;
		pos.y = pos.y;
	}
};