/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2help/handle.c
 * PURPOSE:     WinSock 2 DLL header
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

#include <ndk/iofuncs.h>

#include "wshdrv.h"

/* DATA **********************************************************************/

typedef struct _WSH_HELPER_CONTEXT
{
    HANDLE FileHandle;
    HANDLE ThreadHandle;
    HANDLE DllHandle;
} WSH_HELPER_CONTEXT, *PWAH_HELPER_CONTEXT;

LPFN_WSASEND pWSASend;
LPFN_WSARECV pWSARecv;
LPFN_WSASENDTO pWSASendTo;
LPFN_WSARECVFROM pWSARecvFrom;
LPFN_WSAGETLASTERROR pWSAGetLastError;
LPFN_WSACANCELBLOCKINGCALL pWSACancelBlockingCall;
LPFN_WSASETBLOCKINGHOOK pWSASetBlockingHook;
LPFN_SELECT pSelect;
LPFN_WSASTARTUP pWSAStartup;
LPFN_WSACLEANUP pWSACleanup;
LPFN_GETSOCKOPT pGetSockOpt;
LPFN_WSAIOCTL pWSAIoctl;

#define APCH (HANDLE)'SOR '

/* FUNCTIONS *****************************************************************/

VOID
CALLBACK
ApcThread(ULONG_PTR Context)
{

}

NTSTATUS
WINAPI
DoSocketCancel(PVOID Context1,
               PVOID Context2,
               PVOID Context3)
{
    return STATUS_SUCCESS;
}

NTSTATUS
WINAPI
DoSocketRequest(PVOID Context1,
                PVOID Context2,
                PVOID Context3)
{
    return STATUS_SUCCESS;
}

VOID
CALLBACK
ExitThreadApc(ULONG_PTR Context)
{
    PWAH_HELPER_CONTEXT HelperContext = (PWAH_HELPER_CONTEXT)Context;
    HMODULE DllHandle = HelperContext->DllHandle;

    /* Close the file handle */
    CloseHandle(HelperContext->FileHandle);

    /* Free the context */
    HeapFree(GlobalHeap, 0, HelperContext);

    /* Exit the thread and library */
    FreeLibraryAndExitThread(DllHandle, ERROR_SUCCESS);
}

DWORD
WINAPI
WahCloseHandleHelper(IN HANDLE HelperHandle)
{
    DWORD ErrorCode;
    PWAH_HELPER_CONTEXT Context = HelperHandle;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate the handle */
    if (!Context) return ERROR_INVALID_PARAMETER;

    /* Queue an APC to exit the thread */
    if (QueueUserAPC(ExitThreadApc, Context->ThreadHandle, (ULONG_PTR)Context))
    {
        /* Done */
        return ERROR_SUCCESS;
    }
    else
    {
        /* We failed somewhere */
        return ERROR_GEN_FAILURE;
    }
}

DWORD
WINAPI
WahCloseSocketHandle(IN HANDLE HelperHandle,
                     IN SOCKET Socket)
{
    DWORD ErrorCode;
    PWAH_HELPER_CONTEXT Context = HelperHandle;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate the handle */
    if (!(Context) || (Socket == INVALID_SOCKET) || !(Socket))
    {
        /* Invalid handle and/or socket */
        return ERROR_INVALID_PARAMETER;
    }

    /* Just close the handle and return */
    CloseHandle((HANDLE)Socket);
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WahCreateSocketHandle(IN HANDLE HelperHandle,
                      OUT SOCKET *Socket)
{
    DWORD ErrorCode;
    INT OpenType;
    DWORD Size = sizeof(OpenType);
    DWORD CreateOptions = 0;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION Ea;
    PWAH_EA_DATA EaData;
    CHAR EaBuffer[sizeof(*Ea) + sizeof(*EaData)];
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
	PWAH_HELPER_CONTEXT	Context = (PWAH_HELPER_CONTEXT)HelperHandle;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate the handle and pointer */
    if (!(Context) || !(Socket))
    {
        /* Invalid handle and/or socket */
        return ERROR_INVALID_PARAMETER;
    }

    /* Set pointer to EA */
    Ea = (PFILE_FULL_EA_INFORMATION)EaBuffer;

    /* Get the open type to determine the create options */
    if ((pGetSockOpt(INVALID_SOCKET,
                     SOL_SOCKET,
                     SO_OPENTYPE,
                     (PCHAR)&OpenType,
                     (INT FAR*)&Size) == ERROR_SUCCESS) && (OpenType))
    {
        /* This is a sync open */
        CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
    }

    /* Initialize the attributes for the driver */
    RtlInitUnicodeString(&Name, L"\\Device\\WS2IFSL\\NifsSct");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    /* Set up the EA */
    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;
    Ea->EaNameLength = sizeof("NifsSct");
    Ea->EaValueLength = sizeof(*EaData);
    RtlCopyMemory(Ea->EaName, "NifsSct", Ea->EaNameLength);

    /* Get our EA data */
    EaData = (PWAH_EA_DATA)(Ea + 1);

    /* Write the EA Data */
    EaData->FileHandle = Context->FileHandle;
    EaData->Context = NULL;

    /* Call the driver */
    Status = NtCreateFile((PHANDLE)Socket,
                           FILE_ALL_ACCESS,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           0,
                           FILE_OPEN_IF,
                           CreateOptions,
                           Ea,
                           sizeof(*Ea) + sizeof(*EaData));

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Write the socket handle */
        EaData->Context =(HANDLE)*Socket;

        /* Tell the driver about it */
        if (DeviceIoControl((HANDLE)*Socket,
                            IOCTL_WS2IFSL_SET_HANDLE,
                            &EaData,
                            sizeof(WSH_EA_DATA),
                            NULL,
                            0,
                            &Size,
                            NULL))
        {
            /* Set success */
            ErrorCode = NO_ERROR;
        }
        else
        {
            /* We failed. Get the error and close the socket */
            ErrorCode = GetLastError();
            CloseHandle((HANDLE)*Socket);
            *Socket = 0;
        }
    }
    else
    {
        /* Create file failed, conver error code */
        ErrorCode = RtlNtStatusToDosError(Status);
    }

    /* Return to caller */
    return ErrorCode;
}

INT
WINAPI
WahDisableNonIFSHandleSupport(VOID)
{
    DWORD ErrorCode;
    SC_HANDLE ServiceMgrHandle, Ws2IfsHandle;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Open the service DB */
    ServiceMgrHandle = OpenSCManager(NULL,
                                     SERVICES_ACTIVE_DATABASE,
                                     SC_MANAGER_CREATE_SERVICE);
    if (!ServiceMgrHandle) return GetLastError();

    /* Open the service */
    Ws2IfsHandle = OpenService(ServiceMgrHandle, "WS2IFSL", SERVICE_ALL_ACCESS);

    /* Disable the service */
    ChangeServiceConfig(Ws2IfsHandle,
                        SERVICE_NO_CHANGE,
                        SERVICE_DISABLED,
                        SERVICE_NO_CHANGE,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

    /* Close the handles and return */
    CloseServiceHandle(ServiceMgrHandle);
    CloseServiceHandle(Ws2IfsHandle);
    return ERROR_SUCCESS;
}

INT
WINAPI
WahEnableNonIFSHandleSupport(VOID)
{
    return 0;
}

DWORD
WINAPI
WahOpenHandleHelper(OUT PHANDLE HelperHandle)
{
    DWORD ErrorCode;
    HINSTANCE hWs2_32;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION Ea;
    PWAH_EA_DATA2 EaData;
    CHAR EaBuffer[sizeof(*Ea) + sizeof(*EaData)];
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    DWORD Tid;
    PWAH_HELPER_CONTEXT Context;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate the pointer */
    if (!HelperHandle)
    {
        /* Invalid handle and/or socket */
        return ERROR_INVALID_PARAMETER;
    }

    /* Get ws2_32.dll's handle. We don't load it: it MUST be here */
    hWs2_32 = GetModuleHandle ("WS2_32.DLL");
    if (!hWs2_32) return WSASYSCALLFAILURE;

    /* Dynamically load all its APIs */
    if (!((pGetSockOpt = (LPFN_GETSOCKOPT)GetProcAddress(hWs2_32, "getsockopt"))) ||
        !((pSelect = (LPFN_SELECT)GetProcAddress(hWs2_32, "select"))) ||
        !((pWSACancelBlockingCall = (LPFN_WSACANCELBLOCKINGCALL)GetProcAddress(hWs2_32, "WSACancelBlockingCall"))) ||
        !((pWSACleanup = (LPFN_WSACLEANUP)GetProcAddress(hWs2_32, "WSACleanup"))) ||
        !((pWSAGetLastError = (LPFN_WSAGETLASTERROR)GetProcAddress(hWs2_32, "WSAGetLastError"))) ||
        !((pWSASetBlockingHook = (LPFN_WSASETBLOCKINGHOOK)GetProcAddress(hWs2_32, "WSASetBlockingHook"))) ||
        !((pWSARecv = (LPFN_WSARECV)GetProcAddress(hWs2_32, "WSARecv"))) ||
        !((pWSASend = (LPFN_WSASEND)GetProcAddress(hWs2_32, "WSASend"))) ||
        !((pWSASendTo = (LPFN_WSASENDTO)GetProcAddress(hWs2_32, "WSASendTo"))) ||
        !((pWSAStartup = (LPFN_WSASTARTUP)GetProcAddress(hWs2_32, "WSAStartup"))) ||
        !((pWSARecvFrom = (LPFN_WSARECVFROM)GetProcAddress(hWs2_32, "WSARecvFrom"))) ||
        !((pWSAIoctl = (LPFN_WSAIOCTL)GetProcAddress(hWs2_32, "WSAIoctl"))))
    {
        /* Uh, guess we failed somewhere */
        return WSASYSCALLFAILURE;
    }

    /* Set pointer EA structure */
    Ea = (PFILE_FULL_EA_INFORMATION)EaBuffer;

    /* Create the helper context */
    Context = HeapAlloc(GlobalHeap, 0, sizeof(WSH_HELPER_CONTEXT));
    if (Context)
    {
        /* Create the special request thread */
		Context->ThreadHandle = CreateThread(NULL,
                                             0,
                                             (PVOID)ApcThread,
                                             Context,
                                             CREATE_SUSPENDED,
                                             &Tid);
		if (Context->ThreadHandle)
        {
            /* Create the attributes for the driver open */
			RtlInitUnicodeString(&Name,  L"\\Device\\WS2IFSL\\NifsPvd");
			InitializeObjectAttributes(&ObjectAttributes,
								       &Name,
                                       0,
                                       NULL,
                                       NULL);

            /* Setup the EA */
			Ea->NextEntryOffset = 0;
			Ea->Flags = 0;
			Ea->EaNameLength = sizeof("NifsPvd");
			Ea->EaValueLength = sizeof(*EaData);
			RtlCopyMemory(Ea->EaName, "NifsPvd", Ea->EaNameLength);

            /* Get our EA data */
            EaData = (PWAH_EA_DATA2)(Ea + 1);

            /* Fill out the EA Data */
			EaData->ThreadHandle = Context->ThreadHandle;
			EaData->RequestRoutine = DoSocketRequest;
			EaData->CancelRoutine = DoSocketCancel;
			EaData->ApcContext = Context;
			EaData->Reserved = 0;

            /* Call the driver */
			Status = NtCreateFile(&Context->FileHandle,
								  FILE_ALL_ACCESS,
								  &ObjectAttributes,
								  &IoStatusBlock,
								  NULL,
								  FILE_ATTRIBUTE_NORMAL,
								  0,
								  FILE_OPEN_IF,
								  0,
								  Ea,
								  sizeof(*Ea) + sizeof(*EaData));

            /* Check for success */
            if (NT_SUCCESS(Status))
            {
                /* Resume the thread and return a handle to the context */
                ResumeThread(Context->ThreadHandle);
				*HelperHandle = (HANDLE)Context;
				return ERROR_SUCCESS;
			}
		    else
            {
                /* Get the error code */
			    ErrorCode = RtlNtStatusToDosError(Status);
            }

            /* We failed, mark us as such */
            Context->FileHandle = NULL;
            ResumeThread(Context->ThreadHandle);
        }
		else
        {
            /* Get the error code */
			ErrorCode = GetLastError();
		}

        /* If we got here, we failed, so free the context */
		HeapFree(GlobalHeap, 0, Context);
    }
    else
    {
        /* Get the error code */
        ErrorCode = GetLastError();
    }

    /* Return to caller */
    return ErrorCode;
}

DWORD
WINAPI
WahCompleteRequest(IN HANDLE HelperHandle,
                   IN SOCKET Socket,
                   IN LPWSAOVERLAPPED lpOverlapped,
                   IN DWORD ErrorCode,
                   IN DWORD BytesTransferred)
{
    UNREFERENCED_PARAMETER(HelperHandle);
    UNREFERENCED_PARAMETER(Socket);
    UNREFERENCED_PARAMETER(lpOverlapped);
    UNREFERENCED_PARAMETER(ErrorCode);
    UNREFERENCED_PARAMETER(BytesTransferred);
    return ERROR_SUCCESS;
}

/* EOF */
