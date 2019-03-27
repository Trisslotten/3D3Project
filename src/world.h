#pragma once
#include <vector>
#include "entity.h"

struct Pixel {
	unsigned char r, g, b, a;
};

class World {
private:
	
	uvec2 dims;
	ivec2* steps;
	unsigned int stepsCount = 20;
	unsigned int* emptySteps;
	bool goalReached = false;

	
public:
	World();
	~World();
	
	void setNewGoal();

	void init(std::string filename, unsigned int entityCount);
	
	void addEntity(uvec2 pos) {
		Entity ent(pos.x, pos.y);
		entities.push_back(uvec2(pos.x, pos.y));
	}

	void printEntities() {
		for (uvec2 ent : entities) {
			printf("entity at: %d %d\n", ent.x, ent.y);
		}
	}

	void updateEntities();


	uvec2 getMapDims() const
	{
		return dims;
	}
	unsigned int* getMap() const
	{
		return origMap;
	}

	unsigned int getStepsCount() {
		return stepsCount;
	}

	void setSteps(ivec2* s) {
		steps = s;
		stepsCount = 19;
		goalReached = false;

		for (int e = 0; e < entities.size(); e++) {
			emptySteps[e] = 0;
			int step = 19;
			while (steps[e*20 + step].x == 0 && steps[e* 20 + step].y == 0 && step >= 1) {
				emptySteps[e] = emptySteps[e]+1;
				step--;
			}
		}
	}
	
	std::vector<uvec2> getEntities() {
		return entities;
	}

	bool getGoalReached() {
		return goalReached;
	}

	int mapIdx(int x, int y) {
		return dims.x * y + x;
	}

	int mapSize = 0;
	int entitiesSize = 0;
	std::vector<uvec2> entities;
	unsigned int* origMap;

	uvec2 goal;

	bool finished = false;
	int numComputes = 0;
};