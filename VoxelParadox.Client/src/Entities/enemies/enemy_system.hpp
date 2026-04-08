#pragma once

class Player;
class FractalWorld;
class GameAudioController;

// Updates enemy modules for the active world.
void updateWorldEnemies(FractalWorld& world, Player& player,
                        GameAudioController& audioController, float dt);
