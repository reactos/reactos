#pragma once

#define __GETBYTE(x, byte) (((x) >> ((byte) * 8)) & 0xFF)
#define __FORMBYTE(x, byte) ((((x) & 0xFF) << ((byte) * 8)))
#define __INVERTBYTE(x, byte, maxByte) (__FORMBYTE(__GETBYTE(x, byte), (((maxByte) - (byte)))))

#ifdef _PPC_
#define SWAPQ(x) (__INVERTBYTE((ULONGLONG)(x), 0, 7) | __INVERTBYTE((ULONGLONG)(x), 1, 7) | \
                  __INVERTBYTE((ULONGLONG)(x), 2, 7) | __INVERTBYTE((ULONGLONG)(x), 3, 7) | \
                  __INVERTBYTE((ULONGLONG)(x), 4, 7) | __INVERTBYTE((ULONGLONG)(x), 5, 7) | \
                  __INVERTBYTE((ULONGLONG)(x), 6, 7) | __INVERTBYTE((ULONGLONG)(x), 7, 7))
#define SWAPD(x) (__INVERTBYTE((ULONG)(x), 0, 3) | __INVERTBYTE((ULONG)(x), 1, 3) | \
                  __INVERTBYTE((ULONG)(x), 2, 3) | __INVERTBYTE((ULONG)(x), 3, 3))
#define SWAPW(x) (__INVERTBYTE((USHORT)(x), 0, 1) | __INVERTBYTE((USHORT)(x), 1, 1))
#else
#define SWAPQ(x) x
#define SWAPD(x) x
#define SWAPW(x) x
#endif

#define SQ(Object,Field) Object->Field = SWAPQ(Object->Field)
#define SD(Object,Field) Object->Field = SWAPD(Object->Field)
#define SW(Object,Field) Object->Field = SWAPW(Object->Field)
