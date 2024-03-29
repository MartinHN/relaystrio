#include <Arduino.h>
// on esp devkit lora uses psram pins, disable them!!!
#ifdef F
#undef F
#endif
#define F(x) x

#include "Dbg.h"

auto dbgMain = Dbg("[main]");

// #define DISABLE_WIFI
// #define DISABLE_RTC
// #define DISABLE_BROWNOUT
// #define DISABLE_AGENDA

// #define CONFIG_BROWNOUT_DET_LVL_SEL_5 1

// brownout stuff
// #include "driver/rtc_cntl.h"
// #include "soc/rtc_cntl_reg.h"
// #include "soc/soc.h"

// #ifdef CONFIG_BROWNOUT_DET_LVL
// #define BROWNOUT_DET_LVL CONFIG_BROWNOUT_DET_LVL
// #else
// #define BROWNOUT_DET_LVL 5
// #endif // CONFIG_BROWNOUT_DET_LVL

#ifndef DISABLE_WIFI
#define OTA 1
#else
#define OTA 0
#endif

#if OTA
#include "OTAUpdater.h"
OTAUpdater myOTA;
#endif

#include "LoraApp.h"

#define DBG_MSG 1

#if DBG_MSG
#define DBGMSG(x) PRINT(x)
#define DBGMSGLN(x) PRINTLN(x)
#else
#define DBGMSG(x)                                                                                                                                    \
  {}
#define DBGMSGLN(x)                                                                                                                                  \
  {}
#endif

#include "./lib/JPI/wrappers/esp/OSCAPI.h"
#include "./lib/JPI/wrappers/esp/connectivity.hpp"
// #include "./lib/JPI/wrappers/esp/time.hpp"

#include "RootAPI.h"

// #include "esp32-hal-bt.h" // to disable bt
#include "FileHelpers.hpp"

#ifndef DISABLE_WIFI
#include "webServer.h"
OSCAPI OSCApiParser;
#endif
#include <string>

RootAPI root;

std::string type = "relay";

int checkAgendaTime = 1000;
unsigned long long lastCheckAgendaTime = 0;
// std::string myOSCID = "2";
bool firstValidConnection = false;

// void IRAM_ATTR brown(void *z) { Serial.println(">>>>>>>Brownout"); }

void fileChanged(const String &filename);

void setup() {

  // #ifdef DISABLE_BROWNOUT
  //   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable   detector
  // #else
  //   REG_WRITE(
  //       RTC_CNTL_BROWN_OUT_REG,
  //       RTC_CNTL_BROWN_OUT_ENA             /* Enable BOD */
  //           | RTC_CNTL_BROWN_OUT_PD_RF_ENA /* Automatically power down RF */
  //           /* Reset timeout must be set to >1 even if BOR feature is not
  //           used
  //            */
  //           | (2 << RTC_CNTL_BROWN_OUT_RST_WAIT_S) |
  //           (BROWNOUT_DET_LVL << RTC_CNTL_DBROWN_OUT_THRES_S));

  //   // ESP_ERROR_CHECK(rtc_isr_register(brown, NULL,
  //   // RTC_CNTL_BROWN_OUT_INT_ENA_M));

  //   // REG_SET_BIT(RTC_CNTL_INT_ENA_REG, RTC_CNTL_BROWN_OUT_INT_ENA_M);

  // #endif
  // Pour a bowl of serial

  Serial.begin(115200);

  if (!ESPFS::init())
    return;

  std::string hostName = root.getHostName();
  if (hostName.size() == 0) {
    char tmp[20];
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(tmp, "esp32-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    hostName = tmp;
  }

  LoraApp::setup(hostName);
  LoraApp::onSync = [](const String &tstr) { root.setTimeStr(tstr.c_str()); };
  LoraApp::onActivate = [](bool shouldBeActive) { root.activate(shouldBeActive); };
  LoraApp::onAgendaDisable = [](bool shouldBeDisabled) { root.isAgendaDisabled = shouldBeDisabled; };
  LoraApp::isActive = []() { return root.isActivated; };
  LoraApp::isAgendaDisabled = []() { return root.isAgendaDisabled; };
  LoraApp::getAgendaMD5 = []() { return ESPFS::getMD5ForFile("/agenda.json"); };
  LoraApp::onDisableWifi = [](bool shouldDisable) { connectivity::setWifiDisabled(shouldDisable); };
  LoraApp::onLoraNewAgenda = [](const String &data) {
    ESPFS::writeToFile("/agenda.json", data);
    fileChanged("/agenda.json");
  };

  // if (!btStop())
  //   PRINTLN("error while disabling bt");
#if OTA
  myOTA.setup(hostName);
#endif
  // delay(100);
#ifndef DISABLE_WIFI
  connectivity::setup(type, hostName);
#endif

  // delay(100);
  // esp_now_init();
  if (!root.setup()) {
    PRINTLN("error while setting up root");
  }
#ifndef DISABLE_RTC
  root.rtc.onTimeChange = onTimeChange;
#endif
  updateStateFromAgenda(false);
}

void updateStateFromAgenda(bool quiet) {
  if (root.isAgendaDisabled) {
    return;
  }
  bool shouldBeActive = root.scheduleAPI.shouldBeOn();
  // if (!quiet) {
  // Serial.print("agenda should be ");
  // if (shouldBeActive)
  //   Serial.println("on ");
  // else
  //   Serial.println("off");
  // }
  root.activate(shouldBeActive);
  // OSCMessage amsg("/activate");
  // amsg.add<int>(shouldBeActive ? 1 : 0);
  // connectivity::sendOSCResp(amsg);
}

void fileChanged(const String &filename) {
  Serial.print("file change cb");
  Serial.println(filename);
  if (filename.endsWith("agenda.json"))
    ESPFS::updateMD5ForFile("/agenda.json");
  // dbgMain.print("==========");
  // dbgMain.print(ESPFS::readFile("/agenda.json"));
  // dbgMain.print("----------");
  root.scheduleAPI.loadFromFileSystem();
  updateStateFromAgenda(false);
}

void onTimeChange() {
  Serial.println("time change cb");
  updateStateFromAgenda(false);
}

bool testPin = false;
unsigned long long lastCheckTest = 0;
int checkTestTime = 80;
void loop() {
  auto t = millis();
  Serial.flush();

#ifndef DISABLE_AGENDA
  if (((t - lastCheckAgendaTime) > checkAgendaTime)) {
    updateStateFromAgenda(false);
    lastCheckAgendaTime = t;
  }
#endif
  // if (((t - lastCheckTest) > checkTestTime)) {
  //   testPin = !testPin;
  //   root.activate(testPin);
  //   lastCheckTest = t;
  // }
  LoraApp::handle();
  root.handle();
#ifndef DISABLE_WIFI
  if (connectivity::handleConnection()) {
    if (!firstValidConnection) {
      initWebServer(fileChanged);
#if OTA
      myOTA.begin();
#endif
      firstValidConnection = true;
    }
#if OTA
    myOTA.handle();
#endif

    // PRINTLN(">>>>loop ok");
    OSCMessage msg = {};
    if (connectivity::receiveOSC(msg)) {

      bool needAnswer = false;
      DBGMSG("[msg] new msg : ");
      auto msgStr = OSCAPI::getAddress(msg);
      DBGMSGLN(msgStr.c_str());
      // bundle.getOSCMessage(i)->getAddress(OSCAPI::OSCEndpoint::getBuf());
      // DBGMSGLN(OSCAPI::OSCEndpoint::getBuf());
      auto res = OSCApiParser.processOSC(&root, msg, needAnswer);
      if (!bool(res)) {
        Serial.print("!!! message parsing err : ");
        Serial.println(res.errMsg.c_str());
      }
      if (needAnswer) {
        DBGMSGLN("[msg] try send resp");
        if (!bool(res)) {
          PRINTLN("[msg] invalid res returned from OSCApiParser resp");
        } else if (!res.res) {
          PRINTLN("[msg]  no res returned from OSCApiParser resp");
        } else {
          msg.getAddress(OSCAPI::OSCEndpoint::getBuf());
          OSCMessage rmsg(OSCAPI::OSCEndpoint::getBuf());
          if (OSCApiParser.listToOSCMessage(TypedArgList(res.res), rmsg)) {
            connectivity::sendOSCResp(rmsg);
            auto rString = res.toString();
            auto fullString = (String("sent resp : ") + String(OSCAPI::OSCEndpoint::getBuf()) + " " + String(rString.c_str()) + " to " +
                               connectivity::udpRcv.remoteIP().toString() + ":" + String(connectivity::udpRcv.remotePort()));
            DBGOSC(fullString.c_str());
          }
        }
        // DBGMSG("res : ");
        // auto rS = res.toString();
        // DBGMSGLN(rS.c_str());
      }
      DBGMSGLN("[msg] end OSC");
    }
    // yield();
    delay(1);

  } else {
    // yield();
    delay(1);
  }
#endif // DISABLE_WIFI
}
