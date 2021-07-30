#pragma once
#include "./lib/JPI/src/API.hpp"

struct LightAPI : public APIAndInstance<LightAPI>, LeafNode {

  bool lightOn = false;
  const int lightPin = 15;
  LightAPI() : APIAndInstance<LightAPI>(this) {
    rGetSet<bool>(
        "switch", [this]() { return lightOn; },
        [this](const bool &b) { setLightState(b); });
    rTrig("reboot", &LightAPI::reboot);
  }

  LightAPI(LightAPI &) = delete;

  bool setup() {
    pinMode(lightPin, OUTPUT);
    return true;
  }
  void setLightState(bool b) {
    lightOn = b;
    digitalWrite(lightPin, lightOn);
  }
  void reboot() {
    delay(1000);
    ESP.restart();
  }
};
