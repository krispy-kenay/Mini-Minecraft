#include "terrain.h"
#include <stdexcept>
#include "rivers.h"
#include <iostream>

int floorDiv(int a, int b) {
    int div = a / b;
    int rem = a % b;
    if ((rem != 0) && ((a < 0) != (b < 0))) {
        div--;
    }
    return div;
}

int mod(int a, int b) {
    int result = a % b;
    if (result < 0) {
        result += std::abs(b);
    }
    return result;
}

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context),
    m_allRivers({
        Rivers( // default first river at spawn for demo
                "F",
                {{'F', "FF-[-F+F+F]+[+F-F-F]"}},
                3,      // iterations
                22.5,   // angle
                9.0,    // step size
                48.0,   // startX
                48.0    // startZ
                )
    })
{}

Terrain::~Terrain() {
    // TODO?
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
    
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

void Terrain::createNewRiver(int zoneX, int zoneZ) {
    double startX = (double)(zoneX + 1 + ( std::rand() % ( (zoneX + 62) - (zoneX + 1) + 1 ) ));
    double startZ = (double)(zoneZ + 1 + ( std::rand() % ( (zoneZ + 62) - (zoneZ + 1) + 1 ) ));

    Rivers newRiver("F",
                    {{'F', "FF-[-F+F+F]+[+F-F-F]"}},
                    (int)(2 + std::rand() % (4 - 2 + 1)),           // iterations
                    (double)(20 + std::rand() % (30 - 20 + 1)),     // angle
                    (double)(5 + std::rand() % (15 - 5 + 1)),       // step size
                    startX,
                    startZ
                    );

    m_allRivers.push_back(std::move(newRiver));
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getGlobalBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        // Check the atomic flag of the chunk to see if the block data has been generated
        if (c->hasBlockData()) {
            glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
            return c->getLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                                      static_cast<unsigned int>(y),
                                      static_cast<unsigned int>(z - chunkOrigin.y));
        } else {
            // Not yet sure what we should return if there is no block data,
            // on one hand, returning EMPTY seems sensible,
            // but if a player runs into a chunk with no block data,
            // they would start falling down, so returning a solid block
            // like STONE might be better so that any movement into the chunk is blocked
            // while its generating (and in case the player teleports into a chunk with no block data,
            // they appear stuck in place until the chunk finishes generating)
            return STONE;
        }
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getGlobalBlockAt(glm::vec3 p) const {
    return getGlobalBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setGlobalBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        if (c->hasBlockData()) {
            c->setLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                           static_cast<unsigned int>(y),
                           static_cast<unsigned int>(z - chunkOrigin.y),
                           t);
        }
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context, x, z, &m_allRivers);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

// Draws each Chunk with the given ShaderProgram
void Terrain::draw(const glm::vec3 &playerPosition, ShaderProgram *shaderProgram, ShaderProgram *shaderProgramBlinnPhong, const Camera& camera) {
    const float LOD1_DISTANCE = 64.0f;  // Medium detail
    const float LOD2_DISTANCE = 128.0f; // Low detail
    const float MAX_VIEW_DISTANCE = 256.0f; // Maximum render distance

    // Store the pointers to chunks to be rendered
    // we need to do this so that every chunk will have the correct LOD
    // when we draw them (otherwise, one chunk might be drawn before we 
    // realize that its neighbor has a lower LOD)
    std::vector<Chunk*> chunksToDraw;
    // Store the pointers to chunks whose VBO can be purged
    // to save on GPU memory, this does NOT unload the chunk from regular memory,
    // this will be handled in the `generate` function (since we want seperation of concerns)
    std::vector<Chunk*> chunksToDestroy;
    for (auto& chunkEntry : m_chunks) {
        // Get the pointer to the chunk
        Chunk* chunk = chunkEntry.second.get();

        // Entirely ignore chunks that have not yet been released from the
        // blocktypeworker thread
        if (!chunk->hasBlockData()) {
            continue;
        }

        // Calculate the distance from the player to the chunk
        glm::vec2 chunkCenter = chunk->getCenter();
        float distance = glm::distance(chunkCenter, glm::vec2(playerPosition.x, playerPosition.z));

        // Check if the chunk is within the maximum view distance
        if (distance < MAX_VIEW_DISTANCE) {
            // Check if the chunk is in view
            if (!chunk->isInView(camera)) {
                continue;
            }

            // Update LOD level if necessary
            int lodLevel = 0;
            if (distance > LOD2_DISTANCE) {
                lodLevel = 2;
            } else if (distance > LOD1_DISTANCE) {
                lodLevel = 1;
            }

            // Update the LOD of the chunk
            chunk->setLevelOfDetail(lodLevel);
            // Add the chunk to the list of chunks to draw
            chunksToDraw.push_back(chunk);
        } else {
            // Add the chunk to the list of chunks to destroy
            chunksToDestroy.push_back(chunk);
        }
    }
    // For clarity, we split the process into three parts
    // First, generate the VBO data for the chunk in a separate thread
    // when an update is requested
    for (Chunk* chunk : chunksToDraw) {
        if (chunk->needsUpdate()) {
            VBOWorker* worker = new VBOWorker(chunk);
            QThreadPool::globalInstance()->start(worker);
        }
    }
    // Second, buffer the vertex data to the GPU
    for (Chunk* chunk : chunksToDraw) {
        if (chunk->hasVBOData()) {
            chunk->bufferVertexData();
        }
    }
    // Third, draw the chunks that have been buffered
    for (Chunk* chunk : chunksToDraw) {
        if (chunk->hasGPUData()) {
            chunk->draw(shaderProgram);
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    for (Chunk* chunk : chunksToDraw) {
        if (chunk->hasGPUData()) {
            chunk->drawTransparent(shaderProgramBlinnPhong);
        }
    }

    glCullFace(GL_FRONT);
    for (Chunk* chunk : chunksToDraw) {
        if (chunk->hasGPUData()) {
            chunk->drawTransparent(shaderProgramBlinnPhong);
        }
    }

    glDisable(GL_CULL_FACE);

    // Destroy the chunks VBO data
    for (Chunk* chunk : chunksToDestroy) {
        chunk->destroyVBOdata();
        chunk->setHasGPUData(false);
    }
}

// Generate chunks in zones around the player
void Terrain::generate(const glm::vec3 &playerPosition) {
    // Get the players zone coordinates
    int playerZoneX = static_cast<int>(std::floor(playerPosition.x / 64.f));
    int playerZoneZ = static_cast<int>(std::floor(playerPosition.z / 64.f));

    int generationDistance = 1;
    // Identical set to the generatedTerrain set
    // from which we will remove the zones that are within loading distance
    std::unordered_set<int64_t> zonesToUnload = m_generatedTerrain;

    // Iterate over the zones around the player
    for (int zoneX = playerZoneX - generationDistance; zoneX <= playerZoneX + generationDistance; ++zoneX) {
        for (int zoneZ = playerZoneZ - generationDistance; zoneZ <= playerZoneZ + generationDistance; ++zoneZ) {
            // Get the key for the zone
            int64_t key = toKey(zoneX, zoneZ);
            zonesToUnload.erase(key);
            // Check if the zone has already been generated
            if (m_generatedTerrain.find(key) == m_generatedTerrain.end()) {
                m_generatedTerrain.insert(key);

                // Check if there is a zone file for this zone
                if (zoneFileExists(zoneX, zoneZ)) {
                    // If there is a zone file, load the zone
                    SaveLoadWorker* worker = new SaveLoadWorker(this, zoneX, zoneZ, false);
                    worker->setAutoDelete(true);
                    QThreadPool::globalInstance()->start(worker);
                } else {

                    // 1 in 10 chance of creating river system in a zone.
                    if (rand() % 10 == 0) {
                        createNewRiver(zoneX, zoneZ);
                    }

                    // If the zone hasn't been generated, iterate over the chunks in the zone
                    for (int x = zoneX * 64; x < zoneX * 64 + 64; x += 16) {
                        for (int z = zoneZ * 64; z < zoneZ * 64 + 64; z += 16) {
                            // Check if the chunk has already been generated
                            // (theoretically, this shouldn't happen, but it adds 
                            // an additional failsafe)
                            if (!hasChunkAt(x, z)) {
                                // Create the new chunk
                                Chunk* chunk = instantiateChunkAt(x, z);
                                // Create a new worker thread to generate the block data for this chunk
                                BlockTypeWorker* worker = new BlockTypeWorker(chunk);
                                worker->setAutoDelete(true);
                                QThreadPool::globalInstance()->start(worker);
                            }
                        }
                    }
                }   
            }
        }
    }
}

void Terrain::unloadZone(int zoneX, int zoneZ) {
    // Remove the chunks of this zone from m_chunks
    for (int x = zoneX * 64; x < zoneX * 64 + 64; x += 16) {
        for (int z = zoneZ * 64; z < zoneZ * 64 + 64; z += 16) {
            if (hasChunkAt(x, z)) {
                m_chunks.erase(toKey(x, z));
            }
        }
    }
    // Remove the zone from m_generatedTerrain
    m_generatedTerrain.erase(toKey(zoneX, zoneZ));
}

bool Terrain::zoneFileExists(int zoneX, int zoneZ) {
    int regionX = floorDiv(zoneX, 4);
    int regionZ = floorDiv(zoneZ, 4);

    std::string regionFolder = m_worldFolder + "/Region_" + std::to_string(regionX) + "_" + std::to_string(regionZ);
    std::string zoneFile = regionFolder + "/Zone_" + std::to_string(zoneX) + "_" + std::to_string(zoneZ) + ".dat";
    QFile file(QString::fromStdString(zoneFile));
    return file.exists();
}

void Terrain::saveZone(int zoneX, int zoneZ) {
    int regionX = floorDiv(zoneX, 4);
    int regionZ = floorDiv(zoneZ, 4);

    std::string regionFolder = m_worldFolder + "/Region_" + std::to_string(regionX) + "_" + std::to_string(regionZ);
    std::string zoneFile = regionFolder + "/Zone_" + std::to_string(zoneX) + "_" + std::to_string(zoneZ) + ".dat";

    // Create directories if they don't exist
    QDir dir(QString::fromStdString(regionFolder));
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    std::ofstream ofs(zoneFile, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open file for saving: " << zoneFile << std::endl;
        return;
    }

    for (int chunkX = zoneX * 64; chunkX < zoneX * 64 + 64; chunkX += 16) {
        for (int chunkZ = zoneZ * 64; chunkZ < zoneZ * 64 + 64; chunkZ += 16) {
            if (hasChunkAt(chunkX, chunkZ)) {
                Chunk* chunk = getChunkAt(chunkX, chunkZ).get();
                // Save the chunk only if its block data has been generated
                if (chunk->hasBlockData()) {
                    int chunkIndexX = floorDiv(chunkX, 16);
                    int chunkIndexZ = floorDiv(chunkZ, 16);

                    uint8_t localChunkX = mod(chunkIndexX, 4);
                    uint8_t localChunkZ = mod(chunkIndexZ, 4);

                    ofs.put(static_cast<char>(localChunkX));
                    ofs.put(static_cast<char>(localChunkZ));
                    chunk->serializeModifiedBlocks(ofs);
                }
            }
        }
    }
    ofs.close();
}

void Terrain::loadZone(int zoneX, int zoneZ) {
    int regionX = floorDiv(zoneX, 4);
    int regionZ = floorDiv(zoneZ, 4);

    std::string regionFolder = m_worldFolder + "/Region_" + std::to_string(regionX) + "_" + std::to_string(regionZ);
    std::string zoneFile = regionFolder + "/Zone_" + std::to_string(zoneX) + "_" + std::to_string(zoneZ) + ".dat";

    std::ifstream ifs(zoneFile, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open file for loading: " << zoneFile << std::endl;
        return;
    }

    while (ifs.peek() != EOF) {
        uint8_t localChunkX = ifs.get();
        uint8_t localChunkZ = ifs.get();

        int chunkX = zoneX * 4 + localChunkX;
        int chunkZ = zoneZ * 4 + localChunkZ;

        Chunk* chunk = nullptr;

        if (hasChunkAt(chunkX * 16, chunkZ * 16)) {
            chunk = getChunkAt(chunkX * 16, chunkZ * 16).get();
        } else {
            chunk = instantiateChunkAt(chunkX * 16, chunkZ * 16);
        }
        chunk->setHasBlockData(false);
        chunk->setHasGPUData(false);
        chunk->setHasVBOData(false);
        chunk->destroyVBOdata();
        // Generate the chunk
        chunk->generate();
        // Deserialize modified blocks
        chunk->deserializeModifiedBlocks(ifs);
        // Mark the chunk as having block data generated
        chunk->setHasBlockData(true);
        chunk->setNeedsUpdate(true);
    }

    ifs.close();
}

void Terrain::saveTerrain() {
    for (const auto& zoneKey : m_generatedTerrain) {
        glm::ivec2 zoneCoords = toCoords(zoneKey);
        std::cout << "Saving zone at " << zoneCoords.x << ", " << zoneCoords.y << std::endl;
        SaveLoadWorker* worker = new SaveLoadWorker(this, zoneCoords.x, zoneCoords.y, true);
        QThreadPool::globalInstance()->start(worker);
    }
}
