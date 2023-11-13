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
  const char *prefix;
};
