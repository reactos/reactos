/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test for object security inheritance
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "se.h"

static GENERIC_MAPPING GenericMapping =
{
    STANDARD_RIGHTS_READ    | 0x1001,
    STANDARD_RIGHTS_WRITE   | 0x2002,
    STANDARD_RIGHTS_EXECUTE | 0x4004,
    STANDARD_RIGHTS_ALL     | 0x800F,
};

static
VOID
TestSeAssignSecurity(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    NTSTATUS Status;
    PTOKEN Token;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_DESCRIPTOR ParentDescriptor;
    SECURITY_DESCRIPTOR ExplicitDescriptor;
    ACL EmptyAcl;
    PACL Acl;
    PACL Acl2;
    ULONG AclSize;
    ULONG UsingDefault;
    ULONG CanInherit;
    ULONG AceFlags;
    ULONG AceFlags2;
    ULONG Access;
    PSID GenericSid;
    PSID GenericSid2;
    ACCESS_MASK GenericMask;
    ACCESS_MASK GenericMask2;
    PSID SpecificSid;
    ACCESS_MASK SpecificMask;
    ACCESS_MASK SpecificMask2;
    BOOLEAN ParentUsable;

    Token = SubjectContext->PrimaryToken;
    CheckAcl(Token->DefaultDacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    GENERIC_ALL,
                                    ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    GENERIC_READ | GENERIC_EXECUTE | STANDARD_RIGHTS_READ);
    CheckSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid, NO_SIZE, SeExports->SeAliasAdminsSid);
    CheckSid(Token->PrimaryGroup, NO_SIZE, SeExports->SeLocalSystemSid);
// Flags with no effect on current tests: SEF_SACL_AUTO_INHERIT, SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT
#define StartTestAssign(Parent, Explicit, IsDir, GotDacl, GotSacl)  \
    SecurityDescriptor = NULL;                                      \
    Status = SeAssignSecurity  (Parent,                             \
                                Explicit,                           \
                                &SecurityDescriptor,                \
                                /*NULL,*/                           \
                                IsDir,                              \
                                /*0,*/                              \
                                SubjectContext,                     \
                                &GenericMapping,                    \
                                PagedPool);                         \
    ok_eq_hex(Status, STATUS_SUCCESS);                              \
    if (!skip(NT_SUCCESS(Status), "No security\n"))                 \
    {                                                               \
        PACL Dacl, Sacl;                                            \
        PSID Owner, Group;                                          \
        BOOLEAN Present;                                            \
        BOOLEAN DaclDefaulted, SaclDefaulted;                       \
        BOOLEAN OwnerDefaulted, GroupDefaulted;                     \
        Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,   \
                                              &Present,             \
                                              &Dacl,                \
                                              &DaclDefaulted);      \
        ok_eq_hex(Status, STATUS_SUCCESS);                          \
        ok_eq_uint(Present, GotDacl);                               \
        if (!NT_SUCCESS(Status) || !Present)                        \
            Dacl = NULL;                                            \
        Status = RtlGetSaclSecurityDescriptor(SecurityDescriptor,   \
                                              &Present,             \
                                              &Sacl,                \
                                              &SaclDefaulted);      \
        ok_eq_hex(Status, STATUS_SUCCESS);                          \
        ok_eq_uint(Present, GotSacl);                               \
        if (!NT_SUCCESS(Status) || !Present)                        \
            Sacl = NULL;                                            \
        Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,  \
                                               &Owner,              \
                                               &OwnerDefaulted);    \
        ok_eq_hex(Status, STATUS_SUCCESS);                          \
        if (skip(NT_SUCCESS(Status), "No owner\n"))                 \
            Owner = NULL;                                           \
        Status = RtlGetGroupSecurityDescriptor(SecurityDescriptor,  \
                                               &Group,              \
                                               &GroupDefaulted);    \
        ok_eq_hex(Status, STATUS_SUCCESS);                          \
        if (skip(NT_SUCCESS(Status), "No group\n"))                 \
            Group = NULL;

#define EndTestAssign()                                             \
        SeDeassignSecurity(&SecurityDescriptor);                    \
    }
#define StartTestAssignLoop(Parent, Explicit)                                       \
    {                                                                               \
        BOOLEAN IsDir;                                                              \
        BOOLEAN UsingParent;                                                        \
        BOOLEAN UsingExplicit;                                                      \
        for (IsDir = FALSE; IsDir <= TRUE; IsDir++)                                 \
        {                                                                           \
            for (UsingParent = FALSE; UsingParent <= TRUE; UsingParent++)           \
            {                                                                       \
                for (UsingExplicit = FALSE; UsingExplicit <= TRUE; UsingExplicit++) \
                {                                                                   \
                    StartTestAssign(UsingParent ? Parent : NULL,                    \
                                    UsingExplicit ? Explicit : NULL,                \
                                    IsDir,                                          \
                                    TRUE,                                           \
                                    FALSE)
#define EndTestAssignLoop()                                                         \
                    EndTestAssign()                                                 \
                }                                                                   \
            }                                                                       \
        }                                                                           \
    }
#define TestAssignExpectDefault(Parent, Explicit, IsDir)                                                                \
    StartTestAssign(Parent, Explicit, IsDir, TRUE, FALSE)                                                               \
        ok_eq_uint(DaclDefaulted, FALSE);                                                                               \
        CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,     \
                          ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);   \
        ok_eq_uint(OwnerDefaulted, FALSE);                                                                              \
        CheckSid(Owner, NO_SIZE, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);                                   \
        ok_eq_uint(GroupDefaulted, FALSE);                                                                              \
        CheckSid(Group, NO_SIZE, Token->PrimaryGroup);                                                                  \
    EndTestAssign()
#define TestAssignExpectDefaultAll()                                        \
    TestAssignExpectDefault(&ParentDescriptor, NULL, FALSE)                 \
    TestAssignExpectDefault(&ParentDescriptor, NULL, TRUE)                  \
    TestAssignExpectDefault(NULL, &ExplicitDescriptor, FALSE)               \
    TestAssignExpectDefault(NULL, &ExplicitDescriptor, TRUE)                \
    TestAssignExpectDefault(&ParentDescriptor, &ExplicitDescriptor, FALSE)  \
    TestAssignExpectDefault(&ParentDescriptor, &ExplicitDescriptor, TRUE)

    TestAssignExpectDefault(NULL, NULL, FALSE)
    TestAssignExpectDefault(NULL, NULL, TRUE)

    /* Empty parent/explicit descriptors */
    Status = RtlCreateSecurityDescriptor(&ParentDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = RtlCreateSecurityDescriptor(&ExplicitDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    ok_eq_hex(Status, STATUS_SUCCESS);
    TestAssignExpectDefaultAll()

    /* NULL DACL in parent/explicit descriptor */
    for (UsingDefault = FALSE; UsingDefault <= TRUE; UsingDefault++)
    {
        Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              NULL,
                                              UsingDefault);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              NULL,
                                              UsingDefault);
        ok_eq_hex(Status, STATUS_SUCCESS);
        StartTestAssignLoop(&ParentDescriptor, &ExplicitDescriptor)
            //trace("Explicit %u, Parent %u, Dir %u, Default %u\n", UsingExplicit, UsingParent, IsDir, UsingDefault);
            ok_eq_uint(DaclDefaulted, FALSE);
            if (UsingExplicit)
            {
                ok(Dacl == NULL, "Dacl = %p\n", Dacl);
            }
            else
            {
                CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                                  ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            }
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
        EndTestAssignLoop()
    }

    /* Empty default DACL in parent/explicit descriptor */
    for (UsingDefault = FALSE; UsingDefault <= TRUE; UsingDefault++)
    {
        Status = RtlCreateAcl(&EmptyAcl, sizeof(EmptyAcl), ACL_REVISION);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              &EmptyAcl,
                                              UsingDefault);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              &EmptyAcl,
                                              UsingDefault);
        ok_eq_hex(Status, STATUS_SUCCESS);
        StartTestAssignLoop(&ParentDescriptor, &ExplicitDescriptor)
            //trace("Explicit %u, Parent %u, Dir %u, Default %u\n", UsingExplicit, UsingParent, IsDir, UsingDefault);
            ok_eq_uint(DaclDefaulted, FALSE);
            if (UsingExplicit)
            {
                CheckAcl(Dacl, 0);
            }
            else
            {
                CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                                  ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            }
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
        EndTestAssignLoop()
    }


    AclSize = sizeof(ACL) + FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + RtlLengthSid(SeExports->SeWorldSid);
    Acl = ExAllocatePoolWithTag(PagedPool, AclSize, 'ASmK');
    if (skip(Acl != NULL, "Out of memory\n"))
        return;

    Acl2 = ExAllocatePoolWithTag(PagedPool, AclSize, 'ASmK');
    if (skip(Acl2 != NULL, "Out of memory\n"))
    {
        ExFreePoolWithTag(Acl, 'ASmK');
        return;
    }

    /* Simple DACL in parent/explicit descriptor */
    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlAddAccessAllowedAceEx(Acl, ACL_REVISION, 0, READ_CONTROL, SeExports->SeWorldSid);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);
        StartTestAssignLoop(&ParentDescriptor, &ExplicitDescriptor)
            //trace("Explicit %u, Parent %u, Dir %u, Default %u\n", UsingExplicit, UsingParent, IsDir, UsingDefault);
            ok_eq_uint(DaclDefaulted, FALSE);
            if (UsingExplicit)
            {
                CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,          READ_CONTROL);
            }
            else
            {
                CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                                  ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            }
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
        EndTestAssignLoop()
    }

    /* Object-inheritable DACL in parent/explicit descriptor */
    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlAddAccessAllowedAceEx(Acl, ACL_REVISION, OBJECT_INHERIT_ACE, READ_CONTROL, SeExports->SeWorldSid);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);
        StartTestAssignLoop(&ParentDescriptor, &ExplicitDescriptor)
            //trace("Explicit %u, Parent %u, Dir %u, Default %u\n", UsingExplicit, UsingParent, IsDir, UsingDefault);
            ok_eq_uint(DaclDefaulted, FALSE);
            if (UsingExplicit && (!UsingParent || !FlagOn(UsingDefault, 2)))
            {
                CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, OBJECT_INHERIT_ACE, SeExports->SeWorldSid, READ_CONTROL);
            }
            else if (UsingParent)
            {
                CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, IsDir ? INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE : 0, SeExports->SeWorldSid, READ_CONTROL);
            }
            else
            {
                CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                                  ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            }
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
        EndTestAssignLoop()
    }

    /* Container-inheritable DACL in parent/explicit descriptor */
    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlAddAccessAllowedAceEx(Acl, ACL_REVISION, CONTAINER_INHERIT_ACE, READ_CONTROL, SeExports->SeWorldSid);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);
        StartTestAssignLoop(&ParentDescriptor, &ExplicitDescriptor)
            //trace("Explicit %u, Parent %u, Dir %u, Default %u\n", UsingExplicit, UsingParent, IsDir, UsingDefault);
            ok_eq_uint(DaclDefaulted, FALSE);
            if (UsingExplicit || (UsingParent && IsDir))
            {
                CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeWorldSid, READ_CONTROL);
            }
            else
            {
                CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                                  ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            }
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
        EndTestAssignLoop()
    }

    /* Fully inheritable DACL in parent/explicit descriptor */
    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlAddAccessAllowedAceEx(Acl, ACL_REVISION, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE, READ_CONTROL, SeExports->SeWorldSid);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);
        StartTestAssignLoop(&ParentDescriptor, &ExplicitDescriptor)
            //trace("Explicit %u, Parent %u, Dir %u, Default %u\n", UsingExplicit, UsingParent, IsDir, UsingDefault);
            ok_eq_uint(DaclDefaulted, FALSE);
            if (UsingExplicit && (!UsingParent || !FlagOn(UsingDefault, 2)))
            {
                CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE, SeExports->SeWorldSid, READ_CONTROL);
            }
            else if (UsingParent)
            {
                CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, IsDir ? OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE : 0, SeExports->SeWorldSid, READ_CONTROL);
            }
            else
            {
                CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                                  ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            }
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
        EndTestAssignLoop()
    }

    /* Different DACLs in parent and explicit descriptors */
    for (Access = 0; Access <= 1; Access++)
    {
        if (Access == 1)
        {
            GenericSid = SeExports->SeCreatorOwnerSid;
            SpecificSid = SeExports->SeAliasAdminsSid;
            GenericMask = GENERIC_READ;
            SpecificMask = STANDARD_RIGHTS_READ | 0x0001;
            GenericSid2 = SeExports->SeCreatorGroupSid;
            GenericMask2 = GENERIC_EXECUTE;
            SpecificMask2 = STANDARD_RIGHTS_EXECUTE | 0x0004;
        }
        else
        {
            GenericSid = SeExports->SeWorldSid;
            SpecificSid = SeExports->SeWorldSid;
            GenericMask = READ_CONTROL;
            SpecificMask = READ_CONTROL;
            GenericSid2 = SeExports->SeLocalSystemSid;
            GenericMask2 = SYNCHRONIZE;
            SpecificMask2 = SYNCHRONIZE;
        }
        for (CanInherit = 0; CanInherit <= 255; CanInherit++)
        {
            for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
            {
                Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
                ok_eq_hex(Status, STATUS_SUCCESS);
                AceFlags = CanInherit & 0xf;
                Status = RtlAddAccessAllowedAceEx(Acl, ACL_REVISION, AceFlags, GenericMask, GenericSid);
                ok_eq_hex(Status, STATUS_SUCCESS);
                Status = RtlCreateAcl(Acl2, AclSize, ACL_REVISION);
                ok_eq_hex(Status, STATUS_SUCCESS);
                AceFlags2 = CanInherit >> 4;
                Status = RtlAddAccessAllowedAceEx(Acl2, ACL_REVISION, AceFlags2, GenericMask2, GenericSid2);
                ok_eq_hex(Status, STATUS_SUCCESS);
                Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                                      TRUE,
                                                      Acl,
                                                      BooleanFlagOn(UsingDefault, 1));
                ok_eq_hex(Status, STATUS_SUCCESS);
                Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                                      TRUE,
                                                      Acl2,
                                                      BooleanFlagOn(UsingDefault, 2));
                ok_eq_hex(Status, STATUS_SUCCESS);
                StartTestAssignLoop(&ParentDescriptor, &ExplicitDescriptor)
                    //trace("Explicit %u, Parent %u, Dir %u, Default %u, Inherit %u, Access %u\n", UsingExplicit, UsingParent, IsDir, UsingDefault, CanInherit, Access);
                    ok_eq_uint(DaclDefaulted, FALSE);
                    ParentUsable = UsingParent;
                    if (!IsDir && !FlagOn(AceFlags, OBJECT_INHERIT_ACE))
                        ParentUsable = FALSE;
                    else if (IsDir && !FlagOn(AceFlags, CONTAINER_INHERIT_ACE) &&
                             (!FlagOn(AceFlags, OBJECT_INHERIT_ACE) || FlagOn(AceFlags, NO_PROPAGATE_INHERIT_ACE)))
                        ParentUsable = FALSE;

                    if (UsingExplicit && (!FlagOn(UsingDefault, 2) || !ParentUsable))
                    {
                        CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, AceFlags2, GenericSid2, FlagOn(AceFlags2, INHERIT_ONLY_ACE) ? GenericMask2 : SpecificMask2);
                    }
                    else if (ParentUsable)
                    {
                        if (IsDir && !FlagOn(AceFlags, NO_PROPAGATE_INHERIT_ACE))
                        {
                            if (FlagOn(AceFlags, CONTAINER_INHERIT_ACE) && (SpecificMask != GenericMask || SpecificSid != GenericSid))
                                CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SpecificSid, SpecificMask,
                                                  ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | (AceFlags & OBJECT_INHERIT_ACE), GenericSid, GenericMask);
                            else
                                CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, (FlagOn(AceFlags, CONTAINER_INHERIT_ACE) ? 0 : INHERIT_ONLY_ACE) |
                                                                           (AceFlags & (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)), GenericSid, GenericMask);
                        }
                        else
                            CheckAcl(Dacl, 1, ACCESS_ALLOWED_ACE_TYPE, 0, SpecificSid, SpecificMask);
                    }
                    else
                    {
                        CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, STANDARD_RIGHTS_ALL | 0x800F,
                                          ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, STANDARD_RIGHTS_READ | 0x0005);
                    }
                    ok_eq_uint(OwnerDefaulted, FALSE);
                    CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
                    ok_eq_uint(GroupDefaulted, FALSE);
                    CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
                EndTestAssignLoop()
            }
        }
    }

    /* NULL parameters */
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    KmtStartSeh()
        Status = SeAssignSecurity(NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  SubjectContext,
                                  &GenericMapping,
                                  PagedPool);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");

    SecurityDescriptor = KmtInvalidPointer;
    KmtStartSeh()
        Status = SeAssignSecurity(NULL,
                                  NULL,
                                  &SecurityDescriptor,
                                  FALSE,
                                  NULL,
                                  &GenericMapping,
                                  PagedPool);
        ok_eq_hex(Status, STATUS_NO_TOKEN);
    KmtEndSeh(STATUS_SUCCESS);
    ok_eq_pointer(SecurityDescriptor, NULL);
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");

    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    KmtStartSeh()
        Status = SeAssignSecurity(NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  &GenericMapping,
                                  PagedPool);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");

    /* Test with Token == NULL */
    if (1)
    {
        /* Crash in SeLockSubjectContext while holding a critical region */
        SubjectContext->PrimaryToken = NULL;
        KmtStartSeh()
            SecurityDescriptor = KmtInvalidPointer;
            Status = SeAssignSecurity(NULL,
                                      NULL,
                                      &SecurityDescriptor,
                                      FALSE,
                                      SubjectContext,
                                      &GenericMapping,
                                      PagedPool);
        KmtEndSeh(STATUS_ACCESS_VIOLATION)
        ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
        KeLeaveCriticalRegion();
        ok_eq_pointer(SecurityDescriptor, KmtInvalidPointer);
        SubjectContext->PrimaryToken = Token;
    }
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");

    /* Test with NULL owner in Token */
    if (1)
    {
        /* Crash after locking the subject context */
        PSID OldOwner;
        OldOwner = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
        Token->UserAndGroups[Token->DefaultOwnerIndex].Sid = NULL;
        KmtStartSeh()
            SecurityDescriptor = KmtInvalidPointer;
            Status = SeAssignSecurity(NULL,
                                      NULL,
                                      &SecurityDescriptor,
                                      FALSE,
                                      SubjectContext,
                                      &GenericMapping,
                                      PagedPool);
        KmtEndSeh(STATUS_ACCESS_VIOLATION)
        ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
        SeUnlockSubjectContext(SubjectContext);
        ok_eq_pointer(SecurityDescriptor, KmtInvalidPointer);
        Token->UserAndGroups[Token->DefaultOwnerIndex].Sid = OldOwner;
    }
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");

    /* Test with NULL group in Token */
    if (1)
    {
        PSID OldGroup;
        OldGroup = Token->PrimaryGroup;
        Token->PrimaryGroup = NULL;
        KmtStartSeh()
            SecurityDescriptor = KmtInvalidPointer;
            Status = SeAssignSecurity(NULL,
                                      NULL,
                                      &SecurityDescriptor,
                                      FALSE,
                                      SubjectContext,
                                      &GenericMapping,
                                      PagedPool);
            ok_eq_hex(Status, STATUS_INVALID_PRIMARY_GROUP);
            ok_eq_pointer(SecurityDescriptor, NULL);
            SeDeassignSecurity(&SecurityDescriptor);
        KmtEndSeh(STATUS_SUCCESS);
        Token->PrimaryGroup = OldGroup;
    }
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");

    /* Test with NULL DACL in Token */
    if (1)
    {
        PACL OldDacl;
        OldDacl = Token->DefaultDacl;
        Token->DefaultDacl = NULL;
        KmtStartSeh()
        StartTestAssign(NULL, NULL, FALSE, FALSE, FALSE)
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, Token->PrimaryGroup);
        EndTestAssign()
        KmtEndSeh(STATUS_SUCCESS);
        Token->DefaultDacl = OldDacl;
    }
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");

    /* SEF_DEFAULT_OWNER_FROM_PARENT/SEF_DEFAULT_GROUP_FROM_PARENT */
    SecurityDescriptor = KmtInvalidPointer;
    Status = SeAssignSecurityEx(NULL,
                                NULL,
                                &SecurityDescriptor,
                                NULL,
                                FALSE,
                                SEF_DEFAULT_OWNER_FROM_PARENT,
                                SubjectContext,
                                &GenericMapping,
                                PagedPool);
    ok_eq_hex(Status, STATUS_INVALID_OWNER);
    ok_eq_pointer(SecurityDescriptor, NULL);
    SeDeassignSecurity(&SecurityDescriptor);
    SecurityDescriptor = KmtInvalidPointer;
    Status = SeAssignSecurityEx(NULL,
                                NULL,
                                &SecurityDescriptor,
                                NULL,
                                FALSE,
                                SEF_DEFAULT_GROUP_FROM_PARENT,
                                SubjectContext,
                                &GenericMapping,
                                PagedPool);
    ok_eq_hex(Status, STATUS_INVALID_PRIMARY_GROUP);
    ok_eq_pointer(SecurityDescriptor, NULL);
    SeDeassignSecurity(&SecurityDescriptor);
    SecurityDescriptor = KmtInvalidPointer;
    Status = SeAssignSecurityEx(NULL,
                                NULL,
                                &SecurityDescriptor,
                                NULL,
                                FALSE,
                                SEF_DEFAULT_OWNER_FROM_PARENT | SEF_DEFAULT_GROUP_FROM_PARENT,
                                SubjectContext,
                                &GenericMapping,
                                PagedPool);
    ok_eq_hex(Status, STATUS_INVALID_OWNER);
    ok_eq_pointer(SecurityDescriptor, NULL);
    SeDeassignSecurity(&SecurityDescriptor);

    /* Quick test whether inheritance for SACLs behaves the same as DACLs */
    Status = RtlSetDaclSecurityDescriptor(&ParentDescriptor,
                                          FALSE,
                                          NULL,
                                          FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = RtlSetDaclSecurityDescriptor(&ExplicitDescriptor,
                                          FALSE,
                                          NULL,
                                          FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlSetSaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              NULL,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetSaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              NULL,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);

        TestAssignExpectDefault(&ParentDescriptor, NULL, FALSE)
        TestAssignExpectDefault(&ParentDescriptor, NULL, TRUE)
        StartTestAssign(NULL, &ExplicitDescriptor, FALSE, TRUE, TRUE)
            ok_eq_uint(DaclDefaulted, FALSE);
            CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            ok_eq_uint(SaclDefaulted, FALSE);
            ok_eq_pointer(Sacl, NULL);
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, Token->PrimaryGroup);
        EndTestAssign()
    }

    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlSetSaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              &EmptyAcl,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetSaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              &EmptyAcl,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);

        TestAssignExpectDefault(&ParentDescriptor, NULL, FALSE)
        TestAssignExpectDefault(&ParentDescriptor, NULL, TRUE)
        StartTestAssign(NULL, &ExplicitDescriptor, FALSE, TRUE, TRUE)
            ok_eq_uint(DaclDefaulted, FALSE);
            CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            ok_eq_uint(SaclDefaulted, FALSE);
            CheckAcl(Sacl, 0);
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, Token->PrimaryGroup);
        EndTestAssign()
    }

    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlxAddAuditAccessAceEx(Acl, ACL_REVISION, 0, READ_CONTROL, SeExports->SeWorldSid, TRUE, TRUE);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetSaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetSaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);

        TestAssignExpectDefault(&ParentDescriptor, NULL, FALSE)
        TestAssignExpectDefault(&ParentDescriptor, NULL, TRUE)
        StartTestAssign(NULL, &ExplicitDescriptor, FALSE, TRUE, TRUE)
            ok_eq_uint(DaclDefaulted, FALSE);
            CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            ok_eq_uint(SaclDefaulted, FALSE);
            CheckAcl(Sacl, 1, SYSTEM_AUDIT_ACE_TYPE, SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG, SeExports->SeWorldSid, READ_CONTROL);
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, Token->PrimaryGroup);
        EndTestAssign()
    }

    for (UsingDefault = 0; UsingDefault <= 3; UsingDefault++)
    {
        Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlxAddAuditAccessAceEx(Acl, ACL_REVISION, OBJECT_INHERIT_ACE, READ_CONTROL, SeExports->SeCreatorOwnerSid, TRUE, TRUE);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetSaclSecurityDescriptor(&ParentDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 1));
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = RtlSetSaclSecurityDescriptor(&ExplicitDescriptor,
                                              TRUE,
                                              Acl,
                                              BooleanFlagOn(UsingDefault, 2));
        ok_eq_hex(Status, STATUS_SUCCESS);

        StartTestAssign(&ParentDescriptor, NULL, FALSE, TRUE, TRUE)
            ok_eq_uint(DaclDefaulted, FALSE);
            CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            ok_eq_uint(SaclDefaulted, FALSE);
            CheckAcl(Sacl, 1, SYSTEM_AUDIT_ACE_TYPE, SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid, READ_CONTROL);
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, Token->PrimaryGroup);
        EndTestAssign()
        StartTestAssign(NULL, &ExplicitDescriptor, FALSE, TRUE, TRUE)
            ok_eq_uint(DaclDefaulted, FALSE);
            CheckAcl(Dacl, 2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,    STANDARD_RIGHTS_ALL | 0x800F,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,    STANDARD_RIGHTS_READ | 0x0005);
            ok_eq_uint(SaclDefaulted, FALSE);
            CheckAcl(Sacl, 1, SYSTEM_AUDIT_ACE_TYPE, OBJECT_INHERIT_ACE | SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG, SeExports->SeCreatorOwnerSid, READ_CONTROL);
            ok_eq_uint(OwnerDefaulted, FALSE);
            CheckSid(Owner, NO_SIZE, Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            ok_eq_uint(GroupDefaulted, FALSE);
            CheckSid(Group, NO_SIZE, Token->PrimaryGroup);
        EndTestAssign()
    }

    /* TODO: Test duplicate ACEs */
    /* TODO: Test INHERITED_ACE flag */
    /* TODO: Test invalid ACE flags */
    /* TODO: Test more AutoInheritFlags values */

    ExFreePoolWithTag(Acl2, 'ASmK');
    ExFreePoolWithTag(Acl, 'ASmK');
}

static
VOID
NTAPI
SystemThread(
    _In_ PVOID Context)
{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    ok_eq_pointer(Context, NULL);

    SeCaptureSubjectContext(&SubjectContext);
    TestSeAssignSecurity(&SubjectContext);
    /* TODO: Test SeSetSecurityDescrptorInfo[Ex] */
    SeReleaseSubjectContext(&SubjectContext);
}

static
VOID
TestObRootSecurity(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING ObjectPath = RTL_CONSTANT_STRING(L"\\");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    PVOID RootDirectory;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOLEAN MemoryAllocated;
    PACL Acl;
    BOOLEAN Present;
    BOOLEAN Defaulted;

    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenDirectoryObject(&Handle,
                                   0,
                                   &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No handle\n"))
        return;
    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       &RootDirectory,
                                       NULL);
    ObCloseHandle(Handle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No object\n"))
        return;
    Status = ObGetObjectSecurity(RootDirectory,
                                 &SecurityDescriptor,
                                 &MemoryAllocated);
    ObDereferenceObject(RootDirectory);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No security\n"))
        return;
    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &Acl,
                                          &Defaulted);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Present, TRUE);
    if (!skip(NT_SUCCESS(Status) && Present, "No DACL\n"))
    {
        ok_eq_uint(Defaulted, FALSE);
        CheckAcl(Acl, 4, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,         STANDARD_RIGHTS_READ | DIRECTORY_TRAVERSE | DIRECTORY_QUERY,
                         ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid,   DIRECTORY_ALL_ACCESS,
                         ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid,   DIRECTORY_ALL_ACCESS,
                         ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeRestrictedSid,    STANDARD_RIGHTS_READ | DIRECTORY_TRAVERSE | DIRECTORY_QUERY);
    }
    Status = RtlGetSaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &Acl,
                                          &Defaulted);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Present, FALSE);
    ObReleaseObjectSecurity(SecurityDescriptor, MemoryAllocated);
}

START_TEST(SeInheritance)
{
    PKTHREAD Thread;

    TestObRootSecurity();
    Thread = KmtStartThread(SystemThread, NULL);
    KmtFinishThread(Thread, NULL);
}
