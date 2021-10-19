#pragma once
#include "./lib/JPI/src/API.hpp"

struct RelayAPI : public APIAndInstance<RelayAPI>, LeafNode {

  bool relayOn = false;
  const std::vector<int> relayPins = {15}; // add pins here
  RelayAPI() : APIAndInstance<RelayAPI>(this) {
    rGetSet<bool>(
        "activate", [this]() { return relayOn; },
        [this](const bool &b) {
          setRelayState(b);
        });
  }

  RelayAPI(RelayAPI &) = delete;

  bool setup() override {
    for (auto &p : relayPins)
      pinMode(p, OUTPUT);
    return true;
  }
  void setRelayState(bool b) {
    relayOn = b;
    Serial.print(F("[app] setting relay to  "));
    Serial.println(b ? "on " : "off");
    for (auto &p : relayPins)
      digitalWrite(p, relayOn);
  }
};
