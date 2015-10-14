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
//#include <ddk/ntstatus.h>

/* CSRSS Headers */
#include <subsys/csr/csr.h>
#include <csr_shared.h>
#include <subsys/win/winmsg.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(usercsr);


/* FUNCTIONS ******************************************************************/

/***********************************************************************
 *		ExitWindowsEx (USER32.@)
 */
BOOL WINAPI ExitWindowsEx( UINT uFlags, DWORD dwReserved )
{
    NTSTATUS Status;
    USER_API_MESSAGE ApiMessage;
    PUSER_EXIT_REACTOS ExitReactOSRequest = &ApiMessage.Data.ExitReactOSRequest;

    ExitReactOSRequest->Flags = uFlags;
    //ExitReactOSRequest->Reserved = dwReserved;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpExitWindowsEx),
                                 sizeof(USER_EXIT_REACTOS));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        ExitReactOSRequest->Success = FALSE;
        return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *		RegisterServicesProcess (USER32.@)
 */
BOOL WINAPI
RegisterServicesProcess(DWORD ServicesProcessId)
{
    NTSTATUS Status;
    USER_API_MESSAGE ApiMessage;

    ApiMessage.Data.RegisterServicesProcessRequest.ProcessId = ServicesProcessId;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpRegisterServicesProcess),
                                 sizeof(USER_REGISTER_SERVICES_PROCESS));
    if (!NT_SUCCESS(Status))
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
        NTSTATUS Status;
        USER_API_MESSAGE ApiMessage;

        ApiMessage.Data.RegisterLogonProcessRequest.ProcessId = dwProcessId;
        ApiMessage.Data.RegisterLogonProcessRequest.Register = bRegister;

        Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                     NULL,
                                     CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpRegisterLogonProcess),
                                     sizeof(USER_REGISTER_LOGON_PROCESS));
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to register logon process with CSRSS\n");
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
SetLogonNotifyWindow (HWND Wnd)
{
#if 0
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
#else
    ERR("SetLogonNotifyWindow is not yet implemented in Arwinss\n");
    return FALSE;
#endif
}

/* EOF */
