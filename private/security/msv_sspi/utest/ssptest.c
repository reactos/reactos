/*--

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ssptest.c

Abstract:

    Test program for the NtLmSsp service.

Author:

    28-Jun-1993 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>     // Needed for service controller APIs
#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>
#include <stdio.h>      // printf
#include <stdlib.h>     // strtoul
#include <tstring.h>    // NetpAllocWStrFromWStr


#include <security.h>   // General definition of a Security Support Provider
#include <ntmsv1_0.h>
#include <ntlmsp.h>
#include <ntlmssp.h>    // External definition of the NtLmSsp service
#include <dsgetdc.h>    // External definition of the NtLmSsp service

BOOLEAN QuietMode = FALSE; // Don't be verbose
extern BOOLEAN TestExt;

// BUGBUG Should be in the SDK?
#define MSV1_0_PACKAGE_NAMEW     L"MICROSOFT_AUTHENTICATION_PACKAGE_V1_0"

VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;


    printf("------------------------------------\n");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            printf("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            printf("  ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }

    printf("------------------------------------\n");
}


VOID
PrintTime(
    LPSTR Comment,
    TimeStamp ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - Local time to print

Return Value:

    None

--*/
{
    LARGE_INTEGER LocalTime;

    LocalTime.HighPart = ConvertTime.HighPart;
    LocalTime.LowPart = ConvertTime.LowPart;

    printf( "%s", Comment );

    //
    // If the time is infinite,
    //  just say so.
    //

    if ( LocalTime.HighPart == 0x7FFFFFFF && LocalTime.LowPart == 0xFFFFFFFF ) {
        printf( "Infinite\n" );

    //
    // Otherwise print it more clearly
    //

    } else {

        TIME_FIELDS TimeFields;

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        printf( "%ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second );
    }

}

VOID
PrintStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case ERROR_LOGON_FAILURE:
        printf( " ERROR_LOGON_FAILURE" );
        break;

    case ERROR_ACCESS_DENIED:
        printf( " ERROR_ACCESS_DENIED" );
        break;

    case ERROR_NOT_SUPPORTED:
        printf( " ERROR_NOT_SUPPORTED" );
        break;

    case ERROR_NO_LOGON_SERVERS:
        printf( " ERROR_NO_LOGON_SERVERS" );
        break;

    case ERROR_NO_SUCH_DOMAIN:
        printf( " ERROR_NO_SUCH_DOMAIN" );
        break;

    case ERROR_NO_TRUST_LSA_SECRET:
        printf( " ERROR_NO_TRUST_LSA_SECRET" );
        break;

    case ERROR_NO_TRUST_SAM_ACCOUNT:
        printf( " ERROR_NO_TRUST_SAM_ACCOUNT" );
        break;

    case ERROR_DOMAIN_TRUST_INCONSISTENT:
        printf( " ERROR_DOMAIN_TRUST_INCONSISTENT" );
        break;

    case ERROR_BAD_NETPATH:
        printf( " ERROR_BAD_NETPATH" );
        break;

    case ERROR_FILE_NOT_FOUND:
        printf( " ERROR_FILE_NOT_FOUND" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    case SEC_E_NO_SPM:
        printf( " SEC_E_NO_SPM" );
        break;
    case SEC_E_BAD_PKGID:
        printf( " SEC_E_BAD_PKGID" ); break;
    case SEC_E_NOT_OWNER:
        printf( " SEC_E_NOT_OWNER" ); break;
    case SEC_E_CANNOT_INSTALL:
        printf( " SEC_E_CANNOT_INSTALL" ); break;
    case SEC_E_INVALID_TOKEN:
        printf( " SEC_E_INVALID_TOKEN" ); break;
    case SEC_E_CANNOT_PACK:
        printf( " SEC_E_CANNOT_PACK" ); break;
    case SEC_E_QOP_NOT_SUPPORTED:
        printf( " SEC_E_QOP_NOT_SUPPORTED" ); break;
    case SEC_E_NO_IMPERSONATION:
        printf( " SEC_E_NO_IMPERSONATION" ); break;
    case SEC_E_LOGON_DENIED:
        printf( " SEC_E_LOGON_DENIED" ); break;
    case SEC_E_UNKNOWN_CREDENTIALS:
        printf( " SEC_E_UNKNOWN_CREDENTIALS" ); break;
    case SEC_E_NO_CREDENTIALS:
        printf( " SEC_E_NO_CREDENTIALS" ); break;
    case SEC_E_MESSAGE_ALTERED:
        printf( " SEC_E_MESSAGE_ALTERED" ); break;
    case SEC_E_OUT_OF_SEQUENCE:
        printf( " SEC_E_OUT_OF_SEQUENCE" ); break;
    case SEC_E_INSUFFICIENT_MEMORY:
        printf( " SEC_E_INSUFFICIENT_MEMORY" ); break;
    case SEC_E_INVALID_HANDLE:
        printf( " SEC_E_INVALID_HANDLE" ); break;
    case SEC_E_NOT_SUPPORTED:
        printf( " SEC_E_NOT_SUPPORTED" ); break;

    case SEC_I_CONTINUE_NEEDED:
        printf( " SEC_I_CONTINUE_NEEDED" ); break;

    }

    printf( "\n" );
}

VOID
TestSspRoutine(
    LPWSTR DomainName,
    LPWSTR UserName,
    LPWSTR Password
    )
/*++

Routine Description:

    Test base SSPI functionality

Arguments:

    None

Return Value:

    None

--*/
{
    SECURITY_STATUS SecStatus, TmpStatus;
    CredHandle CredentialHandle1;
    CredHandle CredentialHandle2;
    CtxtHandle ClientContextHandle, NewClientContextHandle;
    CtxtHandle ServerContextHandle, NewServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    ULONG PackageCount,i;
    PSecPkgInfo PackageInfo;
    PSecPkgInfo pTmp;

    SEC_WINNT_AUTH_IDENTITY AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_Lifespan ContextLifespan;
    UCHAR ContextNamesBuffer[sizeof(SecPkgContext_Names)+UNLEN*sizeof(WCHAR)];
    PSecPkgContext_Names ContextNames = (PSecPkgContext_Names) ContextNamesBuffer;

    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    BYTE    bDataBuffer[20];
    BYTE    bSigBuffer[100];
    SecBuffer ExportedBuffer;

#if 0
    DWORD (WINAPI *pDsGetDcNameA)( LPCSTR, LPCSTR, GUID *, LPCSTR, ULONG, PDOMAIN_CONTROLLER_INFOA *);
    do {
        char szBuf[256];
        HINSTANCE  hLogonsrv = LoadLibraryA("logonsrv");
        PDOMAIN_CONTROLLER_INFOA ControllerInfo = NULL;
        ULONG NetStatus = 0;

        if (hLogonsrv == NULL)
        {
            wsprintfA(szBuf, "MSNP32: Logonsrv does not exist\n");
            OutputDebugStringA(szBuf);
            break;
        }

        pDsGetDcNameA = (DWORD (WINAPI *)(LPCSTR, LPCSTR, GUID *, 
                                                  LPCSTR, ULONG,
                                                  PDOMAIN_CONTROLLER_INFOA *))
                                GetProcAddress(hLogonsrv, "DsGetDcNameA");

        if (pDsGetDcNameA == NULL)
        {
            wsprintfA(szBuf, "MSNP32: Entrypoint DsGetDcNameA does not exist in Logonsrv.dll\n");
            OutputDebugStringA(szBuf);
            break;
        }

        NetStatus = (*pDsGetDcNameA)(NULL,
                             "NTDEV",
                             NULL,
                             NULL,
                             DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_FLAT_NAME,
                             &ControllerInfo);

        if (hLogonsrv) FreeLibrary(hLogonsrv);

        if (NetStatus != 0)
        {
            wsprintfA(szBuf, "MSNP32: DsGetDcNameA returned 0x%x\n", NetStatus);
            OutputDebugStringA(szBuf);
            break;
        }

        if (ControllerInfo->DomainName != NULL)
        {
            wsprintfA(szBuf, "MSNP32: DsGetDcName returned DomainControllerName: \"%s\"\n", ControllerInfo->DomainControllerName );
            OutputDebugStringA(szBuf);
        }
        else
        {
            OutputDebugStringA("MSNP32: DsGetDcName returned NULL DomainName\n");
        }
    }while (FALSE);
#endif // #if 0

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    SigBuffers[1].pvBuffer = bSigBuffer;
    SigBuffers[1].cbBuffer = sizeof(bSigBuffer);
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SigBuffers[0].pvBuffer = bDataBuffer;
    SigBuffers[0].cbBuffer = sizeof(bDataBuffer);
    SigBuffers[0].BufferType = SECBUFFER_DATA;
    memset(bDataBuffer,0xeb,sizeof(bDataBuffer));

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

    //
    // Get info about the security packages.
    //

    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );
    TmpStatus = GetLastError();

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "EnumerateSecurityPackages failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
      printf( "PackageCount: %ld\n", PackageCount );
      for ( i= 0; i< PackageCount; i++)
      {
        pTmp = (PackageInfo + i);
        printf( "Name: %ws Comment: %ws\n", pTmp->Name, pTmp->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                pTmp->fCapabilities,
                pTmp->wVersion,
                pTmp->wRPCID,
                pTmp->cbMaxToken );
      }
    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( NTLMSP_NAME, &PackageInfo );
    TmpStatus = GetLastError();

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QuerySecurityPackageInfo failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "Name: %ws Comment: %ws\n", PackageInfo->Name, PackageInfo->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                PackageInfo->fCapabilities,
                PackageInfo->wVersion,
                PackageInfo->wRPCID,
                PackageInfo->cbMaxToken );
    }



    //
    // Acquire a credential handle for the server side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME, // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &CredentialHandle1,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcquireCredentialsHandle failed: ");
        TmpStatus = GetLastError();
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "CredentialHandle1: 0x%lx 0x%lx   ",
                CredentialHandle1.dwLower, CredentialHandle1.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }


    //
    // Acquire a credential handle for the client side
    //


    RtlZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );
// #define DO_OEM 1
#ifndef DO_OEM
    if ( DomainName != NULL ) {
        AuthIdentity.Domain = DomainName;
        AuthIdentity.DomainLength = wcslen(DomainName);
    }
    if ( UserName != NULL ) {
        AuthIdentity.User = UserName;
        AuthIdentity.UserLength = wcslen(UserName);
    }
    if ( Password != NULL ) {
        AuthIdentity.Password = Password;
        AuthIdentity.PasswordLength = wcslen(Password);
    }
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
    //
    // BUGBUG: memory leak here
    //

    if ( DomainName != NULL ) {
        AuthIdentity.Domain = (LPWSTR) NetpAllocStrFromWStr(DomainName);
        AuthIdentity.DomainLength = wcslen(DomainName);
    }
    if ( UserName != NULL ) {
        AuthIdentity.User = (LPWSTR) NetpAllocStrFromWStr(UserName);
        AuthIdentity.UserLength = wcslen(UserName);
    }
    if ( Password != NULL ) {
        AuthIdentity.Password = (LPWSTR) NetpAllocStrFromWStr(Password);
        AuthIdentity.PasswordLength = wcslen(Password);
    }
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME, // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    (DomainName == NULL && UserName == NULL && Password == NULL) ?
                        NULL : &AuthIdentity,
                    NULL,
                    NULL,
                    &CredentialHandle2,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcquireCredentialsHandle failed: " );
        TmpStatus = GetLastError();
        PrintStatus( SecStatus );
        return;
    }


    if ( !QuietMode ) {
        printf( "CredentialHandle2: 0x%lx 0x%lx   ",
                CredentialHandle2.dwLower, CredentialHandle2.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }



    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        printf( "Allocate NegotiateMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    SecStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,               // No Client context yet
                    L"\\\\Frank\\IPC$",  // Faked target name
                    ISC_REQ_SEQUENCE_DETECT | ISC_REQ_DATAGRAM,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        if ( !QuietMode || !NT_SUCCESS(SecStatus) ) {
            printf( "InitializeSecurityContext (negotiate): " );
            PrintStatus( SecStatus );
        }
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nNegotiate Message:\n" );

        printf( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

        DumpBuffer( NegotiateBuffer.pvBuffer, NegotiateBuffer.cbBuffer );
    }


    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        printf( "Allocate ChallengeMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    SecStatus = AcceptSecurityContext(
                    &CredentialHandle1,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ISC_REQ_SEQUENCE_DETECT  | ISC_REQ_DATAGRAM,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        if ( !QuietMode || !NT_SUCCESS(SecStatus) ) {
            printf( "AcceptSecurityContext (Challenge): " );
            PrintStatus( SecStatus );
        }
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nChallenge Message:\n" );

        printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

        DumpBuffer( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
    }


    //
    // Get the AuthenticateMessage (ClientSide)
    //

    ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
    AuthenticateDesc.ulVersion = 0;
    AuthenticateDesc.cBuffers = 1;
    AuthenticateDesc.pBuffers = &AuthenticateBuffer;

    AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
    AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
    if ( AuthenticateBuffer.pvBuffer == NULL ) {
        printf( "Allocate AuthenticateMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    SecStatus = InitializeSecurityContext(
                    NULL,
                    &ClientContextHandle,
                    L"\\\\Frank\\IPC$",     // Faked target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    &ChallengeDesc,
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "InitializeSecurityContext (Authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nAuthenticate Message:\n" );

        printf( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

        DumpBuffer( AuthenticateBuffer.pvBuffer, AuthenticateBuffer.cbBuffer );
    }

    //
    // Finally authenticate the user (ServerSide)
    //

    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    SecStatus = AcceptSecurityContext(
                    NULL,
                    &ServerContextHandle,
                    &AuthenticateDesc,
                    0,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    NULL,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "AcceptSecurityContext (Challenge): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nFinal Authentication:\n" );

        printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );
        printf(" \n" );
    }


    //
    // Query as many attributes as possible
    //


    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_SIZES,
                    &ContextSizes );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "QueryContextAttributes (sizes): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QuerySizes: %ld %ld %ld %ld\n",
                    ContextSizes.cbMaxToken,
                    ContextSizes.cbMaxSignature,
                    ContextSizes.cbBlockSize,
                    ContextSizes.cbSecurityTrailer );
    }

    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_NAMES,
                    ContextNamesBuffer );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "QueryContextAttributes (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryNames: %ws\n", ContextNames->sUserName );
    }


    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_LIFESPAN,
                    &ContextLifespan );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "QueryContextAttributes (lifespan): " );
        PrintStatus( SecStatus );
    }

    if ( NT_SUCCESS(SecStatus) )
    {
        if ( !QuietMode )
        {
            PrintTime("   Start:", ContextLifespan.tsStart );
            PrintTime("  Expiry:", ContextLifespan.tsExpiry );
        }
    }


    //
    // Get the ChallengeMessage (ServerSide)
    //
    // Now make a third call to Initialize to check that RPC can
    // reauthenticate.
    //

    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;


    SecStatus = InitializeSecurityContext(
                    NULL,
                    &ClientContextHandle,
                    L"\\\\Frank\\IPC$",     // Faked target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "InitializeSecurityContext (Re-Authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }



    //
    // Now try to re-authenticate the user (ServerSide)
    //

    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    SecStatus = AcceptSecurityContext(
                    NULL,
                    &ServerContextHandle,
                    &AuthenticateDesc,
                    0,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    NULL,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "AcceptSecurityContext (Re-authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    //
    // Impersonate the client (ServerSide)
    //

    SecStatus = ImpersonateSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        TmpStatus = GetLastError();
        printf( "ImpersonateSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    //
    // Do something while impersonating (Access the token)
    //

    {
        NTSTATUS Status;
        HANDLE TokenHandle = NULL;

        //
        // Open the token,
        //

        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    (BOOLEAN) TRUE, // Not really using the impersonation token
                    &TokenHandle );

        if ( !NT_SUCCESS(Status) ) {
            printf( "Access Thread token while impersonating: " );
            PrintStatus( Status );
            return;
        } else {
            (VOID) NtClose( TokenHandle );
        }
    }


    //
    // RevertToSelf (ServerSide)
    //

    SecStatus = RevertSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "RevertSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    //
    // Sign a message
    //

    SecStatus = MakeSignature(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }


    //
    // Verify the signature
    //

    SecStatus = VerifySignature(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "VerifySignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }



    //
    // Sign a message, this time to check if it can detect a change in the
    // message
    //

    SecStatus = MakeSignature(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }

    //
    // Mess up the message to see if VerifySignature works
    //

    bDataBuffer[10] = 0xec;

    //
    // Verify the signature
    //

    SecStatus = VerifySignature(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != SEC_E_MESSAGE_ALTERED ) {
        printf( "VerifySignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    // Export client context without deleting the old one.

    SecStatus = ExportSecurityContext(
                       &ClientContextHandle,
                       0,
                       &ExportedBuffer, 
                       NULL);

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "ExportSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    SecStatus = ImportSecurityContext(
                        NTLMSP_NAME, // Package Name
                        &ExportedBuffer,
                        NULL,
                        &NewClientContextHandle);

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "ImportSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    SecStatus = FreeContextBuffer(ExportedBuffer.pvBuffer);

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeContextBuffer: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    SecStatus = QueryContextAttributes(
                    &NewClientContextHandle,
                    SECPKG_ATTR_NAMES,
                    ContextNamesBuffer );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryNames: %ws\n", ContextNames->sUserName );
    }

    // Export client context while deleting the old one.

    SecStatus = ExportSecurityContext(
                       &ServerContextHandle,
                       SECPKG_CONTEXT_EXPORT_DELETE_OLD,
                       &ExportedBuffer, 
                       NULL);

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "ExportSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    SecStatus = ImportSecurityContext(
                        NTLMSP_NAME, // Package Name
                        &ExportedBuffer,
                        NULL,
                        &NewServerContextHandle);

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "ImportSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    SecStatus = FreeContextBuffer(ExportedBuffer.pvBuffer);

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeContextBuffer: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    SecStatus = QueryContextAttributes(
                    &NewServerContextHandle,
                    SECPKG_ATTR_NAMES,
                    ContextNamesBuffer );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryNames: %ws\n", ContextNames->sUserName );
    }

    //
    // Delete only the client context. The server context has already been deleted.
    //

    SecStatus = DeleteSecurityContext( &ClientContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }

    //
    // Delete imported contexts
    //

    SecStatus = DeleteSecurityContext( &NewClientContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = DeleteSecurityContext( &NewServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }

    //
    // Free both credential handles
    //

    SecStatus = FreeCredentialsHandle( &CredentialHandle1 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = FreeCredentialsHandle( &CredentialHandle2 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
}

VOID
TestLogonRoutine(
    IN LPWSTR UserName,
    IN LPWSTR DomainName,
    IN LPWSTR Password
    )
{
    NTSTATUS Status;
    PMSV1_0_INTERACTIVE_LOGON LogonInfo;
    ULONG LogonInfoSize = sizeof(MSV1_0_INTERACTIVE_LOGON);
    BOOLEAN WasEnabled;
    UNICODE_STRING Name;
    STRING TempName;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    TOKEN_SOURCE SourceContext;
    PMSV1_0_INTERACTIVE_PROFILE Profile = NULL;
    ULONG ProfileSize;
    LUID LogonId;
    HANDLE TokenHandle = NULL;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    WCHAR UserNameString[100];
    ULONG NameLength = 100;
    PUCHAR Where;

    LogonInfoSize += (wcslen(UserName) + ((DomainName == NULL)? 0 : wcslen(DomainName)) + wcslen(Password) + 3 ) * sizeof(WCHAR);

    LogonInfo = (PMSV1_0_INTERACTIVE_LOGON) LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);

    LogonInfo->MessageType = MsV1_0InteractiveLogon;

    RtlInitUnicodeString(
        &Name,
        UserName
        );

    Where = (PUCHAR) (LogonInfo + 1);

    LogonInfo->UserName.Buffer = (LPWSTR) Where;
    LogonInfo->UserName.Length = Name.Length;
    LogonInfo->UserName.MaximumLength = Name.MaximumLength;
    RtlCopyMemory(
        Where,
        Name.Buffer,
        Name.Length
        );
    Where += LogonInfo->UserName.Length + sizeof(WCHAR);

    RtlInitUnicodeString(
        &Name,
        DomainName
        );

    LogonInfo->LogonDomainName.Buffer = (LPWSTR) Where;
    LogonInfo->LogonDomainName.Length = Name.Length;
    LogonInfo->LogonDomainName.MaximumLength = Name.MaximumLength;
    RtlCopyMemory(
        Where,
        Name.Buffer,
        Name.Length
        );
    Where += LogonInfo->LogonDomainName.Length + sizeof(WCHAR);

    RtlInitUnicodeString(
        &Name,
        Password
        );

    LogonInfo->Password.Buffer = (LPWSTR) Where;
    LogonInfo->Password.Length = Name.Length;
    LogonInfo->Password.Length = Name.MaximumLength;
    RtlCopyMemory(
        Where,
        Name.Buffer,
        Name.Length
        );
    Where += LogonInfo->Password.Length + sizeof(WCHAR);

    LogonInfo->MessageType = MsV1_0InteractiveLogon;

    //
    // Turn on the TCB privilege
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to adjust privilege: GetLastError = 0x%x\n",GetLastError());
        printf("Failed to adjust privilege: 0x%x\n",Status);
        return;
    }
    RtlInitString(
        &TempName,
        "SspTest"
        );
    Status = LsaRegisterLogonProcess(
                &TempName,
                &LogonHandle,
                &Dummy
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to register as a logon process: 0x%x\n",Status);
        return;
    }

    strncpy(
        SourceContext.SourceName,
        "ssptest        ",sizeof(SourceContext.SourceName)
        );
    NtAllocateLocallyUniqueId(
        &SourceContext.SourceIdentifier
        );


    RtlInitString(
        &TempName,
        NTLMSP_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &TempName,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to lookup package %Z: 0x%x\n",&TempName, Status);
        return;
    }

    //
    // Now call LsaLogonUser
    //

    RtlInitString(
        &TempName,
        "ssptest"
        );

    Status = LsaLogonUser(
                LogonHandle,
                &TempName,
                Interactive,
                PackageId,
                LogonInfo,
                LogonInfoSize,
                NULL,           // no token groups
                &SourceContext,
                (PVOID *) &Profile,
                &ProfileSize,
                &LogonId,
                &TokenHandle,
                &Quotas,
                &SubStatus
                );
    if (!NT_SUCCESS(Status))
    {
        printf("lsalogonuser failed: 0x%x\n",Status);
        return;
    }
    if (!NT_SUCCESS(SubStatus))
    {
        printf("LsalogonUser failed: substatus = 0x%x\n",SubStatus);
        return;
    }

    ImpersonateLoggedOnUser( TokenHandle );
    GetUserName(UserNameString,&NameLength);
    printf("Username = %ws\n",UserNameString);
    RevertToSelf();
    NtClose(TokenHandle);



}

DWORD
JunkTest()
{
    return 1;
}


int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Drive the NtLmSsp service

Arguments:



int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Drive the NtLmSsp service

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    LPSTR argument;
    int i;
    ULONG j;
    ULONG Iterations = 0;

    LPWSTR DomainName = NULL;
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;

    enum {
        NoAction,
        ConfigureService,
#define CONFIG_PARAM "/ConfigureService"
        TestSsp,
#define TESTSSP_PARAM "/TestSsp"
#define TESTSSP2_PARAM "/TestSsp:"
#define LOGON_PARAM "/TestLogon"
        TestLogon,
    } Action = NoAction;
#define QUIET_PARAM "/Q"





    //
    // Loop through the arguments handle each in turn
    //

    for ( i=1; i<argc; i++ ) {

        argument = argv[i];

        //
        // Handle /TestSsp
        //

        if ( _stricmp( argument, TESTSSP_PARAM ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestSsp;
            Iterations = 1;

        //
        // Handle /TestSsp:
        //

        } else if ( _strnicmp( argument,
                              TESTSSP2_PARAM,
                              sizeof(TESTSSP2_PARAM)-1 ) == 0 ){
            char *end;
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestSsp;

            Iterations = strtoul( &argument[sizeof(TESTSSP2_PARAM)-1], &end, 10 );

            i++;
            if ( i < argc ) {
                argument = argv[i];
                DomainName = NetpAllocWStrFromStr( argument );

                i++;
                if ( i < argc ) {
                    argument = argv[i];
                    UserName = NetpAllocWStrFromStr( argument );

                    i++;
                    if ( i < argc ) {
                        argument = argv[i];
                        Password = NetpAllocWStrFromStr( argument );
                    }
                }
            }


        } else if ( _strnicmp( argument,
                              LOGON_PARAM,
                              sizeof(LOGON_PARAM)-1 ) == 0 ){
        //
        // Handle /TestLogon
        //
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestLogon;
            Iterations = 1;
            
            if (argc < i + 2)
            {
                goto Usage;
            }
            argument = argv[++i];
            Password = NetpAllocWStrFromStr( argument );
            argument = argv[++i];
            UserName = NetpAllocWStrFromStr( argument );
            if (i < argc)
            {
                argument = argv[++i];
                DomainName = NetpAllocWStrFromStr( argument );
            }
            else
            {
                DomainName = NULL;
            }

        } else {
        //
        // Handle all other parameters
        //

Usage:
            fprintf( stderr, "Usage: ssptest [/OPTIONS]\n\n" );

            fprintf(
                stderr,
                "\n"
                "    " TESTSSP_PARAM "[:<iterations> <DomainName> <UserName> <Password>] - Test basic SSPI.\n"
                "    " LOGON_PARAM "<Password> <UserName> [<DomainName>] - Test LogonUser.\n"
                "    " QUIET_PARAM " - Don't be so verbose\n"
                "\n"
                "\n" );
            return(1);
        }
    }

    //
    // Perform the action requested
    //

    switch ( Action ) 
    {
        case TestSsp: 
        {
            for ( j=0; j<Iterations ; j++ ) {
                TestSspRoutine( DomainName, UserName, Password );
            }
            break;
        }
        case TestLogon: 
        {
            TestLogonRoutine( UserName, DomainName, Password );
        }
    }

    return 0;

}

