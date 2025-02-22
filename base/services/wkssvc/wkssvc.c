/*
 *  ReactOS Services
 *  Copyright (C) 2015 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/wkssvc/wkssvc.c
 * PURPOSE:          Workstation service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wkssvc);

/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"lanmanworkstation";

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;

OSVERSIONINFOW VersionInfo;
HANDLE LsaHandle = NULL;
ULONG LsaAuthenticationPackage = 0;

/* FUNCTIONS *****************************************************************/

static VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = dwState;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;

    if (dwState == SERVICE_START_PENDING ||
        dwState == SERVICE_STOP_PENDING ||
        dwState == SERVICE_PAUSE_PENDING ||
        dwState == SERVICE_CONTINUE_PENDING)
        ServiceStatus.dwWaitHint = 10000;
    else
        ServiceStatus.dwWaitHint = 0;

    if (dwState == SERVICE_RUNNING)
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
}


static
DWORD
ServiceInit(VOID)
{
    LSA_STRING ProcessName = RTL_CONSTANT_STRING("Workstation");
    LSA_STRING PackageName = RTL_CONSTANT_STRING(MSV1_0_PACKAGE_NAME);
    LSA_OPERATIONAL_MODE Mode;
    HANDLE hThread;
    NTSTATUS Status;

    ERR("ServiceInit()\n");

    /* Get the OS version */
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionExW(&VersionInfo);

    InitWorkstationInfo();

    Status = LsaRegisterLogonProcess(&ProcessName,
                                     &LsaHandle,
                                     &Mode);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaRegisterLogonProcess() failed! (Status 0x%08lx)\n", Status);
        return 1;
    }

    Status = LsaLookupAuthenticationPackage(LsaHandle,
                                            &PackageName,
                                            &LsaAuthenticationPackage);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaLookupAuthenticationPackage() failed! (Status 0x%08lx)\n", Status);
        return 1;
    }

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if (!hThread)
    {
        ERR("Can't create PortThread\n");
        return GetLastError();
    }
    else
        CloseHandle(hThread);

    /* Report a running workstation service */
    SetServiceBits(ServiceStatusHandle,
                   SV_TYPE_WORKSTATION,
                   TRUE,
                   TRUE);

    return ERROR_SUCCESS;
}


static
VOID
ServiceShutdown(VOID)
{
    NTSTATUS Status;

    ERR("ServiceShutdown()\n");

    Status = LsaDeregisterLogonProcess(LsaHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaDeRegisterLogonProcess() failed! (Status 0x%08lx)\n", Status);
        return;
    }
}


static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    TRACE("ServiceControlHandler() called\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
            TRACE("  SERVICE_CONTROL_STOP received\n");
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            /* Stop listening to incoming RPC messages */
            RpcMgmtStopServerListening(NULL);
            ServiceShutdown();
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            TRACE("  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            TRACE("  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            TRACE("  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_SHUTDOWN:
            TRACE("  SERVICE_CONTROL_SHUTDOWN received\n");
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            /* Stop listening to incoming RPC messages */
            RpcMgmtStopServerListening(NULL);
            ServiceShutdown();
            UpdateServiceStatus(SERVICE_STOPPED);
            return ERROR_SUCCESS;

        default :
            TRACE("  Control %lu received\n", dwControl);
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}


VOID WINAPI
ServiceMain(DWORD argc, LPTSTR *argv)
{
    DWORD dwError;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    TRACE("ServiceMain() called\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        ERR("RegisterServiceCtrlHandlerExW() failed! (Error %lu)\n", GetLastError());
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    dwError = ServiceInit();
    if (dwError != ERROR_SUCCESS)
    {
        ERR("Service stopped (dwError: %lu\n", dwError);
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    UpdateServiceStatus(SERVICE_RUNNING);
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
