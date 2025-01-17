/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/utils.c
 * PURPOSE:     Memory Management, Resources, ... Utility Functions
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if 0
VOID
MemInit(IN HANDLE Heap);
#endif

BOOL
MemFree(IN PVOID lpMem);

PVOID
MemAlloc(IN DWORD dwFlags,
         IN SIZE_T dwBytes);

LPWSTR
FormatDateTime(IN LPSYSTEMTIME pDateTime);

VOID
FreeDateTime(IN LPWSTR lpszDateTime);

LPWSTR
LoadResourceStringEx(IN HINSTANCE hInstance,
                     IN UINT uID,
                     OUT size_t* pSize OPTIONAL);

LPWSTR
LoadConditionalResourceStringEx(IN HINSTANCE hInstance,
                                IN BOOL bCondition,
                                IN UINT uIDifTrue,
                                IN UINT uIDifFalse,
                                IN size_t* pSize OPTIONAL);

#define LoadResourceString(hInst, uID) \
    LoadResourceStringEx((hInst), (uID), NULL)

#define LoadConditionalResourceString(hInst, bCond, uIDifT, uIDifF) \
    LoadConditionalResourceStringEx((hInst), (bCond), (uIDifT), (uIDifF), NULL)

DWORD
RunCommand(IN LPCWSTR lpszCommand,
           IN LPCWSTR lpszParameters,
           IN INT nShowCmd);


////////////////////  The following comes from MSDN samples  ///////////////////
// https://learn.microsoft.com/en-us/windows/win32/gdi/positioning-objects-on-a-multiple-display-setup
//

//
// Available control flags.
//
#define MONITOR_CENTER   0x0001     // center rect to monitor
#define MONITOR_CLIP     0x0000     // clip rect to monitor
#define MONITOR_WORKAREA 0x0002     // use monitor work area
#define MONITOR_AREA     0x0000     // use monitor entire area

VOID ClipOrCenterRectToMonitor(LPRECT prc, UINT flags);
VOID ClipOrCenterWindowToMonitor(HWND hWnd, UINT flags);
////////////////////////////////////////////////////////////////////////////////


BOOL IsWindowsOS(VOID);
BOOL IsPreVistaOSVersion(VOID);

LPWSTR
GetExecutableVendor(IN LPCWSTR lpszFilename);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __UTILS_H__

/* EOF */
