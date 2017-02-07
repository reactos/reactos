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
LsarSetPrimaryDomain(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_PRIMARY_DOM_INFO Info)
{
    PUNICODE_STRING Buffer;
    ULONG Length = 0;
    NTSTATUS Status;
    LPWSTR Ptr;

    TRACE("LsarSetPrimaryDomain(%p, %p)\n", PolicyObject, Info);

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

    Status = LsapSetObjectAttribute(PolicyObject,
                                    L"PolPrDmN",
                                    Buffer,
                                    Length);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

    Length = 0;
    if (Info->Sid != NULL)
        Length = RtlLengthSid(Info->Sid);

    Status = LsapSetObjectAttribute(PolicyObject,
                                    L"PolPrDmS",
                                    (LPBYTE)Info->Sid,
                                    Length);

    return Status;
}


NTSTATUS
LsarSetAccountDomain(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_ACCOUNT_DOM_INFO Info)
{
    PUNICODE_STRING Buffer;
    ULONG Length = 0;
    NTSTATUS Status;
    LPWSTR Ptr;

    TRACE("LsarSetAccountDomain(%p, %p)\n", PolicyObject, Info);

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

    Status = LsapSetObjectAttribute(PolicyObject,
                                    L"PolAcDmN",
                                    Buffer,
                                    Length);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

    Length = 0;
    if (Info->Sid != NULL)
        Length = RtlLengthSid(Info->Sid);

    Status = LsapSetObjectAttribute(PolicyObject,
                                    L"PolAcDmS",
                                    (LPBYTE)Info->Sid,
                                    Length);

    return Status;
}


NTSTATUS
LsarSetDnsDomain(PLSA_DB_OBJECT PolicyObject,
                 PLSAPR_POLICY_DNS_DOMAIN_INFO Info)
{

    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsarQueryAuditEvents(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSAPR_POLICY_AUDIT_EVENTS_INFO p = NULL;

    p = MIDL_user_allocate(sizeof(LSAPR_POLICY_AUDIT_EVENTS_INFO));
    if (p == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    p->AuditingMode = FALSE; /* no auditing */
    p->EventAuditingOptions = NULL;
    p->MaximumAuditEventCount = 0;

    *PolicyInformation = (PLSAPR_POLICY_INFORMATION)p;

    return STATUS_SUCCESS;
}


NTSTATUS
LsarQueryPrimaryDomain(PLSA_DB_OBJECT PolicyObject,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSAPR_POLICY_PRIMARY_DOM_INFO p = NULL;
    PUNICODE_STRING DomainName;
    ULONG AttributeSize;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    p = MIDL_user_allocate(sizeof(LSAPR_POLICY_PRIMARY_DOM_INFO));
    if (p == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Domain Name */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolPrDmN",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        goto Done;
    }

    if (AttributeSize > 0)
    {
        DomainName = MIDL_user_allocate(AttributeSize);
        if (DomainName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolPrDmN",
                                        DomainName,
                                        &AttributeSize);
        if (Status == STATUS_SUCCESS)
        {
            DomainName->Buffer = (LPWSTR)((ULONG_PTR)DomainName + (ULONG_PTR)DomainName->Buffer);

            TRACE("PrimaryDomainName: %wZ\n", DomainName);

            p->Name.Buffer = MIDL_user_allocate(DomainName->MaximumLength);
            if (p->Name.Buffer == NULL)
            {
                MIDL_user_free(DomainName);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Done;
            }

            p->Name.Length = DomainName->Length;
            p->Name.MaximumLength = DomainName->MaximumLength;
            memcpy(p->Name.Buffer,
                   DomainName->Buffer,
                   DomainName->MaximumLength);
        }

        MIDL_user_free(DomainName);
    }

    /* Domain SID */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolPrDmS",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        goto Done;
    }

    if (AttributeSize > 0)
    {
        p->Sid = MIDL_user_allocate(AttributeSize);
        if (p->Sid == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolPrDmS",
                                        p->Sid,
                                        &AttributeSize);
    }

    *PolicyInformation = (PLSAPR_POLICY_INFORMATION)p;

Done:
    if (!NT_SUCCESS(Status))
    {
        if (p)
        {
            if (p->Name.Buffer)
                MIDL_user_free(p->Name.Buffer);

            if (p->Sid)
                MIDL_user_free(p->Sid);

            MIDL_user_free(p);
        }
    }

    return Status;
}


NTSTATUS
LsarQueryAccountDomain(PLSA_DB_OBJECT PolicyObject,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSAPR_POLICY_ACCOUNT_DOM_INFO p = NULL;
    PUNICODE_STRING DomainName;
    ULONG AttributeSize = 0;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    p = MIDL_user_allocate(sizeof(LSAPR_POLICY_ACCOUNT_DOM_INFO));
    if (p == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Domain Name */
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAcDmN",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        goto Done;
    }

    if (AttributeSize > 0)
    {
        DomainName = MIDL_user_allocate(AttributeSize);
        if (DomainName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolAcDmN",
                                        DomainName,
                                        &AttributeSize);
        if (Status == STATUS_SUCCESS)
        {
            DomainName->Buffer = (LPWSTR)((ULONG_PTR)DomainName + (ULONG_PTR)DomainName->Buffer);

            TRACE("AccountDomainName: %wZ\n", DomainName);

            p->DomainName.Buffer = MIDL_user_allocate(DomainName->MaximumLength);
            if (p->DomainName.Buffer == NULL)
            {
                MIDL_user_free(DomainName);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Done;
            }

            p->DomainName.Length = DomainName->Length;
            p->DomainName.MaximumLength = DomainName->MaximumLength;
            memcpy(p->DomainName.Buffer,
                   DomainName->Buffer,
                   DomainName->MaximumLength);
        }

        MIDL_user_free(DomainName);
    }

    /* Domain SID */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAcDmS",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        goto Done;
    }

    if (AttributeSize > 0)
    {
        p->Sid = MIDL_user_allocate(AttributeSize);
        if (p->Sid == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolAcDmS",
                                        p->Sid,
                                        &AttributeSize);
    }

    *PolicyInformation = (PLSAPR_POLICY_INFORMATION)p;

Done:
    if (!NT_SUCCESS(Status))
    {
        if (p)
        {
            if (p->DomainName.Buffer)
                MIDL_user_free(p->DomainName.Buffer);

            if (p->Sid)
                MIDL_user_free(p->Sid);

            MIDL_user_free(p);
        }
    }

    return Status;
}


NTSTATUS
LsarQueryDnsDomain(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSAPR_POLICY_DNS_DOMAIN_INFO p = NULL;

    p = MIDL_user_allocate(sizeof(LSAPR_POLICY_DNS_DOMAIN_INFO));
    if (p == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    p->Name.Length = 0;
    p->Name.MaximumLength = 0;
    p->Name.Buffer = NULL;
#if 0
            p->Name.Length = wcslen(L"COMPUTERNAME");
            p->Name.MaximumLength = p->Name.Length + sizeof(WCHAR);
            p->Name.Buffer = MIDL_user_allocate(p->Name.MaximumLength);
            if (p->Name.Buffer == NULL)
            {
                MIDL_user_free(p);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            wcscpy(p->Name.Buffer, L"COMPUTERNAME");
#endif

    p->DnsDomainName.Length = 0;
    p->DnsDomainName.MaximumLength = 0;
    p->DnsDomainName.Buffer = NULL;

    p->DnsForestName.Length = 0;
    p->DnsForestName.MaximumLength = 0;
    p->DnsForestName.Buffer = 0;

    memset(&p->DomainGuid, 0, sizeof(GUID));

    p->Sid = NULL; /* no domain, no workgroup */

    *PolicyInformation = (PLSAPR_POLICY_INFORMATION)p;

    return STATUS_SUCCESS;
}

/* EOF */
