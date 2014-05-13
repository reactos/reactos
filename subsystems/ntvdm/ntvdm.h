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
#include <conio.h>
#include <wchar.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <subsys/win/vdm.h>

#include <vddsvc.h>

DWORD WINAPI SetLastConsoleEventActive(VOID);

#include <debug.h>

/*
 * Activate this line if you want to run NTVDM in standalone mode with:
 * ntvdm.exe <program>
 */
// #define STANDALONE

/* FUNCTIONS ******************************************************************/

#ifndef STANDALONE
extern ULONG SessionId;
#endif

extern HANDLE VdmTaskEvent;

VOID DisplayMessage(LPCWSTR Format, ...);

#endif // _NTVDM_H_

/* EOF */
