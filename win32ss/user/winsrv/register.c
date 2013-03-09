/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/register.c
 * PURPOSE:         Register logon window and services process
 * PROGRAMMERS:     Eric Kohl
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "winsrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static BOOLEAN ServicesProcessIdValid = FALSE;
static ULONG_PTR ServicesProcessId = 0;

HWND LogonNotifyWindow = NULL;
ULONG_PTR LogonProcessId = 0;

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvRegisterLogonProcess)
{
    PCSRSS_REGISTER_LOGON_PROCESS RegisterLogonProcessRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.RegisterLogonProcessRequest;

    if (RegisterLogonProcessRequest->Register)
    {
        if (LogonProcessId != 0)
            return STATUS_LOGON_SESSION_EXISTS;

        LogonProcessId = RegisterLogonProcessRequest->ProcessId;
    }
    else
    {
        if (ApiMessage->Header.ClientId.UniqueProcess != (HANDLE)LogonProcessId)
        {
            DPRINT1("Current logon process 0x%x, can't deregister from process 0x%x\n",
                    LogonProcessId, ApiMessage->Header.ClientId.UniqueProcess);
            return STATUS_NOT_LOGON_PROCESS;
        }

        LogonProcessId = 0;
    }

    return STATUS_SUCCESS;
}

/// HACK: ReactOS-specific
CSR_API(RosSetLogonNotifyWindow)
{
    PCSRSS_SET_LOGON_NOTIFY_WINDOW SetLogonNotifyWindowRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.SetLogonNotifyWindowRequest;
    DWORD WindowCreator;

    if (0 == GetWindowThreadProcessId(SetLogonNotifyWindowRequest->LogonNotifyWindow,
                                      &WindowCreator))
    {
        DPRINT1("Can't get window creator\n");
        return STATUS_INVALID_HANDLE;
    }
    if (WindowCreator != LogonProcessId)
    {
        DPRINT1("Trying to register window not created by winlogon as notify window\n");
        return STATUS_ACCESS_DENIED;
    }

    LogonNotifyWindow = SetLogonNotifyWindowRequest->LogonNotifyWindow;

    return STATUS_SUCCESS;
}

CSR_API(SrvRegisterServicesProcess)
{
    PCSRSS_REGISTER_SERVICES_PROCESS RegisterServicesProcessRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.RegisterServicesProcessRequest;

    if (ServicesProcessIdValid == TRUE)
    {
        /* Only accept a single call */
        return STATUS_INVALID_PARAMETER;
    }
    else
    {
        ServicesProcessId = RegisterServicesProcessRequest->ProcessId;
        ServicesProcessIdValid = TRUE;
        return STATUS_SUCCESS;
    }
}

/* EOF */
