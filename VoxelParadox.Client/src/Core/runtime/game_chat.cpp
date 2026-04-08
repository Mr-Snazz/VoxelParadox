#include "runtime/game_chat.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "engine/engine.hpp"
#include "engine/input.hpp"
#include "enemies/enemy_spawn_system.hpp"
#include "items/item_catalog.hpp"
#include "player/player.hpp"
#include "render/hud/hud.hpp"
#include "render/hud/hud_text.hpp"
#include "render/hud/hud_watch_text.hpp"
#include "runtime/runtime_hud_ids.hpp"
#include "world/world_stack.hpp"

namespace {

// Chat stays self-contained on purpose: open/close, text capture, suggestions,
// and HUD visibility all live in one file.

constexpr const char* kChatHistoryGroup = RuntimeHudIds::kChatHistory;
constexpr const char* kChatInputGroup = RuntimeHudIds::kChatInput;
constexpr const char* kChatSuggestionGroup = RuntimeHudIds::kChatSuggestions;
constexpr double kClosedHistoryLifetimeSeconds = 8.0;
constexpr float kChatLeftMargin = 22.0f;
constexpr float kChatInputBottomMargin = 30.0f;
constexpr float kChatHistoryBottomMargin = 64.0f;
constexpr float kChatLineSpacing = 24.0f;
constexpr float kChatSuggestionLineSpacing = 22.0f;
constexpr float kChatSuggestionBottomMargin = 60.0f;

template <typename T>
T* addGroupedHudElement(T* element, const char* groupName, int layer = 0) {
  if (element) {
    element->setLayer(layer);
    HUD::assignToGroup(element, groupName);
  }
  return element;
}

std::vector<std::string> splitWhitespace(const std::string& value) {
  std::istringstream stream(value);
  std::vector<std::string> tokens;
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

std::string lowercaseAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) {
                   return static_cast<char>(std::tolower(ch));
                 });
  return value;
}

std::string stripInventoryPrefix(const std::string& rawValue) {
  const std::string normalized = lowercaseAscii(rawValue);
  if (normalized.rfind("block:", 0) == 0) {
    return rawValue.substr(6);
  }
  if (normalized.rfind("item:", 0) == 0) {
    return rawValue.substr(5);
  }
  return rawValue;
}

std::string displayTextForSuggestion(const std::string& suggestion) {
  if (suggestion == "/get ") {
    return "/get <item> [amount]";
  }
  if (suggestion == "/summon entity:guy ") {
    return "/summon entity:guy <normal|lookat>";
  }
  if (suggestion == "/debug wireframe ") {
    return "/debug wireframe <true|false>";
  }
  if (suggestion == "/debug camera:culling ") {
    return "/debug camera:culling <true|false>";
  }
  if (suggestion == "/debug world:mesh ") {
    return "/debug world:mesh <naive|greedy>";
  }
  return suggestion;
}

bool tryGetCurrentLookTarget(const Player& player, FractalWorld& world,
                             glm::ivec3& outTargetBlock,
                             glm::ivec3& outTargetNormal) {
  const glm::vec3 origin = player.camera.position;
  const glm::vec3 dir = player.camera.getForward();

  glm::ivec3 current = glm::ivec3(glm::floor(origin));
  glm::ivec3 step(0);
  glm::vec3 tMax(0.0f);
  glm::vec3 tDelta(0.0f);

  for (int axis = 0; axis < 3; axis++) {
    if (dir[axis] > 0.0f) {
      step[axis] = 1;
      tMax[axis] =
          (std::floor(origin[axis]) + 1.0f - origin[axis]) / dir[axis];
      tDelta[axis] = 1.0f / dir[axis];
    } else if (dir[axis] < 0.0f) {
      step[axis] = -1;
      tMax[axis] =
          (origin[axis] - std::floor(origin[axis])) / (-dir[axis]);
      tDelta[axis] = 1.0f / (-dir[axis]);
    } else {
      step[axis] = 0;
      tMax[axis] = 1e30f;
      tDelta[axis] = 1e30f;
    }
  }

  const int maxSteps = static_cast<int>(player.breakRange / 0.5f);
  for (int stepIndex = 0; stepIndex < maxSteps; stepIndex++) {
    int axis = 0;
    if (tMax.x < tMax.y) {
      axis = tMax.x < tMax.z ? 0 : 2;
    } else {
      axis = tMax.y < tMax.z ? 1 : 2;
    }

    current[axis] += step[axis];
    const float t = tMax[axis];
    tMax[axis] += tDelta[axis];

    if (t > player.breakRange) {
      break;
    }

    if (!canTargetBlock(world.getBlock(current))) {
      continue;
    }

    outTargetBlock = current;
    outTargetNormal = glm::ivec3(0);
    outTargetNormal[axis] = -step[axis];
    return true;
  }

  return false;
}

bool trySummonEnemyAtLookTarget(GameChatCommandContext& commandContext,
                                EnemyType type, std::string& outFailureReason) {
  FractalWorld* world = commandContext.worldStack.currentWorld();
  if (!world) {
    outFailureReason = "there is no active world.";
    return false;
  }

  glm::ivec3 targetBlock(0);
  glm::ivec3 targetNormal(0);
  if (!tryGetCurrentLookTarget(commandContext.player, *world, targetBlock,
                               targetNormal) ||
      targetNormal != glm::ivec3(0, 1, 0)) {
    outFailureReason = "look at the top face of a solid block first.";
    return false;
  }

  const BlockType supportType = world->getBlock(targetBlock);
  if (!isSolid(supportType)) {
    outFailureReason = "look at the top face of a solid block first.";
    return false;
  }

  const glm::vec3 spawnPosition =
      glm::vec3(targetBlock) + glm::vec3(0.5f, 1.0f, 0.5f);
  const glm::vec3 forward = glm::normalize(commandContext.player.camera.getForward());
  const float yawDegrees =
      glm::degrees(std::atan2(-forward.x, -forward.z));
  return tryForceSpawnWorldEnemyAt(*world, type, spawnPosition, yawDegrees,
                                   &outFailureReason);
}

} // namespace

void GameChat::open() {
  if (open_) {
    return;
  }

  open_ = true;
  nextBackspaceRepeatTime_ = -1.0;
  Input::enableTextInput(true);
}

void GameChat::close() {
  if (!open_) {
    return;
  }

  open_ = false;
  inputBuffer_.clear();
  nextBackspaceRepeatTime_ = -1.0;
  Input::enableTextInput(false);
}

bool GameChat::handleFrameInput(GameChatCommandContext& commandContext,
                                bool allowOpenChat) {
  if (!open_) {
    if (!allowOpenChat || !Input::keyPressed(GLFW_KEY_T)) {
      return false;
    }

    open();
    return true;
  }

  inputBuffer_ += Input::consumeTypedChars();

  const double now = ENGINE::GETTIME();
  if (Input::keyPressed(GLFW_KEY_BACKSPACE)) {
    eraseLastCharacter();
    nextBackspaceRepeatTime_ = now + 0.45;
  } else if (!Input::keyDown(GLFW_KEY_BACKSPACE)) {
    nextBackspaceRepeatTime_ = -1.0;
  } else if (nextBackspaceRepeatTime_ >= 0.0 && now >= nextBackspaceRepeatTime_) {
    eraseLastCharacter();
    nextBackspaceRepeatTime_ = now + 0.05;
  }

  if (Input::keyPressed(GLFW_KEY_TAB)) {
    autocompleteInput();
    return true;
  }

  if (Input::keyPressed(GLFW_KEY_UP) && !lastSubmittedInput_.empty()) {
    inputBuffer_ = lastSubmittedInput_;
    return true;
  }

  if (Input::keyPressed(GLFW_KEY_ESCAPE)) {
    close();
    return true;
  }

  if (Input::keyPressed(GLFW_KEY_ENTER) || Input::keyPressed(GLFW_KEY_KP_ENTER)) {
    submit(commandContext);
    return true;
  }

  return true;
}

void GameChat::setupHud() {
  HUD::createGroup(kChatHistoryGroup, 70, false);
  HUD::createGroup(kChatInputGroup, 71, false);
  HUD::createGroup(kChatSuggestionGroup, 72, false);

  for (int lineIndex = 0; lineIndex < kVisibleHistoryLines; lineIndex++) {
    const float yOffset =
        -(kChatHistoryBottomMargin + kChatLineSpacing * (kVisibleHistoryLines - 1 - lineIndex));
    auto* line = addGroupedHudElement(
        HUD::watchText(
            [this, lineIndex](std::string& out) {
              out = historyLineText(lineIndex);
            },
            makeHUDLayout(HUDAnchor::BOTTOM_LEFT,
                          glm::vec2(kChatLeftMargin, yOffset)),
            glm::vec2(1.0f), 20),
        kChatHistoryGroup, lineIndex);
    if (line) {
      line->setColor(glm::vec3(0.92f, 0.95f, 1.0f));
    }
  }

  auto* inputLine = addGroupedHudElement(
      HUD::watchText(
          [this](std::string& out) {
            out = inputLineText();
          },
          makeHUDLayout(HUDAnchor::BOTTOM_LEFT,
                        glm::vec2(kChatLeftMargin, -kChatInputBottomMargin)),
          glm::vec2(1.0f), 22),
      kChatInputGroup, 0);
  if (inputLine) {
    inputLine->setColor(glm::vec3(1.0f, 0.98f, 0.72f));
  }

  for (int lineIndex = 0; lineIndex < kVisibleSuggestionLines; lineIndex++) {
    const float yOffset =
        -(kChatSuggestionBottomMargin +
          kChatSuggestionLineSpacing *
              (kVisibleSuggestionLines - 1 - lineIndex));
    auto* suggestionLine = addGroupedHudElement(
        HUD::watchText(
            [this, lineIndex](std::string& out) {
              out = suggestionLineText(lineIndex);
            },
            makeHUDLayout(HUDAnchor::BOTTOM_LEFT,
                          glm::vec2(kChatLeftMargin, yOffset)),
            glm::vec2(1.0f), 18),
        kChatSuggestionGroup, lineIndex);
    if (suggestionLine) {
      suggestionLine->setColor(glm::vec3(0.62f, 0.86f, 1.0f));
    }
  }
}

void GameChat::syncHudState() const {
  HUD::setGroupEnabled(kChatHistoryGroup, shouldShowHistory());
  HUD::setGroupEnabled(kChatInputGroup, open_);
  HUD::setGroupEnabled(kChatSuggestionGroup, shouldShowSuggestions());
}

std::string GameChat::historyLineText(int lineIndex) const {
  if (lineIndex < 0 || lineIndex >= kVisibleHistoryLines) {
    return {};
  }

  std::vector<const Entry*> visibleEntries;
  visibleEntries.reserve(kVisibleHistoryLines);

  const double now = ENGINE::GETTIME();
  for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
    if (!open_ && (now - it->timestampSeconds) > kClosedHistoryLifetimeSeconds) {
      continue;
    }
    visibleEntries.push_back(&(*it));
    if (visibleEntries.size() >= static_cast<std::size_t>(kVisibleHistoryLines)) {
      break;
    }
  }

  std::reverse(visibleEntries.begin(), visibleEntries.end());
  if (lineIndex >= static_cast<int>(visibleEntries.size())) {
    return {};
  }

  return visibleEntries[static_cast<std::size_t>(lineIndex)]->text;
}

std::string GameChat::inputLineText() const {
  if (!open_) {
    return {};
  }

  std::string line = "> ";
  line += inputBuffer_;

  const int blinkPhase = static_cast<int>(ENGINE::GETTIME() * 2.0);
  if ((blinkPhase % 2) == 0) {
    line += '_';
  }

  return line;
}

std::string GameChat::suggestionLineText(int lineIndex) const {
  if (lineIndex < 0 || lineIndex >= kVisibleSuggestionLines) {
    return {};
  }

  const std::vector<std::string> candidates = autocompleteCandidates();
  if (lineIndex >= static_cast<int>(candidates.size())) {
    return {};
  }

  return displayTextForSuggestion(
      candidates[static_cast<std::size_t>(lineIndex)]);
}

void GameChat::pushHistory(const std::string& text) {
  if (text.empty()) {
    return;
  }

  history_.push_back({text, ENGINE::GETTIME()});
  while (history_.size() > static_cast<std::size_t>(kMaxHistoryEntries)) {
    history_.pop_front();
  }
}

void GameChat::eraseLastCharacter() {
  if (!inputBuffer_.empty()) {
    inputBuffer_.pop_back();
  }
}

bool GameChat::shouldShowHistory() const {
  if (open_) {
    return true;
  }

  const double now = ENGINE::GETTIME();
  for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
    if ((now - it->timestampSeconds) <= kClosedHistoryLifetimeSeconds) {
      return true;
    }
  }
  return false;
}

bool GameChat::shouldShowSuggestions() const {
  return open_ && !autocompleteCandidates().empty();
}

void GameChat::autocompleteInput() {
  const std::vector<std::string> candidates = autocompleteCandidates();
  if (candidates.empty()) {
    return;
  }

  if (candidates.size() == 1) {
    inputBuffer_ = candidates.front();
    return;
  }

  const std::string prefix = longestCommonPrefix(candidates);
  if (prefix.size() > inputBuffer_.size()) {
    inputBuffer_ = prefix;
  }
}

std::vector<std::string> GameChat::autocompleteCandidates() const {
  if (inputBuffer_.empty() || inputBuffer_.front() != '/') {
    return {};
  }

  const std::string normalized = lowercase(inputBuffer_);

  auto filterByPrefix =
      [&normalized](const std::vector<std::string>& options) {
        std::vector<std::string> candidates;
        for (const std::string& option : options) {
          if (option.rfind(normalized, 0) == 0) {
            candidates.push_back(option);
          }
        }
        return candidates;
      };

  if (normalized == "/debug wireframe" ||
      normalized.rfind("/debug wireframe ", 0) == 0) {
    return filterByPrefix(
        {"/debug wireframe true", "/debug wireframe false"});
  }

  if (normalized == "/debug camera:culling" ||
      normalized.rfind("/debug camera:culling ", 0) == 0) {
    return filterByPrefix(
        {"/debug camera:culling true", "/debug camera:culling false"});
  }

  if (normalized == "/debug world:mesh" ||
      normalized.rfind("/debug world:mesh ", 0) == 0) {
    return filterByPrefix(
        {"/debug world:mesh naive", "/debug world:mesh greedy"});
  }

  if (normalized == "/summon entity:guy" ||
      normalized.rfind("/summon entity:guy ", 0) == 0) {
    return filterByPrefix(
        {"/summon entity:guy normal", "/summon entity:guy lookat"});
  }

  return filterByPrefix({
      "/help",
      "/get ",
      "/summon entity:guy ",
      "/debug wireframe ",
      "/debug camera:culling ",
      "/debug world:mesh ",
  });
}

std::string GameChat::trim(const std::string& value) {
  std::size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    start++;
  }

  std::size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    end--;
  }

  return value.substr(start, end - start);
}

std::string GameChat::lowercase(std::string value) {
  return lowercaseAscii(std::move(value));
}

std::string GameChat::longestCommonPrefix(
    const std::vector<std::string>& values) {
  if (values.empty()) {
    return {};
  }

  std::string prefix = values.front();
  for (std::size_t index = 1; index < values.size() && !prefix.empty();
       index++) {
    const std::string& value = values[index];
    std::size_t sharedLength = 0;
    while (sharedLength < prefix.size() && sharedLength < value.size() &&
           prefix[sharedLength] == value[sharedLength]) {
      sharedLength++;
    }
    prefix.resize(sharedLength);
  }

  return prefix;
}

bool GameChat::tryParsePositiveAmount(const std::string& value, int& outAmount) {
  if (value.empty()) {
    return false;
  }

  char* parseEnd = nullptr;
  const long parsed = std::strtol(value.c_str(), &parseEnd, 10);
  if (parseEnd == value.c_str() || *parseEnd != '\0' || parsed <= 0 ||
      parsed > static_cast<long>(INT_MAX)) {
    return false;
  }

  outAmount = static_cast<int>(parsed);
  return true;
}

bool GameChat::tryParseBooleanWord(const std::string& value, bool& outValue) {
  const std::string normalized = lowercase(value);
  if (normalized == "true" || normalized == "on" || normalized == "1") {
    outValue = true;
    return true;
  }
  if (normalized == "false" || normalized == "off" || normalized == "0") {
    outValue = false;
    return true;
  }
  return false;
}

void GameChat::submit(GameChatCommandContext& commandContext) {
  const std::string submitted = trim(inputBuffer_);
  close();

  if (submitted.empty()) {
    return;
  }

  lastSubmittedInput_ = submitted;

  if (submitted.front() == '/') {
    if (!executeCommand(commandContext, submitted.substr(1))) {
      pushHistory("[Debug] Unknown command. Use /help.");
    }
    return;
  }

  pushHistory(std::string("You: ") + submitted);
}

bool GameChat::executeCommand(GameChatCommandContext& commandContext,
                              const std::string& commandLine) {
  const std::string trimmed = trim(commandLine);
  if (trimmed.empty()) {
    pushHistory("[Debug] Empty command.");
    return true;
  }

  const std::vector<std::string> tokens = splitWhitespace(trimmed);
  if (tokens.empty()) {
    pushHistory("[Debug] Empty command.");
    return true;
  }

  const std::string commandName = lowercase(tokens.front());
  if (commandName == "get") {
    const std::size_t argumentsOffset = trimmed.find_first_not_of(" \t", tokens.front().size());
    const std::string arguments =
        argumentsOffset == std::string::npos ? std::string{} : trimmed.substr(argumentsOffset);
    return executeGetCommand(commandContext, arguments);
  }

  if (commandName == "debug") {
    const std::size_t argumentsOffset =
        trimmed.find_first_not_of(" \t", tokens.front().size());
    const std::string arguments =
        argumentsOffset == std::string::npos ? std::string{}
                                             : trimmed.substr(argumentsOffset);
    return executeDebugCommand(commandContext, arguments);
  }

  if (commandName == "summon") {
    const std::size_t argumentsOffset =
        trimmed.find_first_not_of(" \t", tokens.front().size());
    const std::string arguments =
        argumentsOffset == std::string::npos ? std::string{}
                                             : trimmed.substr(argumentsOffset);
    return executeSummonCommand(commandContext, arguments);
  }

  if (commandName == "help") {
    pushHistory("[Debug] /get <item> [amount]");
    pushHistory("[Debug] /summon entity:guy <normal|lookat>");
    pushHistory("[Debug] /debug wireframe <true|false>");
    pushHistory("[Debug] /debug camera:culling <true|false>");
    pushHistory("[Debug] /debug world:mesh <naive|greedy>");
    return true;
  }

  return false;
}

bool GameChat::executeGetCommand(GameChatCommandContext& commandContext,
                                 const std::string& arguments) {
  const std::vector<std::string> tokens = splitWhitespace(arguments);
  if (tokens.empty()) {
    pushHistory("[Debug] Usage: /get <item> [amount]");
    return true;
  }

  InventoryItem item{};
  const std::string itemToken = stripInventoryPrefix(tokens[0]);
  if (!tryParseInventoryItem(itemToken, item) || item.empty()) {
    pushHistory("[Debug] Unknown item. Example: /get stone 6");
    return true;
  }

  int amount = 1;
  if (tokens.size() >= 2 && !tryParsePositiveAmount(tokens[1], amount)) {
    pushHistory("[Debug] Amount must be a positive integer.");
    return true;
  }

  if (!commandContext.player.tryAddItemToInventory(item, amount)) {
    pushHistory("[Debug] Inventory has no room for that request.");
    return true;
  }

  char buffer[160];
  std::snprintf(buffer, sizeof(buffer), "[Debug] Added %d x %s.", amount,
                getInventoryItemDisplayName(item));
  pushHistory(buffer);
  return true;
}

bool GameChat::executeDebugCommand(GameChatCommandContext& commandContext,
                                   const std::string& arguments) {
  const std::vector<std::string> tokens = splitWhitespace(arguments);
  if (tokens.size() < 2) {
    pushHistory("[Debug] Usage: /debug wireframe <true|false>");
    pushHistory("[Debug] Usage: /debug camera:culling <true|false>");
    pushHistory("[Debug] Usage: /debug world:mesh <naive|greedy>");
    return true;
  }

  const std::string debugTarget = lowercase(tokens[0]);
  const std::string debugValue = lowercase(tokens[1]);

  if (debugTarget == "wireframe") {
    bool enabled = false;
    if (!tryParseBooleanWord(debugValue, enabled)) {
      pushHistory("[Debug] Usage: /debug wireframe <true|false>");
      return true;
    }

    commandContext.wireframeMode = enabled;
    std::printf("[Debug] Wireframe mode: %s\n", enabled ? "ON" : "OFF");
    pushHistory(std::string("[Debug] Wireframe mode set to ") +
                (enabled ? "true." : "false."));
    return true;
  }

  if (debugTarget == "camera:culling") {
    bool enabled = false;
    if (!tryParseBooleanWord(debugValue, enabled)) {
      pushHistory("[Debug] Usage: /debug camera:culling <true|false>");
      return true;
    }

    commandContext.debugThirdPersonView = enabled;
    std::printf("[Debug] Third-person culling inspector: %s\n",
                enabled ? "ON" : "OFF");
    pushHistory(std::string("[Debug] Camera culling inspector set to ") +
                (enabled ? "true." : "false."));
    return true;
  }

  if (debugTarget == "world:mesh") {
    const bool greedyMeshing = debugValue == "greedy";
    if (!greedyMeshing && debugValue != "naive") {
      pushHistory("[Debug] Usage: /debug world:mesh <naive|greedy>");
      return true;
    }

    FractalWorld::setGreedyMeshingEnabled(greedyMeshing);
    commandContext.worldStack.markAllWorldMeshesDirty();
    std::printf("[Debug] World meshing: %s\n",
                greedyMeshing ? "GREEDY" : "NAIVE");
    pushHistory(std::string("[Debug] World meshing set to ") +
                (greedyMeshing ? "greedy." : "naive."));
    return true;
  }

  pushHistory("[Debug] Unknown debug target.");
  return true;
}

bool GameChat::executeSummonCommand(GameChatCommandContext& commandContext,
                                    const std::string& arguments) {
  const std::vector<std::string> tokens = splitWhitespace(arguments);
  if (tokens.empty()) {
    pushHistory("[Debug] Usage: /summon entity:guy <normal|lookat>");
    return true;
  }

  const std::string summonTarget = lowercase(tokens[0]);
  if (summonTarget != "entity:guy") {
    pushHistory("[Debug] Unknown summon target. Try entity:guy.");
    return true;
  }

  const std::string summonMode =
      tokens.size() >= 2 ? lowercase(tokens[1]) : "normal";
  std::string failureReason;

  if (summonMode == "normal") {
    FractalWorld* world = commandContext.worldStack.currentWorld();
    if (!world) {
      pushHistory("[Debug] Summon failed: there is no active world.");
      return true;
    }

    if (!tryForceSpawnWorldEnemyNearPlayer(*world, commandContext.player,
                                           EnemyType::Guy, &failureReason)) {
      pushHistory(std::string("[Debug] Summon failed: ") + failureReason);
      return true;
    }

    pushHistory("[Debug] Summoned entity:guy using normal spawn rules.");
    return true;
  }

  if (summonMode == "lookat") {
    if (!trySummonEnemyAtLookTarget(commandContext, EnemyType::Guy,
                                    failureReason)) {
      pushHistory(std::string("[Debug] Summon failed: ") + failureReason);
      return true;
    }

    pushHistory("[Debug] Summoned entity:guy at the current look target.");
    return true;
  }

  pushHistory("[Debug] Usage: /summon entity:guy <normal|lookat>");
  return true;
}
