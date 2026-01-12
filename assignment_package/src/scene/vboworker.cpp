#include "vboworker.h"
#include <iostream>

VBOWorker::VBOWorker(Chunk* chunk)
    : m_chunk(chunk)
{}

void VBOWorker::run()
{
    try {
        if (m_chunk->needsUpdate() && m_chunk->hasBlockData()) {
                // Generate VBO data for the chunk
                m_chunk->createVBOdata();
                // Mark the chunk as having VBO data ready
                m_chunk->setHasVBOData(true);
                m_chunk->setNeedsUpdate(false);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error generating VBO data for chunk: " << e.what() << std::endl;
    }
    
}
