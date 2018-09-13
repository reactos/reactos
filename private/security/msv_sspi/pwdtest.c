/*--

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    pwdtest.c

Abstract:

    Test program for the changing passwords.

Author:

    30-Apr-1993 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
 Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\pwdtest.c


--*/


//
// Common include files.
//

#include <msp.h>
#define NLP_ALLOCATE
#include <nlp.h>
#include <lsarpc.h>     // Lsar routines
#include <lsaisrv.h>    // LsaIFree and Trusted Client Routines
#include <stdio.h>


//
// Dummy routines from LSA
//

NTSTATUS
LsapAllocateClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG LengthRequired,
    OUT PVOID *ClientBaseAddress
    )

{

    UNREFERENCED_PARAMETER (ClientRequest);
    *ClientBaseAddress = RtlAllocateHeap( MspHeap, 0, LengthRequired );

    if ( *ClientBaseAddress == NULL ) {
        return(STATUS_QUOTA_EXCEEDED);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
LsapFreeClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ClientBaseAddress OPTIONAL
    )
{
    UNREFERENCED_PARAMETER (ClientRequest);
    UNREFERENCED_PARAMETER (ClientBaseAddress);

    return(STATUS_SUCCESS);
}


NTSTATUS
LsapCopyToClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID ClientBaseAddress,
    IN PVOID BufferToCopy
    )

{
    UNREFERENCED_PARAMETER (ClientRequest);
    RtlMoveMemory( ClientBaseAddress, BufferToCopy, Length );
    return(STATUS_SUCCESS);
}


int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Drive the password changing.

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    NTSTATUS Status;
    MSV1_0_CHANGEPASSWORD_REQUEST Request;
    PMSV1_0_CHANGEPASSWORD_RESPONSE ReturnBuffer;
    ULONG ReturnBufferSize;
    NTSTATUS ProtocolStatus;
    OBJECT_ATTRIBUTES LSAObjectAttributes;
    UNICODE_STRING LocalComputerName = { 0, 0, NULL };
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;

    if ( argc < 5 ) {
        fprintf( stderr,
            "Usage: pwdtest DomainName UserName OldPassword NewPassword\n" );
        return(1);
    }

    //
    // Set up MSV1_0.dll environment.
    //

    MspHeap = RtlProcessHeap();

    Status = NlInitialize();

    if ( !NT_SUCCESS( Status ) ) {
        printf("pwdtest: NlInitialize failed, status %x\n", Status);
        return(1);
    }

    Lsa.AllocateClientBuffer = LsapAllocateClientBuffer;
    Lsa.FreeClientBuffer = LsapFreeClientBuffer;
    Lsa.CopyToClientBuffer = LsapCopyToClientBuffer;



    //
    // Open the LSA policy database in case change password needs it
    //

    InitializeObjectAttributes( &LSAObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    Status = LsaOpenPolicy( &LocalComputerName,
                            &LSAObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &NlpPolicyHandle );

    if( !NT_SUCCESS(Status) ) {
        printf("pwdtest: LsaOpenPolicy failed, status %x\n", Status);
        return(1);
    }


    //
    // Get the name of our domain.
    //

    Status = LsaQueryInformationPolicy(
                    NlpPolicyHandle,
                    PolicyPrimaryDomainInformation,
                    (PVOID *) &PrimaryDomainInfo );

    if( !NT_SUCCESS(Status) ) {
        KdPrint(("pwdtest: LsaQueryInformationPolicy failed, status %x\n",
                 Status));
        return(1);
    }

    NlpSamDomainName = PrimaryDomainInfo->Name;



    //
    // Build the request message
    //

    Request.MessageType = MsV1_0ChangePassword;
    RtlCreateUnicodeStringFromAsciiz( &Request.DomainName, argv[1] );
    RtlCreateUnicodeStringFromAsciiz( &Request.AccountName, argv[2] );
    RtlCreateUnicodeStringFromAsciiz( &Request.OldPassword, argv[3] );
    RtlCreateUnicodeStringFromAsciiz( &Request.NewPassword, argv[4] );

    Status = MspLm20ChangePassword( NULL,
                                    &Request,
                                    &Request,
                                    0x7FFFFFFF,
                                    (PVOID *) &ReturnBuffer,
                                    &ReturnBufferSize,
                                    &ProtocolStatus );

    printf( "Status = 0x%lx  0x%lx\n", Status, ProtocolStatus );

    if ( ProtocolStatus == STATUS_CANT_DISABLE_MANDATORY ) {
        printf( "Are you running as SYSTEM?\n" );
    }

    if ( ReturnBufferSize != 0 ) {
        printf( "PasswordInfoValid %ld\n", ReturnBuffer->PasswordInfoValid );
        if ( ReturnBuffer->PasswordInfoValid ) {
            printf( "Min length: %ld  PasswordHistory: %ld  Prop 0x%lx\n",
                ReturnBuffer->DomainPasswordInfo.MinPasswordLength,
                ReturnBuffer->DomainPasswordInfo.PasswordHistoryLength,
                ReturnBuffer->DomainPasswordInfo.PasswordProperties );
        }
    }
    return 0;


}


//
// Stub routines needed by msvpaswd.c
//

NTSTATUS
LsarQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InfoClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    )
{
    return( LsaQueryInformationPolicy( PolicyHandle,
                                       InfoClass,
                                       Buffer ) );
}

VOID
LsaIFree_LSAPR_POLICY_INFORMATION (
    POLICY_INFORMATION_CLASS InfoClass,
    PLSAPR_POLICY_INFORMATION Buffer
    )
{
    UNREFERENCED_PARAMETER (InfoClass);
    UNREFERENCED_PARAMETER (Buffer);
}

NTSTATUS
NlpChangePassword(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING UserName,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PNT_OWF_PASSWORD NtOwfPassword
    )
{
    UNREFERENCED_PARAMETER (DomainName);
    UNREFERENCED_PARAMETER (UserName);
    UNREFERENCED_PARAMETER (LmOwfPassword);
    UNREFERENCED_PARAMETER (NtOwfPassword);
    return(STATUS_SUCCESS);
}



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

    //
    // Initialize global data
    //

    NlpEnumerationHandle = 0;
    NlpSessionCount = 0;

    NlpComputerName.Buffer = NULL;
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


#ifdef notdef

    //
    // Initialize any locks.
    //

    RtlInitializeCriticalSection(&NlpActiveLogonLock);
    RtlInitializeCriticalSection(&NlpSessionCountLock);

    //
    // initialize the cache - creates a critical section is all
    //

    NlpCacheInitialize();
#endif // notdef


    //
    // Attempt to load Netapi.dll
    //

    NlpLoadNetapiDll();

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
