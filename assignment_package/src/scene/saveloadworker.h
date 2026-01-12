#ifndef SAVELOADWORKER_H
#define SAVELOADWORKER_H

#include <QRunnable>
#include "terrain.h"

class Terrain;

class SaveLoadWorker : public QRunnable {
private:
    Terrain* m_terrain;
    int m_zoneX;
    int m_zoneZ;
    bool m_save;
public:
    SaveLoadWorker(Terrain* terrain, int zoneX, int zoneZ, bool save);
    void run() override;
};
#endif // SAVELOADWORKER_H
