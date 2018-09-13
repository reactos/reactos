/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    LOGCLEAR.C

Abstract:

    Contains functions used to log an event indicating who cleared the log.
    This is only called after the security log has been cleared.

Author:

    Dan Lafferty (danl)     01-July-1994

Environment:

    User Mode -Win32

Revision History:

    01-July-1994    danl & robertre
        Created - Rob supplied the code which I fitted into the eventlog.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <msaudite.h>
#include <eventp.h>
#include <tstr.h>

#define NUM_STRINGS     6

//
// Globals
//
    PUNICODE_STRING pGlobalComputerNameU = NULL;
    PANSI_STRING    pGlobalComputerNameA = NULL;

//
// LOCAL FUNCTION PROTOTYPES
//
BOOL
GetUserInfo(
    IN HANDLE Token,
    OUT LPWSTR *UserName,
    OUT LPWSTR *DomainName,
    OUT LPWSTR *AuthenticationId,
    OUT PSID *UserSid
    );

NTSTATUS
ElfpGetComputerName (
    IN  LPSTR   *ComputerNamePtr
    );

PUNICODE_STRING
TmpGetComputerNameW (VOID);

VOID
w_GetComputerName ( );


VOID
ElfpGenerateLogClearedEvent(
    IELF_HANDLE     LogHandle
    )

/*++

Routine Description:

    This function generates an event indicating that the log was cleared.

Arguments:

    LogHandle - This is a handle to the log that the event is to be placed in.
        It is intended that this function only be called when the SecurityLog
        has been cleared.  So it is expected the LogHandle will always be
        a handle to the security log.

Return Value:

    NONE - Either it works or it doesn't.  If it doesn't, there isn't much
        we can do about it.

--*/
{
    LPWSTR  UserName=NULL;
    LPWSTR  DomainName=NULL;
    LPWSTR  AuthenticationId=NULL;
    LPWSTR  ClientUserName=NULL;
    LPWSTR  ClientDomainName=NULL;
    LPWSTR  ClientAuthenticationId=NULL;
    PSID    UserSid=NULL;
    PSID    ClientSid=NULL;
    DWORD   i;
    BOOL    Result;
    HANDLE  Token;
    PUNICODE_STRING StringPtrArray[NUM_STRINGS];
    UNICODE_STRING  StringArray[NUM_STRINGS];
    NTSTATUS        Status;
    LARGE_INTEGER   Time;
    ULONG           EventTime;
    ULONG           LogHandleGrantedAccess;
    PUNICODE_STRING pComputerNameU;
    DWORD           dwStatus;

    //
    // Get information about the Eventlog service (i.e., LocalSystem)
    //
    Result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token);

    if (!Result) {
        ASSERT(FALSE);
        ElfDbgPrint(("OpenProcessToken failed, error = %d", GetLastError()));
        return;
    }

    Result = GetUserInfo(Token,
                         &UserName,
                         &DomainName,
                         &AuthenticationId,
                         &UserSid);

    CloseHandle(Token);

    if (!Result) {
        ElfDbgPrint(("1st GetUserInfo ret'd %d\n",GetLastError()));
        return;
    }

    ElfDbgPrint(("\nGetUserInfo ret'd \nUserName = %ws, "
                 "\nDomainName = %ws, \nAuthenticationId = %ws\n",
                 UserName, DomainName, AuthenticationId));

    //
    // Now impersonate in order to get the client's
    // information.  This call should never fail.
    //
    dwStatus = RpcImpersonateClient(NULL);

    if (dwStatus != RPC_S_OK) {
        ElfDbgPrint(("RPC IMPERSONATION FAILED %d\n", dwStatus));
        ASSERT(FALSE);
    }

    Result = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &Token);

    if (Result) {

        Result = GetUserInfo(Token,
                             &ClientUserName,
                             &ClientDomainName,
                             &ClientAuthenticationId,
                             &ClientSid);

        CloseHandle(Token);

        if (!Result) {
            ElfDbgPrint(("2nd GetUserInfo ret'd %d\n", GetLastError()));
            goto CleanExit;
        }

    } else {

        //
        // We're impersonating here, so this should never
        // happen but just in case, use dashes
        //
        ASSERT(FALSE);

        ClientUserName = L"-";
        ClientDomainName = L"-";
        ClientAuthenticationId = L"-";
    }

    ElfDbgPrint(("\nGetUserInfo ret'd \nUserName = %ws, "
                 "\nDomainName = %ws, \nAuthenticationId = %ws\n",
                 ClientUserName, ClientDomainName, ClientAuthenticationId ));


	RtlInitUnicodeString( &StringArray[0], UserName );
	RtlInitUnicodeString( &StringArray[1], DomainName );
	RtlInitUnicodeString( &StringArray[2], AuthenticationId );
	RtlInitUnicodeString( &StringArray[3], ClientUserName );
	RtlInitUnicodeString( &StringArray[4], ClientDomainName );
	RtlInitUnicodeString( &StringArray[5], ClientAuthenticationId );

    //
    // Create an array of pointers to UNICODE_STRINGs.
    //
    for (i = 0; i < NUM_STRINGS; i++) {
        StringPtrArray[i] = &StringArray[i];
    }

    //
    // Generate the time of the event. This is done on the client side
    // since that is where the event occurred.
    //
    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time, &EventTime);

    //
    // Generate the ComputerName of the client.
    // We have to do this in the client side since this call may be
    // remoted to another server and we would not necessarily have
    // the computer name there.
    //
    pComputerNameU = TmpGetComputerNameW();

    //
    // Since all processes other than LSA are given read-only access
    // to the security log, we have to explicitly give the current
    // process the right to write the "Log cleared" event
    //
    LogHandleGrantedAccess    = LogHandle->GrantedAccess;
    LogHandle->GrantedAccess |= ELF_LOGFILE_WRITE;

    Status = ElfrReportEventW (
                 LogHandle,                         // Log Handle
                 EventTime,                         // Time
                 EVENTLOG_AUDIT_SUCCESS,            // Event Type
                 (USHORT)SE_CATEGID_SYSTEM,         // Event Category
                 SE_AUDITID_AUDIT_LOG_CLEARED,      // EventID
                 NUM_STRINGS,                       // NumStrings
                 0,                                 // DataSize
                 pComputerNameU,                    // pComputerNameU
                 UserSid,                           // UserSid
                 StringPtrArray,                    // *Strings
                 NULL,                              // Data
                 0,                                 // Flags
                 NULL,                              // RecordNumber
                 NULL                               // TimeWritten
                 );

    LogHandle->GrantedAccess = LogHandleGrantedAccess;

CleanExit:
    //
    // We only come down this path if we know for sure that these
    // first three have been allocated.
    //
    ElfpFreeBuffer(UserName);
    ElfpFreeBuffer(DomainName);
    ElfpFreeBuffer(AuthenticationId);

    if (UserSid != NULL) {
        ElfpFreeBuffer(UserSid);
    }
    if (ClientUserName != NULL) {
        ElfpFreeBuffer(ClientUserName);
    }
    if (ClientDomainName != NULL) {
        ElfpFreeBuffer(ClientDomainName);
    }
    if (ClientAuthenticationId != NULL) {
        ElfpFreeBuffer(ClientAuthenticationId);
    }
    if (ClientSid != NULL) {
        ElfpFreeBuffer(ClientSid);
    }

    //
    // Stop impersonating
    //
    dwStatus = RpcRevertToSelf();
    
    if (dwStatus != RPC_S_OK) {
        ElfDbgPrint(("RPC REVERT TO SELF FAILED %d\n", dwStatus));
        ASSERT(FALSE);
    }
}


BOOL
GetUserInfo(
    IN HANDLE Token,
    OUT LPWSTR *UserName,
    OUT LPWSTR *DomainName,
    OUT LPWSTR *AuthenticationId,
    OUT PSID *UserSid
    )
/*++

Routine Description:

    This function gathers information about the user identified with the
    token.

Arguments:
    Token - This token identifies the entity for which we are gathering
        information.
    UserName - This is the entity's user name.
    DomainName -  This is the entity's domain name.
    AuthenticationId -  This is the entity's Authentication ID.
    UserSid - This is the entity's SID.

NOTE:
    Memory is allocated by this routine for UserName, DomainName,
    AuthenticationId, and UserSid.  It is the caller's responsibility to
    free this memory.

Return Value:
    TRUE - If the operation was successful.  It is possible that
        UserSid did not get allocated in the successful case.  Therefore,
        the caller should test prior to freeing.
    FALSE - If unsuccessful.  No memory is allocated in this case.


--*/
{
    PTOKEN_USER     Buffer=NULL;
//    WCHAR         User[256];
    LPWSTR          Domain = NULL;
    LPWSTR          AccountName = NULL;
    SID_NAME_USE    Use;
    BOOL            Result;
    DWORD           RequiredLength;
    DWORD           AccountNameSize;
    DWORD           DomainNameSize;
    TOKEN_STATISTICS Statistics;
    WCHAR           LogonIdString[256];
    DWORD           Status=ERROR_SUCCESS;


    *UserSid = NULL;

    Result = GetTokenInformation ( Token, TokenUser, NULL, 0, &RequiredLength );

    if (!Result) {

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            Buffer = ElfpAllocateBuffer((RequiredLength+1)*sizeof(WCHAR) );

            Result = GetTokenInformation ( Token,
                                           TokenUser,
                                           Buffer,
                                           RequiredLength,
                                           &RequiredLength
                                           );

            if (!Result) {
                ElfDbgPrint(("2nd GetTokenInformation failed, "
                    "error = %d\n",GetLastError()));
                return( FALSE );
            }

        } else {

            DbgPrint("1st GetTokenInformation failed, error = %d\n",GetLastError());
            return( FALSE );
        }
    }


    if (!Result) {
        goto ErrorCleanup;
    }

    AccountNameSize = 0;
    DomainNameSize = 0;

    Result = LookupAccountSidW( L"",
                               Buffer->User.Sid,
                               NULL,
                               &AccountNameSize,
                               NULL,
                               &DomainNameSize,
                               &Use
                               );

    if (!Result) {

        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {

            AccountName = ElfpAllocateBuffer((AccountNameSize+1)*sizeof(WCHAR) );
            Domain = ElfpAllocateBuffer((DomainNameSize+1)*sizeof(WCHAR) );

            if ( AccountName == NULL ) {
                ElfDbgPrint(("LocalAlloc failed allocating %d bytes, "
                    "error = %d\n",AccountNameSize,GetLastError()));
                goto ErrorCleanup;
            }

            if ( Domain == NULL ) {
                ElfDbgPrint(("LocalAlloc failed allocating %d bytes, "
                    "error = %d\n",DomainNameSize,GetLastError()));
                goto ErrorCleanup;
            }

            Result = LookupAccountSidW( L"",
                                       Buffer->User.Sid,
                                       AccountName,
                                       &AccountNameSize,
                                       Domain,
                                       &DomainNameSize,
                                       &Use
                                       );
            if (!Result) {

                ElfDbgPrint(("2nd LookupAccountSid failed, "
                    "error = %d\n",GetLastError()));
                goto ErrorCleanup;
            }

        } else {

            ElfDbgPrint(("1st LookupAccountSid failed, "
                "error = %d\n",GetLastError()));
            goto ErrorCleanup;
        }

    } else {

        ElfDbgPrint(("LookupAccountSid succeeded unexpectedly\n"));
        goto ErrorCleanup;
    }

    ElfDbgPrint(("Name = %ws\\%ws\n",Domain,AccountName));

    Result = GetTokenInformation ( Token, TokenStatistics, &Statistics, sizeof( Statistics ), &RequiredLength );

    if (!Result) {
        ElfDbgPrint(("GetTokenInformation failed, error = %d\n",GetLastError()));
        goto ErrorCleanup;
    }

    swprintf(LogonIdString, L"(0x%X,0x%X)",Statistics.AuthenticationId.HighPart, Statistics.AuthenticationId.LowPart );
    ElfDbgPrint(("LogonIdString = %ws\n",LogonIdString));

    *AuthenticationId = ElfpAllocateBuffer(WCSSIZE(LogonIdString));
    if (*AuthenticationId == NULL) {
        ElfDbgPrint(("[ELF]GetUserInfo: Failed to allocate buffer "
            "for AuthenticationId %d\n",GetLastError()));
        goto ErrorCleanup;
    }
    wcscpy(*AuthenticationId, LogonIdString);

    //
    // Return accumulated information
    //

    *UserSid = ElfpAllocateBuffer(GetLengthSid( Buffer->User.Sid ) );

    Result = CopySid( GetLengthSid( Buffer->User.Sid ), *UserSid, Buffer->User.Sid );

    ElfpFreeBuffer(Buffer);

    *DomainName = Domain;
    *UserName = AccountName;

    return( TRUE );

ErrorCleanup:

    if (Buffer != NULL) {
        ElfpFreeBuffer( Buffer );
    }

    if (Domain != NULL) {
        ElfpFreeBuffer( Domain );
    }

    if (AccountName != NULL) {
        ElfpFreeBuffer( AccountName );
    }

    if (*UserSid != NULL) {
        ElfpFreeBuffer( *UserSid );
    }

    if (*AuthenticationId != NULL) {
        ElfpFreeBuffer( *AuthenticationId );
    }

    return( FALSE );
}

NTSTATUS
ElfpGetComputerName (
    IN  LPSTR   *ComputerNamePtr)

/*++

Routine Description:

    This routine obtains the computer name from a persistent database,
    by calling the GetcomputerNameA Win32 Base API

    This routine assumes the length of the computername is no greater
    than MAX_COMPUTERNAME_LENGTH, space for which it allocates using
    LocalAlloc.  It is necessary for the user to free that space using
    ElfpFreeBuffer when finished.

Arguments:

    ComputerNamePtr - This is a pointer to the location where the pointer
        to the computer name is to be placed.

Return Value:

    NERR_Success - If the operation was successful.

    It will return assorted Net or Win32 or NT error messages if not.

--*/
{
    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;

    //
    // Allocate a buffer to hold the largest possible computer name.
    //

    *ComputerNamePtr = ElfpAllocateBuffer(nSize);

    if (*ComputerNamePtr == NULL) {
        return (GetLastError());
    }

    //
    // Get the computer name string into the locally allocated buffer
    // by calling the Win32 GetComputerNameA API.
    //

    if (!GetComputerNameA(*ComputerNamePtr, &nSize)) {
        ElfpFreeBuffer(*ComputerNamePtr);
        *ComputerNamePtr = NULL;
        return (GetLastError());
    }

    return (ERROR_SUCCESS);
}

VOID
w_GetComputerName ( )

/*++

Routine Description:

    This routine gets the name of the computer. It checks the global
    variable to see if the computer name has already been determined.
    If not, it updates that variable with the name.
    It does this for the UNICODE and the ANSI versions.

Arguments:

    NONE

Return Value:

    NONE


--*/
{
    PUNICODE_STRING     pNameU=NULL;
    PANSI_STRING        pNameA=NULL;
    LPSTR               pName;
    NTSTATUS            Error;
    NTSTATUS            Status;


    if  (pGlobalComputerNameU != NULL) {
        return;
    }
    pNameU = ElfpAllocateBuffer (sizeof (UNICODE_STRING));
    pNameA = ElfpAllocateBuffer (sizeof (ANSI_STRING));

    if ((pNameU != NULL) && (pNameA != NULL)) {

        if ((Error = ElfpGetComputerName (&pName)) == ERROR_SUCCESS) {

            //
            // ElfpComputerName has allocated a buffer to contain the
            // ASCII name of the computer. We use that for the ANSI
            // string structure.
            //
            RtlInitAnsiString ( pNameA, pName );

        } else {
            //
            // We could not get the computer name for some reason. Set up
            // the golbal pointer to point to the NULL string.
            //
            RtlInitAnsiString ( pNameA, "\0");
        }

        //
        // Set up the UNICODE_STRING structure.
        //
        Status = RtlAnsiStringToUnicodeString (
                            pNameU,
                            pNameA,
                            TRUE
                            );

        //
        // If there was no error, set the global variables.
        // Otherwise, free the buffer allocated by ElfpGetComputerName
        // and leave the global variables unchanged.
        //
        if (NT_SUCCESS(Status)) {

            pGlobalComputerNameU = pNameU;      // Set global variable if no error
            pGlobalComputerNameA = pNameA;      // Set global ANSI variable

        } else {

            ElfDbgPrint(("[ELFCLNT] GetComputerName - Error 0x%lx\n", Status));
            ElfpFreeBuffer(pName);
            ElfpFreeBuffer (pNameU);        // Free the buffers
            ElfpFreeBuffer (pNameA);
        }

    }
}


PUNICODE_STRING
TmpGetComputerNameW ( )

/*++

Routine Description:

    This routine gets the UNICODE name of the computer. It checks the global
    variable to see if the computer name has already been determined.
    If not, it calls the worker routine to do that.

Arguments:

    NONE

Return Value:

    Returns a pointer to the computer name, or a NULL.


--*/
{
    if (pGlobalComputerNameU == NULL) {
        w_GetComputerName();
    }
    return (pGlobalComputerNameU);
}



