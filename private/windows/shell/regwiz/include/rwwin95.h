/*********************************************************************
Registration Wizard
rwWin95.h

This file contains special definitions needed by RegWizard that are
found only in the Windows95 SDK.
12/15/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
**********************************************************************/
#ifndef __rwWin95__
#define __rwWin95__

#include <tchar.h>

#ifndef SS_ETCHEDFRAME
#define SS_ETCHEDFRAME      0x00000012L
#endif

#ifndef VER_PLATFORM_WIN32s
#define VER_PLATFORM_WIN32s             0
#endif

#ifndef VER_PLATFORM_WIN32_WINDOWS
#define VER_PLATFORM_WIN32_WINDOWS      1
#endif

#ifndef VER_PLATFORM_WIN32_NT
#define VER_PLATFORM_WIN32_NT           2
#endif

#ifndef WM_DEVICECHANGE
#define WM_DEVICECHANGE		0x0219
#endif

#ifndef WM_HELP
#define WM_HELP             0x0053
#endif

#ifndef WM_SETICON
#define WM_SETICON          0x0080
#endif

WINUSERAPI BOOL WINAPI DrawIconEx(HDC, int, int, HICON, int, int, UINT, HBRUSH, UINT);

#endif	//rwWin95.h
