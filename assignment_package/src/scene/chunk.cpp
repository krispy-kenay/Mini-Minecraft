#include "chunk.h"
#include <iostream>

// The size of a chunk in blocks
// used as an external constant static
// so that if we ever want to change the
// chunk size, we only have to change it here
// (and in Terrain)
const static int chunkXLength = 16;
const static int chunkYLength = 256;
const static int chunkZLength = 16;

const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

// Cube vertex data for a single face
const std::unordered_map<Direction, std::vector<glm::vec4>> faceVertices = {
    {XPOS, { glm::vec4(1.f,0.f,0.f,1.f), glm::vec4(1.f,1.f,0.f,1.f), glm::vec4(1.f,1.f,1.f,1.f), glm::vec4(1.f,0.f,1.f,1.f) }},
    {XNEG, { glm::vec4(0.f,0.f,1.f,1.f), glm::vec4(0.f,1.f,1.f,1.f), glm::vec4(0.f,1.f,0.f,1.f), glm::vec4(0.f,0.f,0.f,1.f) }},
    {YPOS, { glm::vec4(0.f,1.f,0.f,1.f), glm::vec4(1.f,1.f,0.f,1.f), glm::vec4(1.f,1.f,1.f,1.f), glm::vec4(0.f,1.f,1.f,1.f) }},
    {YNEG, { glm::vec4(0.f,0.f,1.f,1.f), glm::vec4(1.f,0.f,1.f,1.f), glm::vec4(1.f,0.f,0.f,1.f), glm::vec4(0.f,0.f,0.f,1.f) }},
    {ZPOS, { glm::vec4(0.f,0.f,1.f,1.f), glm::vec4(1.f,0.f,1.f,1.f), glm::vec4(1.f,1.f,1.f,1.f), glm::vec4(0.f,1.f,1.f,1.f) }},
    {ZNEG, { glm::vec4(1.f,0.f,0.f,1.f), glm::vec4(0.f,0.f,0.f,1.f), glm::vec4(0.f,1.f,0.f,1.f), glm::vec4(1.f,1.f,0.f,1.f) }}
};

// Used to map local overflowing chunk coordinates to neighbor chunk coordinates
// So -1 -> 15 while 16 -> 0
int wrap_value(int value, int min, int max) {
    int range = max - min + 1;
    return (value % range + range) % range + min;
}

Chunk::Chunk(OpenGLContext* context, int x, int z, const std::vector<Rivers>* rivers) : mp_riversList(rivers), Drawable(context), m_blocks(), minX(x), minZ(z), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}, m_needsUpdate(true), m_levelOfDetail(2), m_hasBlockData(false), m_hasVBOData(false), m_hasGPUData(false)
{
    std::fill_n(m_blocks.begin(), chunkXLength*chunkYLength*chunkZLength, EMPTY);
}

Chunk::~Chunk() {
    // Clear vectors
    m_vertexDataOpaque.clear();
    m_indicesOpaque.clear();
    m_vertexDataTransparent.clear();
    m_indicesTransparent.clear();
    
    // Clear neighbor map
    m_neighbors.clear();
    
    // Clear VBO/VAO data through parent Drawable class
    destroyVBOdata();
}

// Does bounds checking with at()
BlockType Chunk::getLocalBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + chunkXLength * y + chunkZLength * chunkYLength * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getLocalBlockAt(int x, int y, int z) const {
     return getLocalBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setLocalBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blockDataMutex.lock();
    m_blocks.at(x + chunkXLength * y + chunkZLength * chunkYLength * z) = t;
    m_needsUpdate = true; // Update VBO data if the block changes

    if (x == 0 && m_neighbors[XNEG]) {
        m_neighbors[XNEG]->m_needsUpdate = true;
    }
    if (x == chunkXLength - 1 && m_neighbors[XPOS]) {
        m_neighbors[XPOS]->m_needsUpdate = true;
    }
    if (z == 0 && m_neighbors[ZNEG]) {
        m_neighbors[ZNEG]->m_needsUpdate = true;
    } 
    if (z == chunkZLength - 1 && m_neighbors[ZPOS]) {
        m_neighbors[ZPOS]->m_needsUpdate = true;
    }
    m_blockDataMutex.unlock();
}

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

// Map block type to color
glm::vec4 Chunk::getBlockColor(BlockType block) {
    glm::vec4 color;
    switch (block) {
        case GRASS:
            color = glm::vec4(95.f, 159.f, 53.f, 255.f) / 255.f;
            break;
        case DIRT:
            color = glm::vec4(121.f, 85.f, 58.f, 255.f) / 255.f;
            break;
        case STONE:
            color = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
            break;
        case WATER:
            color = glm::vec4(0.f, 0.f, 0.75f, 1.f);
            break;
        case SNOW:
            color = glm::vec4(1.f, 1.f, 1.f, 1.f);
            break;
        case BEDROCK:
            color = glm::vec4(2.f, 159.f, 53.f, 255.f) / 255.f;
            break;
        case LAVA:
            color = glm::vec4(0.75f, 0.f, 0.f, 1.f);
            break;
        default:
            // Other block types are not yet handled, so we default to debug purple
            color = glm::vec4(1.f, 0.f, 1.f, 1.f);
            break;
    }
    return color;
}

// Build the VBO data for this Chunk
void Chunk::createVBOdata() {
    // Lock the block data to prevent concurrent modification
    m_blockDataMutex.lock();

    // Setup vector for the buffer
    std::vector<Vertex> vertexDataOpaque;
    std::vector<GLuint> indicesOpaque;

    std::vector<Vertex> vertexDataTransparent;
    std::vector<GLuint> indicesTransparent;

    // Determine the block size to draw based on the level of detail
    int blockSize = std::pow(2, m_levelOfDetail);
    int blockSizeY = std::clamp(blockSize / 2, 1, chunkYLength);
    // Iterate over all block in the chunk in step sizes of blockSize
    for (unsigned int x = 0; x < chunkXLength; x += blockSize) {
        for (unsigned int y = 0; y < chunkYLength; y += blockSizeY) {
            for (unsigned int z = 0; z < chunkZLength; z += blockSize) {
                BlockType block = determineBlockTypeForArea(x, y, z, blockSize);
                if (block != EMPTY) {
                    generateBlockGeometry(x, y, z, block, blockSize, vertexDataOpaque, indicesOpaque, vertexDataTransparent, indicesTransparent);
                }
            }
        }
    }
    
    m_blockDataMutex.unlock();

    // Lock the VBO data to prevent concurrent modification
    m_VBODataMutex.lock();
    // (I'm like 95% sure we don't need this because we have the atomic flag...)
    m_vertexDataOpaque = std::move(vertexDataOpaque);
    m_indicesOpaque = std::move(indicesOpaque);
    m_vertexDataTransparent = std::move(vertexDataTransparent);
    m_indicesTransparent = std::move(indicesTransparent);
    m_VBODataMutex.unlock();
}

void Chunk::bufferVertexData() {
    // If there is no VBO data to be buffered, skip
    if (!m_hasVBOData) {
        return;
    }
    // Lock the VBO data to prevent concurrent modification
    // (I'm like 95% sure we don't need this because we have the atomic flag...)
    m_VBODataMutex.lock();
    // Move interleaved data to GPU
    generateBuffer(INTERLEAVED);
    bindBuffer(INTERLEAVED);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vertexDataOpaque.size() * sizeof(Vertex), m_vertexDataOpaque.data(), GL_STATIC_DRAW);
    // Move index data to GPU
    generateBuffer(INDEX);
    bindBuffer(INDEX);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indicesOpaque.size() * sizeof(GLuint), m_indicesOpaque.data(), GL_STATIC_DRAW);
    indexCounts[INDEX] = static_cast<int>(m_indicesOpaque.size());
    // Move interleaved data to GPU
    generateBuffer(INTERLEAVED_TRANSPARENT);
    bindBuffer(INTERLEAVED_TRANSPARENT);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vertexDataTransparent.size() * sizeof(Vertex), m_vertexDataTransparent.data(), GL_STATIC_DRAW);
    // Move index data to GPU
    generateBuffer(INDEX_TRANSPARENT);
    bindBuffer(INDEX_TRANSPARENT);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indicesTransparent.size() * sizeof(GLuint), m_indicesTransparent.data(), GL_STATIC_DRAW);
    indexCounts[INDEX_TRANSPARENT] = static_cast<int>(m_indicesTransparent.size());

    // Free up memory by clearing the VBO data in RAM
    m_vertexDataOpaque.clear();
    m_indicesOpaque.clear();
    m_vertexDataTransparent.clear();
    m_indicesTransparent.clear();
    // And set the corresponding flag
    m_hasVBOData = false;
    m_hasGPUData = true;
    // Unlock the VBO data again once we copied the data to the GPU
    m_VBODataMutex.unlock();

}

// Draw mode override, not really needed since 
// it's the same as default, but why not
GLenum Chunk::drawMode() {
    return GL_TRIANGLES;
}

void Chunk::addUVHelper(std::vector<Vertex>& faceCorners, int x, int y, Direction direction) {
    float block_size_UV = 1.0f / 16.0f;
    float u_min, v_min, u_max, v_max;

    u_min = x * block_size_UV;
    v_min = 1.0f - (y + 1) * block_size_UV;
    u_max = u_min + block_size_UV;
    v_max = 1.0f - y * block_size_UV;

    if (direction == YPOS) {
        faceCorners[0].uv = glm::vec2(u_max, v_min); // Bottom-right
        faceCorners[1].uv = glm::vec2(u_max, v_max); // Top-right
        faceCorners[2].uv = glm::vec2(u_min, v_max); // Top-left
        faceCorners[3].uv = glm::vec2(u_min, v_min); // Bottom-left
    } else if (direction == YNEG) {
        faceCorners[0].uv = glm::vec2(u_min, v_min); // Bottom-left
        faceCorners[1].uv = glm::vec2(u_max, v_min); // Bottom-right
        faceCorners[2].uv = glm::vec2(u_max, v_max); // Top-right
        faceCorners[3].uv = glm::vec2(u_min, v_max); // Top-left
    } else if (direction == ZPOS || direction == ZNEG) {
        faceCorners[0].uv = glm::vec2(u_min, v_min); // Bottom-left
        faceCorners[1].uv = glm::vec2(u_max, v_min); // Bottom-right
        faceCorners[2].uv = glm::vec2(u_max, v_max); // Top-right
        faceCorners[3].uv = glm::vec2(u_min, v_max); // Top-left
    } else {
        faceCorners[0].uv = glm::vec2(u_max, v_min); // Bottom-right
        faceCorners[1].uv = glm::vec2(u_max, v_max); // Top-right
        faceCorners[2].uv = glm::vec2(u_min, v_max); // Top-left
        faceCorners[3].uv = glm::vec2(u_min, v_min); // Bottom-left
    }
}

void Chunk::addUV(std::vector<Vertex>& faceCorners, Direction direction, BlockType type) {
    if (isOpaque(type)) {
        switch (type) {
        case GRASS:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 8, 2, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 2, 0, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 3, 0, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 3, 0, direction);
                break;
            }
            break;
        case DIRT:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 2, 0, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 2, 0, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 2, 0, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 2, 0, direction);
                break;
            }
            break;
        case STONE:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 1, 0, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 1, 0, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 1, 0, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 1, 0, direction);
                break;
            }
            break;
        case LAVA:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 15, 14, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 15, 14, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 15, 14, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 15, 14, direction);
                break;
            }
            break;
        case BEDROCK:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 1, 1, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 1, 1, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 1, 1, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 1, 1, direction);
                break;
            }
            break;
        case SNOW_DIRT:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 2, 4, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 4, 4, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 4, 4, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 4, 4, direction);
                break;
            }
            break;
        case SNOW:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 2, 4, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 2, 4, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 2, 4, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 2, 4, direction);
                break;
            }
            break;
        default:
            break;
        }
    } else {
        switch (type) {
        case WATER:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 15, 12, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 15, 12, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 15, 12, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 15, 12, direction);
                break;
            }
            break;
        case ICE:
            switch(direction) {
            case YPOS:
                addUVHelper(faceCorners, 3, 4, direction);
                break;
            case YNEG:
                addUVHelper(faceCorners, 3, 4, direction);
                break;
            case ZPOS:
            case ZNEG:
                addUVHelper(faceCorners, 3, 4, direction);
                break;
            case XPOS:
            case XNEG:
                addUVHelper(faceCorners, 3, 4, direction);
                break;
            }
            break;
        default:
            break;
        }
    }
}

bool Chunk::isOpaque(BlockType type) {
    if (type == EMPTY || type == WATER || type == ICE) {
        return false;
    }
    return true;
}

bool Chunk::isOpaqueOrLava(BlockType type) {
    if (type == EMPTY || type == WATER || type == ICE || type == LAVA) {
        return false;
    }
    return true;
}

bool Chunk::isAnimated(BlockType type) {
    if (type == LAVA || type == WATER) {
        return true;
    }
    return false;
}

// Draw the chunk
void Chunk::draw(ShaderProgram* shaderProgram) {
    if (!m_hasBlockData || !m_hasGPUData) {
        return;
    }
    shaderProgram->drawInterleaved(*this);
}

void Chunk::drawTransparent(ShaderProgram* shaderProgram) {
    if (!m_hasBlockData || !m_hasGPUData) {
        return;
    }
    shaderProgram->drawInterleavedTransparent(*this);
}

// Get the level of detail of this chunk
int Chunk::getLevelOfDetail() const {
    return m_levelOfDetail;

}

// Clear old VBO data if the LOD changes
// and set the flag to create new VBO data
void Chunk::setLevelOfDetail(int levelOfDetail) {
    if (levelOfDetail != m_levelOfDetail) {
        // Overwrite old VBO data
        m_levelOfDetail = levelOfDetail;
        m_needsUpdate = true;
        // Update the neighbors' LODs if they exist
        for (const auto& [direction, chunk] : m_neighbors) {
            if (chunk) {
                chunk->m_needsUpdate = true;
            }
        }
    }
}

// Determine the block type for a given area
BlockType Chunk::determineBlockTypeForArea(unsigned int startX, unsigned int startY, unsigned int startZ, int blockSize) {
    // Preallocate memory for block counts
    alignas(16) int blockCounts[MAX_BLOCK_TYPES] = { 0 };

    unsigned int blockSizeY = blockSize / 2;
    blockSizeY = (blockSizeY < 1) ? 1 : (blockSizeY > chunkYLength ? chunkYLength : blockSizeY);

    // Iterate over the area and count the occurrences of each block type
    // Ordered by z -> x -> y for better cache performance
    for (unsigned int z = startZ; z < startZ + blockSize && z < chunkZLength; ++z) {
        for (unsigned int x = startX; x < startX + blockSize && x < chunkXLength; ++x) {
            for (unsigned int y = startY; y < startY + blockSizeY && y < chunkYLength; ++y) {
                BlockType block = getLocalBlockAt(x, y, z);
                if (block != EMPTY) {
                    blockCounts[block]++;
                }
            }
        }
    }

    // Choose the most common block type
    BlockType predominantBlock = EMPTY;
    int maxCount = 0;
    for (int i = 0; i < MAX_BLOCK_TYPES; ++i) {
        if (blockCounts[i] > maxCount) {
            maxCount = blockCounts[i];
            predominantBlock = static_cast<BlockType>(i);
        }
    }

    return predominantBlock;
}

// Check whether a face should be rendered
// based on the block type and the LOD level
// and how the geometry should be generated
void Chunk::generateBlockGeometry(unsigned int x, unsigned int y, unsigned int z, BlockType block, int blockSize, std::vector<Vertex>& vertexDataOpaque, std::vector<GLuint>& indicesOpaque, std::vector<Vertex>& vertexDataTransparent, std::vector<GLuint>& indicesTransparent) {
    // Calculate the world position of the block
    glm::vec4 worldPos = glm::vec4(minX + static_cast<float>(x), static_cast<float>(y), minZ + static_cast<float>(z), 0.f);

    bool is_opaque = isOpaque(block);

    // For each face of the block
    for (auto direction : {XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG}) {
        // Check if the face should be rendered
        if (shouldRenderFace(x, y, z, direction, static_cast<int>(blockSize))) {
            // Get the vertices for this face, scaled by block size
            std::vector<glm::vec4> face = getFaceVertices(direction, blockSize);

            // Starting index for this face
            // GLuint baseIndex = static_cast<GLuint>(vertexData.size());
            GLuint baseIndex;
            if (is_opaque) {
                baseIndex = static_cast<GLuint>(vertexDataOpaque.size());
            } else {
                baseIndex = static_cast<GLuint>(vertexDataTransparent.size());
            }

            // Add vertices for this face
            std::vector<Vertex> faceCorners;
            for (const auto& offset : face) {
                Vertex vertex;
                vertex.position = worldPos + offset;
                vertex.position.w = 1.f;
                vertex.normal = getNormal(direction);
                // vertex.color = getBlockColor(block);
                // vertexData.push_back(vertex);

                faceCorners.push_back(vertex);
            }

            // Add indices for this face (two triangles)
            // In one go to avoid multiple push_back calls
            std::array<GLuint, 6> faceIndices = {
                baseIndex, baseIndex + 1, baseIndex + 2,
                baseIndex, baseIndex + 2, baseIndex + 3
            };

            if (is_opaque) {
                addUV(faceCorners, direction, block);

                // push vertices into corresponding vector
                for (Vertex corner: faceCorners) {
                    // Add animated flag
                    if (isAnimated(block)) {
                        corner.animated = 1.f;
                    }
                    vertexDataOpaque.push_back(corner);
                }

                indicesOpaque.insert(indicesOpaque.end(), faceIndices.begin(), faceIndices.end());
            } else {
                addUV(faceCorners, direction, block);

                // push vertices into corresponding vector
                for (Vertex corner: faceCorners) {
                    // Add animated flag
                    if (isAnimated(block)) {
                        corner.animated = 1.f;
                    }
                    vertexDataTransparent.push_back(corner);
                }

                indicesTransparent.insert(indicesTransparent.end(), faceIndices.begin(), faceIndices.end());
            }

            // indices.insert(indices.end(), faceIndices.begin(), faceIndices.end());
        }
    }
}

// Check if a face should be rendered
bool Chunk::shouldRenderFace(unsigned int x, unsigned int y, unsigned int z, Direction dir, int blockSize) {
    // Get the neighboring block
    int neighborX = x;
    int neighborY = y;
    int neighborZ = z;

    int blockSizeY = std::clamp(blockSize / 2, 1, chunkYLength);

    switch (dir) {
        case XPOS: neighborX += blockSize; break;
        case XNEG: neighborX -= blockSize; break;
        case YPOS: neighborY += blockSizeY; break;
        case YNEG: neighborY -= blockSizeY; break;
        case ZPOS: neighborZ += blockSize; break;
        case ZNEG: neighborZ -= blockSize; break;
    }

    bool is_opaque = isOpaqueOrLava(determineBlockTypeForArea(x, y, z, blockSize));

    // Check bounds and neighbor blocks
    if (neighborX < 0 || neighborX >= chunkXLength ||
        neighborY < 0 || neighborY >= chunkYLength ||
        neighborZ < 0 || neighborZ >= chunkZLength) {
        // Check area in the neighboring chunk
        Chunk* neighborChunk = m_neighbors[dir];
        if (neighborChunk) {
            // Neighbors with different levels of detail are prone
            // to annoying edge cases, so if the neighbor has a lower LOD,
            // we render all faces.
            // (In preliminary testing, the additional faces had neglible impact,
            // while actually performing the necessary checks slows the program down more
            // and is again very error prone)

            if (neighborChunk->m_levelOfDetail >= m_levelOfDetail) {
                if (is_opaque) {
                    return !isOpaqueOrLava(neighborChunk->determineBlockTypeForArea(wrap_value(neighborX, 0, 15), neighborY, wrap_value(neighborZ, 0, 15), blockSize));
                } else {
                    return neighborChunk->determineBlockTypeForArea(wrap_value(neighborX, 0, 15), neighborY, wrap_value(neighborZ, 0, 15), blockSize) == EMPTY;
                }
            } else {
                return true;
            }
        } else {
            return true;
        }
    } else {
        // For LOD > 0, we consider the entire area occupied by the larger block
        if (is_opaque) {
            return !isOpaqueOrLava(determineBlockTypeForArea(neighborX, neighborY, neighborZ, blockSize));
        } else {
            return determineBlockTypeForArea(neighborX, neighborY, neighborZ, blockSize) == EMPTY;
        }
    }
}

// Get the vertices scaled by block size
std::vector<glm::vec4> Chunk::getFaceVertices(Direction dir, int blockSize) {
    std::vector<glm::vec4> face;
    for (const auto& vertex : faceVertices.at(dir)) {
        glm::vec4 vert = vertex;
        vert.x *= blockSize;
        vert.y *= std::clamp(static_cast<float>(blockSize) / 2.f, 1.f, static_cast<float>(chunkYLength));
        vert.z *= blockSize;
        face.push_back(vert);
    }
    return face;
}

// Get the normal for a given direction
glm::vec4 Chunk::getNormal(Direction dir) {
    switch (dir) {
        case XPOS: return glm::vec4(1.f, 0.f, 0.f, 0.f);
        case XNEG: return glm::vec4(-1.f, 0.f, 0.f, 0.f);
        case YPOS: return glm::vec4(0.f, 1.f, 0.f, 0.f);
        case YNEG: return glm::vec4(0.f, -1.f, 0.f, 0.f);
        case ZPOS: return glm::vec4(0.f, 0.f, 1.f, 0.f);
        case ZNEG: return glm::vec4(0.f, 0.f, -1.f, 0.f);
        default:   return glm::vec4(0.f, 0.f, 0.f, 0.f);
    }
}

glm::vec2 Chunk::getCenter() const {
    return glm::vec2(minX + chunkXLength / 2, minZ + chunkZLength / 2);
}

bool Chunk::isInView(const Camera& camera) const {
    std::array<glm::vec4, 6> frustumPlanes = camera.getFrustumPlanes();

    glm::vec3 minPoint(minX, 0.f, minZ);
    glm::vec3 maxPoint(minX + chunkXLength, chunkYLength, minZ + chunkZLength);

    for (const auto& plane : frustumPlanes) {
        glm::vec3 positiveVertex = minPoint;
        if (plane.x >= 0.f) positiveVertex.x = maxPoint.x;
        if (plane.y >= 0.f) positiveVertex.y = maxPoint.y;
        if (plane.z >= 0.f) positiveVertex.z = maxPoint.z;

        if (glm::dot(plane, glm::vec4(positiveVertex, 1.f)) < 0.f) {
            return false;
        }
    }
    return true;
}

void Chunk::setHasBlockData(bool val) {
    m_hasBlockData = val;
}
bool Chunk::hasBlockData() const {
    return m_hasBlockData;
}

void Chunk::setHasVBOData(bool val) {
    m_hasVBOData = val;
}
bool Chunk::hasVBOData() const {
    return m_hasVBOData;
}

void Chunk::setNeedsUpdate(bool val) {
    m_needsUpdate = val;
}
bool Chunk::needsUpdate() const {
    return m_needsUpdate;
}

void Chunk::setHasGPUData(bool val) {
    m_hasGPUData = val;
}
bool Chunk::hasGPUData() const {
    return m_hasGPUData;
}

void Chunk::generate() {
    // Set seed for noise generation
    BiomeNoise::setSeed(1);

    int oceanHeight = 140;
    for (int x = minX; x < minX + 16; ++x) {
        for (int z = minZ; z < minZ + 16; ++z) {
            int height = BiomeNoise::getHeightAt(x, z);
            BiomeNoise::Biome biome = BiomeNoise::getBiomeAt(x, z);
            for (int y = 0; y < 256; ++y) {
                setLocalBlockAt(x - minX, y, z - minZ, getGeneratedBlockAt(x, y, z, height, biome));
            }
        }
    }
}

BlockType Chunk::getGeneratedBlockAt(int x, int y, int z, int height, BiomeNoise::Biome biome) const {
    if (y > 255 || y < 0) {
        return EMPTY;
    }

    if (y == 0) {
        return BEDROCK;
    }

    int caveMaxHeight = 80;
    int caveMinHeight = 40; // increase to reduce load time

    // Caves
    if (y >= caveMinHeight && y < caveMaxHeight) {
        float noise = BiomeNoise::perlin3D(x * 0.1f, y * 0.1f, z * 0.1f);
        if (noise < 0.0f) {
            return y < caveMinHeight + 5 ? LAVA : EMPTY;
        }
    }

    // River logic
    int oceanHeight = 140;
    int riverDepth = 4;

    if (mp_riversList && height > oceanHeight) {
        for (const Rivers& r : *mp_riversList) {
            if (r.isRiverAt(x, z)) {
                // Determine river block types
                if (y >= height) {
                    return EMPTY; // Air above the river
                } else if (y > oceanHeight - riverDepth) {
                    return WATER; // River water
                } else if (y == oceanHeight - riverDepth - 1) {
                    return DIRT; // Riverbed dirt
                }
            }
        }
    }

    if (y > height) {
        // Water
        if (height < 138 && y <= 138 && y >= fmax(height + 1, caveMaxHeight + 1)) {
            return WATER;
        } else {
            return EMPTY;
        }
    } else if (y == height) {
        if (biome == BiomeNoise::GRASSLAND) {
            return GRASS;
        } else if (biome == BiomeNoise::MOUNTAIN) {
            if (height > 200) {
                return SNOW;
            } else {
                return STONE;
            }
        } else {
            return STONE;
        }
    } else if (y > 0 && y < height) {
        if (biome == BiomeNoise::GRASSLAND) {
            return DIRT;
        } else {
            return STONE;
        }
    }

    return EMPTY;
}

void Chunk::serializeModifiedBlocks(std::ofstream& ofs) {
    m_blockDataMutex.lock();
    // Set seed for noise generation
    BiomeNoise::setSeed(1);

    std::unordered_map<unsigned int, BlockType> modifiedBlocks;

    for (unsigned int x = 0; x < chunkXLength; ++x) {
        for (unsigned int z = 0; z < chunkZLength; ++z) {
            int worldX = minX + x;
            int worldZ = minZ + z;
            int height = BiomeNoise::getHeightAt(worldX, worldZ);
            BiomeNoise::Biome biome = BiomeNoise::getBiomeAt(worldX, worldZ);
            for (unsigned int y = 0; y < chunkYLength; ++y) {
                BlockType generatedBlock = getGeneratedBlockAt(worldX, y, worldZ, height, biome);
                BlockType actualBlock = getLocalBlockAt(x, y, z);
                if (generatedBlock != actualBlock) {
                    unsigned int index = x + chunkXLength * y + chunkZLength * chunkYLength * z;
                    modifiedBlocks[index] = actualBlock;
                }
            }
        }
    }

    uint16_t numModifiedBlocks = static_cast<uint16_t>(modifiedBlocks.size());
    ofs.write(reinterpret_cast<const char*>(&numModifiedBlocks), sizeof(numModifiedBlocks));

    for (const auto& entry : modifiedBlocks) {
        unsigned int index = entry.first;
        BlockType blockType = entry.second;

        unsigned int x = index % chunkXLength;
        unsigned int y = (index / chunkXLength) % chunkYLength;
        unsigned int z = index / (chunkXLength * chunkYLength);

        uint8_t xz = (static_cast<uint8_t>(x) & 0x0F) << 4 | (static_cast<uint8_t>(z) & 0x0F);

        // Write data
        ofs.put(static_cast<char>(xz));
        ofs.put(static_cast<char>(y));
        ofs.put(static_cast<char>(blockType));
    }
    m_blockDataMutex.unlock();
}

void Chunk::deserializeModifiedBlocks(std::ifstream& ifs) {
    //m_blockDataMutex.lock();
    uint16_t numModifiedBlocks;
    ifs.read(reinterpret_cast<char*>(&numModifiedBlocks), sizeof(numModifiedBlocks));

    for (uint16_t i = 0; i < numModifiedBlocks; ++i) {
        int xz_int = ifs.get();
        int y_int = ifs.get();
        int blockType_int = ifs.get();

        // Check for EOF
        if (xz_int == EOF || y_int == EOF || blockType_int == EOF) {
            std::cerr << "Unexpected end of file while reading modified blocks." << std::endl;
            return;
        }

        uint8_t xz = static_cast<uint8_t>(xz_int);
        uint8_t y = static_cast<uint8_t>(y_int);
        uint8_t blockType = static_cast<uint8_t>(blockType_int);

        unsigned int x = (xz >> 4) & 0x0F;
        unsigned int z = xz & 0x0F;
        setLocalBlockAt(x, y, z, static_cast<BlockType>(blockType));
    }
}
