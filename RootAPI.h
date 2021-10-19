#pragma once
#include "./lib/JPI/src/API.hpp"
#include "ESPRTC.h"
#include "RelayAPI.h"
#include "SchedulerAPI.h"
#include <fstream>
struct RootAPI : public APIAndInstance<RootAPI>, public MapNode {

  RelayAPI relayApi;
  SchedulerAPI scheduleAPI;
  ESPRTC rtc;
  RootAPI() : APIAndInstance<RootAPI>(this), MapNode() {

    // rFunction<std::string>("getState", &RootAPI::getState);
    // rFunction<void, std::string>("setState", &RootAPI::setState);

    // rFunction<std::string, std::string>("getFile", &RootAPI::getFile);
    // rFunction<bool, std::string, std::string>("saveFile",
    // &RootAPI::saveFile); rFunction<std::string>("ls", &RootAPI::ls);

    rGetSet<bool>(
        "activate", [this]() { return isActivated; },
        [this](const bool &b) { activate(b); });

    rGetSet<std::string>(
        "niceName", [this]() { return getNiceName(); },
        [this](const std::string &s) { setNiceName(s); });

    rGetter<int>("rssi", [this](RootAPI &) {
      // Serial.print("getting rssi  ");
      // Serial.println((int)WiFi.RSSI());
      return (int)WiFi.RSSI();
    });

    rTrig("reboot", &RootAPI::reboot);

    // childs
    addChild("relay", &relayApi);
    addChild("schedule", &scheduleAPI);
    addChild("rtc", &rtc);
  }

  RootAPI(RootAPI &) = delete;

  bool setup() override {
    bool res = true;
    for (auto c : childNodes) {
      PRINT("setting up ");
      PRINTLN(c->idInParent.c_str());
      res &= c->setup();
    }
    PRINTLN("root is all setup");
    return res;
  }

  std::string niceName;
  bool isActivated = false;

  bool activate(bool b) {
    Serial.print(F("[app] activating to  "));
    Serial.println(b ? "on " : "off");
    isActivated = b;
    relayApi.setRelayState(b);
  }

  void setNiceName(const std::string &s) const {
    std::ofstream myfile("/spiffs/nicename.txt");
    if (myfile.is_open()) {
      myfile << s << "\n";
      myfile.close();
    } else {
      PRINTLN(F("!!! cant write nicename file"));
    }
  }

  std::string getNiceName() const {
    std::ifstream myfile("/spiffs/nicename.txt");
    std::string line;
    if (myfile.is_open()) {
      getline(myfile, line);
      myfile.close();
    } else {
      PRINTLN(F("!!! cant write nicename file"));
    }
    return line;
  }

  // void setState(std::string st) { APISerializer::stateToNode(this, st); }
  // std::string getState() { return APISerializer::stateFromNode(this); }

  void reboot() {
    delay(1000);
    ESP.restart();
  }
};
