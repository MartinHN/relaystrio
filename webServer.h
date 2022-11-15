#include "./lib/JPI/wrappers/esp/TimeHelpers.hpp"
#include "./lib/JPI/wrappers/esp/connectivity.hpp"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
int webPort = connectivity::conf::localPort;
AsyncWebServer server(webPort);

// void onRequest(AsyncWebServerRequest *request) {
//   // Handle Unknown Request
//   if (request->path)
//     request->send(404);
// }

// void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len,
//             size_t index, size_t total) {
//   // Handle body
// }

// void onMyUpload(AsyncWebServerRequest *request, String filename, size_t
// index,
//                 uint8_t *data, size_t len, bool final) {
//   Serial.println("got upload cb");
//   // Handle upload
//   //   if (!index) {
//   //     Serial.printf("UploadStart: %s\n", filename.c_str());
//   //   }
//   //   for (size_t i = 0; i < len; i++) {
//   //     Serial.write(data[i]);
//   //   }
//   //   if (final) {
//   //     Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index +
//   len);
//   //   }

//   //   if (!index) {
//   //     request->_tempFile = SPIFFS.open(filename, "w");
//   //   }
//   //   if (request->_tempFile) {
//   //     if (len) {
//   //       request->_tempFile.write(data, len);
//   //     }
//   //     if (final) {
//   //       request->_tempFile.close();
//   //     }
//   //   }
// }

typedef std::function<void(const String &name)> FileChangeCB;
FileChangeCB fileChangeCB;
ArBodyHandlerFunction SPIFFSSetter(const String &filename) {
  return [filename](AsyncWebServerRequest *request, uint8_t *data, size_t len,
                    size_t index, size_t total) {
    Serial.println("got body cb");
    Serial.print("len ");
    auto lS = String(len);
    Serial.println(lS);
    Serial.print("index ");
    auto iS = String(index);
    Serial.println(iS);
    Serial.print("total ");
    auto tS = String(total);
    Serial.println(tS);

    if (!index) {
      Serial.printf("BodyStart: %u B\n", total);
      request->_tempFile = SPIFFS.open(filename, "w");

      if (!SPIFFS.exists(filename)) {
        Serial.print("can not create file ");
        Serial.println(filename);
      }
    }
    if (request->_tempFile) {
      if (len) {
        request->_tempFile.write(data, len);
      }
      if (index + len == total) {
        Serial.printf("BodyEnd: %u B\n", total);
        request->_tempFile.close();
        if (fileChangeCB)
          fileChangeCB(filename);
      }
    } else {
      Serial.println("can not get temp file");
    }
  };
}

void initWebServer(FileChangeCB cb) {
  fileChangeCB = cb;
  Serial.print("server listening on ");
  Serial.println(webPort);
  // Send a GET request to <IP>/get?message=<message>
  // agendaFile
  server.on("/agendaFile", HTTP_GET, [](AsyncWebServerRequest *req) {
    Serial.println("getting agenda.json");
    req->send(SPIFFS, "/agenda.json", "text/plain");
  });
  server.on(
      "/post/agendaFile", HTTP_POST,
      [](AsyncWebServerRequest *req) {
        Serial.println("got agenda req");
        req->send(200);
      },
      nullptr, SPIFFSSetter("/agenda.json"));

  // info
  server.on("/info", HTTP_GET, [](AsyncWebServerRequest *req) {
    Serial.println("getting agenda.json");
    req->send(SPIFFS, "/info.json", "application/json");
  });
  server.on(
      "/post/info", HTTP_POST,
      [](AsyncWebServerRequest *req) {
        Serial.println("got info req");
        req->send(200);
      },
      nullptr, SPIFFSSetter("/info.json"));

  // // niceName
  // server.on("/niceName", HTTP_GET, [](AsyncWebServerRequest *req) {
  //   Serial.println("getting niceName.txt");
  //   req->send(SPIFFS, "/niceName.txt", "text/plain");
  // });

  // //   // POST
  // server.on(
  //     "/post/niceName", HTTP_POST,
  //     [](AsyncWebServerRequest *req) {
  //       Serial.println("got niceName req");
  //       req->send(200);
  //     },
  //     nullptr, SPIFFSSetter("/niceName.txt"));

  // time

  server.on("/time", HTTP_GET, [](AsyncWebServerRequest *req) {
    // struct tm timeinfo;
    time_t epoch(time(nullptr));

    tm *local = localtime(&epoch);
    String localS(asctime(local));
    localS.trim();
    tm *utc = gmtime(&epoch);
    String utcS(asctime(utc));
    utcS.trim();
    // if (!getLocalTime(&timeinfo)) {
    //   PRINT("no time found stuck at :");
    //   PRINTLN(asctime(&timeinfo));
    //   return req->send(404);
    // }
    String timeStr = "{\n";
    timeStr += "\"localTime\":\"" + localS + "\"";
    timeStr += ",\n";
    timeStr += "\"utcTime\":\"" + utcS + "\"";
    timeStr += "\n}";
    Serial.print("getting time : ");
    Serial.println(timeStr);
    req->send(200, "application/json", timeStr);
  });

  // CORS Bullshit
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404);
    }
  });
  //   server.onRequestBody(onMyBody);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader(
      "Access-Control-Allow-Method", "GET, POST, PATCH, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Header",
                                       "Origin, Content-Type, X-Auth-Token");
  server.begin();
}
