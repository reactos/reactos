#ifndef _W32TIME_H
#define _W32TIME_H

#include <stdarg.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winsvc.h>
#include <winuser.h>

#define MAX_VALUE_NAME 16383
#define NTPPORT 123


/* ntpclient.c */
// NTP timestamp
typedef struct _TIMEPACKET
{
  DWORD dwInteger;
  DWORD dwFractional;
} TIMEPACKET, *PTIMEPACKET;

// NTP packet
typedef struct _NTPPACKET
{
  BYTE LiVnMode;
  BYTE Stratum;
  char Poll;
  char Precision;
  long RootDelay;
  long RootDispersion;
  char ReferenceID[4];
  TIMEPACKET ReferenceTimestamp;
  TIMEPACKET OriginateTimestamp;
  TIMEPACKET ReceiveTimestamp;
  TIMEPACKET TransmitTimestamp;
}NTPPACKET, *PNTPPACKET;

ULONG GetServerTime(LPWSTR lpAddress);

#endif /* _W32TIME_H */
