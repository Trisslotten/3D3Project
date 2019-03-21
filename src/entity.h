#pragma once

#define TEAM_FRIENDLY 0
#define TEAM_ENEMY 1

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
	unsigned int team;

	Entity() {
		pos.x = 0;
		pos.y = 0;
		team = TEAM_FRIENDLY;
	}
	Entity(unsigned int tx, unsigned int ty, unsigned int t) {
		pos.x = tx;
		pos.y = ty;
		team = t;
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