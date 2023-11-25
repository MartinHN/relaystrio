#define LoRa_E32_DEBUG 1
// see
// /Users/mememe/Documents/Arduino/libraries/EByte_LoRa_E32_library/LoRa_E32.h
// serial2 messes with psram
#ifdef F
#undef F
#endif
#define F(x) x

#include "lib/COBS-CPP/src/cobs.h"
#define FREQUENCY_170
#include <LoRa_E32.h>

HardwareSerial HWSerial(1);

#define FLUSH_HW_SERIAL HWSerial.flush()
auto dbgPhy = Dbg("[phy]");
#define dbg dbgPhy

byte lastPhyRespStatus = 1;
void auxInterrupt();

bool chkLoc(byte status, int ln, const char *successStr = nullptr) noexcept {
  if (status == 1) {
    if (successStr != nullptr)
      dbg.print(successStr);
  } else {
    dbg.print("failed at", ln, ", code ", getResponseDescriptionByParams(status));
  }
  lastPhyRespStatus = status;
  return status == 1;
}

#define chk(x) chkLoc((x).code, __LINE__)
// ---------- esp32 pins --------------

volatile bool newLoraMsg = false;

uint8_t loraMsgBuf[253];
uint8_t loraMsgSize = 0;
uint8_t readIdx = 0;
uint8_t loraMsgOutBuf[255];
struct LoraPhyClass {

  bool ignoreLora = false;
  volatile unsigned long flagTxMs = 0, flagRxMs = 0;
  std::function<void()> onRxFlag = {};
  // std::function<void()> onTxFlag = {};
  enum class PhyState { None, Tx, Rx };

  volatile PhyState phyState = PhyState::None;
  volatile bool isProcessingRxFlag = false;

#if M5STACK_CORE1
  byte espTxPin = 17;
  byte espRxPin = 16;
  byte auxPin = 5;
  byte m0Pin = 26;
  byte m1Pin = 2;
#else
  byte espTxPin = 17;
  byte espRxPin = 16;
  byte auxPin = 5;
  byte m0Pin = 19;
  byte m1Pin = 18;
#endif

  LoRa_E32 e32ttl;

  // LoRa_E32( txE32pin,  rxE32pin, HardwareSerial*,  auxPin,  m0Pin,  m1Pin,
  // bpsRate,  serialConfig = SERIAL_8N1);
  LoraPhyClass() : e32ttl(espRxPin, espTxPin, &HWSerial, auxPin, m0Pin, m1Pin, UART_BPS_RATE_9600) {}

  bool hasCriticalBug = false;
  void begin() {
    dbg.print("begin");
    if (!e32ttl.begin()) {
      dbg.print("can't start EByte");
      while (1) {
      }
    }
    pinMode(auxPin, INPUT_PULLUP);

    delay(400);

    int maxTries = 3;

    unsigned long serialsToTry[] = {9600, 115200};
    int serNum = 2;
    while (!printConfig() && maxTries-- > 0) {
      dbg.print("no com with E32");

      hasCriticalBug |= !chk(e32ttl.resetModule());
      // HWSerial.updateBaudRate(serialsToTry[maxTries % serNum]);

      delay(1000);
    }
    if (maxTries != 0 && maxTries != 3) {
      dbg.print("!!!!!!!!! got serial", serialsToTry[(maxTries) % serNum]);
    }
    hasCriticalBug |= (maxTries <= 0);
    hasCriticalBug |= !applyLoraConfig();
    if (hasCriticalBug) {
      dbg.print("lora module buuuuuuggged");
      ESP.restart();
    }
    attachInterrupt(auxPin, auxInterrupt, RISING);
  }

  bool applyLoraConfig() {

    ResponseStructContainer c;
    c = e32ttl.getConfiguration();
    if (!chk(c.status))
      return false;
    // It's important get configuration pointer before all other operation
    Configuration configuration = *(Configuration *)c.data;

    // dbg.print("setting optimal lora parameters");
    dbg.print("setting default lora parameters"); // added to be sure
    // configuration.HEAD = 0x00; conf save mode
    configuration.ADDH = 0x00;
    configuration.ADDL = 0x00;
    configuration.CHAN = 0x00;
    configuration.SPED.uartParity = MODE_00_8N1;
    configuration.SPED.uartBaudRate = UART_BPS_9600;
    configuration.SPED.airDataRate = AIR_DATA_RATE_010_24; // AIR_DATA_RATE_010_24; AIR_DATA_RATE_110_192
    configuration.OPTION.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
    configuration.OPTION.ioDriveMode = IO_D_MODE_PUSH_PULLS_PULL_UPS; // IO_D_MODE_OPEN_COLLECTOR , IO_D_MODE_PUSH_PULLS_PULL_UPS;
    configuration.OPTION.wirelessWakeupTime = WAKE_UP_250;
    configuration.OPTION.fec = FEC_0_OFF; // FEC_1_ON; // FEC_0_OFF;
    configuration.OPTION.transmissionPower = POWER_20;
    // custom baudrate
    int targetSerialBaud = 0;
#if 0
    configuration.SPED.uartBaudRate = UART_BPS_115200;
    targetSerialBaud = 115200;
#endif

    if (!chk(e32ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_LOSE))) {
      dbg.print("could not write config");
      return false;
    }
    delay(100);
    if (targetSerialBaud > 0)
      HWSerial.updateBaudRate(targetSerialBaud);

    dbg.print("new config is");
    printConfig();

    c.close();
    return true;
  }

  void end() {
    // TODO?
  }

  void handle() {
    // HWSerial.flush();
    if (ignoreLora)
      return;
    if (e32ttl.getMode() != MODE_TYPE::MODE_0_NORMAL) {
      dbg.print("!!!!!!!!! wrong lora mode");
      e32ttl.setMode(MODE_TYPE::MODE_0_NORMAL);
    }
    if (!newLoraMsg && HWSerial.available()) {
      dbg.print("serial available, just activate");
      newLoraMsg = true;
    } else if (phyState != PhyState::Rx) {
      if (newLoraMsg)
        dbg.print("buggy new loraMsg");
      // else
      // dbg.print("ignore lora handle in non Rx");
      return;
    }
    if (!newLoraMsg)
      return;
    newLoraMsg = false;
    auto t = millis();
    int timeout = 300;
    if (e32ttl.available() == 0) {
      dbg.print("waiting serial");
    }
    while (!(e32ttl.available() > 0)) {
      if ((millis() - t) > timeout) {
        dbg.print("!!!Timeout error!");
        return;
      }
    }
    if (!(e32ttl.available() > 0)) {
      dbg.print("nothing available");
      return;
    }

    // dbg.print("parsing new msg");
    auto serialSize = HWSerial.readBytesUntil(0, loraMsgBuf, sizeof(loraMsgBuf));
    if (serialSize <= 1) {
      dbg.print("no message to read!!");
      loraMsgSize = 0;
      return;
    }
    if (serialSize >= 255) {
      dbg.print("too much to read!!");
      loraMsgSize = 0;
      return;
    }
    while (serialSize > 1) {
      auto cobsSize = cobs::decode(loraMsgBuf, serialSize);
      if (cobsSize <= 1 || cobsSize > 255) {
        dbg.print("!!!!weird cobs msg size", cobsSize);
        loraMsgSize = 0;
        return;
      }
      loraMsgSize = cobsSize;
      // dbg.print("cobs");
      // dbg.printBuffer(loraMsgBuf, loraMsgSize);
      // dbg.print("decoded");
      // dbg.printBuffer(loraMsgBuf, loraMsgSize);
      readIdx = 1; // first is not overiden
                   // flagRxMs = millis();
      isProcessingRxFlag = true;
      onRxFlag();
      isProcessingRxFlag = false;

      loraMsgSize = 0;
      serialSize = 0;
      if (HWSerial.available()) {
        serialSize = HWSerial.readBytesUntil(0, loraMsgBuf, sizeof(loraMsgBuf));
        dbg.print("got extra msg size : ", serialSize);
      }
    }
  }

  uint32_t getTimeOnAirMs(size_t numBytes) const {
    return 1000; // TODO
  }

  bool isReceiving() const { return phyState == PhyState::Rx; }
  void rxMode() {
    // dbg.print("setting RX");
    // e32ttl.setMode(MODE_TYPE::MODE_0_NORMAL);
    phyState = PhyState::Rx;
    // FLUSH_HW_SERIAL;
  }

  void txMode() {
    // dbg.print("setting TX");
    // nothing to do?
    phyState = PhyState::Tx;
    // FLUSH_HW_SERIAL;
  }

  uint32_t send(uint8_t *b, uint8_t len) {
    if (len > 250) {
      dbg.print("error to big of a message ");
      return 0;
    }
    if (phyState != PhyState::Tx)
      txMode();
    uint8_t lenOnWire = len + 2; // cobs  and trailing zero delimiter
    lastSentPacketByteLen = lenOnWire;
    for (int i = 0; i < len; i++)
      loraMsgOutBuf[i + 1] = b[i];
    cobs::encode(loraMsgOutBuf, len + 1);
    // Send message
    auto beforeSend = millis();
    // !!!! next line is blocking
    // chk(e32ttl.sendMessage(&loraMsgOutBuf, len + 1));
    loraMsgOutBuf[lenOnWire - 1] = 0;
    dbg.print("sending lora : ");
    dbg.printBuffer(loraMsgOutBuf, lenOnWire);
    HWSerial.write(loraMsgOutBuf, size_t(lenOnWire));
    HWSerial.flush();
    flagTxMs = millis();
    lastTimeToSendFullPacket = flagTxMs - beforeSend;
    // if (onTxFlag)
    //   onTxFlag();
    // FLUSH_HW_SERIAL;
    return lastTimeToSendFullPacket; // TODO:
                                     // radio.getTimeOnAir(lastSentPacketByteLen);
  }

#define MAX_MSG_SIZE 59

  bool readNext(uint8_t &s) {
    if (readIdx > loraMsgSize) // last element is valid
      return false;
    s = loraMsgBuf[readIdx];

    readIdx += 1;
    if (readIdx > MAX_MSG_SIZE) {
      dbg.print("!!!! overflow of msg");
      return false;
    }
    return true;
  }

  String readString() {
    String res;

    if (readIdx > loraMsgSize) // last element is valid
    {
      dbg.print("no string to parse");
      return {};
    }

    while ((readIdx <= loraMsgSize) && (loraMsgBuf[readIdx] != 0)) {
      res += String(char(loraMsgBuf[readIdx]));
      readIdx++;
      if (readIdx > MAX_MSG_SIZE) {
        dbg.print("!!!! overflow of msg");
        return {};
      }
    }
    if (loraMsgBuf[readIdx] != 0 && (readIdx + 1 != loraMsgSize))
      dbg.print("!!!!string not terminated with 0 ::", readIdx, loraMsgSize);
    // ignore last zero
    readIdx++;

    return res;
  }

  // void discardRemainingBytes() {
  //   uint8_t s;
  //   while (HWSerial.available())
  //     HWSerial.read(&s, 1);
  // }

  bool printConfig() {
    ResponseStructContainer c;
    c = e32ttl.getConfiguration();
    if (!chk(c.status)) {
      return false;
    }
    // It's important get configuration pointer before all other operation
    Configuration configuration = *(Configuration *)c.data;
    static_assert(sizeof(Configuration) == 6);
    dbg.printWithSpace(F("original hex : 0x"));
    for (int i = 0; i < 6; i++) {
      Serial.print(((uint8_t *)(&configuration))[i], HEX);
      Serial.print(" ");
    }
    Serial.print(F("\n"));
    dbg.printWithSpace(F("Chan : "));
    Serial.print(configuration.CHAN, DEC);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.getChannelDescription());
    dbg.printWithSpace(F("SpeedParityBit     : "));
    Serial.print(configuration.SPED.uartParity, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.SPED.getUARTParityDescription());
    dbg.printWithSpace(F("SpeedUARTDatte  : "));
    Serial.print(configuration.SPED.uartBaudRate, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.SPED.getUARTBaudRate());
    dbg.printWithSpace(F("SpeedAirDataRate   : "));
    Serial.print(configuration.SPED.airDataRate, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.SPED.getAirDataRate());

    dbg.printWithSpace(F("OptionTrans        : "));
    Serial.print(configuration.OPTION.fixedTransmission, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getFixedTransmissionDescription());
    dbg.printWithSpace(F("OptionPullup       : "));
    Serial.print(configuration.OPTION.ioDriveMode, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getIODroveModeDescription());
    dbg.printWithSpace(F("OptionWakeup       : "));
    Serial.print(configuration.OPTION.wirelessWakeupTime, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getWirelessWakeUPTimeDescription());
    dbg.printWithSpace(F("OptionFEC          : "));
    Serial.print(configuration.OPTION.fec, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getFECDescription());
    dbg.printWithSpace(F("OptionPower        : "));
    Serial.print(configuration.OPTION.transmissionPower, BIN);
    dbg.printWithSpace(" -> ");
    dbg.print(configuration.OPTION.getTransmissionPowerDescription());
    return true;
  }

  size_t lastSentPacketByteLen = 0;
  int32_t lastTimeToSendFullPacket = 0;
};

LoraPhyClass LoraPhy;

void IRAM_ATTR auxInterrupt() {
  if (LoraPhy.phyState == LoraPhyClass::PhyState::Tx) {
    LoraPhy.rxMode();
    // dbg.print("msg fully sent");
    return;
  }
  if ((LoraPhy.phyState == LoraPhyClass::PhyState::Rx) && !LoraPhy.isProcessingRxFlag) {
    newLoraMsg = true;
    // LoraPhy.flagRxMs = millis();
  }
}
