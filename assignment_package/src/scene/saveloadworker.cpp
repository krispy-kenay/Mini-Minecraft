#include "saveloadworker.h"

SaveLoadWorker::SaveLoadWorker(Terrain* terrain, int zoneX, int zoneZ, bool save) : m_terrain(terrain), m_zoneX(zoneX), m_zoneZ(zoneZ), m_save(save) {}

void SaveLoadWorker::run() {
    if (m_save) {
        m_terrain->saveZone(m_zoneX, m_zoneZ);
    } else {
        m_terrain->loadZone(m_zoneX, m_zoneZ);
    }
}