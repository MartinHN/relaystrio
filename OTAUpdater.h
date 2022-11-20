#include "version.h"
#include <ArduinoOTA.h>

class OTAUpdater {

public:
  void setup(const std::string &hostName) {

    ArduinoOTA.setMdnsEnabled(false); // do not pollute mdns
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
  }

  void begin() { ArduinoOTA.begin(); }
  void handle() { ArduinoOTA.handle(); }

private:
};
