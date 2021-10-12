#include <Arduino.h>

#define DBG_MSG 0

#if DBG_MSG
#define DBGMSG(x) PRINT(x)
#define DBGMSGLN(x) PRINTLN(x)
#else
#define DBGMSG(x)                                                              \
  {}
#define DBGMSGLN(x)                                                            \
  {}
#endif

#include "./lib/JPI/wrappers/esp/OSCAPI.h"
#include "./lib/JPI/wrappers/esp/connectivity.hpp"
#include "./lib/JPI/wrappers/esp/time.hpp"

#include "RootAPI.h"
OSCAPI OSCApiParser;
#include <string>

int relayPin = 3;

RootAPI root;

std::string type = "relay";
std::string myOSCID = "2";

void setup() {
  // Pour a bowl of serial
  Serial.begin(115200);
  connectivity::setup(type, myOSCID);

  delay(100);
  // esp_now_init();

  if (!root.setup()) {
    PRINTLN("error while setting up root");
  }
}

void loop() {
  // auto t = millis();

  if (connectivity::handleConnection()) {
    // PRINTLN(">>>>loop ok");
    OSCBundle bundle;
    if (connectivity::receiveOSC(bundle)) {

      for (int i = 0; i < bundle.size(); i++) {
        bool needAnswer = false;
        auto &msg = *bundle.getOSCMessage(i);
        DBGMSG("new msg : ");
        DBGMSGLN(OSCAPI::getAddress(msg).c_str());
        // bundle.getOSCMessage(i)->getAddress(OSCAPI::OSCEndpoint::getBuf());
        // DBGMSGLN(OSCAPI::OSCEndpoint::getBuf());
        auto res = OSCApiParser.processOSC(&root, msg, needAnswer);
#if 1
        if (needAnswer) {
          DBGMSGLN("try send resp");
          if (!bool(res)) {
            DBGMSGLN("invalid res returned from OSCApiParser resp");
          } else if (!res.res) {
            DBGMSGLN("no res returned from OSCApiParser resp");
          } else {
            msg.getAddress(OSCAPI::OSCEndpoint::getBuf());
            OSCMessage rmsg(OSCAPI::OSCEndpoint::getBuf());
            if (OSCApiParser.listToOSCMessage(TypedArgList(res.res), rmsg)) {
              connectivity::sendOSCResp(rmsg);
            }
          }
          DBGMSG("res : ");
          DBGMSGLN(res.toString().c_str());
        }
#endif
        DBGMSGLN("endOSC");
      }
    }

  } else {
    delay(10);
  }
}
