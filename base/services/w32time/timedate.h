#ifndef _TIMEDATE_H
#define _TIMEDATE_H

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

#define MAX_VALUE_NAME 16383
#define NTPPORT 123

/* Used by w32time.c */
BOOL SystemSetTime(LPSYSTEMTIME lpSystemTime, BOOL SystemTime);

/* Used by w32time.c */
#if DBG
VOID DisplayWin32ErrorDbg(DWORD dwErrorCode, const char *file, int line);
#define DisplayWin32Error(e) DisplayWin32ErrorDbg(e, __FILE__, __LINE__);
#else
VOID DisplayWin32Error(DWORD dwErrorCode);
#endif

/* Used by ntpclient.c */
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


#endif /* _TIMEDATE_H */
