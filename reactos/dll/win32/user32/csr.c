/*
 * COPYRIGHT:       GNU LGPL v2.1 or any later version as
                    published by the Free Software Foundation
 * PROJECT:         ReactOS
 * FILE:            dll/win32/user32/csr.c
 * PURPOSE:         ReactOS-specific interaction with CSR
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
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

/* FUNCTIONS ******************************************************************/

/***********************************************************************
 *		ExitWindowsEx (USER32.@)
 */
BOOL WINAPI ExitWindowsEx( UINT uFlags, DWORD dwReason )
{
    CSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;

    CsrRequest = MAKE_CSR_API(EXIT_REACTOS, CSR_GUI);
    Request.Data.ExitReactosRequest.Flags = uFlags;
    Request.Data.ExitReactosRequest.Reserved = dwReason;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CsrRequest,
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		RegisterServicesProcess (USER32.@)
 */
BOOL WINAPI RegisterServicesProcess(DWORD ServicesProcessId)
{
    CSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;

    CsrRequest = MAKE_CSR_API(REGISTER_SERVICES_PROCESS, CSR_GUI);
    Request.Data.RegisterServicesProcessRequest.ProcessId = (HANDLE)ServicesProcessId;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CsrRequest,
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		RegisterLogonProcess (USER32.@)
 */
BOOL WINAPI RegisterLogonProcess(DWORD dwProcessId, BOOL bRegister)
{
    CSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;

    CsrRequest = MAKE_CSR_API(REGISTER_LOGON_PROCESS, CSR_GUI);
    Request.Data.RegisterLogonProcessRequest.ProcessId = dwProcessId;
    Request.Data.RegisterLogonProcessRequest.Register = bRegister;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CsrRequest,
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *		SetLogonNotifyWindow (USER32.@)
 */
BOOL
WINAPI
SetLogonNotifyWindow (HWND Wnd, HWINSTA WinSta)
{
    /* Maybe we should call NtUserSetLogonNotifyWindow and let that one inform CSRSS??? */
    CSR_API_MESSAGE Request;
    ULONG CsrRequest;
    NTSTATUS Status;

    CsrRequest = MAKE_CSR_API(SET_LOGON_NOTIFY_WINDOW, CSR_GUI);
    Request.Data.SetLogonNotifyWindowRequest.LogonNotifyWindow = Wnd;

    Status = CsrClientCallServer(&Request,
                                 NULL,
                                 CsrRequest,
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}



/* EOF */
