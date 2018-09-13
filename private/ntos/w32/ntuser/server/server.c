/**************************************************************************\
* Module Name: server.c
*
* Server support routines for the CSR stuff.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Created: 10-Dec-90
*
* History:
*   10-Dec-90 created by sMeans
*
\**************************************************************************/


#include "precomp.h"
#pragma hdrstop

#include "dbt.h"
#include "ntdddisk.h"
#include "ntuser.h"
#include <regstr.h>


HANDLE hThreadNotification;
HANDLE hKeyPriority;
UNICODE_STRING PriorityValueName;
IO_STATUS_BLOCK IoStatusRegChange;
ULONG RegChangeBuffer;
HANDLE ghNlsEvent;
BOOL gfLogon;
FARPROC gpfnAttachRoutine;
HANDLE ghPowerRequestEvent;
HANDLE ghMediaRequestEvent;

#define ID_NLS              0
#define ID_POWER            1
#define ID_MEDIACHANGE      2
#define ID_NETDEVCHANGE     3

#define ID_NUM_EVENTS       4

//
// Name of event to pulse to request a device-arrival broadcast,
//
#define SC_BSM_EVENT_NAME   L"ScNetDrvMsg"

//
// What the net drive bitmask was when we last broadcast (initially 0)
//
DWORD LastNetDrives;


HANDLE CsrApiPort;
HANDLE CsrQueryApiPort(VOID);

ULONG
SrvExitWindowsEx(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvEndTask(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvLogon(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvRegisterServicesProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvActivateDebugger(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvGetThreadConsoleDesktop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvDeviceEvent(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvRegisterLogonProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvWin32HeapFail(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

ULONG
SrvWin32HeapStat(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

PCSR_API_ROUTINE UserServerApiDispatchTable[ UserpMaxApiNumber - UserpExitWindowsEx ] = {
    (PCSR_API_ROUTINE)SrvExitWindowsEx,
    (PCSR_API_ROUTINE)SrvEndTask,
    (PCSR_API_ROUTINE)SrvLogon,
    (PCSR_API_ROUTINE)SrvRegisterServicesProcess,
    (PCSR_API_ROUTINE)SrvActivateDebugger,
    (PCSR_API_ROUTINE)SrvGetThreadConsoleDesktop,
    (PCSR_API_ROUTINE)SrvDeviceEvent,
    (PCSR_API_ROUTINE)SrvRegisterLogonProcess,
    (PCSR_API_ROUTINE)SrvWin32HeapFail,
    (PCSR_API_ROUTINE)SrvWin32HeapStat,
};

BOOLEAN UserServerApiServerValidTable[ UserpMaxApiNumber - UserpExitWindowsEx ] = {
    FALSE,      // ExitWindowsEx
    FALSE,      // EndTask
    FALSE,      // Logon
    FALSE,      // RegisterServicesProcess
    FALSE,      // ActivateDebugger
    TRUE,       // GetThreadConsoleDesktop
    FALSE,      // DeviceEvent
    FALSE,      // RegisterLogonProcess
    FALSE,      // Win32HeapFail
    FALSE,      // Win32HeapStat
};

#if DBG
PSZ UserServerApiNameTable[ UserpMaxApiNumber - UserpExitWindowsEx ] = {
    "SrvExitWindowsEx",
    "SrvEndTask",
    "SrvLogon",
    "SrvRegisterServicesProcess",
    "SrvActivateDebugger",
    "SrvGetThreadConsoleDesktop",
    "SrvDeviceEvent",
    "SrvRegisterLogonProcess",
    "SrvWin32HeapFail",
    "SrvWin32HeapStat"
};
#endif // DBG

NTSTATUS
UserServerDllInitialization(
    PCSR_SERVER_DLL psrvdll
    );

NTSTATUS UserClientConnect(PCSR_PROCESS Process, PVOID ConnectionInformation,
        PULONG pulConnectionLen);
VOID     UserHardError(PCSR_THREAD pcsrt, PHARDERROR_MSG pmsg);
NTSTATUS UserClientShutdown(PCSR_PROCESS Process, ULONG dwFlags, BOOLEAN fFirstPass);

VOID GetTimeouts(VOID);
VOID StartRegReadRead(VOID);
VOID RegReadApcProcedure(PVOID RegReadApcContext, PIO_STATUS_BLOCK IoStatus);
VOID NotificationThread(PVOID);
typedef BOOL (*PFNPROCESSCREATE)(DWORD, DWORD, ULONG_PTR, DWORD);

VOID InitializeConsoleAttributes(VOID);
NTSTATUS GetThreadConsoleDesktop(DWORD dwThreadId, HDESK *phdesk);
NTSTATUS MyRegOpenKey(IN HANDLE hKey, IN LPWSTR lpSubKey, OUT PHANDLE phResult);

BOOL BaseSetProcessCreateNotify(PFNPROCESSCREATE pfn);
VOID BaseSrvNlsUpdateRegistryCache(PVOID ApcContext,
                                             PIO_STATUS_BLOCK pIoStatusBlock);
NTSTATUS BaseSrvNlsLogon(BOOL);
NTSTATUS WinStationAPIInit(VOID);

/***************************************************************************\
* UserServerDllInitialization
*
* Called by the CSR stuff to allow a server DLL to initialize itself and
* provide information about the APIs it provides.
*
* Several operations are performed during this initialization:
*
* - The shared heap (client read-only) handle is initialized.
* - The Raw Input Thread (RIT) is launched.
* - GDI is initialized.
*
* History:
* 10-19-92 DarrinM      Integrated xxxUserServerDllInitialize into this rtn.
* 11-08-91 patrickh     move GDI init here from DLL init routine.
* 12-10-90 sMeans       Created.
\***************************************************************************/

NTSTATUS UserServerDllInitialization(
    PCSR_SERVER_DLL psrvdll)
{
    CLIENT_ID ClientId;
    BOOL bAllocated;
    NTSTATUS Status;

    /*
     * Initialize the RIP flags to default
     */
    gdwRIPFlags = RIPF_DEFAULT;

#if DBG
    if (RtlGetNtGlobalFlags() & FLG_SHOW_LDR_SNAPS) {
        RIPMSG0(RIP_WARNING,
                "UserServerDllInitialization: entered");
    }
#endif

    /*
     * Initialize a critical section structure that will be used to protect
     * all of the User Server's critical sections (except a few special
     * cases like the RIT -- see below).
     */

    Status = RtlInitializeCriticalSection(&gcsUserSrv);
    if (!NT_SUCCESS(Status))
    {
        RIPMSG1(RIP_WARNING,
                "UserServerDllInitialization: InitializeCriticalSection failed with Status %x",
                Status);
        return Status;
    }
    EnterCrit();

    /*
     * Remember WINSRV.DLL's hmodule so we can grab resources from it later.
     */
    ghModuleWin = psrvdll->ModuleHandle;

    psrvdll->ApiNumberBase = USERSRV_FIRST_API_NUMBER;
    psrvdll->MaxApiNumber = UserpMaxApiNumber;
    psrvdll->ApiDispatchTable = UserServerApiDispatchTable;

    if (ISTS()) {
        UserServerApiServerValidTable[0] = TRUE; // for ExitWindowsEx
    }

    psrvdll->ApiServerValidTable = UserServerApiServerValidTable;
#if DBG
    psrvdll->ApiNameTable = UserServerApiNameTable;
#else
    psrvdll->ApiNameTable = NULL;
#endif
    psrvdll->ConnectRoutine         = UserClientConnect;
    psrvdll->HardErrorRoutine       = UserHardError;
    psrvdll->ShutdownProcessRoutine = UserClientShutdown;

    /*
     * Create these events used by shutdown
     */
    //BUGBUG we should test the return code of NtCreateEvent
    NtCreateEvent(&gheventCancel, EVENT_ALL_ACCESS, NULL,
                  NotificationEvent, FALSE);
    NtCreateEvent(&gheventCancelled, EVENT_ALL_ACCESS, NULL,
                  NotificationEvent, FALSE);

    /*
     * Create the event used by the power request code.
     */
    NtCreateEvent(&ghPowerRequestEvent, EVENT_ALL_ACCESS, NULL,
                  SynchronizationEvent, FALSE);

    /*
     * Create the event used by the media change code.
     */
    NtCreateEvent(&ghMediaRequestEvent, EVENT_ALL_ACCESS, NULL,
                  SynchronizationEvent, FALSE);

    /*
     * Tell the base what user address to call when it is creating a process
     * (but before the process starts running).
     */
    BaseSetProcessCreateNotify(NtUserNotifyProcessCreate);

    /*
     * Load some strings.
     */
    gpwszaSUCCESS            = (PWSTR)RtlLoadStringOrError(ghModuleWin,
                                STR_SUCCESS, NULL, &bAllocated, FALSE);
    gpwszaSYSTEM_INFORMATION = (PWSTR)RtlLoadStringOrError(ghModuleWin,
                                STR_SYSTEM_INFORMATION, NULL, &bAllocated, FALSE);
    gpwszaSYSTEM_WARNING     = (PWSTR)RtlLoadStringOrError(ghModuleWin,
                                STR_SYSTEM_WARNING, NULL, &bAllocated, FALSE);
    gpwszaSYSTEM_ERROR       = (PWSTR)RtlLoadStringOrError(ghModuleWin,
                                STR_SYSTEM_ERROR, NULL, &bAllocated, FALSE);
    /*
     * Initialize USER
     */

    {
        HANDLE hModBase;

        hModBase = GetModuleHandle(TEXT("kernel32"));
        UserAssert(hModBase);
        gpfnAttachRoutine = GetProcAddress(hModBase,"BaseAttachCompleteThunk");
        UserAssert(gpfnAttachRoutine);

        Status = NtUserInitialize(USERCURRENTVERSION, ghPowerRequestEvent, ghMediaRequestEvent);
        if (!NT_SUCCESS(Status)) {
            RIPMSG1(RIP_WARNING,
                    "UserServerDllInitialization: NtUserInitialize failed with Status %x",
                    Status);
            goto ExitUserInit;
        }
    }

    if (ISTS()) {

        Status = WinStationAPIInit();
        if (!NT_SUCCESS(Status)) {
            RIPMSG1(RIP_WARNING,
                    "UserServerDllInitialization: WinStationAPIInit failed with Status %x",
                    Status);
            goto ExitUserInit;
        }
    }

    /*
     * Start registry notification thread
     */
    Status = RtlCreateUserThread(NtCurrentProcess(), NULL, FALSE, 0, 0, 4*0x1000,
            (PUSER_THREAD_START_ROUTINE)NotificationThread, NULL, &hThreadNotification,
            &ClientId);
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING,
                "UserServerDllInitialization: RtlCreateUserThread failed with Status %x",
                Status);
    }
    CsrAddStaticServerThread(hThreadNotification, &ClientId, 0);

ExitUserInit:
    LeaveCrit();
    return Status;
}

/**************************************************************************\
* UserClientConnect
*
* This function is called once for each client process that connects to the
* User server.  When the client dynlinks to USER.DLL, USER.DLL's init code
* is executed and calls CsrClientConnectToServer to establish the connection.
* The server portion of ConnectToServer calls out this entrypoint.
*
* UserClientConnect first verifies version numbers to make sure the client
* is compatible with this server and then completes all process-specific
* initialization.
*
* History:
* 02-??-91 SMeans       Created.
* 04-02-91 DarrinM      Added User intialization code.
\**************************************************************************/

extern WORD gDispatchTableValues;

NTSTATUS UserClientConnect(
    PCSR_PROCESS Process,
    PVOID ConnectionInformation,
    PULONG pulConnectionLen)
{
    /*
     * Pass the api port to the kernel.  Do this early so the kernel
     * can send a datagram to CSR to activate a debugger.
     */
    if (CsrApiPort == NULL) {
        CsrApiPort = CsrQueryApiPort();
        NtUserSetInformationThread(
                NtCurrentThread(),
                UserThreadCsrApiPort,
                &CsrApiPort,
                sizeof(HANDLE));
    }

    UserAssert(*pulConnectionLen == sizeof(USERCONNECT));
    if (*pulConnectionLen != sizeof(USERCONNECT)) {
        return STATUS_INVALID_PARAMETER;
    }

    ((PUSERCONNECT)ConnectionInformation)->dwDispatchCount = gDispatchTableValues;
    return NtUserProcessConnect(Process->ProcessHandle,
            (PUSERCONNECT)ConnectionInformation, *pulConnectionLen);
}


VOID
RegReadApcProcedure(
    PVOID RegReadApcContext,
    PIO_STATUS_BLOCK IoStatus
    )
{
    UNICODE_STRING ValueString;
    LONG Status;
    BYTE Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    DWORD cbSize;
    ULONG l;

    UNREFERENCED_PARAMETER(RegReadApcContext);
    UNREFERENCED_PARAMETER(IoStatus);

    RtlInitUnicodeString(&ValueString, L"Win32PrioritySeparation");
    Status = NtQueryValueKey(hKeyPriority,
            &ValueString,
            KeyValuePartialInformation,
            (PKEY_VALUE_PARTIAL_INFORMATION)Buf,
            sizeof(Buf),
            &cbSize);
    if (NT_SUCCESS(Status)) {
        l = *((PDWORD)((PKEY_VALUE_PARTIAL_INFORMATION)Buf)->Data);
    } else {
        l = PROCESS_PRIORITY_SEPARATION_MAX;  // last resort default
    }

    NtSetSystemInformation(SystemPrioritySeperation,&l,sizeof(ULONG));

    NtNotifyChangeKey(
        hKeyPriority,
        NULL,
        (PIO_APC_ROUTINE)RegReadApcProcedure,
        NULL,
        &IoStatusRegChange,
        REG_NOTIFY_CHANGE_LAST_SET,
        FALSE,
        &RegChangeBuffer,
        sizeof(RegChangeBuffer),
        TRUE
        );
}

VOID
StartRegReadRead(VOID)
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES OA;

    RtlInitUnicodeString(&UnicodeString,
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\PriorityControl");
    InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (!NT_SUCCESS(NtOpenKey(&hKeyPriority, KEY_READ | KEY_NOTIFY, &OA)))
        UserAssert(FALSE);

    RegReadApcProcedure(NULL, NULL);
}


/***************************************************************************\
* HandleMediaChangeEvent
*
* This routine is responsible for broadcasting the WM_DEVICECHANGE message
* when media arrives or is removed from a CD-ROM device.
*
* History:
* 23-Feb-96     BradG       Modified to handle event per CD-ROM device
* 23-April-96   Salimc      Some CD-ROM drives notify us that media has
*                           arrived before the drive has recognized that it had
*                           new media. The call to DeviceIoctl() will fail in
*                           this case. To fix this we made the following changes
*
*                           aDriveState is an array of tri-state global variable
*                           for each drive.Each variable starts off in an UNKNOWN
*                           state and on the first event with any drive we do the
*                           full MAX_TRIES or less CHECK_VERIFY's which then gets
*                           us into either a INSERTED or EJECTED state. From then
*                           on we know that each new event is going to be the
*                           opposite of what we currently have.
*
*                           UNKNOWN => do upto MAX_TRIES CHECK_VERIFY's with
*                           delay to get into EJECTED or INSERTED state.
*
*                           INSERTED => do 1 CHECK_VERIFY to get into
*                           EJECTED state
*
*                           EJECTED => do upto MAX_TRIES CHECK_VERIFY's with
*                           delay to get into INSERTED state
*
\***************************************************************************/

VOID HandleMediaChangeEvent(VOID)
{
    /*
     * Local variables
     */

    DWORD                   dwRecipients;
    BOOL                    bResult;
    NTSTATUS                Status;
    DEV_BROADCAST_VOLUME    dbcvInfo;
    USERTHREAD_USEDESKTOPINFO utudi;

    ULONG cDrive;

    while (cDrive = (ULONG)NtUserCallNoParam(SFI_GETDEVICECHANGEINFO)) {

        /*
         * Determine if it's an arrival or removal
         */
        bResult = (cDrive & 0x80000000);

        /*
         * Initialize the structures used for BroadcastSystemMessage
         */
        dbcvInfo.dbcv_size = sizeof(dbcvInfo);
        dbcvInfo.dbcv_devicetype = DBT_DEVTYP_VOLUME;
        dbcvInfo.dbcv_reserved = 0;
        dbcvInfo.dbcv_flags = DBTF_MEDIA;
        dbcvInfo.dbcv_unitmask = cDrive;

        dwRecipients = BSM_ALLCOMPONENTS | BSM_ALLDESKTOPS;

        /*
         * Temporarily we must assign this thread to a desktop so we can
         * call USER's BroascastSystemMessage() routine.  We call the
         * private SetThreadDesktopToDefault() to assign ourselves to the
         * desktop that is currently receiving input.
         */
        utudi.hThread = NULL;
        utudi.drdRestore.pdeskRestore = NULL;
        Status = NtUserSetInformationThread(NtCurrentThread(),
                                            UserThreadUseActiveDesktop,
                                            &utudi, sizeof(utudi));
        if (NT_SUCCESS(Status)) {
            /*
             * Broadcast the message
             */
            BroadcastSystemMessage(BSF_FORCEIFHUNG | ((bResult) ? BSF_ALLOWSFW : 0),
                                   &dwRecipients,
                                   WM_DEVICECHANGE,
// HACK: need to or 0x8000 in wParam
//       because this is a flag to let
//       BSM know that lParam is a pointer
//       to a data structure.
                                   0x8000 | ((bResult) ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE),
                                   (LPARAM)&dbcvInfo);

            /*
             * Set our thread's desktop back to NULL.  This will decrement
             * the desktop's reference count.
             */
            NtUserSetInformationThread(NtCurrentThread(),
                                       UserThreadUseDesktop,
                                       &utudi,
                                       sizeof(utudi));
        }
    }
}

DWORD
GetNetworkDrives(
    )
/*++

Routine Description:

    Returns a drive bitmask similar to GetLogicalDrives, but including
    only the network drives.

Arguments:

Return Value:


--*/
{
    DWORD Mask = 0;
    DWORD DriveNumber;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

    if (NT_SUCCESS(NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessDeviceMap,
                                        &ProcessDeviceMapInfo.Query,
                                        sizeof( ProcessDeviceMapInfo.Query ),
                                        NULL
                                      ))) {
        // For all the drives from C to Z
        for (DriveNumber = 2; DriveNumber < 26; DriveNumber++)
        {
            if (ProcessDeviceMapInfo.Query.DriveType[DriveNumber] == DOSDEVICE_DRIVE_REMOTE)
            {
                Mask |= (1 << DriveNumber);
            }
        }
    }

    return Mask;
}

VOID
HandleRemoteNetDeviceChangeEvent(
    )
/*++

Routine Description:


Arguments:

Return Value:

--*/
{
    DWORD    NetDrives;
    DEV_BROADCAST_VOLUME dbv;
    LONG status;
    USERTHREAD_USEDESKTOPINFO utudi;



    /*
     * Temporarily we must assign this thread to a desktop so we can
     * call USER's BroascastSystemMessage() routine.  We call the
     * private SetThreadDesktopToDefault() to assign ourselves to the
     * desktop that is currently receiving input.
     */
    utudi.hThread = NULL;
    utudi.drdRestore.pdeskRestore = NULL;
    status = NtUserSetInformationThread(NtCurrentThread(),
                                        UserThreadUseActiveDesktop,
                                        &utudi, sizeof(utudi));
    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Keep broadcasting until the set of net drives stops changing
    //
    for (;;)
    {

        //
        // Get the current net drive bitmask and compare against the net
        // drive bitmask when we last broadcast
        //
        NetDrives = GetNetworkDrives();

        if (NetDrives == LastNetDrives)
        {
            break;
        }

        //
        // Broadcast about deleted volumes
        //
        dbv.dbcv_size       = sizeof(dbv);
        dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
        dbv.dbcv_reserved   = 0;
        dbv.dbcv_unitmask   = LastNetDrives & ~NetDrives;
        dbv.dbcv_flags      = DBTF_NET;
        if (dbv.dbcv_unitmask != 0)
        {
            DWORD dwRec = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
            status = BroadcastSystemMessage(
                        BSF_FORCEIFHUNG | BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
                        &dwRec,
                        WM_DEVICECHANGE,
                        (WPARAM) DBT_DEVICEREMOVECOMPLETE,
                        (LPARAM)(DEV_BROADCAST_HDR*)(&dbv)
                        );

        }

        //
        // Broadcast about added volumes
        //
        dbv.dbcv_unitmask   = NetDrives & ~LastNetDrives;
        if (dbv.dbcv_unitmask != 0)
        {
            DWORD dwRec = BSM_APPLICATIONS | BSM_ALLDESKTOPS;

            status = BroadcastSystemMessage(
                        BSF_FORCEIFHUNG | BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
                        &dwRec,
                        WM_DEVICECHANGE,
                        (WPARAM) DBT_DEVICEARRIVAL,
                        (LPARAM)(DEV_BROADCAST_HDR*)(&dbv)
                        );


        }

        //
        // Remember the drive set that we last broadcast about
        //
        LastNetDrives = NetDrives;

        //
        // Go around the loop again to detect changes that may have occurred
        // while we were broadcasting
        //
    }

    /*
     * Set our thread's desktop back to NULL.  This will decrement
     * the desktop's reference count.
     */
    NtUserSetInformationThread(NtCurrentThread(),
                               UserThreadUseDesktop,
                               &utudi,
                               sizeof(utudi));

    return;
}

BOOL
CreateBSMEventSD(
    PSECURITY_DESCRIPTOR * SecurityDescriptor
    )
/*++

Routine Description:

    This function creates a security descriptor for the BSM request event.
    It grants EVENT_ALL_ACCESS to local system and EVENT_MODIFY_STATE access
    to the rest of the world.  This prevents principals other than local
    system from waiting for the event.

Arguments:

    SecurityDescriptor - Receives a pointer to the new security descriptor.
        Should be freed with LocalFree.

Return Value:

    TRUE - success

    FALSE - failure, use GetLastError


--*/
{
    NTSTATUS    Status;
    ULONG       AclLength;
    PACL        EventDacl;
    PSID        WorldSid = NULL;
    PSID        SystemSid = NULL;
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    BOOL        retval = TRUE;

    *SecurityDescriptor = NULL;

    Status = RtlAllocateAndInitializeSid( &NtSidAuthority,
                                          1,
                                          SECURITY_LOCAL_SYSTEM_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &SystemSid );

    if (!NT_SUCCESS(Status)) {
        retval = FALSE;
        goto Cleanup;
    }

    Status = RtlAllocateAndInitializeSid( &WorldSidAuthority,
                                          1,
                                          SECURITY_WORLD_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &WorldSid );

    if (!NT_SUCCESS(Status)) {
        retval = FALSE;
        goto Cleanup;
    }


    //
    // Allocate a buffer to contain the SD followed by the DACL
    // Note, the well-known SIDs are expected to have been created
    // by this time
    //

    AclLength = (ULONG)sizeof(ACL) +
                   (2*((ULONG)sizeof(ACCESS_ALLOWED_ACE))) +
                   RtlLengthSid( SystemSid ) +
                   RtlLengthSid( WorldSid ) +
                   8;       // 8 is for good measure

    *SecurityDescriptor = (PSECURITY_DESCRIPTOR)
        LocalAlloc( 0, SECURITY_DESCRIPTOR_MIN_LENGTH + AclLength );

    if (*SecurityDescriptor == NULL) {
        retval = FALSE;
        goto Cleanup;
    }

    EventDacl = (PACL) ((BYTE*)(*SecurityDescriptor) + SECURITY_DESCRIPTOR_MIN_LENGTH);


    //
    // Set up a default ACL
    //
    //    Public: WORLD:EVENT_MODIFY_STATE, SYSTEM:all

    Status = RtlCreateAcl( EventDacl, AclLength, ACL_REVISION2);
    if (!NT_SUCCESS(Status)) {
        retval = FALSE;
        goto Cleanup;
    }


    //
    // WORLD access
    //

    Status = RtlAddAccessAllowedAce (
                 EventDacl,
                 ACL_REVISION2,
                 EVENT_MODIFY_STATE,
                 WorldSid
                 );
    if (!NT_SUCCESS(Status)) {
        retval = FALSE;
        goto Cleanup;
    }


    //
    // SYSTEM access
    //

    Status = RtlAddAccessAllowedAce (
                 EventDacl,
                 ACL_REVISION2,
                 EVENT_ALL_ACCESS,
                 SystemSid
                 );
    if (!NT_SUCCESS(Status)) {
        retval = FALSE;
        goto Cleanup;
    }



    //
    // Now initialize security descriptors
    // that export this protection
    //

    Status = RtlCreateSecurityDescriptor(
                 *SecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    if (!NT_SUCCESS(Status)) {
        retval = FALSE;
        goto Cleanup;
    }

    Status = RtlSetDaclSecurityDescriptor(
                 *SecurityDescriptor,
                 TRUE,                       // DaclPresent
                 EventDacl,
                 FALSE                       // DaclDefaulted
                 );

    if (!NT_SUCCESS(Status)) {
        retval = FALSE;
        goto Cleanup;
    }

Cleanup:

    if (WorldSid) {
        RtlFreeSid(WorldSid);
    }

    if (SystemSid) {
        RtlFreeSid(SystemSid);
    }

    if ((retval == FALSE) && (*SecurityDescriptor != NULL)) {
        LocalFree(*SecurityDescriptor);
        *SecurityDescriptor = NULL;
    }


    return retval;
}




VOID NotificationThread(
    PVOID pJunk)
{
    KPRIORITY   Priority;
    NTSTATUS    Status;
    HANDLE      hEvent[ID_NUM_EVENTS];
    WCHAR       szObjectStr[MAX_SESSION_PATH];
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING UnicodeString;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ULONG       NumEvents = ID_NUM_EVENTS;

    UNREFERENCED_PARAMETER(pJunk);

    try {

        /*
         * Set the priority of the RIT to 3.
         */
        Priority = LOW_PRIORITY + 3;
        NtSetInformationThread(hThreadNotification, ThreadPriority, &Priority,
                sizeof(KPRIORITY));

        /*
         * Setup the NLS event
         */
        NtCreateEvent(&ghNlsEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
        UserAssert( ghNlsEvent != NULL );
        hEvent[ID_NLS] = ghNlsEvent;

        /*
         * Setup the power request event
         */
        hEvent[ID_POWER] = ghPowerRequestEvent;

        /*
         * Setup the MediaChangeEvent
         */
        hEvent[ID_MEDIACHANGE] = ghMediaRequestEvent;


        /*
         *  Setup the NetDeviceChange Event
         */

        if (gSessionId != 0) {
           //
           // Only on remote Session
           //


            swprintf(szObjectStr,L"%ws\\%ld\\BaseNamedObjects\\%ws",
                                                     SESSION_ROOT,gSessionId,SC_BSM_EVENT_NAME);

            RtlInitUnicodeString(&UnicodeString, szObjectStr);

            if (CreateBSMEventSD(&pSD) ) {

                InitializeObjectAttributes(&Attributes,
                                           &UnicodeString,
                                           OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                           NULL,
                                           pSD);

                if (!NT_SUCCESS(NtCreateEvent(&hEvent[ID_NETDEVCHANGE], EVENT_ALL_ACCESS, &Attributes, SynchronizationEvent, FALSE))) {

                    NumEvents--;

                }

                LocalFree(pSD);
            } else {

                NumEvents--;
            }

        } else {
            //
            // On Console we don't wait for NetDeviceEvent
            //
            NumEvents--;
        }

        /*
         * Only want media change events on the console.
         */
        if (gSessionId == 0) {
        }

        StartRegReadRead();

        /*
         * Sit and wait forever.
         */
        while (TRUE) {
            Status = NtWaitForMultipleObjects(NumEvents,
                                              hEvent,
                                              WaitAny,
                                              TRUE,
                                              NULL);


            if (Status == ID_NLS + WAIT_OBJECT_0) {

                /*
                 * Handle the NLS event
                 */
                if (gfLogon) {
                    gfLogon = FALSE;
                    BaseSrvNlsUpdateRegistryCache(NULL, NULL);
                }

            }
            else if (Status == ID_POWER + WAIT_OBJECT_0) {

                /*
                 * Handle the power request event
                 */
                NtUserCallNoParam(SFI_XXXUSERPOWERCALLOUTWORKER);

            }
            else if (Status == ID_MEDIACHANGE + WAIT_OBJECT_0) {

                /*
                 * Handle the media change event
                 */
                HandleMediaChangeEvent();

                NtResetEvent(hEvent[ID_MEDIACHANGE], NULL);
            }
            else if (Status == ID_NETDEVCHANGE + WAIT_OBJECT_0) {

                /*
                 * Handle the NetDevice change event for remote sessions
                 */
                HandleRemoteNetDeviceChangeEvent();

            }


        } // While (TRUE)

    } except (CsrUnhandledExceptionFilter(GetExceptionInformation())) {
        KdPrint(("Registry notification thread is dead, sorry.\n"));
    }
}


UINT GetRegIntFromID(
    HKEY hKey,
    int KeyID,
    UINT nDefault)
{
    LPWSTR lpszValue;
    BOOL fAllocated;
    UNICODE_STRING Value;
    DWORD cbSize;
    BYTE Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 20 * sizeof(WCHAR)];
    NTSTATUS Status;
    UINT ReturnValue;

    lpszValue = (LPWSTR)RtlLoadStringOrError(ghModuleWin,
            KeyID, NULL, &fAllocated, FALSE);

    RtlInitUnicodeString(&Value, lpszValue);
    Status = NtQueryValueKey(hKey,
            &Value,
            KeyValuePartialInformation,
            (PKEY_VALUE_PARTIAL_INFORMATION)Buf,
            sizeof(Buf),
            &cbSize);
    if (NT_SUCCESS(Status)) {

        /*
         * Convert string to int.
         */
        RtlInitUnicodeString(&Value, (LPWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)Buf)->Data);
        RtlUnicodeStringToInteger(&Value, 10, &ReturnValue);
    } else {
        ReturnValue = nDefault;
    }

    LocalFree(lpszValue);

    return(ReturnValue);
}

VOID GetTimeouts(VOID)
{
    HANDLE hCurrentUserKey;
    HANDLE hKey;
    NTSTATUS Status;

    Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hCurrentUserKey);
    if (NT_SUCCESS(Status)) {
        Status = MyRegOpenKey(hCurrentUserKey,
                L"Control Panel\\Desktop",
                &hKey);
        if (NT_SUCCESS(Status)) {
            gCmsHungAppTimeout = GetRegIntFromID(
                    hKey,
                    STR_CMSHUNGAPPTIMEOUT,
                    gCmsHungAppTimeout);
            gCmsWaitToKillTimeout = GetRegIntFromID(
                    hKey,
                    STR_CMSWAITTOKILLTIMEOUT,
                    gCmsWaitToKillTimeout);

            gdwHungToKillCount = gCmsWaitToKillTimeout / gCmsHungAppTimeout;

            gfAutoEndTask = GetRegIntFromID(
                    hKey,
                    STR_AUTOENDTASK,
                    gfAutoEndTask);
            NtClose(hKey);
        }
        NtClose(hCurrentUserKey);
    }

    Status = MyRegOpenKey(NULL,
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control",
            &hKey);
    if (NT_SUCCESS(Status)) {
        gdwServicesWaitToKillTimeout = GetRegIntFromID(
                hKey,
                STR_WAITTOKILLSERVICETIMEOUT,
                gCmsWaitToKillTimeout);
        gdwProcessTerminateTimeout = GetRegIntFromID(
                hKey,
                STR_PROCESSTERMINATETIMEOUT,
                PROCESSTERMINATETIMEOUT);
		if (gdwProcessTerminateTimeout < CMSHUNGAPPTIMEOUT) {
			gdwProcessTerminateTimeout = CMSHUNGAPPTIMEOUT;
		}

        NtClose(hKey);
    }
}

ULONG
SrvLogon(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PLOGONMSG a = (PLOGONMSG)&m->u.ApiMessageData;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ReplyStatus);

    if (!CsrImpersonateClient(NULL))
        return (ULONG)STATUS_UNSUCCESSFUL;

    if (a->fLogon) {

        /*
         * Flush the MultiLingual UI (MUI) alternate
         * resource modules from within NTDLL, so that the 
         * new user logging-on gets his chance to 
         * load his own.
         */
        LdrFlushAlternateResourceModules();

        /*
         *  Take care of NLS cache for LogON.
         */
        BaseSrvNlsLogon(TRUE);

        /*
         *  Set the cleanup event so that the RIT can handle the NLS
         *  registry notification.
         */
        gfLogon = TRUE;
        Status = NtSetEvent( ghNlsEvent, NULL );
        ASSERT(NT_SUCCESS(Status));
    } else {

        /*
         *  Take care of NLS cache for LogOFF.
         */
        BaseSrvNlsLogon(FALSE);
    }

    /*
     * Get timeout values from registry
     */
    GetTimeouts();

    CsrRevertToSelf();

    /*
     * Initialize console attributes
     */
    InitializeConsoleAttributes();

    return (ULONG)STATUS_SUCCESS;
}

ULONG
SrvRegisterLogonProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    /*
     * Fail if this is not the first call.
     */
    if (gIdLogon != 0) {
        return (ULONG)STATUS_UNSUCCESSFUL;
    }

    gIdLogon = *(DWORD*)m->u.ApiMessageData;

    UNREFERENCED_PARAMETER(ReplyStatus);

    return (ULONG)STATUS_SUCCESS;
}

ULONG
SrvWin32HeapFail(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
#if DBG
    PWIN32HEAPFAILMSG a = (PWIN32HEAPFAILMSG)&m->u.ApiMessageData;

    Win32HeapFailAllocations(a->bFail);

#endif
    return (ULONG)STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(m);
    UNREFERENCED_PARAMETER(ReplyStatus);
}

ULONG
SrvWin32HeapStat(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
#if DBG
    extern DWORD Win32HeapStat(PDBGHEAPSTAT phs, DWORD dwLen, BOOL bNeedTagShift);

    PWIN32HEAPSTATMSG a = (PWIN32HEAPSTATMSG)&m->u.ApiMessageData;

    if (!CsrValidateMessageBuffer(m, &a->phs, a->dwLen, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }
    a->dwMaxTag = Win32HeapStat(a->phs, a->dwLen, TRUE);

#endif
    return (ULONG)STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(m);
    UNREFERENCED_PARAMETER(ReplyStatus);
}

ULONG
SrvGetThreadConsoleDesktop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PGETTHREADCONSOLEDESKTOPMSG a = (PGETTHREADCONSOLEDESKTOPMSG)&m->u.ApiMessageData;

    UNREFERENCED_PARAMETER(ReplyStatus);

    return GetThreadConsoleDesktop(a->dwThreadId, &a->hdeskConsole);
}

/***************************************************************************\
* FindWindowFromThread
*
* This is a callback function passed to EnumThreadWindows by
*  to find a top level window owned by a given thread;
*  ideally, we want to find a top level owner.
*
* 07/18/96  GerardoB  Created
\***************************************************************************/
BOOL CALLBACK FindWindowFromThread (HWND hwnd, LPARAM lParam)
{
    BOOL fTopLevelOwner;
#ifdef FE_IME
    if ( IsImeWindow(hwnd) ) {
        return TRUE;
    }
#endif

    fTopLevelOwner = (GetWindow(hwnd, GW_OWNER) == NULL);
    if ((*((HWND *)lParam) == NULL) || fTopLevelOwner) {
        *((HWND *)lParam) = hwnd;
    }
    return !fTopLevelOwner;
}

DWORD GetRipComponent(VOID) { return RIP_USERSRV; }

DWORD GetDbgTagFlags(int tag)
{
    return 0;
    UNREFERENCED_PARAMETER(tag);
}

DWORD GetRipPID(VOID) { return 0; }
DWORD GetRipFlags(VOID) { return gdwRIPFlags; }

VOID SetRipFlags(DWORD dwRipFlags, DWORD dwRipPID)
{
    if ((dwRipFlags != (DWORD)-1) && !(dwRipFlags & ~RIPF_VALIDUSERFLAGS)) {
        gdwRIPFlags = (WORD)((gdwRIPFlags & ~RIPF_VALIDUSERFLAGS) | dwRipFlags);
    }
    UNREFERENCED_PARAMETER(dwRipPID);
}

VOID SetDbgTag(int tag, DWORD dwBitFlags)
{
    UNREFERENCED_PARAMETER(tag);
    UNREFERENCED_PARAMETER(dwBitFlags);
}

VOID UserRtlRaiseStatus(NTSTATUS Status)
{
    RtlRaiseStatus(Status);
}

/*
 * We get this warning if we don't explicitly initalize gZero:
 *
 * C4132: 'gZero' : const object should be initialized
 *
 * But we can't explicitly initialize it since it is a union. So
 * we turn the warning off.
 */
#pragma warning(disable:4132)
CONST ALWAYSZERO gZero;
#pragma warning(default:4132)
