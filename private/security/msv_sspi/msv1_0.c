/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msv1_0.c

Abstract:

    MSV1_0 authentication package.


    The name of this authentication package is:


Author:

    Jim Kelly 11-Apr-1991

Revision History:
    Scott Field (sfield)    15-Jan-98   Add MspNtDeriveCredential
    Chandana Surlu          21-Jul-96   Stolen from \\kernel\razzle3\src\security\msv1_0\msv1_0.c
--*/

#include "msp.h"
#include "nlp.h"


//
// LsaApCallPackage() function dispatch table
//


PLSA_AP_CALL_PACKAGE
MspCallPackageDispatch[] = {
    MspLm20Challenge,
    MspLm20GetChallengeResponse,
    MspLm20EnumUsers,
    MspLm20GetUserInfo,
    MspLm20ReLogonUsers,
    MspLm20ChangePassword,
    MspLm20ChangePassword,
    MspLm20GenericPassthrough,
    MspLm20CacheLogon,
    MspNtSubAuth,
    MspNtDeriveCredential,
    MspLm20CacheLookup
};





///////////////////////////////////////////////////////////////////////
//                                                                   //
// Authentication package dispatch routines.                         //
//                                                                   //
///////////////////////////////////////////////////////////////////////

NTSTATUS
LsaApInitializePackage (
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PSTRING Database OPTIONAL,
    IN PSTRING Confidentiality OPTIONAL,
    OUT PSTRING *AuthenticationPackageName
    )

/*++

Routine Description:

    This service is called once by the LSA during system initialization to
    provide the DLL a chance to initialize itself.

Arguments:

    AuthenticationPackageId - The ID assigned to the authentication
        package.

    LsaDispatchTable - Provides the address of a table of LSA
        services available to authentication packages.  The services
        of this table are ordered according to the enumerated type
        LSA_DISPATCH_TABLE_API.

    Database - This parameter is not used by this authentication package.

    Confidentiality - This parameter is not used by this authentication
        package.

    AuthenticationPackageName - Recieves the name of the
        authentication package.  The authentication package is
        responsible for allocating the buffer that the string is in
        (using the AllocateLsaHeap() service) and returning its
        address here.  The buffer will be deallocated by LSA when it
        is no longer needed.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.


--*/

{

    PSTRING NameString;
    PCHAR NameBuffer;
    NTSTATUS Status;

    //
    // If we haven't already initialized the internals, do it now.
    //

    if (!NlpMsvInitialized) {


        //
        // Use the process heap for memory allocations.
        //

        MspHeap = RtlProcessHeap();

        //
        // Save our assigned authentication package ID.
        // BUGBUG Use NtLmPackageId
        //

        MspAuthenticationPackageId = AuthenticationPackageId;

        //
        // BUGBUG Copy the LSA service dispatch table
        // This is the same as the first 11 fucntions of LsaFunctions
        // as defined in global.h. It was easier to duplicate
        // these globals rather than change the code all over
        //

        Lsa.CreateLogonSession     = LsaDispatchTable->CreateLogonSession;
        Lsa.DeleteLogonSession     = LsaDispatchTable->DeleteLogonSession;
        Lsa.AddCredential          = LsaDispatchTable->AddCredential;
        Lsa.GetCredentials         = LsaDispatchTable->GetCredentials;
        Lsa.DeleteCredential       = LsaDispatchTable->DeleteCredential;
        Lsa.AllocateLsaHeap        = LsaDispatchTable->AllocateLsaHeap;
        Lsa.FreeLsaHeap            = LsaDispatchTable->FreeLsaHeap;
        Lsa.AllocateClientBuffer   = LsaDispatchTable->AllocateClientBuffer;
        Lsa.FreeClientBuffer       = LsaDispatchTable->FreeClientBuffer;
        Lsa.CopyToClientBuffer     = LsaDispatchTable->CopyToClientBuffer;
        Lsa.CopyFromClientBuffer   = LsaDispatchTable->CopyFromClientBuffer;

	//
	// Initialize the change password log.
	//

	MsvPaswdInitializeLog();

        //
        // Initialize netlogon
        //

        Status = NlInitialize();

        if ( !NT_SUCCESS( Status ) ) {
            KdPrint(("NTLM: Error from NlInitialize = %d\n", Status));
            return Status;
        }
        NlpMsvInitialized = TRUE;
    }

    //
    // Allocate and return our package name
    //

    if (ARGUMENT_PRESENT(AuthenticationPackageName))
    {
        NameBuffer = (*(Lsa.AllocateLsaHeap))(sizeof(MSV1_0_PACKAGE_NAME));
        if (!NameBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            KdPrint(("NTLM: Error from Lsa.AllocateLsaHeap\n"));
            return Status;

        }
        strcpy( NameBuffer, MSV1_0_PACKAGE_NAME);

        NameString = (*(Lsa.AllocateLsaHeap))( (ULONG)sizeof(STRING) );
        if (!NameString)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            KdPrint(("NTLM: Error from Lsa.AllocateLsaHeap\n"));
            return Status;
        }

        RtlInitString( NameString, NameBuffer );
        (*AuthenticationPackageName) = NameString;
    }


    RtlInitUnicodeString(
        &NlpMsv1_0PackageName,
        TEXT(MSV1_0_PACKAGE_NAME)
        );

    return STATUS_SUCCESS;

    //
    // Appease the compiler gods by referencing all arguments
    //

    UNREFERENCED_PARAMETER(Confidentiality);
    UNREFERENCED_PARAMETER(Database);

}


NTSTATUS
LsaApCallPackage (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage().

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/

{
    ULONG MessageType;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PMSV1_0_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(MspCallPackageDispatch)/sizeof(MspCallPackageDispatch[0])) ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allow the dispatch routines to only set the return buffer information
    // on success conditions.
    //

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;



    //
    // Call the appropriate routine for this message.
    //

    return (*(MspCallPackageDispatch[MessageType]))(
        ClientRequest,
        ProtocolSubmitBuffer,
        ClientBufferBase,
        SubmitBufferLength,
        ProtocolReturnBuffer,
        ReturnBufferLength,
        ProtocolStatus ) ;

}


NTSTATUS
LsaApCallPackageUntrusted (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for untrusted clients.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/

{
    ULONG MessageType;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PMSV1_0_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(MspCallPackageDispatch)/sizeof(MspCallPackageDispatch[0])) ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Untrusted clients are only allowed to call the ChangePassword function.
    //

    if ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0ChangePassword) {

        return STATUS_ACCESS_DENIED;
    }

    //
    // Allow the dispatch routines to only set the return buffer information
    // on success conditions.
    //

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    //
    // Call the appropriate routine for this message.
    //

    return (*(MspCallPackageDispatch[MessageType]))(
        ClientRequest,
        ProtocolSubmitBuffer,
        ClientBufferBase,
        SubmitBufferLength,
        ProtocolReturnBuffer,
        ReturnBufferLength,
        ProtocolStatus ) ;

}



NTSTATUS
LsaApCallPackagePassthrough (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for passthrough logon requests.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/

{
    ULONG MessageType;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PMSV1_0_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(MspCallPackageDispatch)/sizeof(MspCallPackageDispatch[0])) ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // clients are only allowed to call the SubAuthLogon function.
    //

    if ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0SubAuth) {

        return STATUS_ACCESS_DENIED;
    }

    //
    // Allow the dispatch routines to only set the return buffer information
    // on success conditions.
    //

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    //
    // Call the appropriate routine for this message.
    //

    return (*(MspCallPackageDispatch[MessageType]))(
        ClientRequest,
        ProtocolSubmitBuffer,
        ClientBufferBase,
        SubmitBufferLength,
        ProtocolReturnBuffer,
        ReturnBufferLength,
        ProtocolStatus ) ;

}



VOID
LsaApMsInitialize (
    IN PLSAP_PRIVATE_LSA_SERVICES PrivateLsaApi
    )

/*++

Routine Description:

    This initialization routine is called by the LSA before normal
    package initialization to pass a table of private LSA routine addresses.
    This is intended for use by the standard Microsoft authentication packages.
    only.



Arguments:

    PrivateLsaApi - Provides the address of a table of private LSA
        services available to Microsoft authentication packages.  The services
        of this table are ordered according to the enumerated type
        LSA_PRIVATE_LSA_SERVICES.


Return Status:

    None.



--*/

{

    //
    // Copy the private LSA service dispatch table
    //

    Lsap.GetOperationalMode     = PrivateLsaApi->GetOperationalMode;
    Lsap.ImpersonateClient      = PrivateLsaApi->ImpersonateClient;


    return;

}
