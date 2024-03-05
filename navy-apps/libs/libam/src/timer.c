#include <am.h>
#include <navy.h>

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  struct timeval tv = {};
  gettimeofday(&tv, NULL);
  
  uptime->us = tv.tv_sec * 1000000 + tv.tv_usec;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
