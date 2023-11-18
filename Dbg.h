#pragma once
#include <StreamString.h>
struct Dbg {
  Dbg(const char *_prefix) : prefix(_prefix) {}

  template <typename... Args> void print(Args... a) {
    Serial.print(prefix);
    Serial.print(" : ");
    (printWithSpace(a), ...);
    Serial.println("");
  }

  template <typename Arg> void printWithSpace(Arg a) {
    Serial.print(a);
    Serial.print(" ");
  }

  // void printWithSpace(const std::string &a) {
  //   Serial.print(a.c_str());
  //   Serial.print(" ");
  // }

  template <typename... Args> size_t printf(const char *format, Args... a) {
    Serial.print(prefix);
    Serial.print(" : ");
    Serial.printf(format, a...);
  }

  void printBuffer(uint8_t *buffer, size_t size) {
    Serial.print(prefix);
    Serial.print(" : ");
    Serial.print("size ");
    Serial.println(size);
    Serial.print("0x");
    for (uint8_t i = 0; i < size; i++) {
      if (buffer[i] < 16)
        Serial.print("0");
      Serial.print(int(buffer[i]), HEX);
      Serial.print(" ");
    }
    Serial.println("");
  }

  template <typename... Args> String toStr(Args... a) const {
    StreamString st;
    (printStrWithSpace(a, st), ...);
    return st;
  }

  template <typename Arg> void printStrWithSpace(Arg a, StreamString &ss) const {
    ss.print(a);
    ss.print(" ");
  }

  const char *prefix;
};
