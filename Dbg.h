#pragma once
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

  void printWithSpace(const std::string &a) {
    Serial.print(a.c_str());
    Serial.print(" ");
  }

  template <typename... Args> size_t printf(const char *format, Args... a) {
    Serial.print(prefix);
    Serial.print(" : ");
    Serial.printf(format, a...);
  }
  const char *prefix;
};
