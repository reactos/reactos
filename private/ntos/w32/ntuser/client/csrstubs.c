/***************************** Module Header ******************************\
* Module Name: csrstubs.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Routines to call CSR
*
* 02-27-95 JimA             Created.
*
* Note: This file has been partitioned with #if defines so that the LPC
* marsheling code can be inside 64bit code when running under wow64(32bit on 64bit NT).
* In wow64, the system DLLs for 32bit processes are 32bit.
*
*       The marsheling code can only be depedent on functions NTDLL.
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "csrmsg.h"
#include "dbt.h"
#include "csrhlpr.h"

#if defined(BUILD_CSRWOW64)

#undef RIPERR0
#undef RIPNTERR0
#undef RIPMSG0

#define RIPNTERR0(status, flags, szFmt) {if (NtCurrentTeb()) NtCurrentTeb()->LastErrorValue = RtlNtStatusToDosError(status);}
#define RIPERR0(idErr, flags, szFmt) {if (NtCurrentTeb()) NtCurrentTeb()->LastErrorValue = (idErr);}
#define RIPMSG0(flags, szFmt)

#endif

#define SET_LAST_ERROR_RETURNED()   if (a->dwLastError) RIPERR0(a->dwLastError, RIP_VERBOSE, "")

#if !defined(BUILD_WOW6432)

NTSTATUS
APIENTRY
CallUserpExitWindowsEx(
    IN UINT uFlags,
    IN DWORD dwReserved,
    OUT PBOOL pfSuccess)
{

    USER_API_MSG m;
    PEXITWINDOWSEXMSG a = &m.u.ExitWindowsEx;

    a->uFlags = uFlags;
    a->dwReserved = dwReserved;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              UserpExitWindowsEx
                                            ),
                         sizeof( *a )
                       );

    if (NT_SUCCESS( m.ReturnValue ) || m.ReturnValue == STATUS_CANT_WAIT) {
        SET_LAST_ERROR_RETURNED();
        *pfSuccess = a->fSuccess;
    } else {
        RIPNTERR0(m.ReturnValue, RIP_VERBOSE, "");
        *pfSuccess = FALSE;
    }

    return m.ReturnValue;

}

#endif

#if !defined(BUILD_CSRWOW64)

typedef struct _EXITWINDOWSDATA {
    UINT uFlags;
    DWORD dwReserved;
} EXITWINDOWSDATA, *PEXITWINDOWSDATA;

DWORD ExitWindowsThread(PVOID pvParam);

BOOL WINAPI ExitWindowsWorker(
    UINT uFlags,
    DWORD dwReserved,
    BOOL fSecondThread)
{
    EXITWINDOWSDATA ewd;
    HANDLE hThread;
    DWORD dwThreadId;
    DWORD dwExitCode;
    DWORD idWait;
    MSG msg;
    BOOL fSuccess;
    NTSTATUS Status;

    /*
     * Force a connection so apps will have a windowstation
     * to log off of.
     */
    if (PtiCurrent() == NULL)
        return FALSE;

    /*
     * Check for UI restrictions
     */

    if (!NtUserCallOneParam((ULONG_PTR)uFlags, SFI_PREPAREFORLOGOFF)) {
        RIPMSG0(RIP_WARNING, "ExitWindows called by a restricted thread\n");
        return FALSE;
    }

    Status = CallUserpExitWindowsEx(uFlags, dwReserved, &fSuccess);

    if (NT_SUCCESS( Status )) {
        return fSuccess;
    } else if (Status == STATUS_CANT_WAIT && !fSecondThread) {
        ewd.uFlags = uFlags;
        ewd.dwReserved = dwReserved;
        hThread = CreateThread(NULL, 0, ExitWindowsThread, &ewd,
                0, &dwThreadId);
        if (hThread == NULL) {
            return FALSE;
        }
        while (1) {
            idWait = MsgWaitForMultipleObjectsEx(1, &hThread,
                    INFINITE, QS_ALLINPUT, 0);

            /*
             * If the thread was signaled, we're done.
             */
            if (idWait == WAIT_OBJECT_0)
                break;

            /*
             * Process any waiting messages
             */
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                DispatchMessage(&msg);
        }
        GetExitCodeThread(hThread, &dwExitCode);
        NtClose(hThread);
        if (dwExitCode == ERROR_SUCCESS)
            return TRUE;
        else {
            RIPERR0(dwExitCode, RIP_VERBOSE, "");
            return FALSE;
        }
    } else {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return FALSE;
    }
}

DWORD ExitWindowsThread(
    PVOID pvParam)
{
    PEXITWINDOWSDATA pewd = pvParam;
    DWORD dwExitCode;

    if (ExitWindowsWorker(pewd->uFlags, pewd->dwReserved, TRUE))
        dwExitCode = 0;
    else
        dwExitCode = GetLastError();
    ExitThread(dwExitCode);
    return 0;
}

BOOL WINAPI ExitWindowsEx(
    UINT uFlags,
    DWORD dwReserved)
{
    return ExitWindowsWorker(uFlags, dwReserved, FALSE);
}

#endif

#if !defined(BUILD_WOW6432)

BOOL WINAPI EndTask(
    HWND hwnd,
    BOOL fShutdown,
    BOOL fForce)
{
    USER_API_MSG m;
    PENDTASKMSG a = &m.u.EndTask;

    a->hwnd = hwnd;
    a->fShutdown = fShutdown;
    a->fForce = fForce;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              UserpEndTask
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        SET_LAST_ERROR_RETURNED();
        return a->fSuccess;
    } else {
        RIPNTERR0(m.ReturnValue, RIP_VERBOSE, "");
        return FALSE;
    }
}

VOID
APIENTRY
Logon(
    BOOL fLogon)
{
    USER_API_MSG m;
    PLOGONMSG a = &m.u.Logon;

    a->fLogon = fLogon;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              UserpLogon
                                            ),
                         sizeof(*a)
                       );
}

NTSTATUS
APIENTRY
CallUserpRegisterLogonProcess(
    IN DWORD dwProcessId)
{

    USER_API_MSG m;
    PLOGONMSG a = &m.u.Logon;
    NTSTATUS Status;

    m.u.IdLogon = dwProcessId;
    Status = CsrClientCallServer( (PCSR_API_MSG)&m,
                                  NULL,
                                  CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                                       UserpRegisterLogonProcess),
                                  sizeof(*a));

    return Status;
}

#endif

#if !defined(BUILD_CSRWOW64)

extern BOOL gfLogonProcess;

BOOL RegisterLogonProcess(
    DWORD dwProcessId,
    BOOL fSecure)
{
    gfLogonProcess = (BOOL)NtUserCallTwoParam(dwProcessId, fSecure,
            SFI__REGISTERLOGONPROCESS);

    /*
     * Now, register the logon process into winsrv.
     */
    if (gfLogonProcess) {
        CallUserpRegisterLogonProcess(dwProcessId);
    }

    return gfLogonProcess;
}

#endif

#if !defined(BUILD_WOW6432)

BOOL
WINAPI
RegisterServicesProcess(
    DWORD dwProcessId)
{
    USER_API_MSG m;
    PREGISTERSERVICESPROCESSMSG a = &m.u.RegisterServicesProcess;

    a->dwProcessId = dwProcessId;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              UserpRegisterServicesProcess
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        SET_LAST_ERROR_RETURNED();
        return a->fSuccess;
    } else {
        RIPNTERR0(m.ReturnValue, RIP_VERBOSE, "");
        return FALSE;
    }
}

HDESK WINAPI GetThreadDesktop(
    DWORD dwThreadId)
{
    USER_API_MSG m;
    PGETTHREADCONSOLEDESKTOPMSG a = &m.u.GetThreadConsoleDesktop;

    a->dwThreadId = dwThreadId;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              UserpGetThreadConsoleDesktop
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return NtUserGetThreadDesktop(dwThreadId, a->hdeskConsole);
    } else {
        RIPNTERR0(m.ReturnValue, RIP_VERBOSE, "");
        return NULL;
    }
}


/**************************************************************************\
* DeviceEventWorker
*
* This is a private (not publicly exported) interface that the user-mode
* pnp manager calls when it needs to send a WM_DEVICECHANGE message to a
* specific window handle. The user-mode pnp manager is a service within
* services.exe and as such is not on the interactive window station and
* active desktop, so it can't directly call SendMessage. For broadcasted
* messages (messages that go to all top-level windows), the user-mode pnp
* manager calls BroadcastSystemMessage directly.
*
* PaulaT 06/04/97
*
\**************************************************************************/

ULONG
WINAPI
DeviceEventWorker(
    IN HWND    hWnd,
    IN WPARAM  wParam,
    IN LPARAM  lParam,
    IN DWORD   dwFlags,
    OUT PDWORD pdwResult
    )
{
    USER_API_MSG m;
    PDEVICEEVENTMSG a = &m.u.DeviceEvent;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;
    int cb = 0;

    a->hWnd     = hWnd;
    a->wParam   = wParam;
    a->lParam   = lParam;
    a->dwFlags  = dwFlags;
    a->dwResult = 0;

    //
    // If lParam is specified, it must be marshalled (see the defines
    // for this structure in dbt.h - the structure always starts with
    // DEV_BROADCAST_HDR structure).
    //

    if (lParam) {

        cb = ((PDEV_BROADCAST_HDR)lParam)->dbch_size;

        CaptureBuffer = CsrAllocateCaptureBuffer(1, cb);
        if (CaptureBuffer == NULL) {
            return STATUS_NO_MEMORY;
        }

        CsrCaptureMessageBuffer(CaptureBuffer,
                                (PCHAR)lParam,
                                cb,
                                (PVOID *)&a->lParam);

        //
        // This ends up calling SrvDeviceEvent routine in the server.
        //

        CsrClientCallServer((PCSR_API_MSG)&m,
                            CaptureBuffer,
                            CSR_MAKE_API_NUMBER(USERSRV_SERVERDLL_INDEX,
                                                UserpDeviceEvent),
                            sizeof(*a));

        CsrFreeCaptureBuffer(CaptureBuffer);

    } else {

        //
        // This ends up calling SrvDeviceEvent routine in the server.
        //

        CsrClientCallServer((PCSR_API_MSG)&m,
                            NULL,
                            CSR_MAKE_API_NUMBER(USERSRV_SERVERDLL_INDEX,
                                                UserpDeviceEvent),
                            sizeof(*a));
    }


    if (NT_SUCCESS(m.ReturnValue)) {
        *pdwResult = (DWORD)a->dwResult;
    } else {
        RIPMSG0(RIP_WARNING, "DeviceEventWorker failed.");
    }

    return m.ReturnValue;

} // DeviceEventWorker

#if DBG

VOID
APIENTRY
CsrWin32HeapFail(
    IN DWORD dwFlags,
    IN BOOL  bFail)
{
    USER_API_MSG m;
    PWIN32HEAPFAILMSG a = &m.u.Win32HeapFail;

    a->dwFlags = dwFlags;
    a->bFail = bFail;

    CsrClientCallServer((PCSR_API_MSG)&m,
                        NULL,
                        CSR_MAKE_API_NUMBER(USERSRV_SERVERDLL_INDEX,
                                            UserpWin32HeapFail),
                        sizeof(*a));

    if (!NT_SUCCESS(m.ReturnValue)) {
        RIPNTERR0(m.ReturnValue, RIP_VERBOSE, "UserpWin32HeapFail failed");
    }
}

UINT
APIENTRY
CsrWin32HeapStat(
    PDBGHEAPSTAT    phs,
    DWORD   dwLen)
{
    USER_API_MSG m;
    PWIN32HEAPSTATMSG a = &m.u.Win32HeapStat;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;

    a->dwLen = dwLen;

    CaptureBuffer = CsrAllocateCaptureBuffer(1, dwLen);
    if (CaptureBuffer == NULL) {
        return 0;
    }

    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PCHAR)phs,
                            dwLen,
                            (PVOID *)&a->phs);

    CsrClientCallServer((PCSR_API_MSG)&m,
                        CaptureBuffer,
                        CSR_MAKE_API_NUMBER(USERSRV_SERVERDLL_INDEX,
                                            UserpWin32HeapStat),
                        sizeof(*a));

    if (!NT_SUCCESS(m.ReturnValue)) {
        RIPNTERR0(m.ReturnValue, RIP_VERBOSE, "UserpWin32HeapStat failed");
        a->dwMaxTag = 0;
        goto ErrExit;
    }
    RtlMoveMemory(phs, a->phs, dwLen);

ErrExit:
    CsrFreeCaptureBuffer(CaptureBuffer);

    return a->dwMaxTag;
}

#endif // DBG


#endif

