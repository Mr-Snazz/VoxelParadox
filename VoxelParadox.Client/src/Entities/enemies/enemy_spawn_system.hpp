#pragma once

#include <string>

#include <glm/glm.hpp>

// Client
#include "enemy_types.hpp"

class Player;
class FractalWorld;

// Tries to keep a small runtime population of enemies around the player by
// spawning them on valid nearby surfaces.
void updateWorldEnemySpawning(FractalWorld& world, const Player& player, float dt);

// Forces a spawn attempt near the player using the same safety rules as the
// runtime spawn system, but without waiting for the natural cooldown.
bool tryForceSpawnWorldEnemyNearPlayer(FractalWorld& world, const Player& player,
                                       EnemyType type,
                                       std::string* outFailureReason = nullptr);

// Forces a spawn at a specific top-of-block position after validating the
// enemy can fit there safely.
bool tryForceSpawnWorldEnemyAt(FractalWorld& world, EnemyType type,
                               const glm::vec3& spawnPosition, float yawDegrees,
                               std::string* outFailureReason = nullptr);

// Developer-only helper used by the F9 debug UI to request an immediate spawn
// attempt on the next enemy update.
void requestImmediateEnemySpawnDebug();

// Returns the remaining gameplay time before the next natural spawn attempt.
// Negative means the schedule has not been initialized yet for this world.
float getRemainingNaturalEnemySpawnCooldownSeconds(const FractalWorld& world);
