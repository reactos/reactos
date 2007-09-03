#ifndef _PPCDEBUG_H
#define _PPCDEBUG_H

#include "ppcboot.h"

extern struct _boot_infos_t *BootInfo;
extern void DrawNumber(struct _boot_infos_t *, unsigned long, int, int);
extern void DrawString(struct _boot_infos_t *, const char *, int, int);
#define TRACEXY(x,y) do { \
  unsigned long _x_ = (unsigned long)(x), _y_ = (unsigned long)(y); \
  __asm__("ori 0,0,0"); \
  DrawNumber(BootInfo, __LINE__, 10, 160); \
  DrawString(BootInfo, __FILE__, 100, 160); \
  DrawNumber(BootInfo, _x_, 400, 160); \
  DrawNumber(BootInfo, _y_, 490, 160); \
} while(0)
#define TRACEX(x) TRACEXY(x,0)
#define TRACE TRACEX(0)

#endif//_PPCDEBUG_H
