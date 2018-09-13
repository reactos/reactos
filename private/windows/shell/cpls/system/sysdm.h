/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    sysdm.h

Abstract:

    Applet-wide declaraions and definitions for the 
    System Control Panel Applet.

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_H_
#define _SYSDM_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <windows.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <cpl.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include "resource.h"
#include "helpid.h"
#include "util.h"
#include "sid.h"
#include "general.h"
#include "netid.h"
#include "hardware.h"
#include "hwprof.h"
#include "profile.h"
#include "advanced.h"
#include "perf.h"
#include "virtual.h"
#include "startup.h"
#include "envvar.h"
#include "edtenvar.h"
#include "syspart.h"


//
// Global variables
//
extern TCHAR g_szErrMem[ 200 ];         //  Low memory message
extern TCHAR g_szSystemApplet[ 100 ];   //  "System Control Panel Applet" title
extern HINSTANCE hInstance;
extern TCHAR g_szNull[];
extern BOOL g_fRebootRequired;


//
// Macros
//

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define SIZEOF(x)    sizeof(x)

#define SetLBWidth( hwndLB, szStr, cxCurWidth )     SetLBWidthEx( hwndLB, szStr, cxCurWidth, 0)

#define IsPathSep(ch)       ((ch) == TEXT('\\') || (ch) == TEXT('/'))
#define IsWhiteSpace(ch)    ((ch) == TEXT(' ') || (ch) == TEXT('\t') || (ch) == TEXT('\n') || (ch) == TEXT('\r'))
#define IsDigit(ch)         ((ch) >= TEXT('0') && (ch) <= TEXT('9'))

#define DigitVal(ch)        ((ch) - TEXT('0'))
#define FmtFree(s)          LocalFree(s)            /* Macro to free FormatMessage allocated strings */



//
// Debugging macros
//
#if DBG
#   define  DBG_CODE    1

void DbgPrintf( LPTSTR szFmt, ... );
void DbgStopX(LPSTR mszFile, int iLine, LPTSTR szText );
HLOCAL MemAllocWorker(LPSTR szFile, int iLine, UINT uFlags, UINT cBytes);
HLOCAL MemFreeWorker(LPSTR szFile, int iLine, HLOCAL hMem);
void MemExitCheckWorker(void);


#   define  MemAlloc( f, s )    MemAllocWorker( __FILE__, __LINE__, f, s )
#   define  MemFree( h )        MemFreeWorker( __FILE__, __LINE__, h )
#   define  MEM_EXIT_CHECK()    MemExitCheckWorker()
#   define  DBGSTOP( t )        DbgStopX( __FILE__, __LINE__, TEXT(t) )
#   define  DBGSTOPX( f, l, t ) DbgStopX( f, l, TEXT(t) )
#   define  DBGPRINTF(p)        DbgPrintf p
#   define  DBGOUT(t)           DbgPrintf( TEXT("SYSDM.CPL: %s\n"), TEXT(t) )
#else
#   define  MemAlloc( f, s )    LocalAlloc( f, s )
#   define  MemFree( h )        LocalFree( h )
#   define  MEM_EXIT_CHECK()
#   define  DBGSTOP( t )
#   define  DBGSTOPX( f, l, t )
#   define  DBGPRINTF(p)
#   define  DBGOUT(t)
#endif

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // _SYSDM_H_
