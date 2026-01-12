#include "blocktypeworker.h"

BlockTypeWorker::BlockTypeWorker(Chunk* chunk)
    : m_chunk(chunk)
{}

void BlockTypeWorker::run()
{
    // Generate the block data for the chunk
    m_chunk->generate();
    // Mark the chunk as having block data generated
    m_chunk->setHasBlockData(true);
    // Mark the chunk to update its VBO data
    // (Should not be necessary, but it's better to be safe than sorry)
    m_chunk->setNeedsUpdate(true);
}
