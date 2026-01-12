#include "player.h"
#include <QString>
#include <iostream>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
    mcr_camera(m_camera), flightMode(true), onFloor(true), botCorners(),
    midCorners(), topCorners()
{
    for (int i = 0; i < 4; ++i) {
        float x = (i % 2 == 0) ? 0.4f : -0.4f;
        float z = (i < 2) ? 0.4f : -0.4f;
        botCorners[i] = glm::vec3(x, 0.f, z);
    }

    for (int i = 0; i < 4; ++i) {
        float x = (i % 2 == 0) ? 0.4f : -0.4f;
        float z = (i < 2) ? 0.4f : -0.4f;
        midCorners[i] = glm::vec3(x, 1.f, z);
    }

    for (int i = 0; i < 4; ++i) {
        float x = (i % 2 == 0) ? 0.4f : -0.4f;
        float z = (i < 2) ? 0.4f : -0.4f;
        topCorners[i] = glm::vec3(x, 1.9f, z);
    }
}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.
    flightMode = inputs.fPressed;
    isWalking = false;

    float accel = .5f;
    float friction = 0.95f;
    float gravity = -0.5f;

    m_velocity *= friction;
    m_acceleration *= 0.f;

    if (flightMode) {
        if (inputs.wPressed) { // Forward
            m_acceleration += accel * m_forward;
            isWalking = true;
        } else if (inputs.sPressed) { // Back
            m_acceleration -= accel * m_forward;
            isWalking = true;
        }

        // Left and Right
        if (inputs.dPressed) { // Left
            m_acceleration += accel * m_right;
            isWalking = true;
        } else if (inputs.aPressed) { // Right
            m_acceleration -= accel * m_right;
            isWalking = true;
        }

        // Up and Down
        if (inputs.ePressed) { // Up
            m_acceleration += accel * m_up;
        } else if (inputs.qPressed) { // Down
            m_acceleration -= accel * m_up;
        }

    } else {
        glm::vec3 playerPos = glm::floor(m_position);
        BlockType currentBlock = mcr_terrain.getGlobalBlockAt(playerPos.x, playerPos.y, playerPos.z);

        if (currentBlock && (currentBlock == WATER || currentBlock == LAVA)) {
            accel = .5f * 2.0f / 3.0f;
            friction = 0.95f * 2.0f / 3.0f;
            gravity = -0.5f * 2.0f / 3.0f;

            if (inputs.spacePressed) {
                m_velocity.y = 2.0f; // Swimming upward speed
            }
        } else {
            accel = .5f;
            friction = 0.95f;
            gravity = -0.5f;

            if (inputs.spacePressed && onFloor) { // Jump
                m_velocity.y += 20.f;
                onFloor = false;
            }
        }

        // Forward and Backward
        if (inputs.wPressed) { // Forward
            m_acceleration += accel * m_forward;
            isWalking = true;
        } else if (inputs.sPressed) { // Back
            m_acceleration -= accel * m_forward;
            isWalking = true;
        }

        // Left and Right
        if (inputs.aPressed) { // Left
            m_acceleration += -accel * m_right;
            isWalking = true;
        } else if (inputs.dPressed) { // Right
            m_acceleration += accel * m_right;
            isWalking = true;
        }

        m_acceleration.y = gravity;
    }

    m_velocity += m_acceleration;
}

void Player::computePhysics(float dT, const Terrain &terrain) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.
    glm::vec3 playerPos = glm::floor(m_position);

    glm::vec3 displacement = m_velocity * dT + .5f * m_acceleration * (dT * dT);
    glm::vec3 rayDirection = m_velocity * dT;
    glm::ivec3 out_blockHit(0);
    float out_dist = 0.f;

    bool collide_Floor = collideFloor(glm::vec3(0.f, 1.f, 0.f) * rayDirection, terrain, &out_dist, &out_blockHit);

    // check if head under water for overlay
    BlockType aboveBlock = terrain.getGlobalBlockAt(playerPos.x, glm::round(mcr_position.y) + 1, playerPos.z);
    (aboveBlock == WATER || aboveBlock == LAVA) ? underLiquidBlock = aboveBlock : underLiquidBlock = EMPTY;

    if (flightMode) {
        moveForwardGlobal(displacement.z);
        moveRightGlobal(displacement.x);
        moveUpGlobal(displacement.y);
        onFloor = collide_Floor;
    } else {

        BlockType belowBlock = terrain.getGlobalBlockAt(playerPos.x, playerPos.y - 1, playerPos.z);
        BlockType frontBlock = terrain.getGlobalBlockAt(playerPos.x, playerPos.y, playerPos.z + 1);
        BlockType backBlock = terrain.getGlobalBlockAt(playerPos.x, playerPos.y, playerPos.z - 1);
        BlockType rightBlock = terrain.getGlobalBlockAt(playerPos.x + 1, playerPos.y, playerPos.z);
        BlockType leftBlock = terrain.getGlobalBlockAt(playerPos.x - 1, playerPos.y, playerPos.z);


        bool collide_F = collideF(glm::vec3(0.f, 0.f, 1.f) * rayDirection, terrain, &out_dist, &out_blockHit);
        if (!collide_F) {
            moveForwardGlobal(displacement.z);
        } else if (displacement.z > 0 && (frontBlock && (frontBlock == WATER || frontBlock == LAVA))) {
            moveForwardGlobal(displacement.z);
        } else if (displacement.z <= 0 && (backBlock && (backBlock == WATER || backBlock == LAVA))) {
            moveForwardGlobal(displacement.z);
        }

        bool collide_L = collideR(glm::vec3(1.f, 0.f, 0.f) * rayDirection, terrain, &out_dist, &out_blockHit);
        if (!collide_L) {
            moveRightGlobal(displacement.x);
        } else if (displacement.x > 0 && (rightBlock && (rightBlock == WATER || rightBlock == LAVA))) {
            moveRightGlobal(displacement.x);
        } else if (displacement.x <= 0 && (leftBlock && (leftBlock == WATER || leftBlock == LAVA))) {
            moveRightGlobal(displacement.x);
        }

        bool collide_U = collideU(glm::vec3(0.f, 1.f, 0.f) * rayDirection, terrain, &out_dist, &out_blockHit);
        // BlockType isWater = terrain.getGlobalBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z);
        if ((belowBlock && (belowBlock == WATER || belowBlock == LAVA)) || !collide_U) {
            moveUpGlobal(displacement.y);
        }

        if (collide_Floor && !(belowBlock && (belowBlock == WATER || belowBlock == LAVA))) {
            onTopOf = terrain.getGlobalBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z);
        }

        onFloor = collide_Floor;
    }
}

bool Player::inLiquid(bool checkWaterNotLava) {
    glm::vec3 playerPos = glm::floor(m_position);
    BlockType currBlock = mcr_terrain.getGlobalBlockAt(playerPos.x, playerPos.y - 1, playerPos.z);
    BlockType compare = checkWaterNotLava ? WATER : LAVA;
    return currBlock == compare;
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}

bool Player::collideF(glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    Direction arbitrary = XPOS;
    for (int i = 0; i < 3; i++) {
        if (i == 0) {
            for (int j = 0; j < 2; j++) {
                glm::vec3 rayOriginFront(m_position + botCorners[j]);
                rayOriginFront.z += .1f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        } else if (i == 1) {
            for (int j = 0; j < 2; j++) {
                glm::vec3 rayOriginFront(m_position + midCorners[j]);
                rayOriginFront.z += .1f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        } else {
            for (int j = 0; j < 2; j++) {
                glm::vec3 rayOriginFront(m_position + topCorners[j]);
                rayOriginFront.z += .1f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        }
    }

    return false;
}

bool Player::collideR(glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    Direction arbitrary = XPOS;
    for (int i = 0; i < 3; i++) {
        if (i == 0) {
            for (int j = 0; j < 4; j += 2) {
                glm::vec3 rayOriginFront(m_position + botCorners[j]);
                rayOriginFront.x += .1f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        } else if (i == 1) {
            for (int j = 0; j < 4; j += 2) {
                glm::vec3 rayOriginFront(m_position + midCorners[j]);
                rayOriginFront.x += .1f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        } else {
            for (int j = 0; j < 4; j += 2) {
                glm::vec3 rayOriginFront(m_position + topCorners[j]);
                rayOriginFront.x += .1f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        }
    }

    return false;
}

bool Player::collideU(glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    Direction arbitrary = XPOS;
    for (int i = 0; i < 4; i++) {
        if (i == 0) {
            for (int j = 0; j < 4; j += 1) {
                glm::vec3 rayOriginFront(m_position + botCorners[j]);
                rayOriginFront.y += 1.f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        } else if (i == 1) {
            for (int j = 0; j < 4; j += 1) {
                glm::vec3 rayOriginFront(m_position + midCorners[j]);
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        } else {
            for (int j = 0; j < 4; j += 1) {
                glm::vec3 rayOriginFront(m_position + topCorners[j]);
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (isWaterOrLava(terrain.getGlobalBlockAt(out_blockHit->x, out_blockHit->y, out_blockHit->z))) {
                    return false;
                }
                if (collided) {
                    return collided;
                }
            }
        }
    }

    return false;
}

bool Player::collideFloor(glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    Direction arbitrary = XPOS;
    for (int i = 0; i < 4; i++) {
        if (i == 0) {
            for (int j = 0; j < 4; j += 1) {
                glm::vec3 rayOriginFront(m_position + botCorners[j]);
                rayOriginFront.y += 1.f; // ESSENTIAL
                bool collided = gridMarch(rayOriginFront, rayDirection, terrain, out_dist, out_blockHit, &arbitrary);
                if (collided) {
                    return collided;
                }
            }
        }
    }
    return false;
}

void Player::placeBlock(Terrain &terrain) {
    glm::vec3 rayDirection = 3.f * m_forward;
    float out_dist = 0.f;
    glm::ivec3 out_blockHit(0);
    Direction out_direction = XPOS;
    bool hitBlock = gridMarch(m_camera.mcr_position, rayDirection, terrain, &out_dist, &out_blockHit, &out_direction);
    if (hitBlock) {
        glm::ivec3 newBlockPos = out_blockHit;
        switch (out_direction) {
            case XPOS:
                newBlockPos.x += 1;
                break;
            case XNEG:
                newBlockPos.x -= 1;
                break;
            case YPOS:
                newBlockPos.y += 1;
                break;
            case YNEG:
                newBlockPos.y -= 1;
                break;
            case ZPOS:
                newBlockPos.z += 1;
                break;
            case ZNEG:
                newBlockPos.z -= 1;
                break;
        }

        terrain.setGlobalBlockAt(newBlockPos.x, newBlockPos.y, newBlockPos.z, SNOW);
    }
}

void Player::deleteBlock(Terrain &terrain) {
    glm::vec3 rayDirection = 3.f * m_forward;
    float out_dist = 0.f;
    glm::ivec3 out_blockHit(0);
    Direction arbitrary = XPOS;

    bool hitBlock = gridMarch(m_camera.mcr_position, rayDirection, terrain, &out_dist, &out_blockHit, &arbitrary);
    if (hitBlock && terrain.getGlobalBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z) != BEDROCK) {
        terrain.setGlobalBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z, EMPTY);
    }
}

bool Player::gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit, Direction *out_direction) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getGlobalBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY) {
            if (interfaceAxis == 0) {
                *out_direction = (rayDirection.x > 0) ? XNEG : XPOS;
            } else if (interfaceAxis == 1) {
                *out_direction = (rayDirection.y > 0) ? YNEG : YPOS;
            } else if (interfaceAxis == 2) {
                *out_direction = (rayDirection.z > 0) ? ZNEG : ZPOS;
            }
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

bool Player::isWaterOrLava(BlockType block) {
    return (block == WATER || block == LAVA);
}
