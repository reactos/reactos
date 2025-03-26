/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/ntvdm.h
 * PURPOSE:         Header file to define commonly used stuff
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _NTVDM_H_
#define _NTVDM_H_

/* BUILD CONFIGURATION ********************************************************/

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
 * (called interrupts, etc...), that may break PC-AT compatibility.
 * USE AT YOUR OWN RISK! (disabled by default)
 */
// #define ADVANCED_DEBUGGING

#ifdef ADVANCED_DEBUGGING
#define ADVANCED_DEBUGGING_LEVEL    1
#endif


/* INCLUDES *******************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#ifndef _countof
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

/* String widening macro */
#define __L(x)  L ## x
#define _L(x)   __L(x)
#define L(x)    _L(x)

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <commdlg.h>

#include <subsys/win/vdm.h>

// Do not include stuff that is only defined
// for backwards compatibility in nt_vdd.h
#define NO_NTVDD_COMPAT
#include <vddsvc.h>

DWORD WINAPI SetLastConsoleEventActive(VOID);

#define NTOS_MODE_USER
#include <ndk/kefuncs.h>    // For NtQueryPerformanceCounter()
#include <ndk/rtlfuncs.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

#include <ntstrsafe.h>


/* VARIABLES ******************************************************************/

typedef struct _NTVDM_SETTINGS
{
    ANSI_STRING BiosFileName;
    ANSI_STRING RomFiles;
    UNICODE_STRING FloppyDisks[2];
    UNICODE_STRING HardDisks[4];
} NTVDM_SETTINGS, *PNTVDM_SETTINGS;

extern NTVDM_SETTINGS GlobalSettings;

// Command line of NTVDM
extern INT     NtVdmArgc;
extern WCHAR** NtVdmArgv;

/* Full directory where NTVDM resides, or the SystemRoot\System32 path */
extern WCHAR NtVdmPath[MAX_PATH];
extern ULONG NtVdmPathSize; // Length without NULL terminator.

extern HWND hConsoleWnd;


/* FUNCTIONS ******************************************************************/

/*
 * Interface functions
 */
typedef VOID (*CHAR_PRINT)(IN CHAR Character);
VOID DisplayMessage(IN LPCWSTR Format, ...);
VOID PrintMessageAnsi(IN CHAR_PRINT CharPrint,
                      IN LPCSTR Format, ...);

/*static*/ VOID
UpdateVdmMenuDisks(VOID);

BOOL ConsoleAttach(VOID);
VOID ConsoleDetach(VOID);
VOID ConsoleReattach(HANDLE ConOutHandle);
BOOL IsConsoleHandle(HANDLE hHandle);
VOID MenuEventHandler(PMENU_EVENT_RECORD MenuEvent);
VOID FocusEventHandler(PFOCUS_EVENT_RECORD FocusEvent);

#endif // _NTVDM_H_

/* EOF */
