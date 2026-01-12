#ifndef BIOMENOISE_H
#define BIOMENOISE_H

#include <vector>

class BiomeNoise
{

public:
    enum Biome {
        GRASSLAND,
        MOUNTAIN
    };

    static void setSeed(unsigned int s);
    static Biome getBiomeAt(int x, int z);
    static int getHeightAt(int x, int z);

    static float perlin3D(float x, float y, float z);

private:
    static int biomeFloor;
    static unsigned int seed;
    static float biomeTransitionThreshold;

    static float pseudoRandom(int x, int z);

    static float getGrasslandHeight(int x, int z);
    static float getMountainHeight(int x, int z);
    static bool isTransition(int x, int z);

    static float basic(int x, int y);
    static float smooth(float x, float y);

    static std::vector<int> generatePermutation();
    static float perlin(float x, float y);
    static float fractal(float x, float y, int octaves, float persistence);

    static float fade(float t);
    static float grad(int hash, float x, float y);
    static float lerp(float t, float a, float b);

    static float grad3D(int hash, float x, float y, float z);
};

#endif // BIOMENOISE_H
