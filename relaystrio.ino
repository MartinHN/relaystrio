#include <Arduino.h>

#define OTA 1

#if OTA
#include <ArduinoOTA.h>
#endif

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
// #include "./lib/JPI/wrappers/esp/time.hpp"

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

  std::string hostName = root.getHostName();
  if (hostName.size() == 0) {
    char tmp[20];
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(tmp, "esp32-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);
    hostName = tmp;
  }

#if OTA
  ArduinoOTA.setHostname(hostName.c_str());
  ArduinoOTA.onStart([]() { Serial.println("[ota] Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("[ota] \nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("%u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[ota] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("[ota] Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("[ota] Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("[ota] Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("[ota] Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("[ota] End Failed");
  });
#endif
  // delay(100);
  connectivity::setup(type, hostName);

  // delay(100);
  // esp_now_init();
  if (!root.setup()) {
    PRINTLN("error while setting up root");
  }
  root.rtc.onTimeChange = onTimeChange;

  updateStateFromAgenda();
}

static void updateStateFromAgenda() {
  bool shouldBeActive = root.scheduleAPI.shouldBeOn();
  Serial.print("agenda should be ");
  Serial.println(shouldBeActive ? "on" : "off");
  root.activate(shouldBeActive);
  // OSCMessage amsg("/activate");
  // amsg.add<int>(shouldBeActive ? 1 : 0);
  // connectivity::sendOSCResp(amsg);
}

static void fileChanged(const String &filename) {
  Serial.print("file change cb");
  Serial.println(filename);
  root.scheduleAPI.loadFromFileSystem();
  updateStateFromAgenda();
}

static void onTimeChange() {
  Serial.println("time change cb");
  updateStateFromAgenda();
}

void loop() {
  // auto t = millis();
  root.handle();
  if (connectivity::handleConnection()) {
    if (!firstValidConnection) {
      initWebServer(fileChanged);
#if OTA
      ArduinoOTA.begin();
#endif
      firstValidConnection = true;
    }
#if OTA
    ArduinoOTA.handle();
#endif

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
    // delay(10);

  } else {
    yield();
    // delay(10);
  }
}
