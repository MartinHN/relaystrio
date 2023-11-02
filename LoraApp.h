#include "Dbg.h"
#include "LoraPhyE32.h"
auto dbgCntr = Dbg("[lora]");
#define dbg dbgCntr

namespace LoraApp {

int getNumberInHostName(std::string hn) {
  int n = 0;
  char *s = hn.data();
  while (*s && !isdigit(*s))
    s++;
  if (*s) {
    sscanf(s, "%d", &n);
  }
  return n;
}

void flushSerial() {
  while (Serial2.available())
    Serial2.read();
  Serial2.flush();
}
typedef enum { SYNC = 1, PING, PONG, ACTIVATE } MESSAGE_TYPE;
uint8_t loraUuid = 0;
void setup(std::string hostname) {

  int num = getNumberInHostName(hostname);
  loraUuid = num; // relay start with no offset
  dbg.print("uuid is ", loraUuid);

  LoraPhy.begin();

  LoraPhy.rxMode();
  LoraPhy.onTxFlag = []() {
    // keep listening
    LoraPhy.rxMode();
  };

  LoraPhy.onRxFlag = [] {
    // you can read received data as an Arduino String

    uint8_t type = 99;
    if (!LoraPhy.readNext(type)) {
      dbg.print("corrupted lora msg no type to read");
    }
    dbg.print("new msg", type);

    if ((type == MESSAGE_TYPE::PING)) {
      uint8_t uuid = 0;
      if (!LoraPhy.readNext(uuid)) {
        dbg.print("corrupted lora msg not type to read");
        flushSerial();
      }

      if (!(uuid == 255 || uuid == loraUuid)) {
        dbg.print("ignore ping");
        flushSerial();
        return;
      }
      dbg.print("sending pong");

      uint8_t active = 1;
      uint8_t pongMsg[] = {MESSAGE_TYPE::PONG, loraUuid, active};
      LoraPhy.send(pongMsg, sizeof(pongMsg));

      dbg.print("sent ack");

    } else if (type == MESSAGE_TYPE::SYNC) {

    } else if (type == MESSAGE_TYPE::PONG) {

    } else if (type == MESSAGE_TYPE::ACTIVATE) {

    } else {
      dbg.print("message not supported");
    }
    flushSerial();
    dbg.print("end lora msg");
  };
}

void end() { LoraPhy.end(); }

void handle() { LoraPhy.handle(); }

}; // namespace LoraApp

#undef dbg
