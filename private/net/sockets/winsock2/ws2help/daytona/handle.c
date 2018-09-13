/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module implements the socket handle helper functions for the WinSock 2.0
    helper library.

Author:
    Vadim Eydelman (VadimE)


Revision History:

--*/


#include "precomp.h"
#include "resource.h"
#include "osdef.h"
#include "mswsock.h"

//
//  Private constants.
//

#define FAKE_HELPER_HANDLE                  ((HANDLE)'MKC ')
#define WS2IFSL_SERVICE_NAME                TEXT ("WS2IFSL")

// Extended overlapped structure
typedef struct _OVERLAPPED_CTX {
    OVERLAPPED      ovlp;
#define SocketFile ovlp.hEvent
    HANDLE          ProcessFile;
	ULONG			UniqueId;
    ULONG           BufferLen;
	INT				FromLen;
    union {
	    CHAR			Buffer[1];
        SOCKET          Handle;
    };
} OVERLAPPED_CTX, *POVERLAPPED_CTX;

typedef struct _HANDLE_HELPER_CTX {
	HANDLE				ProcessFile;
	HANDLE				ThreadHdl;
	HANDLE				LibraryHdl;
} HANDLE_HELPER_CTX, *PHANDLE_HELPER_CTX;


/* Private Prototypes */ 
VOID
DoSocketRequest (
    PVOID   Context1,
    PVOID   Context2,
    PVOID   Context3
    );

NTSTATUS
DoSocketCancel (
    PVOID   Context1,
    PVOID   Context2,
    PVOID   Context3
    );

DWORD WINAPI
ApcThread (
    PVOID   param
    );

VOID CALLBACK
ExitThreadApc (
    ULONG_PTR   param
    );

void CALLBACK
WinsockApc (
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN DWORD dwFlags
    );

NTSTATUS
WsErrorToNtStatus (
    IN DWORD Error
    );

/* Private Globals */
BOOL                    Ws2helpInitialized = FALSE;
CRITICAL_SECTION        StartupSyncronization;

/* Our module handle: we keep a reference to it to make sure that
    in is not unloaded while our thread is executing */
HINSTANCE   LibraryHdl;

/* Winsock2 entry points that we call
*/
LPFN_WSASEND                pWSASend=NULL;
LPFN_WSARECV                pWSARecv=NULL;
LPFN_WSASENDTO              pWSASendTo=NULL;
LPFN_WSARECVFROM            pWSARecvFrom=NULL;
LPFN_WSAGETLASTERROR        pWSAGetLastError=NULL;
LPFN_WSACANCELBLOCKINGCALL  pWSACancelBlockingCall = NULL;
LPFN_WSASETBLOCKINGHOOK     pWSASetBlockingHook = NULL;
LPFN_SELECT                 pSelect = NULL;
LPFN_WSASTARTUP             pWSAStartup = NULL;
LPFN_WSACLEANUP             pWSACleanup = NULL;
LPFN_GETSOCKOPT             pGetSockOpt = NULL;
LPFN_WSAIOCTL               pWSAIoctl = NULL;

#if DBG
DWORD       PID=0;
ULONG       DbgLevel = DBG_FAILURES;
#endif


/* Public Functions */


BOOL WINAPI DllMain(
    IN HINSTANCE hinstDll,
    IN DWORD fdwReason,
    LPVOID lpvReserved
    )
{


    switch (fdwReason) {

    case DLL_PROCESS_ATTACH:
        LibraryHdl = hinstDll;
        DisableThreadLibraryCalls (hinstDll);
        __try {
            InitializeCriticalSection (&StartupSyncronization);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            WshPrint(DBG_FAILURES, ("ws2help-DllMain: Failed to initialize"
                                    " startup critical section, excpt: %lx\n",
                                    GetExceptionCode ()));
            LibraryHdl = NULL;
            return FALSE;
        }
        break;


    case DLL_PROCESS_DETACH:

        if (LibraryHdl==NULL)
            break;

        // The calling process is detaching
        // the DLL from its address space.
        //
        // Note that lpvReserved will be NULL if the detach is due to
        // a FreeLibrary() call, and non-NULL if the detach is due to
        // process cleanup.
        //

        if (lpvReserved==NULL) {
            //
            // Free security descriptor if it was allocated
            //
            if (pSDPipe!=NULL)
                FREE_MEM (pSDPipe);
            if (ghWriterEvent!=NULL) {
                CloseHandle (ghWriterEvent);
            }
            DeleteCriticalSection (&StartupSyncronization);
            Ws2helpInitialized = FALSE;
        }
        break;
    }

    return(TRUE);
}


DWORD
WINAPI
WahOpenHandleHelper(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    This routine opens WinSock 2.0 handle helper

Arguments:

    HelperHandle - Points to buffer ion which to return handle.


Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
	PFILE_FULL_EA_INFORMATION	fileEa = alloca(WS2IFSL_PROCESS_EA_INFO_LENGTH);
    OBJECT_ATTRIBUTES   fileAttr;
    UNICODE_STRING      fileName;
    NTSTATUS            status;
    IO_STATUS_BLOCK     ioStatus;
    DWORD               apcThreadId;
    DWORD               rc;
    HINSTANCE           hWS2_32;
	PHANDLE_HELPER_CTX	hCtx;

    rc = ENTER_WS2HELP_API();
    if (rc!=0)
        return rc;

    if (HelperHandle==NULL)
        return ERROR_INVALID_PARAMETER;

    hWS2_32 = GetModuleHandle (TEXT ("WS2_32.DLL"));
    if (hWS2_32==NULL)
        return WSASYSCALLFAILURE;



    if (((pGetSockOpt=(LPFN_GETSOCKOPT)GetProcAddress (hWS2_32, "getsockopt"))==NULL)
            || ((pSelect=(LPFN_SELECT)GetProcAddress (hWS2_32, "select"))==NULL)
            || ((pWSACancelBlockingCall=(LPFN_WSACANCELBLOCKINGCALL)GetProcAddress (hWS2_32, "WSACancelBlockingCall"))==NULL)
            || ((pWSACleanup=(LPFN_WSACLEANUP)GetProcAddress (hWS2_32, "WSACleanup"))==NULL)
            || ((pWSAGetLastError=(LPFN_WSAGETLASTERROR)GetProcAddress (hWS2_32, "WSAGetLastError"))==NULL)
            || ((pWSASetBlockingHook=(LPFN_WSASETBLOCKINGHOOK)GetProcAddress (hWS2_32, "WSASetBlockingHook"))==NULL)
            || ((pWSARecv=(LPFN_WSARECV)GetProcAddress (hWS2_32, "WSARecv"))==NULL)
            || ((pWSASend=(LPFN_WSASEND)GetProcAddress (hWS2_32, "WSASend"))==NULL)
            || ((pWSASendTo=(LPFN_WSASENDTO)GetProcAddress (hWS2_32, "WSASendTo"))==NULL)
            || ((pWSAStartup=(LPFN_WSASTARTUP)GetProcAddress (hWS2_32, "WSAStartup"))==NULL)
            || ((pWSARecvFrom=(LPFN_WSARECVFROM)GetProcAddress (hWS2_32, "WSARecvFrom"))==NULL)
            || ((pWSAIoctl=(LPFN_WSAIOCTL)GetProcAddress(hWS2_32, "WSAIoctl"))==NULL) )
        return WSASYSCALLFAILURE;

        // Create file used to communicate with the driver
    hCtx = (PHANDLE_HELPER_CTX) ALLOC_MEM (sizeof (*hCtx));
    if (hCtx!=NULL) {

        /* Create thread in which to execute file system requests */
		hCtx->ThreadHdl = CreateThread (NULL,
                            0,
                            ApcThread,
                            hCtx,
                            CREATE_SUSPENDED,
                            &apcThreadId);
		if (hCtx->ThreadHdl!=NULL) {

			RtlInitUnicodeString (&fileName, WS2IFSL_PROCESS_FILE_NAME);
			InitializeObjectAttributes (&fileAttr,
								&fileName,
								0,                  // Attributes
								NULL,               // Root directory
								NULL);              // Security descriptor
			fileEa->NextEntryOffset = 0;
			fileEa->Flags = 0;
			fileEa->EaNameLength = WS2IFSL_PROCESS_EA_NAME_LENGTH;
			fileEa->EaValueLength = WS2IFSL_PROCESS_EA_VALUE_LENGTH;
			strcpy (fileEa->EaName, WS2IFSL_PROCESS_EA_NAME);
			GET_WS2IFSL_PROCESS_EA_VALUE (fileEa)->ApcThread = hCtx->ThreadHdl;
			GET_WS2IFSL_PROCESS_EA_VALUE (fileEa)->RequestRoutine = DoSocketRequest;
			GET_WS2IFSL_PROCESS_EA_VALUE (fileEa)->CancelRoutine = DoSocketCancel;
			GET_WS2IFSL_PROCESS_EA_VALUE (fileEa)->ApcContext = hCtx;
#if DBG
		    GET_WS2IFSL_PROCESS_EA_VALUE (fileEa)->DbgLevel = DbgLevel;
#else
			GET_WS2IFSL_PROCESS_EA_VALUE (fileEa)->DbgLevel = 0;
#endif


			status = NtCreateFile (&hCtx->ProcessFile,
								 FILE_ALL_ACCESS,
								 &fileAttr,
								 &ioStatus,
								 NULL,              // Allocation size
								 FILE_ATTRIBUTE_NORMAL,
								 0,                 // ShareAccess
								 FILE_OPEN_IF,      // Create disposition
								 0,                 // Create options
								 fileEa,
								 WS2IFSL_PROCESS_EA_INFO_LENGTH);
			if (NT_SUCCESS (status)) {
                ResumeThread (hCtx->ThreadHdl);
				*HelperHandle = (HANDLE)hCtx;
				WshPrint (DBG_PROCESS,
					("WS2HELP-%lx WahOpenHandleHelper: Opened handle %p\n",
							PID, hCtx));
				return NO_ERROR;

			}
		    else { 
			    WshPrint (DBG_PROCESS|DBG_FAILURES,
				    ("WS2HELP-%lx WahOpenHandleHelper: Could not create process file, status %lx\n",
				    PID, status));
			    rc = RtlNtStatusToDosError (status);
		    
            }
            hCtx->ProcessFile = NULL;
            ResumeThread (hCtx->ThreadHdl);
        }
		else {// if (ApcThreadHdl!=NULL)
			rc = GetLastError ();
			WshPrint (DBG_PROCESS|DBG_FAILURES,
				("WS2HELP-%lx WahOpenHandleHelper: Could not create APC thread, rc=%ld\n",
						PID, rc));
		}
		FREE_MEM (hCtx);
    }
    else {
        WshPrint (DBG_PROCESS|DBG_FAILURES,
            ("WS2HELP-%lx WahOpenHandleHelper: Could allocate helper context\n", PID));
        rc = GetLastError ();
    }


    return rc;
}


DWORD
WINAPI
WahCloseHandleHelper(
    IN HANDLE HelperHandle
    )
/*++

Routine Description:

    This function closes the WinSock 2.0 handle helper.

Arguments:

    HelperHandle - The handle to close.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{
	PHANDLE_HELPER_CTX	hCtx;
    DWORD               rc;

    rc = ENTER_WS2HELP_API();
    if (rc!=0)
        return rc;

    if (HelperHandle==NULL)
        return ERROR_INVALID_PARAMETER;

	hCtx = (PHANDLE_HELPER_CTX)HelperHandle;

        /* Queue APC that exits the thread */
    if (QueueUserAPC (ExitThreadApc, hCtx->ThreadHdl, (ULONG_PTR)hCtx)) {
		WshPrint (DBG_PROCESS, 
			("WS2HELP-%lx WahCloseHandleHelper: Queued close APC.\n", PID));
		return NO_ERROR;
	}
	else {
		WshPrint (DBG_PROCESS|DBG_FAILURES,
			("WS2HELP-%lx WahCloseHandleHelper: Failed to queue close APC.\n", PID));
		return ERROR_GEN_FAILURE;
	}
}


DWORD
WINAPI
WahCreateSocketHandle(
    IN HANDLE           HelperHandle,
    OUT SOCKET          *s
    )
/*++

Routine Description:

    This function creates IFS socket handle for service provider that
    cannot do it by itself.

Arguments:

    HelperHandle - The handle of WinSock 2.0 handle helper.
    S            - buffer to return created socket handle

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
	PFILE_FULL_EA_INFORMATION	fileEa = alloca(WS2IFSL_SOCKET_EA_INFO_LENGTH);
    OBJECT_ATTRIBUTES           fileAttr;
    UNICODE_STRING              fileName;
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;
    DWORD                       error;
    DWORD                       count;
    INT                         openType;
    DWORD                       crOptions;
	PHANDLE_HELPER_CTX			hCtx = (PHANDLE_HELPER_CTX)HelperHandle;
    
    error = ENTER_WS2HELP_API();
    if (error!=0)
        return error;

    if ((HelperHandle==NULL) || (s==NULL))
        return ERROR_INVALID_PARAMETER;

    count = sizeof (openType);
    if ((pGetSockOpt (INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (PCHAR)&openType, &count)==0)
        && (openType!=0)) {
        crOptions = FILE_SYNCHRONOUS_IO_NONALERT;
    }
    else
        crOptions = 0;

    // Create file handle on the driver device
    RtlInitUnicodeString (&fileName, WS2IFSL_SOCKET_FILE_NAME);
    InitializeObjectAttributes (&fileAttr,
                        &fileName,
                        0,                  // Attributes
                        NULL,               // Root directory
                        NULL);              // Security descriptor
    fileEa->NextEntryOffset = 0;
    fileEa->Flags = 0;
    fileEa->EaNameLength = WS2IFSL_SOCKET_EA_NAME_LENGTH;
    fileEa->EaValueLength = WS2IFSL_SOCKET_EA_VALUE_LENGTH;
    strcpy (fileEa->EaName, WS2IFSL_SOCKET_EA_NAME);
        // Supply the context (can't actually supply the handle
        // until it is opened
    GET_WS2IFSL_SOCKET_EA_VALUE (fileEa)->ProcessFile = hCtx->ProcessFile;
    GET_WS2IFSL_SOCKET_EA_VALUE (fileEa)->DllContext = NULL;

    status = NtCreateFile ((HANDLE *)s,
                         FILE_ALL_ACCESS,
                         &fileAttr,
                         &ioStatus,
                         NULL,              // Allocation size
                         FILE_ATTRIBUTE_NORMAL,
                         0,                 // ShareAccess
                         FILE_OPEN_IF,      // Create disposition
                         crOptions,         // Create options
                         fileEa,
                         WS2IFSL_SOCKET_EA_INFO_LENGTH);
    if (NT_SUCCESS (status)) {
            // Now set the actual context
        GET_WS2IFSL_SOCKET_EA_VALUE (fileEa)->DllContext = (HANDLE)*s;
        if (DeviceIoControl (
                        (HANDLE)*s,                         // File Handle
                        IOCTL_WS2IFSL_SET_SOCKET_CONTEXT,   // Control Code
                        GET_WS2IFSL_SOCKET_EA_VALUE (fileEa),// InBuffer
                        sizeof (WS2IFSL_SOCKET_CTX),         // InBufferLength
                        NULL,                               // OutBuffer
                        0,                                  // OutBufferLength
                        &count,                             // BytesReturned
                        NULL)) {                              // Overlapped
            WshPrint (DBG_SOCKET,
                ("WS2HELP-%lx WahCreateSocketHandle: Handle %p\n", PID,  *s));
            error = NO_ERROR;
        }
        else {
            error = GetLastError ();
            NtClose ((HANDLE)*s);
            WshPrint (DBG_SOCKET|DBG_FAILURES,
                ("WS2HELP-%lx WahCreateSocketHandle: Could not set context, rc=%ld\n",
                            PID, error));
            *s = 0;
        }
    }
    else { // if (NtCreateFile succeded)
        error = RtlNtStatusToDosError (status);
        WshPrint (DBG_SOCKET|DBG_FAILURES,
                ("WS2HELP-%lx WahCreateSocketHandle: Could create file, rc=%ld\n",
                        PID, error));
    }

    return error;
}



DWORD
WINAPI
WahCloseSocketHandle(
    IN HANDLE           HelperHandle,
    IN SOCKET           s
    )
/*++

Routine Description:

    This function destroyes IFS socket handle created by WahCreateSocketHandle

Arguments:

    HelperHandle - The handle of WinSock 2.0 handle helper.
    s            - socket handle to close

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
	PHANDLE_HELPER_CTX		hCtx = (PHANDLE_HELPER_CTX)HelperHandle;
	NTSTATUS				status;
    DWORD                   rc;

    rc = ENTER_WS2HELP_API();
    if (rc!=0)
        return rc;

    if ((HelperHandle==NULL)
            || (s==0)
            || (s==INVALID_SOCKET))
        return ERROR_INVALID_PARAMETER;

    WshPrint (DBG_SOCKET,
            ("WS2HELP-%lx WahCloseSocketHandle: Handle %p\n", PID, s));
    status = NtClose ((HANDLE)s);
	if (NT_SUCCESS (status))
		return NO_ERROR;
	else
		return RtlNtStatusToDosError (status);
}

DWORD
WINAPI
WahCompleteRequest(
    IN HANDLE              HelperHandle,
    IN SOCKET              s,
    IN LPWSAOVERLAPPED     lpOverlapped,
    IN DWORD               dwError,
    IN DWORD               cbTransferred
    )
/*++

Routine Description:

    This function simmulates completion of overlapped IO request
    on socket handle created by WasCreateSocketHandle

Arguments:

    HelperHandle - The handle of WinSock 2.0 handle helper.
    s            - socket handle to complete request on
    lpOverlapped - pointer to overlapped structure
    dwError      - WinSock 2.0 error code for opreation being completed
    cbTransferred- number of bytes transferred to/from user buffers as the
                    result of the operation being completed

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    IO_STATUS_BLOCK			IoStatus;
    NTSTATUS				status;
	PHANDLE_HELPER_CTX		hCtx = (PHANDLE_HELPER_CTX)HelperHandle;
    DWORD                   rc;
    
    rc = ENTER_WS2HELP_API();
    if (rc!=0)
        return rc;

    if ((HelperHandle==NULL)
		    || (lpOverlapped==NULL)
            || (s==INVALID_SOCKET)
            || (s==0))
        return ERROR_INVALID_PARAMETER;

        // Setup IO_STATUS block to be used by the driver to complete the
        // operation
    IoStatus.Status = WsErrorToNtStatus (dwError);
    IoStatus.Information = cbTransferred;
        // Call the driver to complete
    status = NtDeviceIoControlFile ((HANDLE)s,
                    lpOverlapped->hEvent,
                    NULL,
                    ((ULONG_PTR)lpOverlapped->hEvent&1) ? NULL : lpOverlapped,
                    (PIO_STATUS_BLOCK)lpOverlapped,
                    IOCTL_WS2IFSL_COMPLETE_PVD_REQ,
                    &IoStatus,
                    sizeof (IO_STATUS_BLOCK),
                    NULL,
                    0);
		// Be carefull not to touch overlapped after NtDeviceIoControlFile
    if (NT_SUCCESS(status) || (status==IoStatus.Status)) {
        WshPrint (DBG_COMPLETE,
            ("WS2HELP-%lx WahCompleteRequest: Handle %p, status %lx, info %ld\n",
                PID, s, IoStatus.Status, IoStatus.Information));
        return NO_ERROR;
    }
    else {
        WshPrint (DBG_COMPLETE|DBG_FAILURES,
            ("WS2HELP-%lx WahCompleteRequest: Failed on handle %p, status %lx\n",
                PID, s, status));
        return ERROR_INVALID_HANDLE;
    }
}


DWORD
WINAPI
WahEnableNonIFSHandleSupport (
    VOID
    )
/*++

Routine Description:

    This function installs and starts Winsock2 Installable File System Layer
    driver to provide socket handles for Non-IFS handle transport service 
    providers.
Arguments:

    None

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    SC_HANDLE   hSCManager, hWS2IFSL;
    DWORD       rc=0;
    TCHAR       WS2IFSL_DISPLAY_NAME[256];

    rc = ENTER_WS2HELP_API();
    if (rc!=0)
        return rc;

    //
    // Get display string for Winsock2 non-IFS handle helper service name
    // (localizable)
    //
    // Use exception handler because of delayload option we
    // use for user32.dll (for hydra compat).
    //
    __try {
        rc = LoadString (LibraryHdl, WS2IFSL_SERVICE_DISPLAY_NAME_STR,
                        WS2IFSL_DISPLAY_NAME, sizeof (WS2IFSL_DISPLAY_NAME));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = 0;
    }
    if (rc==0) {
        rc = GetLastError ();
        if (rc==0)
            rc = ERROR_NOT_ENOUGH_MEMORY;
        WshPrint (DBG_SERVICE|DBG_FAILURES,
            ("WS2HELP-%lx WahEnableNonIFSHandleSupport:"
             " Could not load service display string, err: %ld\n",
            PID, rc));
        return rc;
    }

    rc = 0;

    //
    // Open service database on the local computer
    //

    hSCManager = OpenSCManager (
                        NULL,
                        SERVICES_ACTIVE_DATABASE,
                        SC_MANAGER_CREATE_SERVICE
                        );
    if (hSCManager==NULL) {
        rc = GetLastError ();
        WshPrint (DBG_SERVICE|DBG_FAILURES,
            ("WS2HELP-%lx WahEnableNonIFSHandleSupport: Could not open SC, err: %ld\n",
            PID, rc));
        return rc;
    }




    //
    // Create Winsock2 non-IFS handle helper service
    //

    hWS2IFSL = CreateService (
                    hSCManager,
                    WS2IFSL_SERVICE_NAME,
                    WS2IFSL_DISPLAY_NAME,
                    SERVICE_ALL_ACCESS,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_AUTO_START,
                    SERVICE_ERROR_NORMAL,
                    TEXT ("\\SystemRoot\\System32\\drivers\\ws2ifsl.sys"),
                    TEXT ("PNP_TDI"),   // load group
                    NULL,               // Tag ID
                    NULL,               // Dependencies
                    NULL,               // Start name
                    NULL                // Password
                    );
    if (hWS2IFSL==NULL) {
        //
        // Failure, check if service already exists
        //
        rc = GetLastError ();
        if (rc!=ERROR_SERVICE_EXISTS) {
            //
            // Some other failure, bail out
            //
            WshPrint (DBG_SERVICE|DBG_FAILURES,
                ("WS2HELP-%lx WahEnableNonIFSHandleSupport: Could not create service, err: %ld\n",
                PID, rc));
            CloseServiceHandle (hSCManager);
            return rc;
        }

        rc = 0;

        //
        // Open existing service
        //
        hWS2IFSL = OpenService (
                    hSCManager,
                    WS2IFSL_SERVICE_NAME,
                    SERVICE_ALL_ACCESS);
        if (hWS2IFSL==NULL) {
            //
            // Could not open, bail out
            //
            rc = GetLastError ();
            WshPrint (DBG_SERVICE|DBG_FAILURES,
                ("WS2HELP-%lx WahEnableNonIFSHandleSupport: Could not open service, err: %ld\n",
                PID, rc));
            CloseServiceHandle (hSCManager);
            return rc;
        }

        if (!ChangeServiceConfig (hWS2IFSL,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_AUTO_START,
                    SERVICE_ERROR_NORMAL,
                    TEXT ("\\SystemRoot\\System32\\drivers\\ws2ifsl.sys"),
                                            // lpBinaryPathName 
                    TEXT ("PNP_TDI"),       // load group
                    NULL,                   // Tag ID
                    NULL,                   // Dependencies
                    NULL,                   // Start name
                    NULL,                   // Password
                    WS2IFSL_DISPLAY_NAME    // Display name
                    )) {
            //
            // Could set config, bail out
            //
            rc = GetLastError ();
            WshPrint (DBG_SERVICE|DBG_FAILURES,
                ("WS2HELP-%lx WahEnableNonIFSHandleSupport: Could not enable service, err: %ld\n",
                PID, rc));
            CloseServiceHandle (hSCManager);
            return rc;
        }

    }

    //
    // Go ahead, try start the service
    //
    if (!StartService (hWS2IFSL, 0, NULL)) {
        //
        // Check if it is already running
        //
        rc = GetLastError ();
        if (rc!=ERROR_SERVICE_ALREADY_RUNNING) {
            //
            // Could not start it, bail out
            //
            WshPrint (DBG_SERVICE|DBG_FAILURES,
                ("WS2HELP-%lx WahEnableNonIFSHandleSupport: Could start service, err: %ld\n",
                PID, rc));
            if (!DeleteService (hWS2IFSL)) {
                WshPrint (DBG_SERVICE|DBG_FAILURES,
                    ("WS2HELP-%lx WahEnableNonIFSHandleSupport: Could delete service, err: %ld\n",
                    PID, GetLastError ()));
            }
            CloseServiceHandle (hWS2IFSL);
            CloseServiceHandle (hSCManager);
            return rc;
        }
    }

    ASSERT ((rc==0) || (rc==ERROR_SERVICE_ALREADY_RUNNING));

    WshPrint (DBG_SERVICE,
        ("WS2HELP-%lx WahEnableNonIFSHandleSupport: Created and started service.n",
        PID));
    //
    // Success, cleanup open handles
    //
    CloseServiceHandle (hWS2IFSL);
    CloseServiceHandle (hSCManager);
    return rc;
}

DWORD
WINAPI
WahDisableNonIFSHandleSupport (
    VOID
    )
/*++

Routine Description:

    This function deinstalls Winsock2 Installable File System Layer
    driver to provide socket handles for Non-IFS handle transport service 
    providers.
Arguments:

    None

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    SC_HANDLE   hSCManager, hWS2IFSL;
    DWORD       rc=0;

    rc = ENTER_WS2HELP_API();
    if (rc!=0)
        return rc;

    //
    // Open service database on the local computer
    //

    hSCManager = OpenSCManager (
                        NULL,
                        SERVICES_ACTIVE_DATABASE,
                        SC_MANAGER_CREATE_SERVICE
                        );
    if (hSCManager==NULL) {
        rc = GetLastError ();
        WshPrint (DBG_SERVICE|DBG_FAILURES,
            ("WS2HELP-%lx WahDisableNonIFSHandleSupport: Could not open SC, err: %ld\n",
            PID, rc));
        return rc;
    }

    //
    // Open service itself
    //

    hWS2IFSL = OpenService (
                hSCManager,
                WS2IFSL_SERVICE_NAME,
                SERVICE_ALL_ACCESS);
    if (hWS2IFSL==NULL) {
        rc = GetLastError ();
        WshPrint (DBG_SERVICE|DBG_FAILURES,
            ("WS2HELP-%lx WahDisableNonIFSHandleSupport: Could open service, err: %ld\n",
            PID, rc));
        CloseServiceHandle (hSCManager);
        return rc;
    }

    //
    // Just disable the service, so it won't start on reboot.
    //
    // Deleting service is dangerous because this will require
    // a reboot before it can be installed again and we are
    // working towards no-reboot system
    //
    // Stopping is even worse, because it will go into uncontrollable
    // (STOP_PENDING) state until all handles to it are closed so
    // we won't be able to start it until reboot if some service is
    // holding its handles.
    // 

    if (ChangeServiceConfig (hWS2IFSL,
                SERVICE_NO_CHANGE,  // dwServiceType 
                SERVICE_DISABLED,   // dwStartType
                SERVICE_NO_CHANGE,  // dwErrorControl 
                NULL,               // lpBinaryPathName 
                NULL,               // load group
                NULL,               // Tag ID
                NULL,               // Dependencies
                NULL,               // Start name
                NULL,               // Password
                NULL                // Display name
                )) {
        rc = 0;
        WshPrint (DBG_SERVICE,
            ("WS2HELP-%lx WahDisableNonIFSHandleSupport: Disabled service.\n",
            PID));
    }
    else {
        rc = GetLastError ();
        WshPrint (DBG_SERVICE|DBG_FAILURES,
            ("WS2HELP-%lx WahDisableNonIFSHandleSupport: Could not disable service, err: %ld\n",
            PID, GetLastError ()));
    }

    CloseServiceHandle (hWS2IFSL);
    CloseServiceHandle (hSCManager);
    return rc;
}




DWORD WINAPI
ApcThread (
    PVOID   param
    )
/*++

Routine Description:

    This is a thread which is used by the driver to execute 
    IO system requests
Arguments:

    param   - handle helper context
Return Value:

    0

--*/
{
    NTSTATUS                    status;
	DWORD						rc;
	PHANDLE_HELPER_CTX			hCtx = (PHANDLE_HELPER_CTX)param;
	TCHAR						ModuleName[MAX_PATH];
    LARGE_INTEGER               Timeout;

    //
    // Could not open the file, just clean-up.
    //
    if (hCtx->ProcessFile==NULL)
        return -1;

    Timeout.QuadPart = 0x8000000000000000i64;

    // Increment our module reference count
    // so it does not go away while this thread is
    // running

	rc = GetModuleFileName (LibraryHdl,
							ModuleName,
							sizeof(ModuleName)/sizeof(ModuleName[0]));
	ASSERT (rc>0);

    hCtx->LibraryHdl = LoadLibrary (ModuleName);
    ASSERT (hCtx->LibraryHdl!=NULL);

    WshPrint (DBG_APC_THREAD,
        ("WS2HELP-%lx ApcThread: Initialization completed\n", PID));
            // Wait alertably to let APC's execute
    while (TRUE) {
        status = NtDelayExecution (TRUE, &Timeout);
        if (!NT_SUCCESS (status)) {
            //
            // Sleep for 3 seconds
            //
            LARGE_INTEGER   Timeout2;
            Timeout2.QuadPart = - (3i64*1000i64*1000i64*10i64);
            NtDelayExecution (FALSE, &Timeout2);
        }
    }
    // We should never get here, the thread terminates
    // from the ExitApc
    ASSERT (FALSE);
	return 0;
}

VOID CALLBACK
ExitThreadApc (
    ULONG_PTR   param
    )
/*++

Routine Description:

    This APC routine is used to terminate APC thread
Arguments:

    param   - exit code for thread
Return Value:

    Exit code for thread

--*/
{
	PHANDLE_HELPER_CTX	hCtx = (PHANDLE_HELPER_CTX)param;
    
        // Close the file
    NtClose (hCtx->ProcessFile);
	CloseHandle (hCtx->ThreadHdl);
    WshPrint (DBG_APC_THREAD, ("WS2HELP-%lx ExitThreadApc: Exiting, ctx: %p\n", PID));
    (*pWSACleanup) ();
	FREE_MEM (hCtx);
    FreeLibraryAndExitThread (hCtx->LibraryHdl, 0);
}


void CALLBACK
WinsockApc(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN DWORD dwFlags
    )
/*++

Routine Description:

    This APC routine is executed upon completion of WinSock 2.0 call
Arguments:

    dwError         - WinSock2.0 return code
    cbTransferred   - number of buffers transferred to/from user buffers
    lpOverlapped    - overlapped structure associated with the request
                      (it is actually our extended structure: OVERLAPPED_CTX_
    dwFlags         - flags associated with the request (ignored)
Return Value:

    None
--*/
{
	POVERLAPPED_CTX		ctx = CONTAINING_RECORD (lpOverlapped,
													OVERLAPPED_CTX,
													ovlp);
    IO_STATUS_BLOCK		ioStatus;
    DWORD				ignore;
	WS2IFSL_CMPL_PARAMS	params;
	NTSTATUS			status;

        // Setup status block for the driver
    params.SocketHdl = ctx->SocketFile;
	params.UniqueId = ctx->UniqueId;
    params.DataLen = cbTransferred;
	params.AddrLen = (ULONG)ctx->FromLen;
    params.Status = WsErrorToNtStatus (dwError);

	status = NtDeviceIoControlFile (
					ctx->ProcessFile,	// Handle
					NULL,				// Event
					NULL,				// Apc
					NULL,				// ApcContext
					&ioStatus,			// IoStatus
					IOCTL_WS2IFSL_COMPLETE_DRV_REQ, // IoctlCode
					&params,			// InputBuffer
					sizeof(params),		// InputBufferLength,
					ctx->Buffer,		// OutputBuffer
					ctx->BufferLen      // OutputBufferLength,
					);

    WshPrint (DBG_WINSOCK_APC,
        ("WS2HELP-%lx WinsockApc: Socket %p, id %ld, err %ld, cb %ld, addrlen %ld\n",
            PID, ctx->SocketFile, ctx->UniqueId,
			dwError, cbTransferred, ctx->FromLen));
    FREE_MEM (ctx);
}

VOID
DoSocketRequest (
    PVOID   PvCtx,
    PVOID   PvRequestId,
    PVOID   PvBufferLength
    )
/*++

Routine Description:

	Executes socket request for the ws2ifsl driver

Arguments:

Return Value:

    None
--*/
{
	IO_STATUS_BLOCK		ioStatus;
	NTSTATUS			status;
	WS2IFSL_RTRV_PARAMS	params;
	POVERLAPPED_CTX		ctx;
    PHANDLE_HELPER_CTX	hCtx = PvCtx;

    WshPrint (DBG_REQUEST,
		("WS2HELP-%lx DoSocketRequest: id %ld, buflen %ld\n",
        PID, PtrToUlong(PvRequestId), PtrToUlong(PvBufferLength)));

	params.UniqueId = PtrToUlong (PvRequestId);
	ctx = (POVERLAPPED_CTX)ALLOC_MEM (FIELD_OFFSET(
										OVERLAPPED_CTX,
										Buffer[PtrToUlong(PvBufferLength)]));

	if (ctx!=NULL) {
        ctx->ProcessFile = hCtx->ProcessFile;
			// Use extension field to save driver context
		ctx->UniqueId = PtrToUlong (PvRequestId);
        ctx->BufferLen = PtrToUlong (PvBufferLength);

		status = NtDeviceIoControlFile (
						ctx->ProcessFile,   // Handle
						NULL,				// Event
						NULL,				// Apc
						NULL,				// ApcContext
						&ioStatus,			// IoStatus
						IOCTL_WS2IFSL_RETRIEVE_DRV_REQ, // IoctlCode
						&params,			// InputBuffer
						sizeof(params),		// InputBufferLength,
						ctx->Buffer,		// OutputBuffer
						ctx->BufferLen	// OutputBufferLength,
						);
	}
	else {
		status = NtDeviceIoControlFile (
						ctx->ProcessFile,	// Handle
						NULL,				// Event
						NULL,				// Apc
						NULL,				// ApcContext
						&ioStatus,			// IoStatus
						IOCTL_WS2IFSL_RETRIEVE_DRV_REQ, // IoctlCode
						&params,			// InputBuffer
						sizeof(params),		// InputBufferLength,
						NULL,				// OutputBuffer
						0					// OutputBufferLength,
						);
		ASSERT (!NT_SUCCESS (status));
	}

	if (NT_SUCCESS(status)) {
		DWORD           error, count, flags;
		WSABUF          buf;

		ASSERT (ctx!=NULL);

		// Use hEvent to save socket context (handle)
		ctx->SocketFile = params.DllContext;
		ctx->FromLen = 0;
            
            // Setup request parameters and execute asynchronously
        switch (params.RequestType) {
        case WS2IFSL_REQUEST_READ:
            flags = 0;
            buf.buf = ctx->Buffer;
            buf.len = params.DataLen;
            if ((pWSARecv ((SOCKET)ctx->SocketFile,
                        &buf,
                        1,
                        &count,
                        &flags,
                        &ctx->ovlp,
                        WinsockApc)!=SOCKET_ERROR)
			           || ((error=pWSAGetLastError ())==WSA_IO_PENDING)) {
		        WshPrint (DBG_DRIVER_READ,
                    ("WS2HELP-%lx DoSocketRequest: Read - socket %p, ctx %p,"
					" id %ld, len %ld\n",
                    PID, ctx->SocketFile, ctx, 
					ctx->UniqueId,
					params.DataLen));
                return;
            }
            break;
        case WS2IFSL_REQUEST_WRITE:
			buf.buf = ctx->Buffer;
			buf.len = params.DataLen;
            if ((pWSASend ((SOCKET)ctx->SocketFile,
                        &buf,
                        1,
                        &count,
                        0,
                        &ctx->ovlp,
                        WinsockApc)!=SOCKET_ERROR)
                   || ((error=pWSAGetLastError ())==WSA_IO_PENDING)) {
	            WshPrint (DBG_DRIVER_WRITE,
					("WS2HELP-%lx DoSocketRequest: Write - socket %p, ctx %p,"
					" id %ld, len %ld\n",
                    PID, ctx->SocketFile, ctx, 
					ctx->UniqueId,
					params.DataLen));
                return;
            }
            break;
        case WS2IFSL_REQUEST_SENDTO:
			buf.buf = ctx->Buffer;
			buf.len = params.DataLen;
            if ((pWSASendTo ((SOCKET)ctx->SocketFile,
                        &buf,
                        1,
                        &count,
                        0,
						(const struct sockaddr FAR *)
							&ctx->Buffer[ADDR_ALIGN(params.DataLen)],
                        params.AddrLen,
                        &ctx->ovlp,
                        WinsockApc)!=SOCKET_ERROR)
                   || ((error=pWSAGetLastError ())==WSA_IO_PENDING)) {
                WshPrint (DBG_DRIVER_SEND,
					("WS2HELP-%lx DoSocketRequest: SendTo - socket %p, ctx %p,"
					" id %ld, len %ld, addrlen %ld\n",
                    PID, ctx->SocketFile, ctx, 
					ctx->UniqueId,
					params.DataLen, params.AddrLen));
                return;
            }
            break;
        case WS2IFSL_REQUEST_RECVFROM:
			buf.buf = ctx->Buffer;
			buf.len = params.DataLen;
			flags = params.Flags;
			ctx->FromLen = (INT)params.AddrLen;
            if ((pWSARecvFrom ((SOCKET)ctx->SocketFile,
                        &buf,
                        1,
                        &count,
                        &flags,
						(struct sockaddr FAR *)
							&ctx->Buffer[ADDR_ALIGN(params.DataLen)],
                        &ctx->FromLen,
                        &ctx->ovlp,
                        WinsockApc)!=SOCKET_ERROR)
                   || ((error=pWSAGetLastError ())==WSA_IO_PENDING)) {
                WshPrint (DBG_DRIVER_RECV,
					("WS2HELP-%lx DoSocketRequest: RecvFrom - socket %p, ctx %p,"
					" id %ld, len %ld, addrlen %ld, flags %lx\n",
                    PID, ctx->SocketFile, ctx, 
					ctx->UniqueId,
					params.DataLen, params.AddrLen, params.Flags));
                return;
            }
            break;
        case WS2IFSL_REQUEST_RECV:
			buf.buf = ctx->Buffer;
			buf.len = params.DataLen;
			flags = params.Flags;
            if ((pWSARecv ((SOCKET)ctx->SocketFile,
                        &buf,
                        1,
                        &count,
                        &flags,
                        &ctx->ovlp,
                        WinsockApc)!=SOCKET_ERROR)
                   || ((error=pWSAGetLastError ())==WSA_IO_PENDING)) {
                WshPrint (DBG_DRIVER_RECV,
					("WS2HELP-%lx DoSocketRequest: Recv - socket %p, ctx %p,"
					" id %ld, len %ld, flags %lx\n",
                    PID, ctx->SocketFile, ctx, 
					ctx->UniqueId,
					params.DataLen, params.Flags));
                return;
            }
            break;
        case WS2IFSL_REQUEST_QUERYHANDLE:
            ASSERT (params.DataLen==sizeof (SOCKET));
            if ((pWSAIoctl ((SOCKET)ctx->SocketFile,
                            SIO_QUERY_TARGET_PNP_HANDLE,
                            NULL,
                            0,
                            &ctx->Handle,
                            sizeof (ctx->Handle),
                            &count,
                            &ctx->ovlp,
                            WinsockApc)!=SOCKET_ERROR)
                   || ((error=pWSAGetLastError ())==WSA_IO_PENDING)) {
                WshPrint (DBG_CANCEL,
					("WS2HELP-%lx DoSocketRequest: PnP - socket %p, ctx %p,"
					" id %ld\n",
                    PID, ctx->SocketFile, ctx, 
					ctx->UniqueId));
                return;
            }
            break;
        default:
            ASSERT (FALSE);
        }
		// The Winsock request failed (no APC is going to executed, call it here)
        WinsockApc (error, 0, &ctx->ovlp, 0);
	}

}



INT_PTR
CancelHook (
    void
    )
/*++

Routine Description:

    This is blocking hook that cancels the current request
Arguments:

Return Value:
    FALSE   - to stop polling

--*/
{
    BOOL    res;
    res = pWSACancelBlockingCall ();
    WshPrint (DBG_CANCEL,
        ("WS2HELP-%lx CancelHook: %s\n", PID, res ? "succeded" : "failed"));
    return FALSE;
}


NTSTATUS
DoSocketCancel (
    PVOID   PvCtx,
    PVOID   PvRequestId,
    PVOID   PvDllContext
    )
/*++

Routine Description:
    Hack to attempt to cancel request in progress.
    This works with MSAFD, but may (actully will) not work with
    any other provider which is likely to implement select differently.
    Our hope here that by cancelling select we will also cancel any
    other outstanding requests on a socket handle.

Arguments:

Return Value:

    None
--*/
{
    FARPROC				oldHook;
    int					res;
    fd_set				set;
    struct timeval		timeout;
	IO_STATUS_BLOCK		ioStatus;
	NTSTATUS			status;
	WS2IFSL_CNCL_PARAMS	params;
	SOCKET				s = (SOCKET)PvDllContext;
    WORD                wVersionRequested;
    WSADATA             wsaData;
    DWORD               rc;
    PHANDLE_HELPER_CTX	hCtx = PvCtx;

    WshPrint (DBG_CANCEL, ("WS2HELP-%lx DoSocketCancel: Socket %p, id %d\n",
        PID, PvDllContext, PtrToUlong (PvRequestId)));
    //
    // Request 1.1 so that blocking hooks are supported
    //
    wVersionRequested = MAKEWORD(1, 1);
    if ((rc =(*pWSAStartup) (wVersionRequested, &wsaData))==NO_ERROR) {

        oldHook = pWSASetBlockingHook (CancelHook);
        if (oldHook!=NULL) {

            FD_ZERO (&set);
            FD_SET (s, &set);
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;
            res = pSelect (1, NULL, NULL, &set, &timeout);
            WshPrint (DBG_CANCEL, 
                ("WS2HELP-%lx CancelApc: Done on socket id %ld (res %ld)\n",
                PID, PtrToUlong(PvRequestId), res));
            pWSASetBlockingHook (oldHook);
        }
        else {
            rc = pWSAGetLastError ();
            WshPrint (DBG_CANCEL|DBG_FAILURES,
                ("WS2HELP-%lx DoSocketCancel: Could not install blocking hook, err - %ld\n",
                PID, rc));
        }
        (*pWSACleanup)();
    }
    else {
        WshPrint (DBG_CANCEL|DBG_FAILURES,
            ("WS2HELP-%lx DoSocketCancel: Could not get version 1.1, rc - %ld\n",
            PID, rc));
    }

	params.UniqueId = PtrToUlong (PvRequestId);

	status = NtDeviceIoControlFile (
					hCtx->ProcessFile,  // Handle
					NULL,				// Event
					NULL,				// Apc
					NULL,				// ApcContext
					&ioStatus,			// IoStatus
					IOCTL_WS2IFSL_COMPLETE_DRV_CAN, // IoctlCode
					&params,			// InputBuffer
					sizeof(params),		// InputBufferLength,
					NULL,				// OutputBuffer
					0					// OutputBufferLength,
					);
	ASSERT (NT_SUCCESS (status));


    WshPrint (DBG_CANCEL, 
        ("WS2HELP-%lx CancelApc: Completed on socket %p, id %ld (status %lx)\n",
        PID, s, params.UniqueId, status));

	return status;
}






NTSTATUS
WsErrorToNtStatus (
    DWORD   dwError
    )
/*++

Routine Description:

    This function maps WinSock 2.0 error code to NTSTATUS value
Arguments:

    dwError         - WinSock2.0 return code
    
Return Value:

    NTSTATUS corresponding to dwError
--*/
{
    // Macro that validates that our winsock error array indeces are
    // in sync with winsock2.h defines
#define MAPWSERROR(line,Error,Status)   Status
    // WinSock2.0 error to NTSTATUS MAP
static const NTSTATUS WSAEMap[]= {
    MAPWSERROR (0,      0,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (1,      1,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (2,      2,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (3,      3,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (4,      WSAEINTR,           STATUS_USER_APC),
    MAPWSERROR (5,      5,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (6,      6,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (7,      7,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (8,      8,                  STATUS_UNSUCCESSFUL),
    MAPWSERROR (9,      WSAEBADF,           STATUS_INVALID_PARAMETER),
    MAPWSERROR (10,     10,                 STATUS_UNSUCCESSFUL),     
    MAPWSERROR (11,     11,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (12,     12,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (13,     WSAEACCES,          STATUS_ACCESS_DENIED),
    MAPWSERROR (14,     WSAEFAULT,          STATUS_ACCESS_VIOLATION),
    MAPWSERROR (15,     15,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (16,     16,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (17,     17,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (18,     18,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (19,     19,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (20,     20,                 STATUS_UNSUCCESSFUL),      
    MAPWSERROR (21,     21,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (22,     WSAEINVAL,          STATUS_INVALID_PARAMETER),
    MAPWSERROR (23,     23,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (24,     WSAEMFILE,          STATUS_TOO_MANY_ADDRESSES),
    MAPWSERROR (25,     25,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (26,     26,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (27,     27,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (28,     28,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (29,     29,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (30,     30,                 STATUS_UNSUCCESSFUL),      
    MAPWSERROR (31,     31,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (32,     32,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (33,     33,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (34,     34,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (35,     WSAEWOULDBLOCK,     STATUS_MORE_PROCESSING_REQUIRED),          
    MAPWSERROR (36,     WSAEINPROGRESS,     STATUS_DEVICE_BUSY),
    MAPWSERROR (37,     WSAEALREADY,        STATUS_UNSUCCESSFUL),
    MAPWSERROR (38,     WSAENOTSOCK,        STATUS_INVALID_HANDLE),
    MAPWSERROR (39,     WSAEDESTADDRREQ,    STATUS_INVALID_PARAMETER),
    MAPWSERROR (40,     WSAEMSGSIZE,        STATUS_BUFFER_OVERFLOW),
    MAPWSERROR (41,     WSAEPROTOTYPE,      STATUS_INVALID_PARAMETER),
    MAPWSERROR (42,     WSAENOPROTOOPT,     STATUS_NOT_SUPPORTED),
    MAPWSERROR (43,     WSAEPROTONOSUPPORT, STATUS_NOT_SUPPORTED),
    MAPWSERROR (44,     WSAESOCKTNOSUPPORT, STATUS_NOT_SUPPORTED),
    MAPWSERROR (45,     WSAEOPNOTSUPP,      STATUS_NOT_SUPPORTED),
    MAPWSERROR (46,     WSAEPFNOSUPPORT,    STATUS_NOT_SUPPORTED),
    MAPWSERROR (47,     WSAEAFNOSUPPORT,    STATUS_NOT_SUPPORTED),
    MAPWSERROR (48,     WSAEADDRINUSE,      STATUS_ADDRESS_ALREADY_EXISTS),
    MAPWSERROR (49,     WSAEADDRNOTAVAIL,   STATUS_INVALID_ADDRESS_COMPONENT),
    MAPWSERROR (50,     WSAENETDOWN,        STATUS_UNEXPECTED_NETWORK_ERROR),
    MAPWSERROR (51,     WSAENETUNREACH,     STATUS_NETWORK_UNREACHABLE),
    MAPWSERROR (52,     WSAENETRESET,       STATUS_CONNECTION_RESET),
    MAPWSERROR (53,     WSAECONNABORTED,    STATUS_CONNECTION_ABORTED),
    MAPWSERROR (54,     WSAECONNRESET,      STATUS_CONNECTION_RESET),      
    MAPWSERROR (55,     WSAENOBUFS,         STATUS_INSUFFICIENT_RESOURCES),
    MAPWSERROR (56,     WSAEISCONN,         STATUS_CONNECTION_ACTIVE),
    MAPWSERROR (57,     WSAENOTCONN,        STATUS_INVALID_CONNECTION),
    MAPWSERROR (58,     WSAESHUTDOWN,       STATUS_INVALID_CONNECTION),
    MAPWSERROR (59,     WSAETOOMANYREFS,    STATUS_UNSUCCESSFUL),
    MAPWSERROR (60,     WSAETIMEDOUT,       STATUS_IO_TIMEOUT),
    MAPWSERROR (61,     WSAECONNREFUSED,    STATUS_CONNECTION_REFUSED),
    MAPWSERROR (62,     WSAELOOP,           STATUS_UNSUCCESSFUL),
    MAPWSERROR (63,     WSAENAMETOOLONG,    STATUS_NAME_TOO_LONG),
    MAPWSERROR (64,     WSAEHOSTDOWN,       STATUS_HOST_UNREACHABLE),
    MAPWSERROR (65,     WSAEHOSTUNREACH,    STATUS_HOST_UNREACHABLE),
    MAPWSERROR (66,     WSAENOTEMPTY,       STATUS_UNSUCCESSFUL),
    MAPWSERROR (67,     WSAEPROCLIM,        STATUS_INSUFFICIENT_RESOURCES),
    MAPWSERROR (68,     WSAEUSERS,          STATUS_UNSUCCESSFUL),
    MAPWSERROR (69,     WSAEDQUOT,          STATUS_INSUFFICIENT_RESOURCES),
    MAPWSERROR (70,     WSAESTALE,          STATUS_UNSUCCESSFUL),
    MAPWSERROR (71,     WSAEREMOTE,         STATUS_UNSUCCESSFUL),
    MAPWSERROR (72,     72,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (73,     73,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (74,     74,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (75,     75,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (76,     76,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (77,     77,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (78,     78,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (79,     79,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (80,     80,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (81,     81,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (82,     82,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (83,     83,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (84,     84,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (85,     85,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (86,     86,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (87,     87,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (88,     88,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (89,     89,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (90,     90,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (91,     WSASYSNOTREADY,     STATUS_MISSING_SYSTEMFILE),
    MAPWSERROR (92,     WSAVERNOTSUPPORTED, STATUS_UNSUCCESSFUL),
    MAPWSERROR (93,     WSANOTINITIALISED,  STATUS_APP_INIT_FAILURE),
    MAPWSERROR (94,     94,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (95,     95,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (96,     96,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (97,     97,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (98,     98,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (99,     99,                 STATUS_UNSUCCESSFUL),
    MAPWSERROR (100,    100,                STATUS_UNSUCCESSFUL),
    MAPWSERROR (101,    WSAEDISCON,         STATUS_GRACEFUL_DISCONNECT),
    MAPWSERROR (102,    WSAENOMORE,         STATUS_NO_MORE_ENTRIES),
    MAPWSERROR (103,    WSAECANCELLED,      STATUS_CANCELLED),
    MAPWSERROR (104,    WSAEINVALIDPROCTABLE,STATUS_UNSUCCESSFUL),
    MAPWSERROR (105,    WSAEINVALIDPROVIDER,STATUS_UNSUCCESSFUL),
    MAPWSERROR (106,    WSAEPROVIDERFAILEDINIT,STATUS_UNSUCCESSFUL),
    MAPWSERROR (107,    WSASYSCALLFAILURE,  STATUS_UNSUCCESSFUL),
    MAPWSERROR (108,    WSASERVICE_NOT_FOUND,STATUS_INVALID_SYSTEM_SERVICE),
    MAPWSERROR (109,    WSATYPE_NOT_FOUND,  STATUS_UNSUCCESSFUL),
    MAPWSERROR (110,    WSA_E_NO_MORE,      STATUS_NO_MORE_ENTRIES),
    MAPWSERROR (111,    WSA_E_CANCELLED,    STATUS_CANCELLED),
    MAPWSERROR (112,    WSAEREFUSED,        STATUS_CONNECTION_REFUSED)
    };
        // This is most likely code
    if (dwError==NO_ERROR)
        return NO_ERROR;
        // Process winsock codes
    else if ((dwError>=WSABASEERR) 
            && (dwError<WSABASEERR+sizeof(WSAEMap)/sizeof(WSAEMap[0])))
        return WSAEMap[dwError-WSABASEERR];
        // Process system specific codes
    else {
        switch (dwError) {
        case WSA_IO_PENDING:
        case WSA_IO_INCOMPLETE:
            return STATUS_UNSUCCESSFUL;
        case WSA_INVALID_HANDLE:
            return STATUS_INVALID_HANDLE;
        case WSA_INVALID_PARAMETER:
            return STATUS_INVALID_PARAMETER;
        case WSA_NOT_ENOUGH_MEMORY:
            return STATUS_INSUFFICIENT_RESOURCES;
        case WSA_OPERATION_ABORTED:  
            return STATUS_CANCELLED;
        default:
            return STATUS_UNSUCCESSFUL;
        }
    }

}

DWORD
Ws2helpInitialize (
    VOID
    ) {
    EnterCriticalSection (&StartupSyncronization);
    if (!Ws2helpInitialized) {
        NewCtxInit ();
#if DBG
        ReadDbgInfo ();
#endif
        Ws2helpInitialized = TRUE;
    }
    LeaveCriticalSection (&StartupSyncronization);

    return 0;
}

#if DBG
VOID
ReadDbgInfo (
    VOID
    ) {
    TCHAR                       ProcessFilePath[MAX_PATH+1];
    LPTSTR                      pProcessFileName;
    HKEY                        hDebugKey;
    DWORD                       sz, rc, level;

    PID = GetCurrentProcessId ();
    if (GetModuleFileName (NULL, ProcessFilePath, sizeof (ProcessFilePath))>0) {
        pProcessFileName = _tcsrchr (ProcessFilePath, '\\');
        if (pProcessFileName!=NULL)
            pProcessFileName += 1;
        else
            pProcessFileName = ProcessFilePath;
    }
    else
        DbgPrint ("WS2HELP-%lx ReadDbgInfo: Could not get process name, err=%ld.\n",
                                                PID, GetLastError ());
    if ((rc=RegOpenKeyExA (HKEY_LOCAL_MACHINE,
            WS2IFSL_DEBUG_KEY,
            0,
            KEY_QUERY_VALUE,
            &hDebugKey))==NO_ERROR) {
        sz = sizeof (DbgLevel);
        if (RegQueryValueEx (hDebugKey,
                TEXT("DbgLevel"),
                NULL,
                NULL, 
                (LPBYTE)&level,
                &sz)==0)
			DbgLevel = level;

        sz = sizeof (DbgLevel);
        if (RegQueryValueEx (hDebugKey,
                pProcessFileName,
                NULL,
                NULL, 
                (LPBYTE)&level,
                &sz)==0)
			DbgLevel = level;
        RegCloseKey (hDebugKey);
        DbgPrint ("WS2HELP-%lx ReadDbgInfo: DbgLevel set to %lx.\n",
                   PID, DbgLevel);
        
    }
    else if (rc!=ERROR_FILE_NOT_FOUND)
        DbgPrint ("WS2HELP-%lx ReadDbgInfo: Could not open dbg key (%s), err=%ld.\n",
                   PID, WS2IFSL_DEBUG_KEY, rc);
}


#endif //DBG
