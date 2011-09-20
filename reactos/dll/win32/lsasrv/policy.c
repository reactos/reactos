/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/policy.c
 * PURPOSE:     Policy object routines
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


/* FUNCTIONS ***************************************************************/

NTSTATUS
LsarSetPrimaryDomain(LSAPR_HANDLE PolicyHandle,
                     PLSAPR_POLICY_PRIMARY_DOM_INFO Info)
{
    PUNICODE_STRING Buffer;
    ULONG Length = 0;
    NTSTATUS Status;
    LPWSTR Ptr;

    TRACE("LsarSetPrimaryDomain(%p, %p)\n", PolicyHandle, Info);

    Length = sizeof(UNICODE_STRING) + Info->Name.MaximumLength;
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                             0,
                             Length);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Buffer->Length = Info->Name.Length;
    Buffer->MaximumLength = Info->Name.MaximumLength;
    Buffer->Buffer = (LPWSTR)sizeof(UNICODE_STRING);
    Ptr = (LPWSTR)((ULONG_PTR)Buffer + sizeof(UNICODE_STRING));
    memcpy(Ptr, Info->Name.Buffer, Info->Name.MaximumLength);

    Status = LsapSetObjectAttribute((PLSA_DB_OBJECT)PolicyHandle,
                                    L"PolPrDmN",
                                    Buffer, Length);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

    Length = 0;
    if (Info->Sid != NULL)
        Length = RtlLengthSid(Info->Sid);

    Status = LsapSetObjectAttribute((PLSA_DB_OBJECT)PolicyHandle,
                                    L"PolPrDmS",
                                    (LPBYTE)Info->Sid,
                                    Length);

    return Status;
}


NTSTATUS
LsarSetAccountDomain(LSAPR_HANDLE PolicyHandle,
                     PLSAPR_POLICY_ACCOUNT_DOM_INFO Info)
{
    PUNICODE_STRING Buffer;
    ULONG Length = 0;
    NTSTATUS Status;
    LPWSTR Ptr;

    TRACE("LsarSetAccountDomain(%p, %p)\n", PolicyHandle, Info);

    Length = sizeof(UNICODE_STRING) + Info->DomainName.MaximumLength;
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                             0,
                             Length);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Buffer->Length = Info->DomainName.Length;
    Buffer->MaximumLength = Info->DomainName.MaximumLength;
    Buffer->Buffer = (LPWSTR)sizeof(UNICODE_STRING);
    Ptr = (LPWSTR)((ULONG_PTR)Buffer + sizeof(UNICODE_STRING));
    memcpy(Ptr, Info->DomainName.Buffer, Info->DomainName.MaximumLength);

    Status = LsapSetObjectAttribute((PLSA_DB_OBJECT)PolicyHandle,
                                    L"PolAcDmN",
                                    Buffer, Length);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

    Length = 0;
    if (Info->Sid != NULL)
        Length = RtlLengthSid(Info->Sid);

    Status = LsapSetObjectAttribute((PLSA_DB_OBJECT)PolicyHandle,
                                    L"PolAcDmS",
                                    (LPBYTE)Info->Sid,
                                    Length);

    return Status;
}


NTSTATUS
LsarSetDnsDomain(LSAPR_HANDLE PolicyHandle,
                 PLSAPR_POLICY_DNS_DOMAIN_INFO Info)
{

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
