//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.c
//
//  Contents:   Debugging support functions
//
//  Classes:
//
//  Functions:
//
//  Note:       This file is not compiled for retail builds
//
//  History:    4-29-93   RichardW   Created
//
//----------------------------------------------------------------------------

#if DBG         // NOTE:  This file not compiled for retail builds

#include "precomp.h"
#pragma hdrstop

FILE *  LogFile;
DWORD   WinlogonInfoLevel = 3;


// Debugging support functions.

// These two functions do not exist in Non-Debug builds.  They are wrappers
// to the commnot functions (maybe I should get rid of that as well...)
// that echo the message to a log file.

char * DebLevel[] = {   "Winlogon-Error",
                        "Winlogon-Warn",
                        "Winlogon-Trace",
                        "Winlogon-Trace-Init",
                        "Winlogon-Trace-Timeout",
                        "Winlogon-Trace-SAS",
                        "Winlogon-Trace-State",
                        "Winlogon-Trace-MPR",
                        "Should-not-see",
                        "Winlogon-Trace-Profile",
                        "Should-not-see",
                        "Should-not-see",
                        "Should-not-see",
                        "Winlogon-Trace-Migrate",
                        "Should-not-see",
                        "Winlogon-Trace-Setup",
                        "Winlogon-Trace-SC",
                        "Winlogon-Trace-Notify",
                        "Winlogon-Trace-Job"
                    };

typedef struct _DebugKeys {
    char *  Name;
    DWORD   Value;
} DebugKeys, *PDebugKeys;

DebugKeys   DebugKeyNames[] = {
                {"Error",       DEB_ERROR},
                {"Warning",     DEB_WARN},
                {"Trace",       DEB_TRACE},
                {"Init",        DEB_TRACE_INIT},
                {"Timeout",     DEB_TRACE_TIMEOUT},
                {"Sas",         DEB_TRACE_SAS},
                {"State",       DEB_TRACE_STATE},
                {"MPR",         DEB_TRACE_MPR},
                {"CoolSwitch",  DEB_COOL_SWITCH},
                {"Profile",     DEB_TRACE_PROFILE},
                {"DebugLsa",    DEB_DEBUG_LSA},
                {"DebugSpm",    DEB_DEBUG_LSA},
                {"DebugMpr",    DEB_DEBUG_MPR},
                {"DebugGo",     DEB_DEBUG_NOWAIT},
                {"Migrate",     DEB_TRACE_MIGRATE},
                {"DebugServices", DEB_DEBUG_SERVICES},
                {"Setup",       DEB_TRACE_SETUP},
                {"SC",          DEB_TRACE_SC},
                {"Notify",      DEB_TRACE_NOTIFY},
                {"Job",         DEB_TRACE_JOB}
                };

#define NUM_DEBUG_KEYS  sizeof(DebugKeyNames) / sizeof(DebugKeys)
#define NUM_BREAK_KEYS  sizeof(BreakKeyNames) / sizeof(DebugKeys)

//+---------------------------------------------------------------------------
//
//  Function:   LogEvent
//
//  Synopsis:   Logs an event to the console and, optionally, a file.
//
//  Effects:
//
//  Arguments:  [Mask]   --
//              [Format] --
//              [Format] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
LogEvent(   long            Mask,
            const char *    Format,
            ...)
{
    va_list ArgList;
    int     Level = 0;
    int     PrefixSize = 0;
    char    szOutString[256];
    long    OriWinlogonlMask = Mask;


    if (Mask & WinlogonInfoLevel)
    {
        while (!(Mask & 1))
        {
            Level++;
            Mask >>= 1;
        }
        if (Level >= (sizeof(DebLevel) / sizeof(char *)) )
        {
            Level = (sizeof(DebLevel) / sizeof(char *)) - 1;
        }


        //
        // Make the prefix first:  "Process.Thread> Winlogon-XXX"
        //

        PrefixSize = sprintf(szOutString, "%d.%d> %s: ",
                GetCurrentProcessId(), GetCurrentThreadId(), DebLevel[Level]);


        va_start(ArgList, Format);

        if (_vsnprintf(&szOutString[PrefixSize], sizeof(szOutString) - PrefixSize,
                            Format, ArgList) < 0)
        {
            //
            // Less than zero indicates that the string could not be
            // fitted into the buffer.  Output a special message indicating
            // that:
            //

            OutputDebugStringA("Winlogon!LogEvent:  Could not pack string into 256 bytes\n");

        }
        else
        {
            OutputDebugStringA(szOutString);
        }


        if (LogFile)
        {
            SYSTEMTIME  stTime;
            FILETIME    ftTime;
            FILETIME    localtime;

            NtQuerySystemTime((PLARGE_INTEGER) &ftTime);
            FileTimeToLocalFileTime(&ftTime, &localtime);
            FileTimeToSystemTime(&localtime, &stTime);
            fprintf(LogFile, "%02d:%02d:%02d.%03d: %s\n",
                    stTime.wHour, stTime.wMinute, stTime.wSecond,
                    stTime.wMilliseconds, szOutString);

            fflush(LogFile);
        }

    }

}

void
OpenLogFile(LPSTR   pszLogFile)
{
    LogFile = fopen(pszLogFile, "a");
    if (!LogFile)
    {
        OutputDebugStringA("Winlogon: Could not open logfile for append");
        OutputDebugStringA(pszLogFile);
    }
    DebugLog((DEB_TRACE, "Log file '%s' begins\n", pszLogFile));
}


DWORD
GetDebugKeyValue(
    PDebugKeys      KeyTable,
    int             cKeys,
    LPSTR           pszKey)
{
    int     i;

    for (i = 0; i < cKeys ; i++ )
    {
        if (_strcmpi(KeyTable[i].Name, pszKey) == 0)
        {
            return(KeyTable[i].Value);
        }
    }
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadDebugParameters
//
//  Synopsis:   Loads debug parameters from win.ini
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void
LoadDebugParameters(char * szSection)
{
    char    szVal[128];
    char *  pszDebug;
    int     cbVal;

    cbVal = GetProfileStringA(szSection, "DebugFlags", "Error,Warning", szVal, sizeof(szVal));

    pszDebug = strtok(szVal, ", \t");
    while (pszDebug)
    {
        WinlogonInfoLevel |= GetDebugKeyValue(DebugKeyNames, NUM_DEBUG_KEYS, pszDebug);
        pszDebug = strtok(NULL, ", \t");
    }

    cbVal = GetProfileStringA(szSection, "LogFile", "", szVal, sizeof(szVal));
    if (cbVal)
    {
        OpenLogFile(szVal);
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   InitDebugSupport
//
//  Synopsis:   Initializes debugging support for the Winlogon
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void
InitDebugSupport(void)
{
    LoadDebugParameters("WinlogonDebug");
    LoadDebugParameters("Winlogon");

}



#else // DBG

#pragma warning(disable:4206)   // Disable the empty transation unit
                                // warning/error

#endif  // NOTE:  This file not compiled for retail builds
