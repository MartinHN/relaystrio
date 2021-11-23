#pragma once
#include "./lib/JPI/src/API.hpp"

struct RelayAPI : public APIAndInstance<RelayAPI>, LeafNode {
  static constexpr int pinDelayMs = 0;
  bool relayOn = false;
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
    if (pinDelayMs == 0)
      return;

    if ((lastActMs > 0) && (prevActIdx < int(relayPins.size()) - 1)) {

      auto now = millis();
      int idxAct = (now - lastActMs) / pinDelayMs;
      if (idxAct >= relayPins.size())
        idxAct = relayPins.size() - 1;

      for (int i = prevActIdx + 1; i <= idxAct; i++) {
        auto p = relayPins[i];
        Serial.print(F("[app] setting relay pin "));
        Serial.print(i);
        Serial.println(relayOn ? " on " : " off");
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
    relayOn = b;
    lastActMs = millis();
    prevActIdx = -1;
    Serial.print(F("[app] setting relay to  "));
    Serial.print(lastActMs);
    Serial.println(b ? "on " : "off");
    if (pinDelayMs == 0)
      for (auto &p : relayPins)
        digitalWrite(p, relayOn);
  }
};
