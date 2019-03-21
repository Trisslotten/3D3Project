#pragma once
#include <vector>
#include "entity.h"

class World {
private:
	std::vector<Entity> entities;
	vec2 gridDims;
public:
	World();
	~World();

	void init(unsigned int width, unsigned int height, unsigned int friendlyCount, unsigned int enemyCount);
	void addEntity(vec2 pos, unsigned int team) {
		Entity ent(pos.x, pos.y, team);
		entities.push_back(ent);
	}

	void printEntities() {
		for (Entity ent : entities) {
			printf("entity at: %d %d\n", ent.pos.x, ent.pos.y);
		}
	}
	
	std::vector<Entity> getEntities() {
		return entities;
	}
};