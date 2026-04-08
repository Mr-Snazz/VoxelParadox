#pragma once

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <functional>
#include <limits>
#include <string>

#include "engine/engine.hpp"
#include "engine/input.hpp"

struct TextInputState {
  std::string text;
  std::size_t caretIndex = 0;
  std::size_t selectionStart = 0;
  std::size_t selectionEnd = 0;
  double nextBackspaceRepeatTime = -1.0;
  double nextDeleteRepeatTime = -1.0;
  double nextLeftRepeatTime = -1.0;
  double nextRightRepeatTime = -1.0;

  bool empty() const { return text.empty(); }

  bool hasSelection() const { return selectionStart != selectionEnd; }

  void clampCaret() {
    caretIndex = std::min(caretIndex, text.size());
    selectionStart = std::min(selectionStart, text.size());
    selectionEnd = std::min(selectionEnd, text.size());
  }

  void resetRepeats() {
    nextBackspaceRepeatTime = -1.0;
    nextDeleteRepeatTime = -1.0;
    nextLeftRepeatTime = -1.0;
    nextRightRepeatTime = -1.0;
  }

  void clearSelection() {
    clampCaret();
    selectionStart = caretIndex;
    selectionEnd = caretIndex;
  }

  void setText(const std::string& value) {
    text = value;
    caretIndex = text.size();
    clearSelection();
  }

  void selectAll() {
    selectionStart = 0;
    selectionEnd = text.size();
    caretIndex = selectionEnd;
  }

  void deleteSelection() {
    if (!hasSelection()) {
      return;
    }

    const std::size_t first = std::min(selectionStart, selectionEnd);
    const std::size_t last = std::max(selectionStart, selectionEnd);
    text.erase(first, last - first);
    caretIndex = first;
    clearSelection();
  }

  void insertText(const std::string& typed, std::size_t maxLength = 0) {
    if (typed.empty()) {
      return;
    }

    if (hasSelection()) {
      deleteSelection();
    }

    const std::size_t available =
        maxLength == 0
            ? typed.size()
            : (maxLength > text.size() ? maxLength - text.size() : 0);
    if (available == 0) {
      return;
    }

    const std::string clipped = typed.substr(0, available);
    text.insert(caretIndex, clipped);
    caretIndex += clipped.size();
    clearSelection();
  }

  void moveCaretLeft() {
    if (hasSelection()) {
      caretIndex = std::min(selectionStart, selectionEnd);
      clearSelection();
      return;
    }
    if (caretIndex == 0) {
      return;
    }
    --caretIndex;
    clearSelection();
  }

  void moveCaretRight() {
    if (hasSelection()) {
      caretIndex = std::max(selectionStart, selectionEnd);
      clearSelection();
      return;
    }
    if (caretIndex >= text.size()) {
      return;
    }
    ++caretIndex;
    clearSelection();
  }

  void eraseBackward() {
    if (hasSelection()) {
      deleteSelection();
      return;
    }
    if (caretIndex == 0) {
      return;
    }
    text.erase(caretIndex - 1, 1);
    --caretIndex;
    clearSelection();
  }

  void eraseForward() {
    if (hasSelection()) {
      deleteSelection();
      return;
    }
    if (caretIndex >= text.size()) {
      return;
    }
    text.erase(caretIndex, 1);
    clearSelection();
  }

  template <typename PrefixMeasureFn>
  void placeCaretFromMouse(float localX, PrefixMeasureFn&& measurePrefixWidth) {
    if (text.empty()) {
      caretIndex = 0;
      clearSelection();
      return;
    }

    std::size_t bestIndex = text.size();
    float bestDistance = std::numeric_limits<float>::max();

    for (std::size_t index = 0; index <= text.size(); ++index) {
      const float caretX = measurePrefixWidth(index);
      const float distance = std::fabs(localX - caretX);
      if (distance < bestDistance) {
        bestDistance = distance;
        bestIndex = index;
      }
    }

    caretIndex = bestIndex;
    clearSelection();
  }

  static bool consumeHeldKey(int key, double now, double& nextRepeatTime,
                             double initialDelay = 0.35,
                             double repeatInterval = 0.045) {
    if (Input::keyPressed(key)) {
      nextRepeatTime = now + initialDelay;
      return true;
    }
    if (!Input::keyDown(key)) {
      nextRepeatTime = -1.0;
      return false;
    }
    if (nextRepeatTime >= 0.0 && now >= nextRepeatTime) {
      nextRepeatTime = now + repeatInterval;
      return true;
    }
    return false;
  }
};
