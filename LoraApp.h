#include "Dbg.h"
#include "LoraPhyE32.h"

auto dbgCntr = Dbg("[lora]");
#define dbg dbgCntr
#define HAVE_ZIP 0
#if HAVE_ZIP
// needs zlib-esp32
#include "gzip/config.hpp"
#include "gzip/decompress.hpp"
#include "gzip/utils.hpp"
#include "gzip/version.hpp"
// compile them too
// #include <zlib.h>

String unzip(const String &from) {
  dbg.print("!!!!!!!!!!unziping", from.length());

  bool c = gzip::is_compressed(from.c_str(), from.length()); // false)

  if (!c) {
    dbg.print('not compressed');
    return from;
  }
  std::string decomp;
  try {
    decomp = gzip::decompress(from.c_str(), from.length());
  } catch (...) {
    std::exception_ptr e = std::current_exception();
    dbg.print('error uncompressing', e.__cxa_exception_type()->name());
    decomp = {};
  }
  return String(decomp.c_str());
}
#endif

struct FileRcvT {
  uint8_t numMessageToWait = 0;
  std::vector<String> msg = {};

  bool hasInvalidState() {
    if (numMessageToWait != msg.size()) {
      dbg.print("!!!!!!invalid num message to wait");
      return true;
    }
    if (numMessageToWait == 0) {
      dbg.print("!!!!!!no message to wait");
      return true;
    }

    return false;
  }

  void cleanUp() { numMessageToWait = 0; }
  std::vector<uint8_t> getMissingIdces() {
    if (hasInvalidState())
      return {0};
    std::vector<uint8_t> res;
    int i = 0;
    for (const auto &p : msg) {
      if (p.length() == 0)
        res.push_back(i);

      i++;
    }
    return res;
  }
  bool hasFullMsg() {
    if (!hasInvalidState())
      return false;
    for (const auto &p : msg)
      if (p.length() == 0)
        return false;

    return true;
  }

  String getCompleteMessage() {
    if (hasInvalidState())
      return {};
    String res = {};
    for (const auto &p : msg) {
      if (p.length() == 0) {
        dbg.print("!!!!!!! empty message aborting");
        return {};
      }
      res += p;
    }
    res.trim();
    return res;
  }

  void startRcv(uint8_t numMessageToWait_) {
    numMessageToWait = numMessageToWait_;
    if (numMessageToWait < 1 || numMessageToWait > 250) {
      dbg.print("invalid number of parts for file");
      return;
    }
    msg.clear();
    msg.resize(numMessageToWait);
  }

  void ignoreFile() { numMessageToWait = 0; }
  bool isAcceptingFile() { return numMessageToWait != 0; }

  String addPart(uint8_t idx, const String &part) {
    if (hasInvalidState())
      return {};

    if (idx >= msg.size()) {
      dbg.print("invalid part recieved", idx);
      return {};
    }
    if (part.length() == 0) {
      dbg.print("empty part recieved", idx);
      return {};
    }
    auto &entry = msg[idx];
    dbg.print("adding part", part);
    if (entry.length() > 0) {
      dbg.print("will override data at idx", idx);
      return {};
    }

    entry = part;

    auto missing = getMissingIdces();
    if (missing.size() == 0) {
      auto str = getCompleteMessage();
#if HAVE_ZIP
      return unzip(str);
#else
      return str;
#endif
    } else {
      dbg.print("num Missing", missing.size());
      for (const auto &m : missing)
        dbg.print("    ", m);
    }
    return {};
  }

}; // sruct FileRcvT
namespace LoraApp {

static FileRcvT FileRcv;

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

typedef enum { SYNC = 1, PING, PONG, ACTIVATE, DISABLE_AGENDA, FILE_MSG } MESSAGE_TYPE;

static std::function<void(const String &)> onSync;
static std::function<void(bool)> onActivate;
static std::function<void(bool)> onAgendaDisable;
static std::function<bool()> isActive;
static std::function<bool()> isAgendaDisabled;
static std::function<String()> getAgendaMD5;
static std::function<void(bool)> onDisableWifi;
static std::function<void(const String &)> onLoraNewAgenda;

void flushSerial() {}
uint8_t loraUuid = 0;

#define FLUSH_RETURN_ERR(x)                                                                                                                          \
  {                                                                                                                                                  \
    dbg.print(x);                                                                                                                                    \
    return;                                                                                                                                          \
  }

// dbg.printBuffer(loraMsgBuf, loraMsgSize);

void processFileLoraMsg(FileRcvT &fileRcv) {
  uint8_t partNumber = 0;
  // dbg.printBuffer(loraMsgBuf, loraMsgSize);
  if (!LoraPhy.readNext(partNumber))
    FLUSH_RETURN_ERR("corrupted file msg");

  // dbg.print("   got file msg num :", int(partNumber));
  // dbg.print(partNumber);
  // dbg.printBuffer(loraMsgBuf, loraMsgSize);
  // Serial.printf("Lora() - Free Stack Space: %d\n", uxTaskGetStackHighWaterMark(NULL));

  bool isStartOfFile = partNumber == 255;
  if (isStartOfFile) {

    uint8_t numFileMessages = 0;
    if (!LoraPhy.readNext(numFileMessages))
      FLUSH_RETURN_ERR("corrupted file msg invalid start num");

    dbg.print("   will need :?", numFileMessages);

    uint8_t uuid;
    bool shouldAccept = false;
    while (LoraPhy.readNext(uuid)) {
      if (uuid == 255 || uuid == loraUuid) {
        shouldAccept = true;
        break;
      }
    }
    if (shouldAccept) {
      dbg.print("   start Receive");
      fileRcv.startRcv(numFileMessages);
    } else {
      dbg.print("   ignore Receive");
      fileRcv.ignoreFile();
    }
  } else if (fileRcv.isAcceptingFile()) {
    auto part = LoraPhy.readString();
    dbg.print("   adding part", partNumber);
    auto res = fileRcv.addPart(partNumber, part);
    if (res.length()) {
      onLoraNewAgenda(res);
      fileRcv.cleanUp();
    }
  } else {
    dbg.print("   ignoring file msg");
    fileRcv.cleanUp();
  }
}

void setup(std::string hostname) {

  int num = getNumberInHostName(hostname);
  loraUuid = num; // relay start with no offset
  dbg.print("uuid is ", loraUuid);

  LoraPhy.begin();

  LoraPhy.rxMode();
  // LoraPhy.onTxFlag = []() {
  //   // keep listening
  //   LoraPhy.rxMode();
  // };

  LoraPhy.onRxFlag = [] {
    uint8_t msgType = 99;
    if (!LoraPhy.readNext(msgType))
      FLUSH_RETURN_ERR("corrupted lora msg no msgType to read");

    dbg.print("new msg", msgType);

    if ((msgType == MESSAGE_TYPE::PING)) {

      uint8_t isAgendaDisabled;
      if (!LoraPhy.readNext(isAgendaDisabled))
        FLUSH_RETURN_ERR("   corrupted ping msg no isAgendaDisabled to read");
      onAgendaDisable(isAgendaDisabled > 0);
      uint8_t pingType;
      if (!LoraPhy.readNext(pingType))
        FLUSH_RETURN_ERR("   corrupted ping msg no pingType to read");
      bool shouldSendMD5 = pingType == 1;
      if (pingType > 1)
        FLUSH_RETURN_ERR(dbg.toStr("   corrupted ping msg invalid pingType", pingType));
      uint8_t pingFlags;
      if (!LoraPhy.readNext(pingFlags))
        FLUSH_RETURN_ERR("   corrupted ping msg no pingFlags to read");

      uint8_t slotTimecentiSeconds;
      if (!LoraPhy.readNext(slotTimecentiSeconds))
        FLUSH_RETURN_ERR("   corrupted ping msg no slot time to read");

      int idxInList = -1;
      uint8_t count = 0;
      uint8_t uuid = 0;
      while (LoraPhy.readNext(uuid)) {
        if (uuid == 255 || uuid == loraUuid) {
          idxInList = count;
          break;
        }
        count++;
      }

      if (idxInList == -1) {
        dbg.print("   ignore ping", idxInList);
        return;
      }
      bool shouldDisableWifi = pingFlags == 1;
      if (pingFlags > 1)
        FLUSH_RETURN_ERR(dbg.toStr("   corrupted ping msg invalid pingFlags", pingFlags));
      onDisableWifi(shouldDisableWifi);

      // int sinceTrueRcvTime=millis()- LoraPhy.flagRxMs;
      int delayMs = idxInList * (int(slotTimecentiSeconds) * 10);
      if (delayMs > 5000)
        FLUSH_RETURN_ERR(dbg.toStr("   !!!!delayMs seems wrong", delayMs, "=", idxInList, "*", slotTimecentiSeconds));

      dbg.print("   sending pong in ", delayMs);
      if (delayMs > 0)
        delay(delayMs);
      bool active = false;
      if (isActive)
        active = isActive();
      else
        dbg.print("WTFFFFF");

      std::vector<uint8_t> pongMsg = {MESSAGE_TYPE::PONG, loraUuid, active};
      if (shouldSendMD5) {
        auto md5 = getAgendaMD5();
        for (int i = 0; i < md5.length(); i++) {
          pongMsg.push_back(md5.charAt(i));
        }
        pongMsg.push_back(0);

        if (FileRcv.numMessageToWait)
          for (const auto &m : FileRcv.getMissingIdces())
            pongMsg.push_back(m);
      }
      LoraPhy.send(pongMsg.data(), pongMsg.size());
      dbg.print("   sent pong");

    } else if (msgType == MESSAGE_TYPE::SYNC) {
      auto strDate = LoraPhy.readString();
      dbg.print("   got time", strDate);
      if (onSync)
        onSync(strDate);

    } else if (msgType == MESSAGE_TYPE::PONG) {
      // Ignore pong

    } else if (msgType == MESSAGE_TYPE::ACTIVATE) {
      uint8_t uuid = 0;
      LoraPhy.readNext(uuid);
      if (uuid == 255 || uuid == loraUuid) {
        uint8_t shouldAct;
        LoraPhy.readNext(shouldAct);
        dbg.print("   got act", shouldAct);
        if (onActivate)
          onActivate(shouldAct);
        if (uuid != 255) {
          bool active = false;
          if (isActive)
            active = isActive();
          else
            dbg.print("WTFFFFF2");
          uint8_t pongMsg[] = {MESSAGE_TYPE::PONG, loraUuid, active};
          LoraPhy.send(pongMsg, sizeof(pongMsg));
        }
      } else {
        dbg.print("   ign act for", uuid);
      }

    } else if (msgType == MESSAGE_TYPE::DISABLE_AGENDA) {
      uint8_t shouldDisable;
      LoraPhy.readNext(shouldDisable);
      dbg.print("   got disable agenda", shouldDisable);
      if (onAgendaDisable) {
        onAgendaDisable(shouldDisable);
      }
    } else if (msgType == MESSAGE_TYPE::FILE_MSG) {
      processFileLoraMsg(FileRcv);

    }

    else {
      FLUSH_RETURN_ERR(dbg.toStr("   message not supported", msgType));
    }

    dbg.print("  end lora msg");
  };
}

void end() { LoraPhy.end(); }

void handle() { LoraPhy.handle(); }

}; // namespace LoraApp

#undef dbg
