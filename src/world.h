#pragma once
#include <vector>
#include "entity.h"

struct Pixel {
	unsigned char r, g, b, a;
};

class World {
private:
	
	vec2 dims;
public:
	World();
	~World();

	void init(unsigned int width, unsigned int height, unsigned int entityCount);
	
	void init(std::string filename, unsigned int entityCount);
	
	void addEntity(vec2 pos, unsigned int team) {
		Entity ent(pos.x, pos.y, team);
		entities.push_back(ent);
	}

	void printEntities() {
		for (Entity ent : entities) {
			printf("entity at: %d %d\n", ent.pos.x, ent.pos.y);
		}
	}


	vec2 getMapDims() const
	{
		return dims;
	}
	unsigned char* getMap() const
	{
		return origMap;
	}
	
	std::vector<Entity> getEntities() {
		return entities;
	}

	int mapIdx(int x, int y) {
		return dims.x * y + x;
	}

	int mapSize = 0;
	int entitiesSize = 0;
	std::vector<Entity> entities;
	unsigned char* origMap;
};