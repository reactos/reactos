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
#include <ddk/ntstatus.h>

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

/* SEH Support with PSEH */
#include <pseh/pseh2.h>

#define HOOKID_TO_FLAG(HookId) (1 << ((HookId) + 1))
#define ISITHOOKED(HookId) (GetWin32ClientInfo()->fsHooks & HOOKID_TO_FLAG(HookId))

/* Temporarily in here for now. */
typedef struct _USERAPIHOOKINFO
{
  DWORD m_size;
  LPCWSTR m_dllname1;
  LPCWSTR m_funname1;
  LPCWSTR m_dllname2;
  LPCWSTR m_funname2;
} USERAPIHOOKINFO,*PUSERAPIHOOKINFO;

typedef LRESULT(CALLBACK *WNDPROC_OWP)(HWND,UINT,WPARAM,LPARAM,ULONG_PTR,PDWORD);

typedef struct _UAHOWP
{
  BYTE*  MsgBitArray;
  DWORD  Size;
} UAHOWP, *PUAHOWP;

typedef struct tagUSERAPIHOOK
{
  DWORD   size;
  WNDPROC DefWindowProcA;
  WNDPROC DefWindowProcW;
  UAHOWP  DefWndProcArray;
  FARPROC GetScrollInfo;
  FARPROC SetScrollInfo;
  FARPROC EnableScrollBar;
  FARPROC AdjustWindowRectEx;
  FARPROC SetWindowRgn;
  WNDPROC_OWP PreWndProc;
  WNDPROC_OWP PostWndProc;
  UAHOWP  WndProcArray;
  WNDPROC_OWP PreDefDlgProc;
  WNDPROC_OWP PostDefDlgProc;
  UAHOWP  DlgProcArray;
  FARPROC GetSystemMetrics;
  FARPROC SystemParametersInfoA;
  FARPROC SystemParametersInfoW;
  FARPROC ForceResetUserApiHook;
  FARPROC DrawFrameControl;
  FARPROC DrawCaption;
  FARPROC MDIRedrawFrame;
  FARPROC GetRealWindowOwner;
} USERAPIHOOK, *PUSERAPIHOOK;

typedef enum _UAPIHK
{
  uahLoadInit,
  uahStop,
  uahShutdown
} UAPIHK, *PUAPIHK;

extern RTL_CRITICAL_SECTION gcsUserApiHook;
extern USERAPIHOOK guah;
typedef DWORD (CALLBACK * USERAPIHOOKPROC)(UAPIHK State, ULONG_PTR Info);
BOOL FASTCALL BeginIfHookedUserApiHook(VOID);
BOOL FASTCALL EndUserApiHook(VOID);
BOOL FASTCALL IsInsideUserApiHook(VOID);
VOID FASTCALL ResetUserApiHook(PUSERAPIHOOK);
BOOL FASTCALL IsMsgOverride(UINT,PUAHOWP);

#define LOADUSERAPIHOOK \
   if (!gfServerProcess &&                                \
       !IsInsideUserApiHook() &&                          \
       (gpsi->dwSRVIFlags & SRVINFO_APIHOOK) &&           \
       !RtlIsThreadWithinLoaderCallout())                 \
   {                                                      \
      NtUserCallNoParam(NOPARAM_ROUTINE_LOADUSERAPIHOOK); \
   }                                                      \

/* FIXME: Use ntgdi.h then cleanup... */
LONG WINAPI GdiGetCharDimensions(HDC, LPTEXTMETRICW, LONG *);
BOOL FASTCALL IsMetaFile(HDC);

extern PPROCESSINFO g_ppi;
extern ULONG_PTR g_ulSharedDelta;
extern PSERVERINFO gpsi;
extern BOOL gfServerProcess;

static __inline PVOID
SharedPtrToUser(PVOID Ptr)
{
    ASSERT(Ptr != NULL);
    ASSERT(g_ulSharedDelta != 0);
    return (PVOID)((ULONG_PTR)Ptr - g_ulSharedDelta);
}

static __inline PVOID
DesktopPtrToUser(PVOID Ptr)
{
    PCLIENTINFO pci;
    PDESKTOPINFO pdi;
    GetW32ThreadInfo();
    pci = GetWin32ClientInfo();
    pdi = pci->pDeskInfo;

    ASSERT(Ptr != NULL);
    ASSERT(pdi != NULL);
    if ((ULONG_PTR)Ptr >= (ULONG_PTR)pdi->pvDesktopBase &&
        (ULONG_PTR)Ptr < (ULONG_PTR)pdi->pvDesktopLimit)
    {
        return (PVOID)((ULONG_PTR)Ptr - pci->ulClientDelta);
    }
    else
    {
        /* NOTE: This is slow as it requires a call to win32k. This should only be
                 neccessary if a thread wants to access an object on a different
                 desktop */
        return (PVOID)NtUserGetDesktopMapping(Ptr);
    }
}

static __inline PVOID
SharedPtrToKernel(PVOID Ptr)
{
    ASSERT(Ptr != NULL);
    ASSERT(g_ulSharedDelta != 0);
    return (PVOID)((ULONG_PTR)Ptr + g_ulSharedDelta);
}

static __inline BOOL
IsThreadHooked(PCLIENTINFO pci)
{
    return pci->fsHooks != 0;
}

static __inline PDESKTOPINFO
GetThreadDesktopInfo(VOID)
{
    PTHREADINFO ti;
    PDESKTOPINFO di = NULL;

    ti = GetW32ThreadInfo();
    if (ti != NULL)
        di = GetWin32ClientInfo()->pDeskInfo;

    return di;
}

PCALLPROCDATA FASTCALL ValidateCallProc(HANDLE hCallProc);
PWND FASTCALL ValidateHwnd(HWND hwnd);
PWND FASTCALL ValidateHwndOrDesk(HWND hwnd);
PWND FASTCALL GetThreadDesktopWnd(VOID);
PVOID FASTCALL ValidateHandleNoErr(HANDLE handle, UINT uType);
PWND FASTCALL ValidateHwndNoErr(HWND hwnd);
VOID FASTCALL GetConnected(VOID);
BOOL FASTCALL DefSetText(HWND hWnd, PCWSTR String, BOOL Ansi);
BOOL FASTCALL TestWindowProcess(PWND);
VOID UserGetWindowBorders(DWORD, DWORD, SIZE *, BOOL);
