#pragma once

#include <cstdint>
#include <functional>

namespace ENGINE::PauseManager {

using PauseListener = std::function<void(bool paused)>;

void init(double nowSeconds);
void shutdown();
void update(double nowSeconds, float rawDtSeconds);

bool isPaused();
void setPaused(bool paused);
void togglePaused();

double gameTime();
float gameDeltaTime();
double gameTimeSinceStart();

std::uint64_t addListener(PauseListener listener);
void removeListener(std::uint64_t id);

}  // namespace ENGINE::PauseManager
