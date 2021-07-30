#include <Arduino.h>

#include "./lib/JPI/wrappers/esp/OSCAPI.h"
#include "./lib/JPI/wrappers/esp/connectivity.hpp"
#include "./lib/JPI/wrappers/esp/time.hpp"

#include "LightAPI.h"
OSCAPI OSCApiParser;
#include <string>

int relayPin = 3;

LightAPI root;

std::string type = "light";
std::string myOSCID = "";

void setup() {
  // Pour a bowl of serial
  Serial.begin(115200);
  connectivity::setup(type, myOSCID);

  delay(100);

  if (!root.setup()) {
    PRINTLN("error while setting up root");
  }
}

void loop() {
  // auto t = millis();

  if (connectivity::handleConnection()) {
    OSCBundle bundle;
    if (connectivity::receiveOSC(bundle)) {

      for (int i = 0; i < bundle.size(); i++) {
        bool needAnswer = false;
        auto &msg = *bundle.getOSCMessage(i);
        Serial.print("new msg : ");
        Serial.println(OSCAPI::getAddress(msg).c_str());
        // bundle.getOSCMessage(i)->getAddress(OSCAPI::OSCEndpoint::getBuf());
        // Serial.println(OSCAPI::OSCEndpoint::getBuf());
        auto res = OSCApiParser.processOSC(&root, msg, needAnswer);
#if 1
        if (needAnswer) {
          Serial.println("try send resp");
          if (!bool(res)) {
            Serial.println("invalid res returned from OSCApiParser resp");
          } else if (!res.res) {
            Serial.println("no res returned from OSCApiParser resp");
          } else {
            msg.getAddress(OSCAPI::OSCEndpoint::getBuf());
            OSCMessage rmsg(OSCAPI::OSCEndpoint::getBuf());
            if (OSCApiParser.listToOSCMessage(TypedArgList(res.res), rmsg)) {
              connectivity::sendOSCResp(rmsg);
            }
          }
          Serial.print("res : ");
          Serial.println(res.toString().c_str());
        }
#endif
      }
      Serial.println("endOSC");
    } else {
      delay(1000);
    }
  }
}
