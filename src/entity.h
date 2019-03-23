#pragma once

struct vec2 {
	unsigned int x;
	unsigned int y;

	vec2(unsigned int x, unsigned int y) : x(x), y(y) {}
	vec2() {
		x = 0; y = 0;
	}
};

struct Entity {
	vec2 pos;

	Entity() {
		pos.x = 0;
		pos.y = 0;
	}
	Entity(unsigned int tx, unsigned int ty) {
		pos.x = tx;
		pos.y = ty;
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