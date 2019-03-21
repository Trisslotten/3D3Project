#include "world.h" 
#include <math.h>

World::World() {

}

World::~World() {

}

void World::init(unsigned int width, unsigned int height, unsigned int friendlyCount, unsigned int enemyCount) {
	gridDims.x = width; gridDims.y = height;

	int sqFr = sqrt(friendlyCount);
	int sqEn = sqrt(enemyCount);

	vec2 fStart(0, 0);
	vec2 eStart(width - sqEn, height - sqEn);

	for (int x = fStart.x; x < fStart.x +sqFr; x++) {
		for (int y = fStart.y; y < fStart.y + sqFr; y++) {
			entities.push_back(Entity(x, y, TEAM_FRIENDLY));
		}
	}

	for (int x = eStart.x; x < eStart.x + sqEn; x++) {
		for (int y = eStart.y; y < eStart.y + sqEn; y++) {
			entities.push_back(Entity(x, y, TEAM_ENEMY));
		}
	}
}