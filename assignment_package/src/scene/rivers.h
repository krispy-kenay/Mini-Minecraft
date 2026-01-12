#ifndef RIVERS_H
#define RIVERS_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

struct TurtleState {
    double x;
    double z;
    double angle;
};

class Rivers {
public:
    Rivers(const std::string &axiom,
           const std::unordered_map<char, std::string> &rules,
           int iterations,
           double angleDegrees,
           double stepSize,
           double startX,
           double startZ);

    std::string generateSystem() const;

    void computeRiverPositions();

    bool isRiverAt(int x, int z) const;

private:
    std::string m_axiom;
    std::unordered_map<char, std::string> m_rules;
    int m_iterations;
    double m_angleDegrees;
    double m_stepSize;
    double m_startX;
    double m_startZ;

    std::string m_systemString;
    std::unordered_set<int64_t> m_riverPositions;

    int64_t toKey(int x, int z) const;
};

#endif // RIVERS_H
