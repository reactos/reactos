/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Helper functions for Se tests
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "se.h"

NTSTATUS
RtlxAddAuditAccessAceEx(
    _Inout_ PACL Acl,
    _In_ ULONG Revision,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid,
    _In_ BOOLEAN Success,
    _In_ BOOLEAN Failure)
{
    NTSTATUS Status;
    USHORT AceSize;
    PSYSTEM_AUDIT_ACE Ace;

    if (Success) Flags |= SUCCESSFUL_ACCESS_ACE_FLAG;
    if (Failure) Flags |= FAILED_ACCESS_ACE_FLAG;

    AceSize = FIELD_OFFSET(SYSTEM_AUDIT_ACE, SidStart) + RtlLengthSid(Sid);
    Ace = ExAllocatePoolWithTag(PagedPool, AceSize, 'cAmK');
    if (!Ace)
        return STATUS_INSUFFICIENT_RESOURCES;
    Ace->Header.AceType = SYSTEM_AUDIT_ACE_TYPE;
    Ace->Header.AceFlags = Flags;
    Ace->Header.AceSize = AceSize;
    Ace->Mask = AccessMask;
    Status = RtlCopySid(AceSize - FIELD_OFFSET(SYSTEM_AUDIT_ACE, SidStart),
                        (PSID)&Ace->SidStart,
                        Sid);
    ASSERT(NT_SUCCESS(Status));
    if (NT_SUCCESS(Status))
    {
        Status = RtlAddAce(Acl,
                           Revision,
                           MAXULONG,
                           Ace,
                           AceSize);
    }
    ExFreePoolWithTag(Ace, 'cAmK');
    return Status;
}

VOID
CheckSid__(
    _In_ PSID Sid,
    _In_ ULONG SidSize,
    _In_ PISID ExpectedSid,
    _In_ PCSTR FileAndLine)
{
    BOOLEAN Okay;
    ULONG Length;

    KmtOk(Sid != NULL, FileAndLine, "Sid is NULL\n");
    if (KmtSkip(Sid != NULL, FileAndLine, "No Sid\n"))
        return;
    if (KmtSkip(SidSize >= sizeof(ULONG), FileAndLine, "Sid too small: %lu\n", SidSize))
        return;
    Okay = RtlValidSid(Sid);
    KmtOk(Okay == TRUE, FileAndLine, "Invalid Sid\n");
    if (KmtSkip(Okay, FileAndLine, "Invalid Sid\n"))
        return;

    Length = RtlLengthSid(Sid);
    KmtOk(SidSize >= Length, FileAndLine, "SidSize %lu too small, need %lu\n", SidSize, Length);
    if (KmtSkip(SidSize >= Length, FileAndLine, "Sid too small\n"))
        return;
    Okay = RtlEqualSid(Sid, ExpectedSid);
    KmtOk(Okay, FileAndLine, "Sids %p and %p not equal\n", Sid, ExpectedSid);
    if (!Okay)
    {
        WCHAR Buffer1[128];
        WCHAR Buffer2[128];
        UNICODE_STRING SidString1, SidString2;
        RtlInitEmptyUnicodeString(&SidString1, Buffer1, sizeof(Buffer1));
        RtlInitEmptyUnicodeString(&SidString2, Buffer2, sizeof(Buffer2));
        (void)RtlConvertSidToUnicodeString(&SidString1, Sid, FALSE);
        (void)RtlConvertSidToUnicodeString(&SidString2, ExpectedSid, FALSE);
        KmtOk(0, FileAndLine, "Got %wZ, expected %wZ\n", &SidString1, &SidString2);
    }
}

VOID
VCheckAcl__(
    _In_ PACL Acl,
    _In_ ULONG AceCount,
    _In_ PCSTR FileAndLine,
    _In_ va_list Arguments)
{
    ULONG i;
    ULONG Offset;
    PACE_HEADER AceHeader;
    INT AceType;
    INT AceFlags;
    ACCESS_MASK Mask;
    PISID Sid;
    PACCESS_ALLOWED_ACE AllowedAce;
    PACCESS_DENIED_ACE DeniedAce;
    PSYSTEM_AUDIT_ACE AuditAce;

    KmtOk(Acl != NULL, FileAndLine, "Acl is NULL\n");
    if (KmtSkip(Acl != NULL, FileAndLine, "No ACL\n"))
        return;
    KmtOk((ULONG_PTR)Acl % sizeof(ULONG) == 0, FileAndLine, "Unaligned ACL %p\n", Acl);
    KmtOk(Acl->AclRevision == ACL_REVISION, FileAndLine, "AclRevision is %u\n", Acl->AclRevision);
    KmtOk(Acl->Sbz1 == 0, FileAndLine, "Sbz1 is %u\n", Acl->Sbz1);
    KmtOk(Acl->Sbz2 == 0, FileAndLine, "Sbz2 is %u\n", Acl->Sbz2);
    KmtOk(Acl->AclSize >= sizeof(*Acl), FileAndLine, "AclSize too small: %u\n", Acl->AclSize);
    KmtOk(Acl->AceCount == AceCount, FileAndLine, "AceCount is %u, expected %lu\n", Acl->AceCount, AceCount);
    Offset = sizeof(*Acl);
    for (i = 0; i < Acl->AceCount; i++)
    {
        KmtOk(Acl->AclSize >= Offset + sizeof(*AceHeader), FileAndLine, "AclSize too small (%u) at Offset %lu, ACE #%lu\n", Acl->AclSize, Offset, i);
        if (Acl->AclSize < Offset + sizeof(*AceHeader))
            break;
        AceHeader = (PACE_HEADER)((PUCHAR)Acl + Offset);
        KmtOk((ULONG_PTR)AceHeader % sizeof(ULONG) == 0, FileAndLine, "[%lu] Unaligned ACE %p\n", i, AceHeader);
        KmtOk(AceHeader->AceSize % sizeof(ULONG) == 0, FileAndLine, "[%lu] Unaligned ACE size %u\n", i, AceHeader->AceSize);
        KmtOk(Acl->AclSize >= Offset + AceHeader->AceSize, FileAndLine, "[%lu] AclSize too small (%u) at Offset %lu\n", i, Acl->AclSize, Offset);
        if (Acl->AclSize < Offset + AceHeader->AceSize)
            break;
        Offset += AceHeader->AceSize;
        if (i >= AceCount)
            continue;
        AceType = va_arg(Arguments, INT);
        AceFlags = va_arg(Arguments, INT);
        KmtOk(AceHeader->AceType == AceType, FileAndLine, "[%lu] AceType is %u, expected %u\n", i, AceHeader->AceType, AceType);
        KmtOk(AceHeader->AceFlags == AceFlags, FileAndLine, "[%lu] AceFlags is 0x%x, expected 0x%x\n", i, AceHeader->AceFlags, AceFlags);
        if (AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            Sid = va_arg(Arguments, PSID);
            Mask = va_arg(Arguments, INT);
            KmtOk(AceHeader->AceSize >= sizeof(*AllowedAce), FileAndLine, "[%lu] AllowedAce AceSize too small: %u\n", i, AceHeader->AceSize);
            if (AceHeader->AceSize < sizeof(*AllowedAce))
                continue;
            AllowedAce = (PACCESS_ALLOWED_ACE)AceHeader;
            KmtOk(AllowedAce->Mask == Mask, FileAndLine, "[%lu] AllowedAce Mask is 0x%lx, expected 0x%lx\n", i, AllowedAce->Mask, Mask);
            CheckSid__((PSID)&AllowedAce->SidStart,
                       AceHeader->AceSize - FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart),
                       Sid,
                       FileAndLine);
        }
        else if (AceType == ACCESS_DENIED_ACE_TYPE)
        {
            Sid = va_arg(Arguments, PSID);
            Mask = va_arg(Arguments, INT);
            KmtOk(AceHeader->AceSize >= sizeof(*DeniedAce), FileAndLine, "[%lu] DeniedAce AceSize too small: %u\n", i, AceHeader->AceSize);
            if (AceHeader->AceSize < sizeof(*DeniedAce))
                continue;
            DeniedAce = (PACCESS_DENIED_ACE)AceHeader;
            KmtOk(DeniedAce->Mask == Mask, FileAndLine, "[%lu] DeniedAce Mask is 0x%lx, expected 0x%lx\n", i, DeniedAce->Mask, Mask);
            CheckSid__((PSID)&DeniedAce->SidStart,
                       AceHeader->AceSize - FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart),
                       Sid,
                       FileAndLine);
        }
        else if (AceType == SYSTEM_AUDIT_ACE_TYPE)
        {
            Sid = va_arg(Arguments, PSID);
            Mask = va_arg(Arguments, INT);
            KmtOk(AceHeader->AceSize >= sizeof(*AuditAce), FileAndLine, "[%lu] AuditAce AceSize too small: %u\n", i, AceHeader->AceSize);
            if (AceHeader->AceSize < sizeof(*AuditAce))
                continue;
            AuditAce = (PSYSTEM_AUDIT_ACE)AceHeader;
            KmtOk(AuditAce->Mask == Mask, FileAndLine, "[%lu] AuditAce Mask is 0x%lx, expected 0x%lx\n", i, AuditAce->Mask, Mask);
            CheckSid__((PSID)&AuditAce->SidStart,
                       AceHeader->AceSize - FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart),
                       Sid,
                       FileAndLine);
        }
    }
}

VOID
CheckAcl__(
    _In_ PACL Acl,
    _In_ ULONG AceCount,
    _In_ PCSTR FileAndLine,
    ...)
{
    va_list Arguments;

    va_start(Arguments, FileAndLine);
    VCheckAcl__(Acl, AceCount, FileAndLine, Arguments);
    va_end(Arguments);
}
