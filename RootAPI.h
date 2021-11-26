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
        "hostName", [this]() { return getHostName(); },
        [this](const std::string &s) { setHostName(s); });

    rGetter<int>("rssi", [this](RootAPI &) {
      // Serial.print("getting rssi  ");
      // Serial.println((int)WiFi.RSSI());
      return (int)WiFi.RSSI();
    });

    rFunction<void, std::string>("setTimeStr", &RootAPI::setTimeStr);

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

  void handle() override {
    for (auto c : childNodes) {
      // PRINT("handle up ");
      // PRINTLN(c->idInParent.c_str());
      c->handle();
    }
  }

  std::string niceName;
  bool isActivated = false;

  bool activate(bool b) {
    if (isActivated != b) {
      Serial.print(F("[app] activating to  "));
      Serial.println(b ? "on " : "off");
    }
    isActivated = b;
    relayApi.setRelayState(b);
  }

  void setTimeStr(std::string s) { rtc.setTimeStr(s); }

  // TODO get nicename from info.json

  void setHostName(const std::string &s) const {
    std::ofstream myfile("/spiffs/hostname.txt");
    if (myfile.is_open()) {
      myfile << s << "\n";
      myfile.close();
    } else {
      PRINTLN(F("!!! cant write hostname file"));
    }
  }

  std::string getHostName() const {
    std::ifstream myfile("/spiffs/hostname.txt");
    std::string line;
    if (myfile.is_open()) {
      getline(myfile, line);
      myfile.close();
    } else {
      PRINTLN(F("!!! cant read hostname file"));
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
