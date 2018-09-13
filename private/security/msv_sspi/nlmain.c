/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    nlmain.c

Abstract:

    This file contains the initialization and dispatch routines
    for the LAN Manager portions of the MSV1_0 authentication package.

Author:

    Jim Kelly 11-Apr-1991

Revision History:
    25-Apr-1991 (cliffv)
        Added interactive logon support for PDK.

    Chandana Surlu   21-Jul-1996
        Stolen from \\kernel\razzle3\src\security\msv1_0\nlmain.c

--*/

#include <global.h>

#include "msp.h"
#undef EXTERN
#define NLP_ALLOCATE
#include "nlp.h"
#undef NLP_ALLOCATE

#include <lmsname.h>    // Service Names

#include <safeboot.h>

#include "nlpcache.h"   // logon cache prototypes


NTSTATUS
NlpMapLogonDomain(
    OUT PUNICODE_STRING MappedDomain,
    IN PUNICODE_STRING LogonDomain
    );

NTSTATUS
NlInitialize(
    VOID
    )

/*++

Routine Description:

    Initialize NETLOGON portion of msv1_0 authentication package.

Arguments:

    None.

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.

--*/

{
    NTSTATUS Status;
    LPWSTR ComputerName;
    DWORD ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    NT_PRODUCT_TYPE NtProductType;
    UNICODE_STRING TempUnicodeString;
    HKEY Key ;
    int err ;
    ULONG Size ;
    ULONG Type ;
    ULONG Value ;

    //
    // Initialize global data
    //

    NlpEnumerationHandle = 0;
    NlpLogonAttemptCount = 0;


    NlpComputerName.Buffer = NULL;
    RtlInitUnicodeString( &NlpPrimaryDomainName, NULL );
    NlpSamDomainName.Buffer = NULL;
    NlpSamDomainId = NULL;
    NlpSamDomainHandle = NULL;



    //
    // Get the name of this machine.
    //

    ComputerName = RtlAllocateHeap(
                        MspHeap, 0,
                        ComputerNameLength * sizeof(WCHAR) );

    if (ComputerName == NULL ||
        !GetComputerNameW( ComputerName, &ComputerNameLength )) {

        KdPrint(( "MsV1_0: Cannot get computername %lX\n", GetLastError() ));

        NlpLanmanInstalled = FALSE;
        RtlFreeHeap( MspHeap, 0, ComputerName );
        ComputerName = NULL;
    } else {

        NlpLanmanInstalled = TRUE;
    }

    //
    // For Safe mode boot (minimal, no networking)
    // turn off the lanmaninstalled flag, since no network components will
    // be started.
    //

    err = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Control\\SafeBoot\\Option",
                0,
                KEY_READ,
                &Key );

    if ( err == 0 )
    {
        Value = 0 ;
        Size = sizeof( ULONG );

        RegQueryValueExW(
                Key,
                L"OptionValue",
                0,
                &Type,
                (PUCHAR) &Value,
                &Size );

        RegCloseKey( Key );

        if ( Value == SAFEBOOT_MINIMAL )
        {
            NlpLanmanInstalled = FALSE ;
        }

    }

    RtlInitUnicodeString( &NlpComputerName, ComputerName );

    //
    // Determine if this machine is running Windows NT or Lanman NT.
    //  LanMan NT runs on a domain controller.
    //

    if ( !RtlGetNtProductType( &NtProductType ) ) {
        KdPrint(( "MsV1_0: Nt Product Type undefined (WinNt assumed)\n" ));
        NtProductType = NtProductWinNt;
    }

    NlpWorkstation = (BOOLEAN)(NtProductType != NtProductLanManNt);



    //
    // Initialize any locks.
    //

    RtlInitializeCriticalSection(&NlpActiveLogonLock);

    //
    // initialize the cache - creates a critical section is all
    //

    NlpCacheInitialize();


    //
    // Attempt to load Netlogon.dll
    //

    NlpLoadNetlogonDll();

#ifdef COMPILED_BY_DEVELOPER
    KdPrint(("msv1_0: COMPILED_BY_DEVELOPER breakpoint.\n"));
    DbgBreakPoint();
#endif // COMPILED_BY_DEVELOPER



    //
    // Initialize useful encryption constants
    //

    Status = RtlCalculateLmOwfPassword( "", &NlpNullLmOwfPassword );
    ASSERT( NT_SUCCESS(Status) );

    RtlInitUnicodeString(&TempUnicodeString, NULL);
    Status = RtlCalculateNtOwfPassword(&TempUnicodeString,
                                       &NlpNullNtOwfPassword);
    ASSERT( NT_SUCCESS(Status) );

    //
    // Initialize the SubAuthentication Dlls
    //

    Msv1_0SubAuthenticationInitialization();




#ifdef notdef
    //
    // If we weren't successful,
    //  Clean up global resources we intended to initialize.
    //

    if ( !NT_SUCCESS(Status) ) {
        if ( NlpComputerName.Buffer != NULL ) {
            MIDL_user_free( NlpComputerName.Buffer );
        }

    }
#endif // notdef

    return STATUS_SUCCESS;

}




NTSTATUS
NlWaitForEvent(
    LPWSTR EventName,
    ULONG Timeout
    )

/*++

Routine Description:

    Wait up to Timeout seconds for EventName to be triggered.

Arguments:

    EventName - Name of event to wait on

    Timeout - Timeout for event (in seconds).

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
    STATUS_NETLOGON_NOT_STARTED - Timeout occurred.

--*/

{
    NTSTATUS Status;

    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventNameString;
    LARGE_INTEGER LocalTimeout;


    //
    // Create an event for us to wait on.
    //

    RtlInitUnicodeString( &EventNameString, EventName);
    InitializeObjectAttributes( &EventAttributes, &EventNameString, 0, 0, NULL);

    Status = NtCreateEvent(
                   &EventHandle,
                   SYNCHRONIZE,
                   &EventAttributes,
                   NotificationEvent,
                   (BOOLEAN) FALSE      // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(Status)) {

        //
        // If the event already exists, the server beat us to creating it.
        // Just open it.
        //

        if( Status == STATUS_OBJECT_NAME_EXISTS ||
            Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = NtOpenEvent( &EventHandle,
                                  SYNCHRONIZE,
                                  &EventAttributes );

        }
        if ( !NT_SUCCESS(Status)) {
            KdPrint(("[MSV1_0] OpenEvent failed %lx\n", Status ));
            return Status;
        }
    }


    //
    // Wait for NETLOGON to initialize.  Wait a maximum of Timeout seconds.
    //

    LocalTimeout.QuadPart = ((LONGLONG)(Timeout)) * (-10000000);
    Status = NtWaitForSingleObject( EventHandle, (BOOLEAN)FALSE, &LocalTimeout);
    (VOID) NtClose( EventHandle );

    if ( !NT_SUCCESS(Status) || Status == STATUS_TIMEOUT ) {
        if ( Status == STATUS_TIMEOUT ) {
            Status = STATUS_NETLOGON_NOT_STARTED;   // Map to an error condition
        }
        return Status;
    }

    return STATUS_SUCCESS;
}


BOOLEAN
NlDoingSetup(
    VOID
    )

/*++

Routine Description:

    Returns TRUE if we're running setup.

Arguments:

    NONE.

Return Status:

    TRUE - We're currently running setup
    FALSE - We're not running setup or aren't sure.

--*/

{
    LONG RegStatus;

    HKEY KeyHandle = NULL;
    DWORD ValueType;
    DWORD Value;
    DWORD ValueSize;

    //
    // Open the key for HKLM\SYSTEM\Setup
    //

    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    "SYSTEM\\Setup",
                    0,      //Reserved
                    KEY_QUERY_VALUE,
                    &KeyHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        KdPrint(( "NlDoingSetup: Cannot open registy key 'HKLM\\SYSTEM\\Setup' %ld.\n",
                  RegStatus ));
        return FALSE;
    }

    //
    // Get the value that says whether we're doing setup.
    //

    ValueSize = sizeof(Value);
    RegStatus = RegQueryValueExA(
                    KeyHandle,
                    "SystemSetupInProgress",
                    0,
                    &ValueType,
                    (LPBYTE)&Value,
                    &ValueSize );

    RegCloseKey( KeyHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        KdPrint(( "NlDoingSetup: Cannot query value of 'HKLM\\SYSTEM\\Setup\\SystemSetupInProgress' %ld.\n",
                  RegStatus ));
        return FALSE;
    }

    if ( ValueType != REG_DWORD ) {
        KdPrint(( "NlDoingSetup: value of 'HKLM\\SYSTEM\\Setup\\SystemSetupInProgress'is not a REG_DWORD %ld.\n",
                  ValueType ));
        return FALSE;
    }

    if ( ValueSize != sizeof(Value) ) {
        KdPrint(( "NlDoingSetup: value size of 'HKLM\\SYSTEM\\Setup\\SystemSetupInProgress'is not 4 %ld.\n",
                  ValueSize ));
        return FALSE;
    }

    if ( Value != 1 ) {
        // KdPrint(( "NlDoingSetup: not doing setup\n" ));
        return FALSE;
    }

    KdPrint((  "NlDoingSetup: doing setup\n" ));
    return TRUE;

}


NTSTATUS
NlWaitForNetlogon(
    ULONG Timeout
    )

/*++

Routine Description:

    Wait up to Timeout seconds for the netlogon service to start.

Arguments:

    Timeout - Timeout for event (in seconds).

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
    STATUS_NETLOGON_NOT_STARTED - Timeout occurred.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    LPQUERY_SERVICE_CONFIG AllocServiceConfig = NULL;
    QUERY_SERVICE_CONFIG DummyServiceConfig;
    DWORD ServiceConfigSize;


    //
    // If the netlogon service is currently running,
    //  skip the rest of the tests.
    //

    Status = NlWaitForEvent( L"\\NETLOGON_SERVICE_STARTED", 0 );

    if ( NT_SUCCESS(Status) ) {
        return Status;
    }

    //
    // If we're in setup,
    //  don't bother waiting for netlogon to start.
    //

    if ( NlDoingSetup() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }


    //
    // Open a handle to the Netlogon Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        KdPrint(( "[MSV1_0] NlWaitForNetlogon: OpenSCManager failed: "
                      "%lu\n", GetLastError()));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        SERVICE_NETLOGON,
                        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

    if ( ServiceHandle == NULL ) {
        KdPrint(( "[MSV1_0] NlWaitForNetlogon: OpenService failed: "
                      "%lu\n", GetLastError()));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }


    //
    // If the Netlogon service isn't configured to be automatically started
    //  by the service controller, don't bother waiting for it to start.
    //
    // ?? Pass "DummyServiceConfig" and "sizeof(..)" since QueryService config
    //  won't allow a null pointer, yet.

    if ( QueryServiceConfig(
            ServiceHandle,
            &DummyServiceConfig,
            sizeof(DummyServiceConfig),
            &ServiceConfigSize )) {

        ServiceConfig = &DummyServiceConfig;

    } else {

        NetStatus = GetLastError();
        if ( NetStatus != ERROR_INSUFFICIENT_BUFFER ) {
            KdPrint(( "[MSV1_0] NlWaitForNetlogon: QueryServiceConfig failed: "
                      "%lu\n", NetStatus));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        AllocServiceConfig = RtlAllocateHeap( MspHeap, 0, ServiceConfigSize );
        ServiceConfig = AllocServiceConfig;

        if ( AllocServiceConfig == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        if ( !QueryServiceConfig(
                ServiceHandle,
                ServiceConfig,
                ServiceConfigSize,
                &ServiceConfigSize )) {

            KdPrint(( "[MSV1_0] NlWaitForNetlogon: QueryServiceConfig "
                      "failed again: %lu\n", GetLastError()));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }
    }

    if ( ServiceConfig->dwStartType != SERVICE_AUTO_START ) {
        KdPrint(( "[MSV1_0] NlWaitForNetlogon: Netlogon start type invalid:"
                          "%lu\n", ServiceConfig->dwStartType ));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }



    //
    // Loop waiting for the netlogon service to start.
    //  (Convert Timeout to a number of 10 second iterations)
    //

    Timeout = (Timeout+9)/10;
    for (;;) {


        //
        // Query the status of the Netlogon service.
        //

        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {

            KdPrint(( "[MSV1_0] NlWaitForNetlogon: QueryServiceStatus failed: "
                          "%lu\n", GetLastError() ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        //
        // Return or continue waiting depending on the state of
        //  the netlogon service.
        //

        switch( ServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            Status = STATUS_SUCCESS;
            goto Cleanup;

        case SERVICE_STOPPED:

            //
            // If Netlogon failed to start,
            //  error out now.  The caller has waited long enough to start.
            //
            if ( ServiceStatus.dwWin32ExitCode != ERROR_SERVICE_NEVER_STARTED ){
#if DBG
                KdPrint(( "[MSV1_0] NlWaitForNetlogon: "
                          "Netlogon service couldn't start: %lu %lx\n",
                          ServiceStatus.dwWin32ExitCode,
                          ServiceStatus.dwWin32ExitCode ));
                if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
                    KdPrint(( "         Service specific error code: %lu %lx\n",
                              ServiceStatus.dwServiceSpecificExitCode,
                              ServiceStatus.dwServiceSpecificExitCode ));
                }
#endif // DBG
                Status = STATUS_NETLOGON_NOT_STARTED;
                goto Cleanup;
            }

            //
            // If Netlogon has never been started on this boot,
            //  continue waiting for it to start.
            //

            break;

        //
        // If Netlogon is trying to start up now,
        //  continue waiting for it to start.
        //
        case SERVICE_START_PENDING:
            break;

        //
        // Any other state is bogus.
        //
        default:
            KdPrint(( "[MSV1_0] NlWaitForNetlogon: "
                      "Invalid service state: %lu\n",
                      ServiceStatus.dwCurrentState ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;

        }


        //
        // Wait ten seconds for the netlogon service to start.
        //  If it has successfully started, just return now.
        //

        Status = NlWaitForEvent( L"\\NETLOGON_SERVICE_STARTED", 10 );

        if ( Status != STATUS_NETLOGON_NOT_STARTED ) {
            goto Cleanup;
        }

        //
        // If we've waited long enough for netlogon to start,
        //  time out now.
        //

        if ( (--Timeout) == 0 ) {
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }


    }

    /* NOT REACHED */

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    if ( AllocServiceConfig != NULL ) {
        RtlFreeHeap( MspHeap, 0, AllocServiceConfig );
    }
    return Status;
}


NTSTATUS
NlSamInitialize(
    ULONG Timeout
    )

/*++

Routine Description:

    Initialize the MSV1_0 Authentication Package's communication to the SAM
    database.  This initialization will take place once immediately prior
    to the first actual use of the SAM database.

Arguments:

    Timeout - Timeout for event (in seconds).

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.

--*/

{
    NTSTATUS Status;

    //
    // locals that are staging area for globals.
    //

    LSA_HANDLE PolicyHandle = NULL;
    UNICODE_STRING PrimaryDomainName;
    PSID SamDomainId = NULL;
    UNICODE_STRING SamDomainName;
    SAMPR_HANDLE SamDomainHandle = NULL;
    BOOLEAN UasCompatibilityRequired;


    OBJECT_ATTRIBUTES PolicyObjectAttributes;
    PLSAPR_POLICY_INFORMATION PolicyPrimaryDomainInfo = NULL;
    PLSAPR_POLICY_INFORMATION PolicyAccountDomainInfo = NULL;

    SAMPR_HANDLE SamHandle = NULL;
#ifdef SAM
    PSAMPR_DOMAIN_INFO_BUFFER DomainInfo = NULL;
#endif // SAM

    PrimaryDomainName.Buffer = NULL;
    SamDomainName.Buffer = NULL;


    //
    // Wait for SAM to finish initialization.
    //

    Status = NlWaitForEvent( L"\\SAM_SERVICE_STARTED", Timeout );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }



    //
    // Determine the DomainName and DomainId of the Account Database
    //

    InitializeObjectAttributes( &PolicyObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    Status = I_LsaIOpenPolicyTrusted(&PolicyHandle);

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    ASSERT(PolicyHandle);

    Status = I_LsarQueryInformationPolicy( PolicyHandle,
                                           PolicyAccountDomainInformation,
                                           &PolicyAccountDomainInfo );


    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    if ( PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid == NULL ||
         PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainName.Length == 0 ) {
        KdPrint(( "MsV1_0: Account domain info from LSA invalid.\n"));
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    Status = I_LsarQueryInformationPolicy(
                                PolicyHandle,
                                PolicyPrimaryDomainInformation,
                                &PolicyPrimaryDomainInfo );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    if ( PolicyPrimaryDomainInfo->PolicyPrimaryDomainInfo.Name.Length == 0 )
    {
        KdPrint(( "MsV1_0: Primary domain info from LSA invalid.\n"));
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }


    //
    // save PrimaryDomainName
    //

    PrimaryDomainName.Length = PolicyPrimaryDomainInfo->PolicyPrimaryDomainInfo.Name.Length;
    PrimaryDomainName.MaximumLength = PrimaryDomainName.Length;

    PrimaryDomainName.Buffer =
            (PWSTR)RtlAllocateHeap( MspHeap, 0, PrimaryDomainName.MaximumLength );

    if ( PrimaryDomainName.Buffer == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( PrimaryDomainName.Buffer,
                    PolicyPrimaryDomainInfo->PolicyPrimaryDomainInfo.Name.Buffer,
                    PrimaryDomainName.Length );


    //
    // Save the domain id of this domain
    //

    SamDomainId = RtlAllocateHeap(
                        MspHeap, 0,
                        RtlLengthSid( PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid ));

    if ( SamDomainId == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( SamDomainId,
                   PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid,
                   RtlLengthSid( PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid ));


    //
    // Save the name of the account database on this machine.
    //
    // On a workstation, the account database is refered to by the machine
    // name and not the database name.

    // The above being true, the machine name is set to MACHINENAME during
    // setup and for the duration when the machine has a real machine name
    // until the end of setup, NlpSamDomainName will still have MACHINENAME.
    // This is not what the caller expects to authenticate against, so we
    // force a look from the Lsa all the time.

    // We assume that NlpSamDomainName will get the right info from the Lsa

    SamDomainName.Length = PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainName.Length;
    SamDomainName.MaximumLength = (USHORT)
        (SamDomainName.Length + sizeof(WCHAR));

    SamDomainName.Buffer =
        RtlAllocateHeap( MspHeap, 0, SamDomainName.MaximumLength );

    if ( SamDomainName.Buffer == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( SamDomainName.Buffer,
                   PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainName.Buffer,
                   SamDomainName.MaximumLength );


    //
    // Open our connection with SAM
    //

    Status = I_SamIConnect( NULL,     // No server name
                            &SamHandle,
                            SAM_SERVER_CONNECT,
                            (BOOLEAN) TRUE );   // Indicate we are privileged

    if ( !NT_SUCCESS(Status) ) {
        SamHandle = NULL;
        KdPrint(( "MsV1_0: Cannot SamIConnect %lX\n", Status));
        goto Cleanup;
    }

    //
    // Open the domain.
    //

    Status = I_SamrOpenDomain( SamHandle,
                               DOMAIN_ALL_ACCESS,
                               SamDomainId,
                               &SamDomainHandle );

    if ( !NT_SUCCESS(Status) ) {
        SamDomainHandle = NULL;
        KdPrint(( "MsV1_0: Cannot SamrOpenDomain %lX\n", Status));
        goto Cleanup;
    }

#ifdef SAM
    #error
    //
    // Ensure the role in SAM is compatible with Netlogon's role
    //  ?? Use DomainUasInformation once it is defined
    //

    Status = I_SamrQueryInformationDomain( SamDomainHandle,
                                           DomainGeneralInformation,
                                           &DomainInfo );
    if ( !NT_SUCCESS(Status) ) {
        DomainInfo = NULL;
        KdPrint(( "MsV1_0: Cannot SamrQueryInformationDomain %lX\n", Status));
        goto Cleanup;
    }

    // ?? Doesn't define this properly
    UasCompatibilityRequired = DomainInfo->General.UasCompatibilityRequired;

    I_SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainInfo, DomainGeneralInformation );
#endif // SAM



    Status = STATUS_SUCCESS;

    EnterCriticalSection(&NtLmGlobalCritSect);

    if( !NlpSamInitialized ) {

        NlpPolicyHandle = PolicyHandle;
        NlpPrimaryDomainName = PrimaryDomainName;
        NlpSamDomainId = SamDomainId;
        NlpSamDomainName = SamDomainName;
        NlpSamDomainHandle = SamDomainHandle;

        //
        // mark locals invalid so they don't get freed.
        //

        PolicyHandle = NULL;
        PrimaryDomainName.Buffer = NULL;
        SamDomainId = NULL;
        SamDomainName.Buffer = NULL;
        SamDomainHandle = NULL;


        NlpSamInitialized = TRUE;
    }

    LeaveCriticalSection(&NtLmGlobalCritSect);


Cleanup:

    if ( PrimaryDomainName.Buffer != NULL ) {
        RtlFreeHeap( MspHeap, 0, PrimaryDomainName.Buffer );
    }

    if ( SamDomainName.Buffer != NULL ) {
        RtlFreeHeap( MspHeap, 0, SamDomainName.Buffer );
    }

    if ( SamDomainHandle != NULL ) {
        (VOID) I_SamrCloseHandle( &SamDomainHandle );
    }

    if ( SamDomainId != NULL ) {
        RtlFreeHeap( MspHeap, 0, SamDomainId );
    }

    if (PolicyHandle != NULL) {
        (VOID) I_LsarClose( &PolicyHandle );
    }

    if ( PolicyAccountDomainInfo != NULL ) {
        I_LsaIFree_LSAPR_POLICY_INFORMATION( PolicyAccountDomainInformation,
                                             PolicyAccountDomainInfo );
    }

    if ( PolicyPrimaryDomainInfo != NULL ) {
        I_LsaIFree_LSAPR_POLICY_INFORMATION( PolicyPrimaryDomainInformation,
                                             PolicyPrimaryDomainInfo );
    }

    if ( SamHandle != NULL ) {
        (VOID) I_SamrCloseHandle( &SamHandle );
    }


    return Status;

}



NTSTATUS
MspLm20Challenge (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0Lm20ChallengeRequest.  It is called by
    the LanMan server to determine the Challenge to pass back to a
    redirector trying to establish a connection to the server.  The server
    is responsible remembering this Challenge and passing in back to this
    authentication package on a subsequent MsV1_0Lm20Logon request.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.




--*/

{
    NTSTATUS Status;
    PMSV1_0_LM20_CHALLENGE_REQUEST ChallengeRequest;
    PMSV1_0_LM20_CHALLENGE_RESPONSE ChallengeResponse;
    CLIENT_BUFFER_DESC ClientBufferDesc;

/*
    ULONG Seed[2];
    CYPHER_BLOCK Challenge;
*/

    UNREFERENCED_PARAMETER( ClientBufferBase );

    ASSERT( sizeof(LM_CHALLENGE) == MSV1_0_CHALLENGE_LENGTH );
/*
    ASSERT( sizeof(LM_CHALLENGE) == sizeof(CYPHER_BLOCK) );
*/
    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_LM20_CHALLENGE_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    ChallengeRequest = (PMSV1_0_LM20_CHALLENGE_REQUEST) ProtocolSubmitBuffer;

    ASSERT( ChallengeRequest->MessageType == MsV1_0Lm20ChallengeRequest );

    //
    // Allocate a buffer to return to the caller.
    //

    *ReturnBufferSize = sizeof(MSV1_0_LM20_CHALLENGE_RESPONSE);

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_LM20_CHALLENGE_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    ChallengeResponse = (PMSV1_0_LM20_CHALLENGE_RESPONSE) ClientBufferDesc.MsvBuffer;

    //
    // Fill in the return buffer.
    //

    ChallengeResponse->MessageType = MsV1_0Lm20ChallengeRequest;

    //
    // Compute a random seed.
    //

    Status = SspGenerateRandomBits(
                    ChallengeResponse->ChallengeToClient,
                    MSV1_0_CHALLENGE_LENGTH
                    );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );


Cleanup:

    //
    // If we weren't successful, free the buffer in the clients address space.
    //

    if ( !NT_SUCCESS(Status) ) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    //
    // Return status to the caller.
    //

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;

}

#define NULL_SESSION_REQUESTED RETURN_RESERVED_PARAMETER

NTSTATUS
MspLm20GetChallengeResponse (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0Lm20GetChallengeResponse.  It is called by
    the LanMan redirector to determine the Challenge Response to pass to a
    server when trying to establish a connection to the server.

    This routine is passed a Challenge from the server.  This routine encrypts
    the challenge with either the specified password or with the password
    implied by the specified Logon Id.

    Two Challenge responses are returned.  One is based on the Unicode password
    as given to the Authentication package.  The other is based on that
    password converted to a multi-byte character set (e.g., ASCII) and upper
    cased.  The redirector should use whichever (or both) challenge responses
    as it needs them.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

--*/

{
    NTSTATUS Status;
    PMSV1_0_GETCHALLENRESP_REQUEST GetRespRequest;

    CLIENT_BUFFER_DESC ClientBufferDesc;
    PMSV1_0_GETCHALLENRESP_RESPONSE GetRespResponse;

    PMSV1_0_PRIMARY_CREDENTIAL Credential = NULL;
    PMSV1_0_PRIMARY_CREDENTIAL PrimaryCredential = NULL;
    MSV1_0_PRIMARY_CREDENTIAL BuiltCredential;

    //
    // Responses to return to the caller.
    //
    LM_RESPONSE LmResponse;
    STRING LmResponseString;

    NT_RESPONSE NtResponse;
    STRING NtResponseString;

    PMSV1_0_NTLM3_RESPONSE pNtlm3Response = NULL;

    UNICODE_STRING UserName;
    UNICODE_STRING LogonDomainName;
    USER_SESSION_KEY UserSessionKey;
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];

    UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
    ULONG NtLmProtocolSupported;

    //
    // Initialization
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    RtlInitUnicodeString( &UserName, NULL );
    RtlInitUnicodeString( &LogonDomainName, NULL );

    RtlZeroMemory( &UserSessionKey, sizeof(UserSessionKey) );
    RtlZeroMemory( LanmanSessionKey, sizeof(LanmanSessionKey) );

    //
    // If no credentials are associated with the client, a null session
    // will be used.  For a downlevel server, the null session response is
    // a 1-byte null string (\0).  Initialize LmResponseString to the
    // null session response.
    //

    RtlInitString( &LmResponseString, "" );
    LmResponseString.Length = 1;

    //
    // Initialize the NT response to the NT null session credentials,
    // which are zero length.
    //

    RtlInitString( &NtResponseString, NULL );

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_GETCHALLENRESP_REQUEST_V1) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    GetRespRequest = (PMSV1_0_GETCHALLENRESP_REQUEST) ProtocolSubmitBuffer;

    ASSERT( GetRespRequest->MessageType == MsV1_0Lm20GetChallengeResponse );

    if ( (GetRespRequest->ParameterControl & USE_PRIMARY_PASSWORD) == 0 ) {
        RELOCATE_ONE( &GetRespRequest->Password );
    }



    //
    // If we don't support the request (such as the caller is asking for an
    // LM challenge response and we do't support it, return an error here.
    //

    NtLmProtocolSupported = NtLmGlobalLmProtocolSupported;

    if( (GetRespRequest->ParameterControl & RETURN_NON_NT_USER_SESSION_KEY) &&
        NtLmProtocolSupported == NoLm ) {

        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // if caller wants NTLM++, so be it...
    //

    if ( (GetRespRequest->ParameterControl & GCR_NTLM3_PARMS) ) {
        PMSV1_0_AV_PAIR pAV;

        UCHAR TargetInfoBuffer[3*sizeof(MSV1_0_AV_PAIR) + (DNS_MAX_NAME_LENGTH+CNLEN+2)*sizeof(WCHAR)];

        NULL_RELOCATE_ONE( &GetRespRequest->UserName );
        NULL_RELOCATE_ONE( &GetRespRequest->LogonDomainName );
        NULL_RELOCATE_ONE( &GetRespRequest->ServerName );

        //
        // if no target provided and we're config'd to use NTLMv2, then complain
        //

        if ((NtLmProtocolSupported >= UseNtlm3) &&
            (GetRespRequest->ServerName.Length == 0)) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        // if target is just a domain name or domain name followed by
        //  server name, make it into an AV pair list
        if (!(GetRespRequest->ParameterControl & GCR_TARGET_INFO)) {
            UNICODE_STRING DomainName;
            UNICODE_STRING ServerName;
            unsigned int i;

            //
            // check length of name to make sure it fits in my buffer
            //

            if (GetRespRequest->ServerName.Length > (DNS_MAX_NAME_LENGTH+CNLEN+2)*sizeof(WCHAR)) {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // init AV list in temp buffer
            //

            pAV = MsvpAvlInit(TargetInfoBuffer);

            //
            // see if there's a NULL in the middle of the server name
            //  that indicates that it's really a domain name followed by a server name
            //

            DomainName = GetRespRequest->ServerName;
            ServerName.Length = 0;

            for (i = 0; i < (DomainName.Length/sizeof(WCHAR)); i++) {
                if (DomainName.Buffer[i] == 0) {
                    // take length of domain name without the NULL
                    DomainName.Length = (USHORT) i*sizeof(WCHAR);
                    // adjust server name and length to point after the domain name
                    ServerName.Length = GetRespRequest->ServerName.Length - (i+1)*sizeof(WCHAR);
                    ServerName.Buffer = GetRespRequest->ServerName.Buffer + (i+1);
                    break;
                }
            }

            //
            // strip off possible trailing null after the server name
            //

            for (i = 0; i < (ServerName.Length / sizeof(WCHAR)); i++) {
                if (ServerName.Buffer[i] == 0) {
                    ServerName.Length = (USHORT)i*sizeof(WCHAR);
                    break;
                }
            }

            //
            // put both names in the AV list (if both exist)
            //

            MsvpAvlAdd(pAV, MsvAvNbDomainName, &DomainName, sizeof(TargetInfoBuffer));
            if (ServerName.Length > 0) {
                MsvpAvlAdd(pAV, MsvAvNbComputerName, &ServerName, sizeof(TargetInfoBuffer));
            }

            //
            // make the request point at AV list instead of names.
            //

            GetRespRequest->ServerName.Length = (USHORT)MsvpAvlLen(pAV, sizeof(TargetInfoBuffer));
            GetRespRequest->ServerName.Buffer = (PWCHAR)pAV;
        }

        //
        // if we're only using NTLMv2 or better, then complain if either
        //  computer name or server name missing
        //

        if (NtLmProtocolSupported >= RefuseNtlm3NoTarget) {
            pAV = (PMSV1_0_AV_PAIR)GetRespRequest->ServerName.Buffer;
            if (MsvpAvlGet(pAV, MsvAvNbDomainName, GetRespRequest->ServerName.Length) == NULL ||
                MsvpAvlGet(pAV, MsvAvNbComputerName, GetRespRequest->ServerName.Length) == NULL) {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
    }


    //
    // If the caller wants information from the credentials of a specified
    //  LogonId, get those credentials from the LSA.
    //
    // If there are no such credentials,
    //  tell the caller to use the NULL session.
    //

#define PRIMARY_CREDENTIAL_NEEDED \
        (RETURN_PRIMARY_LOGON_DOMAINNAME | \
        RETURN_PRIMARY_USERNAME | \
        USE_PRIMARY_PASSWORD )

    if ( ((GetRespRequest->ParameterControl & PRIMARY_CREDENTIAL_NEEDED) != 0 ) && ((GetRespRequest->ParameterControl & NULL_SESSION_REQUESTED) == 0)) {

        Status = NlpGetPrimaryCredential(
                        &GetRespRequest->LogonId,
                        &PrimaryCredential,
                        NULL );

        if ( NT_SUCCESS(Status) ) {

            if ( GetRespRequest->ParameterControl & RETURN_PRIMARY_USERNAME ) {
                UserName = PrimaryCredential->UserName;
            }

            if ( GetRespRequest->ParameterControl &
                 RETURN_PRIMARY_LOGON_DOMAINNAME ) {

#ifndef DONT_MAP_DOMAIN_ON_REQUEST
                //
                // Map the user's logon domain against the current mapping
                // in the registry.
                //

                Status = NlpMapLogonDomain(
                            &LogonDomainName,
                            &PrimaryCredential->LogonDomainName
                            );
                if (!NT_SUCCESS(Status)) {
                    goto Cleanup;
                }
#else
                LogonDomainName = PrimaryCredential->LogonDomainName;
#endif
            }

        } else if ( Status == STATUS_NO_SUCH_LOGON_SESSION ||
                    Status == STATUS_UNSUCCESSFUL ) {

            //
            // Clean up the status code
            //

            Status = STATUS_NO_SUCH_LOGON_SESSION;

            //
            // If the caller wants at least the password from the primary
            //  credential, just use a NULL session primary credential.
            //

            if ( (GetRespRequest->ParameterControl & USE_PRIMARY_PASSWORD ) ==
                    USE_PRIMARY_PASSWORD ) {

                PrimaryCredential = NULL;

            //
            // If part of the information was supplied by the caller,
            //  report the error to the caller.
            //
            } else {
                KdPrint(("MSV1_0: MspLm20GetChallengeResponse: cannot "
                         " GetPrimaryCredential %lx\n", Status ));
                goto Cleanup;
            }
        } else {
                KdPrint(("MSV1_0: MspLm20GetChallengeResponse: cannot "
                         " GetPrimaryCredential %lx\n", Status ));
                goto Cleanup;
        }

        Credential = PrimaryCredential;

    }


    //
    // If the caller passed in a password to use,
    //  use it to build a credential.
    //

    if ( (GetRespRequest->ParameterControl & USE_PRIMARY_PASSWORD) == 0 ) {

        NlpPutOwfsInPrimaryCredential( &GetRespRequest->Password,
                                       &BuiltCredential );

        //
        // Use the newly allocated credential to get the password information
        // from.
        //
        Credential = &BuiltCredential;

    }

    //
    // Build the appropriate response.
    //

    if ( Credential != NULL ) {

        //
        // If the DC is asserted to have been upgraded, we should use NTLM3
        //  if caller supplies the NTLM3 parameters
        //

        if ((NtLmProtocolSupported >= UseNtlm3) &&
            (GetRespRequest->ParameterControl & GCR_NTLM3_PARMS)
            ) {

            USHORT Ntlm3ResponseSize;
            UNICODE_STRING Ntlm3UserName;
            UNICODE_STRING Ntlm3LogonDomainName;
            UNICODE_STRING Ntlm3ServerName;

            // use the server name supplied by the caller
            Ntlm3ServerName = GetRespRequest->ServerName;

            // even if user name and domain are supplied, use current logged
            //  in user if so requested

            if (GetRespRequest->ParameterControl & USE_PRIMARY_PASSWORD) {
                Ntlm3UserName = Credential->UserName;
                Ntlm3LogonDomainName = Credential->LogonDomainName;
            } else {
                Ntlm3UserName = GetRespRequest->UserName;
                Ntlm3LogonDomainName = GetRespRequest->LogonDomainName;
            }

            //
            // Allocate the response
            //

            Ntlm3ResponseSize =
                sizeof(MSV1_0_NTLM3_RESPONSE) + Ntlm3ServerName.Length;

            pNtlm3Response = (*Lsa.AllocateLsaHeap)( Ntlm3ResponseSize );

            if ( pNtlm3Response == NULL ) {
                KdPrint(("MSV1_0: MspLm20GetChallengeResponse: No memory %ld\n",
                    Ntlm3ResponseSize ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            MsvpLm20GetNtlm3ChallengeResponse(
                &Credential->NtOwfPassword,
                &Ntlm3UserName,
                &Ntlm3LogonDomainName,
                &Ntlm3ServerName,
                GetRespRequest->ChallengeToClient,
                pNtlm3Response,  // BUGBUG??
                (PMSV1_0_LM3_RESPONSE)&LmResponse,
                &UserSessionKey,
                (PLM_SESSION_KEY)LanmanSessionKey
                );

            NtResponseString.Buffer = (PUCHAR) pNtlm3Response;
            NtResponseString.Length = Ntlm3ResponseSize;
            LmResponseString.Buffer = (PUCHAR) &LmResponse;
            LmResponseString.Length = sizeof(LmResponse);
        } else {

            //
            // if requested, generate our own challenge, and mix it with that
            //  of the server's
            //

            if (GetRespRequest->ParameterControl & GENERATE_CLIENT_CHALLENGE) {

                SspGenerateRandomBits(ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);

#ifdef USE_CONSTANT_CHALLENGE
                RtlZeroMemory(ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);
#endif

                RtlCopyMemory(
                    ChallengeToClient,
                    GetRespRequest->ChallengeToClient,
                    MSV1_0_CHALLENGE_LENGTH
                    );

                MsvpCalculateNtlm2Challenge (
                    GetRespRequest->ChallengeToClient,
                    ChallengeFromClient,
                    GetRespRequest->ChallengeToClient
                    );

            }

            Status = RtlCalculateNtResponse(
                        (PNT_CHALLENGE) GetRespRequest->ChallengeToClient,
                        &Credential->NtOwfPassword,
                        &NtResponse );

            if ( !NT_SUCCESS( Status ) ) {
                goto Cleanup;
            }


            //
            // send the client challenge back in the LM response slot if we made one
            //
            if (GetRespRequest->ParameterControl & GENERATE_CLIENT_CHALLENGE) {

                RtlZeroMemory(
                    &LmResponse,
                    sizeof(LmResponse)
                    );

                RtlCopyMemory(
                    &LmResponse,
                    ChallengeFromClient,
                    MSV1_0_CHALLENGE_LENGTH
                    );
            //
            // Return the LM response if policy set that way for backwards compatibility.
            //

            } else if ((NtLmProtocolSupported <= AllowLm) ) {
                Status = RtlCalculateLmResponse(
                            (PLM_CHALLENGE) GetRespRequest->ChallengeToClient,
                            &Credential->LmOwfPassword,
                            &LmResponse );

                if ( !NT_SUCCESS( Status ) ) {
                    goto Cleanup;
                }

            //
            //
            //  Can't return LM response -- so use NT response
            //   (to allow LM_KEY generatation)
            //

            } else {
                RtlCopyMemory(
                        &LmResponse,
                        &NtResponse,
                        sizeof(LmResponse)
                        );
            }

            NtResponseString.Buffer = (PUCHAR) &NtResponse;
            NtResponseString.Length = sizeof(NtResponse);
            LmResponseString.Buffer = (PUCHAR) &LmResponse;
            LmResponseString.Length = sizeof(LmResponse);

            //
            // Compute the session keys
            //

            if (GetRespRequest->ParameterControl & GENERATE_CLIENT_CHALLENGE) {

                //
                // assert: we're talking to an NT4-SP4 or later server
                //          and the user's DC hasn't been upgraded to NTLM++
                //  generate session key from MD4(NT hash) -
                //  aka NtUserSessionKey - that is different for each session
                //

                Status = RtlCalculateUserSessionKeyNt(
                                &NtResponse,
                                &Credential->NtOwfPassword,
                                &UserSessionKey );

                if ( !NT_SUCCESS( Status ) ) {
                    goto Cleanup;
                }

                MsvpCalculateNtlm2SessionKeys(
                    &UserSessionKey,
                    ChallengeToClient,
                    ChallengeFromClient,
                    (PUSER_SESSION_KEY)&UserSessionKey,
                    (PLM_SESSION_KEY)LanmanSessionKey
                    );

            } else if ( GetRespRequest->ParameterControl & RETURN_NON_NT_USER_SESSION_KEY){

                //
                // If the redir didn't negotiate an NT protocol with the server,
                //  use the lanman session key.
                //

                if ( Credential->LmPasswordPresent ) {

                    ASSERT( sizeof(UserSessionKey) >= sizeof(LanmanSessionKey) );

                    RtlCopyMemory( &UserSessionKey,
                                   &Credential->LmOwfPassword,
                                   sizeof(LanmanSessionKey) );
                }

            } else {

                if ( !Credential->NtPasswordPresent ) {

                    RtlCopyMemory( &Credential->NtOwfPassword,
                                &NlpNullNtOwfPassword,
                                sizeof(Credential->NtOwfPassword) );
                }

                Status = RtlCalculateUserSessionKeyNt(
                                &NtResponse,
                                &Credential->NtOwfPassword,
                                &UserSessionKey );

                if ( !NT_SUCCESS( Status ) ) {
                    goto Cleanup;
                }
            }

            if ( Credential->LmPasswordPresent ) {
                RtlCopyMemory( LanmanSessionKey,
                               &Credential->LmOwfPassword,
                               sizeof(LanmanSessionKey) );
            }

        } // UseNtlm3

    }


    //
    // Allocate a buffer to return to the caller.
    //

    *ReturnBufferSize = sizeof(MSV1_0_GETCHALLENRESP_RESPONSE) +
                        LogonDomainName.Length + sizeof(WCHAR) +
                        UserName.Length + sizeof(WCHAR) +
                        NtResponseString.Length + sizeof(WCHAR) +
                        LmResponseString.Length + sizeof(WCHAR);

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_GETCHALLENRESP_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    GetRespResponse = (PMSV1_0_GETCHALLENRESP_RESPONSE) ClientBufferDesc.MsvBuffer;


    //
    // Fill in the return buffer.
    //

    GetRespResponse->MessageType = MsV1_0Lm20GetChallengeResponse;
    RtlCopyMemory( GetRespResponse->UserSessionKey,
                   &UserSessionKey,
                   sizeof(UserSessionKey));
    RtlCopyMemory( GetRespResponse->LanmanSessionKey,
                   LanmanSessionKey,
                   sizeof(LanmanSessionKey) );


    //
    // Copy the logon domain name (the string may be empty)
    //

    NlpPutClientString( &ClientBufferDesc,
                        &GetRespResponse->LogonDomainName,
                        &LogonDomainName );

    //
    // Copy the user name (the string may be empty)
    //

    NlpPutClientString( &ClientBufferDesc,
                        &GetRespResponse->UserName,
                        &UserName );

    //
    // Copy the Challenge Responses to the client buffer.
    //

    NlpPutClientString(
                &ClientBufferDesc,
                (PUNICODE_STRING)
                    &GetRespResponse->CaseSensitiveChallengeResponse,
                (PUNICODE_STRING) &NtResponseString );

    NlpPutClientString(
                &ClientBufferDesc,
                (PUNICODE_STRING)
                    &GetRespResponse->CaseInsensitiveChallengeResponse,
                (PUNICODE_STRING)&LmResponseString );


    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );

Cleanup:

    //
    // If we weren't successful, free the buffer in the clients address space.
    //

    if ( !NT_SUCCESS(Status) ) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    //
    // Cleanup locally used resources
    //

    if ( PrimaryCredential != NULL ) {
        (*Lsa.FreeLsaHeap)( PrimaryCredential );
    }

#ifndef DONT_MAP_DOMAIN_ON_REQUEST

    if (LogonDomainName.Buffer != NULL) {
        NtLmFree(LogonDomainName.Buffer);
    }
#endif

    if ( pNtlm3Response != NULL ) {
        (*Lsa.FreeLsaHeap)( pNtlm3Response );
    }

    //
    // Return status to the caller.
    //

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;

}


NTSTATUS
MspLm20EnumUsers (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0Lm20EnumerateUsers.  This routine
    enumerates all of the interactive, service, and batch logons to the MSV1_0
    authentication package.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.



--*/

{
    NTSTATUS Status;
    PMSV1_0_ENUMUSERS_REQUEST EnumRequest;
    PMSV1_0_ENUMUSERS_RESPONSE EnumResponse;
    CLIENT_BUFFER_DESC ClientBufferDesc;
    ULONG LogonCount = 0;
    PACTIVE_LOGON Logon;
    BOOLEAN ActiveLogonsAreLocked = FALSE;

    PUCHAR Where;

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );
    UNREFERENCED_PARAMETER( ClientBufferBase );

    if ( SubmitBufferSize < sizeof(MSV1_0_ENUMUSERS_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    EnumRequest = (PMSV1_0_ENUMUSERS_REQUEST) ProtocolSubmitBuffer;

    ASSERT( EnumRequest->MessageType == MsV1_0EnumerateUsers );

    //
    // Count the current number of active logons
    //

    NlpLockActiveLogons();
    ActiveLogonsAreLocked = TRUE;

    for( Logon = NlpActiveLogons; Logon != NULL; Logon = Logon->Next ) {
        LogonCount ++;
    }

    //
    // Allocate a buffer to return to the caller.
    //

    *ReturnBufferSize = sizeof(MSV1_0_ENUMUSERS_RESPONSE) +
                            LogonCount * (sizeof(LUID) + sizeof(ULONG));


    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_ENUMUSERS_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    EnumResponse = (PMSV1_0_ENUMUSERS_RESPONSE) ClientBufferDesc.MsvBuffer;

    //
    // Fill in the return buffer.
    //

    EnumResponse->MessageType = MsV1_0EnumerateUsers;
    EnumResponse->NumberOfLoggedOnUsers = LogonCount;

    Where = (PUCHAR)(EnumResponse + 1);

    //
    // Loop through the Active Logon Table copying the LogonId of each session.
    //

    EnumResponse->LogonIds = (PLUID)(ClientBufferDesc.UserBuffer +
                                (Where - ClientBufferDesc.MsvBuffer));
    for( Logon = NlpActiveLogons; Logon != NULL; Logon = Logon->Next ) {
        *((PLUID)Where) = Logon->LogonId,
        Where += sizeof(LUID);
    }

    //
    // Loop through the Active Logon Table copying the EnumHandle of
    //  each session.
    //

    EnumResponse->EnumHandles = (PULONG)(ClientBufferDesc.UserBuffer +
                                    (Where - ClientBufferDesc.MsvBuffer));
    for( Logon = NlpActiveLogons; Logon != NULL; Logon = Logon->Next ) {
        *((PULONG)Where) = Logon->EnumHandle,
        Where += sizeof(ULONG);
    }

    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );

Cleanup:

    //
    // Be sure to unlock the lock on the Active logon list.
    //

    if ( ActiveLogonsAreLocked ) {
        NlpUnlockActiveLogons();
    }

    //
    // If we weren't successful, free the buffer in the clients address space.
    //

    if ( !NT_SUCCESS(Status)) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    //
    // Return status to the caller.
    //

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;

}


NTSTATUS
MspLm20GetUserInfo (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0GetUserInfo.  This routine
    returns information describing a particular Logon Id.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.



--*/

{
    NTSTATUS Status;
    PMSV1_0_GETUSERINFO_REQUEST GetInfoRequest;
    PMSV1_0_GETUSERINFO_RESPONSE GetInfoResponse = NULL;

    CLIENT_BUFFER_DESC ClientBufferDesc;

    BOOLEAN ActiveLogonsAreLocked = FALSE;
    PACTIVE_LOGON *ActiveLogon;
    PACTIVE_LOGON Logon;
    ULONG SidLength;

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    UNREFERENCED_PARAMETER( ClientBufferBase );

    if ( SubmitBufferSize < sizeof(MSV1_0_GETUSERINFO_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    GetInfoRequest = (PMSV1_0_GETUSERINFO_REQUEST) ProtocolSubmitBuffer;

    ASSERT( GetInfoRequest->MessageType == MsV1_0GetUserInfo );

    //
    // Find the Active logon entry for this particular Logon Id.
    //

    NlpLockActiveLogons();
    ActiveLogonsAreLocked = TRUE;

    if (!NlpFindActiveLogon( &GetInfoRequest->LogonId, &ActiveLogon )){
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    Logon = *ActiveLogon;

    //
    // Allocate a buffer to return to the caller.
    //

    SidLength = RtlLengthSid( Logon->UserSid );
    *ReturnBufferSize = sizeof(MSV1_0_GETUSERINFO_RESPONSE) +
                            Logon->UserName.Length + sizeof(WCHAR) +
                            Logon->LogonDomainName.Length + sizeof(WCHAR) +
                            Logon->LogonServer.Length + sizeof(WCHAR) +
                            SidLength;


    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_GETUSERINFO_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    GetInfoResponse = (PMSV1_0_GETUSERINFO_RESPONSE) ClientBufferDesc.MsvBuffer;


    //
    // Fill in the return buffer.
    //

    GetInfoResponse->MessageType = MsV1_0GetUserInfo;
    GetInfoResponse->LogonType = Logon->LogonType;

    //
    // Copy ULONG aligned data first
    //

    GetInfoResponse->UserSid = ClientBufferDesc.UserBuffer +
                               ClientBufferDesc.StringOffset;

    RtlCopyMemory( ClientBufferDesc.MsvBuffer + ClientBufferDesc.StringOffset,
                   Logon->UserSid,
                   SidLength );

    ClientBufferDesc.StringOffset += SidLength;

    //
    // Copy WCHAR aligned data
    //

    NlpPutClientString( &ClientBufferDesc,
                        &GetInfoResponse->UserName,
                        &Logon->UserName );

    NlpPutClientString( &ClientBufferDesc,
                        &GetInfoResponse->LogonDomainName,
                        &Logon->LogonDomainName );

    NlpPutClientString( &ClientBufferDesc,
                        &GetInfoResponse->LogonServer,
                        &Logon->LogonServer );


    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );

Cleanup:

    //
    // Be sure to unlock the lock on the Active logon list.
    //

    if ( ActiveLogonsAreLocked ) {
        NlpUnlockActiveLogons();
    }

    //
    // If we weren't successful, free the buffer in the clients address space.
    //

    if ( !NT_SUCCESS(Status)) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    //
    // Return status to the caller.
    //

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;

}


NTSTATUS
MspLm20ReLogonUsers (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0RelogonUsers.  For each logon session
    which was validated by the specified domain controller,  the logon session
    is re-established with that same domain controller.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.


--*/

{
    UNREFERENCED_PARAMETER( ClientRequest );
    UNREFERENCED_PARAMETER( ProtocolSubmitBuffer);
    UNREFERENCED_PARAMETER( ClientBufferBase);
    UNREFERENCED_PARAMETER( SubmitBufferSize);
    UNREFERENCED_PARAMETER( ReturnBufferSize);

    *ProtocolReturnBuffer = NULL;
    *ProtocolStatus = STATUS_NOT_IMPLEMENTED;
    return STATUS_SUCCESS;

}



NTSTATUS
MspLm20GenericPassthrough (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0Lm20GenericPassthrough. It is called by
    a client wishing to make a CallAuthenticationPackage call against
    a domain controller.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.




--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PMSV1_0_PASSTHROUGH_REQUEST PassthroughRequest;
    PMSV1_0_PASSTHROUGH_RESPONSE PassthroughResponse;
    CLIENT_BUFFER_DESC ClientBufferDesc;
    BOOLEAN Authoritative;
    PNETLOGON_VALIDATION_GENERIC_INFO ValidationGeneric = NULL;

    NETLOGON_GENERIC_INFO LogonGeneric;
    PNETLOGON_LOGON_IDENTITY_INFO LogonInformation;


    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );
    *ProtocolStatus = STATUS_SUCCESS;

    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_PASSTHROUGH_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    PassthroughRequest = (PMSV1_0_PASSTHROUGH_REQUEST) ProtocolSubmitBuffer;

    RELOCATE_ONE( &PassthroughRequest->DomainName );
    RELOCATE_ONE( &PassthroughRequest->PackageName );

    //
    // Make sure the buffer fits in the supplied size
    //

    if (PassthroughRequest->LogonData != NULL) {

        if (PassthroughRequest->LogonData + PassthroughRequest->DataLength <
            PassthroughRequest->LogonData ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        if ((ULONG_PTR)ClientBufferBase + SubmitBufferSize < (ULONG_PTR)ClientBufferBase ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        if (PassthroughRequest->LogonData + PassthroughRequest->DataLength >
            (PUCHAR) ClientBufferBase + SubmitBufferSize) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Reset the pointers for the validation data
        //

        PassthroughRequest->LogonData =
                (PUCHAR) PassthroughRequest -
                (ULONG_PTR) ClientBufferBase +
                (ULONG_PTR) PassthroughRequest->LogonData;

    }

    //
    // Build the structure to pass to Netlogon
    //

    RtlZeroMemory(
        &LogonGeneric,
        sizeof(LogonGeneric)
        );

    LogonGeneric.Identity.LogonDomainName = PassthroughRequest->DomainName;
    LogonGeneric.PackageName = PassthroughRequest->PackageName;
    LogonGeneric.LogonData = PassthroughRequest->LogonData;
    LogonGeneric.DataLength = PassthroughRequest->DataLength;

    LogonInformation =
        (PNETLOGON_LOGON_IDENTITY_INFO) &LogonGeneric;

    //
    // Call Netlogon to remote the request
    //

    //
    // Wait for NETLOGON to finish initialization.
    //

    if ( !NlpNetlogonInitialized ) {

        Status = NlWaitForNetlogon( NETLOGON_STARTUP_TIME );

        if ( !NT_SUCCESS(Status) ) {
            if ( Status != STATUS_NETLOGON_NOT_STARTED ) {
                goto Cleanup;
            }
        } else {
            NlpNetlogonInitialized = TRUE;
        }
    }

    if ( NlpNetlogonInitialized ) {
        Status = (*NlpNetLogonSamLogon)(
                    NULL,           // Server name
                    NULL,           // Computer name
                    NULL,           // Authenticator
                    NULL,           // ReturnAuthenticator
                    NetlogonGenericInformation,
                    (LPBYTE) &LogonInformation,
                    NetlogonValidationGenericInfo2,
                    (LPBYTE *) &ValidationGeneric,
                    &Authoritative );

        //
        // Reset Netlogon initialized flag if local netlogon cannot be
        //  reached.
        //  (Use a more explicit status code)
        //

        if ( Status == RPC_NT_SERVER_UNAVAILABLE ||
             Status == RPC_NT_UNKNOWN_IF ||
             Status == STATUS_NETLOGON_NOT_STARTED ) {
            Status = STATUS_NETLOGON_NOT_STARTED;
            NlpNetlogonInitialized = FALSE;

        }
    } else {

        //
        // no netlogon: see if the request is destined for the local domain,
        // to allow WORKGROUP support.
        //

        if (  LogonInformation->LogonDomainName.Length == 0 ||
             (LogonInformation->LogonDomainName.Length != 0 &&
              RtlEqualDomainName( &NlpSamDomainName,
                                     &LogonInformation->LogonDomainName ) )
            ) {


            PNETLOGON_GENERIC_INFO GenericInfo;
            NETLOGON_VALIDATION_GENERIC_INFO GenericValidation;
            NTSTATUS ProtocolStatus;

            GenericInfo = (PNETLOGON_GENERIC_INFO) LogonInformation;
            GenericValidation.ValidationData = NULL;
            GenericValidation.DataLength = 0;

            //
            // unwrap passthrough message and pass it off to dispatch.
            //

            Status = LsaICallPackagePassthrough(
                        &GenericInfo->PackageName,
                        0,  // Indicate pointers are relative.
                        GenericInfo->LogonData,
                        GenericInfo->DataLength,
                        (PVOID *) &GenericValidation.ValidationData,
                        &GenericValidation.DataLength,
                        &ProtocolStatus
                        );

            if(NT_SUCCESS( Status ) )
                Status = ProtocolStatus;


            //
            // If the call succeeded, allocate the return message.
            //

            if (NT_SUCCESS(Status)) {
                PNETLOGON_VALIDATION_GENERIC_INFO ReturnInfo;
                ULONG ValidationLength;

                ValidationLength = sizeof(*ReturnInfo) + GenericValidation.DataLength;

                ReturnInfo = (PNETLOGON_VALIDATION_GENERIC_INFO) MIDL_user_allocate(
                                ValidationLength
                                );

                if (ReturnInfo != NULL) {
                    if ( GenericValidation.DataLength == 0 ||
                         GenericValidation.ValidationData == NULL ) {
                        ReturnInfo->DataLength = 0;
                        ReturnInfo->ValidationData = NULL;
                    } else {

                        ReturnInfo->DataLength = GenericValidation.DataLength;
                        ReturnInfo->ValidationData = (PUCHAR) (ReturnInfo + 1);

                        RtlCopyMemory(
                            ReturnInfo->ValidationData,
                            GenericValidation.ValidationData,
                            ReturnInfo->DataLength );

                    }

                    ValidationGeneric = ReturnInfo;

                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                if (GenericValidation.ValidationData != NULL) {
                    LsaIFreeReturnBuffer(GenericValidation.ValidationData);
                }

            }


        } else {
            Status = STATUS_NETLOGON_NOT_STARTED;
        }
    }

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Allocate a buffer to return to the caller.
    //

    *ReturnBufferSize = sizeof(MSV1_0_PASSTHROUGH_RESPONSE) +
                        ValidationGeneric->DataLength;

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_PASSTHROUGH_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    PassthroughResponse = (PMSV1_0_PASSTHROUGH_RESPONSE) ClientBufferDesc.MsvBuffer;

    //
    // Fill in the return buffer.
    //

    PassthroughResponse->MessageType = MsV1_0GenericPassthrough;
    PassthroughResponse->DataLength = ValidationGeneric->DataLength;
    PassthroughResponse->ValidationData = ClientBufferDesc.UserBuffer + sizeof(MSV1_0_PASSTHROUGH_RESPONSE);


    RtlCopyMemory(
        PassthroughResponse + 1,
        ValidationGeneric->ValidationData,
        ValidationGeneric->DataLength
        );

    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );


Cleanup:

    if (ValidationGeneric != NULL) {
        MIDL_user_free(ValidationGeneric);
    }

    if ( !NT_SUCCESS(Status)) {
        NlpFreeClientBuffer( &ClientBufferDesc );
    }

    *ProtocolStatus = Status;
    return(STATUS_SUCCESS);

}


NTSTATUS
MspLm20CacheLogon (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0Lm20CacheLogon. It is called by
    a client wishing to cache logon information in the logon cache

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.




--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PMSV1_0_CACHE_LOGON_REQUEST CacheRequest;
    PNETLOGON_INTERACTIVE_INFO LogonInfo;
    PNETLOGON_VALIDATION_SAM_INFO2 ValidationInfo;

    PVOID SupplementalCacheData;
    ULONG SupplementalCacheDataLength;

    //
    // NOTE: this entry point only allows callers within the LSA process
    //

    if (ClientRequest != NULL) {
        *ProtocolStatus = STATUS_ACCESS_DENIED;
        return(STATUS_SUCCESS);
    }

    *ProtocolStatus = STATUS_SUCCESS;


    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_CACHE_LOGON_REQUEST_OLD) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    CacheRequest = (PMSV1_0_CACHE_LOGON_REQUEST) ProtocolSubmitBuffer;
    LogonInfo = (PNETLOGON_INTERACTIVE_INFO) CacheRequest->LogonInformation;
    ValidationInfo = (PNETLOGON_VALIDATION_SAM_INFO2) CacheRequest->ValidationInformation;

    SupplementalCacheData = CacheRequest->SupplementalCacheData;
    SupplementalCacheDataLength = CacheRequest->SupplementalCacheDataLength;

    //
    // Actually add the cache entry
    //

    *ProtocolStatus = NlpAddCacheEntry(
                        LogonInfo,
                        ValidationInfo,
                        SupplementalCacheData,
                        SupplementalCacheDataLength
                        );

Cleanup:

    return(STATUS_SUCCESS);

    UNREFERENCED_PARAMETER( ClientRequest);
    UNREFERENCED_PARAMETER( ProtocolReturnBuffer);
    UNREFERENCED_PARAMETER( ClientBufferBase);
    UNREFERENCED_PARAMETER( ReturnBufferSize);
}


NTSTATUS
MspLm20CacheLookup (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for LsaCallAuthenticationPackage()
    with a message type of MsV1_0Lm20CacheLookup. It is called by
    a client wishing to extract cache logon information and optionally
    verify the credential.

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.




--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PMSV1_0_CACHE_LOOKUP_REQUEST CacheRequest;
    PMSV1_0_CACHE_LOOKUP_RESPONSE CacheResponse;
    NETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    PNETLOGON_VALIDATION_SAM_INFO2 ValidationInfo = NULL;
    CACHE_PASSWORDS cachePasswords;
    CLIENT_BUFFER_DESC ClientBufferDesc;

    PNT_OWF_PASSWORD pNtOwfPassword = NULL;
    NT_OWF_PASSWORD ComputedNtOwfPassword;

    PVOID SupplementalCacheData = NULL;
    ULONG SupplementalCacheDataLength;

    //
    // Ensure the client is from the LSA process
    //

    if (ClientRequest != NULL) {
        *ProtocolStatus = STATUS_ACCESS_DENIED;
        return(STATUS_SUCCESS);
    }

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );


    *ProtocolStatus = STATUS_SUCCESS;


    //
    // Ensure the specified Submit Buffer is of reasonable size and
    // relocate all of the pointers to be relative to the LSA allocated
    // buffer.
    //

    if ( SubmitBufferSize < sizeof(MSV1_0_CACHE_LOOKUP_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    CacheRequest = (PMSV1_0_CACHE_LOOKUP_REQUEST) ProtocolSubmitBuffer;
    RtlZeroMemory(
        &LogonInfo,
        sizeof(LogonInfo)
        );

    //
    // NOTE: this submit call only supports in-process calls within the LSA
    // so buffers within the submit buffer are assumed to be valid and
    // hence not validated in the same way that out-proc calls are.
    //

    LogonInfo.LogonDomainName = CacheRequest->DomainName;
    LogonInfo.UserName = CacheRequest->UserName;



    if( CacheRequest->CredentialType != MSV1_0_CACHE_LOOKUP_CREDTYPE_NONE &&
        CacheRequest->CredentialType != MSV1_0_CACHE_LOOKUP_CREDTYPE_RAW &&
        CacheRequest->CredentialType != MSV1_0_CACHE_LOOKUP_CREDTYPE_NTOWF ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // get the cache entry
    //

    *ProtocolStatus = NlpGetCacheEntry(
                            &LogonInfo,
                            &ValidationInfo,
                            &cachePasswords,
                            &SupplementalCacheData,
                            &SupplementalCacheDataLength
                            );

    if (!NT_SUCCESS(*ProtocolStatus)) {
        goto Cleanup;
    }

    if( CacheRequest->CredentialType == MSV1_0_CACHE_LOOKUP_CREDTYPE_NONE ) {
        if( CacheRequest->CredentialInfoLength != 0 ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    //
    // verify the password, if necessary.
    //

    if( CacheRequest->CredentialType == MSV1_0_CACHE_LOOKUP_CREDTYPE_RAW ) {

        //
        // convert RAW to NTOWF.
        //

        UNICODE_STRING TempPassword;

        if( CacheRequest->CredentialInfoLength > 0xFFFF ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        TempPassword.Buffer = (PWSTR)&CacheRequest->CredentialSubmitBuffer;
        TempPassword.Length = (USHORT)CacheRequest->CredentialInfoLength;
        TempPassword.MaximumLength = TempPassword.Length;

        pNtOwfPassword = &ComputedNtOwfPassword;

        Status = RtlCalculateNtOwfPassword( &TempPassword, pNtOwfPassword );

        if( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }

        //
        // now, convert the request to NT_OWF style.
        //

        CacheRequest->CredentialType = MSV1_0_CACHE_LOOKUP_CREDTYPE_NTOWF;
        CacheRequest->CredentialInfoLength = sizeof( NT_OWF_PASSWORD );

    }

    if( CacheRequest->CredentialType == MSV1_0_CACHE_LOOKUP_CREDTYPE_NTOWF ) {
        if( CacheRequest->CredentialInfoLength != sizeof( NT_OWF_PASSWORD ) ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        if( !cachePasswords.SecretPasswords.NtPasswordPresent ) {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }

        if( pNtOwfPassword == NULL ) {
            pNtOwfPassword = (PNT_OWF_PASSWORD)&CacheRequest->CredentialSubmitBuffer;
        }

        Status = NlpComputeSaltedHashedPassword(
                    pNtOwfPassword,
                    pNtOwfPassword,
                    &ValidationInfo->EffectiveName
                    );

        if(!NT_SUCCESS( Status )) {
            goto Cleanup;
        }

        if(RtlCompareMemory(
                    pNtOwfPassword,
                    &cachePasswords.SecretPasswords.NtOwfPassword,
                    sizeof( NT_OWF_PASSWORD )
                    ) != sizeof(NT_OWF_PASSWORD) )
        {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
    }

    //
    // Return the validation info here.
    //

    *ReturnBufferSize = sizeof(MSV1_0_CACHE_LOOKUP_RESPONSE);

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_CACHE_LOOKUP_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    CacheResponse = (PMSV1_0_CACHE_LOOKUP_RESPONSE) ClientBufferDesc.MsvBuffer;

    //
    // Fill in the return buffer.
    //

    CacheResponse->MessageType = MsV1_0CacheLookup;
    CacheResponse->ValidationInformation = ValidationInfo;

    CacheResponse->SupplementalCacheData = SupplementalCacheData;
    CacheResponse->SupplementalCacheDataLength = SupplementalCacheDataLength;

    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );


Cleanup:

    if ( !NT_SUCCESS(Status)) {
        NlpFreeClientBuffer( &ClientBufferDesc );

        if (ValidationInfo != NULL) {
            MIDL_user_free( ValidationInfo );
        }

        if (SupplementalCacheData != NULL) {
            MIDL_user_free( SupplementalCacheData );
        }
    }

    ZeroMemory( &ComputedNtOwfPassword, sizeof( ComputedNtOwfPassword ) );
    ZeroMemory( &cachePasswords, sizeof(cachePasswords) );

    return(STATUS_SUCCESS);
    UNREFERENCED_PARAMETER( ClientBufferBase);

}



NTSTATUS
LsaApLogonUserEx2 (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    OUT PLUID LogonId,
    OUT PNTSTATUS SubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * SupplementalCredentials
    )

/*++

Routine Description:

    This routine is used to authenticate a user logon attempt.  This is
    the user's initial logon.  A new LSA logon session will be established
    for the user and validation information for the user will be returned.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    LogonType - Identifies the type of logon being attempted.

    ProtocolSubmitBuffer - Supplies the authentication
        information specific to the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the authentication information was resident.
        This may be necessary to fix-up any pointers within the
        authentication information buffer.

    SubmitBufferSize - Indicates the Size, in bytes,
        of the authentication information buffer.

    ProfileBuffer - Is used to return the address of the profile
        buffer in the client process.  The authentication package is
        responsible for allocating and returning the profile buffer
        within the client process.  However, if the LSA subsequently
        encounters an error which prevents a successful logon, then
        the LSA will take care of deallocating that buffer.  This
        buffer is expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

     ProfileBufferSize - Receives the Size (in bytes) of the
        returned profile buffer.

    SubStatus - If the logon failed due to account restrictions, the
        reason for the failure should be returned via this parameter.
        The reason is authentication-package specific.  The substatus
        values for authentication package "MSV1.0" are:

            STATUS_INVALID_LOGON_HOURS

            STATUS_INVALID_WORKSTATION

            STATUS_PASSWORD_EXPIRED

            STATUS_ACCOUNT_DISABLED

    TokenInformationLevel - If the logon is successful, this field is
        used to indicate what level of information is being returned
        for inclusion in the Token to be created.  This information
        is returned via the TokenInformation parameter.

    TokenInformation - If the logon is successful, this parameter is
        used by the authentication package to return information to
        be included in the token.  The format and content of the
        buffer returned is indicated by the TokenInformationLevel
        return value.

    AccountName - A Unicode string describing the account name
        being logged on to.  This parameter must always be returned
        regardless of the success or failure of the operation.

    AuthenticatingAuthority - A Unicode string describing the Authenticating
        Authority for the logon.  This string may optionally be omitted.

    PrimaryCredentials - Returns primary credentials for handing to other
        packages.

    SupplementalCredentials - Array of supplemental credential blobs for
        other packages.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the logon
        could not be completed because the client does not have
        sufficient quota to allocate the return buffer.

    STATUS_NO_LOGON_SERVERS - Indicates that no domain controllers
        are currently able to service the authentication request.

    STATUS_LOGON_FAILURE - Indicates the logon attempt failed.  No
        indication as to the reason for failure is given, but typical
        reasons include mispelled usernames, mispelled passwords.

    STATUS_ACCOUNT_RESTRICTION - Indicates the user account and
        password were legitimate, but that the user account has some
        restriction preventing successful logon at this time.

    STATUS_BAD_VALIDATION_CLASS - The authentication information
        provided is not a validation class known to the specified
        authentication package.

    STATUS_INVALID_LOGON_CLASS - LogonType was invalid.

    STATUS_LOGON_SESSION_COLLISION - Internal Error: A LogonId was selected for
        this logon session.  The selected LogonId already exists.

    STATUS_NETLOGON_NOT_STARTED - The Sam Server or Netlogon service was
        required to perform this function.  The required server was not running.

    STATUS_NO_MEMORY - Insufficient virtual memory or pagefile quota exists.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LSA_TOKEN_INFORMATION_TYPE LsaTokenInformationType = LsaTokenInformationV2;

    PNETLOGON_VALIDATION_SAM_INFO2 NlpUser = NULL;


    PACTIVE_LOGON LogonEntry = NULL;
    BOOLEAN LogonEntryLinked = FALSE;

    BOOLEAN LogonSessionCreated = FALSE;
    BOOLEAN LogonCredentialAdded = FALSE;
    ULONG Flags = 0;
    BOOLEAN Authoritative;
    BOOLEAN BadPasswordCountZeroed;
    BOOLEAN StandaloneWorkstation = FALSE;

    PSID UserSid = NULL;

    PMSV1_0_PRIMARY_CREDENTIAL Credential = NULL;
    ULONG CredentialSize;

    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR Seed;

    PUNICODE_STRING WorkStationName = NULL;

    // Need to figure out whether to delete the profile  buffer

    BOOLEAN fSubAuthEx = FALSE;

    //
    // deferred NTLM3 checks.
    //

    BOOLEAN fNtLm3 = FALSE;

    //
    // Temporary storage while we try to figure
    // out what our username and authenticating
    // authority is.
    //

    UNICODE_STRING TmpName;
    WCHAR TmpNameBuffer[UNLEN];
    UNICODE_STRING TmpAuthority;
    WCHAR TmpAuthorityBuffer[DNS_MAX_NAME_LENGTH];

    //
    // Logon Information.
    //
    NETLOGON_LOGON_INFO_CLASS LogonLevel;
    NETLOGON_INTERACTIVE_INFO LogonInteractive;
    NETLOGON_NETWORK_INFO LogonNetwork;
    PNETLOGON_LOGON_IDENTITY_INFO LogonInformation;

    PMSV1_0_LM20_LOGON NetworkAuthentication = NULL;

    //
    // Secret information, if we are doing a service logon
    //
    LSAPR_HANDLE PolicyHandle = NULL, SecretHandle;
    PLSAPR_CR_CIPHER_VALUE SecretCurrent = NULL;
    UNICODE_STRING Prefix, SavedPassword;
    BOOLEAN ServiceSecretLogon = FALSE;
    PMSV1_0_INTERACTIVE_LOGON Authentication = NULL;



    //
    // Initialize
    //

    *ProfileBuffer = NULL;
    *SubStatus = STATUS_SUCCESS;
    *AuthenticatingAuthority = NULL;
    *AccountName = NULL;

    TmpName.Buffer        = TmpNameBuffer;
    TmpName.MaximumLength = UNLEN * sizeof( WCHAR );
    TmpName.Length        = 0;

    TmpAuthority.Buffer        = TmpAuthorityBuffer;
    TmpAuthority.MaximumLength = DNS_MAX_NAME_LENGTH * sizeof( WCHAR );
    TmpAuthority.Length        = 0;

    *SupplementalCredentials = 0;

    RtlZeroMemory(
        PrimaryCredentials,
        sizeof(SECPKG_PRIMARY_CRED)
        );

    //
    // Check the Authentication information and build a LogonInformation
    // structure to pass to SAM or Netlogon.
    //
    // NOTE: Netlogon treats Service and Batch logons as if they are
    //       Interactive.
    //

    switch ( LogonType ) {
    case Service:
    case Interactive:
    case Batch:
    case NetworkCleartext:
        {
            MSV1_0_PRIMARY_CREDENTIAL BuiltCredential;

            WorkStationName = &NlpComputerName;

            //
            // Ensure this is really an interactive logon.
            //

            Authentication =
                (PMSV1_0_INTERACTIVE_LOGON) ProtocolSubmitBuffer;

            if ( (Authentication->MessageType != MsV1_0InteractiveLogon ) &&
                 (Authentication->MessageType != MsV1_0WorkstationUnlockLogon) ) {
                KdPrint(("MSV1_0: LsaApLogonUser: Bad Validation Class %d\n", Authentication->MessageType));
                Status = STATUS_BAD_VALIDATION_CLASS;
                goto Cleanup;
            }



            //
            // If the password length is greater than 255 (i.e., the
            // upper byte of the length is non-zero) then the password
            // has been run-encoded for privacy reasons.  Get the
            // run-encode seed out of the upper-byte of the length
            // for later use.
            //
            //



            SeedAndLength = (PSECURITY_SEED_AND_LENGTH)
                            &Authentication->Password.Length;
            Seed = SeedAndLength->Seed;
            SeedAndLength->Seed = 0;

            //
            // Enforce length restrictions on username and password.
            //

            if ( Authentication->UserName.Length > (UNLEN*sizeof(WCHAR)) ||
                Authentication->Password.Length > (PWLEN*sizeof(WCHAR)) ) {
                KdPrint(("MSV1_0: LsaApLogonUser: Name or password too long\n"));
                Status = STATUS_NAME_TOO_LONG;
                goto Cleanup;
            }


            //
            // Relocate any pointers to be relative to 'Authentication'
            //

            NULL_RELOCATE_ONE( &Authentication->LogonDomainName );

            RELOCATE_ONE( &Authentication->UserName );

            NULL_RELOCATE_ONE( &Authentication->Password );


            if ( LogonType == Service ) {

                //
                // If we have a service logon, the password we got is likely the name of the secret
                // that is holding the account password.  Make sure to read that secret here
                //
                RtlInitUnicodeString( &Prefix, L"_SC_" );
                if ( RtlPrefixUnicodeString( &Prefix, &Authentication->Password, TRUE ) ) {

                    Status = LsaIOpenPolicyTrusted( &PolicyHandle );

                    if ( NT_SUCCESS( Status ) ) {

                        Status = LsarOpenSecret( PolicyHandle,
                                                 ( PLSAPR_UNICODE_STRING )&Authentication->Password,
                                                 SECRET_QUERY_VALUE,
                                                 &SecretHandle );

                        if ( NT_SUCCESS( Status ) ) {

                            Status = LsarQuerySecret( SecretHandle,
                                                      &SecretCurrent,
                                                      NULL,
                                                      NULL,
                                                      NULL );

                            if ( NT_SUCCESS( Status ) && (SecretCurrent != NULL) ) {

                                RtlCopyMemory( &SavedPassword,
                                               &Authentication->Password,
                                               sizeof( UNICODE_STRING ) );
                                Authentication->Password.Length = ( USHORT )SecretCurrent->Length;
                                Authentication->Password.MaximumLength =
                                                                  ( USHORT )SecretCurrent->MaximumLength;
                                Authentication->Password.Buffer = ( USHORT * )SecretCurrent->Buffer;
                                ServiceSecretLogon = TRUE;
                            }

                            LsarClose( &SecretHandle );
                        }

                        LsarClose( &PolicyHandle );
                    }

                }


                if ( !NT_SUCCESS( Status ) ) {

                    goto Cleanup;
                }

            }
            //
            // Now decode the password, if necessary
            //

            if (Seed != 0 ) {
                try {
                    RtlRunDecodeUnicodeString( Seed, &Authentication->Password);
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = STATUS_ILL_FORMED_PASSWORD;
                    goto Cleanup;
                }
            }

            //
            // Copy out the user name and Authenticating Authority so we can audit them.
            //

            RtlCopyUnicodeString( &TmpName, &Authentication->UserName );

            if ( Authentication->LogonDomainName.Buffer != NULL ) {

                RtlCopyUnicodeString( &TmpAuthority, &Authentication->LogonDomainName );
            }


            //
            // Put the password in the PrimaryCredential to pass to the sundry security packages.
            //

            PrimaryCredentials->Password.Length = PrimaryCredentials->Password.MaximumLength =
                Authentication->Password.Length;
            PrimaryCredentials->Password.Buffer = (*Lsa.AllocateLsaHeap)(Authentication->Password.Length);

            if (PrimaryCredentials->Password.Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory(
                PrimaryCredentials->Password.Buffer,
                Authentication->Password.Buffer,
                Authentication->Password.Length
                );
            PrimaryCredentials->Flags = PRIMARY_CRED_CLEAR_PASSWORD;


            //
            // We're all done with the cleartext password
            //  Don't let it get to the pagefile.
            //

            try {
                if ( Authentication->Password.Buffer != NULL ) {
                    RtlEraseUnicodeString( &Authentication->Password );
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = STATUS_ILL_FORMED_PASSWORD;
                goto Cleanup;
            }

            //
            // Compute the OWF of the password.
            //

            NlpPutOwfsInPrimaryCredential( &PrimaryCredentials->Password,
                                           &BuiltCredential );


            //
            // Define the description of the user to log on.
            //
            LogonLevel = NetlogonInteractiveInformation;
            LogonInformation =
                (PNETLOGON_LOGON_IDENTITY_INFO) &LogonInteractive;

            LogonInteractive.Identity.LogonDomainName =
                Authentication->LogonDomainName;
            LogonInteractive.Identity.ParameterControl = 0;

            LogonInteractive.Identity.UserName = Authentication->UserName;
            LogonInteractive.Identity.Workstation = NlpComputerName;


            LogonInteractive.LmOwfPassword = BuiltCredential.LmOwfPassword;
            LogonInteractive.NtOwfPassword = BuiltCredential.NtOwfPassword;

        }

        break;

    case Network:
        {
            PMSV1_0_LM20_LOGON Authentication;

            //
            // Ensure this is really a network logon request.
            //

            Authentication =
                (PMSV1_0_LM20_LOGON) ProtocolSubmitBuffer;


            NetworkAuthentication = Authentication;

            if ( Authentication->MessageType != MsV1_0Lm20Logon &&
                 Authentication->MessageType != MsV1_0SubAuthLogon  &&
                 Authentication->MessageType != MsV1_0NetworkLogon ) {
                KdPrint(("MSV1_0: LsaApLogonUser: Bad Validation Class\n"));
                Status = STATUS_BAD_VALIDATION_CLASS;
                goto Cleanup;
            }


            //
            // Relocate any pointers to be relative to 'Authentication'
            //

            NULL_RELOCATE_ONE( &Authentication->LogonDomainName );

            NULL_RELOCATE_ONE( &Authentication->UserName );

            RELOCATE_ONE( &Authentication->Workstation );

            //
            // Copy out the user name and Authenticating Authority so we can audit them.
            //

            if ( Authentication->UserName.Buffer != NULL ) {

                RtlCopyUnicodeString( &TmpName, &Authentication->UserName );
            }

            if ( Authentication->LogonDomainName.Buffer != NULL ) {

                RtlCopyUnicodeString( &TmpAuthority, &Authentication->LogonDomainName );
            }

            NULL_RELOCATE_ONE((PUNICODE_STRING)&Authentication->CaseSensitiveChallengeResponse );

            NULL_RELOCATE_ONE((PUNICODE_STRING)&Authentication->CaseInsensitiveChallengeResponse );


            //
            // Define the description of the user to log on.
            //
            LogonLevel = NetlogonNetworkInformation;
            LogonInformation =
                (PNETLOGON_LOGON_IDENTITY_INFO) &LogonNetwork;

            LogonNetwork.Identity.LogonDomainName =
                Authentication->LogonDomainName;

            if ( Authentication->MessageType == MsV1_0Lm20Logon ) {
                LogonNetwork.Identity.ParameterControl = MSV1_0_CLEARTEXT_PASSWORD_ALLOWED;
            } else {
                ASSERT( CLEARTEXT_PASSWORD_ALLOWED == MSV1_0_CLEARTEXT_PASSWORD_ALLOWED );
                LogonNetwork.Identity.ParameterControl =
                    Authentication->ParameterControl;

                // For NT 5.0 SubAuth Packages, there is a SubAuthPackageId. Stuff
                // that into ParameterControl so pre 5.0 MsvSamValidate won't choke.

                if ( Authentication->MessageType == MsV1_0SubAuthLogon )
                {
                    PMSV1_0_SUBAUTH_LOGON SubAuthentication =
                        (PMSV1_0_SUBAUTH_LOGON)  ProtocolSubmitBuffer;

                    // Need to not delete return buffers even in case of error
                    // for MsV1_0SubAuthLogon (includes arap).

                    fSubAuthEx = TRUE;

                    LogonNetwork.Identity.ParameterControl |=
                        (SubAuthentication->SubAuthPackageId << MSV1_0_SUBAUTHENTICATION_DLL_SHIFT) | MSV1_0_SUBAUTHENTICATION_DLL_EX;
                }
            }


            LogonNetwork.Identity.UserName = Authentication->UserName;
            LogonNetwork.Identity.Workstation = Authentication->Workstation;

            WorkStationName = &Authentication->Workstation;

            LogonNetwork.NtChallengeResponse =
                Authentication->CaseSensitiveChallengeResponse;
            LogonNetwork.LmChallengeResponse =
                Authentication->CaseInsensitiveChallengeResponse;
            ASSERT( LM_CHALLENGE_LENGTH ==
                    sizeof(Authentication->ChallengeToClient) );


            //
            // If using client challenge, then mix it with the server's challenge
            //  to get the challenge we pass on. It would make more sense to do this
            //  in MsvpPasswordValidate, except that would require the DCs to be upgraded.
            //  Doing it here only requires agreement between the client and server, because
            //  the modified challenge will be passed on to the DCs.
            //

            if ((Authentication->ParameterControl & MSV1_0_USE_CLIENT_CHALLENGE) &&
                (Authentication->CaseSensitiveChallengeResponse.Length == NT_RESPONSE_LENGTH) &&
                (Authentication->CaseInsensitiveChallengeResponse.Length >= MSV1_0_CHALLENGE_LENGTH))
            {
                MsvpCalculateNtlm2Challenge (
                    Authentication->ChallengeToClient,
                    Authentication->CaseInsensitiveChallengeResponse.Buffer,
                    (PUCHAR)&LogonNetwork.LmChallenge
                    );

            } else {
                RtlCopyMemory(
                    &LogonNetwork.LmChallenge,
                    Authentication->ChallengeToClient,
                    LM_CHALLENGE_LENGTH );
            }

            //
            // if using NTLM3, then check that the target info is for this machine.
            //

            if ((Authentication->ParameterControl & MSV1_0_USE_CLIENT_CHALLENGE) &&
                (Authentication->CaseSensitiveChallengeResponse.Length >= sizeof(MSV1_0_NTLM3_RESPONSE)))
            {

                fNtLm3 = TRUE;

                //
                // defer NTLM3 checks until later on when SAM initialized.
                //

            }



            //
            // Enforce length restrictions on username
            //

            if ( Authentication->UserName.Length > (UNLEN*sizeof(WCHAR)) ) {
                KdPrint(("MSV1_0: LsaApLogonUser: Name too long\n"));
                Status = STATUS_NAME_TOO_LONG;
                goto Cleanup;
            }

            //
            // If this is a null session logon,
            //  just build a NULL token.
            //

            if ( Authentication->UserName.Length == 0 &&
                 Authentication->CaseSensitiveChallengeResponse.Length == 0 &&
                 (Authentication->CaseInsensitiveChallengeResponse.Length == 0 ||
                  (Authentication->CaseInsensitiveChallengeResponse.Length == 1 &&
                  *Authentication->CaseInsensitiveChallengeResponse.Buffer == '\0') ) ) {

                LsaTokenInformationType = LsaTokenInformationNull;
            }
        }

        break;

    default:
        return STATUS_INVALID_LOGON_TYPE;

    }


    //
    // Allocate a LogonId for this logon session.
    //

    Status = NtAllocateLocallyUniqueId( LogonId );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    NEW_TO_OLD_LARGE_INTEGER( (*LogonId), LogonInformation->LogonId );


    PrimaryCredentials->LogonId = *LogonId;

    //
    // Create a new logon session
    //

    Status = (*Lsa.CreateLogonSession)( LogonId );
    if( !NT_SUCCESS(Status) ) {
        KdPrint(( "MSV1_0: LsaApLogonUser: Collision from CreateLogonSession\n"));
        goto Cleanup;
    }

    LogonSessionCreated = TRUE;


    //
    // Don't worry about SAM or the LSA if this is a Null Session logon.
    //
    // The server does a Null Session logon during initialization.
    // It shouldn't have to wait for SAM to initialize.
    //

    if ( LsaTokenInformationType != LsaTokenInformationNull ) {

        //
        // If Sam is not yet initialized,
        //  do it now.
        //

        if ( !NlpSamInitialized ) {
            Status = NlSamInitialize( SAM_STARTUP_TIME );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }
        }

        //
        // If this is a workstation,
        //  differentiate between a standalone workstation and a member
        //  workstation.
        //
        // (This is is done on every logon, rather than during initialization,
        // to allow the value to be changed via the UI).
        //

        if ( NlpWorkstation && NlpPolicyHandle != NULL ) {
#if 0
            PLSAPR_POLICY_INFORMATION PolicyPrimaryDomainInfo = NULL;

            Status = I_LsarQueryInformationPolicy(
                        NlpPolicyHandle,
                        PolicyPrimaryDomainInformation,
                        &PolicyPrimaryDomainInfo );

            if ( NT_SUCCESS(Status) ) {
                StandaloneWorkstation =
                    (PolicyPrimaryDomainInfo->PolicyPrimaryDomainInfo.Sid == NULL);

                I_LsaIFree_LSAPR_POLICY_INFORMATION( PolicyPrimaryDomainInformation,
                                                     PolicyPrimaryDomainInfo );

            } else {
                StandaloneWorkstation = FALSE;
            }
#endif
///            EnterCriticalSection(&NtLmGlobalCritSect);
            StandaloneWorkstation = (NtLmGlobalTargetFlags == NTLMSSP_TARGET_TYPE_SERVER);
///            LeaveCriticalSection(&NtLmGlobalCritSect);

        } else {
            StandaloneWorkstation = FALSE;
        }
    }

    //
    // Try again to load netlogon.dll
    //
    if ( NlpNetlogonDllHandle == NULL ) {
        NlpLoadNetlogonDll();
    }

    //
    // do NTLM3 processing that was deferred until now due to initialization
    // requirements.
    //

    if( fNtLm3 )
    {

        PMSV1_0_AV_PAIR pAV;
        PMSV1_0_NTLM3_RESPONSE pResp;
        LONG iRespLen;

        ULONG NtLmProtocolSupported = NtLmGlobalLmProtocolSupported;



        //
        // get the computer name from the response
        //

        pResp = (PMSV1_0_NTLM3_RESPONSE)
            NetworkAuthentication->CaseSensitiveChallengeResponse.Buffer;
        iRespLen = NetworkAuthentication->CaseSensitiveChallengeResponse.Length -
            sizeof(MSV1_0_NTLM3_RESPONSE);

        pAV = MsvpAvlGet((PMSV1_0_AV_PAIR)pResp->Buffer, MsvAvNbComputerName, iRespLen);

        //
        // if there is one (OK to be missing), see that it is us
        // REVIEW -- only allow it to be missing if registry says OK?
        // BUGBUG: dns forms
        //

        if (pAV) {
            if ((NlpComputerName.Length != pAV->AvLen) ||
                (!RtlEqualMemory(NlpComputerName.Buffer, pAV+1, pAV->AvLen)) ) {
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }
        } else if (NtLmProtocolSupported >= RefuseNtlm3NoTarget) {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }


        //
        // get the domain name from the response
        //

        pAV = MsvpAvlGet((PMSV1_0_AV_PAIR)pResp->Buffer, MsvAvNbDomainName, iRespLen);

        //
        // must exist and must be us.
        //

        if (pAV) {

            UNICODE_STRING Candidate;

            Candidate.Buffer = (PWSTR)(pAV+1);
            Candidate.Length = pAV->AvLen;
            Candidate.MaximumLength = pAV->AvLen;


            if( StandaloneWorkstation ) {
                if( !RtlEqualDomainName(&NlpComputerName, &Candidate) ) {
                    Status = STATUS_LOGON_FAILURE;
                    goto Cleanup;
                }

            } else {
                if( !RtlEqualDomainName(&NlpPrimaryDomainName, &Candidate) ) {
                    Status = STATUS_LOGON_FAILURE;
                    goto Cleanup;
                }
            }

        } else {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
    }


    //
    // Do the actual logon now.
    //
    //
    // If a null token is being built,
    //  don't authenticate at all.
    //

    if ( LsaTokenInformationType == LsaTokenInformationNull ) {

        /* Nothing to do here. */


    //
    // Call Sam directly to get the validation information when:
    //
    //  The network is not installed, OR
    //  This is a standalone workstation (not a member of a domain).
    //  This is a workstation and we're logging onto an account on the
    //      workstation.
    //



    } else if ( NlpNetlogonDllHandle == NULL || !NlpLanmanInstalled ||
       StandaloneWorkstation ||
       ( NlpWorkstation &&
         LogonInformation->LogonDomainName.Length != 0 &&
         RtlEqualDomainName( &NlpSamDomainName,
                             &LogonInformation->LogonDomainName )) ) {

        // Allow guest logons only

        DWORD AccountsToTry = MSVSAM_SPECIFIED | MSVSAM_GUEST;

        if ((LogonType == Network) &&
            (LogonNetwork.Identity.ParameterControl & MSV1_0_TRY_GUEST_ACCOUNT_ONLY))
        {
            AccountsToTry = MSVSAM_GUEST;
        }

        //
        // Get the Validation information from the local SAM database
        //


        Status = MsvSamValidate(
                    NlpSamDomainHandle,
                    NlpUasCompatibilityRequired,
                    MsvApSecureChannel,
                    &NlpComputerName,   // Logon Server is this machine
                    &NlpSamDomainName,
                    NlpSamDomainId,
                    LogonLevel,
                    LogonInformation,
                    NetlogonValidationSamInfo2,
                    (PVOID *) &NlpUser,
                    &Authoritative,
                    &BadPasswordCountZeroed,
                    AccountsToTry);

        if ( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }

        // So we don't get a LOGON COLLISION from the old msv package

        Flags |= LOGON_BY_LOCAL;

    //
    // If we couldn't validate via one of the above mechanisms,
    //  call the local Netlogon service to get the validation information.
    //

    } else {

        //
        // Wait for NETLOGON to finish initialization.
        //

        if ( !NlpNetlogonInitialized ) {

            Status = NlWaitForNetlogon( NETLOGON_STARTUP_TIME );

            if ( !NT_SUCCESS(Status) ) {
                if ( Status != STATUS_NETLOGON_NOT_STARTED ) {
                    goto Cleanup;
                }
            } else {
                NlpNetlogonInitialized = TRUE;
            }
        }

        //
        // Actually call the netlogon service.
        //

        if ( NlpNetlogonInitialized ) {

            Status = (*NlpNetLogonSamLogon)(
                        NULL,           // Server name
                        NULL,           // Computer name
                        NULL,           // Authenticator
                        NULL,           // ReturnAuthenticator
                        LogonLevel,
                        (LPBYTE) &LogonInformation,
                        NetlogonValidationSamInfo2,
                        (LPBYTE *) &NlpUser,
                        &Authoritative );

            //
            // Reset Netlogon initialized flag if local netlogon cannot be
            //  reached.
            //  (Use a more explicit status code)
            //

            if( !NT_SUCCESS(Status) )
            {
                switch (Status)
                {
                    //
                    // for documented errors that netlogon can return
                    // for authoritative failures, leave the status code as-is.
                    //

                    case STATUS_NO_TRUST_LSA_SECRET:
                    case STATUS_TRUSTED_DOMAIN_FAILURE:
                    case STATUS_INVALID_INFO_CLASS:
                    case STATUS_TRUSTED_RELATIONSHIP_FAILURE:
                    case STATUS_ACCESS_DENIED:
                    case STATUS_NO_SUCH_USER:
                    case STATUS_WRONG_PASSWORD:
                    case STATUS_INVALID_LOGON_HOURS:
                    case STATUS_PASSWORD_EXPIRED:
                    case STATUS_ACCOUNT_DISABLED:
                    case STATUS_INVALID_PARAMETER:
                    case STATUS_PASSWORD_MUST_CHANGE:
                    case STATUS_ACCOUNT_EXPIRED:
                    {
                        break;
                    }

                    //
                    // for errors that are known to occur during unexpected
                    // conditions, over-ride status to allow cache lookup.
                    //

                    case RPC_NT_SERVER_UNAVAILABLE:
                    case RPC_NT_UNKNOWN_IF:
                    case STATUS_NETLOGON_NOT_STARTED:
                    {
                        Status = STATUS_NETLOGON_NOT_STARTED;
                        NlpNetlogonInitialized = FALSE;
                        break;
                    }

                    // default will catch a host of RPC related errors.
                    // some mentioned below.
                    //case EPT_NT_NOT_REGISTERED:
                    //case RPC_NT_CALL_FAILED_DNE:
                    //case RPC_NT_SERVER_TOO_BUSY:
                    //case RPC_NT_CALL_FAILED:
                    default:
                    {
                        Status = STATUS_NETLOGON_NOT_STARTED;
                        NlpNetlogonInitialized = FALSE;
                        break;
                    }
                }
            }
        }


        //
        // If this is the requested domain,
        //  go directly to SAM if the netlogon service isn't available.
        //
        // We want to go to the netlogon service if it is available since it
        // does special handling of bad passwords and account lockout.  However,
        // if the netlogon service is down, the local SAM database makes a
        // better cache than any other mechanism.
        //

        if ( !NlpNetlogonInitialized &&
              LogonInformation->LogonDomainName.Length != 0 &&
              RtlEqualDomainName( &NlpSamDomainName,
                                     &LogonInformation->LogonDomainName ) ) {

            // Allow guest logons only

            DWORD AccountsToTry = MSVSAM_SPECIFIED | MSVSAM_GUEST;

            if ((LogonType == Network) &&
                (LogonNetwork.Identity.ParameterControl & MSV1_0_TRY_GUEST_ACCOUNT_ONLY))
            {
                AccountsToTry = MSVSAM_GUEST;
            }

            //
            // Get the Validation information from the local SAM database
            //

            Status = MsvSamValidate(
                        NlpSamDomainHandle,
                        NlpUasCompatibilityRequired,
                        MsvApSecureChannel,
                        &NlpComputerName,   // Logon Server is this machine
                        &NlpSamDomainName,
                        NlpSamDomainId,
                        LogonLevel,
                        LogonInformation,
                        NetlogonValidationSamInfo2,
                        (PVOID *) &NlpUser,
                        &Authoritative,
                        &BadPasswordCountZeroed,
                        AccountsToTry);

            if ( !NT_SUCCESS( Status ) ) {
                goto Cleanup;
            }

            // So we don't get a LOGON COLLISION from the old msv package

            Flags |= LOGON_BY_LOCAL;


        //
        // If Netlogon was successful,
        //  add this user to the logon cache.
        //

        } else if ( NT_SUCCESS( Status ) ) {

            //
            // Indicate this session was validated by the Netlogon
            //  service.
            //

            Flags |= LOGON_BY_NETLOGON;

            //
            // Cache interactive logon information.
            //
            //      NOTE: Batch and Service logons are treated
            //            the same as Interactive here.
            //

            if (LogonType == Interactive ||
                LogonType == Service ||
                LogonType == Batch) {

                NTSTATUS ntStatus;

                ntStatus = NlpAddCacheEntry(&LogonInteractive, NlpUser, NULL, 0);
            }

        //
        // If Netlogon is simply not available at this time,
        //  try to logon through the cache.
        //
        // STATUS_NO_LOGON_SERVERS indicates the netlogon service couldn't
        //  contact a DC to handle this request.
        //
        // STATUS_NETLOGON_NOT_STARTED indicates the local netlogon service
        //  isn't running.
        //
        //
        // Even though we change the cache only for interactive logons,
        // we use the cache for ANY logon type.  This not only allows a
        // user to logon interactively, but it allows that same user to
        // connect from another machine while the DC is down.
        //

        } else if ( Status == STATUS_NO_LOGON_SERVERS ||
                    Status == STATUS_NETLOGON_NOT_STARTED ) {

            NTSTATUS ntStatus;
            CACHE_PASSWORDS cachePasswords;
            ULONG LocalFlags = 0;

            //
            // Try to logon via the cache.
            //
            //

            ntStatus = NlpGetCacheEntry(LogonInformation, &NlpUser, &cachePasswords, NULL, NULL);

            if (!NT_SUCCESS(ntStatus)) {

                //
                // The original status code is more interesting than
                // the fact that the cache didn't work.
                //

                NlpUser = NULL;     // NlpGetCacheEntry dirties this
                goto Cleanup;
            }

            if( LogonType != Network )
            {

                //
                // The cache information contains salted hashed passwords,
                // so modify the logon information similarly.
                //

                ntStatus = NlpComputeSaltedHashedPassword(
                            &LogonInteractive.NtOwfPassword,
                            &LogonInteractive.NtOwfPassword,
                            &NlpUser->EffectiveName
                            );
                if (!NT_SUCCESS(ntStatus)) {
                    goto Cleanup;
                }

                ntStatus = NlpComputeSaltedHashedPassword(
                            &LogonInteractive.LmOwfPassword,
                            &LogonInteractive.LmOwfPassword,
                            &NlpUser->EffectiveName
                            );
                if (!NT_SUCCESS(ntStatus)) {
                    goto Cleanup;
                }

            } else {

                PMSV1_0_PRIMARY_CREDENTIAL TempPrimaryCredential;
                ULONG PrimaryCredentialSize;

                //
                // because the cache no longer stores OWFs, the cached salted OWF
                // is not useful for validation for network logon.
                // The only place we can get a OWF to match is the active logon
                // cache
                //

                ntStatus = NlpGetPrimaryCredentialByUserDomain(
                                &LogonInformation->LogonDomainName,
                                &LogonInformation->UserName,
                                &TempPrimaryCredential,
                                &PrimaryCredentialSize
                                );

                if(!NT_SUCCESS(ntStatus)) {
                    Status = STATUS_WRONG_PASSWORD;
                    goto Cleanup;
                }

                //
                // copy out the OWFs, then free the allocated buffer.
                //

                if( TempPrimaryCredential->NtPasswordPresent ) {
                    CopyMemory(&cachePasswords.SecretPasswords.NtOwfPassword, &TempPrimaryCredential->NtOwfPassword, sizeof(NT_OWF_PASSWORD));
                    cachePasswords.SecretPasswords.NtPasswordPresent = TRUE;
                } else {
                    cachePasswords.SecretPasswords.NtPasswordPresent = FALSE;
                }

                if( TempPrimaryCredential->LmPasswordPresent ) {
                    CopyMemory(&cachePasswords.SecretPasswords.LmOwfPassword, &TempPrimaryCredential->LmOwfPassword, sizeof(LM_OWF_PASSWORD));
                    cachePasswords.SecretPasswords.LmPasswordPresent = TRUE;
                } else {
                    cachePasswords.SecretPasswords.LmPasswordPresent = FALSE;
                }

                ZeroMemory( TempPrimaryCredential, PrimaryCredentialSize );
                (*Lsa.FreeLsaHeap)( TempPrimaryCredential );

            }


            //
            // Now we have the information from the cache, validate the
            // user's password
            //

            if (!MsvpPasswordValidate(
                    NlpUasCompatibilityRequired,
                    LogonLevel,
                    (PVOID)LogonInformation,
                    &cachePasswords.SecretPasswords,
                    &LocalFlags,
                    &NlpUser->UserSessionKey,
                    (PLM_SESSION_KEY)
                        &NlpUser->ExpansionRoom[SAMINFO_LM_SESSION_KEY]
                    )) {
                Status = STATUS_WRONG_PASSWORD;
                goto Cleanup;
            }

            Status = STATUS_SUCCESS;

            //
            // The cache always returns a NETLOGONV_VALIDATION_SAM_INFO2
            // structure so set the LOGON_EXTRA_SIDS flag, whether or not
            // there are extra sids. Also, if there was a package ID indicated
            // put it in the PrimaryCredentials and remove it from the
            // NlpUser structure so it doesn't confuse anyone else.
            //

            PrimaryCredentials->Flags |= NlpUser->UserFlags & PRIMARY_CRED_PACKAGE_MASK;
            NlpUser->UserFlags &= ~PRIMARY_CRED_PACKAGE_MASK;
            NlpUser->UserFlags |= LOGON_CACHED_ACCOUNT | LOGON_EXTRA_SIDS | LocalFlags;
            Flags |= LOGON_BY_CACHE;

        //
        // If the account is permanently dead on the domain controller,
        //  Flush this entry from the cache.
        //
        // Notice that STATUS_INVALID_LOGON_HOURS is not in the list below.
        // This ensures a user will be able to remove his portable machine
        // from the net and use it after hours.
        //
        // Notice the STATUS_WRONG_PASSWORD is not in the list below.
        // We're as likely to flush the cache for typo'd passwords as anything
        // else.  What we'd really like to do is flush the cache if the
        // password on the DC is different than the one in cache; but that's
        // impossible to detect.
        //
        // ONLY DO THIS FOR INTERACTIVE LOGONS
        // (not Service or Batch).
        //

        } else if ( LogonType == Interactive                &&
                    (Status == STATUS_NO_SUCH_USER          ||
                     Status == STATUS_INVALID_WORKSTATION   ||
                     Status == STATUS_PASSWORD_EXPIRED      ||
                     Status == STATUS_ACCOUNT_DISABLED) ) {

            //
            // Delete the cache entry

            NTSTATUS ntStatus;

            ntStatus = NlpDeleteCacheEntry(&LogonInteractive);
            KdPrint(("MSV1_0: LsaApLogonUser: NlpDeleteCacheEntry returns %x\n", ntStatus));

            goto Cleanup;

        } else {

            goto Cleanup;
        }
    }



    //
    // For everything except network logons,
    //  save the credentials in the LSA,
    //  create active logon table entry,
    //  return the interactive profile buffer.
    //

    if ( LogonType == Interactive ||
         LogonType == Service     ||
         LogonType == Batch       ||
         LogonType == NetworkCleartext
       ) {
        PACTIVE_LOGON *ActiveLogon;
        ULONG LogonEntrySize;
        PUCHAR Where;
        USHORT LogonCount;
        ULONG UserSidSize;
        UNICODE_STRING SamAccountName;
        UNICODE_STRING NetbiosDomainName;
        UNICODE_STRING DnsDomainName;
        UNICODE_STRING Upn;
        UNICODE_STRING LogonServer;

        //
        // Grab the various forms of the account name
        //

        NlpGetAccountNames( LogonInformation,
                            NlpUser,
                            &SamAccountName,
                            &NetbiosDomainName,
                            &DnsDomainName,
                            &Upn );


        //
        // Build the primary credential
        //
        //

#ifdef MAP_DOMAIN_NAMES_AT_LOGON
        {
            UNICODE_STRING MappedDomain;
            RtlInitUnicodeString(
                &MappedDomain,
                NULL
                );

            Status = NlpMapLogonDomain(
                        &MappedDomain,
                        &NetbiosDomainName );

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }
            Status = NlpMakePrimaryCredential( &MappedDomain,
                                               &SamAccountName,
                                               &PrimaryCredentials->Password,
                                               &Credential,
                                               &CredentialSize );

            if (MappedDomain.Buffer != NULL) {
                NtLmFree(MappedDomain.Buffer);
            }

            if ( !NT_SUCCESS( Status ) ) {
                goto Cleanup;
            }

        }
#else

        Status = NlpMakePrimaryCredential( &NetbiosDomainName,
                                           &SamAccountName,
                                           &PrimaryCredentials->Password,
                                           &Credential,
                                           &CredentialSize );

        if ( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }
#endif

        //
        // Fill the username and domain name into the primary credential
        //  that's passed to the other security packages.
        //
        // The names filled in are the effective names after authentication.
        //  For instance, it isn't the UPN passed to this function.
        //

        PrimaryCredentials->DownlevelName.Length = PrimaryCredentials->DownlevelName.MaximumLength =
            SamAccountName.Length;
        PrimaryCredentials->DownlevelName.Buffer = (*Lsa.AllocateLsaHeap)(SamAccountName.Length);

        if (PrimaryCredentials->DownlevelName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlCopyMemory(
            PrimaryCredentials->DownlevelName.Buffer,
            SamAccountName.Buffer,
            SamAccountName.Length
            );

        PrimaryCredentials->DomainName.Length = PrimaryCredentials->DomainName.MaximumLength =
            NetbiosDomainName.Length;

        PrimaryCredentials->DomainName.Buffer = (*Lsa.AllocateLsaHeap)(NetbiosDomainName.Length);

        if (PrimaryCredentials->DomainName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlCopyMemory(
            PrimaryCredentials->DomainName.Buffer,
            NetbiosDomainName.Buffer,
            NetbiosDomainName.Length
            );

        //
        // Fill the UPN and Dns DomainName into the primary credential
        //  that's passed to the other security packages.
        //

        if ( Upn.Length != 0 ) {
            PrimaryCredentials->Upn.Length = PrimaryCredentials->Upn.MaximumLength =
                Upn.Length;
            PrimaryCredentials->Upn.Buffer = (*Lsa.AllocateLsaHeap)(Upn.Length);

            if (PrimaryCredentials->Upn.Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory(
                PrimaryCredentials->Upn.Buffer,
                Upn.Buffer,
                Upn.Length
                );
        }

        if ( DnsDomainName.Length != 0 ) {
            PrimaryCredentials->DnsDomainName.Length = PrimaryCredentials->DnsDomainName.MaximumLength =
                DnsDomainName.Length;
            PrimaryCredentials->DnsDomainName.Buffer = (*Lsa.AllocateLsaHeap)(DnsDomainName.Length);

            if (PrimaryCredentials->DnsDomainName.Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory(
                PrimaryCredentials->DnsDomainName.Buffer,
                DnsDomainName.Buffer,
                DnsDomainName.Length
                );
        }

        RtlCopyMemory(&LogonServer, &NlpUser->LogonServer, sizeof(UNICODE_STRING));

        if ( LogonServer.Length != 0 ) {
            PrimaryCredentials->LogonServer.Length = PrimaryCredentials->LogonServer.MaximumLength =
                LogonServer.Length;
            PrimaryCredentials->LogonServer.Buffer = (*Lsa.AllocateLsaHeap)(LogonServer.Length);

            if (PrimaryCredentials->LogonServer.Buffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory(
                PrimaryCredentials->LogonServer.Buffer,
                LogonServer.Buffer,
                LogonServer.Length
                );
        }


        //
        // Save the credential in the LSA.
        //

        Status = NlpAddPrimaryCredential( LogonId,
                                          Credential,
                                          CredentialSize );

        if ( !NT_SUCCESS( Status ) ) {
            KdPrint(( "MSV1_0: LsaApLogonUser: error from AddCredential %lX\n",
                Status));
            goto Cleanup;
        }
        LogonCredentialAdded = TRUE;

        //
        // Build a Sid for this user.
        //

        UserSid = NlpMakeDomainRelativeSid( NlpUser->LogonDomainId,
                                            NlpUser->UserId );

        if ( UserSid == NULL ) {
            Status = STATUS_NO_MEMORY;
            KdPrint(("MSV1_0: LsaApLogonUser: No memory\n"));
            goto Cleanup;
        }
        PrimaryCredentials->UserSid = UserSid;
        UserSid = NULL;



        UserSidSize = RtlLengthSid( PrimaryCredentials->UserSid );


        //
        // Allocate an entry for the active logon table.
        //

        LogonEntrySize = ROUND_UP_COUNT(sizeof(ACTIVE_LOGON), ALIGN_DWORD) +
              ROUND_UP_COUNT(UserSidSize, sizeof(WCHAR)) +
              SamAccountName.Length + sizeof(WCHAR) +
              NetbiosDomainName.Length + sizeof(WCHAR) +
              NlpUser->LogonServer.Length + sizeof(WCHAR);

        LogonEntry = RtlAllocateHeap( MspHeap, 0, LogonEntrySize );

        if ( LogonEntry == NULL ) {
            Status = STATUS_NO_MEMORY;
            KdPrint(("MSV1_0: LsaApLogonUser: No memory %ld\n",
                    sizeof(ACTIVE_LOGON)));
            goto Cleanup;
        }

        //
        // Fill in the logon table entry.
        //

        Where = (PUCHAR)(LogonEntry + 1);

        OLD_TO_NEW_LARGE_INTEGER(
            LogonInformation->LogonId,
            LogonEntry->LogonId );

        LogonEntry->Flags = Flags;
        LogonEntry->LogonType = LogonType;

        //
        // Copy DWORD aligned fields first.
        //

        Where = ROUND_UP_POINTER( Where, ALIGN_DWORD );
        Status = RtlCopySid(UserSidSize, (PSID)Where, PrimaryCredentials->UserSid);

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        LogonEntry->UserSid = (PSID) Where;
        Where += UserSidSize;

        //
        // Copy WCHAR aligned fields
        //

        Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );
        NlpPutString( &LogonEntry->UserName,
                      &SamAccountName,
                      &Where );

        NlpPutString( &LogonEntry->LogonDomainName,
                      &NetbiosDomainName,
                      &Where );

        NlpPutString( &LogonEntry->LogonServer,
                      &NlpUser->LogonServer,
                      &Where );


        //
        // Get the next enumeration handle for this session.
        //

        NlpLockActiveLogons();

        NlpEnumerationHandle ++;
        LogonEntry->EnumHandle = NlpEnumerationHandle;

        //
        // Insert this entry into the active logon table.
        //

        if (NlpFindActiveLogon( LogonId, &ActiveLogon )){

            //
            // This Logon ID is already in use.
            //

            NlpUnlockActiveLogons();

            Status = STATUS_LOGON_SESSION_COLLISION;
            KdPrint((
                "MSV1_0: LsaApLogonUser: Collision from NlpFindActiveLogon\n"));
            goto Cleanup;
        }

        LogonEntry->Next = *ActiveLogon;
        *ActiveLogon = LogonEntry;
        LogonEntryLinked = TRUE;
        NlpUnlockActiveLogons();


        //
        // Ensure the LogonCount is at least as big as it is for this
        //  machine.
        //

        LogonCount = (USHORT) NlpCountActiveLogon( &NetbiosDomainName,
                                                   &SamAccountName );
        if ( NlpUser->LogonCount < LogonCount ) {
            NlpUser->LogonCount = LogonCount;
        }

        //
        // Alocate the profile buffer to return to the client
        //

        Status = NlpAllocateInteractiveProfile(
                    ClientRequest,
                    (PMSV1_0_INTERACTIVE_PROFILE *) ProfileBuffer,
                    ProfileBufferSize,
                    NlpUser );

        if ( !NT_SUCCESS( Status ) ) {
            KdPrint((
                "MSV1_0: LsaApLogonUser: Allocate Profile Failed: %lx\n", Status));
            goto Cleanup;
        }

    } else if ( LogonType == Network ) {

        //
        // if doing client challenge, and it's a vanilla NTLM response,
        //  and it's not a null session, compute unique per-session session keys
        //      N.B: not needed if it's NTLM++, not possible if LM
        //

        if ((NetworkAuthentication->ParameterControl & MSV1_0_USE_CLIENT_CHALLENGE) &&
            (NetworkAuthentication->CaseSensitiveChallengeResponse.Length == NT_RESPONSE_LENGTH ) && // vanilla NTLM response
            (NetworkAuthentication->CaseInsensitiveChallengeResponse.Length >= MSV1_0_CHALLENGE_LENGTH ) &&
            (NlpUser != NULL))       // NULL session iff NlpUser == NULL
        {
            MsvpCalculateNtlm2SessionKeys(
                &NlpUser->UserSessionKey,
                NetworkAuthentication->ChallengeToClient,
                NetworkAuthentication->CaseInsensitiveChallengeResponse.Buffer,
                (PUSER_SESSION_KEY)&NlpUser->UserSessionKey,
                (PLM_SESSION_KEY)&NlpUser->ExpansionRoom[SAMINFO_LM_SESSION_KEY]
                );

        }

        //
        // Alocate the profile buffer to return to the client
        //

        Status = NlpAllocateNetworkProfile(
                    ClientRequest,
                    (PMSV1_0_LM20_LOGON_PROFILE *) ProfileBuffer,
                    ProfileBufferSize,
                    NlpUser,
                    LogonNetwork.Identity.ParameterControl );
        if ( !NT_SUCCESS( Status ) ) {
            KdPrint((
                "MSV1_0: LsaApLogonUser: Allocate Profile Failed: %lx. This could also be a status for a subauth logon.\n", Status));
            goto Cleanup;
        }
    }


    //
    // Build the token information to return to the LSA
    //

    switch (LsaTokenInformationType) {
    case LsaTokenInformationV2:

        Status = NlpMakeTokenInformationV2(
                        NlpUser,
                        (PLSA_TOKEN_INFORMATION_V2 *)TokenInformation );

        if ( !NT_SUCCESS( Status ) ) {
            KdPrint((
                "MSV1_0: LsaApLogonUser: MakeTokenInformationV2 Failed: %lx\n", Status));
            goto Cleanup;
        }
        break;

    case LsaTokenInformationNull:
        {
            PLSA_TOKEN_INFORMATION_NULL VNull;

            VNull = (*Lsa.AllocateLsaHeap)(sizeof(LSA_TOKEN_INFORMATION_NULL) );
            if ( VNull == NULL ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            VNull->Groups = NULL;

            VNull->ExpirationTime.HighPart = 0x7FFFFFFF;
            VNull->ExpirationTime.LowPart = 0xFFFFFFFF;

            *TokenInformation = VNull;
        }

    }

    *TokenInformationType = LsaTokenInformationType;


    Status = STATUS_SUCCESS;

Cleanup:

    //
    // Restore the saved password
    //
    if ( ServiceSecretLogon ) {

        RtlCopyMemory( &Authentication->Password,
                       &SavedPassword,
                       sizeof( UNICODE_STRING ) );

        //
        // Free the secret value we read...
        //
        LsaIFree_LSAPR_CR_CIPHER_VALUE( SecretCurrent );
    }

    //
    // If the logon wasn't successful,
    //  cleanup resources we would have returned to the caller.
    //

    if ( !NT_SUCCESS(Status) ) {

        if ( LogonSessionCreated ) {
            (VOID)(*Lsa.DeleteLogonSession)( LogonId );
        }

        if ( LogonEntry != NULL ) {
            if ( LogonEntryLinked ) {
                LsaApLogonTerminated( LogonId );
            } else {
                if ( LogonCredentialAdded ) {
                    (VOID) NlpDeletePrimaryCredential(
                                LogonId );
                }
                RtlFreeHeap( MspHeap, 0, LogonEntry );
            }
        }

        // Special case for MsV1_0SubAuthLogon (includes arap).
        // (Don't free ProfileBuffer during error conditions which may not be fatal)

        if (!fSubAuthEx)
        {
            if ( *ProfileBuffer != NULL ) {
                if (ClientRequest != (PLSA_CLIENT_REQUEST) (-1))
                    (VOID)(*Lsa.FreeClientBuffer)( ClientRequest, *ProfileBuffer );
                else
                    (VOID)(*Lsa.FreeLsaHeap)( *ProfileBuffer );

                *ProfileBuffer = NULL;
            }
        }

        if (PrimaryCredentials->DownlevelName.Buffer != NULL) {
            (*Lsa.FreeLsaHeap)(PrimaryCredentials->DownlevelName.Buffer);
        }

        if (PrimaryCredentials->DomainName.Buffer != NULL) {
            (*Lsa.FreeLsaHeap)(PrimaryCredentials->DomainName.Buffer);
        }

        if (PrimaryCredentials->DnsDomainName.Buffer != NULL) {
            (*Lsa.FreeLsaHeap)(PrimaryCredentials->DnsDomainName.Buffer);
        }

        if (PrimaryCredentials->Upn.Buffer != NULL) {
            (*Lsa.FreeLsaHeap)(PrimaryCredentials->Upn.Buffer);
        }

        if (PrimaryCredentials->Password.Buffer != NULL) {

            RtlZeroMemory(
                PrimaryCredentials->Password.Buffer,
                PrimaryCredentials->Password.Length
                );

            (*Lsa.FreeLsaHeap)(PrimaryCredentials->Password.Buffer);
        }

        if (PrimaryCredentials->LogonServer.Buffer != NULL) {
            (*Lsa.FreeLsaHeap)(PrimaryCredentials->LogonServer.Buffer);
        }

        RtlZeroMemory(
            PrimaryCredentials,
            sizeof(SECPKG_PRIMARY_CRED)
            );
    }

    //
    // Copy out Authenticating authority and user name.
    //

    if ( NT_SUCCESS(Status) && LsaTokenInformationType != LsaTokenInformationNull ) {

        //
        // Use the information from the NlpUser structure, since it gives
        // us accurate information about what account we're logging on to,
        // rather than who we were.
        //

        if ( LogonType != Network ) {
            TmpName = NlpUser->EffectiveName;
        }

        if ( LogonType == Network &&
             NlpUser->UserFlags & LOGON_GUEST &&
             NlpUser->EffectiveName.Length ) {

            TmpName = NlpUser->EffectiveName;
        }

        TmpAuthority  = NlpUser->LogonDomainName;
    }

    *AccountName = (*Lsa.AllocateLsaHeap)( sizeof( UNICODE_STRING ) );

    if ( *AccountName != NULL ) {

        (*AccountName)->Buffer = (*Lsa.AllocateLsaHeap)(TmpName.Length + sizeof( UNICODE_NULL) );

        if ( (*AccountName)->Buffer != NULL ) {

            (*AccountName)->MaximumLength = TmpName.Length + sizeof( UNICODE_NULL );
            RtlCopyUnicodeString( *AccountName, &TmpName );

        } else {

            RtlInitUnicodeString( *AccountName, NULL );
        }
    }

    *AuthenticatingAuthority = (*Lsa.AllocateLsaHeap)( sizeof( UNICODE_STRING ) );

    if ( *AuthenticatingAuthority != NULL ) {

        (*AuthenticatingAuthority)->Buffer = (*Lsa.AllocateLsaHeap)( TmpAuthority.Length + sizeof( UNICODE_NULL ) );

        if ( (*AuthenticatingAuthority)->Buffer != NULL ) {

            (*AuthenticatingAuthority)->MaximumLength = (USHORT)(TmpAuthority.Length + sizeof( UNICODE_NULL ));
            RtlCopyUnicodeString( *AuthenticatingAuthority, &TmpAuthority );

        } else {

            RtlInitUnicodeString( *AuthenticatingAuthority, NULL );
        }
    }

    *MachineName = NULL;

    if (WorkStationName != NULL) {

        *MachineName = (*Lsa.AllocateLsaHeap)( sizeof( UNICODE_STRING ) );

        if ( *MachineName != NULL ) {

            (*MachineName)->Buffer = (*Lsa.AllocateLsaHeap)( WorkStationName->Length + sizeof( UNICODE_NULL ) );

            if ( (*MachineName)->Buffer != NULL ) {

                (*MachineName)->MaximumLength = (USHORT)(WorkStationName->Length + sizeof( UNICODE_NULL ));
                RtlCopyUnicodeString( *MachineName, WorkStationName );

            } else {

                RtlInitUnicodeString( *MachineName, NULL );
            }
        }
    }

    //
    // Map status codes to prevent specific information from being
    // released about this user.
    //
    switch (Status) {
    case STATUS_WRONG_PASSWORD:
    case STATUS_NO_SUCH_USER:

        //
        // sleep 3 seconds to "discourage" dictionary attacks.
        // Don't worry about interactive logon dictionary attacks.
        // They will be slow anyway.
        //
        // per bug 171041, SField, RichardW, CliffV all decided this
        // delay has almost zero value for Win2000.  Offline attacks at
        // sniffed wire traffic are more efficient and viable.  Further,
        // opimizations in logon code path make failed interactive logons
        // very fast.
        //
//        if (LogonType != Interactive) {
//            Sleep( 3000 );
//        }

        //
        // This is for auditing.  Make sure to clear it out before
        // passing it out of LSA to the caller.
        //

        *SubStatus = Status;
        Status = STATUS_LOGON_FAILURE;
        break;

    case STATUS_INVALID_LOGON_HOURS:
    case STATUS_INVALID_WORKSTATION:
    case STATUS_PASSWORD_EXPIRED:
    case STATUS_ACCOUNT_DISABLED:
        *SubStatus = Status;
        Status = STATUS_ACCOUNT_RESTRICTION;
        break;

    //
    // This shouldn't happen, but guard against it anyway.
    //
    case STATUS_ACCOUNT_RESTRICTION:
        *SubStatus = STATUS_ACCOUNT_RESTRICTION;
        break;

    default:
        break;

    }


    //
    // Cleanup locally used resources
    //

    if ( Credential != NULL ) {
        (*Lsa.FreeLsaHeap)( Credential );
    }

    if ( NlpUser != NULL ) {
        MIDL_user_free( NlpUser );
    }

    if ( UserSid != NULL ) {
        (*Lsa.FreeLsaHeap)( UserSid );
    }


    //
    // Return status to the caller
    //

    return Status;

}


VOID
LsaApLogonTerminated (
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine is used to notify each authentication package when a logon
    session terminates.  A logon session terminates when the last token
    referencing the logon session is deleted.

Arguments:

    LogonId - Is the logon ID that just logged off.

Return Status:

    None.



--*/

{
    NTSTATUS Status;

    PACTIVE_LOGON LogonEntry;
    PACTIVE_LOGON *ActiveLogon;
    NETLOGON_INTERACTIVE_INFO LogonInteractive;
    PNETLOGON_INTERACTIVE_INFO LogonInteractivePointer;

    //
    // Find the entry and de-link it from the active logon table.
    //

    NlpLockActiveLogons();

    if ( !NlpFindActiveLogon( LogonId, &ActiveLogon ) ) {
        NlpUnlockActiveLogons();
        return;
    }

    LogonEntry = *ActiveLogon;
    *ActiveLogon = LogonEntry->Next;
    NlpUnlockActiveLogons();


    //
    // Delete the credential.
    //
    // (Currently the LSA deletes all of the credentials before calling
    // the authentication package.  This line is added to be compatible
    // with a more reasonable LSA.)
    //

    (VOID) NlpDeletePrimaryCredential( &LogonEntry->LogonId );

    //
    // Deallocate the now orphaned entry.
    //

    RtlFreeHeap( MspHeap, 0, LogonEntry );


    //
    // NB: We don't delete the logon session or credentials.
    //  That will be done by the LSA itself after we return.
    //

    return;

}

//+-------------------------------------------------------------------------
//
//  Function:   SspAcceptCredentials
//
//  Synopsis:   This routine is called after another package has logged
//              a user on.  The other package provides a user name and
//              password and the Kerberos package will create a logon
//              session for this user.
//
//  Effects:    Creates a logon session
//
//  Arguments:  LogonType - Type of logon, such as network or interactive
//              PrimaryCredentials - Primary credentials for the account,
//                  containing a domain name, password, SID, etc.
//              SupplementalCredentials - If present, contains credentials
//                  from the account itself.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
SspAcceptCredentials(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PSECPKG_PRIMARY_CRED PrimaryCredentials,
    IN PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials
    )
{
    PMSV1_0_PRIMARY_CREDENTIAL Credential = NULL;
    ULONG CredentialSize;
    NTSTATUS Status = STATUS_SUCCESS;
    LUID SystemLuid = SYSTEM_LUID;
//    HKEY KeyHandle = NULL;
//    LONG RegStatus;
//    ULONG RegValueType;
//    WCHAR RegDomainName[DNS_MAX_NAME_LENGTH +1];
//    ULONG RegDomainSize = (DNS_MAX_NAME_LENGTH+1) * sizeof(WCHAR);
    UNICODE_STRING DomainNameToUse;
    PACTIVE_LOGON *ActiveLogon;
    PACTIVE_LOGON LogonEntry = NULL;
    ULONG LogonEntrySize;
    ULONG UserSidSize;
    PUCHAR Where;
    USHORT LogonCount;
    BOOLEAN LogonEntryLinked = FALSE;
    BOOLEAN LsaCredentialAdded = FALSE;
    PMSV1_0_SUPPLEMENTAL_CREDENTIAL MsvCredentials = NULL;

    LUID CredentialLuid;

    CredentialLuid = PrimaryCredentials->LogonId;

    //
    // If there is no cleartext password, bail out here because we
    // can't build a real credential.
    //

    if ((PrimaryCredentials->Flags & PRIMARY_CRED_CLEAR_PASSWORD) == 0) {
        if (!ARGUMENT_PRESENT(SupplementalCredentials)) {
            Status = STATUS_SUCCESS;
            goto Cleanup;
        } else {
            //
            // Validate the MSV credentials
            //

            MsvCredentials = (PMSV1_0_SUPPLEMENTAL_CREDENTIAL) SupplementalCredentials->Credentials;
            if (SupplementalCredentials->CredentialSize < sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL))
            {
                //
                // BUGBUG: bad credentials - ignore them
                //
                Status = STATUS_SUCCESS;
                goto Cleanup;
            }
            if (MsvCredentials->Version != MSV1_0_CRED_VERSION)
            {
                Status = STATUS_SUCCESS;
                goto Cleanup;
            }
        }
    }


    //
    // stash the credential associated with SYSTEM under another logonID
    // this is done so we can utilize that credential at a later time if
    // requested by the caller.
    //

    if (RtlEqualLuid(
            &CredentialLuid,
            &SystemLuid
            ))
    {
///        Status = STATUS_SUCCESS;
///        goto Cleanup;

        CredentialLuid = NtLmGlobalLuidMachineLogon;
    }

    //
    // If this is an update, just change the password
    //

    if ((PrimaryCredentials->Flags & PRIMARY_CRED_UPDATE) != 0) {
        if ((PrimaryCredentials->Flags & PRIMARY_CRED_CLEAR_PASSWORD) != 0) {

            NlpChangePasswordByLogonId(
                &CredentialLuid,
                &PrimaryCredentials->Password
                );
        }
        goto Cleanup;
    }


#if 0
    //
    // Check the registry for a domain name to use
    //

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT(MSV1_0_SUBAUTHENTICATION_KEY),
                    0,      //Reserved
                    KEY_QUERY_VALUE,
                    &KeyHandle
                    );

    if (RegStatus == ERROR_SUCCESS) {

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        TEXT("PreferredDomain"),
                        NULL,
                        &RegValueType,
                        (PUCHAR) RegDomainName,
                        &RegDomainSize
                        );

        if (RegStatus == ERROR_SUCCESS) {
            RtlInitUnicodeString(
                &DomainNameToUse,
                RegDomainName
                );
        }
        RegCloseKey(KeyHandle);
    }
#endif

    if( NtLmLocklessGlobalPreferredDomainString.Buffer != NULL ) {
        DomainNameToUse = NtLmLocklessGlobalPreferredDomainString;
    } else {
        DomainNameToUse = PrimaryCredentials->DomainName;
    }

    //
    // Build the primary credential
    //

    if ((PrimaryCredentials->Flags & PRIMARY_CRED_CLEAR_PASSWORD) != 0) {

        Status = NlpMakePrimaryCredential( &DomainNameToUse,
                                           &PrimaryCredentials->DownlevelName,
                                           &PrimaryCredentials->Password,
                                           &Credential,
                                           &CredentialSize );
    } else {
        Status = NlpMakePrimaryCredentialFromMsvCredential(
                    &DomainNameToUse,
                    &PrimaryCredentials->DownlevelName,
                    MsvCredentials,
                    &Credential,
                    &CredentialSize );
    }

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    //
    // Now create an entry in the active logon list
    //


    UserSidSize = RtlLengthSid( PrimaryCredentials->UserSid );


    //
    // Allocate an entry for the active logon table.
    //

    LogonEntrySize = ROUND_UP_COUNT(sizeof(ACTIVE_LOGON), ALIGN_DWORD) +
          ROUND_UP_COUNT(UserSidSize, sizeof(WCHAR)) +
          PrimaryCredentials->DownlevelName.Length + sizeof(WCHAR) +
          PrimaryCredentials->DomainName.Length + sizeof(WCHAR) +
          PrimaryCredentials->LogonServer.Length + sizeof(WCHAR);

    LogonEntry = RtlAllocateHeap( MspHeap, 0, LogonEntrySize );

    if ( LogonEntry == NULL ) {
        Status = STATUS_NO_MEMORY;
        KdPrint(("MSV1_0: SpAcceptCredentials: No memory %ld\n",
                sizeof(ACTIVE_LOGON)));
        goto Cleanup;
    }

    //
    // Fill in the logon table entry.
    //

    Where = (PUCHAR)(LogonEntry + 1);

    OLD_TO_NEW_LARGE_INTEGER(
        CredentialLuid,
        LogonEntry->LogonId );

    //
    // Indicate that this was a logon by another package because we don't want to
    // notify Netlogon of the logoff.
    //

    LogonEntry->Flags = LOGON_BY_OTHER_PACKAGE;
    LogonEntry->LogonType = LogonType;

    //
    // Copy DWORD aligned fields first.
    //

    Where = ROUND_UP_POINTER( Where, ALIGN_DWORD );
    Status = RtlCopySid(UserSidSize, (PSID)Where, PrimaryCredentials->UserSid);

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    LogonEntry->UserSid = (PSID) Where;
    Where += UserSidSize;

    //
    // Copy WCHAR aligned fields
    //

    Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );
    NlpPutString( &LogonEntry->UserName,
                  &PrimaryCredentials->DownlevelName,
                  &Where );

    NlpPutString( &LogonEntry->LogonDomainName,
                  &PrimaryCredentials->DomainName,
                  &Where );

    NlpPutString( &LogonEntry->LogonServer,
                  &PrimaryCredentials->LogonServer,
                  &Where );



    //
    // Insert this entry into the active logon table.
    //

    NlpLockActiveLogons();
    if (NlpFindActiveLogon( &CredentialLuid, &ActiveLogon )){

        //
        // This Logon ID is already in use.
        //


        //
        // Check to see if this was someone we logged on
        //
        // BUGBUG: we should do this check early so we don't waste time
        // building the structures.
        //

        if (((*ActiveLogon)->Flags & (LOGON_BY_CACHE | LOGON_BY_NETLOGON | LOGON_BY_LOCAL)) != 0) {

            //
            // We did the logon, so don't bother to add it again.
            //

            RtlFreeHeap( MspHeap, 0, LogonEntry );
            (*Lsa.FreeLsaHeap)( Credential );
            Credential = NULL;
            Status = STATUS_SUCCESS;

        } else {
            Status = STATUS_LOGON_SESSION_COLLISION;
            KdPrint((
                "MSV1_0: SpAcceptCredentials: Collision from NlpFindActiveLogon\n"));

        }

        NlpUnlockActiveLogons();

        goto Cleanup;
    }

    NlpEnumerationHandle ++;
    LogonEntry->EnumHandle = NlpEnumerationHandle;
    LogonEntry->Next = *ActiveLogon;
    *ActiveLogon = LogonEntry;
    LogonEntryLinked = TRUE;
    NlpUnlockActiveLogons();

    //
    // Save the credential in the LSA.
    //

    Status = NlpAddPrimaryCredential(
                &CredentialLuid,
                Credential,
                CredentialSize
                );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrint(( "MSV1_0: SpAcceptCredentials: error from AddCredential %lX\n",
            Status));
        goto Cleanup;
    }

    LsaCredentialAdded = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status)) {
        if (LogonEntry != NULL) {
            if (LogonEntryLinked) {
                LsaApLogonTerminated( &CredentialLuid );
            } else {

                if ( LsaCredentialAdded ) {
                    (VOID) NlpDeletePrimaryCredential(
                                &CredentialLuid );
                }
                RtlFreeHeap( MspHeap, 0, LogonEntry );
            }
        } else if (Credential != NULL) {

            if (LsaCredentialAdded) {
                (VOID) NlpDeletePrimaryCredential(
                            &CredentialLuid
                            );

            } else {
                (*Lsa.FreeLsaHeap)( Credential );
            }

        }
    }
    if ( Credential != NULL ) {
        (*Lsa.FreeLsaHeap)( Credential );
    }


    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   NlpMapLogonDomain
//
//  Synopsis:   This routine is called while MSV1_0 package is logging
//              a user on.  The logon domain name is mapped to another
//              domain to be stored in the credential.
//
//  Effects:    Allocates output string
//
//  Arguments:  MappedDomain - Receives mapped domain name
//              LogonDomain - Domain to which user is logging on
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
NlpMapLogonDomain(
    OUT PUNICODE_STRING MappedDomain,
    IN PUNICODE_STRING LogonDomain
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
#if 0
    HKEY KeyHandle = NULL;
    LONG RegStatus;
    ULONG RegValueType;
    WCHAR RegDomainName[DNS_MAX_NAME_LENGTH+1];
    ULONG RegDomainSize = (DNS_MAX_NAME_LENGTH+1) * sizeof(WCHAR);
    UNICODE_STRING DomainNameToUse;
#endif


#if 0

    //
    // Check the registry for a domain name to map
    //

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT(MSV1_0_SUBAUTHENTICATION_KEY),
                    0,      //Reserved
                    KEY_QUERY_VALUE,
                    &KeyHandle
                    );

    if (RegStatus == ERROR_SUCCESS) {

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        TEXT("MappedDomain"),
                        NULL,
                        &RegValueType,
                        (PUCHAR) RegDomainName,
                        &RegDomainSize
                        );

        if (RegStatus == ERROR_SUCCESS) {
            RtlInitUnicodeString(
                &DomainNameToUse,
                RegDomainName
                );
        }
        RegCloseKey(KeyHandle);
    }

    if ((RegStatus != ERROR_SUCCESS) ||
         !RtlEqualDomainName(
            LogonDomain,
            &DomainNameToUse
            ))
    {
        Status = NtLmDuplicateUnicodeString(
                    MappedDomain,
                    LogonDomain
                    );
        goto Cleanup;
    }

#endif

    if( (NtLmLocklessGlobalMappedDomainString.Buffer == NULL) ||
        !RtlEqualDomainName( LogonDomain, &NtLmLocklessGlobalMappedDomainString )
        )
    {
        Status = NtLmDuplicateUnicodeString(
                    MappedDomain,
                    LogonDomain
                    );
        goto Cleanup;
    }


#if 0
    //
    // Check the registry for a domain name to use
    //

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT(MSV1_0_SUBAUTHENTICATION_KEY),
                    0,      //Reserved
                    KEY_QUERY_VALUE,
                    &KeyHandle
                    );

    if (RegStatus == ERROR_SUCCESS) {

        RegDomainSize = (DNS_MAX_NAME_LENGTH+1) * sizeof(WCHAR);
        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        TEXT("PreferredDomain"),
                        NULL,
                        &RegValueType,
                        (PUCHAR) RegDomainName,
                        &RegDomainSize
                        );

        if (RegStatus == ERROR_SUCCESS) {
            RtlInitUnicodeString(
                &DomainNameToUse,
                RegDomainName
                );
        }
        RegCloseKey(KeyHandle);
    }

#endif

    if ( NtLmLocklessGlobalPreferredDomainString.Buffer == NULL )
    {
        Status = NtLmDuplicateUnicodeString(
                    MappedDomain,
                    LogonDomain
                    );
    } else {
        Status = NtLmDuplicateUnicodeString(
                    MappedDomain,
                    &NtLmLocklessGlobalPreferredDomainString
                    );
    }


Cleanup:
    return(Status);
}



// calculate NTLM2 challenge from client and server challenges
VOID
MsvpCalculateNtlm2Challenge (
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    OUT UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH]
    )
{
    MD5_CTX Md5Context;

    MD5Init(
        &Md5Context
        );
    MD5Update(
        &Md5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );
    MD5Update(
        &Md5Context,
        ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );
    MD5Final(
        &Md5Context
        );
    ASSERT(MD5DIGESTLEN >= MSV1_0_CHALLENGE_LENGTH);

    RtlCopyMemory(
        Challenge,
        Md5Context.digest,
        MSV1_0_CHALLENGE_LENGTH
        );
}


// calculate NTLM2 session keys from User session key given
//  to us by the system with the user's account

VOID
MsvpCalculateNtlm2SessionKeys (
    IN PUSER_SESSION_KEY NtUserSessionKey,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    OUT PUSER_SESSION_KEY LocalUserSessionKey,
    OUT PLM_SESSION_KEY LocalLmSessionKey
    )
{
    // SESSKEY = HMAC(NtUserSessionKey, (ChallengeToClient, ChallengeFromClient))
    //  Lm session key is first 8 bytes of session key
    HMACMD5_CTX HMACMD5Context;

    HMACMD5Init(
        &HMACMD5Context,
        (PUCHAR)NtUserSessionKey,
        sizeof(*NtUserSessionKey)
        );
    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );
    HMACMD5Update(
        &HMACMD5Context,
        ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );
    HMACMD5Final(
        &HMACMD5Context,
        (PUCHAR)LocalUserSessionKey
        );
    RtlCopyMemory(
        LocalLmSessionKey,
        LocalUserSessionKey,
        sizeof(*LocalLmSessionKey)
        );
}


// calculate NTLM3 OWF from credentials
VOID
MsvpCalculateNtlm3Owf (
    IN PNT_OWF_PASSWORD pNtOwfPassword,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pLogonDomainName,
    OUT UCHAR Ntlm3Owf[MSV1_0_NTLM3_OWF_LENGTH]
    )
{
    HMACMD5_CTX HMACMD5Context;
    WCHAR UCUserName[UNLEN+1];
    UNICODE_STRING UCUserNameString = {0, UNLEN, UCUserName};

    RtlUpcaseUnicodeString(
        &UCUserNameString,
        pUserName,
        FALSE
        );


    // Calculate NTLM3 OWF -- HMAC(MD4(P), (UserName, LogonDomainName))

    HMACMD5Init(
        &HMACMD5Context,
        (PUCHAR)pNtOwfPassword,
        sizeof(*pNtOwfPassword)
        );

    HMACMD5Update(
        &HMACMD5Context,
        (PUCHAR)UCUserNameString.Buffer,
        pUserName->Length
        );

    HMACMD5Update(
        &HMACMD5Context,
        (PUCHAR)pLogonDomainName->Buffer,
        pLogonDomainName->Length
        );

    HMACMD5Final(
        &HMACMD5Context,
        Ntlm3Owf
        );
}


// calculate LM3 response from credentials
VOID
MsvpLm3Response (
    IN PNT_OWF_PASSWORD pNtOwfPassword,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pLogonDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN PMSV1_0_LM3_RESPONSE pLm3Response,
    OUT UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH]
    )
{
    HMACMD5_CTX HMACMD5Context;
    UCHAR Ntlm3Owf[MSV1_0_NTLM3_OWF_LENGTH];

    // get NTLM3 OWF

    MsvpCalculateNtlm3Owf (
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        Ntlm3Owf
        );

    // Calculate NTLM3 Response
    // HMAC(Ntlm3Owf, (NS, V, HV, T, NC, S))

    HMACMD5Init(
        &HMACMD5Context,
        Ntlm3Owf,
        MSV1_0_NTLM3_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        (PUCHAR)pLm3Response->ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    ASSERT(MD5DIGESTLEN == MSV1_0_NTLM3_RESPONSE_LENGTH);

    HMACMD5Final(
        &HMACMD5Context,
        Response
        );

    return;
}


VOID
MsvpNtlm3Response (
    IN PNT_OWF_PASSWORD pNtOwfPassword,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pLogonDomainName,
    IN ULONG ServerNameLength,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN PMSV1_0_NTLM3_RESPONSE pNtlm3Response,
    OUT UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH],
    OUT PUSER_SESSION_KEY UserSessionKey,
    OUT PLM_SESSION_KEY LmSessionKey
    )
{
    HMACMD5_CTX HMACMD5Context;
    UCHAR Ntlm3Owf[MSV1_0_NTLM3_OWF_LENGTH];

    // get NTLM3 OWF

    MsvpCalculateNtlm3Owf (
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        Ntlm3Owf
        );

    // Calculate NTLM3 Response
    // HMAC(Ntlm3Owf, (NS, V, HV, T, NC, S))

    HMACMD5Init(
        &HMACMD5Context,
        Ntlm3Owf,
        MSV1_0_NTLM3_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        ChallengeToClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        &pNtlm3Response->RespType,
        (MSV1_0_NTLM3_INPUT_LENGTH + ServerNameLength)
        );

    ASSERT(MD5DIGESTLEN == MSV1_0_NTLM3_RESPONSE_LENGTH);

    HMACMD5Final(
        &HMACMD5Context,
        Response
        );

    // now compute the session keys
    //  HMAC(Kr, R)
    HMACMD5Init(
        &HMACMD5Context,
        Ntlm3Owf,
        MSV1_0_NTLM3_OWF_LENGTH
        );

    HMACMD5Update(
        &HMACMD5Context,
        Response,
        MSV1_0_NTLM3_RESPONSE_LENGTH
        );

    ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);
    HMACMD5Final(
        &HMACMD5Context,
        (PUCHAR)UserSessionKey
        );

    ASSERT(MSV1_0_LANMAN_SESSION_KEY_LENGTH <= MSV1_0_USER_SESSION_KEY_LENGTH);
    RtlCopyMemory(
        LmSessionKey,
        UserSessionKey,
        MSV1_0_LANMAN_SESSION_KEY_LENGTH);

    return;
}


NTSTATUS
MsvpLm20GetNtlm3ChallengeResponse (
    IN PNT_OWF_PASSWORD pNtOwfPassword,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pLogonDomainName,
    IN PUNICODE_STRING pServerName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT PMSV1_0_NTLM3_RESPONSE pNtlm3Response,
    OUT PMSV1_0_LM3_RESPONSE pLm3Response,
    OUT PUSER_SESSION_KEY UserSessionKey,
    OUT PLM_SESSION_KEY LmSessionKey
    )
/*++

Routine Description:

    This routine calculates the NT and LM response for the NTLM3
    authentication protocol
    It generates the time stamp, version numbers, and
    client challenge, and the NTLM3 and LM3 responses.

--*/

{

    NTSTATUS Status;

    // fill in version numbers, timestamp, and client's challenge

    pNtlm3Response->RespType = 1;
    pNtlm3Response->HiRespType = 1;
    pNtlm3Response->Flags = 0;
    pNtlm3Response->MsgWord = 0;

    Status = NtQuerySystemTime ( (PLARGE_INTEGER)&pNtlm3Response->TimeStamp );
    ASSERT( NT_SUCCESS(Status) );

    SspGenerateRandomBits(
        pNtlm3Response->ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );

#ifdef USE_CONSTANT_CHALLENGE
    pNtlm3Response->TimeStamp = 0;
    RtlZeroMemory(
        pNtlm3Response->ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );
#endif

    RtlCopyMemory(
        pNtlm3Response->Buffer,
        pServerName->Buffer,
        pServerName->Length
        );

    // Calculate NTLM3 response, filling in response field
    MsvpNtlm3Response (
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        pServerName->Length,
        ChallengeToClient,
        pNtlm3Response,
        pNtlm3Response->Response,
        UserSessionKey,
        LmSessionKey
        );

    // Use same challenge to compute the LM3 response
    RtlCopyMemory(
        pLm3Response->ChallengeFromClient,
        pNtlm3Response->ChallengeFromClient,
        MSV1_0_CHALLENGE_LENGTH
        );

    // Calculate LM3 response
    MsvpLm3Response (
        pNtOwfPassword,
        pUserName,
        pLogonDomainName,
        ChallengeToClient,
        pLm3Response,
        pLm3Response->Response
        );

    return STATUS_SUCCESS;
}


// MsvAvInit -- function to initialize AV pair list

PMSV1_0_AV_PAIR
MsvpAvlInit(
    IN void * pAvList
    )
{
    PMSV1_0_AV_PAIR pAvPair;

    pAvPair = (PMSV1_0_AV_PAIR)pAvList;
    pAvPair->AvId = MsvAvEOL;
    pAvPair->AvLen = 0;
    return pAvPair;
}

// MsvpAvGet -- function to find a particular AV pair by ID

PMSV1_0_AV_PAIR
MsvpAvlGet(
    IN PMSV1_0_AV_PAIR pAvList,             // first pair of AV pair list
    IN MSV1_0_AVID AvId,                    // AV pair to find
    IN LONG cAvList                         // size of AV list
    )
{
    PMSV1_0_AV_PAIR pAvPair;

    pAvPair = pAvList;

    while (1) {
        if (pAvPair->AvId == AvId)
            return pAvPair;
        if (pAvPair->AvId == MsvAvEOL)
            return NULL;
       cAvList -= (pAvPair->AvLen + sizeof(MSV1_0_AV_PAIR));
       if (cAvList <= 0)
           return NULL;
       pAvPair = (PMSV1_0_AV_PAIR)((PUCHAR)pAvPair + pAvPair->AvLen + sizeof(MSV1_0_AV_PAIR));
    }
}

// MsvpAvlLen -- function to find length of a AV list

ULONG
MsvpAvlLen(
    IN PMSV1_0_AV_PAIR pAvList,            // first pair of AV pair list
    IN LONG cAvList                        // max size of AV list
    )
{
    PMSV1_0_AV_PAIR pCurPair;

    // find the EOL
    pCurPair = MsvpAvlGet(pAvList, MsvAvEOL, cAvList);
    if( pCurPair == NULL )
        return 0;

    // compute length (not forgetting the EOL pair)
    return (ULONG)(((PUCHAR)pCurPair - (PUCHAR)pAvList) + sizeof(MSV1_0_AV_PAIR));
}

// MsvpAvlAdd -- function to add an AV pair to a list
// assumes buffer is long enough!
// returns NULL on failure.

PMSV1_0_AV_PAIR
MsvpAvlAdd(
    IN PMSV1_0_AV_PAIR pAvList,             // first pair of AV pair list
    IN MSV1_0_AVID AvId,                    // AV pair to add
    IN PUNICODE_STRING pString,             // value of pair
    IN LONG cAvList                         // max size of AV list
    )
{
    PMSV1_0_AV_PAIR pCurPair;

    // find the EOL
    pCurPair = MsvpAvlGet(pAvList, MsvAvEOL, cAvList);
    if( pCurPair == NULL )
        return NULL;

    //
    // append the new AvPair (assume the buffer is long enough!)
    //

    pCurPair->AvId = (USHORT)AvId;
    pCurPair->AvLen = (USHORT)pString->Length;
    memcpy(pCurPair+1, pString->Buffer, pCurPair->AvLen);

    // top it off with a new EOL
    pCurPair = (PMSV1_0_AV_PAIR)((PUCHAR)pCurPair + sizeof(MSV1_0_AV_PAIR) + pCurPair->AvLen);
    pCurPair->AvId = MsvAvEOL;
    pCurPair->AvLen = 0;

    return pCurPair;
}


// MsvpAvlSize -- fucntion to calculate length needed for an AV list
ULONG
MsvpAvlSize(
    IN ULONG iPairs,            // number of AV pairs response will include
    IN ULONG iPairsLen          // total size of values for the pairs
    )
{
    return (
        iPairs * sizeof(MSV1_0_AV_PAIR) +   // space for the pairs' headers
        iPairsLen +                         // space for pairs' values
        sizeof(MSV1_0_AV_PAIR)              // space for the EOL
        );
}

