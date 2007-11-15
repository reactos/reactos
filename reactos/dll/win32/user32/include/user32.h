/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/user32/include/user32.h
 * PURPOSE:         Win32 User Library
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <assert.h>
#include <stdio.h>
#include <math.h>

/* SDK/NDK Headers */
#define _USER32_
#define OEMRESOURCE
#define NTOS_MODE_USER
#define WIN32_NO_STATUS
#include <windows.h>
#include <winuser.h>
#include <windowsx.h>
#include <winnls32.h>
#include <ndk/ntndk.h>

/* CSRSS Headers */
#include <csrss/csrss.h>

/* Public Win32K Headers */
#include <win32k/ntusrtyp.h>
#include <win32k/ntuser.h>
#include <win32k/callback.h>

/* WINE Headers */
#include <wine/unicode.h>

/* Internal User32 Headers */
#include "user32p.h"

/* FIXME: Use ntgdi.h then cleanup... */
HGDIOBJ STDCALL  NtGdiSelectObject(HDC  hDC, HGDIOBJ  hGDIObj);
BOOL STDCALL NtGdiPatBlt(HDC hdcDst, INT x, INT y, INT cx, INT cy, DWORD rop4);
LONG STDCALL GdiGetCharDimensions(HDC, LPTEXTMETRICW, LONG *);
BOOL FASTCALL IsMetaFile(HDC);

extern PW32PROCESSINFO g_pi;
extern PW32PROCESSINFO g_kpi;

static __inline PVOID
SharedPtrToUser(PVOID Ptr)
{
    ASSERT(Ptr != NULL);
    ASSERT(g_pi != NULL);
    ASSERT(g_pi->UserHeapDelta != 0);
    return (PVOID)((ULONG_PTR)Ptr - g_pi->UserHeapDelta);
}

static __inline PVOID
DesktopPtrToUser(PVOID Ptr)
{
    PW32THREADINFO ti = GetW32ThreadInfo();
    ASSERT(Ptr != NULL);
    ASSERT(ti != NULL);
    ASSERT(ti->DesktopHeapDelta != 0);
    return (PVOID)((ULONG_PTR)Ptr - ti->DesktopHeapDelta);
}

static __inline PVOID
SharedPtrToKernel(PVOID Ptr)
{
    ASSERT(Ptr != NULL);
    ASSERT(g_pi != NULL);
    ASSERT(g_pi->UserHeapDelta != 0);
    return (PVOID)((ULONG_PTR)Ptr + g_pi->UserHeapDelta);
}

PCALLPROC FASTCALL ValidateCallProc(HANDLE hCallProc);
