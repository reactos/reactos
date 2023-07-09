/****************************** Module Header ******************************\
* Module Name: usercall.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains all kernel mode entry points
*
* History:
* 12-98 Hiroyama    Created
\***************************************************************************/

#ifndef _USERCALL_
#define _USERCALL_

#include "w32wow64.h"

#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

W32KAPI
ULONG_PTR
NtUserCallNoParam(
    IN DWORD xpfnProc);

W32KAPI
ULONG_PTR
NtUserCallOneParam(
    IN ULONG_PTR dwParam,
    IN DWORD xpfnProc);

W32KAPI
KERNEL_ULONG_PTR
NtUserCallHwnd(
    IN HWND hwnd,
    IN DWORD xpfnProc);

W32KAPI
KERNEL_ULONG_PTR
NtUserCallHwndLock(
    IN HWND hwnd,
    IN DWORD xpfnProc);

W32KAPI
ULONG_PTR
NtUserCallHwndOpt(
    IN HWND hwnd,
    IN DWORD xpfnProc);

W32KAPI
ULONG_PTR
NtUserCallTwoParam(
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2,
    IN DWORD xpfnProc);

W32KAPI
ULONG_PTR
NtUserCallHwndParam(
    IN HWND hwnd,
    IN ULONG_PTR dwParam,
    IN DWORD xpfnProc);

W32KAPI
ULONG_PTR
NtUserCallHwndParamLock(
    IN HWND hwnd,
    IN ULONG_PTR dwParam,
    IN DWORD xpfnProc);

#endif  /* _USERCALL_ */

