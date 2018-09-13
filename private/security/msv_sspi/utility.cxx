/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    utility.cxx

Abstract:

    Private NtLmSsp service utility routines.

Author:

    Cliff Van Dyke (cliffv) 9-Jun-1993

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
    ChandanS  06-Aug-1996 Stolen from net\svcdlls\ntlmssp\common\utility.c

--*/

//
// Common include files.
//

#include <global.h>

//
// Include files specific to this .c file
//

#include <netlib.h>     // NetpMemoryFree()
#include <secobj.h>     // ACE_DATA ...
#include <stdio.h>      // vsprintf().
#include <tstr.h>       // TCHAR_ equates.

#define SSP_TOKEN_ACCESS (READ_CONTROL              |\
                          WRITE_DAC                 |\
                          TOKEN_DUPLICATE           |\
                          TOKEN_IMPERSONATE         |\
                          TOKEN_QUERY               |\
                          TOKEN_QUERY_SOURCE        |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT)




SECURITY_STATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    )
/*++

Routine Description:

    Convert an NtStatus code to the corresponding Security status code. For
    particular errors that are required to be returned as is (for setup code)
    don't map the errors.


Arguments:

    NtStatus - NT status to convert

Return Value:

    Returns security status code.

BUGBUG:
    We should fix setup to not require these errors. MMS 3/3/97


--*/
{

#if 0
    SECURITY_STATUS SecStatus;

    //
    // Check for security status and let them through
    //

    if  (HRESULT_FACILITY(NtStatus) == FACILITY_SECURITY )
    {
        return (NtStatus);
    }

    switch(NtStatus){

    case STATUS_SUCCESS:
        SecStatus = SEC_E_OK;
        break;

    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        break;

    case STATUS_NETLOGON_NOT_STARTED:
    case STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
    case STATUS_NO_LOGON_SERVERS:
    case STATUS_NO_SUCH_DOMAIN:
    case STATUS_BAD_NETWORK_PATH:
    case STATUS_TRUST_FAILURE:
    case STATUS_TRUSTED_RELATIONSHIP_FAILURE:
        // BUGBUG: what should this be?
        SecStatus = SEC_E_NO_AUTHENTICATING_AUTHORITY;
        break;

    case STATUS_NO_SUCH_LOGON_SESSION:
        SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
        break;

    case STATUS_INVALID_PARAMETER:
    case STATUS_PARTIAL_COPY:
        SecStatus = SEC_E_INVALID_TOKEN;
        break;

    case STATUS_PRIVILEGE_NOT_HELD:
        SecStatus = SEC_E_NOT_OWNER;
        break;

    case STATUS_INVALID_HANDLE:
        SecStatus = SEC_E_INVALID_HANDLE;
        break;

    case STATUS_BUFFER_TOO_SMALL:
        // BUGBUG: there should be a better code
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        break;

    case STATUS_NOT_SUPPORTED:
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        break;

    case STATUS_OBJECT_NAME_NOT_FOUND:
        SecStatus = SEC_E_TARGET_UNKNOWN;
        break;

    // See bug 75803 .
    case STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT:
    case STATUS_NOLOGON_SERVER_TRUST_ACCOUNT:
    case STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT:
    case STATUS_TRUSTED_DOMAIN_FAILURE:
        SecStatus = NtStatus;
        break;

    case STATUS_LOGON_FAILURE:
    case STATUS_NO_SUCH_USER:
    case STATUS_ACCOUNT_DISABLED:
    case STATUS_ACCOUNT_RESTRICTION:
    case STATUS_ACCOUNT_LOCKED_OUT:
    case STATUS_WRONG_PASSWORD:
    case STATUS_ACCOUNT_EXPIRED:
    case STATUS_PASSWORD_EXPIRED:
        SecStatus = SEC_E_LOGON_DENIED;
        break;

    case STATUS_NAME_TOO_LONG:
    case STATUS_ILL_FORMED_PASSWORD:

        // BUGBUG: what should this be?
        SecStatus = SEC_E_INVALID_TOKEN;
        break;

    case STATUS_INTERNAL_ERROR:
        SecStatus = SEC_E_INTERNAL_ERROR;
        break;

    default:

        //
        // BUGBUG: remove this now.
        //

        DbgPrint("MSV1_0: Unable to map error code 0x%x, returning SEC_E_INTERNAL_ERROR\n",NtStatus);
        if ( DefaultStatus != 0 ) {
            SecStatus = DefaultStatus;
        } else {
            SspPrint((SSP_API, "\n\n\n BUGBUG: Unable to map error code 0x%x\n\n\n\n",NtStatus));
            SecStatus = SEC_E_INTERNAL_ERROR;
        }
        break;
    }

#endif
    return(NtStatus);
}


BOOLEAN
SspTimeHasElapsed(
    IN LARGE_INTEGER StartTime,
    IN DWORD Timeout
    )
/*++

Routine Description:

    Determine if "Timeout" milliseconds have elapsed since StartTime.

Arguments:

    StartTime - Specifies an absolute time when the event started (100ns units).

    Timeout - Specifies a relative time in milliseconds.  0xFFFFFFFF indicates
        that the time will never expire.

Return Value:

    TRUE -- iff Timeout milliseconds have elapsed since StartTime.

--*/
{
    LARGE_INTEGER TimeNow;
    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER Period;

    //
    // If the period to too large to handle (i.e., 0xffffffff is forever),
    //  just indicate that the timer has not expired.
    //
    // (0x7fffffff is a little over 24 days).
    //

    if ( Timeout> 0x7fffffff ) {
        return FALSE;
    }

    //
    // Compute the elapsed time
    //

    NtQuerySystemTime( &TimeNow );
    ElapsedTime.QuadPart = TimeNow.QuadPart - StartTime.QuadPart;

    //
    // Convert Timeout from milliseconds into 100ns units.
    //

    Period.QuadPart = Int32x32To64( (LONG)Timeout, 10000 );


    //
    // If the elapsed time is negative (totally bogus),
    //  or greater than the maximum allowed,
    //  indicate the period has elapsed.
    //

    if ( ElapsedTime.QuadPart < 0 || ElapsedTime.QuadPart > Period.QuadPart ) {
        return TRUE;
    }

    return FALSE;
}


SECURITY_STATUS
SspDuplicateToken(
    IN HANDLE OriginalToken,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicatedToken
    )
/*++

Routine Description:

    Duplicates a token

Arguments:

    OriginalToken - Token to duplicate
    DuplicatedToken - Receives handle to duplicated token

Return Value:

    Any error from NtDuplicateToken

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    QualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    QualityOfService.EffectiveOnly = FALSE;
    QualityOfService.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    QualityOfService.ImpersonationLevel = ImpersonationLevel;
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    Status = NtDuplicateToken(
                OriginalToken,
                SSP_TOKEN_ACCESS,
                &ObjectAttributes,
                FALSE,
                TokenImpersonation,
                DuplicatedToken
                );

    return(SspNtStatusToSecStatus(Status, SEC_E_NO_IMPERSONATION));
}


LPWSTR
SspAllocWStrFromWStr(
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Allocate and copy unicode string (wide character strdup)

Arguments:

    Unicode - pointer to wide character string to make copy of

Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NtLmFree.

--*/

{
    DWORD   Size;
    LPWSTR  ptr;

    Size = WCSSIZE(Unicode);
    ptr = (LPWSTR)NtLmAllocate(Size);
    if ( ptr != NULL) {
        RtlCopyMemory(ptr, Unicode, Size);
    }
    return ptr;
}


VOID
SspHidePassword(
    IN OUT PUNICODE_STRING Password
    )
/*++

Routine Description:

    Run-encodes the password so that it is not very visually
    distinguishable.  This is so that if it makes it to a
    paging file, it wont be obvious.


    WARNING - This routine will use the upper portion of the
    password's length field to store the seed used in encoding
    password.  Be careful you don't pass such a string to
    a routine that looks at the length (like and RPC routine).

Arguments:

    Seed - The seed to use to hide the password.

    PasswordSource - Contains password to hide.

Return Value:


--*/
{
#if 0
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    UCHAR LocalSeed;

    SspPrint((SSP_API_MORE, "Entering SspHidePassword\n"));
    LocalSeed = 0;


    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&Password->Length;
    //ASSERT(*((LPWCH)SeedAndLength+Password->Length) == 0);
    ASSERT((SeedAndLength->Seed) == 0);

    RtlRunEncodeUnicodeString(
        &LocalSeed,
        Password
        );

    SeedAndLength->Seed = LocalSeed;
    SspPrint((SSP_API_MORE, "Leaving SspHidePassword\n"));
#endif
    SspPrint((SSP_API_MORE, "Entering SspHidePassword\n"));

    SspProtectMemory( Password->Buffer, (ULONG)Password->Length );

    SspPrint((SSP_API_MORE, "Leaving SspHidePassword\n"));
}


VOID
SspRevealPassword(
    IN OUT PUNICODE_STRING HiddenPassword
    )
/*++

Routine Description

    Reveals a previously hidden password so that it
    is plain text once again.

Arguments:

    HiddenPassword - Contains the password to reveal

Return Value

--*/
{
#if 0
    PSECURITY_SEED_AND_LENGTH SeedAndLength;

    UCHAR Seed;
    SspPrint((SSP_API_MORE, "Entering SspRevealPassword\n"));

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH)&HiddenPassword->Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    RtlRunDecodeUnicodeString(
           Seed,
           HiddenPassword
           );

    SspPrint((SSP_API_MORE, "Leaving SspRevealPassword\n"));
#endif
    SspPrint((SSP_API_MORE, "Entering SspRevealPassword\n"));

    SspUnprotectMemory( HiddenPassword->Buffer, (ULONG)HiddenPassword->Length );

    SspPrint((SSP_API_MORE, "Leaving SspRevealPassword\n"));

}


BOOLEAN
SspGetTokenBuffer(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PSecBuffer * Token,
    IN BOOLEAN ReadonlyOK
    )

/*++

Routine Description:

    This routine parses a Token Descriptor and pulls out the useful
    information.

Arguments:

    TokenDescriptor - Descriptor of the buffer containing (or to contain) the
        token. If not specified, TokenBuffer and TokenSize will be returned
        as NULL.

    TokenBuffer - Returns a pointer to the buffer for the token.

    TokenSize - Returns a pointer to the location of the size of the buffer.

    ReadonlyOK - TRUE if the token buffer may be readonly.

Return Value:

    TRUE - If token buffer was properly found.

--*/

{
    ULONG i, Index = 0;

    //
    // If there is no TokenDescriptor passed in,
    //  just pass out NULL to our caller.
    //

    ASSERT(*Token != NULL);
    if ( !ARGUMENT_PRESENT( TokenDescriptor) ) {
        return TRUE;
    }

    if (TokenDescriptor->ulVersion != SECBUFFER_VERSION)
    {
        return FALSE;
    }

    //
    // Loop through each described buffer.
    //

    for ( i=0; i<TokenDescriptor->cBuffers ; i++ ) {
        PSecBuffer Buffer = &TokenDescriptor->pBuffers[i];
        if ( (Buffer->BufferType & (~SECBUFFER_READONLY)) == SECBUFFER_TOKEN ) {

            //
            // If the buffer is readonly and readonly isn't OK,
            // reject the buffer.
            //

            if (!ReadonlyOK && (Buffer->BufferType & SECBUFFER_READONLY))
            {
                return  FALSE;
            }

            //
            // It is possible that there are > 1 buffers of type SECBUFFER_TOKEN
            // eg, the rdr
            //

            if (Index != BufferIndex)
            {
                Index++;
                continue;
            }

            //
            // Return the requested information
            //

            if (!NT_SUCCESS(LsaFunctions->MapBuffer(Buffer, Buffer)))
            {
                return FALSE;
            }
            *Token = Buffer;
            return TRUE;
        }

    }

    //
    // If we didn't have a buffer, fine.
    //

    SspPrint((SSP_API_MORE, "SspGetTokenBuffer: No token passed in\n"));

    return TRUE;
}

