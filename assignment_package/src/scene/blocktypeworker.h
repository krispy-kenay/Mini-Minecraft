#ifndef BLOCKTYPEWORKER_H
#define BLOCKTYPEWORKER_H

#include <QRunnable>
#include "chunk.h"
#include "biomenoise.h"

class BlockTypeWorker : public QRunnable
{
public:
    BlockTypeWorker(Chunk* chunk);
    void run() override;

private:
    Chunk* m_chunk;
};

#endif // BLOCKTYPEWORKER_H
