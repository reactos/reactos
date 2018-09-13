/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module contains the initialization code for the Client-Server (CS)
    Client DLL.

Author:

    Steve Wood (stevewo) 8-Oct-1990

Environment:

    User Mode only

Revision History:

--*/

#include "csrdll.h"
#include "ldrp.h"

BOOLEAN
CsrDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function is the DLL initialization routine for the Client DLL
    This function gets control when the application links to this DLL
    are snapped.

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

Return Value:

    Status value.

--*/

{
    Context;

    if (Reason != DLL_PROCESS_ATTACH) {
        return( TRUE );
        }

    //
    // Remember our DLL handle in a global variable.
    //

    CsrDllHandle = DllHandle;

    return( TRUE );
}


NTSTATUS
CsrOneTimeInitialize( VOID )
{
    NTSTATUS Status;

    //
    // Save away system information in a global variable
    //

    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       &CsrNtSysInfo,
                                       sizeof( CsrNtSysInfo ),
                                       NULL
                                     );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    //
    // Use the process heap for memory allocation.
    //

    CsrHeap = RtlProcessHeap();

    CsrInitOnceDone = TRUE;

    return( STATUS_SUCCESS );
}


NTSTATUS
CsrClientConnectToServer(
    IN PWSTR ObjectDirectory,
    IN ULONG ServerDllIndex,
    IN PCSR_CALLBACK_INFO CallbackInformation OPTIONAL,
    IN PVOID ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    )

/*++

Routine Description:

    This function is called by the client side DLL to connect with its
    server side DLL.

Arguments:

    ObjectDirectory - Points to a null terminate string that is the same
        as the value of the ObjectDirectory= argument passed to the CSRSS
        program.

    ServerDllIndex - Index of the server DLL that is being connected to.
        It should match one of the ServerDll= arguments passed to the CSRSS
        program.

    CallbackInformation - An optional pointer to a structure that contains
        a pointer to the client callback function dispatch table.

    ConnectionInformation - An optional pointer to uninterpreted data.
        This data is intended for clients to pass package, version and
        protocol identification information to the server to allow the
        server to determine if it can satisify the client before
        accepting the connection.  Upon return to the client, the
        ConnectionInformation data block contains any information passed
        back from the server DLL by its call to the
        CsrCompleteConnection call.  The output data overwrites the
        input data.

    ConnectionInformationLength - Pointer to the length of the
        ConnectionInformation data block.  The output value is the
        length of the data stored in the ConnectionInformation data
        block by the server's call to the NtCompleteConnectPort
        service.  This parameter is OPTIONAL only if the
        ConnectionInformation parameter is NULL, otherwise it is
        required.

    CalledFromServer - On output, TRUE if the dll has been called from
        a server process.

Return Value:

    Status value.

--*/

{
    NTSTATUS Status;
    CSR_API_MSG m;
    PCSR_CLIENTCONNECT_MSG a = &m.u.ClientConnect;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    HANDLE CsrServerModuleHandle;
    STRING ProcedureName;
    ANSI_STRING DllName;
    UNICODE_STRING DllName_U;
    PIMAGE_NT_HEADERS NtHeaders;

    if (ARGUMENT_PRESENT( ConnectionInformation ) &&
        (!ARGUMENT_PRESENT( ConnectionInformationLength ) ||
          *ConnectionInformationLength == 0
        )
       ) {
        return( STATUS_INVALID_PARAMETER );
        }

    if (!CsrInitOnceDone) {
        Status = CsrOneTimeInitialize();
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }
        }

    if (ARGUMENT_PRESENT( CallbackInformation )) {
        CsrLoadedClientDll[ ServerDllIndex ] = RtlAllocateHeap( CsrHeap, MAKE_TAG( CSR_TAG ), sizeof(CSR_CALLBACK_INFO) );
        if ( !CsrLoadedClientDll[ ServerDllIndex ] ) {
            return STATUS_NO_MEMORY;
            }
        CsrLoadedClientDll[ ServerDllIndex ]->ApiNumberBase =
                CallbackInformation->ApiNumberBase;
        CsrLoadedClientDll[ ServerDllIndex ]->MaxApiNumber =
                CallbackInformation->MaxApiNumber;
        CsrLoadedClientDll[ ServerDllIndex ]->CallbackDispatchTable =
                CallbackInformation->CallbackDispatchTable;
    }

    //
    // if we are being called by a server process, skip lpc port initialization
    // and call to server connect routine and just initialize heap.  the
    // dll initialization routine will do any necessary initialization.  this
    // stuff only needs to be done for the first connect.
    //

    if ( CsrServerProcess == TRUE ) {
        *CalledFromServer = CsrServerProcess;
        return STATUS_SUCCESS;
        }

    //
    // If the image is an NT Native image, we are running in the
    // context of the server.
    //

    NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
    CsrServerProcess =
        (NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE) ? TRUE : FALSE;

    if ( CsrServerProcess ) {
        extern PVOID NtDllBase;
        RtlInitAnsiString( &DllName, "csrsrv" );
        Status = RtlAnsiStringToUnicodeString(&DllName_U, &DllName, TRUE);
        ASSERT(NT_SUCCESS(Status));

        LdrDisableThreadCalloutsForDll(NtDllBase);

        Status = LdrGetDllHandle(
        		UNICODE_NULL,
                    NULL,
        		&DllName_U,
                    (PVOID *)&CsrServerModuleHandle
                    );

        RtlFreeUnicodeString(&DllName_U);

        CsrServerProcess = TRUE;

        RtlInitString(&ProcedureName,"CsrCallServerFromServer");
        Status = LdrGetProcedureAddress(
                        CsrServerModuleHandle,
                        &ProcedureName,
                        0L,
                        (PVOID *)&CsrServerApiRoutine
                        );
        ASSERT(NT_SUCCESS(Status));

        ASSERT (CsrPortHeap==NULL);
        CsrPortHeap = RtlProcessHeap();

        CsrPortBaseTag = RtlCreateTagHeap( CsrPortHeap,
                                           0,
                                           L"CSRPORT!",
                                           L"CAPTURE\0"
                                         );

        if (ARGUMENT_PRESENT(CalledFromServer)) {
            *CalledFromServer = CsrServerProcess;
            }
        return STATUS_SUCCESS;
        }

    if ( ARGUMENT_PRESENT(ConnectionInformation) ) {
        CsrServerProcess = FALSE;
        if (CsrPortHandle == NULL) {
            Status = CsrpConnectToServer( ObjectDirectory );
            if (!NT_SUCCESS( Status )) {
                return( Status );
                }
            }

        a->ServerDllIndex = ServerDllIndex;
        a->ConnectionInformationLength = *ConnectionInformationLength;
        if (ARGUMENT_PRESENT( ConnectionInformation )) {
            CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                      a->ConnectionInformationLength
                                                    );
            if (CaptureBuffer == NULL) {
                return( STATUS_NO_MEMORY );
                }

            CsrAllocateMessagePointer( CaptureBuffer,
                                       a->ConnectionInformationLength,
                                       (PVOID *)&a->ConnectionInformation
                                     );
            RtlMoveMemory( a->ConnectionInformation,
                           ConnectionInformation,
                           a->ConnectionInformationLength
                         );

            *ConnectionInformationLength = a->ConnectionInformationLength;
            }
        else {
            CaptureBuffer = NULL;
            }

        Status = CsrClientCallServer( &m,
                                      CaptureBuffer,
                                      CSR_MAKE_API_NUMBER( CSRSRV_SERVERDLL_INDEX,
                                                           CsrpClientConnect
                                                         ),
                                      sizeof( *a )
                                    );

        if (CaptureBuffer != NULL) {
            if (ARGUMENT_PRESENT( ConnectionInformation )) {
                RtlMoveMemory( ConnectionInformation,
                               a->ConnectionInformation,
                               *ConnectionInformationLength
                             );
                }

            CsrFreeCaptureBuffer( CaptureBuffer );
            }
        }
    else {
        Status = STATUS_SUCCESS;
        }

    if (ARGUMENT_PRESENT(CalledFromServer)) {
        *CalledFromServer = CsrServerProcess;
        }
    return( Status );
}

BOOLEAN
xProtectHandle(
    HANDLE hObject
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

    Status = NtQueryObject( hObject,
                            ObjectHandleFlagInformation,
                            &HandleInfo,
                            sizeof( HandleInfo ),
                            NULL
                          );
    if (NT_SUCCESS( Status )) {
        HandleInfo.ProtectFromClose = TRUE;

        Status = NtSetInformationObject( hObject,
                                         ObjectHandleFlagInformation,
                                         &HandleInfo,
                                         sizeof( HandleInfo )
                                       );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        }

    return FALSE;
}

NTSTATUS
CsrpConnectToServer(
    IN PWSTR ObjectDirectory
    )
{
    NTSTATUS Status;
    REMOTE_PORT_VIEW ServerView;
    ULONG MaxMessageLength;
    ULONG ConnectionInformationLength;
    CSR_API_CONNECTINFO ConnectionInformation;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    HANDLE PortSection;
    PORT_VIEW ClientView;
    ULONG n;
    LARGE_INTEGER SectionSize;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID SystemSid;

    //
    // Create the port name string by combining the passed in object directory
    // name with the port name.
    //

    n = ((wcslen( ObjectDirectory ) + 1) * sizeof( WCHAR )) +
        sizeof( CSR_API_PORT_NAME );
    CsrPortName.Length = 0;
    CsrPortName.MaximumLength = (USHORT)n;
    CsrPortName.Buffer = RtlAllocateHeap( CsrHeap, MAKE_TAG( CSR_TAG ), n );
    if (CsrPortName.Buffer == NULL) {
        return( STATUS_NO_MEMORY );
        }
    RtlAppendUnicodeToString( &CsrPortName, ObjectDirectory );
    RtlAppendUnicodeToString( &CsrPortName, L"\\" );
    RtlAppendUnicodeToString( &CsrPortName, CSR_API_PORT_NAME );

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;


    //
    // Create a section to contain the Port Memory.  Port Memory is private
    // memory that is shared between the client and server processes.
    // This allows data that is too large to fit into an API request message
    // to be passed to the server.
    //

    SectionSize.LowPart = CSR_PORT_MEMORY_SIZE;
    SectionSize.HighPart = 0;

    Status = NtCreateSection( &PortSection,
                              SECTION_ALL_ACCESS,
                              NULL,
                              &SectionSize,
                              PAGE_READWRITE,
                              SEC_RESERVE,
                              NULL
                            );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    //
    // Connect to the server.  This includes a description of the Port Memory
    // section so that the LPC connection logic can make the section visible
    // to both the client and server processes.  Also pass information the
    // server needs in the connection information structure.
    //

    ClientView.Length = sizeof( ClientView );
    ClientView.SectionHandle = PortSection;
    ClientView.SectionOffset = 0;
    ClientView.ViewSize = SectionSize.LowPart;
    ClientView.ViewBase = 0;
    ClientView.ViewRemoteBase = 0;

    ServerView.Length = sizeof( ServerView );
    ServerView.ViewSize = 0;
    ServerView.ViewBase = 0;

    ConnectionInformationLength = sizeof( ConnectionInformation );
    ConnectionInformation.ExpectedVersion = CSR_VERSION;

    SystemSid = NULL;
    Status = RtlAllocateAndInitializeSid( &NtAuthority,
                                          1,
                                          SECURITY_LOCAL_SYSTEM_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &SystemSid
                                        );
    Status = NtSecureConnectPort( &CsrPortHandle,
                          &CsrPortName,
                          &DynamicQos,
                          &ClientView,
                          SystemSid,
                          &ServerView,
                          (PULONG)&MaxMessageLength,
                          (PVOID)&ConnectionInformation,
                          (PULONG)&ConnectionInformationLength
                        );
    RtlFreeSid( SystemSid );
    NtClose( PortSection );
    if (!NT_SUCCESS( Status )) {
        IF_DEBUG {
            DbgPrint( "CSRDLL: Unable to connect to %wZ Server - Status == %X\n",
                      &CsrPortName,
                      Status
                    );
            }

        return( Status );
        }
    xProtectHandle(CsrPortHandle);

    NtCurrentPeb()->ReadOnlySharedMemoryBase = ConnectionInformation.SharedSectionBase;
    NtCurrentPeb()->ReadOnlySharedMemoryHeap = ConnectionInformation.SharedSectionHeap;
    NtCurrentPeb()->ReadOnlyStaticServerData = (PVOID *)ConnectionInformation.SharedStaticServerData;

#if DBG
    CsrDebug = ConnectionInformation.DebugFlags;
#endif
    CsrObjectDirectory = ConnectionInformation.ObjectDirectory;

    CsrPortMemoryRemoteDelta = (ULONG_PTR)ClientView.ViewRemoteBase -
                               (ULONG_PTR)ClientView.ViewBase;

    IF_CSR_DEBUG( LPC ) {
        DbgPrint( "CSRDLL: ClientView: Base=%p  RemoteBase=%p  Delta: %lX  Size=%lX\n",
                  ClientView.ViewBase,
                  ClientView.ViewRemoteBase,
                  CsrPortMemoryRemoteDelta,
                  (ULONG)ClientView.ViewSize
                );
        }

    //
    // Create a sparse heap in the share memory section.  Initially
    // commit just one page.
    //

    CsrPortHeap = RtlCreateHeap( HEAP_CLASS_8,                      // Flags
                                 ClientView.ViewBase,               // HeapBase
                                 ClientView.ViewSize,               // ReserveSize
                                 CsrNtSysInfo.PageSize,             // CommitSize
                                 0,                                 // Reserved
                                 0                                  // GrowthThreshold
                               );
    if (CsrPortHeap == NULL) {
        NtClose( CsrPortHandle );
        CsrPortHandle = NULL;

        return( STATUS_NO_MEMORY );
        }

    CsrPortBaseTag = RtlCreateTagHeap( CsrPortHeap,
                                       0,
                                       L"CSRPORT!",
                                       L"!CSRPORT\0"
                                       L"CAPTURE\0"
                                     );

    return( STATUS_SUCCESS );
}
