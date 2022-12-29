#pragma once
#include "./lib/JPI/src/API.hpp"
#include "ESPRTC.h"
#include "RelayAPI.h"
#include "SchedulerAPI.h"
#include <fstream>

struct RootAPI : public APIAndInstance<RootAPI>, public MapNode {

  RelayAPI relayApi;
  SchedulerAPI scheduleAPI;
#ifndef DISABLE_RTC
  ESPRTC rtc;
#endif
  bool isAgendaDisabled = false;
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
    rFunction<void, bool>("isAgendaDisabled", &RootAPI::setAgendaDisabled);

    rTrig("reboot", &RootAPI::reboot);

    // childs
    addChild("relay", &relayApi);
    addChild("schedule", &scheduleAPI);
#ifndef DISABLE_RTC
    addChild("rtc", &rtc);
#endif
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

  std::string niceName = "";
  bool isActivated = false;

  bool activate(bool b) {
    try {
      if (isActivated != b) {
        isActivated = b;

        Serial.print("[app] activating to  ");
        if (b)
          Serial.println("on  ");
        else
          Serial.println("off ");
        relayApi.setRelayState(b);
      }

    } catch (...) {
      std::exception_ptr e = std::current_exception();
      Serial.println("!!!!! error while activating");
      if (e)
        Serial.println(e.__cxa_exception_type()->name());
      return false;
    }
    return true;
  }

  void setTimeStr(std::string s) {
#ifndef DISABLE_RTC
    rtc.setTimeStr(s);
#endif
  }

  void setAgendaDisabled(bool b) {
    Serial.print("Agenda is ");
    if (b)
      Serial.println("disabled ");
    else
      Serial.println("enabled ");
    isAgendaDisabled = b;
    // else if(msg.address === "/isAgendaDisabled"){
    //   if(msg.args.length === 1){
    //     const a  = msg.args[0]

    //     isAgendaDisabled = a!=="0" && !!a
    //     console.log("isAgendaDisabled = ",isAgendaDisabled)
    //     if(!isAgendaDisabled){
    //       activate(!!getAgendaShouldActivate())
    //     }
    //   }
    //   }
  }
  // TODO get nicename from info.json

  void setHostName(const std::string &s) const {
    std::ofstream myfile("/spiffs/hostname.txt");
    if (myfile.is_open()) {
      myfile << s << "\n";
      myfile.close();
    } else {
      PRINTLN("!!! cant write hostname file");
    }
  }

  std::string getHostName() const {
    std::ifstream myfile("/spiffs/hostname.txt");
    std::string line = {};
    if (myfile.is_open()) {
      getline(myfile, line);
      myfile.close();
    } else {
      myfile.close();
      PRINTLN("!!! cant read hostname file");
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
