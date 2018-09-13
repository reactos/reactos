/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    notify.c

Abstract:

    This module implements the notification handle helper functions for the WinSock 2.0
    helper library.

Author:
    Vadim Eydelman (VadimE)


Revision History:

--*/
#include "precomp.h"
#include "osdef.h"
#include <accctrl.h>
#include <aclapi.h>

//
//  Private constants.
//
#define FAKE_NOTIFICATION_HELPER_HANDLE     ((HANDLE)'VD  ')
#define WS2_PIPE_BASE           L"\\Device\\NamedPipe\\"
#define WS2_PIPE_FORMAT         L"Winsock2\\CatalogChangeListener-%x-%x"
#define WS2_PIPE_WILDCARD       L"WINSOCK2\\CATALOGCHANGELISTENER-*-*"

/* Private Prototypes */
VOID
PipeListenApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

DWORD
GetWinsockRootSecurityDescriptor (
    OUT PSECURITY_DESCRIPTOR    *pDescr
    );

DWORD
BuildPipeSecurityDescriptor (
    IN  PSECURITY_DESCRIPTOR    pBaseDescr,
    OUT PSECURITY_DESCRIPTOR    *pDescr
    );


// Current pipe number (to avoid unnecessary collisions during
// pipe creation)
LONG PipeSerialNumber = 0;

// Security descriptor that we use to grant write permissions to pipe
// to only those who have write permisions to registry key with
// winsock catalog
PSECURITY_DESCRIPTOR        pSDPipe = NULL;

#if DBG
VOID        DumpSid (PSID pSid, LPSTR AccessType);
#endif


DWORD
WINAPI
WahOpenNotificationHandleHelper(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    This routine opens WinSock 2.0 notification handle helper device

Arguments:

    HelperHandle - Points to buffer ion which to return handle.


Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    DWORD   rc;
    rc = ENTER_WS2HELP_API ();
    if (rc!=0)
        return rc;

    //
    //  Validate parameters.
    //

    if( HelperHandle == NULL ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Just return a fake handle.
    //

    *HelperHandle = FAKE_NOTIFICATION_HELPER_HANDLE;

    return NO_ERROR;

}   // WahOpenNotificationHandleHelper

DWORD
WINAPI
WahCloseNotificationHandleHelper(
    IN HANDLE HelperHandle
    )

/*++

Routine Description:

    This function closes the WinSock 2.0 notification helper device.

Arguments:

    HelperHandle - The handle to close.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{
    DWORD   rc;

    rc = ENTER_WS2HELP_API ();
    if (rc!=0)
        return rc;

    //
    //  Validate parameters.
    //

    if( HelperHandle != FAKE_NOTIFICATION_HELPER_HANDLE ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Nothing to do.
    //

    return NO_ERROR;

}   // WahCloseNotificationHandleHelper

DWORD
WINAPI
WahCreateNotificationHandle(
    IN HANDLE           HelperHandle,
    OUT HANDLE          *h
    )
/*++

Routine Description:

    This function creates notificaion handle to receive asyncronous
    interprocess notifications.

Arguments:

    HelperHandle - The handle of WinSock 2.0 handle helper.
    h            - buffer to return created notification handle

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    WCHAR               name[MAX_PATH];
    UNICODE_STRING      uName;
    OBJECT_ATTRIBUTES   attr;
    DWORD               rc = 0;
    LARGE_INTEGER       readTimeout;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;

    PSECURITY_DESCRIPTOR    pSDKey, pSDTemp;

    rc = ENTER_WS2HELP_API ();
    if (rc!=0)
        return rc;


    if (HelperHandle==NULL)
        return ERROR_INVALID_PARAMETER;
    else if ((HelperHandle!=FAKE_NOTIFICATION_HELPER_HANDLE)
            || (h==NULL))
        return ERROR_INVALID_PARAMETER;

    //
    // Build security descriptor for the pipe if we have not
    // already done this
    //

    if (pSDPipe==NULL) {
        //
        // First get security descriptor of the registry key that
        // contains the Winsock2 catalogs
        //
        rc = GetWinsockRootSecurityDescriptor (&pSDKey);
        if (rc==0) {
            //
            // Build the pipe security descriptor with grants
            // write permissions to same set of SIDs that have
            // write permissions to the registry key.
            //
            rc = BuildPipeSecurityDescriptor (pSDKey, &pSDTemp);
            if (rc==0) {
                //
                // Set the global if someone hasn't done this
                // while we were building it
                //
                if (InterlockedCompareExchangePointer (&pSDPipe,
                                            pSDTemp,
                                            NULL
                                            )!=NULL)
                    //
                    // Someone else did this, free ours
                    //
                    FREE_MEM (pSDTemp);
            }
            //
            // Free registry key descriptor
            //
            FREE_MEM (pSDKey);
        }
    }

    if (rc==0) {

        //
        // We use default timeout on our pipe (we do not actually
        // care what it is)
        //
        readTimeout.QuadPart =  -10 * 1000 * 50;

        do {
            //
            // Try to build unique pipe name using serial number
            //
            swprintf (name, WS2_PIPE_BASE WS2_PIPE_FORMAT,
                            GetCurrentProcessId(), PipeSerialNumber);
            InterlockedIncrement (&PipeSerialNumber);
            RtlInitUnicodeString( &uName, name );
            InitializeObjectAttributes (
                        &attr,
                        &uName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        pSDPipe);

            //
            // Try to create it
            //

            status = NtCreateNamedPipeFile (
                        h,                              // Handle
                        GENERIC_READ                    // DesiredAccess
                            | SYNCHRONIZE
                            | WRITE_DAC,
                        &attr,                          // Obja
                        &ioStatusBlock,                 // IoStatusBlock
                        FILE_SHARE_WRITE,               // Share access
                        FILE_CREATE,                    // CreateDisposition
                        0,                              // CreateFlags
                        FILE_PIPE_MESSAGE_TYPE,         // NamedPipeType
                        FILE_PIPE_MESSAGE_MODE,         // ReadMode
                        FILE_PIPE_QUEUE_OPERATION,      // CompletionMode
                        1,                              // MaxInstances
                        4,                              // InboundQuota
                        0,                              // OutboundQuota
                        &readTimeout                    // Timeout
                        );
            //
            // Continue on doing this if we have name collision
            // (serial number wrapped!!! or someone attempts to
            // interfere with out operation by using the same
            // naming scheme!!!)
            //
        }
        while (status==STATUS_OBJECT_NAME_COLLISION);

        if (NT_SUCCESS (status)) {
            WshPrint (DBG_NOTIFY,
                ("WS2HELP-%lx WahCreateNotificationHandle: Created pipe %ls.\n",
                PID, name));
            rc = 0;
        }
        else {
            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                ("WS2HELP-%lx WahCreateNotificationHandle: Could not create pipe %ls, status: %lx\n",
                PID, name, status));
            rc = RtlNtStatusToDosError (status);
        }
    }

    return rc;
}


DWORD
WINAPI
WahWaitForNotification(
    IN HANDLE           HelperHandle,
    IN HANDLE           h,
    IN LPWSAOVERLAPPED  lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
/*++

Routine Description:

    This function waits for asyncronous interprocess notifications
    received on the pipe.

Arguments:

    HelperHandle - The handle of WinSock 2.0 handle helper.
    h            - notification handle
    lpOverlapped - overlapped structure for async IO
    lpCompletionRoutine - completion routine for async IO

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    DWORD           rc;
    NTSTATUS        status;
    IO_STATUS_BLOCK localIoStatus;

    HANDLE          event;
    PIO_APC_ROUTINE apcRoutine;
    PVOID           apcContext;
    PIO_STATUS_BLOCK ioStatus;

    rc = ENTER_WS2HELP_API ();
    if (rc!=0)
        return rc;

    if (HelperHandle==NULL)
        return ERROR_INVALID_PARAMETER;
    else if (HelperHandle!=FAKE_NOTIFICATION_HELPER_HANDLE)
        return ERROR_INVALID_PARAMETER;


    //
    // Disconnect previous client if any.
    // (If none were connected, the call fails, but
    // we ignore the error anyway).
    //
    status = NtFsControlFile(
                        h,
                        NULL,
                        NULL,   // ApcRoutine
                        NULL,   // ApcContext
                        &localIoStatus,
                        FSCTL_PIPE_DISCONNECT,
                        NULL,   // InputBuffer
                        0,      // InputBufferLength,
                        NULL,   // OutputBuffer
                        0       // OutputBufferLength
                        );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject( h, FALSE, NULL );
    }

    if (lpOverlapped) {
        //
        // Async IO, use overlapped for IoStatus and read buffer
        //
        ioStatus = (PIO_STATUS_BLOCK)lpOverlapped;
        ioStatus->Status = STATUS_PENDING;
        if (lpCompletionRoutine) {
            //
            // Async IO with completion routine
            //
            event = NULL;
            apcRoutine = PipeListenApc;
            apcContext = lpCompletionRoutine;
        }
        else {
            //
            // Event based or completion port async IO
            // 1 in low-order bit means they want to bypass completion
            // port
            //
            event = lpOverlapped->hEvent;
            apcRoutine = NULL;
            apcContext = ((ULONG_PTR)lpOverlapped->hEvent & 1) ? NULL : lpOverlapped;
        }
    }
    else {
        //
        // Synchronous IO, use locals for IoStatus and read buffer
        //
        ioStatus = &localIoStatus;
        apcRoutine = NULL;
        apcContext = NULL;
        event = NULL;
    }

    //
    // Listen for new client
    //
    status = NtFsControlFile (
                    h,                  // Filehandle
                    event,              // Event
                    apcRoutine,         // ApcRoutine
                    apcContext,         // ApcContext
                    ioStatus,           // IoStatusBlock
                    FSCTL_PIPE_LISTEN,  // IoControlCode
                    NULL,               // InputBuffer
                    0,                  // InputBufferLength
                    NULL,               // OutputBuffer
                    0                   // OutputBufferLength
                    );

    if ((lpOverlapped==NULL) && (status==STATUS_PENDING)) {
        //
        // Wait for completion if IO was synchronous and
        // NtFsControlFile returned pending
        //
        status = NtWaitForSingleObject( h, FALSE, NULL );
        if (NT_SUCCESS (status)) {
            status = ioStatus->Status;
        }
        else {
            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                ("WS2HELP-%lx WahWaitForNotification:"
                " Wait failed, status: %lx\n",
                PID, status));
        }
    }

    // Convert status code
    if (status==STATUS_SUCCESS)
        rc = 0;
    else if (status==STATUS_PENDING) {
        rc = WSA_IO_PENDING;
    }
    else {
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx WahWaitForNotification:"
            " Wait failed, status: %lx\n",
            PID, status));
        rc = RtlNtStatusToDosError (status);
    }
    return rc;
}

DWORD
WahNotifyAllProcesses (
    IN HANDLE           HelperHandle
    ) {
/*++

Routine Description:

    This function notifies all the processes that listen for
    notifications

Arguments:

    HelperHandle - The handle of WinSock 2.0 handle helper.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
    HANDLE          hDir, hPipe;
    struct {
        FILE_NAMES_INFORMATION  Info;
        WCHAR                   FileName[255];
    }               NameInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING  NameFormat;
    PUNICODE_STRING pNameFormat;
    NTSTATUS        status;
    UNICODE_STRING  PipeRootName;
    OBJECT_ATTRIBUTES PipeRootAttr;
    DWORD           id = GetCurrentProcessId (), count;
    WCHAR           name[MAX_PATH];
    UNICODE_STRING  uName;
    OBJECT_ATTRIBUTES attr;
    DWORD           rc;

    rc = ENTER_WS2HELP_API ();
    if (rc!=0)
        return rc;

    if (HelperHandle==NULL)
        return ERROR_INVALID_PARAMETER;
    else if (HelperHandle!=FAKE_NOTIFICATION_HELPER_HANDLE)
        return ERROR_INVALID_PARAMETER;

    //
    // Open handle to pipe root directory
    //
    RtlInitUnicodeString( &PipeRootName, WS2_PIPE_BASE );
    InitializeObjectAttributes(
        &PipeRootAttr,
        &PipeRootName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile (
              &hDir,                                // FileHandle
              FILE_LIST_DIRECTORY | SYNCHRONIZE,    // DesiredAccess
              &PipeRootAttr,                        // ObjectAttributes
              &ioStatusBlock,                       // IoStatusBlock
              FILE_SHARE_READ|FILE_SHARE_WRITE,     // ShareAccess
              FILE_DIRECTORY_FILE                   // OpenOptions
                | FILE_SYNCHRONOUS_IO_NONALERT
                | FILE_OPEN_FOR_BACKUP_INTENT);

    if (NT_SUCCESS(status)) {
        //
        // Enumerate all pipes that match our pattern
        //
        RtlInitUnicodeString( &NameFormat, WS2_PIPE_WILDCARD );
        pNameFormat = &NameFormat;
        while ((status=NtQueryDirectoryFile (
                            hDir,                       // File Handle
                            NULL,                       // Event
                            NULL,                       // Apc routine
                            NULL,                       // Apc context
                            &ioStatusBlock,             // IoStatusBlock
                            &NameInfo,                  // FileInformation
                            sizeof(NameInfo),           // Length
                            FileNamesInformation,       // FileInformationClass
                            TRUE,                       // ReturnSingleEntry
                            pNameFormat,                // FileName
                            (BOOLEAN)(pNameFormat!=NULL)// RestartScan
                            ))==STATUS_SUCCESS) {

            pNameFormat = NULL; // No need for pattern on second
                                // and all successive enum calls

            //
            // Create client and of the pipe that matched the
            // pattern
            //
            NameInfo.Info.FileName[
                NameInfo.Info.FileNameLength
                    /sizeof(NameInfo.Info.FileName[0])] = 0;

            swprintf (name, WS2_PIPE_BASE L"%ls",
                        NameInfo.Info.FileName);

            RtlInitUnicodeString( &uName, name );
            InitializeObjectAttributes(
                &attr,
                &uName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );
            status = NtCreateFile (
                            &hPipe,
                            GENERIC_WRITE | SYNCHRONIZE,
                            &attr,
                            &ioStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ,
                            OPEN_EXISTING,
                            0,
                            NULL,
                            0);
            if (NT_SUCCESS (status)) {
                WshPrint (DBG_NOTIFY,
                    ("WS2HELP-%lx WahNotifyAllProcesses:"
                    " Opened pipe %ls.\n",
                    PID, name));
                NtClose (hPipe);
            }
#if DBG
            else if (status==STATUS_PIPE_NOT_AVAILABLE) {
                WshPrint (DBG_NOTIFY,
                    ("WS2HELP-%lx WahNotifyAllProcesses:"
                    " Pipe %ls is not currently listening.\n",
                    PID, name));
            }
            else {
                WshPrint (DBG_NOTIFY|DBG_FAILURES,
                    ("WS2HELP-%lx WahNotifyAllProcesses:"
                    " Could not open pipe %ls, status: %lx\n",
                    PID, name, status));
            }
#endif
        }
        if (status!=STATUS_NO_MORE_FILES) {
            //
            // Some other failure, means we could not even
            // enumerate
            //
            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                ("WS2HELP-%lx WahNotifyAllProcesses:"
                " Could enumerate pipes, status: %lx\n",
                PID, status));
        }
        NtClose (hDir);
    }
    else {
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx WahNotifyAllProcesses:"
            " Could open pipe root, status: %lx\n",
            PID, status));
    }

    return 0;
}

// Private functions

VOID
PipeListenApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
/*++

Routine Description:

    This NT IO APC that converts parameters and calls client APC.

Arguments:

    ApcContext  -   apc context - pointer to client APC

    IoStatusBlock - io status block - part of client's overlapped structure

    Reserved    -   reserved


Return Value:

    None


--*/
{
    DWORD   rc;
    switch (IoStatusBlock->Status) {
    case STATUS_SUCCESS:
        rc = 0;
        break;
    case STATUS_CANCELLED:
        rc = WSA_OPERATION_ABORTED;
        break;
    default:
        rc = WSASYSCALLFAILURE;
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx PipeListenApc:"
            " Failed, status: %lx\n",
            PID, IoStatusBlock->Status));
        break;

    }
    (*(LPWSAOVERLAPPED_COMPLETION_ROUTINE)ApcContext)
            (rc, 0, (LPWSAOVERLAPPED)IoStatusBlock, 0);
}


DWORD
GetWinsockRootSecurityDescriptor (
    PSECURITY_DESCRIPTOR    *pDescr
    )
/*++

Routine Description:

    Reads security descriptor of the registry key that contains
    Winsock2 catalogs

Arguments:

    pDescr  - buffer to receive locally allocated pointer to descriptor
Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    HKEY                hKey;
    DWORD               sz;
    DWORD               rc;

    //
    // Open the key
    //
    rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                    WINSOCK_REGISTRY_ROOT,
                    0,
                    KEY_READ|READ_CONTROL,
                    &hKey
                    );
    if (rc==0) {
        //
        // Get the required buffer size
        //
        sz = 0;
        rc = RegGetKeySecurity (hKey,
                         DACL_SECURITY_INFORMATION, // we only need
                                                    // DACL to see
                                                    // who has write
                                                    // permissions to
                                                    // the key
                         NULL,
                         &sz);
        if (rc==ERROR_INSUFFICIENT_BUFFER) {
            //
            // Allocate buffer
            //
            *pDescr = (PSECURITY_DESCRIPTOR)ALLOC_MEM (sz);
            if (pDescr!=NULL) {
                //
                // Fetch the data
                //
                rc = RegGetKeySecurity (hKey,
                                 DACL_SECURITY_INFORMATION,
                                 *pDescr,
                                 &sz);
                if (rc==0) {
                    ;
                }
                else {
                    WshPrint (DBG_NOTIFY|DBG_FAILURES,
                        ("WS2HELP-%lx GetWinsockRootExplicitAccess:"
                        " Failed to get key security (data), err: %ld\n",
                        PID, rc));
                    FREE_MEM (*pDescr);
                }
            }
            else {
                rc = GetLastError ();
                WshPrint (DBG_NOTIFY|DBG_FAILURES,
                    ("WS2HELP-%lx GetWinsockRootExplicitAccess:"
                    " Failed to allocate security descriptor, err: %ld\n",
                    PID, rc));
                if (rc==0)
                    rc = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else {
            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                ("WS2HELP-%lx GetWinsockRootExplicitAccess:"
                " Failed to get key security (size), err: %ld\n",
                PID, rc));
        }
        //
        // Do not need key anymore
        //
        RegCloseKey (hKey);
    }
    else {
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx GetWinsockRootExplicitAccess:"
            " Failed to open winsock root key, err: %ld\n",
            PID, rc));
    }
    return rc;
}


DWORD
BuildPipeSecurityDescriptor (
    IN  PSECURITY_DESCRIPTOR    pBaseDescr,
    OUT PSECURITY_DESCRIPTOR    *pDescr
    )
/*++

Routine Description:

    Builds security descriptor with the same write permissions
    as in base descriptor (which is the descriptor of the
    registry key) minus network users

Arguments:

    pBaseDescr  - descriptor of the registry key from which to
                    read write permisions

    pDescr      - buffer to receive locally allocated pointer to descriptor

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/
{
    PACL                    pBaseDacl, pDacl;
    BOOL                    DaclPresent, DaclDefaulted;
    DWORD                   cbDacl;
    DWORD                   rc = 0;
    ACL_SIZE_INFORMATION    sizeInfo;
    ULONG                   i;
    ACE_HEADER              *pAce;
    SID_IDENTIFIER_AUTHORITY    siaNt = SECURITY_NT_AUTHORITY;
    PSID                    pSidNetUser;
    SID_IDENTIFIER_AUTHORITY    siaCreator = SECURITY_CREATOR_SID_AUTHORITY;
    PSID                    pSidCrOwner;
    PSID                    pSidCrGroup;


    *pDescr = NULL;

    //
    // Get DACL from the base descriptor
    //
    if (!GetSecurityDescriptorDacl (
                        pBaseDescr,
                        &DaclPresent,
                        &pBaseDacl,
                        &DaclDefaulted
                        )) {
        rc = GetLastError ();
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
            " Failed to get DACL from base descriptor, err: %ld\n",
            PID, rc));
        if (rc==0)
            rc = ERROR_GEN_FAILURE;

        return rc;
    }

    //
    // Allocate SID for network users
    //

    if (!AllocateAndInitializeSid (&siaNt,
            1,
            SECURITY_NETWORK_RID,
            0,0,0,0,0,0,0,
            &pSidNetUser
            )) {
        rc = GetLastError ();
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
            " Failed to allocate net user SID, err: %ld\n",
            PID, rc));
        if (rc==0)
            rc = ERROR_GEN_FAILURE;

        return rc;
    }

    //
    // Allocate SID for creator/owner
    //

    if (!AllocateAndInitializeSid (&siaCreator,
            1,
            SECURITY_CREATOR_OWNER_RID,
            0,0,0,0,0,0,0,
            &pSidCrOwner
            )) {
        rc = GetLastError ();
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
            " Failed to allocate creator owner SID, err: %ld\n",
            PID, rc));
        if (rc==0)
            rc = ERROR_GEN_FAILURE;
        FreeSid (pSidNetUser);

        return rc;
    }

    //
    // Allocate SID for creator group
    //

    if (!AllocateAndInitializeSid (&siaCreator,
            1,
            SECURITY_CREATOR_GROUP_RID,
            0,0,0,0,0,0,0,
            &pSidCrGroup
            )) {
        rc = GetLastError ();
        WshPrint (DBG_NOTIFY|DBG_FAILURES,
            ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
            " Failed to allocate creator group SID, err: %ld\n",
            PID, rc));
        if (rc==0)
            rc = ERROR_GEN_FAILURE;
        FreeSid (pSidCrOwner);
        FreeSid (pSidNetUser);

        return rc;
    }

    //
    // Our DACL will at least contain an ACE that denies
    // access to all network users
    //
    cbDacl = sizeof (ACL)
                + FIELD_OFFSET (ACCESS_DENIED_ACE, SidStart)
                + GetLengthSid (pSidNetUser);


    //
    // If base DACL is present and non-NULL, we will need to
    // parse it, count all the accounts that have write permissions,
    // so we can allocate space in security descriptor to
    // hold them
    //
    if (DaclPresent && pBaseDacl!=NULL) {
        //
        // Get number of ACEs in the DACL
        //
        if (GetAclInformation (pBaseDacl,
                                &sizeInfo,
                                sizeof (sizeInfo),
                                AclSizeInformation
                                )) {
            //
            // Enumerate all ACEs to get the required size
            // of the DACL that we are about to build
            //
            for (i=0; i<sizeInfo.AceCount; i++) {
                if (GetAce (pBaseDacl, i, &pAce)) {
                    //
                    // Only count access-allowed or access-denied ACEs
                    // with write access to the key
                    //
                    switch (pAce->AceType) {
                    case ACCESS_ALLOWED_ACE_TYPE:
                        #define paAce ((ACCESS_ALLOWED_ACE  *)pAce)
                        if (((paAce->Mask & KEY_WRITE)==KEY_WRITE)
                                && !EqualSid (&paAce->SidStart, pSidCrOwner)
                                && !EqualSid (&paAce->SidStart, pSidCrGroup)
                                    ) {
                            cbDacl += FIELD_OFFSET (ACCESS_ALLOWED_ACE, SidStart)
                                        +  GetLengthSid (&paAce->SidStart);
                        }
                        #undef paAce
                        break;
                    case ACCESS_DENIED_ACE_TYPE:
                        #define pdAce ((ACCESS_DENIED_ACE  *)pAce)
                        if ((pdAce->Mask & KEY_WRITE)==KEY_WRITE) {
                            cbDacl += FIELD_OFFSET (ACCESS_DENIED_ACE, SidStart)
                                        +  GetLengthSid (&pdAce->SidStart);
                        }
                        #undef pdAce
                        break;
                    }

                }
                else {
                    rc = GetLastError ();
                    WshPrint (DBG_NOTIFY|DBG_FAILURES,
                        ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                        " Failed to get ACE # %ld, err: %ld\n",
                        PID, i, rc));
                    if (rc==0)
                        rc = ERROR_GEN_FAILURE;
                    break;
                }
            } // for
        }
        else {
            rc = GetLastError ();
            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                " Failed to get DACL size info, err: %ld\n",
                PID, rc));
            if (rc==0)
                rc = ERROR_GEN_FAILURE;
        }
    } // if DaclPresent and pDacl!=NULL
    else {
        //
        // Base DACL does not exist or empty
        //
        rc = 0;
    }


    if (rc==0) {
        //
        // Allocate memory for the descriptor and DACL
        //
        *pDescr = (PSECURITY_DESCRIPTOR)ALLOC_MEM
                        (sizeof (SECURITY_DESCRIPTOR)+cbDacl);
        if (*pDescr!=NULL) {
            pDacl = (PACL)((PUCHAR)(*pDescr)+sizeof(SECURITY_DESCRIPTOR));

            //
            // Initialize descriptor and DACL
            //

            if (InitializeSecurityDescriptor (*pDescr,
                            SECURITY_DESCRIPTOR_REVISION)
                   && InitializeAcl (pDacl, cbDacl, ACL_REVISION)) {

                //
                // First add access-denied ace for all
                // network users
                //

                if (AddAccessDeniedAce (pDacl,
                                        ACL_REVISION,
                                        GENERIC_WRITE
                                            |STANDARD_RIGHTS_WRITE
                                            |SYNCHRONIZE,
                                        pSidNetUser
                                        )) {
#if DBG
                    DumpSid (pSidNetUser, "Denying");
#endif

                    //
                    // If base DACL is present and non-NULL, we will need to
                    // parse it add all ACEs that have write permisions
                    // to the DACL we build
                    //
                    if (DaclPresent && (pBaseDacl!=NULL)) {
                        //
                        // Enumerate all ACEs and copy them
                        // to the new DACL
                        //
                        for (i=0; i<sizeInfo.AceCount; i++) {
                            if (GetAce (pBaseDacl, i, &pAce)) {
                                //
                                // Only count access-allowed or access-denied ACEs
                                // with write access to the key
                                //
                                switch (pAce->AceType) {
                                case ACCESS_ALLOWED_ACE_TYPE:
                                    #define paAce ((ACCESS_ALLOWED_ACE  *)pAce)
                                    if (((paAce->Mask & KEY_WRITE)==KEY_WRITE)
                                            && !EqualSid (&paAce->SidStart, pSidCrOwner)
                                            && !EqualSid (&paAce->SidStart, pSidCrGroup)
                                        ) {
                                        if (AddAccessAllowedAce (pDacl,
                                                ACL_REVISION,
                                                GENERIC_WRITE
                                                    | STANDARD_RIGHTS_WRITE
                                                    | SYNCHRONIZE
                                                    | FILE_READ_ATTRIBUTES,
                                                &paAce->SidStart
                                                )) {
#if DBG
                                            DumpSid (&paAce->SidStart, "Allowing");
#endif
                                        }
                                        else {
                                            rc = GetLastError ();
                                            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                                                ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                                                " Failed to add access allowed ACE # %ld, err: %ld\n",
                                                PID, i, rc));
                                            if (rc==0)
                                                rc = ERROR_GEN_FAILURE;
                                        }
                                    }
                                    #undef paAce
                                    break;
                                case ACCESS_DENIED_ACE_TYPE:
                                    #define pdAce ((ACCESS_DENIED_ACE  *)pAce)
                                    if ((pdAce->Mask & KEY_WRITE)==KEY_WRITE) {
                                        if (AddAccessDeniedAce (pDacl,
                                                ACL_REVISION,
                                                GENERIC_WRITE
                                                    | STANDARD_RIGHTS_WRITE
                                                    | SYNCHRONIZE,
                                                &pdAce->SidStart
                                                )) {
#if DBG
                                            DumpSid (&pdAce->SidStart, "Denying");
#endif
                                        }
                                        else {
                                            rc = GetLastError ();
                                            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                                                ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                                                " Failed to add access denied ACE # %ld, err: %ld\n",
                                                PID, i, rc));
                                            if (rc==0)
                                                rc = ERROR_GEN_FAILURE;
                                        }
                                    }
                                    #undef pdAce
                                    break;
                                }
                                if (rc!=0) {
                                    // Stop enumeration in case
                                    // of failure
                                    break;
                                }
                            }
                            else {
                                rc = GetLastError ();
                                WshPrint (DBG_NOTIFY|DBG_FAILURES,
                                    ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                                    " Failed to re-get ACE # %ld, err: %ld\n",
                                    PID, i, rc));
                                if (rc==0)
                                    rc = ERROR_GEN_FAILURE;
                                break;
                            }
                        } // for
                    } // if (DaclPresent and pBaseDacl!=NULL)
                }
                else {
                    rc = GetLastError ();
                    WshPrint (DBG_NOTIFY|DBG_FAILURES,
                        ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                        " Failed to add accees denied ace for net users %ld, err: %ld\n",
                        PID, rc));
                    if (rc==0)
                        rc = ERROR_GEN_FAILURE;
                }
            }
            else {
                rc = GetLastError ();
                WshPrint (DBG_NOTIFY|DBG_FAILURES,
                    ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                    " Failed to initialize DACL, err: %ld\n",
                    PID, rc));
                if (rc==0)
                    rc = ERROR_GEN_FAILURE;
            }

            //
            // If we succeded in building of the
            // DACL, add it to the security descriptor
            //
            if (rc==0) {
                if (SetSecurityDescriptorDacl (
                                *pDescr,
                                TRUE,
                                pDacl,
                                FALSE
                                )) {
                    rc = 0;
                }
                else {
                    rc = GetLastError ();
                    WshPrint (DBG_NOTIFY|DBG_FAILURES,
                        ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                        " Failed to set DACL, err: %ld\n",
                        PID, rc));
                    if (rc==0)
                        rc = ERROR_GEN_FAILURE;
                }
            } // if rc==0 (DACL is built)
            else {
                // Failed to build DACL, free memory for security descriptor
                FREE_MEM (*pDescr);
                *pDescr = NULL;
            }
        }
        else {
            rc = GetLastError ();
            WshPrint (DBG_NOTIFY|DBG_FAILURES,
                ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                " Failed to allocate security descriptor, err: %ld\n",
                PID, rc));
                if (rc==0)
                    rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    } // if rc==0 (Parsed base DACL ok)

    //
    // Free net user sid which we already copied to DACL in security
    // descriptor
    //
    FreeSid (pSidNetUser);
    FreeSid (pSidCrOwner);
    FreeSid (pSidCrGroup);

    return rc;
}

#if DBG
VOID
DumpSid (
    PSID    pSid,
    LPSTR   AccessType
    ) {
    TCHAR   Name[256];
    DWORD   szName = sizeof (Name);
    TCHAR   Domain[256];
    DWORD   szDomain = sizeof (Domain);
    SID_NAME_USE    nameUse;
    static HANDLE SAM_SERVICE_STARTED_EVENT = NULL;

    if (DbgLevel & DBG_NOTIFY) {
        if (SAM_SERVICE_STARTED_EVENT==NULL) {
            SAM_SERVICE_STARTED_EVENT = CreateEvent (NULL,
                                        FALSE,
                                        FALSE,
                                        TEXT("SAM_SERVICE_STARTED"));
            if (SAM_SERVICE_STARTED_EVENT!=NULL) {
                DWORD   rc;
                rc = WaitForSingleObject (SAM_SERVICE_STARTED_EVENT, 0);
                CloseHandle (SAM_SERVICE_STARTED_EVENT);
                if (rc!=WAIT_OBJECT_0) {
                    // Reset global so we try this again
                    SAM_SERVICE_STARTED_EVENT = NULL;
                    return;
                }
                // proceed without resetting global so we do not try again
                
            }
            else
                // Event must not have been created
                return;
        }

        if (LookupAccountSid (NULL,
                            pSid,
                            Name,
                            &szName,
                            Domain,
                            &szDomain,
                            &nameUse
                            )) {
            WshPrint (DBG_NOTIFY,
                ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                " %s write permissions to %s\\%s (use:%d).\n",
                PID, AccessType, Domain, Name, nameUse));

        }
        else {
            DWORD   i, n;
            WshPrint (DBG_NOTIFY,
                ("WS2HELP-%lx BuildPipeSecurityDescriptor:"
                " Could not lookup name for sid, err: %ld.\n",
                PID, GetLastError()));
            WshPrint (DBG_NOTIFY,
                ("WS2HELP-%lx, SID dump: S-%d-%d",
                PID, SID_REVISION, GetSidIdentifierAuthority(pSid)->Value[6]));
            n = *GetSidSubAuthorityCount(pSid);
            for (i=0; i<n; i++)
                WshPrint (DBG_NOTIFY, ("-%d", *GetSidSubAuthority (pSid, i)));
            WshPrint (DBG_NOTIFY, ("\n"));
        }
    }
}
#endif
