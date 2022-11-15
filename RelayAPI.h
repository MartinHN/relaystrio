#pragma once
#include "./lib/JPI/src/API.hpp"

struct RelayAPI : public APIAndInstance<RelayAPI>, LeafNode {
  static constexpr int pinDelayMs = 0;
  static constexpr int debounceTime = pinDelayMs == 0 ? 100 : pinDelayMs;
  unsigned long long lastChangeTime = 0;

  bool relayOn = false;
  bool handleRelayOn = false;
  unsigned long lastActMs = 0;
  int prevActIdx = -1;
  const std::vector<int> relayPins = {27, 26, 25, 33, 32}; // add pins here
  // const std::vector<int> states;
  RelayAPI() : APIAndInstance<RelayAPI>(this) {
    rGetSet<bool>(
        "activate", [this]() { return relayOn; },
        [this](const bool &b) {
          setRelayState(b);
        });
  }

  // clearStates() {
  //   states.clear();
  //   for (auto p : relayPins) {
  //     states.push_back(0);
  //   }
  // }

  RelayAPI(RelayAPI &) = delete;

  bool setup() override {
    for (auto &p : relayPins)
      pinMode(p, OUTPUT);
    return true;
  }
  void handle() override {
    const auto now = millis();
    if ((lastChangeTime > 0) && ((now - lastChangeTime) < debounceTime)) {
      if ((lastActMs > 0) && (prevActIdx < int(relayPins.size()) - 1)) {
        lastActMs = now;
        prevActIdx = -1;
      }
      return;
    }

    if (pinDelayMs == 0) {
      if (handleRelayOn != relayOn) {
        handleRelayOn = relayOn;
        Serial.print(F("[app] setting relay to  "));
        if (relayOn)
          Serial.print(F("on  "));
        else
          Serial.print(F("off "));
        Serial.println(now / 1000.0);
        for (auto &p : relayPins)
          digitalWrite(p, relayOn);
      }
      return;
    }

    if ((lastActMs > 0) && (prevActIdx < int(relayPins.size()) - 1)) {

      int idxAct = (now - lastActMs) / pinDelayMs;
      if (idxAct < 0) {
        Serial.println(F(">>>>>>>>>WHaaaat???"));
        idxAct = 0;
      }
      if (prevActIdx < -1) {
        Serial.println(F(">>>>>>>>>WHaaaat???"));
        prevActIdx = -1;
      }
      if (idxAct >= relayPins.size())
        idxAct = relayPins.size() - 1;

      for (int i = prevActIdx + 1; i <= idxAct; i++) {
        Serial.print(F("[app] setting relay pin "));
        Serial.print(i);
        if (relayOn)
          Serial.print(F(" on  "));
        else
          Serial.print(F(" off "));
        Serial.println(now / 1000.0);
        auto p = relayPins[i];
        digitalWrite(p, relayOn);
      }
      prevActIdx = idxAct;
      // for (int i = 0; i < relayPins.size(); i++) {
      //   auto p = relayPins[i];
      //   auto destS = relayOn;
      //   if (i > idxAct)
      //     destS = !destS;
      //   digitalWrite()
      // }
      // for (int i = 0; i < states.size() ; i++){
      //   auto s = states[i];
      //   if(relayOn!=s){

      //   }
      // }
    }
  }

  void setRelayState(bool b) {
    const auto now = millis();
    if (relayOn != b) {
      if (now - lastChangeTime < debounceTime) {
        Serial.println(F("[app] debouncing relay "));
      }
      lastChangeTime = now;
      // Serial.print(F("[app] setting relay to  "));
      // if (b)
      //   Serial.print(F("on  "));
      // else
      //   Serial.print(F("off "));
    }
    relayOn = b;

    if (pinDelayMs == 0) {

    } else {
      prevActIdx = -1;
      lastActMs = now;
    }
  }
};
