/* $Id: services.c,v 1.17 2004/04/12 17:14:55 navaraf Exp $
 *
 * service control manager
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 */

/* NOTE:
 * - Services.exe is NOT a native application, it is a GUI app.
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <stdio.h>
#include <windows.h>
#undef CreateService
#undef OpenService

#include "services.h"
#include <services/scmprot.h>

#define NDEBUG
#include <debug.h>



/* GLOBALS ******************************************************************/

#define PIPE_TIMEOUT 10000


/* FUNCTIONS *****************************************************************/

void
PrintString(char* fmt,...)
{
#ifdef DBG
    char buffer[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    OutputDebugStringA(buffer);
#endif
}


BOOL
ScmCreateStartEvent(PHANDLE StartEvent)
{
    HANDLE hEvent;

    hEvent = CreateEvent(NULL,
                         TRUE,
                         FALSE,
                         _T("SvcctrlStartEvent_A3725DX"));
    if (hEvent == NULL) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            hEvent = OpenEvent(EVENT_ALL_ACCESS,
                               FALSE,
                               _T("SvcctrlStartEvent_A3725DX"));
            if (hEvent == NULL) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }
    *StartEvent = hEvent;
    return TRUE;
}


typedef struct _SERVICE_THREAD_DATA
{
    HANDLE hPipe;
    PSERVICE pService;
} SERVICE_THREAD_DATA, *PSERVICE_THREAD_DATA;


VOID FASTCALL
ScmHandleDeleteServiceRequest(
    PSERVICE_THREAD_DATA pServiceThreadData,
    PSCM_SERVICE_REQUEST Request,
    DWORD RequestSize,
    PSCM_SERVICE_REPLY Reply,
    LPDWORD ReplySize)
{
    Reply->ReplyStatus = RtlNtStatusToDosError(
        ScmStartService(pServiceThreadData->pService, NULL));
    *ReplySize = sizeof(DWORD);
}


VOID FASTCALL
ScmHandleStartServiceRequest(
    PSERVICE_THREAD_DATA pServiceThreadData,
    PSCM_SERVICE_REQUEST Request,
    DWORD RequestSize,
    PSCM_SERVICE_REPLY Reply,
    LPDWORD ReplySize)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE KeyHandle;

    InitializeObjectAttributes(
       &ObjectAttributes,
       &pServiceThreadData->pService->RegistryPath,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL);
        						  
    Status = ZwOpenKey(
       &KeyHandle,
       KEY_READ,
       &ObjectAttributes);

    if (NT_SUCCESS(Status))
    {
       Status = ZwDeleteKey(KeyHandle);
       if (NT_SUCCESS(Status))
       {
          ZwClose(KeyHandle);
          CHECKPOINT;
          RemoveEntryList(&pServiceThreadData->pService->ServiceListEntry);
       }
    }
    Reply->ReplyStatus = RtlNtStatusToDosError(Status);
    *ReplySize = sizeof(DWORD);
}


DWORD
WINAPI
ScmNamedPipeServiceThread(LPVOID Context)
{
    SCM_SERVICE_REQUEST Request;
    SCM_SERVICE_REPLY Reply;
    DWORD cbBytesRead;
    DWORD cbWritten, cbReplyBytes;
    BOOL fSuccess;
    PSERVICE_THREAD_DATA pServiceThreadData = Context;

    ConnectNamedPipe(pServiceThreadData->hPipe, NULL);

    for (;;)
    {
        fSuccess = ReadFile(
            pServiceThreadData->hPipe,
            &Request,
            sizeof(Request),
            &cbBytesRead,
            NULL);

        if (!fSuccess || cbBytesRead == 0)
        {
            break;
        }

        switch (Request.RequestCode)
        {
            case SCM_DELETESERVICE:
                ScmHandleDeleteServiceRequest(
                    pServiceThreadData,
                    &Request,
                    cbBytesRead,
                    &Reply,
                    &cbReplyBytes);
                break;

            case SCM_STARTSERVICE:
                ScmHandleStartServiceRequest(
                    pServiceThreadData,
                    &Request,
                    cbBytesRead,
                    &Reply,
                    &cbReplyBytes);
                break;
        }

        fSuccess = WriteFile(
            pServiceThreadData->hPipe,
            &Reply,
            cbReplyBytes,
            &cbWritten,
            NULL);

        if (!fSuccess || cbReplyBytes != cbWritten)
        {
            break;
        }
    }

    FlushFileBuffers(pServiceThreadData->hPipe);
    DisconnectNamedPipe(pServiceThreadData->hPipe);
    CloseHandle(pServiceThreadData->hPipe);

    return NO_ERROR;
}


VOID FASTCALL
ScmCreateServiceThread(
    HANDLE hSCMPipe,
    PSCM_OPENSERVICE_REPLY Reply,
    LPDWORD ReplySize,
    PSERVICE Service)
{
    HANDLE hPipe, hThread;
    DWORD dwThreadId;
    PSERVICE_THREAD_DATA ServiceThreadData;

    ServiceThreadData = HeapAlloc(
        GetProcessHeap(),
        0,
        sizeof(SERVICE_THREAD_DATA));

    if (ServiceThreadData == NULL)
    {
        Reply->ReplyStatus = ERROR_NO_SYSTEM_RESOURCES;
        CHECKPOINT;
        return;
    }

    swprintf(Reply->PipeName, L"\\\\.\\pipe\\SCM.%08X.%08X", hSCMPipe, Service);

    hPipe = CreateNamedPipeW(
        Reply->PipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        sizeof(SCM_SERVICE_REQUEST),
        sizeof(SCM_SERVICE_REPLY),
        PIPE_TIMEOUT,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        Reply->ReplyStatus = ERROR_NO_SYSTEM_RESOURCES;
        CHECKPOINT;
        return;
    }

    ServiceThreadData->hPipe = hPipe;
    ServiceThreadData->pService = Service;

    hThread = CreateThread(
        NULL,
        0,
        ScmNamedPipeServiceThread,
        ServiceThreadData,
        0,
        &dwThreadId);

    if (!hThread)
    {
        Reply->ReplyStatus = ERROR_NO_SYSTEM_RESOURCES;
        CHECKPOINT;
        CloseHandle(hPipe);
        return;
    }

    Reply->ReplyStatus = NO_ERROR;
    *ReplySize = sizeof(SCM_OPENSERVICE_REPLY);
}


VOID FASTCALL
ScmHandleOpenServiceRequest(
    HANDLE hPipe,
    PSCM_REQUEST Request,
    DWORD RequestSize,
    PSCM_REPLY Reply,
    LPDWORD ReplySize)
{
    PSERVICE Service;
    UNICODE_STRING ServiceName;

    DPRINT("OpenService\n");

    if (RequestSize != sizeof(SCM_OPENSERVICE_REQUEST))
    {
        Reply->ReplyStatus = ERROR_INVALID_PARAMETER;
        return;
    }

    ServiceName.Length = Request->OpenService.ServiceName.Length;
    ServiceName.Buffer = Request->OpenService.ServiceName.Buffer;
    Service = ScmFindService(&ServiceName);
    if (Service == NULL)
    {
        DPRINT("OpenService - Service not found\n");
        Reply->ReplyStatus = ERROR_SERVICE_DOES_NOT_EXIST;
        return;
    }

    DPRINT("OpenService - Service found\n");
    ScmCreateServiceThread(
        hPipe,
        &Reply->OpenService,
        ReplySize,
        Service);
}


VOID FASTCALL
ScmHandleCreateServiceRequest(
    HANDLE hPipe,
    PSCM_REQUEST Request,
    DWORD RequestSize,
    PSCM_REPLY Reply,
    LPDWORD ReplySize)
{
    PSERVICE Service;
    NTSTATUS Status;
    UNICODE_STRING ServiceName;

    DPRINT("CreateService - %S\n", Request->CreateService.ServiceName.Buffer);

    if (RequestSize != sizeof(SCM_CREATESERVICE_REQUEST))
    {
        Reply->ReplyStatus = ERROR_INVALID_PARAMETER;
        return;
    }

    Status = RtlCheckRegistryKey(
        RTL_REGISTRY_SERVICES,
        Request->CreateService.ServiceName.Buffer);

    if (NT_SUCCESS(Status))
    {
        DPRINT("CreateService - Already exists\n");
        Reply->ReplyStatus = ERROR_SERVICE_EXISTS;
        return;
    }

    Status = RtlCreateRegistryKey(
        RTL_REGISTRY_SERVICES,
        Request->CreateService.ServiceName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("CreateService - Can't create key (%x)\n", Status);
        Reply->ReplyStatus = Status;
        return;
    }

    Status = RtlWriteRegistryValue(
        RTL_REGISTRY_SERVICES,
        Request->CreateService.ServiceName.Buffer,
        L"Type",
        REG_DWORD,
        &Request->CreateService.dwServiceType,
        sizeof(DWORD));

    if (!NT_SUCCESS(Status))
    {
        DPRINT("CreateService - Error writing value\n");
        Reply->ReplyStatus = RtlNtStatusToDosError(Status);
        return;
    }

    Status = RtlWriteRegistryValue(
        RTL_REGISTRY_SERVICES,
        Request->CreateService.ServiceName.Buffer,
        L"Start",
        REG_DWORD,
        &Request->CreateService.dwStartType,
        sizeof(DWORD));

    if (!NT_SUCCESS(Status))
    {
        Reply->ReplyStatus = RtlNtStatusToDosError(Status);
        return;
    }

    Status = RtlWriteRegistryValue(
        RTL_REGISTRY_SERVICES,
        Request->CreateService.ServiceName.Buffer,
        L"ErrorControl",
        REG_DWORD,
        &Request->CreateService.dwErrorControl,
        sizeof(DWORD));

    if (!NT_SUCCESS(Status))
    {
        Reply->ReplyStatus = RtlNtStatusToDosError(Status);
        return;
    }

    if (Request->CreateService.BinaryPathName.Length)
    {
        Status = RtlWriteRegistryValue(
            RTL_REGISTRY_SERVICES,
            Request->CreateService.ServiceName.Buffer,
            L"ImagePath",
            REG_SZ,
            Request->CreateService.BinaryPathName.Buffer,
            Request->CreateService.BinaryPathName.Length + sizeof(UNICODE_NULL));

        if (!NT_SUCCESS(Status))
        {
            DPRINT("CreateService - Error writing value\n");
            Reply->ReplyStatus = RtlNtStatusToDosError(Status);
            return;
        }
    }

    if (Request->CreateService.LoadOrderGroup.Length)
    {
        Status = RtlWriteRegistryValue(
            RTL_REGISTRY_SERVICES,
            Request->CreateService.ServiceName.Buffer,
            L"Group",
            REG_SZ,
            Request->CreateService.LoadOrderGroup.Buffer,
            Request->CreateService.LoadOrderGroup.Length + sizeof(UNICODE_NULL));

        if (!NT_SUCCESS(Status))
        {
            Reply->ReplyStatus = RtlNtStatusToDosError(Status);
            return;
        }
    }

    if (Request->CreateService.Dependencies.Length)
    {
        Status = RtlWriteRegistryValue(
            RTL_REGISTRY_SERVICES,
            Request->CreateService.ServiceName.Buffer,
            L"Dependencies",
            REG_SZ,
            Request->CreateService.Dependencies.Buffer,
            Request->CreateService.Dependencies.Length + sizeof(UNICODE_NULL));

        if (!NT_SUCCESS(Status))
        {
            Reply->ReplyStatus = RtlNtStatusToDosError(Status);
            return;
        }
    }

    if (Request->CreateService.DisplayName.Length)
    {
        Status = RtlWriteRegistryValue(
            RTL_REGISTRY_SERVICES,
            Request->CreateService.ServiceName.Buffer,
            L"DisplayName",
            REG_SZ,
            Request->CreateService.DisplayName.Buffer,
            Request->CreateService.DisplayName.Length + sizeof(UNICODE_NULL));

        if (!NT_SUCCESS(Status))
        {
            Reply->ReplyStatus = RtlNtStatusToDosError(Status);
            return;
        }
    }

    if (Request->CreateService.ServiceStartName.Length ||
        Request->CreateService.Password.Length)
    {
        DPRINT1("Unimplemented case\n");
    }

    ServiceName.Length = Request->OpenService.ServiceName.Length;
    ServiceName.Buffer = Request->OpenService.ServiceName.Buffer;
    Service = ScmCreateServiceListEntry(&ServiceName);
    if (Service == NULL)
    {
        DPRINT("CreateService - Error creating service list entry\n");
        Reply->ReplyStatus = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    DPRINT("CreateService - Success\n");
    ScmCreateServiceThread(
        hPipe,
        &Reply->OpenService,
        ReplySize,
        Service);
}


BOOL
ScmNamedPipeHandleRequest(
    HANDLE hPipe,
    PSCM_REQUEST Request,
    DWORD RequestSize,
    PSCM_REPLY Reply,
    LPDWORD ReplySize)
{
    *ReplySize = sizeof(DWORD);

    if (RequestSize < sizeof(DWORD))
    {
        Reply->ReplyStatus = ERROR_INVALID_PARAMETER;
        return TRUE;
    }

    DPRINT("RequestCode: %x\n", Request->RequestCode);

    switch (Request->RequestCode)
    {
        case SCM_OPENSERVICE:
            ScmHandleOpenServiceRequest(
                hPipe,
                Request,
                RequestSize,
                Reply,
                ReplySize);
            break;

        case SCM_CREATESERVICE:
            ScmHandleCreateServiceRequest(
                hPipe,
                Request,
                RequestSize,
                Reply,
                ReplySize);
            break;

        default:
            Reply->ReplyStatus = ERROR_INVALID_PARAMETER;
    }

    return TRUE;
}


DWORD
WINAPI
ScmNamedPipeThread(LPVOID Context)
{
    SCM_REQUEST Request;
    SCM_REPLY Reply;
    DWORD cbReplyBytes;
    DWORD cbBytesRead;
    DWORD cbWritten;
    BOOL fSuccess;
    HANDLE hPipe;

    hPipe = (HANDLE)Context;

    DPRINT("ScmNamedPipeThread(%x) - Accepting SCM commands through named pipe\n", hPipe);
    
    for (;;) {
        fSuccess = ReadFile(hPipe,
                            &Request,
                            sizeof(Request),
                            &cbBytesRead,
                            NULL);
        if (!fSuccess || cbBytesRead == 0) {
            break;
        }
        if (ScmNamedPipeHandleRequest(hPipe, &Request, cbBytesRead, &Reply, &cbReplyBytes)) {
            fSuccess = WriteFile(hPipe,
                                 &Reply,
                                 cbReplyBytes,
                                 &cbWritten,
                                 NULL);
            if (!fSuccess || cbReplyBytes != cbWritten) {
                break;
            }
        }
    }
    DPRINT("ScmNamedPipeThread(%x) - Disconnecting named pipe connection\n", hPipe);
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    DPRINT("ScmNamedPipeThread(%x) - Done.\n", hPipe);
    return ERROR_SUCCESS;
}

BOOL ScmCreateNamedPipe(VOID)
{
    DWORD dwThreadId;
    BOOL fConnected;
    HANDLE hThread;
    HANDLE hPipe;

    DPRINT("ScmCreateNamedPipe() - CreateNamedPipe(\"\\\\.\\pipe\\Ntsvcs\")\n");

    hPipe = CreateNamedPipe("\\\\.\\pipe\\Ntsvcs",
              PIPE_ACCESS_DUPLEX,
              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
              PIPE_UNLIMITED_INSTANCES,
              sizeof(SCM_REQUEST),
              sizeof(SCM_REPLY),
              PIPE_TIMEOUT,
              NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        DPRINT("CreateNamedPipe() failed (%d)\n", GetLastError());
        return FALSE;
    }

    DPRINT("CreateNamedPipe() - calling ConnectNamedPipe(%x)\n", hPipe);
    fConnected = ConnectNamedPipe(hPipe,
                   NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    DPRINT("CreateNamedPipe() - ConnectNamedPipe() returned %d\n", fConnected);

    if (fConnected) {
        DPRINT("Pipe connected\n");
        hThread = CreateThread(NULL,
                               0,
                               ScmNamedPipeThread,
                               (LPVOID)hPipe,
                               0,
                               &dwThreadId);
        if (!hThread) {
            DPRINT("Could not create thread (%d)\n", GetLastError());
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            DPRINT("CreateNamedPipe() - returning FALSE\n");
           return FALSE;
        }
    } else {
        DPRINT("Pipe not connected\n");
        CloseHandle(hPipe);
        DPRINT("CreateNamedPipe() - returning FALSE\n");
        return FALSE;
    }
    DPRINT("CreateNamedPipe() - returning TRUE\n");
    return TRUE;
}

DWORD
WINAPI
ScmNamedPipeListenerThread(LPVOID Context)
{
//    HANDLE hPipe;
    DPRINT("ScmNamedPipeListenerThread(%x) - aka SCM.\n", Context);

//    hPipe = (HANDLE)Context;
    for (;;) {
        DPRINT("SCM: Waiting for new connection on named pipe...\n");
        /* Create named pipe */
        if (!ScmCreateNamedPipe()) {
            DPRINT1("\nSCM: Failed to create named pipe\n");
            break;
            //ExitThread(0);
        }
        DPRINT("\nSCM: named pipe session created.\n");
        Sleep(10);
    }
    DPRINT("\n\nWARNING: ScmNamedPipeListenerThread(%x) - Aborted.\n\n", Context);
    return ERROR_SUCCESS;
}

BOOL StartScmNamedPipeThreadListener(void)
{
    DWORD dwThreadId;
    HANDLE hThread;

    hThread = CreateThread(NULL,
                 0,
                 ScmNamedPipeListenerThread,
                 NULL, /*(LPVOID)hPipe,*/
                 0,
                 &dwThreadId);

    if (!hThread) {
        DPRINT1("SERVICES: Could not create thread (Status %lx)\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

int STDCALL
WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd)
{
  HANDLE hScmStartEvent;
  HANDLE hEvent;
  NTSTATUS Status;

  DPRINT("SERVICES: Service Control Manager\n");

  /* Create start event */
  if (!ScmCreateStartEvent(&hScmStartEvent))
    {
      DPRINT1("SERVICES: Failed to create start event\n");
      ExitThread(0);
    }

  DPRINT("SERVICES: created start event with handle %x.\n", hScmStartEvent);

  /* FIXME: more initialization */


  /* Create the service database */
  Status = ScmCreateServiceDataBase();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("SERVICES: failed to create SCM database (Status %lx)\n", Status);
      ExitThread(0);
    }

  /* Update service database */
  ScmGetBootAndSystemDriverState();

#if 0
    DPRINT("SERVICES: Attempting to create named pipe...\n");
    /* Create named pipe */
    if (!ScmCreateNamedPipe()) {
        DPRINT1("SERVICES: Failed to create named pipe\n");
        ExitThread(0);
    }
    DPRINT("SERVICES: named pipe created successfully.\n");
#else
    DPRINT("SERVICES: Attempting to create named pipe listener...\n");
    if (!StartScmNamedPipeThreadListener()) {
        DPRINT1("SERVICES: Failed to create named pipe listener thread.\n");
        ExitThread(0);
    }
    DPRINT("SERVICES: named pipe listener thread created.\n");
#endif
   /* FIXME: create listener thread for pipe */


  /* Register service process with CSRSS */
  RegisterServicesProcess(GetCurrentProcessId());

  DPRINT("SERVICES: Initialized.\n");

  /* Signal start event */
  SetEvent(hScmStartEvent);

  /* FIXME: register event handler (used for system shutdown) */
//  SetConsoleCtrlHandler(...);


  /* Start auto-start services */
  ScmAutoStartServices();

  /* FIXME: more to do ? */


  DPRINT("SERVICES: Running.\n");

#if 1
  hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  WaitForSingleObject(hEvent, INFINITE);
#else
    for (;;)
      {
    NtYieldExecution();
      }
#endif

  DPRINT("SERVICES: Finished.\n");

  ExitThread(0);
  return(0);
}

/* EOF */
