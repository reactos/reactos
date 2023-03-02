/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security subsystem debug routines support
 * COPYRIGHT:   Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Converts an Access Control Entry (ACE) type to a string.
 *
 * @return
 * Returns a converted ACE type strings. If no
 * known ACE type is found, it will return
 * UNKNOWN TYPE.
 */
static
PCSTR
SepGetAceTypeString(
    _In_ UCHAR AceType)
{
#define TOSTR(x)    #x
    static const PCSTR AceTypes[] =
    {
        TOSTR(ACCESS_ALLOWED_ACE_TYPE),
        TOSTR(ACCESS_DENIED_ACE_TYPE),
        TOSTR(SYSTEM_AUDIT_ACE_TYPE),
        TOSTR(SYSTEM_ALARM_ACE_TYPE),
        TOSTR(ACCESS_ALLOWED_COMPOUND_ACE_TYPE),
        TOSTR(ACCESS_ALLOWED_OBJECT_ACE_TYPE),
        TOSTR(ACCESS_DENIED_OBJECT_ACE_TYPE),
        TOSTR(SYSTEM_AUDIT_OBJECT_ACE_TYPE),
        TOSTR(SYSTEM_ALARM_OBJECT_ACE_TYPE),
        TOSTR(ACCESS_ALLOWED_CALLBACK_ACE_TYPE),
        TOSTR(ACCESS_DENIED_CALLBACK_ACE_TYPE),
        TOSTR(ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE),
        TOSTR(ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE),
        TOSTR(SYSTEM_AUDIT_CALLBACK_ACE_TYPE),
        TOSTR(SYSTEM_ALARM_CALLBACK_ACE_TYPE),
        TOSTR(SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE),
        TOSTR(SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE),
        TOSTR(SYSTEM_MANDATORY_LABEL_ACE_TYPE),
    };
#undef TOSTR

    if (AceType < RTL_NUMBER_OF(AceTypes))
        return AceTypes[AceType];
    else
        return "UNKNOWN TYPE";
}

/**
 * @brief
 * Dumps the ACE flags to the debugger output.
 */
static
VOID
SepDumpAceFlags(
    _In_ UCHAR AceFlags)
{
#define ACE_FLAG_PRINT(x)  \
    if (AceFlags & x)      \
    {                      \
        DPRINT(#x "\n"); \
    }

    ACE_FLAG_PRINT(OBJECT_INHERIT_ACE);
    ACE_FLAG_PRINT(CONTAINER_INHERIT_ACE);
    ACE_FLAG_PRINT(NO_PROPAGATE_INHERIT_ACE);
    ACE_FLAG_PRINT(INHERIT_ONLY_ACE);
    ACE_FLAG_PRINT(INHERITED_ACE);
#undef ACE_FLAG_PRINT
}

/**
 * @brief
 * Iterates and dumps each ACE debug info in an ACL.
 */
static
VOID
SepDumpAces(
    _In_ PACL Acl)
{
    NTSTATUS Status;
    PACE Ace;
    ULONG AceIndex;
    PSID Sid;
    UNICODE_STRING SidString;

    /* Loop all ACEs and dump their info */
    for (AceIndex = 0; AceIndex < Acl->AceCount; AceIndex++)
    {
        /* Get the ACE at this index */
        Status = RtlGetAce(Acl, AceIndex, (PVOID*)&Ace);
        if (!NT_SUCCESS(Status))
        {
            /*
             * Normally this should never happen.
             * Just fail gracefully and stop further
             * debugging of ACEs.
             */
            DPRINT("SepDumpAces(): Failed to find the next ACE, stop dumping info...\n");
            return;
        }

        DPRINT("================== %lu# ACE DUMP INFO ==================\n", AceIndex);
        DPRINT("Ace -> 0x%p\n", Ace);
        DPRINT("Ace->Header.AceType -> %s\n", SepGetAceTypeString(Ace->Header.AceType));
        DPRINT("Ace->AccessMask -> 0x%08lx\n", Ace->AccessMask);

        Sid = SepGetSidFromAce(Ace->Header.AceType, Ace);
        ASSERT(Sid);
        RtlConvertSidToUnicodeString(&SidString, Sid, TRUE);
        DPRINT("Ace SID -> %wZ\n", &SidString);
        RtlFreeUnicodeString(&SidString);

        DPRINT("Ace->Header.AceSize -> %u\n", Ace->Header.AceSize);
        DPRINT("Ace->Header.AceFlags:\n");
        SepDumpAceFlags(Ace->Header.AceFlags);
    }
}

/**
 * @brief
 * Dumps debug info of an Access Control List (ACL).
 */
static
VOID
SepDumpAclInfo(
    _In_ PACL Acl,
    _In_ BOOLEAN IsSacl)
{
    /* Dump relevant info */
    DPRINT("================== %s DUMP INFO ==================\n", IsSacl ? "SACL" : "DACL");
    DPRINT("Acl->AclRevision -> %u\n", Acl->AclRevision);
    DPRINT("Acl->AclSize -> %u\n", Acl->AclSize);
    DPRINT("Acl->AceCount -> %u\n", Acl->AceCount);

    /* Dump all the ACEs present on this ACL */
    SepDumpAces(Acl);
}

/**
 * @brief
 * Dumps control flags of a security descriptor to the debugger.
 */
static
VOID
SepDumpSdControlInfo(
    _In_ SECURITY_DESCRIPTOR_CONTROL SdControl)
{
#define SD_CONTROL_PRINT(x) \
    if (SdControl & x)      \
    {                       \
        DPRINT(#x "\n");  \
    }

    SD_CONTROL_PRINT(SE_OWNER_DEFAULTED);
    SD_CONTROL_PRINT(SE_GROUP_DEFAULTED);
    SD_CONTROL_PRINT(SE_DACL_PRESENT);
    SD_CONTROL_PRINT(SE_DACL_DEFAULTED);
    SD_CONTROL_PRINT(SE_SACL_PRESENT);
    SD_CONTROL_PRINT(SE_SACL_DEFAULTED);
    SD_CONTROL_PRINT(SE_DACL_UNTRUSTED);
    SD_CONTROL_PRINT(SE_SERVER_SECURITY);
    SD_CONTROL_PRINT(SE_DACL_AUTO_INHERIT_REQ);
    SD_CONTROL_PRINT(SE_SACL_AUTO_INHERIT_REQ);
    SD_CONTROL_PRINT(SE_DACL_AUTO_INHERITED);
    SD_CONTROL_PRINT(SE_SACL_AUTO_INHERITED);
    SD_CONTROL_PRINT(SE_DACL_PROTECTED);
    SD_CONTROL_PRINT(SE_SACL_PROTECTED);
    SD_CONTROL_PRINT(SE_RM_CONTROL_VALID);
    SD_CONTROL_PRINT(SE_SELF_RELATIVE);
#undef SD_CONTROL_PRINT
}

/**
 * @brief
 * Dumps each security identifier (SID) of an access token to debugger.
 */
static
VOID
SepDumpSidsOfToken(
    _In_ PSID_AND_ATTRIBUTES Sids,
    _In_ ULONG SidCount)
{
    ULONG SidIndex;
    UNICODE_STRING SidString;

    /* Loop all SIDs and dump them */
    for (SidIndex = 0; SidIndex < SidCount; SidIndex++)
    {
        RtlConvertSidToUnicodeString(&SidString, Sids[SidIndex].Sid, TRUE);
        DPRINT("%lu# %wZ\n", SidIndex, &SidString);
        RtlFreeUnicodeString(&SidString);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Dumps debug information of a security descriptor to the debugger.
 */
VOID
SepDumpSdDebugInfo(
    _In_opt_ PISECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNICODE_STRING SidString;
    PSID OwnerSid, GroupSid;
    PACL Dacl, Sacl;

    /* Don't dump anything if no SD was provided */
    if (!SecurityDescriptor)
    {
        return;
    }

    /* Cache the necessary security buffers to dump info from */
    OwnerSid = SepGetOwnerFromDescriptor(SecurityDescriptor);
    GroupSid = SepGetGroupFromDescriptor(SecurityDescriptor);
    Sacl = SepGetSaclFromDescriptor(SecurityDescriptor);
    Dacl = SepGetDaclFromDescriptor(SecurityDescriptor);

    DPRINT("================== SECURITY DESCRIPTOR DUMP INFO ==================\n");
    DPRINT("SecurityDescriptor -> 0x%p\n", SecurityDescriptor);
    DPRINT("SecurityDescriptor->Revision -> %u\n", SecurityDescriptor->Revision);
    DPRINT("SecurityDescriptor->Control:\n");
    SepDumpSdControlInfo(SecurityDescriptor->Control);

    /* Dump the Owner SID if the SD belongs to an owner */
    if (OwnerSid)
    {
        RtlConvertSidToUnicodeString(&SidString, OwnerSid, TRUE);
        DPRINT("SD Owner SID -> %wZ\n", &SidString);
        RtlFreeUnicodeString(&SidString);
    }

    /* Dump the Group SID if the SD belongs to a group */
    if (GroupSid)
    {
        RtlConvertSidToUnicodeString(&SidString, GroupSid, TRUE);
        DPRINT("SD Group SID -> %wZ\n", &SidString);
        RtlFreeUnicodeString(&SidString);
    }

    /* Dump the ACL contents of SACL if this SD has one */
    if (Sacl)
    {
        SepDumpAclInfo(Sacl, TRUE);
    }

    /* Dump the ACL contents of DACL if this SD has one */
    if (Dacl)
    {
        SepDumpAclInfo(Dacl, FALSE);
    }
}

/**
 * @brief
 * Dumps debug information of an access token to the debugger.
 */
VOID
SepDumpTokenDebugInfo(
    _In_opt_ PTOKEN Token)
{
    UNICODE_STRING SidString;

    /* Don't dump anything if no token was provided */
    if (!Token)
    {
        return;
    }

    /* Dump relevant token info */
    DPRINT("================== ACCESS TOKEN DUMP INFO ==================\n");
    DPRINT("Token -> 0x%p\n", Token);
    DPRINT("Token->ImageFileName -> %s\n", Token->ImageFileName);
    DPRINT("Token->TokenSource.SourceName -> \"%-.*s\"\n",
             RTL_NUMBER_OF(Token->TokenSource.SourceName),
             Token->TokenSource.SourceName);
    DPRINT("Token->TokenSource.SourceIdentifier -> %lu.%lu\n",
             Token->TokenSource.SourceIdentifier.HighPart,
             Token->TokenSource.SourceIdentifier.LowPart);

    RtlConvertSidToUnicodeString(&SidString, Token->PrimaryGroup, TRUE);
    DPRINT("Token primary group SID -> %wZ\n", &SidString);
    RtlFreeUnicodeString(&SidString);

    DPRINT("Token user and groups SIDs:\n");
    SepDumpSidsOfToken(Token->UserAndGroups, Token->UserAndGroupCount);

    if (SeTokenIsRestricted(Token))
    {
        DPRINT("Token restricted SIDs:\n");
        SepDumpSidsOfToken(Token->RestrictedSids, Token->RestrictedSidCount);
    }
}

/**
 * @brief
 * Dumps security access rights to the debugger.
 */
VOID
SepDumpAccessRightsStats(
    _In_opt_ PACCESS_CHECK_RIGHTS AccessRights)
{
    /* Don't dump anything if no access check rights list was provided */
    if (!AccessRights)
    {
        return;
    }

    DPRINT("================== ACCESS CHECK RIGHTS STATISTICS ==================\n");
    DPRINT("Remaining access rights -> 0x%08lx\n", AccessRights->RemainingAccessRights);
    DPRINT("Granted access rights -> 0x%08lx\n", AccessRights->GrantedAccessRights);
    DPRINT("Denied access rights -> 0x%08lx\n", AccessRights->DeniedAccessRights);
}

/* EOF */
