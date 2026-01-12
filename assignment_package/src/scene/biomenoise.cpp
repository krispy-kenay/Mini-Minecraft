#include "biomenoise.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

float BiomeNoise::biomeTransitionThreshold = 0.01f;
unsigned int BiomeNoise::seed = 1;
int BiomeNoise::biomeFloor = 145;
void BiomeNoise::setSeed(unsigned int s) {
    seed = s;
    srand(seed);
}

float BiomeNoise::pseudoRandom(int x, int z) {
    unsigned int n = x * 123456789 + z * 987654321 + seed * 144630960;
    n = (n ^ (n >> 13)) * 1274126177;
    n = n ^ (n >> 16);
    return (n % 1000) / 1000.0f;
}

// determines if within threshold transitioning between biomes
bool BiomeNoise::isTransition(int x, int z) {
    float value = fabs(perlin(x * 0.005f, z * 0.005f));
    return value < biomeTransitionThreshold && value > -biomeTransitionThreshold;
}

BiomeNoise::Biome BiomeNoise::getBiomeAt(int x, int z) {
    float noiseValue = perlin(x * 0.005f, z * 0.005f);

    if (noiseValue > biomeTransitionThreshold) {
        return MOUNTAIN;
    } else if (noiseValue < -biomeTransitionThreshold) {
        return GRASSLAND;
    }

    // in between
    float value = pseudoRandom(x, z);
    if (value > 0.5f) {
        return MOUNTAIN;
    }
    return GRASSLAND;
}

float BiomeNoise::getGrasslandHeight(int x, int z) {
    return biomeFloor + 30.0f * fractal(x * 0.01f, z * 0.01f, 4, 0.5f);
}

float BiomeNoise::getMountainHeight(int x, int z) {
    return biomeFloor + 115.0f * fractal(x * 0.02f, z * 0.02f, 4, 0.5f);
}

int BiomeNoise::getHeightAt(int x, int z) {
    float grasslandHeight = getGrasslandHeight(x, z);
    float mountainHeight = getMountainHeight(x, z);
    float height = grasslandHeight; // default grassland

    Biome b = getBiomeAt(x, z);
    if (isTransition(x, z)) {
        float t = fabs(smooth(x * 0.005f, z * 0.005f));
        height = lerp(t, grasslandHeight, mountainHeight);
    } else if (b == MOUNTAIN) {
        height = mountainHeight;
    }

    if (height > 255) {
        return 255;
    } else if (height < 0) {
        return 0;
    }
    return round(height);
}

float BiomeNoise::fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float BiomeNoise::lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float BiomeNoise::grad(int hash, float x, float y) {
    int h = hash & 3;
    float u = h & 1 ? x : -x;
    float v = h & 2 ? y : -y;
    return u + v;
}

float BiomeNoise::basic(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

float BiomeNoise::smooth(float x, float y) {
    int intX = static_cast<int>(x);
    int intY = static_cast<int>(y);

    float fractionalX = x - intX;
    float fractionalY = y - intY;

    float v1 = basic(intX, intY);
    float v2 = basic(intX + 1, intY);
    float v3 = basic(intX, intY + 1);
    float v4 = basic(intX + 1, intY + 1);

    float i1 = lerp(fractionalX, v1, v2);
    float i2 = lerp(fractionalX, v3, v4);

    return lerp(fractionalY, i1, i2);
}


std::vector<int> BiomeNoise::generatePermutation() {
    std::vector<int> permutation(256);
    for (int i = 0; i < 256; ++i) {
        permutation[i] = i;
    }

    // use seed
    std::default_random_engine engine(seed);
    std::shuffle(permutation.begin(), permutation.end(), engine);

    return permutation;
}

float BiomeNoise::perlin(float x, float y) {
    int X = static_cast<int>(floor(x)) & 255;
    int Y = static_cast<int>(floor(y)) & 255;

    x -= floor(x);
    y -= floor(y);

    float u = fade(x);
    float v = fade(y);

    // get permutation from seed
    std::vector<int> permutation = generatePermutation();
    std::vector<int> p(permutation.begin(), permutation.end());
    p.insert(p.end(), permutation.begin(), permutation.end());

    int A = p[X] + Y;
    int B = p[X + 1] + Y;

    return lerp(v,
                lerp(u, grad(p[A], x, y),
                     grad(p[B], x - 1, y)),
                lerp(u, grad(p[A + 1], x, y - 1),
                     grad(p[B + 1], x - 1, y - 1)));
}

float BiomeNoise::fractal(float x, float y, int octaves, float persistence) {
    float total = 0.0f;
    float maxAmplitude = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; i++) {
        total += perlin(x * frequency, y * frequency) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxAmplitude;
}

float BiomeNoise::perlin3D(float x, float y, float z) {
    int X = static_cast<int>(floor(x)) & 255;
    int Y = static_cast<int>(floor(y)) & 255;
    int Z = static_cast<int>(floor(z)) & 255;

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    std::vector<int> permutation = generatePermutation();
    std::vector<int> p(permutation.begin(), permutation.end());
    p.insert(p.end(), permutation.begin(), permutation.end());

    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return lerp(w,
                lerp(v,
                     lerp(u, grad3D(p[AA], x, y, z), grad3D(p[BA], x - 1, y, z)),
                     lerp(u, grad3D(p[AB], x, y - 1, z), grad3D(p[BB], x - 1, y - 1, z))),
                lerp(v,
                     lerp(u, grad3D(p[AA + 1], x, y, z - 1), grad3D(p[BA + 1], x - 1, y, z - 1)),
                     lerp(u, grad3D(p[AB + 1], x, y - 1, z - 1), grad3D(p[BB + 1], x - 1, y - 1, z - 1))));
}

float BiomeNoise::grad3D(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);

    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}


