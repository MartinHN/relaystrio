#include <Arduino.h>

#define DBG_MSG 1

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

#include "webServer.h"
OSCAPI OSCApiParser;
#include <string>

int relayPin = 3;

RootAPI root;

std::string type = "relay";
// std::string myOSCID = "2";
bool firstValidConnection = false;
void setup() {
  // Pour a bowl of serial
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.println("SPIFFS inited");
  Serial.print("total bytes");
  Serial.println(String(SPIFFS.totalBytes()));
  Serial.print("used bytes");
  Serial.println(String(SPIFFS.usedBytes()));

  std::string hostName = root.getNiceName();
  if (hostName.size() == 0) {
    hostName = "defaultNiceName";
  }

  delay(100);
  connectivity::setup(type, hostName);

  delay(100);
  // esp_now_init();
  if (!root.setup()) {
    PRINTLN("error while setting up root");
  }
  root.rtc.onTimeChange = onTimeChange;
}

static void updateStateFromAgenda() {
  bool shouldBeActive = root.scheduleAPI.shouldBeOn();
  Serial.print("agend should be ");
  Serial.println(shouldBeActive ? "on" : "off");
  root.activate(shouldBeActive);
}

static void fileChanged() {
  Serial.println("file change cb");
  root.scheduleAPI.loadFromFileSystem();
  updateStateFromAgenda();
}

static void onTimeChange() {
  Serial.println("time change cb");
  updateStateFromAgenda();
}

void loop() {
  // auto t = millis();

  if (connectivity::handleConnection()) {
    if (!firstValidConnection) {
      initWebServer(fileChanged);
      firstValidConnection = true;
    }
    // PRINTLN(">>>>loop ok");
    OSCBundle bundle;
    if (connectivity::receiveOSC(bundle)) {

      for (int i = 0; i < bundle.size(); i++) {
        bool needAnswer = false;
        auto &msg = *bundle.getOSCMessage(i);
        DBGMSG(F("[msg] new msg : "));
        DBGMSGLN(OSCAPI::getAddress(msg).c_str());
        // bundle.getOSCMessage(i)->getAddress(OSCAPI::OSCEndpoint::getBuf());
        // DBGMSGLN(OSCAPI::OSCEndpoint::getBuf());
        auto res = OSCApiParser.processOSC(&root, msg, needAnswer);
        if (!bool(res)) {
          Serial.print("!!! message parsing err : ");
          Serial.println(res.errMsg.c_str());
        }
#if 1
        if (needAnswer) {
          DBGMSGLN(F("[msg] try send resp"));
          if (!bool(res)) {
            PRINTLN(F("[msg] invalid res returned from OSCApiParser resp"));
          } else if (!res.res) {
            PRINTLN(F("[msg]  no res returned from OSCApiParser resp"));
          } else {
            msg.getAddress(OSCAPI::OSCEndpoint::getBuf());
            OSCMessage rmsg(OSCAPI::OSCEndpoint::getBuf());
            if (OSCApiParser.listToOSCMessage(TypedArgList(res.res), rmsg)) {
              connectivity::sendOSCResp(rmsg);
              DBGOSC((String("sent resp : ") +
                      String(OSCAPI::OSCEndpoint::getBuf()) + " " +
                      String(res.toString().c_str()) + " to " +
                      connectivity::udpRcv.remoteIP().toString() + ":" +
                      String(connectivity::udpRcv.remotePort()))
                         .c_str());
            }
          }
          // DBGMSG("res : ");
          // DBGMSGLN(res.toString().c_str());
        }
#endif
        DBGMSGLN(F("[msg] end OSC"));
      }
    }
    yield();
    delay(10);

  } else {
    delay(10);
  }
}
