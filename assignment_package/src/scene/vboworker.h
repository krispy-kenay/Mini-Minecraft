#ifndef VBOWORKER_H
#define VBOWORKER_H

#include <QRunnable>
#include "chunk.h"

class VBOWorker : public QRunnable
{
public:
    VBOWorker(Chunk* chunk);
    void run() override;

private:
    Chunk* m_chunk;
};

#endif // VBOWORKER_H
