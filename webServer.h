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

typedef std::function<void()> FileChangeCB;
FileChangeCB fileChangeCB;
ArBodyHandlerFunction SPIFFSSetter(const String &filename) {
  return [filename](AsyncWebServerRequest *request, uint8_t *data, size_t len,
                    size_t index, size_t total) {
    Serial.println("got body cb");
    Serial.print("len ");
    Serial.println(String(len));
    Serial.print("index ");
    Serial.println(String(index));
    Serial.print("total ");
    Serial.println(String(total));

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
          fileChangeCB();
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
  server.on("/agendaFile", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("getting agenda.json");
    request->send(SPIFFS, "/agenda.json", "text/plain");
  });

  //   // POST
  server.on(
      "/post/agendaFile", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        Serial.println("got agenda req");
        request->send(200);
      },
      nullptr, SPIFFSSetter("/agenda.json"));

  //   server.onRequestBody(onMyBody);
  server.begin();
}
