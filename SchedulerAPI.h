#pragma once
#include "./lib/JPI/src/API.hpp"
#include "SPIFFS.h"
#include <fstream>

#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

#define DBG_SCH 0

#if DBG_SCH
#define DBGSCH(x) PRINT(x)
#define DBGSCHLN(x) PRINTLN(x)
#else
#define DBGSCH(x)                                                              \
  {}
#define DBGSCHLN(x)                                                            \
  {}
#endif
/*

{
  "name": "default",
  "defaultWeek": {
    "defaultDay": {
      "dayName": "default",
      "hourRangeList": [
        {
          "start": "09:00",
          "end": "18:30"
        }
      ]
    },
    "exceptions": [
      {
        "dayName": "mardi",
        "hourRangeList": [
          {
            "start": "09:00",
            "end": "18:00"
          }
        ]
      }
    ]
  },
  "agendaExceptionList": [
    {
      "name": "ococ",
      "dates": {
        "start": "19/10/2021",
        "end": "19/10/2021"
      },
      "dayValue": {
        "dayName": "default",
        "hourRangeList": []
      }
    }
  ]
}

*/

struct MyHour {
  // std::string originalString;
  int hour = -1;
  int minutes = -1;
  void fromString(std::string &s) {
    // originalString = s;
    const char *data = s.c_str();
    // const char data[] = "12:30";
    char *ep;

    hour = (unsigned char)strtol(data, &ep, 10);
    if (!ep || *ep != ':') {
      Serial.print("!!! cannot parse hour: - wrong format");
      Serial.println(data);
      return;
    }

    minutes = (unsigned char)strtol(ep + 1, &ep, 10);
    if (!ep || *ep != '\0') {
      Serial.print("!!! cannot parse minutes: - wrong format");
      Serial.println(data);
      return;
    }
  }

  int getDayMinutes() const { return hour * 60 + minutes; }
};

template <class Archive> void load(Archive &ar, MyHour &ms) {
  std::string s_temp = {};
  ar(s_temp);
  ms.fromString(s_temp);
}

struct MyDay {
  // std::string originalString;
  int dayOfMonth = -1;
  int month = -1;
  int year = -1;
  void fromString(std::string &s) {
    // originalString = s;
    const char *data = s.c_str();
    // const char data[] = "12:30";
    char *ep;

    dayOfMonth = (int)strtol(data, &ep, 10);
    if (!ep || *ep != '/') {
      Serial.print("!!! cannot parse day of month: - wrong format");
      Serial.println(data);
      return;
    }

    month = (int)strtol(ep + 1, &ep, 10) - 1;
    if (!ep || *ep != '/') {
      Serial.print("!!! cannot parse month: - wrong format");
      Serial.println(data);
      return;
    }
    year = (int)strtol(ep + 1, &ep, 10) - 1900;
    if (!ep || *ep != '\0') {
      Serial.print("!!! cannot parse year: - wrong format");
      Serial.println(data);
      return;
    }
    // DBGSCH("day created (month starts at 0,y1900): ");
    // DBGSCH(String(dayOfMonth));
    // DBGSCH("/");
    // DBGSCH(String(month));
    // DBGSCH("/");
    // DBGSCHLN(String(year));
  }
  static int toDayOfEpoch(int dom, int m, int y) {
    // DBGSCH("to2k ");
    // DBGSCH(String(dom));
    // DBGSCH("/");
    // DBGSCH(String(m));
    // DBGSCH("/");
    // DBGSCHLN(String(y));
    return dom + m * 32 + y * 12 * 32;
  }

  int cmp(const tm &t) const {
    return toDayOfEpoch(t.tm_mday, t.tm_mon, t.tm_year) -
           toDayOfEpoch(dayOfMonth, month, year);
  }
};

template <class Archive> void load(Archive &ar, MyDay &ms) {
  std::string s_temp = {};
  ar(s_temp);
  ms.fromString(s_temp);
}

namespace cereal {
// void prologue(JSONOutputArchive &, MyHour const &) {}
// void epilogue(JSONOutputArchive &, MyHour const &) {}
void prologue(JSONInputArchive &, MyHour const &) {}
void epilogue(JSONInputArchive &, MyHour const &) {}
void prologue(JSONInputArchive &, MyDay const &) {}
void epilogue(JSONInputArchive &, MyDay const &) {}
} // namespace cereal

struct HourRange {
  MyHour start;
  MyHour end;
  template <class AR> void serialize(AR &ar) {
    ar(CEREAL_NVP(start), CEREAL_NVP(end));
  }

  bool isActiveForDayMinute(int dayMin) const {
    auto st = start.getDayMinutes();
    auto ed = end.getDayMinutes();
    if (ed == 0)
      ed = 24 * 60;
    if (ed < st)
      ed += 24 * 60;

    DBGSCH("Day: check if  ");
    DBGSCH(String(dayMin));
    DBGSCH(" is btw ");
    DBGSCH(String(st));
    DBGSCH(" and ");
    DBGSCHLN(String(ed));
    return dayMin >= st && dayMin <= ed;
  }
};

struct DayType {
  std::string dayName = {};
  std::vector<HourRange> hourRangeList = {};
  template <class AR> void serialize(AR &ar) {
    ar(CEREAL_NVP(dayName), CEREAL_NVP(hourRangeList));
  }

  bool isActiveAtMinute(int dayMinute) const {
    for (auto &d : hourRangeList) {
      if (d.isActiveForDayMinute(dayMinute))
        return true;
    }
    DBGSCHLN("day: no matching hourRanges ");

    return false;
  }
};

const std::vector<std::string> &dayNames() {
  static std::vector<std::string> days{
      "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi", "dimanche"};
  return days;
}

const std::string getDnFromDow(int dow) {
  if (dow >= 0 && dow < 7) {
    return dayNames()[dow];
  }
  return {};
}
struct WeekType {
  DayType defaultDay;
  std::vector<DayType> exceptions;
  template <class AR> void serialize(AR &ar) {
    ar(CEREAL_NVP(defaultDay), CEREAL_NVP(exceptions));
  }

  const DayType &getDayForDow(int dow) const {
    auto targetDN = getDnFromDow(dow);
    DBGSCH("week: searching day  : ");
    DBGSCHLN(targetDN.c_str());
    for (auto &e : exceptions) {
      if (e.dayName == targetDN) {
        return e;
      }
    }
    return defaultDay;
  }
  bool isActiveForLocalTime(const tm &t) const {
    auto d = getDayForDow((t.tm_wday + 6) % 7);
    DBGSCH("week: found day matching : ");
    DBGSCHLN(d.dayName.c_str());
    return d.isActiveAtMinute(t.tm_hour * 60 + t.tm_min);
  }
};

struct DayPeriod {
  MyDay start;
  MyDay end;
  template <class AR> void serialize(AR &ar) {
    ar(CEREAL_NVP(start), CEREAL_NVP(end));
  }

  bool includeLocalTime(const tm &t) const {
    auto offStart = start.cmp(t);
    auto offEnd = end.cmp(t);
    DBGSCH("cmp :");
    DBGSCH(String(offStart));
    DBGSCH(" : ");
    DBGSCHLN(String(offEnd));

    return (offStart >= 0 && offEnd <= 0);
  }
};
struct AgendaException {
  std::string name = {};
  DayPeriod dates = {};
  DayType dayValue = {};
  template <class AR> void serialize(AR &ar) {
    ar(CEREAL_NVP(name), CEREAL_NVP(dates), CEREAL_NVP(dayValue));
  }
  bool includeLocalTime(const tm &t) const { return dates.includeLocalTime(t); }
};
struct Agenda {
  std::string name = {};
  WeekType defaultWeek;
  std::vector<AgendaException> agendaExceptionList;
  template <class AR> void serialize(AR &ar) {
    ar(CEREAL_NVP(name), CEREAL_NVP(defaultWeek),
       CEREAL_NVP(agendaExceptionList));
  }

  bool isActiveForLocalTime(const tm &t) const {
    DBGSCH("looking if active for : ");
    DBGSCHLN(asctime(&t));
    for (const auto &e : agendaExceptionList) {
      DBGSCH(" checking exception : ");
      DBGSCHLN(e.name.c_str());
      if (e.includeLocalTime(t)) {
        DBGSCHLN(" dates matches , checking hour range ");
        return e.dayValue.isActiveAtMinute(t.tm_hour * 60 + t.tm_min);
      }
    }
    DBGSCHLN("using default week");
    return defaultWeek.isActiveForLocalTime(t);
  }
};

struct SchedulerAPI : public APIAndInstance<SchedulerAPI>, LeafNode {

  SchedulerAPI() : APIAndInstance<SchedulerAPI>(this) {}

  SchedulerAPI(SchedulerAPI &) = delete;

  bool setup() override { return loadFromFileSystem(); }
  bool shouldBeOn() {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    return agenda.isActiveForLocalTime(*timeinfo);
  }

  int fileSize(const char *filePath) {

    std::streampos fsize = 0;
    std::ifstream file(filePath, std::ios::binary);

    fsize = file.tellg();
    file.seekg(0, std::ios::end);
    fsize = file.tellg() - fsize;
    file.close();

    return fsize;
  }

  bool loadFromFileSystem() {

    auto f = SPIFFS.open("/agenda.json", "r");
    if (!f) {
      Serial.println("!!! can't open agenda file");
      return false;
    }

    std::ifstream fileToRead("/spiffs/agenda.json");
    // ifstream file( "example.txt", ios::binary | ios::ate);
    auto size = fileSize("/spiffs/agenda.json");
    // if (size == 0) {
    //   Serial.println("agenda file is empty");
    //   return false;
    // }
    DBGSCH("file has size : ");
    DBGSCHLN(String(size));
    try {
      cereal::JSONInputArchive archive(fileToRead);
      agenda.serialize(archive);
      f.close();
    } catch (const cereal::Exception &e) {
      Serial.println("!!!!! error while loading agenda");
      Serial.println(e.what());
      agenda = {};
      return false;
    } catch (...) {
      std::exception_ptr e = std::current_exception();
      Serial.println("!!!!! STD error while loading agenda");
      if (e)
        Serial.println(e.__cxa_exception_type()->name());

      return false;
    }
    DBGSCHLN(">>>> agenda file is deserialized");

    return true;
  }
  Agenda agenda;
};
