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

#include <ntlsa.h>

typedef struct _TOKEN_AUDIT_POLICY_INFORMATION
{
    ULONG PolicyCount;
    struct
    {
        ULONG Category;
        UCHAR Value;
    } Policies[1];
} TOKEN_AUDIT_POLICY_INFORMATION, *PTOKEN_AUDIT_POLICY_INFORMATION;

/* GLOBALS ********************************************************************/

POBJECT_TYPE SeTokenObjectType = NULL;
ERESOURCE SepTokenLock;

TOKEN_SOURCE SeSystemTokenSource = {"*SYSTEM*", {0}};
LUID SeSystemAuthenticationId = SYSTEM_LUID;
LUID SeAnonymousAuthenticationId = ANONYMOUS_LOGON_LUID;

static GENERIC_MAPPING SepTokenMapping = {
    TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_EXECUTE,
    TOKEN_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO SeTokenInformationClass[] = {

    /* Class 0 not used, blame M$! */
    ICI_SQ_SAME( 0, 0, 0),

    /* TokenUser */
    ICI_SQ_SAME( sizeof(TOKEN_USER),                   sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE ),
    /* TokenGroups */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE ),
    /* TokenPrivileges */
    ICI_SQ_SAME( sizeof(TOKEN_PRIVILEGES),             sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE ),
    /* TokenOwner */
    ICI_SQ_SAME( sizeof(TOKEN_OWNER),                  sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenPrimaryGroup */
    ICI_SQ_SAME( sizeof(TOKEN_PRIMARY_GROUP),          sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenDefaultDacl */
    ICI_SQ_SAME( sizeof(TOKEN_DEFAULT_DACL),           sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE ),
    /* TokenSource */
    ICI_SQ_SAME( sizeof(TOKEN_SOURCE),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE ),
    /* TokenType */
    ICI_SQ_SAME( sizeof(TOKEN_TYPE),                   sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenImpersonationLevel */
    ICI_SQ_SAME( sizeof(SECURITY_IMPERSONATION_LEVEL), sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenStatistics */
    ICI_SQ_SAME( sizeof(TOKEN_STATISTICS),             sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE ),
    /* TokenRestrictedSids */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS),                 sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSessionId */
    ICI_SQ_SAME( sizeof(ULONG),                        sizeof(ULONG), ICIF_QUERY | ICIF_SET ),
    /* TokenGroupsAndPrivileges */
    ICI_SQ_SAME( sizeof(TOKEN_GROUPS_AND_PRIVILEGES),  sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSessionReference */
    ICI_SQ_SAME( sizeof(ULONG),                        sizeof(ULONG), ICIF_SET | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenSandBoxInert */
    ICI_SQ_SAME( sizeof(ULONG),                        sizeof(ULONG), ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenAuditPolicy */
    ICI_SQ_SAME( /* FIXME */0,                         sizeof(ULONG), ICIF_QUERY | ICIF_SET | ICIF_QUERY_SIZE_VARIABLE ),
    /* TokenOrigin */
    ICI_SQ_SAME( sizeof(TOKEN_ORIGIN),                 sizeof(ULONG), ICIF_QUERY | ICIF_SET | ICIF_QUERY_SIZE_VARIABLE ),
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
        DPRINT1("FIXME: Pretending tokens are equal!\n");
        IsEqual = TRUE;
    }

    *Equal = IsEqual;
    return STATUS_SUCCESS;
}

static
VOID
SepUpdateSinglePrivilegeFlagToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index)
{
    ULONG TokenFlag;
    NT_ASSERT(Index < Token->PrivilegeCount);

    /* The high part of all values we are interested in is 0 */
    if (Token->Privileges[Index].Luid.HighPart != 0)
    {
        return;
    }

    /* Check for certain privileges to update flags */
    if (Token->Privileges[Index].Luid.LowPart == SE_CHANGE_NOTIFY_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_TRAVERSE_PRIVILEGE;
    }
    else if (Token->Privileges[Index].Luid.LowPart == SE_BACKUP_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_BACKUP_PRIVILEGE;
    }
    else if (Token->Privileges[Index].Luid.LowPart == SE_RESTORE_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_RESTORE_PRIVILEGE;
    }
    else if (Token->Privileges[Index].Luid.LowPart == SE_IMPERSONATE_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_IMPERSONATE_PRIVILEGE;
    }
    else
    {
        /* Nothing to do */
        return;
    }

    /* Check if the specified privilege is enabled */
    if (Token->Privileges[Index].Attributes & SE_PRIVILEGE_ENABLED)
    {
        /* It is enabled, so set the flag */
        Token->TokenFlags |= TokenFlag;
    }
    else
    {
        /* Is is disabled, so remove the flag */
        Token->TokenFlags &= ~TokenFlag;
    }
}

static
VOID
SepUpdatePrivilegeFlagsToken(
    _Inout_ PTOKEN Token)
{
    ULONG i;

    /* Loop all privileges */
    for (i = 0; i < Token->PrivilegeCount; i++)
    {
        /* Updates the flags dor this privilege */
        SepUpdateSinglePrivilegeFlagToken(Token, i);
    }
}

static
VOID
SepRemovePrivilegeToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index)
{
    ULONG MoveCount;
    NT_ASSERT(Index < Token->PrivilegeCount);

    /* Calculate the number of trailing privileges */
    MoveCount = Token->PrivilegeCount - Index - 1;
    if (MoveCount != 0)
    {
        /* Move them one location ahead */
        RtlMoveMemory(&Token->Privileges[Index],
                      &Token->Privileges[Index + 1],
                      MoveCount * sizeof(LUID_AND_ATTRIBUTES));
    }

    /* Update privilege count */
    Token->PrivilegeCount--;
}

VOID
NTAPI
SepFreeProxyData(PVOID ProxyData)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
SepCopyProxyData(PVOID* Dest,
                 PVOID Src)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
    if (NewToken->TokenInUse)
    {
        BOOLEAN IsEqual;
        NTSTATUS Status;

        /* Maybe we're trying to set the same token */
        OldToken = PsReferencePrimaryToken(Process);
        if (OldToken == NewToken)
        {
            /* So it's a nop. */
            *OldTokenP = OldToken;
            return STATUS_SUCCESS;
        }

        Status = SepCompareTokens(OldToken, NewToken, &IsEqual);
        if (!NT_SUCCESS(Status))
        {
            *OldTokenP = NULL;
            PsDereferencePrimaryToken(OldToken);
            return Status;
        }

        if (!IsEqual)
        {
            *OldTokenP = NULL;
            PsDereferencePrimaryToken(OldToken);
            return STATUS_TOKEN_ALREADY_IN_USE;
        }
        /* Silently return STATUS_SUCCESS but do not set the new token,
         * as it's already in use elsewhere. */
        *OldTokenP = OldToken;
        return STATUS_SUCCESS;
    }

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

    return uLength;
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

    return STATUS_SUCCESS;
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
    PTOKEN AccessToken = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    Status = ObCreateObject(PreviousMode,
                            SeTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(TOKEN),
                            0,
                            0,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObject() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Zero out the buffer */
    RtlZeroMemory(AccessToken, sizeof(TOKEN));

    Status = ZwAllocateLocallyUniqueId(&AccessToken->TokenId);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(AccessToken);
        return Status;
    }

    AccessToken->TokenLock = &SepTokenLock;

    AccessToken->TokenType  = TokenType;
    AccessToken->ImpersonationLevel = Level;
    RtlCopyLuid(&AccessToken->AuthenticationId, &Token->AuthenticationId);
    RtlCopyLuid(&AccessToken->ModifiedId, &Token->ModifiedId);

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

    AccessToken->UserAndGroups = ExAllocatePoolWithTag(PagedPool,
                                                       uLength,
                                                       TAG_TOKEN_USERS);
    if (AccessToken->UserAndGroups == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];

    Status = RtlCopySidAndAttributesArray(AccessToken->UserAndGroupCount,
                                          Token->UserAndGroups,
                                          uLength,
                                          AccessToken->UserAndGroups,
                                          EndMem,
                                          &EndMem,
                                          &uLength);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SepFindPrimaryGroupAndDefaultOwner(AccessToken,
                                                Token->PrimaryGroup,
                                                0);
    if (!NT_SUCCESS(Status))
        goto done;

    AccessToken->PrivilegeCount = Token->PrivilegeCount;

    uLength = AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
    AccessToken->Privileges = ExAllocatePoolWithTag(PagedPool,
                                                    uLength,
                                                    TAG_TOKEN_PRIVILAGES);
    if (AccessToken->Privileges == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    for (i = 0; i < AccessToken->PrivilegeCount; i++)
    {
        RtlCopyLuid(&AccessToken->Privileges[i].Luid,
                    &Token->Privileges[i].Luid);
        AccessToken->Privileges[i].Attributes =
        Token->Privileges[i].Attributes;
    }

    if (Token->DefaultDacl)
    {
        AccessToken->DefaultDacl = ExAllocatePoolWithTag(PagedPool,
                                                         Token->DefaultDacl->AclSize,
                                                         TAG_TOKEN_ACL);
        if (AccessToken->DefaultDacl == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        memcpy(AccessToken->DefaultDacl,
               Token->DefaultDacl,
               Token->DefaultDacl->AclSize);
    }

    *NewAccessToken = AccessToken;

done:
    if (!NT_SUCCESS(Status))
    {
        if (AccessToken)
        {
            if (AccessToken->UserAndGroups)
                ExFreePoolWithTag(AccessToken->UserAndGroups, TAG_TOKEN_USERS);

            if (AccessToken->Privileges)
                ExFreePoolWithTag(AccessToken->Privileges, TAG_TOKEN_PRIVILAGES);

            if (AccessToken->DefaultDacl)
                ExFreePoolWithTag(AccessToken->DefaultDacl, TAG_TOKEN_ACL);

            ObDereferenceObject(AccessToken);
        }
    }

    return Status;
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
    ProcessLuid = ProcessToken->AuthenticationId;

    /* Dereference the token */
    ObFastDereferenceObject(&PsGetCurrentProcess()->Token, ProcessToken);

    /* Get our LUID */
    CallerLuid = Token->AuthenticationId;

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

    return Status;
}

VOID
NTAPI
SepDeleteToken(PVOID ObjectBody)
{
    PTOKEN AccessToken = (PTOKEN)ObjectBody;

    if (AccessToken->UserAndGroups)
        ExFreePoolWithTag(AccessToken->UserAndGroups, TAG_TOKEN_USERS);

    if (AccessToken->Privileges)
        ExFreePoolWithTag(AccessToken->Privileges, TAG_TOKEN_PRIVILAGES);

    if (AccessToken->DefaultDacl)
        ExFreePoolWithTag(AccessToken->DefaultDacl, TAG_TOKEN_ACL);
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

    /* Initialize the Token type */
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
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &SeTokenObjectType);
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


NTSTATUS
NTAPI
SepCreateToken(OUT PHANDLE TokenHandle,
               IN KPROCESSOR_MODE PreviousMode,
               IN ACCESS_MASK DesiredAccess,
               IN POBJECT_ATTRIBUTES ObjectAttributes,
               IN TOKEN_TYPE TokenType,
               IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
               IN PLUID AuthenticationId,
               IN PLARGE_INTEGER ExpirationTime,
               IN PSID_AND_ATTRIBUTES User,
               IN ULONG GroupCount,
               IN PSID_AND_ATTRIBUTES Groups,
               IN ULONG GroupLength,
               IN ULONG PrivilegeCount,
               IN PLUID_AND_ATTRIBUTES Privileges,
               IN PSID Owner,
               IN PSID PrimaryGroup,
               IN PACL DefaultDacl,
               IN PTOKEN_SOURCE TokenSource,
               IN BOOLEAN SystemToken)
{
    PTOKEN AccessToken;
    LUID TokenId;
    LUID ModifiedId;
    PVOID EndMem;
    ULONG uLength;
    ULONG i;
    NTSTATUS Status;
    ULONG TokenFlags = 0;

    /* Loop all groups */
    for (i = 0; i < GroupCount; i++)
    {
        /* Check for mandatory groups */
        if (Groups[i].Attributes & SE_GROUP_MANDATORY)
        {
            /* Force them to be enabled */
            Groups[i].Attributes |= (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT);
        }

        /* Check of the group is an admin group */
        if (RtlEqualSid(SeAliasAdminsSid, Groups[i].Sid))
        {
            /* Remember this so we can optimize queries later */
            TokenFlags |= TOKEN_HAS_ADMIN_GROUP;
        }
    }

    Status = ZwAllocateLocallyUniqueId(&TokenId);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ZwAllocateLocallyUniqueId(&ModifiedId);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ObCreateObject(PreviousMode,
                            SeTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(TOKEN),
                            0,
                            0,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObject() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Zero out the buffer */
    RtlZeroMemory(AccessToken, sizeof(TOKEN));

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

    AccessToken->UserAndGroupCount = GroupCount + 1;
    AccessToken->PrivilegeCount = PrivilegeCount;

    AccessToken->TokenFlags = TokenFlags;
    AccessToken->TokenType = TokenType;
    AccessToken->ImpersonationLevel = ImpersonationLevel;

    /*
     * Normally we would just point these members into the variable information
     * area; however, our ObCreateObject() call can't allocate a variable information
     * area, so we allocate them seperately and provide a destroy function.
     */

    uLength = sizeof(SID_AND_ATTRIBUTES) * AccessToken->UserAndGroupCount;
    uLength += RtlLengthSid(User->Sid);
    for (i = 0; i < GroupCount; i++)
        uLength += RtlLengthSid(Groups[i].Sid);

    // FIXME: should use the object itself
    AccessToken->UserAndGroups = ExAllocatePoolWithTag(PagedPool,
                                                       uLength,
                                                       TAG_TOKEN_USERS);
    if (AccessToken->UserAndGroups == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];

    Status = RtlCopySidAndAttributesArray(1,
                                          User,
                                          uLength,
                                          AccessToken->UserAndGroups,
                                          EndMem,
                                          &EndMem,
                                          &uLength);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = RtlCopySidAndAttributesArray(GroupCount,
                                          Groups,
                                          uLength,
                                          &AccessToken->UserAndGroups[1],
                                          EndMem,
                                          &EndMem,
                                          &uLength);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SepFindPrimaryGroupAndDefaultOwner(AccessToken,
                                                PrimaryGroup,
                                                Owner);
    if (!NT_SUCCESS(Status))
        goto done;

    // FIXME: should use the object itself
    uLength = PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
    if (uLength == 0) uLength = sizeof(PVOID);
    AccessToken->Privileges = ExAllocatePoolWithTag(PagedPool,
                                                    uLength,
                                                    TAG_TOKEN_PRIVILAGES);
    if (AccessToken->Privileges == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            RtlCopyMemory(AccessToken->Privileges,
                          Privileges,
                          PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
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
                      Privileges,
                      PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
    }

    if (!NT_SUCCESS(Status))
        goto done;

    /* Update privilege flags */
    SepUpdatePrivilegeFlagsToken(AccessToken);

    if (DefaultDacl != NULL)
    {
        // FIXME: should use the object itself
        AccessToken->DefaultDacl = ExAllocatePoolWithTag(PagedPool,
                                                         DefaultDacl->AclSize,
                                                         TAG_TOKEN_ACL);
        if (AccessToken->DefaultDacl == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        RtlCopyMemory(AccessToken->DefaultDacl,
                      DefaultDacl,
                      DefaultDacl->AclSize);
    }
    else
    {
        AccessToken->DefaultDacl = NULL;
    }

    if (!SystemToken)
    {
        Status = ObInsertObject((PVOID)AccessToken,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                TokenHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ObInsertObject() failed (Status %lx)\n", Status);
        }
    }
    else
    {
        /* Return pointer instead of handle */
        *TokenHandle = (HANDLE)AccessToken;
    }

done:
    if (!NT_SUCCESS(Status))
    {
        if (AccessToken)
        {
            /* Dereference the token, the delete procedure will clean up */
            ObDereferenceObject(AccessToken);
        }
    }

    return Status;
}

PTOKEN
NTAPI
SepCreateSystemProcessToken(VOID)
{
    LUID_AND_ATTRIBUTES Privileges[25];
    ULONG GroupAttributes, OwnerAttributes;
    SID_AND_ATTRIBUTES Groups[32];
    LARGE_INTEGER Expiration;
    SID_AND_ATTRIBUTES UserSid;
    ULONG GroupLength;
    PSID PrimaryGroup;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSID Owner;
    ULONG i;
    PTOKEN Token;
    NTSTATUS Status;

    /* Don't ever expire */
    Expiration.QuadPart = -1;

    /* All groups mandatory and enabled */
    GroupAttributes = SE_GROUP_ENABLED | SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT;
    OwnerAttributes = SE_GROUP_ENABLED | SE_GROUP_OWNER | SE_GROUP_ENABLED_BY_DEFAULT;

    /* User is system */
    UserSid.Sid = SeLocalSystemSid;
    UserSid.Attributes = 0;

    /* Primary group is local system */
    PrimaryGroup = SeLocalSystemSid;

    /* Owner is admins */
    Owner = SeAliasAdminsSid;

    /* Groups are admins, world, and authenticated users */
    Groups[0].Sid = SeAliasAdminsSid;
    Groups[0].Attributes = OwnerAttributes;
    Groups[1].Sid = SeWorldSid;
    Groups[1].Attributes = GroupAttributes;
    Groups[2].Sid = SeAuthenticatedUserSid;
    Groups[2].Attributes = OwnerAttributes;
    GroupLength = sizeof(SID_AND_ATTRIBUTES) +
                  SeLengthSid(Groups[0].Sid) +
                  SeLengthSid(Groups[1].Sid) +
                  SeLengthSid(Groups[2].Sid);
    ASSERT(GroupLength <= sizeof(Groups));

    /* Setup the privileges */
    i = 0;
    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeTcbPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeCreateTokenPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeTakeOwnershipPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeCreatePagefilePrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeLockMemoryPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeAssignPrimaryTokenPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeIncreaseQuotaPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeIncreaseBasePriorityPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeCreatePermanentPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeDebugPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeAuditPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeSecurityPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeSystemEnvironmentPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeChangeNotifyPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeBackupPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeRestorePrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeShutdownPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeLoadDriverPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeProfileSingleProcessPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeSystemtimePrivilege;
    ASSERT(i == 20);

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    ASSERT(SeSystemDefaultDacl != NULL);

    /* Create the token */
    Status = SepCreateToken((PHANDLE)&Token,
                            KernelMode,
                            0,
                            &ObjectAttributes,
                            TokenPrimary,
                            0,
                            &SeSystemAuthenticationId,
                            &Expiration,
                            &UserSid,
                            3,
                            Groups,
                            GroupLength,
                            20,
                            Privileges,
                            Owner,
                            PrimaryGroup,
                            SeSystemDefaultDacl,
                            &SeSystemTokenSource,
                            TRUE);
    ASSERT(Status == STATUS_SUCCESS);

    /* Return the token */
    return Token;
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
    NTSTATUS Status;
    PSECURITY_IMPERSONATION_LEVEL SeImpersonationLvl;
    PAGED_CODE();

    switch (TokenInformationClass)
    {
        case TokenImpersonationLevel:
            /* It is mandatory to have an impersonation token */
            if (((PTOKEN)Token)->TokenType != TokenImpersonation)
            {
                Status = STATUS_INVALID_INFO_CLASS;
                break;
            }

            /* Allocate the output buffer */
            SeImpersonationLvl = ExAllocatePoolWithTag(PagedPool, sizeof(SECURITY_IMPERSONATION_LEVEL), TAG_SE);
            if (SeImpersonationLvl == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            /* Set impersonation level and return the structure */
            *SeImpersonationLvl = ((PTOKEN)Token)->ImpersonationLevel;
            *TokenInformation = SeImpersonationLvl;
            Status = STATUS_SUCCESS;
            break;

        default:
            UNIMPLEMENTED;
            break;
    }

    return Status;
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
NTSTATUS
NTAPI
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
    NTSTATUS Status;

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
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(TokenHandle,
                                       (TokenInformationClass == TokenSource) ? TOKEN_QUERY_SOURCE : TOKEN_QUERY,
                                       SeTokenObjectType,
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
                    if (TokenInformationLength >= RequiredLength)
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

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
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

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
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

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
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

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
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

                    if (ReturnLength != NULL)
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

                if (Token->DefaultDacl != NULL)
                {
                    RequiredLength += Token->DefaultDacl->AclSize;
                }

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        if (Token->DefaultDacl != NULL)
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

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *ts = Token->TokenSource;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *tt = Token->TokenType;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
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

                /* Fail if the token is not an impersonation token */
                if (Token->TokenType != TokenImpersonation)
                {
                    Status = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                RequiredLength = sizeof(SECURITY_IMPERSONATION_LEVEL);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *sil = Token->ImpersonationLevel;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
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

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
                    {
                        RtlCopyLuid(&to->OriginatingLogonSession,
                                    &Token->AuthenticationId);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
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
                    if (TokenInformationLength >= RequiredLength)
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

                    if (ReturnLength != NULL)
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

                if (NT_SUCCESS(Status))
                {
                    _SEH2_TRY
                    {
                        /* buffer size was already verified, no need to check here again */
                        *(PULONG)TokenInformation = SessionId;

                        if (ReturnLength != NULL)
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

    return Status;
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
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    Status = DefaultSetInfoBufferCheck(TokenInformationClass,
                                       SeTokenInformationClass,
                                       sizeof(SeTokenInformationClass) / sizeof(SeTokenInformationClass[0]),
                                       TokenInformation,
                                       TokenInformationLength,
                                       PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        /* Invalid buffers */
        DPRINT("NtSetInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }

    if (TokenInformationClass == TokenSessionId)
    {
        NeededAccess |= TOKEN_ADJUST_SESSIONID;
    }

    Status = ObReferenceObjectByHandle(TokenHandle,
                                       NeededAccess,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        switch (TokenInformationClass)
        {
            case TokenOwner:
            {
                if (TokenInformationLength >= sizeof(TOKEN_OWNER))
                {
                    PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
                    PSID InputSid = NULL, CapturedSid;

                    _SEH2_TRY
                    {
                        InputSid = to->Owner;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        goto Cleanup;
                    }
                    _SEH2_END;

                    Status = SepCaptureSid(InputSid,
                                           PreviousMode,
                                           PagedPool,
                                           FALSE,
                                           &CapturedSid);
                    if (NT_SUCCESS(Status))
                    {
                        RtlCopySid(RtlLengthSid(CapturedSid),
                                   Token->UserAndGroups[Token->DefaultOwnerIndex].Sid,
                                   CapturedSid);
                        SepReleaseSid(CapturedSid,
                                      PreviousMode,
                                      FALSE);
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
                if (TokenInformationLength >= sizeof(TOKEN_PRIMARY_GROUP))
                {
                    PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
                    PSID InputSid = NULL, CapturedSid;

                    _SEH2_TRY
                    {
                        InputSid = tpg->PrimaryGroup;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        goto Cleanup;
                    }
                    _SEH2_END;

                    Status = SepCaptureSid(InputSid,
                                           PreviousMode,
                                           PagedPool,
                                           FALSE,
                                           &CapturedSid);
                    if (NT_SUCCESS(Status))
                    {
                        RtlCopySid(RtlLengthSid(CapturedSid),
                                   Token->PrimaryGroup,
                                   CapturedSid);
                        SepReleaseSid(CapturedSid,
                                      PreviousMode,
                                      FALSE);
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
                if (TokenInformationLength >= sizeof(TOKEN_DEFAULT_DACL))
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
                        goto Cleanup;
                    }
                    _SEH2_END;

                    if (InputAcl != NULL)
                    {
                        PACL CapturedAcl;

                        /* Capture and copy the dacl */
                        Status = SepCaptureAcl(InputAcl,
                                               PreviousMode,
                                               PagedPool,
                                               TRUE,
                                               &CapturedAcl);
                        if (NT_SUCCESS(Status))
                        {
                            /* Free the previous dacl if present */
                            if(Token->DefaultDacl != NULL)
                            {
                                ExFreePoolWithTag(Token->DefaultDacl, TAG_TOKEN_ACL);
                            }

                            Token->DefaultDacl = ExAllocatePoolWithTag(PagedPool,
                                                                       CapturedAcl->AclSize,
                                                                       TAG_TOKEN_ACL);
                            if (!Token->DefaultDacl)
                            {
                                ExFreePoolWithTag(CapturedAcl, TAG_ACL);
                                Status = STATUS_NO_MEMORY;
                            }
                            else
                            {
                                /* Set the new dacl */
                                RtlCopyMemory(Token->DefaultDacl, CapturedAcl, CapturedAcl->AclSize);
                                ExFreePoolWithTag(CapturedAcl, TAG_ACL);
                            }
                        }
                    }
                    else
                    {
                        /* Clear and free the default dacl if present */
                        if (Token->DefaultDacl != NULL)
                        {
                            ExFreePoolWithTag(Token->DefaultDacl, TAG_TOKEN_ACL);
                            Token->DefaultDacl = NULL;
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
                    /* Buffer size was already verified, no need to check here again */
                    SessionId = *(PULONG)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    goto Cleanup;
                }
                _SEH2_END;

                if (!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                            PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                Token->SessionId = SessionId;
                break;
            }

            case TokenSessionReference:
            {
                ULONG SessionReference;

                _SEH2_TRY
                {
                    /* Buffer size was already verified, no need to check here again */
                    SessionReference = *(PULONG)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    goto Cleanup;
                }
                _SEH2_END;

                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    goto Cleanup;
                }

                /* Check if it is 0 */
                if (SessionReference == 0)
                {
                    /* Atomically set the flag in the token */
                    RtlInterlockedSetBits(&Token->TokenFlags,
                                          TOKEN_SESSION_NOT_REFERENCED);
                }

                break;

            }


            case TokenAuditPolicy:
            {
                PTOKEN_AUDIT_POLICY_INFORMATION PolicyInformation =
                    (PTOKEN_AUDIT_POLICY_INFORMATION)TokenInformation;
                SEP_AUDIT_POLICY AuditPolicy;
                ULONG i;

                _SEH2_TRY
                {
                    ProbeForRead(PolicyInformation,
                                 FIELD_OFFSET(TOKEN_AUDIT_POLICY_INFORMATION,
                                              Policies[PolicyInformation->PolicyCount]),
                                 sizeof(ULONG));

                    /* Loop all policies in the structure */
                    for (i = 0; i < PolicyInformation->PolicyCount; i++)
                    {
                        /* Set the corresponding bits in the packed structure */
                        switch (PolicyInformation->Policies[i].Category)
                        {
                            case AuditCategorySystem:
                                AuditPolicy.PolicyElements.System = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryLogon:
                                AuditPolicy.PolicyElements.Logon = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryObjectAccess:
                                AuditPolicy.PolicyElements.ObjectAccess = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryPrivilegeUse:
                                AuditPolicy.PolicyElements.PrivilegeUse = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryDetailedTracking:
                                AuditPolicy.PolicyElements.DetailedTracking = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryPolicyChange:
                                AuditPolicy.PolicyElements.PolicyChange = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryAccountManagement:
                                AuditPolicy.PolicyElements.AccountManagement = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryDirectoryServiceAccess:
                                AuditPolicy.PolicyElements.DirectoryServiceAccess = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryAccountLogon:
                                AuditPolicy.PolicyElements.AccountLogon = PolicyInformation->Policies[i].Value;
                                break;
                        }
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    goto Cleanup;
                }
                _SEH2_END;

                if (!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                            PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                /* Set the new audit policy */
                Token->AuditPolicy = AuditPolicy;

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            case TokenOrigin:
            {
                TOKEN_ORIGIN TokenOrigin;

                _SEH2_TRY
                {
                    /* Copy the token origin */
                    TokenOrigin = *(PTOKEN_ORIGIN)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    goto Cleanup;
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                /* Check if there is no token origin set yet */
                if ((Token->OriginatingLogonSession.LowPart == 0) &&
                    (Token->OriginatingLogonSession.HighPart == 0))
                {
                    /* Set the token origin */
                    Token->OriginatingLogonSession =
                        TokenOrigin.OriginatingLogonSession;
                }

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            default:
            {
                DPRINT1("Invalid TokenInformationClass: 0x%lx\n",
                        TokenInformationClass);
                Status = STATUS_INVALID_INFO_CLASS;
                break;
            }
        }
Cleanup:
        ObDereferenceObject(Token);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationToken failed with Status 0x%lx\n", Status);
    }

    return Status;
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
    OBJECT_HANDLE_INFORMATION HandleInformation;
    NTSTATUS Status;

    PAGED_CODE();

    if (TokenType != TokenImpersonation &&
        TokenType != TokenPrimary)
        return STATUS_INVALID_PARAMETER;

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(NewTokenHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    Status = SepCaptureSecurityQualityOfService(ObjectAttributes,
                                                PreviousMode,
                                                PagedPool,
                                                FALSE,
                                                &CapturedSecurityQualityOfService,
                                                &QoSPresent);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateToken() failed to capture QoS! Status: 0x%x\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(ExistingTokenHandle,
                                       TOKEN_DUPLICATE,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status))
    {
        SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                           PreviousMode,
                                           FALSE);
        return Status;
    }

    /*
     * Fail, if the original token is an impersonation token and the caller
     * tries to raise the impersonation level of the new token above the
     * impersonation level of the original token.
     */
    if (Token->TokenType == TokenImpersonation)
    {
        if (QoSPresent &&
            CapturedSecurityQualityOfService->ImpersonationLevel >Token->ImpersonationLevel)
        {
            ObDereferenceObject(Token);
            SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                               PreviousMode,
                                               FALSE);
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }
    }

    /*
     * Fail, if a primary token is to be created from an impersonation token
     * and and the impersonation level of the impersonation token is below SecurityImpersonation.
     */
    if (Token->TokenType == TokenImpersonation &&
        TokenType == TokenPrimary &&
        Token->ImpersonationLevel < SecurityImpersonation)
    {
        ObDereferenceObject(Token);
        SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                           PreviousMode,
                                           FALSE);
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

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
                                (DesiredAccess ? DesiredAccess : HandleInformation.GrantedAccess),
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

    /* Free the captured structure */
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


static
ULONG
SepAdjustPrivileges(
    _Inout_ PTOKEN Token,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PLUID_AND_ATTRIBUTES NewState,
    _In_ ULONG NewStateCount,
    _Out_opt_ PTOKEN_PRIVILEGES PreviousState,
    _In_ BOOLEAN ApplyChanges,
    _Out_ PULONG ChangedPrivileges)
{
    ULONG i, j, PrivilegeCount, ChangeCount, NewAttributes;

    /* Count the found privileges and those that need to be changed */
    PrivilegeCount = 0;
    ChangeCount = 0;

    /* Loop all privileges in the token */
    for (i = 0; i < Token->PrivilegeCount; i++)
    {
        /* Shall all of them be disabled? */
        if (DisableAllPrivileges)
        {
            /* The new attributes are the old ones, but disabled */
            NewAttributes = Token->Privileges[i].Attributes & ~SE_PRIVILEGE_ENABLED;
        }
        else
        {
            /* Otherwise loop all provided privileges */
            for (j = 0; j < NewStateCount; j++)
            {
                /* Check if this is the LUID we are looking for */
                if (RtlEqualLuid(&Token->Privileges[i].Luid, &NewState[j].Luid))
                {
                    DPRINT("Found privilege\n");

                    /* Copy SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_REMOVED */
                    NewAttributes = NewState[j].Attributes;
                    NewAttributes &= (SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_REMOVED);
                    NewAttributes |= Token->Privileges[i].Attributes & ~SE_PRIVILEGE_ENABLED;

                    /* Stop looking */
                    break;
                }
            }

            /* Check if we didn't find the privilege */
            if (j == NewStateCount)
            {
                /* Continue with the token's next privilege */
                continue;
            }
        }

        /* We found a privilege, count it */
        PrivilegeCount++;

        /* Does the privilege need to be changed? */
        if (Token->Privileges[i].Attributes != NewAttributes)
        {
            /* Does the caller want the old privileges? */
            if (PreviousState != NULL)
            {
                /* Copy the old privilege */
                PreviousState->Privileges[ChangeCount] = Token->Privileges[i];
            }

            /* Does the caller want to apply the changes? */
            if (ApplyChanges)
            {
                /* Shall we remove the privilege? */
                if (NewAttributes & SE_PRIVILEGE_REMOVED)
                {
                    /* Set the token as disabled and update flags for it */
                    Token->Privileges[i].Attributes &= ~SE_PRIVILEGE_ENABLED;
                    SepUpdateSinglePrivilegeFlagToken(Token, i);

                    /* Remove the privilege */
                    SepRemovePrivilegeToken(Token, i);

                    /* Fix the running index */
                    i--;

                    /* Continue with next */
                    continue;
                }

                /* Set the new attributes and update flags */
                Token->Privileges[i].Attributes = NewAttributes;
                SepUpdateSinglePrivilegeFlagToken(Token, i);
            }

            /* Increment the change count */
            ChangeCount++;
        }
    }

    /* Set the number of saved privileges */
    if (PreviousState != NULL)
        PreviousState->PrivilegeCount = ChangeCount;

    /* Return the number of changed privileges */
    *ChangedPrivileges = ChangeCount;

    /* Check if we missed some */
    if (!DisableAllPrivileges && (PrivilegeCount < NewStateCount))
    {
        return STATUS_NOT_ALL_ASSIGNED;
    }

    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_to_opt_(BufferLength,*ReturnLength)
        PTOKEN_PRIVILEGES PreviousState,
    _When_(PreviousState!=NULL, _Out_) PULONG ReturnLength)
{
    PLUID_AND_ATTRIBUTES CapturedPrivileges = NULL;
    KPROCESSOR_MODE PreviousMode;
    ULONG CapturedCount = 0;
    ULONG CapturedLength = 0;
    ULONG NewStateSize = 0;
    ULONG ChangeCount;
    ULONG RequiredLength;
    PTOKEN Token;
    NTSTATUS Status;
    PAGED_CODE();

    DPRINT("NtAdjustPrivilegesToken() called\n");

    /* Fail, if we do not disable all privileges but NewState is NULL */
    if (DisableAllPrivileges == FALSE && NewState == NULL)
        return STATUS_INVALID_PARAMETER;

    PreviousMode = KeGetPreviousMode ();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe NewState */
            if (DisableAllPrivileges == FALSE)
            {
                /* First probe the header */
                ProbeForRead(NewState, sizeof(TOKEN_PRIVILEGES), sizeof(ULONG));

                CapturedCount = NewState->PrivilegeCount;
                NewStateSize = FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges[CapturedCount]);

                ProbeForRead(NewState, NewStateSize, sizeof(ULONG));
            }

            /* Probe PreviousState and ReturnLength */
            if (PreviousState != NULL)
            {
                ProbeForWrite(PreviousState, BufferLength, sizeof(ULONG));
                ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* This is kernel mode, we trust the caller */
        if (DisableAllPrivileges == FALSE)
            CapturedCount = NewState->PrivilegeCount;
    }

    /* Do we need to capture the new state? */
    if (DisableAllPrivileges == FALSE)
    {
        _SEH2_TRY
        {
            /* Capture the new state array of privileges */
            Status = SeCaptureLuidAndAttributesArray(NewState->Privileges,
                                                     CapturedCount,
                                                     PreviousMode,
                                                     NULL,
                                                     0,
                                                     PagedPool,
                                                     TRUE,
                                                     &CapturedPrivileges,
                                                     &CapturedLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* Reference the token */
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       TOKEN_ADJUST_PRIVILEGES | (PreviousState != NULL ? TOKEN_QUERY : 0),
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference token (Status %lx)\n", Status);

        /* Release the captured privileges */
        if (CapturedPrivileges != NULL)
            SeReleaseLuidAndAttributesArray(CapturedPrivileges,
                                            PreviousMode,
                                            TRUE);

        return Status;
    }

    /* Lock the token */
    ExEnterCriticalRegionAndAcquireResourceExclusive(Token->TokenLock);

    /* Count the privileges that need to be changed, do not apply them yet */
    Status = SepAdjustPrivileges(Token,
                                 DisableAllPrivileges,
                                 CapturedPrivileges,
                                 CapturedCount,
                                 NULL,
                                 FALSE,
                                 &ChangeCount);

    /* Check if the caller asked for the previous state */
    if (PreviousState != NULL)
    {
        /* Calculate the required length */
        RequiredLength = FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges[ChangeCount]);

        /* Try to return the required buffer length */
        _SEH2_TRY
        {
            *ReturnLength = RequiredLength;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do cleanup and return the exception code */
            Status = _SEH2_GetExceptionCode();
            goto Cleanup;
        }
        _SEH2_END;

        /* Fail, if the buffer length is smaller than the required length */
        if (BufferLength < RequiredLength)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
    }

    /* Now enter SEH, since we might return the old privileges */
    _SEH2_TRY
    {
        /* This time apply the changes */
        Status = SepAdjustPrivileges(Token,
                                     DisableAllPrivileges,
                                     CapturedPrivileges,
                                     CapturedCount,
                                     PreviousState,
                                     TRUE,
                                     &ChangeCount);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do cleanup and return the exception code */
        Status = _SEH2_GetExceptionCode();
        goto Cleanup;
    }
    _SEH2_END;

Cleanup:
    /* Unlock and dereference the token */
    ExReleaseResourceAndLeaveCriticalRegion(Token->TokenLock);
    ObDereferenceObject(Token);

    /* Release the captured privileges */
    if (CapturedPrivileges != NULL)
        SeReleaseLuidAndAttributesArray(CapturedPrivileges,
                                        PreviousMode,
                                        TRUE);

    DPRINT ("NtAdjustPrivilegesToken() done\n");
    return Status;
}

NTSTATUS
NTAPI
NtCreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TOKEN_TYPE TokenType,
    _In_ PLUID AuthenticationId,
    _In_ PLARGE_INTEGER ExpirationTime,
    _In_ PTOKEN_USER TokenUser,
    _In_ PTOKEN_GROUPS TokenGroups,
    _In_ PTOKEN_PRIVILEGES TokenPrivileges,
    _In_opt_ PTOKEN_OWNER TokenOwner,
    _In_ PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
    _In_opt_ PTOKEN_DEFAULT_DACL TokenDefaultDacl,
    _In_ PTOKEN_SOURCE TokenSource)
{
    HANDLE hToken;
    KPROCESSOR_MODE PreviousMode;
    ULONG PrivilegeCount, GroupCount;
    PSID OwnerSid, PrimaryGroupSid;
    PACL DefaultDacl;
    LARGE_INTEGER LocalExpirationTime = {{0, 0}};
    LUID LocalAuthenticationId;
    TOKEN_SOURCE LocalTokenSource;
    SECURITY_QUALITY_OF_SERVICE LocalSecurityQos;
    PLUID_AND_ATTRIBUTES CapturedPrivileges = NULL;
    PSID_AND_ATTRIBUTES CapturedUser = NULL;
    PSID_AND_ATTRIBUTES CapturedGroups = NULL;
    PSID CapturedOwnerSid = NULL;
    PSID CapturedPrimaryGroupSid = NULL;
    PACL CapturedDefaultDacl = NULL;
    ULONG PrivilegesLength, UserLength, GroupsLength;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(TokenHandle);

            if (ObjectAttributes != NULL)
            {
                ProbeForRead(ObjectAttributes,
                             sizeof(OBJECT_ATTRIBUTES),
                             sizeof(ULONG));
                LocalSecurityQos = *(SECURITY_QUALITY_OF_SERVICE*)ObjectAttributes->SecurityQualityOfService;
            }

            ProbeForRead(AuthenticationId,
                         sizeof(LUID),
                         sizeof(ULONG));
            LocalAuthenticationId = *AuthenticationId;

            LocalExpirationTime = ProbeForReadLargeInteger(ExpirationTime);

            ProbeForRead(TokenUser,
                         sizeof(TOKEN_USER),
                         sizeof(ULONG));

            ProbeForRead(TokenGroups,
                         sizeof(TOKEN_GROUPS),
                         sizeof(ULONG));
            GroupCount = TokenGroups->GroupCount;

            ProbeForRead(TokenPrivileges,
                         sizeof(TOKEN_PRIVILEGES),
                         sizeof(ULONG));
            PrivilegeCount = TokenPrivileges->PrivilegeCount;

            if (TokenOwner != NULL)
            {
                ProbeForRead(TokenOwner,
                             sizeof(TOKEN_OWNER),
                             sizeof(ULONG));
                OwnerSid = TokenOwner->Owner;
            }
            else
            {
                OwnerSid = NULL;
            }

            ProbeForRead(TokenPrimaryGroup,
                         sizeof(TOKEN_PRIMARY_GROUP),
                         sizeof(ULONG));
            PrimaryGroupSid = TokenPrimaryGroup->PrimaryGroup;

            if (TokenDefaultDacl != NULL)
            {
                ProbeForRead(TokenDefaultDacl,
                             sizeof(TOKEN_DEFAULT_DACL),
                             sizeof(ULONG));
                DefaultDacl = TokenDefaultDacl->DefaultDacl;
            }
            else
            {
                DefaultDacl = NULL;
            }

            ProbeForRead(TokenSource,
                         sizeof(TOKEN_SOURCE),
                         sizeof(ULONG));
            LocalTokenSource = *TokenSource;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        if (ObjectAttributes != NULL)
            LocalSecurityQos = *(SECURITY_QUALITY_OF_SERVICE*)ObjectAttributes->SecurityQualityOfService;
        LocalAuthenticationId = *AuthenticationId;
        LocalExpirationTime = *ExpirationTime;
        GroupCount = TokenGroups->GroupCount;
        PrivilegeCount = TokenPrivileges->PrivilegeCount;
        OwnerSid = TokenOwner ? TokenOwner->Owner : NULL;
        PrimaryGroupSid = TokenPrimaryGroup->PrimaryGroup;
        DefaultDacl = TokenDefaultDacl ? TokenDefaultDacl->DefaultDacl : NULL;
        LocalTokenSource = *TokenSource;
    }

    /* Check token type */
    if ((TokenType < TokenPrimary) ||
        (TokenType > TokenImpersonation))
    {
        return STATUS_BAD_TOKEN_TYPE;
    }

    /* Capture the user SID and attributes */
    Status = SeCaptureSidAndAttributesArray(&TokenUser->User,
                                            1,
                                            PreviousMode,
                                            NULL,
                                            0,
                                            PagedPool,
                                            FALSE,
                                            &CapturedUser,
                                            &UserLength);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture the groups SID and attributes array */
    Status = SeCaptureSidAndAttributesArray(&TokenGroups->Groups[0],
                                            GroupCount,
                                            PreviousMode,
                                            NULL,
                                            0,
                                            PagedPool,
                                            FALSE,
                                            &CapturedGroups,
                                            &GroupsLength);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture privileges */
    Status = SeCaptureLuidAndAttributesArray(&TokenPrivileges->Privileges[0],
                                             PrivilegeCount,
                                             PreviousMode,
                                             NULL,
                                             0,
                                             PagedPool,
                                             FALSE,
                                             &CapturedPrivileges,
                                             &PrivilegesLength);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture the token owner SID */
    if (TokenOwner != NULL)
    {
        Status = SepCaptureSid(OwnerSid,
                               PreviousMode,
                               PagedPool,
                               FALSE,
                               &CapturedOwnerSid);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    /* Capture the token primary group SID */
    Status = SepCaptureSid(PrimaryGroupSid,
                           PreviousMode,
                           PagedPool,
                           FALSE,
                           &CapturedPrimaryGroupSid);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture DefaultDacl */
    if (DefaultDacl != NULL)
    {
        Status = SepCaptureAcl(DefaultDacl,
                               PreviousMode,
                               NonPagedPool,
                               FALSE,
                               &CapturedDefaultDacl);
    }

    /* Call the internal function */
    Status = SepCreateToken(&hToken,
                            PreviousMode,
                            DesiredAccess,
                            ObjectAttributes,
                            TokenType,
                            LocalSecurityQos.ImpersonationLevel,
                            &LocalAuthenticationId,
                            &LocalExpirationTime,
                            CapturedUser,
                            GroupCount,
                            CapturedGroups,
                            0, // FIXME: Should capture
                            PrivilegeCount,
                            CapturedPrivileges,
                            CapturedOwnerSid,
                            CapturedPrimaryGroupSid,
                            CapturedDefaultDacl,
                            &LocalTokenSource,
                            FALSE);
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

Cleanup:

    /* Release what we captured */
    SeReleaseSidAndAttributesArray(CapturedUser, PreviousMode, FALSE);
    SeReleaseSidAndAttributesArray(CapturedGroups, PreviousMode, FALSE);
    SeReleaseLuidAndAttributesArray(CapturedPrivileges, PreviousMode, FALSE);
    SepReleaseSid(CapturedOwnerSid, PreviousMode, FALSE);
    SepReleaseSid(CapturedPrimaryGroupSid, PreviousMode, FALSE);
    SepReleaseAcl(CapturedDefaultDacl, PreviousMode, FALSE);

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
    PETHREAD Thread, NewThread;
    HANDLE hToken;
    PTOKEN Token, NewToken = NULL, PrimaryToken;
    BOOLEAN CopyOnOpen, EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    SE_IMPERSONATION_STATE ImpersonationState;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl = NULL;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    BOOLEAN RestoreImpersonation = FALSE;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(TokenHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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
        ObDereferenceObject(Thread);
        return STATUS_NO_TOKEN;
    }

    if (ImpersonationLevel == SecurityAnonymous)
    {
        PsDereferenceImpersonationToken(Token);
        ObDereferenceObject(Thread);
        return STATUS_CANT_OPEN_ANONYMOUS;
    }

    /*
     * Revert to self if OpenAsSelf is specified.
     */

    if (OpenAsSelf)
    {
        RestoreImpersonation = PsDisableImpersonation(PsGetCurrentThread(),
                                                      &ImpersonationState);
    }

    if (CopyOnOpen)
    {
        Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS,
                                           PsThreadType, KernelMode,
                                           (PVOID*)&NewThread, NULL);
        if (NT_SUCCESS(Status))
        {
            PrimaryToken = PsReferencePrimaryToken(NewThread->ThreadsProcess);

            Status = SepCreateImpersonationTokenDacl(Token, PrimaryToken, &Dacl);

            ObFastDereferenceObject(&NewThread->ThreadsProcess->Token, PrimaryToken);

            if (NT_SUCCESS(Status))
            {
                if (Dacl)
                {
                    RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                                SECURITY_DESCRIPTOR_REVISION);
                    RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl,
                                                 FALSE);
                }

                InitializeObjectAttributes(&ObjectAttributes, NULL, HandleAttributes,
                                           NULL, Dacl ? &SecurityDescriptor : NULL);


                Status = SepDuplicateToken(Token, &ObjectAttributes, EffectiveOnly,
                                           TokenImpersonation, ImpersonationLevel,
                                           KernelMode, &NewToken);
                if (NT_SUCCESS(Status))
                {
                    ObReferenceObject(NewToken);
                    Status = ObInsertObject(NewToken, NULL, DesiredAccess, 0, NULL,
                                            &hToken);
                }
            }
        }
    }
    else
    {
        Status = ObOpenObjectByPointer(Token, HandleAttributes,
                                       NULL, DesiredAccess, SeTokenObjectType,
                                       PreviousMode, &hToken);
    }

    if (Dacl) ExFreePoolWithTag(Dacl, TAG_TOKEN_ACL);

    if (RestoreImpersonation)
    {
        PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }

    ObDereferenceObject(Token);

    if (NT_SUCCESS(Status) && CopyOnOpen)
    {
        PsImpersonateClient(Thread, NewToken, FALSE, EffectiveOnly, ImpersonationLevel);
    }

    if (NewToken) ObDereferenceObject(NewToken);

    if (CopyOnOpen && NewThread) ObDereferenceObject(NewThread);

    ObDereferenceObject(Thread);

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
    NTSTATUS Status;

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
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    Status = ObReferenceObjectByHandle(FirstTokenHandle,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&FirstToken,
                                       NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ObReferenceObjectByHandle(SecondTokenHandle,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
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
