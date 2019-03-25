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

	Entity() {
		pos.x = 0;
		pos.y = 0;
	}
	Entity(unsigned int tx, unsigned int ty) {
		pos.x = tx;
		pos.y = ty;
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