/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RtlDeleteAce
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static
PACL
MakeAcl(
    _In_ ULONG AceCount,
    ...)
{
    PACL Acl;
    PACE_HEADER AceHeader;
    ULONG AclSize;
    ULONG AceSizes[10];
    ULONG i;
    va_list Args;

    ASSERT(AceCount <= RTL_NUMBER_OF(AceSizes));
    AclSize = sizeof(ACL);
    va_start(Args, AceCount);
    for (i = 0; i < AceCount; i++)
    {
        AceSizes[i] = va_arg(Args, int);
        AclSize += AceSizes[i];
    }
    va_end(Args);

    Acl = AllocateGuarded(AclSize);
    if (!Acl)
    {
        skip("Failed to allocate %lu bytes\n", AclSize);
        return NULL;
    }

    Acl->AclRevision = ACL_REVISION;
    Acl->Sbz1 = 0;
    Acl->AclSize = AclSize;
    Acl->AceCount = AceCount;
    Acl->Sbz2 = 0;

    AceHeader = (PACE_HEADER)(Acl + 1);
    for (i = 0; i < AceCount; i++)
    {
        AceHeader->AceType = 0;
        AceHeader->AceFlags = 0;
        AceHeader->AceSize = AceSizes[i];
        AceHeader = (PACE_HEADER)((PCHAR)AceHeader + AceHeader->AceSize);
    }

    return Acl;
}

START_TEST(RtlDeleteAce)
{
    PACL Acl;
    PACCESS_ALLOWED_ACE Ace;
    ULONG AceSize;
    PISID Sid;
    NTSTATUS Status;
    int i;

    Acl = MakeAcl(0);
    if (Acl)
    {
        ok(RtlValidAcl(Acl), "Acl is invalid\n");

        /* There is no first ACE -- should stay untouched */
        Status = RtlDeleteAce(Acl, 0);
        ok(Status == STATUS_INVALID_PARAMETER, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL), "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 0, "AceCount = %u\n", Acl->AceCount);

        /* Index -1 -- should stay untouched */
        Status = RtlDeleteAce(Acl, 0xFFFFFFFF);
        ok(Status == STATUS_INVALID_PARAMETER, "Status = %lx\n", Status);
        FreeGuarded(Acl);
    }

    AceSize = FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + FIELD_OFFSET(SID, SubAuthority);
    Acl = MakeAcl(1, (int)AceSize);
    if (Acl)
    {
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        Sid = (PISID)&Ace->SidStart;
        Sid->Revision = SID_REVISION;
        Sid->SubAuthorityCount = 0;
        RtlZeroMemory(&Sid->IdentifierAuthority, sizeof(Sid->IdentifierAuthority));

        ok(RtlValidAcl(Acl), "Acl is invalid\n");

        /* Out of bounds delete -- should stay untouched */
        Status = RtlDeleteAce(Acl, 1);
        ok(Status == STATUS_INVALID_PARAMETER, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + AceSize, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 1, "AceCount = %u\n", Acl->AceCount);
        ok(Ace->Header.AceSize == AceSize, "AceSize = %u\n", Ace->Header.AceSize);

        /* Index -1 -- should stay untouched */
        Status = RtlDeleteAce(Acl, 0xFFFFFFFF);
        ok(Status == STATUS_INVALID_PARAMETER, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + AceSize, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 1, "AceCount = %u\n", Acl->AceCount);
        ok(Ace->Header.AceSize == AceSize, "AceSize = %u\n", Ace->Header.AceSize);

        /* Delete the first (and only) ACE */
        Status = RtlDeleteAce(Acl, 0);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + AceSize, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 0, "AceCount = %u\n", Acl->AceCount);
        ok(Ace->Header.AceSize == 0, "AceSize = %u\n", Ace->Header.AceSize);
        FreeGuarded(Acl);
    }

    AceSize = FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + FIELD_OFFSET(SID, SubAuthority[1]);
    Acl = MakeAcl(4, (int)AceSize, (int)AceSize + 4, (int)AceSize + 8, (int)AceSize + 12);
    if (Acl)
    {
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        for (i = 0; i < 4; i++)
        {
            Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
            Sid = (PISID)&Ace->SidStart;
            Sid->Revision = SID_REVISION;
            Sid->SubAuthorityCount = 0;
            RtlZeroMemory(&Sid->IdentifierAuthority, sizeof(Sid->IdentifierAuthority));
            Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        }

        ok(RtlValidAcl(Acl), "Acl is invalid\n");

        /* Out of bounds delete -- should stay untouched */
        Status = RtlDeleteAce(Acl, 4);
        ok(Status == STATUS_INVALID_PARAMETER, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + 4 * AceSize + 24, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 4, "AceCount = %u\n", Acl->AceCount);
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        ok(Ace->Header.AceSize == AceSize, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 4, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 8, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 12, "AceSize = %u\n", Ace->Header.AceSize);

        /* Index -1 -- should stay untouched */
        Status = RtlDeleteAce(Acl, 0xFFFFFFFF);
        ok(Status == STATUS_INVALID_PARAMETER, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + 4 * AceSize + 24, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 4, "AceCount = %u\n", Acl->AceCount);
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        ok(Ace->Header.AceSize == AceSize, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 4, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 8, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 12, "AceSize = %u\n", Ace->Header.AceSize);

        /* Delete the last ACE */
        Status = RtlDeleteAce(Acl, 3);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + 4 * AceSize + 24, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 3, "AceCount = %u\n", Acl->AceCount);
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        ok(Ace->Header.AceSize == AceSize, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 4, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 8, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == 0, "AceSize = %u\n", Ace->Header.AceSize);

        /* Delete the second ACE */
        Status = RtlDeleteAce(Acl, 1);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + 4 * AceSize + 24, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 2, "AceCount = %u\n", Acl->AceCount);
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        ok(Ace->Header.AceSize == AceSize, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == AceSize + 8, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == 0, "AceSize = %u\n", Ace->Header.AceSize);

        /* Delete the first ACE */
        Status = RtlDeleteAce(Acl, 0);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + 4 * AceSize + 24, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 1, "AceCount = %u\n", Acl->AceCount);
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        ok(Ace->Header.AceSize == AceSize + 8, "AceSize = %u\n", Ace->Header.AceSize);
        Ace = (PACCESS_ALLOWED_ACE)((PCHAR)Ace + Ace->Header.AceSize);
        ok(Ace->Header.AceSize == 0, "AceSize = %u\n", Ace->Header.AceSize);

        /* Delete the remaining ACE */
        Status = RtlDeleteAce(Acl, 0);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        ok(Acl->AclSize == sizeof(ACL) + 4 * AceSize + 24, "AclSize = %u\n", Acl->AclSize);
        ok(Acl->AceCount == 0, "AceCount = %u\n", Acl->AceCount);
        Ace = (PACCESS_ALLOWED_ACE)(Acl + 1);
        ok(Ace->Header.AceSize == 0, "AceSize = %u\n", Ace->Header.AceSize);

        FreeGuarded(Acl);
    }
}
