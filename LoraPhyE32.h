// #define LoRa_E32_DEBUG 1
// see
// /Users/mememe/Documents/Arduino/libraries/EByte_LoRa_E32_library/LoRa_E32.h

#include <LoRa_E32.h>
// serial2 messes with psram
#ifdef F
#undef F
#endif
#define F(x) x

#define FLUSH_SERIAL2 Serial2.flush()
auto dbgPhy = Dbg("[phy]");
#define dbg dbgPhy

byte lastPhyRespStatus = 1;
void auxInterrupt();

// byte numAllowedFails = 10;
bool chkLoc(byte status, int ln, const char *successStr = nullptr) noexcept {
  if (status == 1) {
    if (successStr != nullptr)
      dbg.print(successStr);
  } else {
    dbg.print("failed at", ln, ", code ",
              getResponseDescriptionByParams(status));
    // if (numAllowedFails-- <= 0)
    //   ESP.restart();
  }
  lastPhyRespStatus = status;
  return status == 1;
}

#define chk(x) chkLoc((x).code, __LINE__)
// ---------- esp32 pins --------------

volatile bool newLoraMsg = false;

struct LoraPhyClass {

  volatile unsigned long flagTxMs = 0, flagRxMs = 0;
  std::function<void()> onRxFlag = {};
  std::function<void()> onTxFlag = {};
  enum class PhyState { None, Tx, Rx };

  PhyState phyState = PhyState::None;
  bool isProcessingRxFlag = false;

  byte espTxPin = 17;
  byte espRxPin = 16;
  byte auxPin = 5;
  byte m0Pin = 19;
  byte m1Pin = 18;

  LoRa_E32 e32ttl;

  // LoRa_E32( txE32pin,  rxE32pin, HardwareSerial*,  auxPin,  m0Pin,  m1Pin,
  // bpsRate,  serialConfig = SERIAL_8N1);
  LoraPhyClass()
      : e32ttl(espRxPin, espTxPin, &Serial2, auxPin, m0Pin, m1Pin,
               UART_BPS_RATE_9600) {}

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
      // Serial2.updateBaudRate(serialsToTry[maxTries % serNum]);

      delay(1000);
    }
    if (maxTries != 0 && maxTries != 3) {
      dbg.print("!!!!!!!!! got serial", serialsToTry[(maxTries) % serNum]);
    }
    hasCriticalBug |= (maxTries == 0);
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

#define USE_CUSTOM_PARAMS
#ifndef USE_CUSTOM_PARAMS
    dbg.print("setting default lora parameters"); // added to be sure
    configuration.SPED.airDataRate =
        AIR_DATA_RATE_011_48; // AIR_DATA_RATE_010_24;
    configuration.SPED.uartBaudRate = UART_BPS_9600;
    configuration.OPTION.fec = FEC_1_ON;
#else
    // dbg.print("setting optimal lora parameters");
    dbg.print("setting default lora parameters"); // added to be sure
    configuration.CHAN = 0x00;
    configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
    configuration.SPED.uartBaudRate = UART_BPS_9600;
    // configuration.SPED.uartBaudRate = UART_BPS_115200;
    // int targetSerialBaud = 115200;
    configuration.OPTION.fec = FEC_0_OFF;
#endif
    if (!chk(e32ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_LOSE))) {
      dbg.print("could not write config");
      return false;
    }
    delay(100);
#ifdef USE_CUSTOM_PARAMS
    // Serial2.updateBaudRate(targetSerialBaud);
#endif
    dbg.print("new default config is");
    printConfig();

    c.close();
    return true;
    /*
       ResponseStructContainer c;
       c = e32ttl.getConfiguration();
       // It's important get configuration pointer before all other operation
       Configuration configuration = *(Configuration *)c.data;
       Serial.println(c.status.getResponseDescription());
       Serial.println(c.status.code);

       configuration.ADDL = 0x0;
       configuration.ADDH = 0x1;
       configuration.CHAN = 0x19;

       configuration.OPTION.fec = FEC_0_OFF;
       configuration.OPTION.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
       configuration.OPTION.ioDriveMode = IO_D_MODE_PUSH_PULLS_PULL_UPS;
       configuration.OPTION.transmissionPower = POWER_17;
       configuration.OPTION.wirelessWakeupTime = WAKE_UP_1250;

       configuration.SPED.airDataRate = AIR_DATA_RATE_011_48;
       configuration.SPED.uartBaudRate = UART_BPS_115200;
       configuration.SPED.uartParity = MODE_00_8N1;

       // Set configuration changed and set to not hold the configuration
       ResponseStatus rs = e32ttl.setConfiguration(configuration,
       WRITE_CFG_PWR_DWN_LOSE); Serial.println(rs.getResponseDescription());
       Serial.println(rs.code);
       */
  }

  void end() {
    // TODO?
  }

  void handle() {
    // Serial2.flush();
    if (phyState != PhyState::Rx)
      return;
    if (e32ttl.getMode() != MODE_TYPE::MODE_0_NORMAL) {
      dbg.print("wrong lora mode");
      e32ttl.setMode(MODE_TYPE::MODE_0_NORMAL);
    }
    if (newLoraMsg) {
      newLoraMsg = false;
      auto t = millis();
      int timeout = 300;
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
      flagRxMs = millis();
      if (onRxFlag) {
        isProcessingRxFlag = true;
        onRxFlag();
        isProcessingRxFlag = false;
      }
    }
  }

  uint32_t getTimeOnAirMs(size_t numBytes) const {
    return 1000; // TODO
  }

  bool isReceiving() const { return phyState == PhyState::Rx; }
  void rxMode() {
    // dbg.print("setting RX");
    e32ttl.setMode(MODE_TYPE::MODE_0_NORMAL);
    phyState = PhyState::Rx;
    FLUSH_SERIAL2;
  }

  void txMode() {
    // dbg.print("setting TX");
    // nothing to do?
    phyState = PhyState::Tx;
    FLUSH_SERIAL2;
  }

  uint32_t send(uint8_t *b, uint8_t len) {
    if (phyState != PhyState::Tx)
      txMode();
    lastSentPacketByteLen = len;

    // Send message
    auto beforeSend = millis();
    // !!!! next line is blocking
    chk(e32ttl.sendMessage(b, len));
    flagTxMs = millis();
    lastTimeToSendFullPacket = flagTxMs - beforeSend;
    if (onTxFlag)
      onTxFlag();
    FLUSH_SERIAL2;
    return lastTimeToSendFullPacket; // TODO:
                                     // radio.getTimeOnAir(lastSentPacketByteLen);
  }

  bool readNext(uint8_t &s) {

    // ResponseStructContainer rs = e32ttl.receiveMessage(1);
    // if (!chk(rs.status)) {
    //   rs.close();
    //   return false;
    // }

    // s = *(uint8_t *)rs.data;
    // rs.close();
    // return true;
    if (!Serial2.available())
      return false;
    Serial2.read(&s, 1);
    return true;
  }

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
  // dbg.print("Aux Up!");
  if ((LoraPhy.phyState == LoraPhyClass::PhyState::Rx) &&
      !LoraPhy.isProcessingRxFlag) {
    // dbg.print("msgInterrupt!");
    // if (Serial2.available()) {
    newLoraMsg = true;
    // LoraPhy.flagRxMs = millis();
    // if (LoraPhy.onRxFlag)
    //   LoraPhy.onRxFlag();
    // newLoraMsg = false;
    // }
  }
}
