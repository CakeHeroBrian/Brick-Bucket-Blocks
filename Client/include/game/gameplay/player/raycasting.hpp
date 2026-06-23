#pragma once

#include <glm/glm.hpp>

class World;
class Block;

struct HitResult {
    glm::ivec3 blockPosition;
    glm::ivec3 faceNormal;
    float distance;
};

glm::i32vec3 getChunkPosition(glm::vec3 position);
glm::i32vec3 getBlockPosition(glm::vec3 position);

bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, World& world, HitResult& result);

bool placeBlock(const glm::vec3& origin, const glm::vec3& direction, const Block& block, World& world);
bool breakBlock(const glm::vec3& origin, const glm::vec3& direction, World& world);