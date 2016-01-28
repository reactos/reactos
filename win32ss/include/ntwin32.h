
#pragma once

/* These are opaque for user mode */
typedef struct _PROCESSINFO *PPROCESSINFO;
typedef struct _THREADINFO *PTHREADINFO;

typedef struct _W32CLIENTINFO
{
    ULONG CI_flags;
    ULONG cSpins;
    ULONG ulWindowsVersion;
    ULONG ulAppCompatFlags;
    ULONG ulAppCompatFlags2;
    ULONG W32ClientInfo[57];
} W32CLIENTINFO, *PW32CLIENTINFO;
