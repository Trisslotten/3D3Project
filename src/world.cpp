#include "world.h" 
#include <math.h>
#include "lodepng/lodepng.h"

World::World() {

}

World::~World() {
	delete origMap;
}

void World::init(unsigned int width, unsigned int height, unsigned int entityCount) {
	dims.x = width; dims.y = height;
	int sqFr = sqrt(entityCount);

	vec2 fStart(0, 0);

	for (int x = fStart.x; x < fStart.x +sqFr; x++) {
		for (int y = fStart.y; y < fStart.y + sqFr; y++) {
			entities.push_back(Entity(x, y, TEAM_FRIENDLY));
		}
	}
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
	origMap = new unsigned char[width*height];
	mapSize = width * height * sizeof(unsigned char);
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

	int sqFr = sqrt(entityCount);

	for (int x = 0; x < sqFr; x++) {
		for (int y = 0; y < sqFr; y++) {
			entities.push_back(Entity(x, y, TEAM_FRIENDLY));
		}
	}
}