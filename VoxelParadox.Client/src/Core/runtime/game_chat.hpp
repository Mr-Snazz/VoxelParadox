#pragma once

#include <deque>
#include <string>
#include <vector>

class Player;
class WorldStack;

struct GameChatCommandContext {
  Player& player;
  WorldStack& worldStack;
  bool& wireframeMode;
  bool& debugThirdPersonView;
};

class GameChat {
public:
  bool isOpen() const {
    return open_;
  }

  void open();
  void close();

  bool handleFrameInput(GameChatCommandContext& commandContext,
                        bool allowOpenChat);

  void setupHud();
  void syncHudState() const;

  std::string historyLineText(int lineIndex) const;
  std::string inputLineText() const;
  std::string suggestionLineText(int lineIndex) const;

private:
  struct Entry {
    std::string text;
    double timestampSeconds = 0.0;
  };

  static constexpr int kMaxHistoryEntries = 32;
  static constexpr int kVisibleHistoryLines = 6;
  static constexpr int kVisibleSuggestionLines = 4;

  bool open_ = false;
  std::string inputBuffer_;
  std::string lastSubmittedInput_;
  std::deque<Entry> history_;
  double nextBackspaceRepeatTime_ = -1.0;

  void pushHistory(const std::string& text);
  void submit(GameChatCommandContext& commandContext);
  void eraseLastCharacter();
  bool shouldShowHistory() const;
  bool shouldShowSuggestions() const;
  void autocompleteInput();
  std::vector<std::string> autocompleteCandidates() const;

  static std::string trim(const std::string& value);
  static std::string lowercase(std::string value);
  static std::string longestCommonPrefix(const std::vector<std::string>& values);
  static bool tryParsePositiveAmount(const std::string& value, int& outAmount);
  static bool tryParseBooleanWord(const std::string& value, bool& outValue);

  bool executeCommand(GameChatCommandContext& commandContext,
                      const std::string& commandLine);
  bool executeGetCommand(GameChatCommandContext& commandContext,
                         const std::string& arguments);
  bool executeDebugCommand(GameChatCommandContext& commandContext,
                           const std::string& arguments);
  bool executeSummonCommand(GameChatCommandContext& commandContext,
                            const std::string& arguments);
};
