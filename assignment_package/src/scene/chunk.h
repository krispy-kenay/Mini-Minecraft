#pragma once
#include "../smartpointerhelp.h"
#include "../glm_includes.h"
#include <array>
#include <algorithm>
#include <unordered_map>
#include <cstddef>
#include <fstream>
#include <QMutex>
#include "../drawable.h"
#include "../shaderprogram.h"
#include "camera.h"
#include "biomenoise.h"
#include "rivers.h"


//using namespace std; 

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, LAVA, BEDROCK,
    ICE, SNOW, SNOW_DIRT
};

// This variable holds the number of different block types.
// We use this to speed up certain portions of the code, 
// for example, we can generate a fixed size array to
// count how many blocks of each type there are 
// in `determineBlockTypeForArea()`
//
// !!!! Update this when you add more block types !!!!
// (It will crash if you don't do this!)
const int MAX_BLOCK_TYPES = 10;

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

// A vertex struct that holds all the per-vertex attributes
struct Vertex {
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec4 color;
    glm::vec2 uv;
    float animated = 0.f;
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.
class Chunk : public Drawable {
private:
    // --- Member variables ---
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // The coordinates of the chunk's lower-left corner in world space
    int minX, minZ;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;    
    // Member variable that specifies the level of detail for this chunk
    // 0 is the highest level
    int m_levelOfDetail;

    // ------ VBO data ------
    // Opaque Vertex data for this chunk
    std::vector<Vertex> m_vertexDataOpaque;
    // Opaque Index data for this chunk
    std::vector<GLuint> m_indicesOpaque;
    // Transparent Vertex data for this chunk
    std::vector<Vertex> m_vertexDataTransparent;
    // Transparent Index data for this chunk
    std::vector<GLuint> m_indicesTransparent;

    // ------ Mutexes ------
    // Mutex to protect the block data of this chunk
    QMutex m_blockDataMutex;
    // Mutex to protect the VBO data of this chunk
    QMutex m_VBODataMutex;

    // ------ Atomics ------
    // Variable to indicate whether or not this chunk has its block data generated
    std::atomic<bool> m_hasBlockData;
    // Variable to indicate whether or not this chunks VBO data needs to be updated
    std::atomic<bool> m_needsUpdate;
    // Variable to indicate whether or not the VBO data has been generated (and stored in CPU memory)
    std::atomic<bool> m_hasVBOData;
    // Variable to indicate whether or not the VBO data has been transferred to the GPU
    std::atomic<bool> m_hasGPUData;
    
    // --- VBO helper functions ---
    // Determine the block type for a given area
    // between start and start + blockSize
    BlockType determineBlockTypeForArea(unsigned int startX, unsigned int startY, unsigned int startZ, int blockSize);
    // Fill the vertexData and indices vectors with the appropriate geometry
    // for the current LOD
    void generateBlockGeometry(unsigned int x, unsigned int y, unsigned int z, BlockType block, int blockSize, std::vector<Vertex>& vertexDataOpaque, std::vector<GLuint>& indicesOpaque, std::vector<Vertex>& vertexDataTransparent, std::vector<GLuint>& indicesTransparent);
    // Check whether a face should be rendered
    bool shouldRenderFace(unsigned int x, unsigned int y, unsigned int z, Direction dir, int blockSize);
    // Get a vector with the vertices of a face
    std::vector<glm::vec4> getFaceVertices(Direction dir, int blockSize);
    // Get the normal for a face
    glm::vec4 getNormal(Direction dir);
    // Get the color for a block
    glm::vec4 getBlockColor(BlockType block);
    void addUV(std::vector<Vertex>&, Direction, BlockType);
    void addUVHelper(std::vector<Vertex>&, int x, int y, Direction dir);

    bool isOpaque(BlockType);
    bool isOpaqueOrLava(BlockType);
    bool isAnimated(BlockType);

    const std::vector<Rivers>* mp_riversList;

public:
    // --- Constructor ---
    // Default constructor
    Chunk(OpenGLContext* context, int x, int z, const std::vector<Rivers>* rivers);
    // Generate the block data for this chunk
    // (Yes this is not a constructor, but its crucial in "constructing" the chunk)
    void generate();
    // Destructor
    ~Chunk();

    // --- Getters ---
    // Get block type in local chunk coordinates
    BlockType getLocalBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getLocalBlockAt(int x, int y, int z) const;
    // Get the generated block type in local chunk coordinates
    BlockType getGeneratedBlockAt(int x, int y, int z, int height, BiomeNoise::Biome biome) const;
    // Get the level of detail for this chunk
    int getLevelOfDetail() const;
    // Get whether this chunk has block data generated yet (this prevents race conditions)
    bool hasBlockData() const;
    // Check whether this chunk needs its VBO data updated
    bool needsUpdate() const;
    // Check whether this chunk has its VBO data generated
    bool hasVBOData() const;
    // Check whether this chunk has its VBO data sent to the GPU
    bool hasGPUData() const;
    // Get the center of this Chunks coordinates
    glm::vec2 getCenter() const;
    // Check if this Chunk is in the view frustum of the camera
    bool isInView(const Camera& camera) const;

    // --- Setters ---
    // Set block type in local chunk coordinates
    void setLocalBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    // Set the level of detail for this chunk, and update the VBO data (if necessary)
    void setLevelOfDetail(int levelOfDetail);
    // Mark this this chunk as having block data generated
    void setHasBlockData(bool val);
    // Mark this chunk as needing its VBO data updated
    void setNeedsUpdate(bool val);
    // Mark this chunk as having its VBO data generated
    void setHasVBOData(bool val);
    // Mark this chunk as having its VBO data sent to the GPU
    void setHasGPUData(bool val);

    // --- Helpers ---
    // Helper function to create links between neighboring Chunks
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    // --- VBO functions ---
    // Override the mode that OpenGL should use to draw objects in the chunk
    GLenum drawMode() override;
    // Create the VBO data for this chunk
    virtual void createVBOdata() override;
    // Send vertex / VBO data to the GPU
    void bufferVertexData();
    // Draw the Chunk
    void draw(ShaderProgram* shaderProgram);
    void drawTransparent(ShaderProgram* shaderProgram);

    // --- IO operations ---
    // Serialize the modified blocks in this chunk to a file
    void serializeModifiedBlocks(std::ofstream& ofs);
    // Deserialize the modified blocks in this chunk from a file
    void deserializeModifiedBlocks(std::ifstream& ifs);
};
