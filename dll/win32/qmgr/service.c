/*
 * ServiceMain function for qmgr running within svchost
 *
 * Copyright 2007 (C) Google (Roy Shea)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "windef.h"
#include "winsvc.h"
#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

HANDLE stop_event = NULL;

static SERVICE_STATUS_HANDLE status_handle;
static SERVICE_STATUS status;

static VOID
UpdateStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = dwCurrentState;
    if (dwCurrentState == SERVICE_START_PENDING)
        status.dwControlsAccepted = 0;
    else
        status.dwControlsAccepted
            = (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE
               | SERVICE_ACCEPT_SHUTDOWN);
    status.dwWin32ExitCode = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint = 0;
    status.dwWaitHint = dwWaitHint;

    if (!SetServiceStatus(status_handle, &status)) {
        ERR("failed to set service status\n");
        SetEvent(stop_event);
    }
}

/* Handle incoming ControlService signals */
static DWORD WINAPI
ServiceHandler(DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context)
{
    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        TRACE("shutting down service\n");
        UpdateStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(stop_event);
        break;
    default:
        FIXME("ignoring handle service ctrl %lx\n", ctrl);
        UpdateStatus(status.dwCurrentState, NO_ERROR, 0);
        break;
    }

    return NO_ERROR;
}

/* Main thread of the service */
static BOOL
StartCount(void)
{
    HRESULT hr;
    DWORD dwReg;

    TRACE("\n");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return FALSE;

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                              RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,
                              NULL);
    if (FAILED(hr))
        return FALSE;

    hr = CoRegisterClassObject(&CLSID_BackgroundCopyManager,
                               (IUnknown *) &BITS_ClassFactory.IClassFactory_iface,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &dwReg);
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

/* Service entry point */
VOID WINAPI
ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
    HANDLE fileTxThread;
    DWORD threadId;
    TRACE("\n");

    stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!stop_event) {
        ERR("failed to create stop_event\n");
        return;
    }

    status_handle = RegisterServiceCtrlHandlerExW(L"BITS", ServiceHandler, NULL);
    if (!status_handle) {
        ERR("failed to register handler: %lu\n", GetLastError());
        return;
    }

    UpdateStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    if (!StartCount()) {
        ERR("failed starting service thread\n");
        UpdateStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    globalMgr.jobEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!globalMgr.jobEvent) {
        ERR("Couldn't create event: error %ld\n", GetLastError());
        UpdateStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    fileTxThread = CreateThread(NULL, 0, fileTransfer, NULL, 0, &threadId);
    if (!fileTxThread)
    {
        ERR("Failed starting file transfer thread\n");
        UpdateStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    UpdateStatus(SERVICE_RUNNING, NO_ERROR, 0);

    WaitForSingleObject(fileTxThread, INFINITE);
    UpdateStatus(SERVICE_STOPPED, NO_ERROR, 0);
    CloseHandle(stop_event);
    TRACE("service stopped\n");

    CoUninitialize();
}
