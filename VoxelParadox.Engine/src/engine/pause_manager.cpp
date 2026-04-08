#include "engine/pause_manager.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace ENGINE::PauseManager {
namespace {

struct PauseListenerEntry {
  std::uint64_t id = 0;
  PauseListener fn;
};

bool initialized = false;
bool paused = false;

double startTimeSeconds = 0.0;
double gameNowSeconds = 0.0;
float gameDeltaTimeSeconds = 0.0f;

std::vector<PauseListenerEntry> listeners;
std::uint64_t nextListenerId = 1;

float clampNonNegative(float value) {
  return value > 0.0f ? value : 0.0f;
}

}  // namespace

void init(double nowSeconds) {
  initialized = true;
  paused = false;
  startTimeSeconds = nowSeconds;
  gameNowSeconds = nowSeconds;
  gameDeltaTimeSeconds = 0.0f;
}

void shutdown() {
  initialized = false;
  paused = false;
  startTimeSeconds = 0.0;
  gameNowSeconds = 0.0;
  gameDeltaTimeSeconds = 0.0f;
  listeners.clear();
  nextListenerId = 1;
}

void update(double nowSeconds, float rawDtSeconds) {
  if (!initialized) {
    init(nowSeconds);
  }

  if (paused) {
    gameDeltaTimeSeconds = 0.0f;
    return;
  }

  gameDeltaTimeSeconds = clampNonNegative(rawDtSeconds);
  gameNowSeconds += static_cast<double>(gameDeltaTimeSeconds);
}

bool isPaused() {
  return paused;
}

void setPaused(bool value) {
  if (paused == value) {
    return;
  }

  paused = value;
  for (const PauseListenerEntry& entry : listeners) {
    if (entry.fn) {
      entry.fn(paused);
    }
  }
}

void togglePaused() {
  setPaused(!paused);
}

double gameTime() {
  return gameNowSeconds;
}

float gameDeltaTime() {
  return gameDeltaTimeSeconds;
}

double gameTimeSinceStart() {
  return gameNowSeconds - startTimeSeconds;
}

std::uint64_t addListener(PauseListener listener) {
  const std::uint64_t id = nextListenerId++;
  listeners.push_back(PauseListenerEntry{id, std::move(listener)});
  return id;
}

void removeListener(std::uint64_t id) {
  listeners.erase(
      std::remove_if(listeners.begin(), listeners.end(),
                     [id](const PauseListenerEntry& entry) {
                       return entry.id == id;
                     }),
      listeners.end());
}

}  // namespace ENGINE::PauseManager
