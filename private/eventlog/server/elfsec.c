
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    elfsec.c


Author:

    Dan Hinsley (danhi)     28-Mar-1992

Environment:

    Calls NT native APIs.

Revision History:

    27-Oct-1993     danl
        Make Eventlog service a DLL and attach it to services.exe.
        Removed functions that create well-known SIDs.  This information
        is now passed into the Elfmain as a Global data structure containing
        all well-known SIDs.
    28-Mar-1992     danhi
        created - based on scsec.c in svcctrl by ritaw
    03-Mar-1995     markbl
        Added guest & anonymous logon log access restriction feature.

--*/

#include <eventp.h>
#include <elfcfg.h>

#define PRIVILEGE_BUF_SIZE  512

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    );

NTSTATUS
ElfpReleasePrivilege(
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //
// Structure that describes the mapping of generic access rights to  //
// object specific access rights for a LogFile object.               //
//                                                                   //
//-------------------------------------------------------------------//

static GENERIC_MAPPING LogFileObjectMapping = {

    STANDARD_RIGHTS_READ           |       // Generic read
        ELF_LOGFILE_READ,

    STANDARD_RIGHTS_WRITE          |       // Generic write
        ELF_LOGFILE_WRITE,

    STANDARD_RIGHTS_EXECUTE        |       // Generic execute
        ELF_LOGFILE_START          |
        ELF_LOGFILE_STOP           |
        ELF_LOGFILE_CONFIGURE,

    ELF_LOGFILE_ALL_ACCESS                 // Generic all
    };


//-------------------------------------------------------------------//
//                                                                   //
// Functions                                                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
ElfpCreateLogFileObject(
    PLOGFILE LogFile,
    DWORD Type,
    ULONG GuestAccessRestriction
    )
/*++

Routine Description:

    This function creates the security descriptor which represents
    an active log file.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:


--*/
{
    NTSTATUS Status;
    DWORD NumberOfAcesToUse;

#define ELF_LOGFILE_OBJECT_ACES  10            // Number of ACEs in this DACL

    RTL_ACE_DATA AceData[ELF_LOGFILE_OBJECT_ACES] = {

        {ACCESS_DENIED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_ALL_ACCESS,               &AnonymousLogonSid},

        {ACCESS_DENIED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_ALL_ACCESS,               &(ElfGlobalData->AliasGuestsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_ALL_ACCESS,               &(ElfGlobalData->LocalSystemSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR, &(ElfGlobalData->AliasAdminsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_BACKUP,                   &(ElfGlobalData->AliasBackupOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR, &(ElfGlobalData->AliasSystemOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ,                     &(ElfGlobalData->WorldSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->AliasAdminsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->AliasSystemOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->WorldSid)}
        };

    PRTL_ACE_DATA pAceData = NULL;

    //
    // NON_SECURE logfiles let anyone read/write to them, secure ones
    // only let admins/local system do this.  so for secure files we just
    // don't use the last ACE
    //
    // Adjust the ACL start based on the passed GuestAccessRestriction flag.
    // The first two aces deny all log access to guests and/or anonymous
    // logons. The flag, GuestAccessRestriction, indicates that these two
    // deny access aces should be applied. Note that the deny aces and the
    // GuestAccessRestriction flag are not applicable to the security log,
    // since users and anonymous logons, by default, do not have access.
    //

    switch (Type) {

        case ELF_LOGFILE_SECURITY:
            pAceData = AceData + 2;         // Deny ACEs *not* applicable
            NumberOfAcesToUse = 3;
            break;

        case ELF_LOGFILE_SYSTEM:
            if (GuestAccessRestriction == ELF_GUEST_ACCESS_RESTRICTED) {
                pAceData = AceData;         // Deny ACEs *applicable*
                NumberOfAcesToUse = 8;
            }
            else {
                pAceData = AceData + 2;     // Deny ACEs *not* applicable
                NumberOfAcesToUse = 6;
            }
            break;

        case ELF_LOGFILE_APPLICATION:
            if (GuestAccessRestriction == ELF_GUEST_ACCESS_RESTRICTED) {
                pAceData = AceData;         // Deny ACEs *applicable*
                NumberOfAcesToUse = 10;
            }
            else {
                pAceData = AceData + 2;     // Deny ACEs *not* applicable
                NumberOfAcesToUse = 8;
            }
            break;

    }
    Status = RtlCreateUserSecurityObject(
                   pAceData,
                   NumberOfAcesToUse,
                   NULL,                        // Owner
                   NULL,                        // Group
                   TRUE,                        // IsDirectoryObject
                   &LogFileObjectMapping,
                   &LogFile->Sd);

    if (! NT_SUCCESS(Status)) {
        ElfDbgPrintNC((
            "[ELF] ElfpCreateLogFileObject: ElfCreateUserSecurityObject "
            "failed - %X\n", Status));
    }

    return (Status);
}


VOID
ElfpDeleteLogFileObject(
    PLOGFILE LogFile
    )
/*++

Routine Description:

    This function deletes the self-relative security descriptor which
    represents an eventlog logfile object.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:

    None.

--*/
{
    (void) RtlDeleteSecurityObject(&LogFile->Sd);
}


NTSTATUS
ElfpAccessCheckAndAudit(
    IN     LPWSTR SubsystemName,
    IN     LPWSTR ObjectTypeName,
    IN     LPWSTR ObjectName,
    IN OUT IELF_HANDLE ContextHandle,
    IN     PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN     ACCESS_MASK DesiredAccess,
    IN     PGENERIC_MAPPING GenericMapping,
    IN     BOOL ForSecurityLog
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheckAndAuditAlarm; and reverts back to
    itself before returning.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling this routine.

    ObjectTypeName - Supplies the name of the type of the object being
        accessed.

    ObjectName - Supplies the name of the object being accessed.

    ContextHandle - Supplies the context handle to the object.  On return, the
        granted access is written to the AccessGranted field of this structure
        if this call succeeds.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ForSecurityLog - TRUE if the access check is for the security log.
        This is a special case that may require a privilege check.

Return Value:

    NT status mapped to Win32 errors.

--*/
{
    NTSTATUS   Status;
    RPC_STATUS RpcStatus;

    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;

    BOOLEAN         GenerateOnClose = FALSE;
    NTSTATUS        AccessStatus;
    ACCESS_MASK     GrantedAccess = 0;
    HANDLE          ClientToken = NULL;
    PRIVILEGE_SET   PrivilegeSet;
    ULONG           PrivilegeSetLength = sizeof(PRIVILEGE_SET);
    ULONG           privileges[1];


    GenericMapping = &LogFileObjectMapping;

    RtlInitUnicodeString(&Subsystem, SubsystemName);
    RtlInitUnicodeString(&ObjectType, ObjectTypeName);
    RtlInitUnicodeString(&Object, ObjectName);

    if ((RpcStatus = RpcImpersonateClient(NULL)) != RPC_S_OK) {

        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Failed to impersonate "
            "client %08lx\n", RpcStatus));

        return RpcStatus;
    }

    //
    // Get a token handle for the client
    //
    Status = NtOpenThreadToken (NtCurrentThread(),
                                TOKEN_QUERY,        // DesiredAccess
                                TRUE,               // OpenAsSelf
                                &ClientToken);

    if (!NT_SUCCESS(Status)) {

        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: NtOpenThreadToken Failed: "
                         "0x%lx\n",Status));
        goto CleanExit;
    }

    //
    // We want to see if we can get the desired access, and if we do
    // then we also want all our other accesses granted.
    // MAXIMUM_ALLOWED gives us this.
    //
    DesiredAccess |= MAXIMUM_ALLOWED;

    //
    // Bug #57153 -- Make sure that the current user has the right to manage
    // the security log.  Without this check, the Eventlog will allow all
    // administrators to manage the log, even if they don't have the access
    //
    if (ForSecurityLog) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    Status = NtAccessCheck(SecurityDescriptor,
                           ClientToken,
                           DesiredAccess,
                           GenericMapping,
                           &PrivilegeSet,
                           &PrivilegeSetLength,
                           &GrantedAccess,
                           &AccessStatus);

    if (! NT_SUCCESS(Status)) {

        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Error calling "
                         "NtAccessCheck %08lx\n",
                     Status));

        goto CleanExit;
    }

    if (AccessStatus != STATUS_SUCCESS) {

        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Access status is %08lx\n",
                     AccessStatus));

        //
        // MarkBl 1/30/95 : Modified this code a bit to give backup operators
        //                  the ability to open the security log for purposes
        //                  of backup.
        //

        if ((AccessStatus == STATUS_ACCESS_DENIED) && (ForSecurityLog)) {

            //
            // MarkBl 1/30/95 :  First, evalutate the existing code (performed
            //                   for read or clear access), since its
            //                   privilege check is more rigorous than mine.
            //

            Status = STATUS_ACCESS_DENIED;

            if (!(DesiredAccess & ELF_LOGFILE_WRITE)) {

                //
                // If read or clear access to the security log is desired,
                // then we will see if this user passes the privilege check.
                //
                //
                // Do Privilege Check for SeSecurityPrivilege
                // (SE_SECURITY_NAME).
                //
                // MarkBl 1/30/95 : Modified code to fall through on error
                //                  instead of the jump to 'CleanExit'.
                //

                Status = ElfpTestClientPrivilege(SE_SECURITY_PRIVILEGE,
                                                 ClientToken);

                if (NT_SUCCESS(Status)) {

                    GrantedAccess |= (ELF_LOGFILE_READ |
                                      ELF_LOGFILE_CLEAR);
                }
                else {

                    ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: "
                                     "ElfpTestClientPrivilege failed %x\n",
                                 Status));
                }
            }

            //
            // MarkBl 1/30/95 : Finally, my code. If this user has backup
            //                  privilege, let the open succeed.
            //

            if (!NT_SUCCESS(Status)) {

                Status = ElfpTestClientPrivilege(SE_BACKUP_PRIVILEGE,
                                                 ClientToken);

                if (NT_SUCCESS(Status)) {

                    GrantedAccess |= ELF_LOGFILE_BACKUP;
                }
                else {
                    goto CleanExit;
                }
            }
        }
        else {
            Status = AccessStatus;
        }
    }


    //
    // Revert to Self
    //
    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK) {

        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Fail to revert to "
            "self %08lx\n", RpcStatus));
        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //

    }

    //
    // Get SeAuditPrivilege so I can call NtOpenObjectAuditAlarm.
    // If any of this stuff fails, I don't want the status to overwrite the
    // status that I got back from the access and privilege checks.
    //

    privileges[0] = SE_AUDIT_PRIVILEGE;
    AccessStatus  = ElfpGetPrivilege(1, privileges);

    if (!NT_SUCCESS(AccessStatus)) {

        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: ElfpGetPrivilege "
                         "(Enable) failed.  Status is 0x%x\n",
                     AccessStatus));
    }

    //
    // Call the Audit Alarm function.
    //

    AccessStatus = NtOpenObjectAuditAlarm (
                        &Subsystem,
                        (PVOID)ContextHandle,
                        &ObjectType,
                        &Object,
                        SecurityDescriptor,
                        ClientToken,            // Handle ClientToken
                        DesiredAccess,
                        GrantedAccess,
                        &PrivilegeSet,          // PPRIVLEGE_SET
                        FALSE,                  // BOOLEAN ObjectCreation,
                        TRUE,                   // BOOLEAN AccessGranted,
                        &GenerateOnClose);

    if (!NT_SUCCESS(AccessStatus)) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: NtOpenObjectAuditAlarm "
                         "failed. status is 0x%lx\n",
                     AccessStatus));
    }
    else {

        if (GenerateOnClose) {
            ContextHandle->Flags |= ELF_LOG_HANDLE_GENERATE_ON_CLOSE;
        }
    }

    //
    // Update the GrantedAccess in the context handle.
    //
    ContextHandle->GrantedAccess = GrantedAccess;

    NtClose(ClientToken);

    ElfpReleasePrivilege();

    return(Status);

CleanExit:

    //
    // Revert to Self
    //
    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK) {

        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Fail to revert to "
                         "self %08lx\n",
                     RpcStatus));
        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //
    }

    if (ClientToken != NULL) {
        NtClose(ClientToken);
    }

    return(Status);
}


VOID
ElfpCloseAudit(
    IN  LPWSTR      SubsystemName,
    IN  IELF_HANDLE ContextHandle
    )

/*++

Routine Description:

    If the GenerateOnClose flag in the ContextHandle is set, then this function
    calls NtCloseAuditAlarm in order to generate a close audit for this handle.

Arguments:

    ContextHandle - This is a pointer to an ELF_HANDLE structure.  This is the
        handle that is being closed.

Return Value:

    none.

--*/
{
    UNICODE_STRING  Subsystem;
    NTSTATUS        Status;
    NTSTATUS        AccessStatus;
    ULONG           privileges[1];

    RtlInitUnicodeString(&Subsystem, SubsystemName);

    if (ContextHandle->Flags & ELF_LOG_HANDLE_GENERATE_ON_CLOSE) {

        BOOLEAN     WasEnabled = FALSE;

        //
        // Get Audit Privilege
        //
        privileges[0] = SE_AUDIT_PRIVILEGE;
        AccessStatus = ElfpGetPrivilege(1, privileges);

        if (!NT_SUCCESS(AccessStatus)) {
            ElfDbgPrint(("[ELF] ElfpCloseAudit: ElfpGetPrivilege "
                             "(Enable) failed.  Status is 0x%lx\n",
                         AccessStatus));
        }

        //
        // Generate the Audit.
        //
        Status = NtCloseObjectAuditAlarm(
                    &Subsystem,
                    ContextHandle,
                    TRUE);

        if (!NT_SUCCESS(Status)) {
            ElfDbgPrint(("[ELF] ElfpCloseAudit: NtCloseObjectAuditAlarm Failed: "
            "0x%lx\n",Status));
        }

        ContextHandle->Flags &= (~ELF_LOG_HANDLE_GENERATE_ON_CLOSE);

        ElfpReleasePrivilege();

#ifdef REMOVE
        //
        // Release Audit Privilege
        //
        Status = RtlAdjustPrivilege(
                    SE_AUDIT_PRIVILEGE,
                    FALSE,              // Disable
                    FALSE,              // Use Process's token
                    &WasEnabled);

        if (!NT_SUCCESS(Status)) {
            ElfDbgPrint(("[ELF] ElfpCloseAudit: RtlAdjustPrivilege "
                "(Disable) failed.  Status is 0x%lx\n",Status));
        }
#endif

    }
    return;
}


NTSTATUS
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )
/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling ElfpReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT 
	functions that are called.

--*/
{
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege = (PTOKEN_PRIVILEGES) LocalAlloc(
                                              LMEM_FIXED,
                                              sizeof(TOKEN_PRIVILEGES) +
                                                  (sizeof(LUID_AND_ATTRIBUTES) *
                                                   numPrivileges)
                                              );

    if (pTokenPrivilege == NULL) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege:LocalAlloc Failed %d\n, GetLastError()"));
        return STATUS_NO_MEMORY;
    }

    pTokenPrivilege->PrivilegeCount  = numPrivileges;
    for (i=0; i<numPrivileges ;i++ ) {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertLongToLuid(
                                                pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;

    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja,NULL,0L,NULL,NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_DUPLICATE,
                &ourToken);

    if (!NT_SUCCESS(ntStatus)) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtOpenThreadToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pTokenPrivilege);
        return ntStatus;
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus)) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtDuplicateToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        return ntStatus;
    }

    //
    // Add new privileges
    //

    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                0,                          // size of previous state buffer
                NULL,                       // no previous state info
                &returnLen);                // numBytes required for buffer.

    if (!NT_SUCCESS(ntStatus)) {

        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtAdjustPrivilegesToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return ntStatus;
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&newToken,
                (ULONG)sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus)) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtAdjustPrivilegesToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return ntStatus;
    }

    LocalFree(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return STATUS_SUCCESS;
}


NTSTATUS
ElfpReleasePrivilege(
    VOID
    )
/*++

Routine Description:

    This function relinquishes privileges obtained by calling ElfpGetPrivilege().

Arguments:

    none

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.


--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      &NewToken,
                                      (ULONG)sizeof(HANDLE));

    if ( !NT_SUCCESS(ntStatus) ) {
        return ntStatus;
    }


    return(NO_ERROR);
}


NTSTATUS
ElfpTestClientPrivilege(
    IN ULONG  ulPrivilege,
    IN HANDLE hThreadToken     OPTIONAL
    )

/*++

Routine Description:

    Checks if the client has the supplied privilege.

Arguments:

    None

Return Value:

    STATUS_SUCCESS - if the client has the appropriate privilege.

    STATUS_ACCESS_DENIED - client does not have the required privilege

--*/
{
    NTSTATUS      Status;
    PRIVILEGE_SET PrivilegeSet;
    BOOLEAN       Privileged;
    HANDLE        Token;
    RPC_STATUS    RpcStatus;

    UNICODE_STRING SubSystemName;
    RtlInitUnicodeString(&SubSystemName, L"Eventlog");

    if (hThreadToken != NULL) {
        Token = hThreadToken;
    }
    else {

        RpcStatus = RpcImpersonateClient(NULL);

        if (RpcStatus != RPC_S_OK) {

            ElfDbgPrint(("[ELF] ElfpTestClientPrivilege: "
                             "RpcImpersonateClient FAILED 0x%lx\n",
                         RpcStatus));

            return RpcStatus;
        }

        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_QUERY,
                                   TRUE,
                                   &Token);

        if (!NT_SUCCESS(Status)) {

            //
            // Forget it.
            //

            ElfDbgPrint(("[ELF] ElfpTestClientPrivilege: "
                             "NtOpenThreadToken FAILED 0x%lx\n",
                         Status));

            RpcRevertToSelf();

            return Status;
        }
    }

    //
    // See if the client has the required privilege
    //

    PrivilegeSet.PrivilegeCount          = 1;
    PrivilegeSet.Control                 = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid       = RtlConvertLongToLuid(ulPrivilege);
    PrivilegeSet.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtPrivilegeCheck(Token,
                              &PrivilegeSet,
                              &Privileged);

    if (NT_SUCCESS(Status) || (Status == STATUS_PRIVILEGE_NOT_HELD)) {

        Status = NtPrivilegeObjectAuditAlarm(
                                    &SubSystemName,
                                    NULL,
                                    Token,
                                    0,
                                    &PrivilegeSet,
                                    Privileged);
    }

    if (hThreadToken == NULL ) {

        //
        // We impersonated inside of this function
        //
        NtClose(Token);
        RpcRevertToSelf();
    }

    //
    // Handle unexpected errors
    //

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // If they failed the privilege check, return an error
    //

    if (!Privileged) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // They passed muster
    //

    return STATUS_SUCCESS;
}