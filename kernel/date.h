#ifndef __DATE_H__
#define __DATE_H__
#include "types.h"

struct rtcdate {
  uint second;
  uint minute;
  uint hour;
  uint day;
  uint month;
  uint year;
};
#endif // __DATE_H__
