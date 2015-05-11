/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ntvdm.h
 * PURPOSE:         Header file to define commonly used stuff
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _NTVDM_H_
#define _NTVDM_H_

/* INCLUDES *******************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <subsys/win/vdm.h>

// Do not include stuff that is only defined
// for backwards compatibility in nt_vdd.h
#define NO_NTVDD_COMPAT
#include <vddsvc.h>

DWORD WINAPI SetLastConsoleEventActive(VOID);

#define NTOS_MODE_USER
#include <ndk/kefuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/rtltypes.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

#include <debug.h>

/*
 * Activate this line if you want to run NTVDM in standalone mode with:
 * ntvdm.exe <program>
 */
// #define STANDALONE

/*
 * Activate this line for Win2k compliancy
 */
// #define WIN2K_COMPLIANT

/*
 * Activate this line if you want advanced hardcoded debug facilities
 * (called interrupts, etc...), that break PC-AT compatibility.
 * USE AT YOUR OWN RISK! (disabled by default)
 */
// #define ADVANCED_DEBUGGING

/* FUNCTIONS ******************************************************************/

extern HANDLE VdmTaskEvent;

// Command line of NTVDM
extern INT     NtVdmArgc;
extern WCHAR** NtVdmArgv;


/*
 * Interface functions
 */
VOID DisplayMessage(LPCWSTR Format, ...);

/*static*/ VOID
CreateVdmMenu(HANDLE ConOutHandle);
/*static*/ VOID
DestroyVdmMenu(VOID);

BOOL ConsoleAttach(VOID);
VOID ConsoleDetach(VOID);
VOID MenuEventHandler(PMENU_EVENT_RECORD MenuEvent);
VOID FocusEventHandler(PFOCUS_EVENT_RECORD FocusEvent);

#endif // _NTVDM_H_

/* EOF */
