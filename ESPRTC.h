#pragma once
#include "./lib/JPI/src/API.hpp"
#include "time.h"
#include <RtcDS1307.h>

#define TIME_TESTS 1
// #include <TZ.h>
#include <Wire.h>
#define MY_TZ PSTR("CET-1CEST,M3.5.0,M10.5.0/3") // TZ.h Europe_Paris

struct ESPRTC : public APIAndInstance<ESPRTC>, LeafNode {

  const char *ntpServer = "pool.ntp.org";
  // const long gmtOffset_sec = 3600;
  // const int daylightOffset_sec = 3600;

  // TwoWire wire;
  RtcDS1307<TwoWire> rtc;
  ESPRTC() : APIAndInstance<ESPRTC>(this), rtc(Wire) {
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // rFunction<void, int32_t>("setLocalEpoch", &ESPRTC::setLocalTimeUTC);

    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    configTzTime(MY_TZ, ntpServer);

    instance(this);
  }

  ESPRTC(ESPRTC &) = delete;
  bool setup() override {

    rtc.Begin(); // add i2c pin if needed
    delay(20);
    syncRTCToLocal();
    // printLocalTime();

    sntp_set_time_sync_notification_cb(ntpSyncCbStatic);

#if TIME_TESTS
    {
      // PRINTLN(">>>>>>>>>>>start auto test");
      // tm compile_time{
      //     0,          //       int	tm_sec;
      //     0,          //   int	tm_min;
      //     0,          //   int	tm_hour;
      //     12,         //   int	tm_mday;
      //     11,         //   int	tm_mon;
      //     2021 - 1900 //   int	tm_year;
      // };
      // // char *month[10];
      // // sscanf(__DATE__, "%s %d %d", month, &compile_time.tm_mday,
      // //        &compile_time.tm_year);

      // PRINT("setting approximative compile time date : ");
      // PRINT("from : ");
      // // PRINT(__DATE__);
      // // PRINT(std::to_string(compile_time.tm_year).c_str());
      // // // PRINT(" to :");
      // PRINT(asctime(&compile_time));
      // time_t epoch = mktime(&compile_time);
      // PRINT(" => is now : ");
      // PRINTLN(asctime(&compile_time));
      // setLocalTimeUTC(epoch);
      // yield();
      // delay(100);
      // if (!getLocalTimeUTC(&epoch)) {
      //   PRINTLN("can't get first local utc time");
      // }
      // yield();
      // delay(3000);
      // time_t nepoch;
      // if (!getLocalTimeUTC(&nepoch)) {
      //   PRINTLN("can't get second local utc time");
      // }

      // PRINT("time diff  :  ");
      // PRINTLN(std::to_string(nepoch - epoch).c_str());

      // PRINTLN(">>>>>>>>>end auto test");
    }
#endif
    return true;
  }

  bool syncLocalToRTC() {
    time_t epoch;
    if (!getLocalTimeUTC(&epoch)) {
      PRINTLN("can't get local utc time");
      return false;
    }
    setRTCTime(epoch);
    return true;
  }

  bool syncRTCToLocal() {
    time_t epoch;
    if (!getRTCTimeUTC(&epoch)) {
      PRINTLN("can't get rtc utc time");
      return false;
    }
    setLocalTimeUTC(epoch);
    return true;
  }

  void setRTCTime(time_t epochTime) {
    if (!rtc.GetIsRunning()) {
      PRINTLN("rtc not running");
      return;
    }
    RTCIsUpdating = true;
    RtcDateTime dt;
    dt.InitWithEpoch64Time(epochTime);
    rtc.SetDateTime(dt);
    RTCIsUpdating = false;
  }

  void setLocalTimeUTC(time_t epoch) {
    struct timeval tv = {epoch, 0};
    settimeofday(&tv, NULL);
  }

  // sntp_sync_time_cb_t

  void ntpSyncCb(timeval *tv) {
    if (RTCIsUpdating) {
      PRINTLN("setting from rtc?");
      return;
    }
    PRINTLN("got ntp update");

    setRTCTime(tv->tv_sec); // time_t

    // we may want to stop ntp here
    // if(sntp_enabled()){sntp_stop();}
  }

  // epoch  time getters
  bool getLocalTimeUTC(time_t *epoch) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      PRINT("no time found stuck at :");
      PRINTLN(asctime(&timeinfo));
      return false;
    }
    time(epoch);

    return true;
  }

  bool getRTCTimeUTC(time_t *epoch) {
    if (!rtc.GetIsRunning())
      return false;
    auto rd = rtc.GetDateTime();
    *epoch = rd.Epoch64Time();
    return true;
  }

  // Static helpers

  static ESPRTC *instance(ESPRTC *set = nullptr) {
    static ESPRTC *inst = nullptr;
    if (set) {
      if (inst != nullptr)
        PRINTLN("Ooo double initialization = double time??? OhOoooH");
      inst = set;
    }
    return inst;
  }

  static void ntpSyncCbStatic(timeval *tv) {
    if (auto i = instance())
      i->ntpSyncCb(tv);
    else
      PRINTLN("time changed before instanciaition of RTC");
  }

  bool RTCIsUpdating = false;
};

/// Notes
/*
for auto ntp disocvery : check LWIP_DHCP_GET_NTP_SRV
(SNTP_GET_SERVERS_FROM_DHCP)

*/
// static void printLocalTime() {
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo)) {
//     PRINTLN("Failed to obtain time");
//     return;
//   }
//   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
// }

// static tm rtctoTm(const RtcDateTime &dt, bool hadDst = false) {
//   tm tinfo{
//       dt.Second(),   //       int	tm_sec;
//       dt.Minute(),   //   int	tm_min;
//       dt.Hour(),     //   int	tm_hour;
//       dt.Day(),      //   int	tm_mday;
//       dt.Month(),    //   int	tm_mon;
//       dt.Year(),     //   int	tm_year;
//                      //   int	tm_wday;
//                      //   int	tm_yday;
//       hadDst ? 0 : 1 //   int	tm_isdst; // not sure its used
//                      // #ifdef __TM_GMTOFF
//                      //   long	__TM_GMTOFF;
//                      // #endif
//                      // #ifdef __TM_ZONE
//                      // const char *__TM_ZONE;
//   };
//   mktime(&tinfo);
//   return tinfo;
// }

// static RtcDateTime tmToRtc(const tm &tinfo) {

//   return RtcDateTime(tinfo.tm_year, // uint16_t year,
//                      tinfo.tm_mon,  //     uint8_t month,
//                      tinfo.tm_mday, //      uint8_t dayOfMonth,
//                      tinfo.tm_hour, //     uint8_t hour,
//                      tinfo.tm_min,  //    uint8_t minute,
//                      tinfo.tm_sec   //    uint8_t second
//   );
// }
