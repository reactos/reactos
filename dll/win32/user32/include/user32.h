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
    if ((ULONG_PTR)Ptr >= (ULONG_PTR)ti->DesktopHeapBase &&
        (ULONG_PTR)Ptr < (ULONG_PTR)ti->DesktopHeapBase + ti->DesktopHeapLimit)
    {
        return (PVOID)((ULONG_PTR)Ptr - ti->DesktopHeapDelta);
    }
    else
    {
        /* NOTE: This is slow as it requires a call to win32k. This should only be
                 neccessary if a thread wants to access an object on a different
                 desktop */
        return NtUserGetDesktopMapping(Ptr);
    }
}

static __inline PVOID
SharedPtrToKernel(PVOID Ptr)
{
    ASSERT(Ptr != NULL);
    ASSERT(g_pi != NULL);
    ASSERT(g_pi->UserHeapDelta != 0);
    return (PVOID)((ULONG_PTR)Ptr + g_pi->UserHeapDelta);
}

static __inline BOOL
IsThreadHooked(PW32THREADINFO ti)
{
    return ti->Hooks != 0;
}

static __inline PDESKTOP
GetThreadDesktopInfo(VOID)
{
    PW32THREADINFO ti;
    PDESKTOP di = NULL;

    ti = GetW32ThreadInfo();
    if (ti != NULL)
        di = DesktopPtrToUser(ti->Desktop);

    return di;
}

PCALLPROC FASTCALL ValidateCallProc(HANDLE hCallProc);
PWINDOW FASTCALL ValidateHwnd(HWND hwnd);
PWINDOW FASTCALL ValidateHwndOrDesk(HWND hwnd);
PWINDOW FASTCALL GetThreadDesktopWnd(VOID);
PVOID FASTCALL ValidateHandleNoErr(HANDLE handle, UINT uType);
PWINDOW FASTCALL ValidateHwndNoErr(HWND hwnd);
