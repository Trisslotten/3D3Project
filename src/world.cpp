#include "world.h" 
#include <math.h>
#include "lodepng/lodepng.h"

World::World() {

}

World::~World() {
	delete origMap;
}

void World::setNewGoal() {
	goal = uvec2(rand() % dims.x, rand() % dims.y);
	//goal = uvec2(3, 4);
	while (origMap[mapIdx(goal.x, goal.y)] == 1) {
		goal = uvec2(rand() % dims.x, rand() % dims.y);
	}
	//goal = uvec2(5, 1);
}

void World::updateEntities() {
	bool didSomething = false;
	if (stepsCount > 0) {
		for (int e = 0; e < entities.size(); e++) {
			int stepIdx = stepsCount - emptySteps[e];
			if (stepIdx >= 0) {
				ivec2 step = steps[e*entities.size() + stepIdx];
				entities[e].move(step);
				didSomething = true;
				
				if (entities[e].pos.x == goal.x && entities[e].pos.y == goal.y) {
					goalReached = true;
					printf("Goal reached!\n");
					setNewGoal();
				}
			}
		}
		stepsCount--;
	}
	finished = !didSomething;
}

void World::init(std::string filename, unsigned int entityCount) {
	std::vector<unsigned char> image; //the raw pixels
	unsigned width, height;

	//decode
	unsigned error = lodepng::decode(image, width, height, filename, LCT_RGBA);
	Pixel* pixels = reinterpret_cast<Pixel*>(image.data());

	


	if (error) {
		printf("[lodepng] Failed to load map.\n");
		return;
	}

	printf("[World] Loaded map with dimensions: %d x %d \n", width, height);

	dims.x = width; dims.y = height;
	origMap = new unsigned int[width*height];
	mapSize = width * height * sizeof(unsigned int);
	entitiesSize = entityCount * sizeof(Entity);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = mapIdx(x, y);
			if (pixels[idx].r == 255) {
				origMap[idx] = 1;
			}
			else {
				origMap[idx] = 0;
			}
			printf("%d", origMap[idx]);
		}
		printf("\n");
	}

	for (int i = 0; i < entityCount; i++) {
		uvec2 pos(rand() % width, rand() % height);
		while (origMap[mapIdx(goal.x, goal.y)] == 1) {
			pos = uvec2(rand() % width, rand() % height);
		}
		entities.push_back(Entity(pos.x, pos.y));
		//entities.push_back(Entity(1, 1));
	}
	emptySteps = new unsigned int[entities.size()];
	
	setNewGoal();
	printf("Goal at: %d %d\n", goal.x, goal.y);
}