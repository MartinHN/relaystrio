#pragma once
#include "./lib/JPI/src/API.hpp"
#include "LightAPI.h"

struct RootAPI : public APIAndInstance<RootAPI>, public MapNode {

  LightAPI lightapi;
  RootAPI() : APIAndInstance<RootAPI>(this), MapNode() {

    rFunction<std::string>("getState", &RootAPI::getState);
    rFunction<void, std::string>("setState", &RootAPI::setState);

    // rFunction<std::string, std::string>("getFile", &RootAPI::getFile);
    // rFunction<bool, std::string, std::string>("saveFile",
    // &RootAPI::saveFile); rFunction<std::string>("ls", &RootAPI::ls);

    rTrig("reboot", &RootAPI::reboot);
    addChild("light", &lightapi);
  }

  RootAPI(RootAPI &) = delete;

  bool setup() { return lightapi.setup(); }

  void setState(std::string st) { APISerializer::stateToNode(this, st); }
  std::string getState() { return APISerializer::stateFromNode(this); }

  void reboot() {
    delay(1000);
    ESP.restart();
  }
};
