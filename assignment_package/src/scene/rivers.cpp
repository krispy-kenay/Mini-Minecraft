#include "rivers.h"
#include <stack>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

static int64_t toKeyHelper(int x, int z) {
    int64_t x64 = x;
    int64_t z64 = z;
    return (x64 << 32) | (z64 & 0xffffffff);
}

Rivers::Rivers(const std::string &axiom,
               const std::unordered_map<char, std::string> &rules,
               int iterations,
               double angleDegrees,
               double stepSize,
               double startX,
               double startZ)
    : m_axiom(axiom), m_rules(rules), m_iterations(iterations),
    m_angleDegrees(angleDegrees), m_stepSize(stepSize),
    m_startX(startX), m_startZ(startZ)
{
    m_systemString = generateSystem();
    computeRiverPositions();
}

std::string Rivers::generateSystem() const {
    std::string current = m_axiom;
    for (int i = 0; i < m_iterations; i++) {
        std::string next;
        for (char c : current) {
            if (m_rules.find(c) != m_rules.end()) {
                next += m_rules.at(c);
            } else {
                next.push_back(c);
            }
        }
        current = next;
    }
    return current;
}

int64_t Rivers::toKey(int x, int z) const {
    return toKeyHelper(x, z);
}

void Rivers::computeRiverPositions() {
    std::stack<TurtleState> stateStack;
    TurtleState currentState{m_startX, m_startZ, 0.0};

    double angleDelta = m_angleDegrees;

    for (char c : m_systemString) {
        switch (c) {
        case 'F': {
            double radians = glm::radians(currentState.angle);
            double dx = std::cos(radians);
            double dz = std::sin(radians);

            int steps = (int)std::round(m_stepSize);
            double cx = currentState.x;
            double cz = currentState.z;

            for (int i = 0; i < steps; i++) {
                cx += dx;
                cz += dz;
                int ix = (int)std::round(cx);
                int iz = (int)std::round(cz);
                m_riverPositions.insert(toKey(ix, iz));
            }

            currentState.x = cx;
            currentState.z = cz;
        }
        break;
        case '+':
            currentState.angle += angleDelta;
            break;
        case '-':
            currentState.angle -= angleDelta;
            break;
        case '[':
            stateStack.push(currentState);
            break;
        case ']':
            if (!stateStack.empty()) {
                currentState = stateStack.top();
                stateStack.pop();
            }
            break;
        default:
            break;
        }
    }
}

bool Rivers::isRiverAt(int x, int z) const {
    return m_riverPositions.find(toKey(x, z)) != m_riverPositions.end();
}
