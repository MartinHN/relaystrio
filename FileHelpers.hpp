#pragma once

#include <MD5Builder.h>

auto dbgESPFS = Dbg("[fs]");
#define dbg dbgESPFS

namespace ESPFS {

bool init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }

  Serial.println("SPIFFS inited");
  Serial.print("total kilo bytes ");
  auto totB = String(SPIFFS.totalBytes() / 1000.f);
  Serial.println(totB);
  Serial.print("used kilo bytes ");
  auto uB = String(SPIFFS.usedBytes() / 1000.f);
  Serial.println(uB);
  return true;
}

bool writeToFile(const String &filename, const String &data) {
  auto f = SPIFFS.open(filename, "w");
  if (!f) {
    dbg.print("can not write to", filename);
    return false;
  }
  f.print(data);
  f.close();
  return true;
}

String readFile(const String &filename) {
  auto f = SPIFFS.open(filename, "r");
  if (!f) {
    dbg.print("can not read file", filename);
    return {};
  }
  String res = f.readString();
  f.close();
  return res;
}

void updateMD5ForFile(const String &filename) {
  MD5Builder _md5;
  {
    auto f = SPIFFS.open(filename, "r");
    if (!f) {
      dbg.print("[md5] original file open failed");
      return;
    }
    _md5.begin();
    _md5.addStream(f, 190 * 1000);
    f.close();
  }
  _md5.calculate();
  auto md5Str = _md5.toString();
  if (md5Str.length() != 32) {
    dbg.print("[md5] invalid md5");
    return;
  }
  auto md5Filename = filename + ".md5";
  writeToFile(md5Filename, md5Str);
  dbg.print("[md5] wrote", md5Filename, " :: ", md5Str);
}

String getMD5ForFile(const String &filename) {
  auto f = SPIFFS.open(filename + ".md5", "r");
  if (!f) {
    dbg.print("[md5] can not read md5");
    return {};
  }
  String res = f.readString();
  res.trim();
  f.close();
  return res;
}

} // namespace ESPFS

#undef dbg
