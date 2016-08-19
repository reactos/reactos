/*
 * PROJECT:         ReactOS Event Log Viewer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/mscutils/eventvwr/eventvwr.h
 * PURPOSE:         Event Log Viewer header
 * PROGRAMMERS:     Marc Piulachs (marc.piulachs at codexchange [dot] net)
 *                  Eric Kohl
 *                  Hermes Belusca-Maito
 */

#ifndef _EVENTVWR_PCH_
#define _EVENTVWR_PCH_

// #pragma once

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>

#include <ndk/rtlfuncs.h>

#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#include <strsafe.h>

#include <commctrl.h>
#include <commdlg.h>

#include <richedit.h>

/* Missing RichEdit flags in our richedit.h */
#define AURL_ENABLEURL          1
#define AURL_ENABLEEMAILADDR    2
#define AURL_ENABLETELNO        4
#define AURL_ENABLEEAURLS       8
#define AURL_ENABLEDRIVELETTERS 16

#include <windowsx.h>

/*
 * windowsx.h extensions
 */
#define EnableDlgItem(hDlg, nID, bEnable)   \
    EnableWindow(GetDlgItem((hDlg), (nID)), (bEnable))

#define ProgressBar_SetPos(hwndCtl,pos)    \
    ((int)SNDMSG((hwndCtl),PBM_SETPOS,(WPARAM)(int)(pos),(LPARAM)0))
#define ProgressBar_SetRange(hwndCtl,range)    \
    ((int)SNDMSG((hwndCtl),PBM_SETRANGE,(WPARAM)0,(LPARAM)(range)))
#define ProgressBar_SetStep(hwndCtl,inc)    \
    ((int)SNDMSG((hwndCtl),PBM_SETSTEP,(WPARAM)(int)(inc),(LPARAM)0))
#define ProgressBar_StepIt(hwndCtl)         \
    ((int)SNDMSG((hwndCtl),PBM_STEPIT,(WPARAM)0,(LPARAM)0))

#define StatusBar_GetItemRect(hwndCtl,index,lprc)   \
    ((BOOL)SNDMSG((hwndCtl),SB_GETRECT,(WPARAM)(int)(index),(LPARAM)(RECT*)(lprc)))
#define StatusBar_SetText(hwndCtl,index,data)   \
    ((BOOL)SNDMSG((hwndCtl),SB_SETTEXT,(WPARAM)(index),(LPARAM)(data)))

#ifndef WM_APP
    #define WM_APP 0x8000
#endif

#include "resource.h"

extern HINSTANCE hInst;


/*
 * Structure that caches information about an opened event log.
 */
typedef struct _EVENTLOG
{
    LIST_ENTRY ListEntry;

    // HANDLE hEventLog;       // At least for user logs, a handle is kept opened (by eventlog service) as long as the event viewer has the focus on this log.

    PWSTR ComputerName;     // Computer where the log resides

/** Cached information **/
    PWSTR LogName;          // Internal name (from registry, or file path for user logs)
    PWSTR FileName;         // Cached, for user logs; retrieved once (at startup) from registry for system logs (i.e. may be different from the one opened by the eventlog service)
    // PWSTR DisplayName;     // The default value is the one computed; can be modified by the user for this local session only.
    // We can use the TreeView' item name for the DisplayName...
    BOOL Permanent;         // TRUE: system log; FALSE: user log

/** Volatile information **/
    // ULONG Flags;
    // ULONG MaxSize;          // Always retrieved from registry (only valid for system logs)
    // ULONG Retention;        // Always retrieved from registry (only valid for system logs)
} EVENTLOG, *PEVENTLOG;

typedef struct _EVENTLOGFILTER
{
    LIST_ENTRY ListEntry;

    LONG ReferenceCount;

    // HANDLE hEnumEventsThread;
    // HANDLE hStopEnumEvent;

    // PWSTR DisplayName;     // The default value is the one computed; can be modified by the user for this local session only.
    // We can use the TreeView' item name for the DisplayName...

    BOOL Information;
    BOOL Warning;
    BOOL Error;
    BOOL AuditSuccess;
    BOOL AuditFailure;

    // ULONG Category;
    ULONG EventID;

    /*
     * The following three string filters are multi-strings that enumerate
     * the list of sources/users/computers to be shown. If a string points
     * to an empty string: "\0", it filters for an empty source/user/computer.
     * If a string points to NULL, it filters for all sources/users/computers.
     */
    PWSTR Sources;
    PWSTR Users;
    PWSTR ComputerNames;

    /* List of event logs maintained by this filter */
    ULONG NumOfEventLogs;
    PEVENTLOG EventLogs[ANYSIZE_ARRAY];
} EVENTLOGFILTER, *PEVENTLOGFILTER;

#endif /* _EVENTVWR_PCH_ */
