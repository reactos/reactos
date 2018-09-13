/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msvpaswd.c

Abstract:

    This file contains the MSV1_0 Authentication Package password routines.

Author:

    Dave Hart    (davehart)   12-Mar-1992

Revision History:
    Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\msvpaswd.c

--*/

#include <global.h>

#include "msp.h"
#include "nlp.h"

#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmremutl.h>
#include <lmwksta.h>



NTSTATUS
MspStopImpersonating (
    VOID
    )

/*++

Routine Description:

    Stop impersonating.  This is used to stop impersonating either
    ourselves (see MspDisableAdminsAlis) or a client.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    Other values are unexpected, but will be those returned by
    NtSetInformationThread.

--*/

{

    NTSTATUS
        NtStatus;

    HANDLE
        Token = NULL;

    NtStatus = NtSetInformationThread(
                   NtCurrentThread(),
                   ThreadImpersonationToken,
                   (PVOID)&Token,
                   sizeof(Token)
                   );

#if DBG
    if ( !NT_SUCCESS(NtStatus) ) {

        KdPrint(("MspStopImpersonating: Cannot stop impersonating, status %x\n",
                 NtStatus));
    }
#endif \\DBG

    return( NtStatus );

}

NTSTATUS
MspDisableAdminsAlias (
    VOID
    )

/*++

Routine Description:

    Remove the current thread from the Administrators alias.  This
    is accomplished by impersonating our own thread, then removing
    the Administrators alias membership from the impersonation
    token.  Use MspStopImpersonating() to stop impersonating and
    thereby restore the thread to the Administrators alias.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

--*/

{
    NTSTATUS                 Status;
    HANDLE                   TokenHandle = NULL;
    HANDLE                   FilteredToken = NULL;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = SECURITY_NT_AUTHORITY;
    PSID                     AdminSid = NULL;
    SID                      LocalSystemSid = {SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID};
    BYTE                     GroupBuffer[sizeof(TOKEN_GROUPS) + sizeof(SID_AND_ATTRIBUTES)];
    PTOKEN_GROUPS            TokenGroups = (PTOKEN_GROUPS) GroupBuffer;

    //
    // Make sure we aren't impersonating anyone else
    // (that will prevent the RtlImpersonateSelf() call from succeeding).
    //

    Status = MspStopImpersonating();

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Open our process token so we can filter it to disable the
    // Administrators and LocalSystem SIDs
    //

    Status = RtlImpersonateSelf(SecurityDelegation);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY,
                TRUE,           // open as self
                &TokenHandle
                );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Build the SID for the Administrators alias.  The Administrators
    // alias SID is well known, S-1-5-32-544.
    //

    Status = RtlAllocateAndInitializeSid(
        &IdentifierAuthority,         // SECURITY_NT_AUTHORITY (5)
        2,                            // SubAuthorityCount
        SECURITY_BUILTIN_DOMAIN_RID,  // 32
        DOMAIN_ALIAS_RID_ADMINS,      // 544
        0,0,0,0,0,0,
        &AdminSid
        );

    if ( !NT_SUCCESS(Status) ) {

        KdPrint(("MspDisableAdminsAlias: RtlAllocateAndInitializeSid returns %x\n",
                 Status));
        goto Cleanup;
    }

    //
    // Disable the Administrators and LocalSystem aliases.
    //

    TokenGroups->GroupCount = 2;
    TokenGroups->Groups[0].Sid = AdminSid;
    TokenGroups->Groups[0].Attributes = 0;   // SE_GROUP_ENABLED not on.
    TokenGroups->Groups[1].Sid = &LocalSystemSid;
    TokenGroups->Groups[1].Attributes = 0;   // SE_GROUP_ENABLED not on.

    Status = NtFilterToken(
                 TokenHandle,
                 0,                     // no flags
                 TokenGroups,
                 NULL,                  // no privileges
                 NULL,                  // no restricted sids
                 &FilteredToken
                 );

    if ( !NT_SUCCESS(Status) ) {

        KdPrint(("MspDisableAdminsAlias: NtFilter returns %x\n",
                 Status));
        goto Cleanup;
    }
    Status = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                &FilteredToken,
                sizeof(HANDLE)
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

Cleanup:

    if (AdminSid) {
        RtlFreeSid(AdminSid);
    }

    if (TokenHandle) {
        NtClose(TokenHandle);
    }

    if (FilteredToken) {
        NtClose(FilteredToken);
    }

    return Status;
}


NTSTATUS
MspAddBackslashesComputerName(
    IN PUNICODE_STRING ComputerName,
    OUT PUNICODE_STRING UncComputerName
    )

/*++

Routine Description:

    This function makes a copy of a Computer Name, prepending backslashes
    if they are not already present.

Arguments:

    ComputerName - Pointer to Computer Name without backslashes.

    UncComputerName - Pointer to Unicode String structure that will be
        initialized to reference the computerName with backslashes
        prepended if not already present.  The Unicode Buffer will be
        terminated with a Unicode NULL, so that it can be passed as
        a parameter to routines expecting a null terminated Wide String.
        When this string is finished with, the caller must free its
        memory via RtlFreeHeap.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN HasBackslashes = FALSE;
    BOOLEAN IsNullTerminated = FALSE;
    USHORT OutputNameLength;
    USHORT OutputNameMaximumLength;
    PWSTR StartBuffer = NULL;

    //
    // If the computername is NULL, a zero length string, or the name already begins with
    // backslashes and is wide char null terminated, just use it unmodified.
    //

    if( (!ARGUMENT_PRESENT(ComputerName)) || ComputerName->Length == 0 ) {
        UncComputerName->Buffer = NULL;
        UncComputerName->Length = 0;
        UncComputerName->MaximumLength = 0;
        goto AddBackslashesComputerNameFinish;
    }

    //
    // Name is not NULL or zero length.  Check if name already has
    // backslashes and a trailing Unicode Null
    //

    OutputNameLength = ComputerName->Length + (2 * sizeof(WCHAR));
    OutputNameMaximumLength = OutputNameLength + sizeof(WCHAR);

    if ((ComputerName && ComputerName->Length >= 2 * sizeof(WCHAR)) &&
        (ComputerName->Buffer[0] == L'\\') &&
        (ComputerName->Buffer[1] == L'\\')) {

        HasBackslashes = TRUE;
        OutputNameLength -= (2 * sizeof(WCHAR));
        OutputNameMaximumLength -= (2 * sizeof(WCHAR));
    }

    if ((ComputerName->Length + (USHORT) sizeof(WCHAR) <= ComputerName->MaximumLength) &&
        (ComputerName->Buffer[ComputerName->Length/sizeof(WCHAR)] == UNICODE_NULL)) {

        IsNullTerminated = TRUE;
    }

    if (HasBackslashes && IsNullTerminated) {

        *UncComputerName = *ComputerName;
        goto AddBackslashesComputerNameFinish;
    }

    //
    // Name either does not have backslashes or is not NULL terminated.
    // Make a copy with leading backslashes and a wide NULL terminator.
    //

    UncComputerName->Length = OutputNameLength;
    UncComputerName->MaximumLength = OutputNameMaximumLength;

    UncComputerName->Buffer = RtlAllocateHeap(
                                 MspHeap,
                                 0,
                                 OutputNameMaximumLength
                                 );

    if (UncComputerName->Buffer == NULL) {

        KdPrint(("MspAddBackslashes...: Out of memory copying ComputerName.\n"));
        Status = STATUS_NO_MEMORY;
        goto AddBackslashesComputerNameError;
    }

    StartBuffer = UncComputerName->Buffer;

    if (!HasBackslashes) {

        UncComputerName->Buffer[0] = UncComputerName->Buffer[1] = L'\\';
        StartBuffer +=2;
    }

    RtlCopyMemory(
        StartBuffer,
        ComputerName->Buffer,
        ComputerName->Length
        );

    UncComputerName->Buffer[UncComputerName->Length / sizeof(WCHAR)] = UNICODE_NULL;

AddBackslashesComputerNameFinish:

    return(Status);

AddBackslashesComputerNameError:

    goto AddBackslashesComputerNameFinish;
}



#ifndef DONT_LOG_PASSWORD_CHANGES
#include <stdio.h>
HANDLE MsvPaswdLogFile = NULL;
#define MSVPASWD_LOGNAME L"\\debug\\PASSWD.LOG"
#define MSVPASWD_BAKNAME L"\\debug\\PASSWD.BAK"

ULONG
MsvPaswdInitializeLog(
    VOID
    )
/*++

Routine Description:

    Initializes the debugging log file used by DCPROMO and the dssetup apis

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
{
    ULONG dwErr = ERROR_SUCCESS;
    WCHAR LogFileName[ MAX_PATH + 1 ], BakFileName[ MAX_PATH + 1 ];


    if ( !GetWindowsDirectoryW( LogFileName,
                                sizeof( LogFileName )/sizeof( WCHAR ) ) ) {

        dwErr = GetLastError();
    } else {

        wcscpy( BakFileName, LogFileName );
        wcscat( LogFileName, MSVPASWD_LOGNAME );
        wcscat( BakFileName, MSVPASWD_BAKNAME );

        //
        // Copy the existing (maybe) log file to a backup
        //
    //if ( CopyFile( LogFileName, BakFileName, FALSE ) == FALSE ) {
    //
    // }


        MsvPaswdLogFile = CreateFileW( LogFileName,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL );

        if ( MsvPaswdLogFile == INVALID_HANDLE_VALUE ) {

            dwErr = GetLastError();

            MsvPaswdLogFile = NULL;

        } else {

            if( SetFilePointer( MsvPaswdLogFile,
                                0, 0,
                                FILE_END ) == 0xFFFFFFFF ) {

                dwErr = GetLastError();

                CloseHandle( MsvPaswdLogFile );
                MsvPaswdLogFile = NULL;
            }

        }
    }

    return( dwErr );
}



ULONG
MsvPaswdCloseLog(
    VOID
    )
/*++

Routine Description:

    Closes the debugging log file used by DCPROMO and the dssetup apis

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
{
    ULONG dwErr = ERROR_SUCCESS;

    if ( MsvPaswdLogFile != NULL ) {

        CloseHandle( MsvPaswdLogFile );
        MsvPaswdLogFile = NULL;
    }

    return( dwErr );
}



//
// Stolen and hacked up from netlogon code
//

VOID
MsvPaswdDebugDumpRoutine(
    IN LPSTR Format,
    va_list arglist
    )

{
    char OutputBuffer[2049];
    ULONG length;
    ULONG BytesWritten;
    SYSTEMTIME SystemTime;
    static BeginningOfLine = TRUE;

    //
    // If we don't have an open log file, just bail
    //
    if ( MsvPaswdLogFile == NULL ) {

        return;
    }

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // If we're writing to the debug terminal,
        //  indicate this is a Netlogon message.
        //

        //
        // Put the timestamp at the begining of the line.
        //

        GetLocalTime( &SystemTime );
        length += (ULONG) sprintf( &OutputBuffer[length],
                                   "%02u/%02u %02u:%02u:%02u ",
                                   SystemTime.wMonth,
                                   SystemTime.wDay,
                                   SystemTime.wHour,
                                   SystemTime.wMinute,
                                   SystemTime.wSecond );
    }

    //
    // Put a the information requested by the caller onto the line
    //
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {

        OutputBuffer[length-1] = '\r';
        OutputBuffer[length] = '\n';
        OutputBuffer[length+1] = '\0';
        length++;
    }

    ASSERT( length <= sizeof( OutputBuffer ) / sizeof( CHAR ) );


    //
    // Write the debug info to the log file.
    //
    if ( !WriteFile( MsvPaswdLogFile,
                     OutputBuffer,
                     length,
                     &BytesWritten,
                     NULL ) ) {

    }


}

VOID
MsvPaswdLogPrintRoutine(
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    va_start(arglist, Format);

    MsvPaswdDebugDumpRoutine( Format, arglist );

    va_end(arglist);

}




ULONG
MsvPaswdSetAndClearLog(
    VOID
    )
/*++

Routine Description:

    Flushes the log and seeks to the end of the file

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

--*/
{
    ULONG dwErr = ERROR_SUCCESS;
    if ( MsvPaswdLogFile != NULL ) {

        if( FlushFileBuffers( MsvPaswdLogFile ) == FALSE ) {

            dwErr = GetLastError();
        }
    }

    return( dwErr );

}

#endif // DONT_LOG_PASSWORD_CHANGES


NTSTATUS
MspChangePasswordSam(
    IN PUNICODE_STRING UncComputerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN BOOLEAN Impersonating,
    OUT PDOMAIN_PASSWORD_INFORMATION *DomainPasswordInfo,
    OUT PPOLICY_PRIMARY_DOMAIN_INFO *PrimaryDomainInfo OPTIONAL,
    OUT PBOOLEAN Authoritative
    )

/*++

Routine Description:

    This routine is called by MspChangePassword to change the password
    on a Windows NT machine.

Arguments:

    UncComputerName - Name of the target machine.  This name must begin with
        two backslashes.

    UserName - Name of the user to change password for.

    OldPassword - Plaintext current password.

    NewPassword - Plaintext replacement password.

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    DomainPasswordInfo - Password restriction information (returned only if
        status is STATUS_PASSWORD_RESTRICTION).

    PrimaryDomainInfo - DomainNameInformation (returned only if status is
        STATUS_BACKUP_CONTROLLER).

    Authoritative - The failure was authoritative and no retries should be
        made.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    ...

--*/

{
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SAM_HANDLE                  SamHandle = NULL;
    SAM_HANDLE                  DomainHandle = NULL;
    LSA_HANDLE                  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES           LSAObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;

    //
    // Assume all failures are authoritative
    //

    *Authoritative = TRUE;

    //
    // If we're impersonating (ie, winlogon impersonated its caller before calling us),
    // impersonate again.  This allows us to get the name of the caller for auditing.
    //

    if ( Impersonating ) {

        Status = (*Lsap.ImpersonateClient)(ClientRequest);

    } else {

        //
        // Since the System context is a member of the Administrators alias,
        // when we connect with the local SAM we come in as an Administrator.
        // (When it's remote, we go over the null session and so have very
        // low access).  We don't want to be an Administrator because that
        // would allow the user to change the password on an account whose
        // ACL prohibits the user from changing the password.  So we'll
        // temporarily impersonate ourself and disable the Administrators
        // alias in the impersonation token.
        //

        Status = MspDisableAdminsAlias();
    }





    if (!NT_SUCCESS( Status )) {
        goto Cleanup;
    }

    try
    {

        Status = SamChangePasswordUser2(
                    UncComputerName,
                    UserName,
                    OldPassword,
                    NewPassword
                    );
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }

    MsvPaswdLogPrint(("SamChangePasswordUser2 on machine %wZ for user %wZ returned 0x%x\n",
        UncComputerName,
        UserName,
        Status
        ));

    if ( !NT_SUCCESS(Status) ) {

#ifdef COMPILED_BY_DEVELOPER
        KdPrint(("MspChangePasswordSam: SamChangePasswordUser2(%wZ) failed, status %x\n",
                 UncComputerName, Status));
#endif // COMPILED_BY_DEVELOPER

        //
        // If we failed to connect and we were impersonating a client
        // then we may want to try again using the NULL session.
        // Only try this if we found a server last try.  Otherwise,
        // we'll subject our user to another long timeout.
        //

        if (( Impersonating ) &&
            ( Status != STATUS_WRONG_PASSWORD ) &&
            ( Status != STATUS_PASSWORD_RESTRICTION ) &&
            ( Status != STATUS_ACCOUNT_RESTRICTION ) &&
            ( Status != RPC_NT_SERVER_UNAVAILABLE) &&
            ( Status != STATUS_INVALID_DOMAIN_ROLE) ) {

            Status = MspDisableAdminsAlias();

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            Status = SamChangePasswordUser2(
                        UncComputerName,
                        UserName,
                        OldPassword,
                        NewPassword
                        );

            MsvPaswdLogPrint(("SamChangePasswordUser2 retry on machine %wZ for user %wZ returned 0x%x\n",
                UncComputerName,
                UserName,
                Status
                ));

#ifdef COMPILED_BY_DEVELOPER
            if ( !NT_SUCCESS(Status) ) {
                KdPrint(("MspChangePasswordSam: SamChangePasswordUser2(%wZ) (2nd attempt) failed, status %x\n",
                 UncComputerName, Status));
                }
#endif // COMPILED_BY_DEVELOPER
        }

    }



    if ( !NT_SUCCESS(Status) ) {

#ifdef COMPILED_BY_DEVELOPER
        KdPrint(("MspChangePasswordSam: Cannot change password for %wZ, status %x\n",
                 UserName, Status));
#endif // COMPILED_BY_DEVELOPER
        if (Status == RPC_NT_SERVER_UNAVAILABLE ||
            Status == RPC_S_SERVER_UNAVAILABLE ) {

            Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
        } else if (Status == STATUS_PASSWORD_RESTRICTION) {

            //
            // Get the password restrictions for this domain and return them
            //



            //
            // Get the SID of the account domain from LSA
            //

            InitializeObjectAttributes( &LSAObjectAttributes,
                                          NULL,             // Name
                                          0,                // Attributes
                                          NULL,             // Root
                                          NULL );           // Security Descriptor

            Status = LsaOpenPolicy( UncComputerName,
                                    &LSAObjectAttributes,
                                    POLICY_VIEW_LOCAL_INFORMATION,
                                    &LSAPolicyHandle );

            if( !NT_SUCCESS(Status) ) {
                KdPrint(("MspChangePasswordSam: LsaOpenPolicy(%wZ) failed, status %x\n",
                         UncComputerName, Status));
                LSAPolicyHandle = NULL;
                goto Cleanup;
            }

            Status = LsaQueryInformationPolicy(
                            LSAPolicyHandle,
                            PolicyAccountDomainInformation,
                            (PVOID *) &AccountDomainInfo );

            if( !NT_SUCCESS(Status) ) {
                KdPrint(("MspChangePasswordSam: LsaQueryInformationPolicy(%wZ) failed, status %x\n",
                         UncComputerName, Status));
                AccountDomainInfo = NULL;
                goto Cleanup;
            }


            //
            // Setup ObjectAttributes for SamConnect call.
            //

            InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);
            ObjectAttributes.SecurityQualityOfService = &SecurityQos;

            SecurityQos.Length = sizeof(SecurityQos);
            SecurityQos.ImpersonationLevel = SecurityIdentification;
            SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
            SecurityQos.EffectiveOnly = FALSE;

            Status = SamConnect(
                         UncComputerName,
                         &SamHandle,
                         SAM_SERVER_LOOKUP_DOMAIN,
                         &ObjectAttributes
                         );


            if ( !NT_SUCCESS(Status) ) {
                KdPrint(("MspChangePasswordSam: Cannot open sam on %wZ, status %x\n",
                         UncComputerName, Status));
                DomainHandle = NULL;
                goto Cleanup;
            }


            //
            // Open the Account domain in SAM.
            //

            Status = SamOpenDomain(
                         SamHandle,
                         GENERIC_EXECUTE,
                         AccountDomainInfo->DomainSid,
                         &DomainHandle
                         );

            if ( !NT_SUCCESS(Status) ) {
                KdPrint(("MspChangePasswordSam: Cannot open domain on %wZ, status %x\n",
                         UncComputerName, Status));
                DomainHandle = NULL;
                goto Cleanup;
            }


            Status = SamQueryInformationDomain(
                            DomainHandle,
                            DomainPasswordInformation,
                            (PVOID *)DomainPasswordInfo );

            if (!NT_SUCCESS(Status)) {
                *DomainPasswordInfo = NULL;
            } else {
                Status = STATUS_PASSWORD_RESTRICTION;
            }
        }

        goto Cleanup;
    }


Cleanup:

    //
    // If the only problem is that this is a BDC,
    //  Return the domain name back to the caller.
    //

    if ( (Status == STATUS_BACKUP_CONTROLLER ||
         Status == STATUS_INVALID_DOMAIN_ROLE) &&
         PrimaryDomainInfo != NULL ) {

        NTSTATUS TempStatus;

        //
        // Open the LSA if we haven't already.
        //

        if (LSAPolicyHandle == NULL) {

            InitializeObjectAttributes( &LSAObjectAttributes,
                                        NULL,             // Name
                                        0,                // Attributes
                                        NULL,             // Root
                                        NULL );           // Security Descriptor

            TempStatus = LsaOpenPolicy( UncComputerName,
                                        &LSAObjectAttributes,
                                        POLICY_VIEW_LOCAL_INFORMATION,
                                        &LSAPolicyHandle );

            if( !NT_SUCCESS(TempStatus) ) {
                KdPrint(("MspChangePasswordSam: LsaOpenPolicy(%wZ) failed, status %x\n",
                     UncComputerName, TempStatus));
                LSAPolicyHandle = NULL;
            }


        }

        if (LSAPolicyHandle != NULL) {
            TempStatus = LsaQueryInformationPolicy(
                            LSAPolicyHandle,
                            PolicyPrimaryDomainInformation,
                            (PVOID *) PrimaryDomainInfo );

            if( !NT_SUCCESS(TempStatus) ) {
                KdPrint(("MspChangePasswordSam: LsaQueryInformationPolicy(%wZ) failed, status %x\n",
                         UncComputerName, TempStatus));
                *PrimaryDomainInfo = NULL;
    #ifdef COMPILED_BY_DEVELOPER
            } else {
                KdPrint(("MspChangePasswordSam: %wZ is really a BDC in domain %wZ\n",
                         UncComputerName, &(*PrimaryDomainInfo)->Name));
    #endif // COMPILED_BY_DEVELOPER
            }
        }

        Status = STATUS_BACKUP_CONTROLLER;

    }

    //
    // Check for non-authoritative failures
    //

    if (( Status != STATUS_ACCESS_DENIED) &&
        ( Status != STATUS_WRONG_PASSWORD ) &&
        ( Status != STATUS_NO_SUCH_USER ) &&
        ( Status != STATUS_PASSWORD_RESTRICTION ) &&
        ( Status != STATUS_ACCOUNT_RESTRICTION ) &&
        ( Status != STATUS_INVALID_DOMAIN_ROLE) ) {
        *Authoritative = FALSE;
    }

    //
    // Stop impersonating.
    //

    {
        NTSTATUS TempStatus;

        TempStatus = MspStopImpersonating();

    }

    //
    // Free Locally used resources
    //


    if (SamHandle) {
        SamCloseHandle(SamHandle);
    }

    if (DomainHandle) {
        SamCloseHandle(DomainHandle);
    }

    if( LSAPolicyHandle != NULL ) {
        LsaClose( LSAPolicyHandle );
    }

    if ( AccountDomainInfo != NULL ) {
        (VOID) LsaFreeMemory( AccountDomainInfo );
    }

    return Status;
}


NTSTATUS
MspChangePasswordDownlevel(
    IN PUNICODE_STRING UncComputerName,
    IN PUNICODE_STRING UserNameU,
    IN PUNICODE_STRING OldPasswordU,
    IN PUNICODE_STRING NewPasswordU,
    OUT PBOOLEAN Authoritative
    )

/*++

Routine Description:

    This routine is called by MspChangePassword to change the password
    on an OS/2 User-level server.  First we try sending an encrypted
    request to the server, failing that we fall back on plaintext.

Arguments:

    UncComputerName - Pointer to Unicode String containing the Name of the
        target machine.  This name must begin with two backslashes and
        must be null terminated.

    UserNameU    - Name of the user to change password for.

    OldPasswordU - Plaintext current password.

    NewPasswordU - Plaintext replacement password.

    Authoritative - If the attempt failed with an error that would
        otherwise cause the password attempt to fail, this flag, if false,
        indicates that the error was not authoritative and the attempt
        should proceed.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

--*/

{
    NTSTATUS         Status;
    NET_API_STATUS   NetStatus;
    DWORD            Length;
    LPWSTR           UserName = NULL;
    LPWSTR           OldPassword = NULL;
    LPWSTR           NewPassword = NULL;

    *Authoritative = TRUE;

    //
    // Convert UserName from UNICODE_STRING to null-terminated wide string
    // for use by RxNetUserPasswordSet.
    //

    Length = UserNameU->Length;

    UserName = RtlAllocateHeap(
                   MspHeap, 0,
                   Length + sizeof(TCHAR)
                   );

    if ( NULL == UserName ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( UserName, UserNameU->Buffer, Length );

    UserName[ Length / sizeof(TCHAR) ] = 0;

    //
    // Convert OldPassword from UNICODE_STRING to null-terminated wide string.
    //

    Length = OldPasswordU->Length;

    OldPassword = RtlAllocateHeap( MspHeap, 0, Length + sizeof(TCHAR) );

    if ( NULL == OldPassword ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( OldPassword, OldPasswordU->Buffer, Length );

    OldPassword[ Length / sizeof(TCHAR) ] = 0;

    //
    // Convert NewPassword from UNICODE_STRING to null-terminated wide string.
    //

    Length = NewPasswordU->Length;

    NewPassword = RtlAllocateHeap( MspHeap, 0, Length + sizeof(TCHAR) );

    if ( NULL == NewPassword ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( NewPassword, NewPasswordU->Buffer, Length );

    NewPassword[ Length / sizeof(TCHAR) ] = 0;

#ifdef COMPILED_BY_DEVELOPER

    KdPrint(("MSV1_0: Changing password on downlevel server:\n"
        "\tUncComputerName: %wZ\n"
        "\tUserName:     %ws\n"
        "\tOldPassword:  %ws\n"
        "\tNewPassword:  %ws\n",
        UncComputerName,
        UserName,
        OldPassword,
        NewPassword
        ));

#endif // COMPILED_BY_DEVELOPER

    //
    // Attempt to change password on downlevel server.
    //

    NetStatus = RxNetUserPasswordSet(
                    UncComputerName->Buffer,
                    UserName,
                    OldPassword,
                    NewPassword);

    MsvPaswdLogPrint(("RxNetUserPasswordSet on machine %ws for user %ws returned 0x%x\n",
        UncComputerName->Buffer,
        UserName,
        NetStatus
        ));

#ifdef COMPILED_BY_DEVELOPER
    KdPrint(("MSV1_0: RxNUserPasswordSet returns %d.\n", NetStatus));
#endif // COMPILED_BY_DEVELOPER

    // Since we overload the computername as the domain name,
    // map NERR_InvalidComputer to STATUS_NO_SUCH_DOMAIN, since
    // that will give the user a nice error message.
    //
    // ERROR_PATH_NOT_FOUND is returned on a standalone workstation that
    //  doesn't have the network installed.
    //

    if (NetStatus == NERR_InvalidComputer ||
        NetStatus == ERROR_PATH_NOT_FOUND) {

        Status = STATUS_NO_SUCH_DOMAIN;

    // ERROR_SEM_TIMEOUT can be returned when the computer name doesn't
    //  exist.
    //
    // ERROR_REM_NOT_LIST can also be returned when the computer name
    //  doesn't exist.
    //

    } else if ( NetStatus == ERROR_SEM_TIMEOUT ||
                NetStatus == ERROR_REM_NOT_LIST) {

        Status = STATUS_BAD_NETWORK_PATH;

    } else if ( (NetStatus == ERROR_INVALID_PARAMETER) &&
                ((wcslen(NewPassword) > LM20_PWLEN) ||
                 (wcslen(OldPassword) > LM20_PWLEN)) ) {

        //
        // The net api returns ERROR_INVALID_PARAMETER if the password
        // could not be converted to the LM OWF password.  Return
        // STATUS_PASSWORD_RESTRICTION for this.
        //

        Status = STATUS_PASSWORD_RESTRICTION;

        //
        // We never made it to the other machine, so we should continue
        // trying to change the password.
        //

        *Authoritative = FALSE;
    } else {
        Status = NetpApiStatusToNtStatus( NetStatus );
    }

Cleanup:

    //
    // Free UserName if used.
    //

    if (UserName) {

        RtlFreeHeap(MspHeap, 0, UserName);
    }

    //
    // Free OldPassword if used. (Don't let password make it to page file)
    //

    if (OldPassword) {
        RtlZeroMemory( OldPassword, wcslen(OldPassword)*sizeof(WCHAR) );
        RtlFreeHeap(MspHeap, 0, OldPassword);
    }

    //
    // Free NewPassword if used. (Don't let password make it to page file)
    //

    if (NewPassword) {
        RtlZeroMemory( NewPassword, wcslen(NewPassword)*sizeof(WCHAR) );
        RtlFreeHeap(MspHeap, 0, NewPassword);
    }

    return Status;
}



NTSTATUS
MspChangePassword(
    IN OUT PUNICODE_STRING ComputerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN BOOLEAN Impersonating,
    OUT PDOMAIN_PASSWORD_INFORMATION *DomainPasswordInfo,
    OUT PPOLICY_PRIMARY_DOMAIN_INFO *PrimaryDomainInfo OPTIONAL,
    OUT PBOOLEAN Authoritative
    )

/*++

Routine Description:

    This routine is called by MspLM20ChangePassword to change the password
    on the specified server.  The server may be either NT or Downlevel.

Arguments:

    ComputerName - Name of the target machine.  This name may or may not
        begin with two backslashes.

    UserName - Name of the user to change password for.

    OldPassword - Plaintext current password.

    NewPassword - Plaintext replacement password.

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    DomainPasswordInfo - Password restriction information (returned only if
        status is STATUS_PASSWORD_RESTRICTION).

    PrimaryDomainInfo - DomainNameInformation (returned only if status is
        STATUS_BACKUP_CONTROLLER).

    Authoritative - Indicates that the error code is authoritative
        and it indicates that password changing should stop. If false,
        password changing should continue.


Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_PASSWORD_RESTRICTION - Password changing is restricted.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING UncComputerName;

    *Authoritative = TRUE;

    //
    // Ensure the server name is a UNC server name.
    //

    Status = MspAddBackslashesComputerName( ComputerName, &UncComputerName );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("MspChangePassword: MspAddBackslashes..(%wZ) failed, status %x\n",
                 ComputerName, Status));
        return(Status);
    }


    //
    // Assume the Server is an NT server and try to change the password.
    //

    Status = MspChangePasswordSam(
                 &UncComputerName,
                 UserName,
                 OldPassword,
                 NewPassword,
                 ClientRequest,
                 Impersonating,
                 DomainPasswordInfo,
                 PrimaryDomainInfo,
                 Authoritative );

    //
    // If MspChangePasswordSam returns anything other than
    // STATUS_CANT_ACCESS_DOMAIN_INFO, it was able to connect
    // to the remote computer so we won't try downlevel.
    //

    if (Status == STATUS_CANT_ACCESS_DOMAIN_INFO) {
        NET_API_STATUS NetStatus;
        DWORD OptionsSupported;

        //
        // only if target machine doesn't support SAM protocol do we attempt
        // downlevel.
        // MspAddBackslashesComputerName() NULL terminates the buffer.
        //

        NetStatus = NetRemoteComputerSupports(
                     (LPWSTR)UncComputerName.Buffer,
                     SUPPORTS_RPC | SUPPORTS_LOCAL | SUPPORTS_SAM_PROTOCOL,
                     &OptionsSupported
                     );

        if( NetStatus == NERR_Success && !(OptionsSupported & SUPPORTS_SAM_PROTOCOL) ) {

            Status = MspChangePasswordDownlevel(
                        &UncComputerName,
                        UserName,
                        OldPassword,
                        NewPassword,
                        Authoritative );

        }
    }


    //
    // Free UncComputerName.Buffer if different from ComputerName.
    //

    if ( UncComputerName.Buffer != ComputerName->Buffer ) {
        RtlFreeHeap(MspHeap, 0, UncComputerName.Buffer);
    }

    return(Status);
}



NTSTATUS
MspLm20ChangePassword (
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
    with a message type of MsV1_0ChangePassword.  This routine changes an
    account's password by either calling SamXxxPassword (for NT domains) or
    RxNetUserPasswordSet (for downlevel domains and standalone servers).

Arguments:

    The arguments to this routine are identical to those of LsaApCallPackage.
    Only the special attributes of these parameters as they apply to
    this routine are mentioned here.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_PASSWORD_RESTRICTION - The password change failed because the
        - password doesn't meet one or more domain restrictions.  The
        - response buffer is allocated.  If the PasswordInfoValid flag is
        - set it contains valid information otherwise it contains no
        - information because this was a down-level change.

    STATUS_BACKUP_CONTROLLER - The named machine is a Backup Domain Controller.
        Changing password is only allowed on the Primary Domain Controller.

--*/

{
    PMSV1_0_CHANGEPASSWORD_REQUEST ChangePasswordRequest = NULL;
    PMSV1_0_CHANGEPASSWORD_RESPONSE ChangePasswordResponse;
    NTSTATUS        Status = STATUS_SUCCESS;
    NTSTATUS        SavedStatus = STATUS_SUCCESS;
    LPWSTR          DomainName = NULL;
    PDOMAIN_CONTROLLER_INFO DCInfo = NULL;
    UNICODE_STRING  DCNameString;
    UNICODE_STRING  ClientNetbiosDomain = {0};
    PUNICODE_STRING  ClientDsGetDcDomain;
    UNICODE_STRING  ClientDnsDomain = {0};
    UNICODE_STRING  ClientName = {0};
    UNICODE_STRING  ValidatedAccountName;
    UNICODE_STRING  ValidatedDomainName;
    LPWSTR          ValidatedOldPasswordBuffer;
    LPWSTR          ValidatedNewPasswordBuffer;
    NET_API_STATUS  NetStatus;
    PPOLICY_LSA_SERVER_ROLE_INFO PolicyLsaServerRoleInfo = NULL;
    PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;
    PWKSTA_INFO_100 WkstaInfo100 = NULL;
    BOOLEAN PasswordBufferValidated = FALSE;
    CLIENT_BUFFER_DESC ClientBufferDesc;
    PSECURITY_SEED_AND_LENGTH SeedAndLength;
    PDS_NAME_RESULTW NameResult = NULL;
    HANDLE DsHandle = NULL;
    UCHAR           Seed;
    BOOLEAN         Authoritative = TRUE;

    BOOLEAN         AttemptRediscovery = FALSE;

    RtlInitUnicodeString(
        &DCNameString,
        NULL
        );
    //
    // Sanity checks.
    //

    NlpInitClientBuffer( &ClientBufferDesc, ClientRequest );

    if ( SubmitBufferSize < sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    ChangePasswordRequest = (PMSV1_0_CHANGEPASSWORD_REQUEST) ProtocolSubmitBuffer;

    ASSERT( ChangePasswordRequest->MessageType == MsV1_0ChangePassword ||
            ChangePasswordRequest->MessageType == MsV1_0ChangeCachedPassword );

    RELOCATE_ONE( &ChangePasswordRequest->DomainName );

    RELOCATE_ONE( &ChangePasswordRequest->AccountName );

    if ( ChangePasswordRequest->MessageType == MsV1_0ChangeCachedPassword ) {
        NULL_RELOCATE_ONE( &ChangePasswordRequest->OldPassword );
    } else {
        RELOCATE_ONE_ENCODED( &ChangePasswordRequest->OldPassword );
    }

    RELOCATE_ONE_ENCODED( &ChangePasswordRequest->NewPassword );

    //
    // save away copies of validated buffers to check later.
    //

    RtlCopyMemory( &ValidatedDomainName, &ChangePasswordRequest->DomainName, sizeof(ValidatedDomainName) );
    RtlCopyMemory( &ValidatedAccountName, &ChangePasswordRequest->AccountName, sizeof(ValidatedAccountName) );

    ValidatedOldPasswordBuffer = ChangePasswordRequest->OldPassword.Buffer;
    ValidatedNewPasswordBuffer = ChangePasswordRequest->NewPassword.Buffer;


    SeedAndLength = (PSECURITY_SEED_AND_LENGTH) &ChangePasswordRequest->OldPassword.Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    if (Seed != 0) {

        try {
            RtlRunDecodeUnicodeString(
                Seed,
                &ChangePasswordRequest->OldPassword
                );

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_ILL_FORMED_PASSWORD;
            goto Cleanup;
        }
    }

    SeedAndLength = (PSECURITY_SEED_AND_LENGTH) &ChangePasswordRequest->NewPassword.Length;
    Seed = SeedAndLength->Seed;
    SeedAndLength->Seed = 0;

    if (Seed != 0) {

        if( ChangePasswordRequest->NewPassword.Buffer !=
            ValidatedNewPasswordBuffer ) {

            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        try {
            RtlRunDecodeUnicodeString(
                Seed,
                &ChangePasswordRequest->NewPassword
                );

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_ILL_FORMED_PASSWORD;
            goto Cleanup;
        }
    }

    //
    // sanity check that we didn't whack over buffers.
    //

    if( !RtlCompareMemory(
                        &ValidatedDomainName,
                        &ChangePasswordRequest->DomainName,
                        sizeof(ValidatedDomainName)
                        )
                        ||
        !RtlCompareMemory(
                        &ValidatedAccountName,
                        &ChangePasswordRequest->AccountName,
                        sizeof(ValidatedAccountName)
                        )
                        ||
        (ValidatedOldPasswordBuffer != ChangePasswordRequest->OldPassword.Buffer)
                        ||
        (ValidatedNewPasswordBuffer != ChangePasswordRequest->NewPassword.Buffer)
                        ) {

            Status= STATUS_INVALID_PARAMETER;
            goto Cleanup;
    }


    *ReturnBufferSize = 0;
    *ProtocolReturnBuffer = NULL;
    *ProtocolStatus = STATUS_PENDING;
    PasswordBufferValidated = TRUE;

    MsvPaswdLogPrint(("Attempting password change server/domain %wZ for user %wZ\n",
        &ChangePasswordRequest->DomainName,
        &ChangePasswordRequest->AccountName
        ));

#ifdef COMPILED_BY_DEVELOPER

    KdPrint(("MSV1_0:\n"
             "\tDomain:\t%wZ\n"
             "\tAccount:\t%wZ\n"
             "\tOldPassword(%d)\n"
             "\tNewPassword(%d)\n",
             &ChangePasswordRequest->DomainName,
             &ChangePasswordRequest->AccountName,
             (int) ChangePasswordRequest->OldPassword.Length,
             (int) ChangePasswordRequest->NewPassword.Length
             ));

#endif // COMPILED_BY_DEVELOPER


    //
    // If we're just changing the cached password,
    //  skip changing the password on the domain.
    //

    if ( ChangePasswordRequest->MessageType == MsV1_0ChangeCachedPassword ) {
        ClientName = ChangePasswordRequest->AccountName;
        ClientNetbiosDomain = ChangePasswordRequest->DomainName;
        Status = STATUS_SUCCESS;
        goto PasswordChangeSuccessfull;
    }


    //
    // If the client supplied a non-nt4 name, go ahead and convert it here.
    //

    if (ChangePasswordRequest->DomainName.Length == 0) {
        DWORD DsStatus;
        WCHAR NameBuffer[UNLEN+1];
        LPWSTR NameString = NameBuffer;
        ULONG Index;
        BOOLEAN fWorkstation = FALSE;

        //
        // BUGBUG: is this too limited?
        //

        if (ChangePasswordRequest->AccountName.Length / sizeof(WCHAR) > UNLEN) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        RtlCopyMemory(
            NameBuffer,
            ChangePasswordRequest->AccountName.Buffer,
            ChangePasswordRequest->AccountName.Length
            );
        NameBuffer[ChangePasswordRequest->AccountName.Length/sizeof(WCHAR)] = L'\0';


        DsStatus = DsBindW(
                        NULL,
                        NULL,
                        &DsHandle
                        );

        //
        // we allow the bind to fail on a member workstation, which allows
        // us to attempt a 'manual' crack.
        //

        if (DsStatus != ERROR_SUCCESS) {

            fWorkstation = NlpWorkstation;

            if( !fWorkstation ) {
                SspPrint(( SSP_CRITICAL, "MspLm20ChangePassword: DsBindW returned 0x%lx.\n",
                        DsStatus ));
                Status = NetpApiStatusToNtStatus( DsStatus );
                goto Cleanup;
            }
        }

        if( !fWorkstation ) {
            DsStatus = DsCrackNamesW(
                        DsHandle,
                        0,                          // no flags
                        DS_UNKNOWN_NAME,
                        DS_NT4_ACCOUNT_NAME,
                        1,
                        &NameString,
                        &NameResult
                        );

            if (DsStatus != ERROR_SUCCESS) {
                SspPrint(( SSP_CRITICAL, "MspLm20ChangePassword: DsCrackNamesW returned 0x%lx.\n",
                            DsStatus ));
                Status = NetpApiStatusToNtStatus( DsStatus );
                goto Cleanup;
            }

            //
            // Look for the name in the result
            //

            if (NameResult->cItems < 1) {
                SspPrint(( SSP_CRITICAL, "MspLm20ChangePassword: DsCrackNamesW returned zero items.\n"));
                Status = STATUS_INTERNAL_ERROR;
                goto Cleanup;
            }

        }

        //
        // Check if the crack succeeded
        //

        if ( (fWorkstation) ||
             (NameResult->rItems[0].status != DS_NAME_NO_ERROR)
             ) {

            //
            // The name wasn't mapped. Try converting it manually by
            // splitting it at the '@'
            //

            RtlInitUnicodeString(
                &ClientName,
                NameBuffer
                );

            // shortest possible is 3 Unicode chars (eg: a@a)
            if (ClientName.Length < (sizeof(WCHAR) * 3)) {
                Status = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }

            for (Index = (ClientName.Length / sizeof(WCHAR)) - 1; Index != 0 ; Index-- ) {
                if (ClientName.Buffer[Index] == L'@') {

                    RtlInitUnicodeString(
                        &ClientDnsDomain,
                        &ClientName.Buffer[Index+1]
                        );

                    ClientName.Buffer[Index] = L'\0';
                    ClientName.Length = (USHORT) Index * sizeof(WCHAR);


                    break;
                }
            }

            //
            // If the name couldn't be parsed, give up and go home
            //

            if (ClientDnsDomain.Length == 0) {
                Status = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }

            //
            // This isn't really the Netbios Domain name, but it is the best we have.
            //

            ClientNetbiosDomain = ClientDnsDomain;

            for (Index = 0; Index < (ClientNetbiosDomain.Length / sizeof(WCHAR)) ; Index++ ) {

                //
                // truncate the netbios domain to the first DOT
                //

                if( ClientNetbiosDomain.Buffer[Index] == L'.' ) {
                    ClientNetbiosDomain.Length = (USHORT)(Index * sizeof(WCHAR));
                    ClientNetbiosDomain.MaximumLength = ClientNetbiosDomain.Length;
                    break;
                }
            }

        }
        else
        {

            RtlInitUnicodeString(
                &ClientDnsDomain,
                NameResult->rItems[0].pDomain
                );
            RtlInitUnicodeString(
                &ClientName,
                NameResult->rItems[0].pName
                );
            RtlInitUnicodeString(
                &ClientNetbiosDomain,
                NameResult->rItems[0].pName
                );
            //
            // Move the pointer for the name up to the first "\" in the name
            //

            for (Index = 0; Index < ClientName.Length / sizeof(WCHAR) ; Index++ ) {
                if (ClientName.Buffer[Index] == L'\\') {
                    RtlInitUnicodeString(
                        &ClientName,
                        &ClientName.Buffer[Index+1]
                        );

                    // Set the Netbios Domain Name to the string to the left of the backslash
                    ClientNetbiosDomain.Length = (USHORT)(Index * sizeof(WCHAR));
                    break;
                }
            }
        }

    } else {

        ClientName = ChangePasswordRequest->AccountName;
        ClientNetbiosDomain = ChangePasswordRequest->DomainName;
    }



    // Make sure that NlpSamInitialized is TRUE. If we logon using
    // Kerberos, this may not be true.

    if ( !NlpSamInitialized)
    {
        Status = NlSamInitialize( SAM_STARTUP_TIME );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }


    //
    // Check to see if the name provided is a domain name. If it has no
    // leading "\\" and does not match the name of the computer, it may be.
    //

    if ((( ClientNetbiosDomain.Length < 3 * sizeof(WCHAR)) ||
        ( ClientNetbiosDomain.Buffer[0] != L'\\' &&
          ClientNetbiosDomain.Buffer[1] != L'\\' ) ) &&
          !RtlEqualDomainName(
                   &NlpComputerName,
                   &ClientNetbiosDomain )) {

        //
        // Check if we are a DC in this domain.
        //  If so, use this DC.
        //  BUGBUG: should allow DnsDomain Name compare, too.
        //

        if ( !NlpWorkstation &&
                   RtlEqualDomainName(
                       &NlpSamDomainName,
                       &ClientNetbiosDomain )) {

            DCNameString = NlpComputerName;
        }

        if (DCNameString.Buffer == NULL) {

            // BUGBUG: Should really pass both names to internal version of DsGetDcName
            if ( ClientDnsDomain.Length != 0 ) {
                ClientDsGetDcDomain = &ClientDnsDomain;
            } else {
                ClientDsGetDcDomain = &ClientNetbiosDomain;
            }

            //
            // Build a zero terminated domain name.
            //

            DomainName = RtlAllocateHeap(
                            MspHeap,
                            0,
                            ClientDsGetDcDomain->Length + sizeof(WCHAR));

            if ( DomainName == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory( DomainName,
                           ClientDsGetDcDomain->Buffer,
                           ClientDsGetDcDomain->Length );
            DomainName[ClientDsGetDcDomain->Length / sizeof(WCHAR)] = 0;

            NetStatus = DsGetDcNameW(
                                NULL,
                                DomainName,
                                NULL,           // no domain guid
                                NULL,           // no site name
                                DS_WRITABLE_REQUIRED,
                                &DCInfo );

            if ( NetStatus != NERR_Success ) {
                SspPrint(( SSP_CRITICAL, "MspLm20ChangePassword: DsGetDcNameW returned 0x%lx.\n",
                    NetStatus));

                Status = NetpApiStatusToNtStatus( NetStatus );
                if( Status == STATUS_INTERNAL_ERROR )
                    Status = STATUS_NO_SUCH_DOMAIN;
            } else {
                RtlInitUnicodeString( &DCNameString, DCInfo->DomainControllerName );
            }

            AttemptRediscovery = TRUE;
        }

        if (NT_SUCCESS(Status)) {

            Status = MspChangePassword(
                         &DCNameString,
                         &ClientName,
                         &ChangePasswordRequest->OldPassword,
                         &ChangePasswordRequest->NewPassword,
                         ClientRequest,
                         ChangePasswordRequest->Impersonating,
                         &DomainPasswordInfo,
                         NULL,
                         &Authoritative );

            //
            // If we succeeded or got back an authoritative answer
            if ( NT_SUCCESS(Status) || Authoritative) {
                goto PasswordChangeSuccessfull;
            }

        }

    }

    //
    // Free the DC info so we can call DsGetDcName again.
    //

    if ( DCInfo != NULL ) {
        NetApiBufferFree(DCInfo);
        DCInfo = NULL;
    }


    //
    // attempt re-discovery.
    //

    if( AttemptRediscovery ) {

        NetStatus = DsGetDcNameW(
                            NULL,
                            DomainName,
                            NULL,           // no domain guid
                            NULL,           // no site name
                            DS_FORCE_REDISCOVERY | DS_WRITABLE_REQUIRED,
                            &DCInfo );

        if ( NetStatus != NERR_Success ) {

            SspPrint(( SSP_CRITICAL, "MspLm20ChangePassword: DsGetDcNameW (re-discover) returned 0x%lx.\n",
                NetStatus));

            DCInfo = NULL;
            Status = NetpApiStatusToNtStatus( NetStatus );
            if( Status == STATUS_INTERNAL_ERROR )
                Status = STATUS_NO_SUCH_DOMAIN;
        } else {
            RtlInitUnicodeString( &DCNameString, DCInfo->DomainControllerName );

            Status = MspChangePassword(
                         &DCNameString,
                         &ClientName,
                         &ChangePasswordRequest->OldPassword,
                         &ChangePasswordRequest->NewPassword,
                         ClientRequest,
                         ChangePasswordRequest->Impersonating,
                         &DomainPasswordInfo,
                         NULL,
                         &Authoritative );

            //
            // If we succeeded or got back an authoritative answer
            if ( NT_SUCCESS(Status) || Authoritative) {
                goto PasswordChangeSuccessfull;
            }

            //
            // Free the DC info so we can call DsGetDcName again.
            //

            if ( DCInfo != NULL ) {
                NetApiBufferFree(DCInfo);
                DCInfo = NULL;
            }
        }
    }


    if (Status != STATUS_BACKUP_CONTROLLER) {
        //
        // Change the password assuming the DomainName is really a server name
        //
        // The domain name is overloaded to be either a domain name or a server
        // name.  The server name is useful when changing the password on a LM2.x
        // standalone server, which is a "member" of a domain but uses a private
        // account database.
        //

        Status = MspChangePassword(
                     &ClientNetbiosDomain,
                     &ClientName,
                     &ChangePasswordRequest->OldPassword,
                     &ChangePasswordRequest->NewPassword,
                     ClientRequest,
                     ChangePasswordRequest->Impersonating,
                     &DomainPasswordInfo,
                     &PrimaryDomainInfo,
                     &Authoritative );



        //
        // If DomainName is actually a server name,
        //  just return the status to the caller.
        //

        if ( Authoritative &&
             ( Status != STATUS_BAD_NETWORK_PATH ||
               ( ClientNetbiosDomain.Length >= 3 * sizeof(WCHAR) &&
                 ClientNetbiosDomain.Buffer[0] == L'\\' &&
                 ClientNetbiosDomain.Buffer[1] == L'\\' ) ) ) {

            //
            // If \\xxx was specified, but xxx doesn't exist,
            //  return the status code that the DomainName field is bad.
            //

            if ( Status == STATUS_BAD_NETWORK_PATH ) {
                Status = STATUS_NO_SUCH_DOMAIN;
            }
        }


        //
        // If we didn't get an error that this was a backup controller,
        // we are out of here.
        //

        if (Status != STATUS_BACKUP_CONTROLLER) {
            goto PasswordChangeSuccessfull;
        }


    }

    //
    // If the specified machine was a BDC in a domain,
    //    Pretend the caller passed us the domain name in the first place.
    //

    if ( Status == STATUS_BACKUP_CONTROLLER && PrimaryDomainInfo != NULL ) {

        ClientNetbiosDomain = PrimaryDomainInfo->Name;
        Status = STATUS_BAD_NETWORK_PATH;
    } else {
        goto PasswordChangeSuccessfull;
    }




    //
    // Build a zero terminated domain name.
    //

    // BUGBUG: Should really pass both names to internal version of DsGetDcName
    if ( ClientDnsDomain.Length != 0 ) {
        ClientDsGetDcDomain = &ClientDnsDomain;
    } else {
        ClientDsGetDcDomain = &ClientNetbiosDomain;
    }

    if( DomainName )
        RtlFreeHeap( MspHeap, 0, DomainName );

    DomainName = RtlAllocateHeap(
                    MspHeap,
                    0,
                    ClientDsGetDcDomain->Length + sizeof(WCHAR));

    if ( DomainName == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory( DomainName,
                   ClientDsGetDcDomain->Buffer,
                   ClientDsGetDcDomain->Length );
    DomainName[ClientDsGetDcDomain->Length / sizeof(WCHAR)] = 0;

    AttemptRediscovery = FALSE;

retry:
    {
        DWORD dwGetDcFlags = 0;

        if( AttemptRediscovery )
            dwGetDcFlags |= DS_FORCE_REDISCOVERY;

        //
        // Determine the PDC of the named domain so we can change the password there.
        //

        NetStatus = DsGetDcNameW(
                            NULL,
                            DomainName,
                            NULL,           // no domain guid
                            NULL,           // no site name
                            dwGetDcFlags | DS_WRITABLE_REQUIRED,
                            &DCInfo );

        if ( NetStatus != NERR_Success ) {

            SspPrint(( SSP_CRITICAL, "MspLm20ChangePassword: DsGetDcNameW returned 0x%lx.\n",
                NetStatus));

            Status = NetpApiStatusToNtStatus( NetStatus );
            if( Status == STATUS_INTERNAL_ERROR )
                Status = STATUS_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        RtlInitUnicodeString( &DCNameString, DCInfo->DomainControllerName );

        Status = MspChangePassword(
                     &DCNameString,
                     &ClientName,
                     &ChangePasswordRequest->OldPassword,
                     &ChangePasswordRequest->NewPassword,
                     ClientRequest,
                     ChangePasswordRequest->Impersonating,
                     &DomainPasswordInfo,
                     NULL,
                     &Authoritative );

        if( !NT_SUCCESS(Status) && !Authoritative && !AttemptRediscovery ) {
            AttemptRediscovery = TRUE;
            goto retry;
        }
    }


PasswordChangeSuccessfull:

    //
    // Allocate and initialize the response buffer.
    //

    SavedStatus = Status;

    *ReturnBufferSize = sizeof(MSV1_0_CHANGEPASSWORD_RESPONSE);

    Status = NlpAllocateClientBuffer( &ClientBufferDesc,
                                      sizeof(MSV1_0_CHANGEPASSWORD_RESPONSE),
                                      *ReturnBufferSize );


    if ( !NT_SUCCESS( Status ) ) {
        KdPrint(("MSV1_0: MspLm20ChangePassword: cannot alloc client buffer\n"));
        *ReturnBufferSize = 0;
        goto Cleanup;
    }

    ChangePasswordResponse = (PMSV1_0_CHANGEPASSWORD_RESPONSE) ClientBufferDesc.MsvBuffer;

    ChangePasswordResponse->MessageType = MsV1_0ChangePassword;


    //
    // Copy the DomainPassword restrictions out to the caller depending on
    //  whether it was passed to us.
    //
    // Mark the buffer as valid or invalid to let the caller know.
    //
    // if STATUS_PASSWORD_RESTRICTION is returned.  This status can be
    // returned by either SAM or a down-level change.  Only SAM will return
    // valid data so we have a flag in the buffer that says whether the data
    // is valid or not.
    //

    if ( DomainPasswordInfo == NULL ) {
        ChangePasswordResponse->PasswordInfoValid = FALSE;
    } else {
        ChangePasswordResponse->DomainPasswordInfo = *DomainPasswordInfo;
        ChangePasswordResponse->PasswordInfoValid = TRUE;
    }


    //
    // Flush the buffer to the client's address space.
    //

    Status = NlpFlushClientBuffer( &ClientBufferDesc,
                                   ProtocolReturnBuffer );



    //
    // Update cached credentials with the new password.
    //
    // This is done by calling NlpChangePassword,
    // which takes encrypted passwords, so encrypt 'em.
    //

    if ( NT_SUCCESS(SavedStatus) ) {

        //
        // Failure of NlpChangePassword is OK, that means that the
        // account we've been working with isn't the one we're
        // caching credentials for.
        //

        (VOID) NlpChangePassword(
                     &ClientNetbiosDomain,
                     &ClientName,
                     &ChangePasswordRequest->NewPassword
                     );

    }


    Status = SavedStatus;


Cleanup:


    //
    // Free Locally allocated resources
    //

    if (DomainName != NULL) {
        RtlFreeHeap(MspHeap, 0, DomainName);
    }

    if ( DCInfo != NULL ) {
        NetApiBufferFree(DCInfo);
    }

    if ( WkstaInfo100 != NULL ) {
        NetApiBufferFree(WkstaInfo100);
    }

    if ( DomainPasswordInfo != NULL ) {
        SamFreeMemory(DomainPasswordInfo);
    }

    if ( PrimaryDomainInfo != NULL ) {
        (VOID) LsaFreeMemory( PrimaryDomainInfo );
    }


    if ( DsHandle != NULL) {
        DsUnBindW(
            &DsHandle
            );
    }

    //
    // Free Policy Server Role Information if used.
    //

    if (PolicyLsaServerRoleInfo != NULL) {

        I_LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyLsaServerRoleInformation,
            (PLSAPR_POLICY_INFORMATION) PolicyLsaServerRoleInfo
            );
    }

    //
    // Free the return buffer.
    //

    NlpFreeClientBuffer( &ClientBufferDesc );

    //
    // Don't let the password stay in the page file.
    //

    if ( PasswordBufferValidated ) {
        RtlEraseUnicodeString( &ChangePasswordRequest->OldPassword );
        RtlEraseUnicodeString( &ChangePasswordRequest->NewPassword );
    }

    //
    // Flush the log to disk
    //

    MsvPaswdSetAndClearLog();

    //
    // Return status to the caller.
    //


    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}
