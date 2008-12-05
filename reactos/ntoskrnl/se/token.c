/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/token.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, SepInitializeTokenImplementation)
#endif

/* GLOBALS ********************************************************************/

POBJECT_TYPE SepTokenObjectType = NULL;
ERESOURCE SepTokenLock;

static GENERIC_MAPPING SepTokenMapping = {TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_EXECUTE,
TOKEN_ALL_ACCESS};

static const INFORMATION_CLASS_INFO SeTokenInformationClass[] = {
    
    /* Class 0 not used, blame M$! */
    ICI_SQ_SAME( 0, 0, 0),
    
    /* TokenUser */
    ICI_SQ_SAME( sizeof(TOKEN_USER),                   sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenGroups */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenPrivileges */
    ICI_SQ_SAME( sizeof(TOKEN_PRIVILEGES),             sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenOwner */
    ICI_SQ_SAME( sizeof(TOKEN_OWNER),                  sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenPrimaryGroup */
    ICI_SQ_SAME( sizeof(TOKEN_PRIMARY_GROUP),          sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenDefaultDacl */
    ICI_SQ_SAME( sizeof(TOKEN_DEFAULT_DACL),           sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenSource */
    ICI_SQ_SAME( sizeof(TOKEN_SOURCE),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenType */
    ICI_SQ_SAME( sizeof(TOKEN_TYPE),                   sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenImpersonationLevel */
    ICI_SQ_SAME( sizeof(SECURITY_IMPERSONATION_LEVEL), sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenStatistics */
    ICI_SQ_SAME( sizeof(TOKEN_STATISTICS),             sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenRestrictedSids */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSessionId */
    ICI_SQ_SAME( sizeof(ULONG),                        sizeof(ULONG), ICIF_QUERY | ICIF_SET ),
    /* TokenGroupsAndPrivileges */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS_AND_PRIVILEGES),  sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSessionReference */
    ICI_SQ_SAME( /* FIXME */0,                         sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSandBoxInert */
    ICI_SQ_SAME( sizeof(ULONG),                        sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenAuditPolicy */
    ICI_SQ_SAME( /* FIXME */0,                         sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenOrigin */
    ICI_SQ_SAME( sizeof(TOKEN_ORIGIN),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
};

/* FUNCTIONS *****************************************************************/

static NTSTATUS
SepCompareTokens(IN PTOKEN FirstToken,
                 IN PTOKEN SecondToken,
                 OUT PBOOLEAN Equal)
{
    BOOLEAN Restricted, IsEqual = FALSE;
    
    ASSERT(FirstToken != SecondToken);
    
    /* FIXME: Check if every SID that is present in either token is also present in the other one */
    
    Restricted = SeTokenIsRestricted(FirstToken);
    if (Restricted == SeTokenIsRestricted(SecondToken))
    {
        if (Restricted)
        {
            /* FIXME: Check if every SID that is restricted in either token is also restricted in the other one */
        }
        
        /* FIXME: Check if every privilege that is present in either token is also present in the other one */
    }
    
    *Equal = IsEqual;
    return STATUS_SUCCESS;
}

VOID
NTAPI
SepFreeProxyData(PVOID ProxyData)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
SepCopyProxyData(PVOID* Dest, PVOID Src)
{
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
NTAPI
SeExchangePrimaryToken(PEPROCESS Process,
                       PACCESS_TOKEN NewTokenP,
                       PACCESS_TOKEN* OldTokenP)
{
    PTOKEN OldToken;
    PTOKEN NewToken = (PTOKEN)NewTokenP;
    
    PAGED_CODE();
    
    if (NewToken->TokenType != TokenPrimary) return(STATUS_BAD_TOKEN_TYPE);
    if (NewToken->TokenInUse) return(STATUS_TOKEN_ALREADY_IN_USE);
    
    /* Mark new token in use */
    NewToken->TokenInUse = 1;
    
    /* Reference the New Token */
    ObReferenceObject(NewToken);
    
    /* Replace the old with the new */
    OldToken = ObFastReplaceObject(&Process->Token, NewToken);
    
    /* Mark the Old Token as free */
    OldToken->TokenInUse = 0;
    
    *OldTokenP = (PACCESS_TOKEN)OldToken;
    return STATUS_SUCCESS;
}

VOID
NTAPI
SeDeassignPrimaryToken(PEPROCESS Process)
{
    PTOKEN OldToken;
    
    /* Remove the Token */
    OldToken = ObFastReplaceObject(&Process->Token, NULL);
    
    /* Mark the Old Token as free */
    OldToken->TokenInUse = 0;
}

static ULONG
RtlLengthSidAndAttributes(ULONG Count,
                          PSID_AND_ATTRIBUTES Src)
{
    ULONG i;
    ULONG uLength;
    
    PAGED_CODE();
    
    uLength = Count * sizeof(SID_AND_ATTRIBUTES);
    for (i = 0; i < Count; i++)
        uLength += RtlLengthSid(Src[i].Sid);
    
    return(uLength);
}


NTSTATUS
NTAPI
SepFindPrimaryGroupAndDefaultOwner(PTOKEN Token,
                                   PSID PrimaryGroup,
                                   PSID DefaultOwner)
{
    ULONG i;
    
    Token->PrimaryGroup = 0;
    
    if (DefaultOwner)
    {
        Token->DefaultOwnerIndex = Token->UserAndGroupCount;
    }
    
    /* Validate and set the primary group and user pointers */
    for (i = 0; i < Token->UserAndGroupCount; i++)
    {
        if (DefaultOwner &&
            RtlEqualSid(Token->UserAndGroups[i].Sid, DefaultOwner))
        {
            Token->DefaultOwnerIndex = i;
        }
        
        if (RtlEqualSid(Token->UserAndGroups[i].Sid, PrimaryGroup))
        {
            Token->PrimaryGroup = Token->UserAndGroups[i].Sid;
        }
    }
    
    if (Token->DefaultOwnerIndex == Token->UserAndGroupCount)
    {
        return(STATUS_INVALID_OWNER);
    }
    
    if (Token->PrimaryGroup == 0)
    {
        return(STATUS_INVALID_PRIMARY_GROUP);
    }
    
    return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
SepDuplicateToken(PTOKEN Token,
                  POBJECT_ATTRIBUTES ObjectAttributes,
                  BOOLEAN EffectiveOnly,
                  TOKEN_TYPE TokenType,
                  SECURITY_IMPERSONATION_LEVEL Level,
                  KPROCESSOR_MODE PreviousMode,
                  PTOKEN* NewAccessToken)
{
    ULONG uLength;
    ULONG i;
    PVOID EndMem;
    PTOKEN AccessToken;
    NTSTATUS Status;
    
    PAGED_CODE();
    
    Status = ObCreateObject(PreviousMode,
                            SepTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(TOKEN),
                            0,
                            0,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObject() failed (Status %lx)\n");
        return(Status);
    }
    
    Status = ZwAllocateLocallyUniqueId(&AccessToken->TokenId);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(AccessToken);
        return(Status);
    }
    
    Status = ZwAllocateLocallyUniqueId(&AccessToken->ModifiedId);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(AccessToken);
        return(Status);
    }
    
    AccessToken->TokenLock = &SepTokenLock;
    
    AccessToken->TokenInUse = 0;
    AccessToken->TokenType  = TokenType;
    AccessToken->ImpersonationLevel = Level;
    RtlCopyLuid(&AccessToken->AuthenticationId, &Token->AuthenticationId);
    
    AccessToken->TokenSource.SourceIdentifier.LowPart = Token->TokenSource.SourceIdentifier.LowPart;
    AccessToken->TokenSource.SourceIdentifier.HighPart = Token->TokenSource.SourceIdentifier.HighPart;
    memcpy(AccessToken->TokenSource.SourceName,
           Token->TokenSource.SourceName,
           sizeof(Token->TokenSource.SourceName));
    AccessToken->ExpirationTime.QuadPart = Token->ExpirationTime.QuadPart;
    AccessToken->UserAndGroupCount = Token->UserAndGroupCount;
    AccessToken->DefaultOwnerIndex = Token->DefaultOwnerIndex;
    
    uLength = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
    for (i = 0; i < Token->UserAndGroupCount; i++)
        uLength += RtlLengthSid(Token->UserAndGroups[i].Sid);
    
    AccessToken->UserAndGroups =
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
                                               uLength,
                                               TAG('T', 'O', 'K', 'u'));
    
    EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];
    
    Status = RtlCopySidAndAttributesArray(AccessToken->UserAndGroupCount,
                                          Token->UserAndGroups,
                                          uLength,
                                          AccessToken->UserAndGroups,
                                          EndMem,
                                          &EndMem,
                                          &uLength);
    if (NT_SUCCESS(Status))
    {
        Status = SepFindPrimaryGroupAndDefaultOwner(
                                                    AccessToken,
                                                    Token->PrimaryGroup,
                                                    0);
    }
    
    if (NT_SUCCESS(Status))
    {
        AccessToken->PrivilegeCount = Token->PrivilegeCount;
        
        uLength = AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
        AccessToken->Privileges =
        (PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
                                                    uLength,
                                                    TAG('T', 'O', 'K', 'p'));
        
        for (i = 0; i < AccessToken->PrivilegeCount; i++)
        {
            RtlCopyLuid(&AccessToken->Privileges[i].Luid,
                        &Token->Privileges[i].Luid);
            AccessToken->Privileges[i].Attributes =
            Token->Privileges[i].Attributes;
        }
        
        if ( Token->DefaultDacl )
        {
            AccessToken->DefaultDacl =
            (PACL) ExAllocatePoolWithTag(PagedPool,
                                         Token->DefaultDacl->AclSize,
                                         TAG('T', 'O', 'K', 'd'));
            memcpy(AccessToken->DefaultDacl,
                   Token->DefaultDacl,
                   Token->DefaultDacl->AclSize);
        }
        else
        {
            AccessToken->DefaultDacl = 0;
        }
    }
    
    if ( NT_SUCCESS(Status) )
    {
        *NewAccessToken = AccessToken;
        return(STATUS_SUCCESS);
    }
    
    return(Status);
}

NTSTATUS
NTAPI
SeSubProcessToken(IN PTOKEN ParentToken,
                  OUT PTOKEN *Token,
                  IN BOOLEAN InUse,
                  IN ULONG SessionId)
{
    PTOKEN NewToken;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    
    /* Initialize the attributes and duplicate it */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = SepDuplicateToken(ParentToken,
                               &ObjectAttributes,
                               FALSE,
                               TokenPrimary,
                               ParentToken->ImpersonationLevel,
                               KernelMode,
                               &NewToken);
    if (NT_SUCCESS(Status))
    {
        /* Insert it */
        Status = ObInsertObject(NewToken,
                                NULL,
                                0,
                                0,
                                NULL,
                                NULL);
        if (NT_SUCCESS(Status))
        {
            /* Set the session ID */
            NewToken->SessionId = SessionId;
            NewToken->TokenInUse = InUse;
            
            /* Return the token */
            *Token = NewToken;
        }
    }
    
    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
SeIsTokenChild(IN PTOKEN Token,
               OUT PBOOLEAN IsChild)
{
    PTOKEN ProcessToken;
    LUID ProcessLuid, CallerLuid;
    
    /* Assume failure */
    *IsChild = FALSE;
    
    /* Reference the process token */
    ProcessToken = PsReferencePrimaryToken(PsGetCurrentProcess());
    
    /* Get the ID */
    ProcessLuid = ProcessToken->TokenId;
    
    /* Dereference the token */
    ObFastDereferenceObject(&PsGetCurrentProcess()->Token, ProcessToken);
    
    /* Get our LUID */
    CallerLuid = Token->TokenId;
    
    /* Compare the LUIDs */
    if (RtlEqualLuid(&CallerLuid, &ProcessLuid)) *IsChild = TRUE;
    
    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SeCopyClientToken(IN PACCESS_TOKEN Token,
                  IN SECURITY_IMPERSONATION_LEVEL Level,
                  IN KPROCESSOR_MODE PreviousMode,
                  OUT PACCESS_TOKEN* NewToken)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    
    PAGED_CODE();
    
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    Status = SepDuplicateToken(Token,
                               &ObjectAttributes,
                               FALSE,
                               TokenImpersonation,
                               Level,
                               PreviousMode,
                               (PTOKEN*)NewToken);
    
    return(Status);
}

VOID NTAPI
SepDeleteToken(PVOID ObjectBody)
{
    PTOKEN AccessToken = (PTOKEN)ObjectBody;
    
    if (AccessToken->UserAndGroups)
        ExFreePool(AccessToken->UserAndGroups);
    
    if (AccessToken->Privileges)
        ExFreePool(AccessToken->Privileges);
    
    if (AccessToken->DefaultDacl)
        ExFreePool(AccessToken->DefaultDacl);
}


VOID
INIT_FUNCTION
NTAPI
SepInitializeTokenImplementation(VOID)
{
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    
    ExInitializeResource(&SepTokenLock);
    
    DPRINT("Creating Token Object Type\n");
    
    /*  Initialize the Token type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Token");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(TOKEN);
    ObjectTypeInitializer.GenericMapping = SepTokenMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.ValidAccessMask = TOKEN_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.DeleteProcedure = SepDeleteToken;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &SepTokenObjectType);
}

VOID
NTAPI
SeAssignPrimaryToken(IN PEPROCESS Process,
                     IN PTOKEN Token)
{
    PAGED_CODE();
    
    /* Sanity checks */
    ASSERT(Token->TokenType == TokenPrimary);
    ASSERT(!Token->TokenInUse);
    
    /* Clean any previous token */
    if (Process->Token.Object) SeDeassignPrimaryToken(Process);
    
    /* Set the new token */
    ObReferenceObject(Token);
    Token->TokenInUse = TRUE;
    ObInitializeFastReference(&Process->Token, Token);
}

PTOKEN
NTAPI
SepCreateSystemProcessToken(VOID)
{
    NTSTATUS Status;
    ULONG uSize;
    ULONG i;
    ULONG uLocalSystemLength;
    ULONG uWorldLength;
    ULONG uAuthUserLength;
    ULONG uAdminsLength;
    PTOKEN AccessToken;
    PVOID SidArea;
    
    PAGED_CODE();
    
    uLocalSystemLength = RtlLengthSid(SeLocalSystemSid);
    uWorldLength       = RtlLengthSid(SeWorldSid);
    uAuthUserLength    = RtlLengthSid(SeAuthenticatedUserSid);
    uAdminsLength      = RtlLengthSid(SeAliasAdminsSid);
    
    /*
     * Initialize the token
     */
    Status = ObCreateObject(KernelMode,
                            SepTokenObjectType,
                            NULL,
                            KernelMode,
                            NULL,
                            sizeof(TOKEN),
                            0,
                            0,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }
    
    Status = ExpAllocateLocallyUniqueId(&AccessToken->TokenId);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(AccessToken);
        return NULL;
    }
    
    Status = ExpAllocateLocallyUniqueId(&AccessToken->ModifiedId);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(AccessToken);
        return NULL;
    }
    
    Status = ExpAllocateLocallyUniqueId(&AccessToken->AuthenticationId);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(AccessToken);
        return NULL;
    }
    
    AccessToken->TokenLock = &SepTokenLock;
    
    AccessToken->TokenType = TokenPrimary;
    AccessToken->ImpersonationLevel = SecurityDelegation;
    AccessToken->TokenSource.SourceIdentifier.LowPart = 0;
    AccessToken->TokenSource.SourceIdentifier.HighPart = 0;
    memcpy(AccessToken->TokenSource.SourceName, "SeMgr\0\0\0", 8);
    AccessToken->ExpirationTime.QuadPart = -1;
    AccessToken->UserAndGroupCount = 4;
    
    uSize = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
    uSize += uLocalSystemLength;
    uSize += uWorldLength;
    uSize += uAuthUserLength;
    uSize += uAdminsLength;
    
    AccessToken->UserAndGroups =
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
                                               uSize,
                                               TAG('T', 'O', 'K', 'u'));
    SidArea = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];
    
    i = 0;
    AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
    AccessToken->UserAndGroups[i++].Attributes = 0;
    RtlCopySid(uLocalSystemLength, SidArea, SeLocalSystemSid);
    SidArea = (char*)SidArea + uLocalSystemLength;
    
    AccessToken->DefaultOwnerIndex = i;
    AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
    AccessToken->PrimaryGroup = (PSID) SidArea;
    AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT;
    Status = RtlCopySid(uAdminsLength, SidArea, SeAliasAdminsSid);
    SidArea = (char*)SidArea + uAdminsLength;
    
    AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
    AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
    RtlCopySid(uWorldLength, SidArea, SeWorldSid);
    SidArea = (char*)SidArea + uWorldLength;
    
    AccessToken->UserAndGroups[i].Sid = (PSID) SidArea;
    AccessToken->UserAndGroups[i++].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
    RtlCopySid(uAuthUserLength, SidArea, SeAuthenticatedUserSid);
    SidArea = (char*)SidArea + uAuthUserLength;
    
    AccessToken->PrivilegeCount = 20;
    
    uSize = AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
    AccessToken->Privileges =
    (PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
                                                uSize,
                                                TAG('T', 'O', 'K', 'p'));
    
    i = 0;
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeTcbPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeCreateTokenPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeTakeOwnershipPrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeCreatePagefilePrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeLockMemoryPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeAssignPrimaryTokenPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeIncreaseQuotaPrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeIncreaseBasePriorityPrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeCreatePermanentPrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeDebugPrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeAuditPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeSecurityPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeSystemEnvironmentPrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeChangeNotifyPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeBackupPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeRestorePrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeShutdownPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeLoadDriverPrivilege;
    
    AccessToken->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;
    AccessToken->Privileges[i++].Luid = SeProfileSingleProcessPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeSystemtimePrivilege;
#if 0
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeUndockPrivilege;
    
    AccessToken->Privileges[i].Attributes = 0;
    AccessToken->Privileges[i++].Luid = SeManageVolumePrivilege;
#endif
    
    ASSERT(i == 20);
    
    uSize = sizeof(ACL);
    uSize += sizeof(ACE) + uLocalSystemLength;
    uSize += sizeof(ACE) + uAdminsLength;
    uSize = (uSize & (~3)) + 8;
    AccessToken->DefaultDacl =
    (PACL) ExAllocatePoolWithTag(PagedPool,
                                 uSize,
                                 TAG('T', 'O', 'K', 'd'));
    Status = RtlCreateAcl(AccessToken->DefaultDacl, uSize, ACL_REVISION);
    if ( NT_SUCCESS(Status) )
    {
        Status = RtlAddAccessAllowedAce(AccessToken->DefaultDacl, ACL_REVISION, GENERIC_ALL, SeLocalSystemSid);
    }
    
    if ( NT_SUCCESS(Status) )
    {
        Status = RtlAddAccessAllowedAce(AccessToken->DefaultDacl, ACL_REVISION, GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE|READ_CONTROL, SeAliasAdminsSid);
    }
    
    if ( ! NT_SUCCESS(Status) )
    {
        ObDereferenceObject(AccessToken);
        return NULL;
    }
    
    return AccessToken;
}

/* PUBLIC FUNCTIONS ***********************************************************/
 
/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeFilterToken(IN PACCESS_TOKEN ExistingToken,
              IN ULONG Flags,
              IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
              IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
              IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
              OUT PACCESS_TOKEN * FilteredToken)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeQueryInformationToken(IN PACCESS_TOKEN Token,
                        IN TOKEN_INFORMATION_CLASS TokenInformationClass,
                        OUT PVOID *TokenInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeQuerySessionIdToken(IN PACCESS_TOKEN Token,
                      IN PULONG pSessionId)
{
    *pSessionId = ((PTOKEN)Token)->SessionId;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
SeQueryAuthenticationIdToken(IN PACCESS_TOKEN Token,
                             OUT PLUID LogonId)
{
    PAGED_CODE();
    
    *LogonId = ((PTOKEN)Token)->AuthenticationId;
    
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
SECURITY_IMPERSONATION_LEVEL
NTAPI
SeTokenImpersonationLevel(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();
    
    return ((PTOKEN)Token)->ImpersonationLevel;
}


/*
 * @implemented
 */
TOKEN_TYPE NTAPI
SeTokenType(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();
    
    return ((PTOKEN)Token)->TokenType;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
SeTokenIsAdmin(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();
    return (((PTOKEN)Token)->TokenFlags & TOKEN_WRITE_RESTRICTED) != 0;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
SeTokenIsRestricted(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();
    return (((PTOKEN)Token)->TokenFlags & TOKEN_IS_RESTRICTED) != 0;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
SeTokenIsWriteRestricted(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();
    return (((PTOKEN)Token)->TokenFlags & TOKEN_HAS_RESTORE_PRIVILEGE) != 0;
}

/* SYSTEM CALLS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS NTAPI
NtQueryInformationToken(IN HANDLE TokenHandle,
                        IN TOKEN_INFORMATION_CLASS TokenInformationClass,
                        OUT PVOID TokenInformation,
                        IN ULONG TokenInformationLength,
                        OUT PULONG ReturnLength)
{
    union
    {
        PVOID Ptr;
        ULONG Ulong;
    } Unused;
    PTOKEN Token;
    ULONG RequiredLength;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
    
    /* Check buffers and class validity */
    Status = DefaultQueryInfoBufferCheck(TokenInformationClass,
                                         SeTokenInformationClass,
                                         sizeof(SeTokenInformationClass) / sizeof(SeTokenInformationClass[0]),
                                         TokenInformation,
                                         TokenInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    
    if(!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }
    
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       (TokenInformationClass == TokenSource) ? TOKEN_QUERY_SOURCE : TOKEN_QUERY,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        switch (TokenInformationClass)
        {
            case TokenUser:
            {
                PTOKEN_USER tu = (PTOKEN_USER)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenUser)\n");
                RequiredLength = sizeof(TOKEN_USER) +
                RtlLengthSid(Token->UserAndGroups[0].Sid);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        Status = RtlCopySidAndAttributesArray(1,
                                                              &Token->UserAndGroups[0],
                                                              RequiredLength - sizeof(TOKEN_USER),
                                                              &tu->User,
                                                              (PSID)(tu + 1),
                                                              &Unused.Ptr,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenGroups:
            {
                PTOKEN_GROUPS tg = (PTOKEN_GROUPS)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenGroups)\n");
                RequiredLength = sizeof(tg->GroupCount) +
                RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        ULONG SidLen = RequiredLength - sizeof(tg->GroupCount) -
                        ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES));
                        PSID_AND_ATTRIBUTES Sid = (PSID_AND_ATTRIBUTES)((ULONG_PTR)TokenInformation + sizeof(tg->GroupCount) +
                                                                        ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES)));
                        
                        tg->GroupCount = Token->UserAndGroupCount - 1;
                        Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
                                                              &Token->UserAndGroups[1],
                                                              SidLen,
                                                              &tg->Groups[0],
                                                              (PSID)Sid,
                                                              &Unused.Ptr,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenPrivileges:
            {
                PTOKEN_PRIVILEGES tp = (PTOKEN_PRIVILEGES)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenPrivileges)\n");
                RequiredLength = sizeof(tp->PrivilegeCount) +
                (Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        tp->PrivilegeCount = Token->PrivilegeCount;
                        RtlCopyLuidAndAttributesArray(Token->PrivilegeCount,
                                                      Token->Privileges,
                                                      &tp->Privileges[0]);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenOwner:
            {
                ULONG SidLen;
                PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenOwner)\n");
                SidLen = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
                RequiredLength = sizeof(TOKEN_OWNER) + SidLen;
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        to->Owner = (PSID)(to + 1);
                        Status = RtlCopySid(SidLen,
                                            to->Owner,
                                            Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenPrimaryGroup:
            {
                ULONG SidLen;
                PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenPrimaryGroup)\n");
                SidLen = RtlLengthSid(Token->PrimaryGroup);
                RequiredLength = sizeof(TOKEN_PRIMARY_GROUP) + SidLen;
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        tpg->PrimaryGroup = (PSID)(tpg + 1);
                        Status = RtlCopySid(SidLen,
                                            tpg->PrimaryGroup,
                                            Token->PrimaryGroup);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenDefaultDacl:
            {
                PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenDefaultDacl)\n");
                RequiredLength = sizeof(TOKEN_DEFAULT_DACL);
                
                if(Token->DefaultDacl != NULL)
                {
                    RequiredLength += Token->DefaultDacl->AclSize;
                }
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        if(Token->DefaultDacl != NULL)
                        {
                            tdd->DefaultDacl = (PACL)(tdd + 1);
                            RtlCopyMemory(tdd->DefaultDacl,
                                          Token->DefaultDacl,
                                          Token->DefaultDacl->AclSize);
                        }
                        else
                        {
                            tdd->DefaultDacl = NULL;
                        }
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenSource:
            {
                PTOKEN_SOURCE ts = (PTOKEN_SOURCE)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenSource)\n");
                RequiredLength = sizeof(TOKEN_SOURCE);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        *ts = Token->TokenSource;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenType:
            {
                PTOKEN_TYPE tt = (PTOKEN_TYPE)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenType)\n");
                RequiredLength = sizeof(TOKEN_TYPE);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        *tt = Token->TokenType;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenImpersonationLevel:
            {
                PSECURITY_IMPERSONATION_LEVEL sil = (PSECURITY_IMPERSONATION_LEVEL)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenImpersonationLevel)\n");
                RequiredLength = sizeof(SECURITY_IMPERSONATION_LEVEL);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        *sil = Token->ImpersonationLevel;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenStatistics:
            {
                PTOKEN_STATISTICS ts = (PTOKEN_STATISTICS)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenStatistics)\n");
                RequiredLength = sizeof(TOKEN_STATISTICS);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        ts->TokenId = Token->TokenId;
                        ts->AuthenticationId = Token->AuthenticationId;
                        ts->ExpirationTime = Token->ExpirationTime;
                        ts->TokenType = Token->TokenType;
                        ts->ImpersonationLevel = Token->ImpersonationLevel;
                        ts->DynamicCharged = Token->DynamicCharged;
                        ts->DynamicAvailable = Token->DynamicAvailable;
                        ts->GroupCount = Token->UserAndGroupCount - 1;
                        ts->PrivilegeCount = Token->PrivilegeCount;
                        ts->ModifiedId = Token->ModifiedId;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenOrigin:
            {
                PTOKEN_ORIGIN to = (PTOKEN_ORIGIN)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenOrigin)\n");
                RequiredLength = sizeof(TOKEN_ORIGIN);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        RtlCopyLuid(&to->OriginatingLogonSession,
                                    &Token->AuthenticationId);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenGroupsAndPrivileges:
                DPRINT1("NtQueryInformationToken(TokenGroupsAndPrivileges) not implemented\n");
                Status = STATUS_NOT_IMPLEMENTED;
                break;
                
            case TokenRestrictedSids:
            {
                PTOKEN_GROUPS tg = (PTOKEN_GROUPS)TokenInformation;
                
                DPRINT("NtQueryInformationToken(TokenRestrictedSids)\n");
                RequiredLength = sizeof(tg->GroupCount) +
                RtlLengthSidAndAttributes(Token->RestrictedSidCount, Token->RestrictedSids);
                
                _SEH2_TRY
                {
                    if(TokenInformationLength >= RequiredLength)
                    {
                        ULONG SidLen = RequiredLength - sizeof(tg->GroupCount) -
                        (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES));
                        PSID_AND_ATTRIBUTES Sid = (PSID_AND_ATTRIBUTES)((ULONG_PTR)TokenInformation + sizeof(tg->GroupCount) +
                                                                        (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES)));
                        
                        tg->GroupCount = Token->RestrictedSidCount;
                        Status = RtlCopySidAndAttributesArray(Token->RestrictedSidCount,
                                                              Token->RestrictedSids,
                                                              SidLen,
                                                              &tg->Groups[0],
                                                              (PSID)Sid,
                                                              &Unused.Ptr,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    
                    if(ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                break;
            }
                
            case TokenSandBoxInert:
                DPRINT1("NtQueryInformationToken(TokenSandboxInert) not implemented\n");
                Status = STATUS_NOT_IMPLEMENTED;
                break;
                
            case TokenSessionId:
            {
                ULONG SessionId = 0;
                
                DPRINT("NtQueryInformationToken(TokenSessionId)\n");
                
                Status = SeQuerySessionIdToken(Token,
                                               &SessionId);
                
                if(NT_SUCCESS(Status))
                {
                    _SEH2_TRY
                    {
                        /* buffer size was already verified, no need to check here again */
                        *(PULONG)TokenInformation = SessionId;
                        
                        if(ReturnLength != NULL)
                        {
                            *ReturnLength = sizeof(ULONG);
                        }
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                }
                
                break;
            }
                
            default:
                DPRINT1("NtQueryInformationToken(%d) invalid information class\n", TokenInformationClass);
                Status = STATUS_INVALID_INFO_CLASS;
                break;
        }
        
        ObDereferenceObject(Token);
    }
    
    return(Status);
}


/*
 * NtSetTokenInformation: Partly implemented.
 * Unimplemented:
 *  TokenOrigin, TokenDefaultDacl
 */

NTSTATUS NTAPI
NtSetInformationToken(IN HANDLE TokenHandle,
                      IN TOKEN_INFORMATION_CLASS TokenInformationClass,
                      OUT PVOID TokenInformation,
                      IN ULONG TokenInformationLength)
{
    PTOKEN Token;
    KPROCESSOR_MODE PreviousMode;
    ULONG NeededAccess = TOKEN_ADJUST_DEFAULT;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
    
    Status = DefaultSetInfoBufferCheck(TokenInformationClass,
                                       SeTokenInformationClass,
                                       sizeof(SeTokenInformationClass) / sizeof(SeTokenInformationClass[0]),
                                       TokenInformation,
                                       TokenInformationLength,
                                       PreviousMode);
    
    if(!NT_SUCCESS(Status))
    {
        /* Invalid buffers */
        DPRINT("NtSetInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }
    
    if(TokenInformationClass == TokenSessionId)
    {
        NeededAccess |= TOKEN_ADJUST_SESSIONID;
    }
    
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       NeededAccess,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        switch (TokenInformationClass)
        {
            case TokenOwner:
            {
                if(TokenInformationLength >= sizeof(TOKEN_OWNER))
                {
                    PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
                    PSID InputSid = NULL;
                    
                    _SEH2_TRY
                    {
                        InputSid = to->Owner;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                    
                    if(NT_SUCCESS(Status))
                    {
                        PSID CapturedSid;
                        
                        Status = SepCaptureSid(InputSid,
                                               PreviousMode,
                                               PagedPool,
                                               FALSE,
                                               &CapturedSid);
                        if(NT_SUCCESS(Status))
                        {
                            RtlCopySid(RtlLengthSid(CapturedSid),
                                       Token->UserAndGroups[Token->DefaultOwnerIndex].Sid,
                                       CapturedSid);
                            SepReleaseSid(CapturedSid,
                                          PreviousMode,
                                          FALSE);
                        }
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }
                
            case TokenPrimaryGroup:
            {
                if(TokenInformationLength >= sizeof(TOKEN_PRIMARY_GROUP))
                {
                    PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
                    PSID InputSid = NULL;
                    
                    _SEH2_TRY
                    {
                        InputSid = tpg->PrimaryGroup;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                    
                    if(NT_SUCCESS(Status))
                    {
                        PSID CapturedSid;
                        
                        Status = SepCaptureSid(InputSid,
                                               PreviousMode,
                                               PagedPool,
                                               FALSE,
                                               &CapturedSid);
                        if(NT_SUCCESS(Status))
                        {
                            RtlCopySid(RtlLengthSid(CapturedSid),
                                       Token->PrimaryGroup,
                                       CapturedSid);
                            SepReleaseSid(CapturedSid,
                                          PreviousMode,
                                          FALSE);
                        }
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }
                
            case TokenDefaultDacl:
            {
                if(TokenInformationLength >= sizeof(TOKEN_DEFAULT_DACL))
                {
                    PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;
                    PACL InputAcl = NULL;
                    
                    _SEH2_TRY
                    {
                        InputAcl = tdd->DefaultDacl;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                    
                    if(NT_SUCCESS(Status))
                    {
                        if(InputAcl != NULL)
                        {
                            PACL CapturedAcl;
                            
                            /* capture and copy the dacl */
                            Status = SepCaptureAcl(InputAcl,
                                                   PreviousMode,
                                                   PagedPool,
                                                   TRUE,
                                                   &CapturedAcl);
                            if(NT_SUCCESS(Status))
                            {
                                /* free the previous dacl if present */
                                if(Token->DefaultDacl != NULL)
                                {
                                    ExFreePool(Token->DefaultDacl);
                                }
                                
                                /* set the new dacl */
                                Token->DefaultDacl = CapturedAcl;
                            }
                        }
                        else
                        {
                            /* clear and free the default dacl if present */
                            if(Token->DefaultDacl != NULL)
                            {
                                ExFreePool(Token->DefaultDacl);
                                Token->DefaultDacl = NULL;
                            }
                        }
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }
                
            case TokenSessionId:
            {
                ULONG SessionId = 0;
                
                _SEH2_TRY
                {
                    /* buffer size was already verified, no need to check here again */
                    SessionId = *(PULONG)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
                
                if(NT_SUCCESS(Status))
                {
                    if(!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                               PreviousMode))
                    {
                        Status = STATUS_PRIVILEGE_NOT_HELD;
                        break;
                    }
                    
                    Token->SessionId = SessionId;
                }
                break;
            }
                
            default:
            {
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }
        }
        
        ObDereferenceObject(Token);
    }
    
    return(Status);
}


/*
 * @implemented
 *
 * NOTE: Some sources claim 4th param is ImpersonationLevel, but on W2K
 * this is certainly NOT true, thou i can't say for sure that EffectiveOnly
 * is correct either. -Gunnar
 * This is true. EffectiveOnly overrides SQOS.EffectiveOnly. - IAI
 */
NTSTATUS NTAPI
NtDuplicateToken(IN HANDLE ExistingTokenHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                 IN BOOLEAN EffectiveOnly,
                 IN TOKEN_TYPE TokenType,
                 OUT PHANDLE NewTokenHandle)
{
    KPROCESSOR_MODE PreviousMode;
    HANDLE hToken;
    PTOKEN Token;
    PTOKEN NewToken;
    PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService;
    BOOLEAN QoSPresent;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = KeGetPreviousMode();
    
    if(PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(NewTokenHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        
        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    
    Status = SepCaptureSecurityQualityOfService(ObjectAttributes,
                                                PreviousMode,
                                                PagedPool,
                                                FALSE,
                                                &CapturedSecurityQualityOfService,
                                                &QoSPresent);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateToken() failed to capture QoS! Status: 0x%x\n", Status);
        return Status;
    }
    
    Status = ObReferenceObjectByHandle(ExistingTokenHandle,
                                       TOKEN_DUPLICATE,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        Status = SepDuplicateToken(Token,
                                   ObjectAttributes,
                                   EffectiveOnly,
                                   TokenType,
                                   (QoSPresent ? CapturedSecurityQualityOfService->ImpersonationLevel : SecurityAnonymous),
                                   PreviousMode,
                                   &NewToken);
        
        ObDereferenceObject(Token);
        
        if (NT_SUCCESS(Status))
        {
            Status = ObInsertObject((PVOID)NewToken,
                                    NULL,
                                    DesiredAccess,
                                    0,
                                    NULL,
                                    &hToken);
            
            if (NT_SUCCESS(Status))
            {
                _SEH2_TRY
                {
                    *NewTokenHandle = hToken;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
        }
    }
    
    /* free the captured structure */
    SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                       PreviousMode,
                                       FALSE);
    
    return Status;
}

NTSTATUS NTAPI
NtAdjustGroupsToken(IN HANDLE TokenHandle,
                    IN BOOLEAN ResetToDefault,
                    IN PTOKEN_GROUPS NewState,
                    IN ULONG BufferLength,
                    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
                    OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
NtAdjustPrivilegesToken (IN HANDLE TokenHandle,
                         IN BOOLEAN DisableAllPrivileges,
                         IN PTOKEN_PRIVILEGES NewState,
                         IN ULONG BufferLength,
                         OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
                         OUT PULONG ReturnLength OPTIONAL)
{
    //  PLUID_AND_ATTRIBUTES Privileges;
    KPROCESSOR_MODE PreviousMode;
    ULONG PrivilegeCount;
    PTOKEN Token;
    //  ULONG Length;
    ULONG i;
    ULONG j;
    ULONG k;
    ULONG Count;
#if 0
    ULONG a;
    ULONG b;
    ULONG c;
#endif
    NTSTATUS Status;
    
    PAGED_CODE();
    
    DPRINT ("NtAdjustPrivilegesToken() called\n");
    
    //  PrivilegeCount = NewState->PrivilegeCount;
    PreviousMode = KeGetPreviousMode ();
    //  SeCaptureLuidAndAttributesArray(NewState->Privileges,
    //                                  PrivilegeCount,
    //                                  PreviousMode,
    //                                  NULL,
    //                                  0,
    //                                  NonPagedPool,
    //                                  1,
    //                                  &Privileges,
    //                                  &Length);
    
    Status = ObReferenceObjectByHandle (TokenHandle,
                                        TOKEN_ADJUST_PRIVILEGES | (PreviousState != NULL ? TOKEN_QUERY : 0),
                                        SepTokenObjectType,
                                        PreviousMode,
                                        (PVOID*)&Token,
                                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("Failed to reference token (Status %lx)\n", Status);
        //      SeReleaseLuidAndAttributesArray(Privileges,
        //                                      PreviousMode,
        //                                      0);
        return Status;
    }
    
    
#if 0
    SepAdjustPrivileges(Token,
                        0,
                        PreviousMode,
                        PrivilegeCount,
                        Privileges,
                        PreviousState,
                        &a,
                        &b,
                        &c);
#endif
    
    PrivilegeCount = (BufferLength - FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges)) /
    sizeof(LUID_AND_ATTRIBUTES);
    
    if (PreviousState != NULL)
        PreviousState->PrivilegeCount = 0;
    
    k = 0;
    if (DisableAllPrivileges == TRUE)
    {
        for (i = 0; i < Token->PrivilegeCount; i++)
        {
            if (Token->Privileges[i].Attributes != 0)
            {
                DPRINT ("Attributes differ\n");
                
                /* Save current privilege */
                if (PreviousState != NULL)
                {
                    if (k < PrivilegeCount)
                    {
                        PreviousState->PrivilegeCount++;
                        PreviousState->Privileges[k].Luid = Token->Privileges[i].Luid;
                        PreviousState->Privileges[k].Attributes = Token->Privileges[i].Attributes;
                    }
                    else
                    {
                        /* FIXME: Should revert all the changes, calculate how
                         * much space would be needed, set ResultLength
                         * accordingly and fail.
                         */
                    }
                    k++;
                }
                
                /* Update current privlege */
                Token->Privileges[i].Attributes &= ~SE_PRIVILEGE_ENABLED;
            }
        }
        Status = STATUS_SUCCESS;
    }
    else
    {
        Count = 0;
        for (i = 0; i < Token->PrivilegeCount; i++)
        {
            for (j = 0; j < NewState->PrivilegeCount; j++)
            {
                if (Token->Privileges[i].Luid.LowPart == NewState->Privileges[j].Luid.LowPart &&
                    Token->Privileges[i].Luid.HighPart == NewState->Privileges[j].Luid.HighPart)
                {
                    DPRINT ("Found privilege\n");
                    
                    if ((Token->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) !=
                        (NewState->Privileges[j].Attributes & SE_PRIVILEGE_ENABLED))
                    {
                        DPRINT ("Attributes differ\n");
                        DPRINT ("Current attributes %lx  desired attributes %lx\n",
                                Token->Privileges[i].Attributes,
                                NewState->Privileges[j].Attributes);
                        
                        /* Save current privilege */
                        if (PreviousState != NULL)
                        {
                            if (k < PrivilegeCount)
                            {
                                PreviousState->PrivilegeCount++;
                                PreviousState->Privileges[k].Luid = Token->Privileges[i].Luid;
                                PreviousState->Privileges[k].Attributes = Token->Privileges[i].Attributes;
                            }
                            else
                            {
                                /* FIXME: Should revert all the changes, calculate how
                                 * much space would be needed, set ResultLength
                                 * accordingly and fail.
                                 */
                            }
                            k++;
                        }
                        
                        /* Update current privlege */
                        Token->Privileges[i].Attributes &= ~SE_PRIVILEGE_ENABLED;
                        Token->Privileges[i].Attributes |=
                        (NewState->Privileges[j].Attributes & SE_PRIVILEGE_ENABLED);
                        DPRINT ("New attributes %lx\n",
                                Token->Privileges[i].Attributes);
                    }
                    Count++;
                }
            }
        }
        Status = Count < NewState->PrivilegeCount ? STATUS_NOT_ALL_ASSIGNED : STATUS_SUCCESS;
    }
    
    if (ReturnLength != NULL)
    {
        *ReturnLength = sizeof(TOKEN_PRIVILEGES) +
        (sizeof(LUID_AND_ATTRIBUTES) * (k - 1));
    }
    
    ObDereferenceObject (Token);
    
    //  SeReleaseLuidAndAttributesArray(Privileges,
    //                                  PreviousMode,
    //                                  0);
    
    DPRINT ("NtAdjustPrivilegesToken() done\n");
    
    return Status;
}

NTSTATUS NTAPI
NtCreateToken(OUT PHANDLE TokenHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes,
              IN TOKEN_TYPE TokenType,
              IN PLUID AuthenticationId,
              IN PLARGE_INTEGER ExpirationTime,
              IN PTOKEN_USER TokenUser,
              IN PTOKEN_GROUPS TokenGroups,
              IN PTOKEN_PRIVILEGES TokenPrivileges,
              IN PTOKEN_OWNER TokenOwner,
              IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
              IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
              IN PTOKEN_SOURCE TokenSource)
{
    HANDLE hToken;
    PTOKEN AccessToken;
    LUID TokenId;
    LUID ModifiedId;
    PVOID EndMem;
    ULONG uLength;
    ULONG i;
    KPROCESSOR_MODE PreviousMode;
    ULONG nTokenPrivileges = 0;
    LARGE_INTEGER LocalExpirationTime = {{0, 0}};
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
    
    if(PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(TokenHandle);
            ProbeForRead(AuthenticationId,
                         sizeof(LUID),
                         sizeof(ULONG));
            LocalExpirationTime = ProbeForReadLargeInteger(ExpirationTime);
            ProbeForRead(TokenUser,
                         sizeof(TOKEN_USER),
                         sizeof(ULONG));
            ProbeForRead(TokenGroups,
                         sizeof(TOKEN_GROUPS),
                         sizeof(ULONG));
            ProbeForRead(TokenPrivileges,
                         sizeof(TOKEN_PRIVILEGES),
                         sizeof(ULONG));
            ProbeForRead(TokenOwner,
                         sizeof(TOKEN_OWNER),
                         sizeof(ULONG));
            ProbeForRead(TokenPrimaryGroup,
                         sizeof(TOKEN_PRIMARY_GROUP),
                         sizeof(ULONG));
            ProbeForRead(TokenDefaultDacl,
                         sizeof(TOKEN_DEFAULT_DACL),
                         sizeof(ULONG));
            ProbeForRead(TokenSource,
                         sizeof(TOKEN_SOURCE),
                         sizeof(ULONG));
            nTokenPrivileges = TokenPrivileges->PrivilegeCount;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        
        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        nTokenPrivileges = TokenPrivileges->PrivilegeCount;
        LocalExpirationTime = *ExpirationTime;
    }
    
    Status = ZwAllocateLocallyUniqueId(&TokenId);
    if (!NT_SUCCESS(Status))
        return(Status);
    
    Status = ZwAllocateLocallyUniqueId(&ModifiedId);
    if (!NT_SUCCESS(Status))
        return(Status);
    
    Status = ObCreateObject(PreviousMode,
                            SepTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(TOKEN),
                            0,
                            0,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObject() failed (Status %lx)\n");
        return(Status);
    }
    
    AccessToken->TokenLock = &SepTokenLock;
    
    RtlCopyLuid(&AccessToken->TokenSource.SourceIdentifier,
                &TokenSource->SourceIdentifier);
    memcpy(AccessToken->TokenSource.SourceName,
           TokenSource->SourceName,
           sizeof(TokenSource->SourceName));
    
    RtlCopyLuid(&AccessToken->TokenId, &TokenId);
    RtlCopyLuid(&AccessToken->AuthenticationId, AuthenticationId);
    AccessToken->ExpirationTime = *ExpirationTime;
    RtlCopyLuid(&AccessToken->ModifiedId, &ModifiedId);
    
    AccessToken->UserAndGroupCount = TokenGroups->GroupCount + 1;
    AccessToken->PrivilegeCount    = TokenPrivileges->PrivilegeCount;
    AccessToken->UserAndGroups     = 0;
    AccessToken->Privileges        = 0;
    
    AccessToken->TokenType = TokenType;
    AccessToken->ImpersonationLevel = ((PSECURITY_QUALITY_OF_SERVICE)
                                       (ObjectAttributes->SecurityQualityOfService))->ImpersonationLevel;
    
    /*
     * Normally we would just point these members into the variable information
     * area; however, our ObCreateObject() call can't allocate a variable information
     * area, so we allocate them seperately and provide a destroy function.
     */
    
    uLength = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
    uLength += RtlLengthSid(TokenUser->User.Sid);
    for (i = 0; i < TokenGroups->GroupCount; i++)
        uLength += RtlLengthSid(TokenGroups->Groups[i].Sid);
    
    AccessToken->UserAndGroups =
    (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
                                               uLength,
                                               TAG('T', 'O', 'K', 'u'));
    
    EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];
    
    Status = RtlCopySidAndAttributesArray(1,
                                          &TokenUser->User,
                                          uLength,
                                          AccessToken->UserAndGroups,
                                          EndMem,
                                          &EndMem,
                                          &uLength);
    if (NT_SUCCESS(Status))
    {
        Status = RtlCopySidAndAttributesArray(TokenGroups->GroupCount,
                                              TokenGroups->Groups,
                                              uLength,
                                              &AccessToken->UserAndGroups[1],
                                              EndMem,
                                              &EndMem,
                                              &uLength);
    }
    
    if (NT_SUCCESS(Status))
    {
        Status = SepFindPrimaryGroupAndDefaultOwner(
                                                    AccessToken,
                                                    TokenPrimaryGroup->PrimaryGroup,
                                                    TokenOwner->Owner);
    }
    
    if (NT_SUCCESS(Status))
    {
        uLength = TokenPrivileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
        AccessToken->Privileges =
        (PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PagedPool,
                                                    uLength,
                                                    TAG('T', 'O', 'K', 'p'));
        
        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
            {
                RtlCopyMemory(AccessToken->Privileges,
                              TokenPrivileges->Privileges,
                              nTokenPrivileges * sizeof(LUID_AND_ATTRIBUTES));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else
        {
            RtlCopyMemory(AccessToken->Privileges,
                          TokenPrivileges->Privileges,
                          nTokenPrivileges * sizeof(LUID_AND_ATTRIBUTES));
        }
    }
    
    if (NT_SUCCESS(Status))
    {
        AccessToken->DefaultDacl =
        (PACL) ExAllocatePoolWithTag(PagedPool,
                                     TokenDefaultDacl->DefaultDacl->AclSize,
                                     TAG('T', 'O', 'K', 'd'));
        memcpy(AccessToken->DefaultDacl,
               TokenDefaultDacl->DefaultDacl,
               TokenDefaultDacl->DefaultDacl->AclSize);
    }
    
    Status = ObInsertObject ((PVOID)AccessToken,
                             NULL,
                             DesiredAccess,
                             0,
                             NULL,
                             &hToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObInsertObject() failed (Status %lx)\n", Status);
    }
    
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            *TokenHandle = hToken;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenThreadTokenEx(IN HANDLE ThreadHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN BOOLEAN OpenAsSelf,
                    IN ULONG HandleAttributes,
                    OUT PHANDLE TokenHandle)
{
    PETHREAD Thread;
    HANDLE hToken;
    PTOKEN Token, NewToken, PrimaryToken;
    BOOLEAN CopyOnOpen, EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    SE_IMPERSONATION_STATE ImpersonationState;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl = NULL;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
    
    if(PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(TokenHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        
        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    
    /*
     * At first open the thread token for information access and verify
     * that the token associated with thread is valid.
     */
    
    Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_QUERY_INFORMATION,
                                       PsThreadType, PreviousMode, (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    
    Token = PsReferenceImpersonationToken(Thread, &CopyOnOpen, &EffectiveOnly,
                                          &ImpersonationLevel);
    if (Token == NULL)
    {
        ObfDereferenceObject(Thread);
        return STATUS_NO_TOKEN;
    }
    
    ObDereferenceObject(Thread);
    
    if (ImpersonationLevel == SecurityAnonymous)
    {
        ObfDereferenceObject(Token);
        return STATUS_CANT_OPEN_ANONYMOUS;
    }
    
    /*
     * Revert to self if OpenAsSelf is specified.
     */
    
    if (OpenAsSelf)
    {
        PsDisableImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }
    
    if (CopyOnOpen)
    {
        Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS,
                                           PsThreadType, PreviousMode,
                                           (PVOID*)&Thread, NULL);
        if (!NT_SUCCESS(Status))
        {
            ObfDereferenceObject(Token);
            if (OpenAsSelf)
            {
                PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
            }
            return Status;
        }
        
        PrimaryToken = PsReferencePrimaryToken(Thread->ThreadsProcess);
        Status = SepCreateImpersonationTokenDacl(Token, PrimaryToken, &Dacl);
        ASSERT(FALSE);
        ObfDereferenceObject(PrimaryToken);
        ObfDereferenceObject(Thread);
        if (!NT_SUCCESS(Status))
        {
            ObfDereferenceObject(Token);
            if (OpenAsSelf)
            {
                PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
            }
            return Status;
        }
        
        RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                    SECURITY_DESCRIPTOR_REVISION);
        RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl,
                                     FALSE);
        
        InitializeObjectAttributes(&ObjectAttributes, NULL, HandleAttributes,
                                   NULL, &SecurityDescriptor);
        
        Status = SepDuplicateToken(Token, &ObjectAttributes, EffectiveOnly,
                                   TokenImpersonation, ImpersonationLevel,
                                   KernelMode, &NewToken);
        ExFreePool(Dacl);
        if (!NT_SUCCESS(Status))
        {
            ObfDereferenceObject(Token);
            if (OpenAsSelf)
            {
                PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
            }
            return Status;
        }
        
        Status = ObInsertObject(NewToken, NULL, DesiredAccess, 0, NULL,
                                &hToken);
        
    }
    else
    {
        Status = ObOpenObjectByPointer(Token, HandleAttributes,
                                       NULL, DesiredAccess, SepTokenObjectType,
                                       PreviousMode, &hToken);
    }
    
    ObfDereferenceObject(Token);
    
    if (OpenAsSelf)
    {
        PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }
    
    if(NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            *TokenHandle = hToken;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    
    return Status;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
NtOpenThreadToken(IN HANDLE ThreadHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN BOOLEAN OpenAsSelf,
                  OUT PHANDLE TokenHandle)
{
    return NtOpenThreadTokenEx(ThreadHandle, DesiredAccess, OpenAsSelf, 0,
                               TokenHandle);
}



/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtCompareTokens(IN HANDLE FirstTokenHandle,
                IN HANDLE SecondTokenHandle,
                OUT PBOOLEAN Equal)
{
    KPROCESSOR_MODE PreviousMode;
    PTOKEN FirstToken, SecondToken;
    BOOLEAN IsEqual;
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
    
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteBoolean(Equal);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        
        if (!NT_SUCCESS(Status))
            return Status;
    }
    
    Status = ObReferenceObjectByHandle(FirstTokenHandle,
                                       TOKEN_QUERY,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&FirstToken,
                                       NULL);
    if (!NT_SUCCESS(Status))
        return Status;
    
    Status = ObReferenceObjectByHandle(SecondTokenHandle,
                                       TOKEN_QUERY,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&SecondToken,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(FirstToken);
        return Status;
    }
    
    if (FirstToken != SecondToken)
    {
        Status = SepCompareTokens(FirstToken,
                                  SecondToken,
                                  &IsEqual);
    }
    else
        IsEqual = TRUE;
    
    ObDereferenceObject(FirstToken);
    ObDereferenceObject(SecondToken);
    
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            *Equal = IsEqual;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    
    return Status;
}

NTSTATUS
NTAPI
NtFilterToken(IN HANDLE ExistingTokenHandle,
              IN ULONG Flags,
              IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
              IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
              IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
              OUT PHANDLE NewTokenHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtImpersonateAnonymousToken(IN HANDLE Thread)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
