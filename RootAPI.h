#pragma once
#include "./lib/JPI/src/API.hpp"
#include "ESPRTC.h"
#include "RelayAPI.h"
#include "SchedulerAPI.h"
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

  // void setState(std::string st) { APISerializer::stateToNode(this, st); }
  // std::string getState() { return APISerializer::stateFromNode(this); }

  void reboot() {
    delay(1000);
    ESP.restart();
  }
};
