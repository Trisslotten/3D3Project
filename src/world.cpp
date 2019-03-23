#include "world.h" 
#include <math.h>
#include "lodepng/lodepng.h"

World::World() {

}

World::~World() {
	delete origMap;
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

	for (int i = 0; i < entityCount; i++) {
		vec2 pos(rand() % width, rand() % height);
		entities.push_back(Entity(pos.x, pos.y));
	}
}