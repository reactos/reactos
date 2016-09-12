/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/policy.c
 * PURPOSE:     Policy object routines
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

#include "lsasrv.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
WINAPI
LsaIOpenPolicyTrusted(OUT LSAPR_HANDLE *PolicyHandle)
{
    PLSA_DB_OBJECT PolicyObject;
    NTSTATUS Status;

    TRACE("(%p)\n", PolicyHandle);

    Status = LsapOpenDbObject(NULL,
                              NULL,
                              L"Policy",
                              LsaDbPolicyObject,
                              POLICY_ALL_ACCESS,
                              TRUE,
                              &PolicyObject);

    if (NT_SUCCESS(Status))
        *PolicyHandle = (LSAPR_HANDLE)PolicyObject;

    return Status;
}


NTSTATUS
LsarQueryAuditLog(PLSA_DB_OBJECT PolicyObject,
                  PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PPOLICY_AUDIT_LOG_INFO AuditLogInfo = NULL;
    ULONG AttributeSize;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    AttributeSize = sizeof(POLICY_AUDIT_LOG_INFO);
    AuditLogInfo = MIDL_user_allocate(AttributeSize);
    if (AuditLogInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAdtLg",
                                    AuditLogInfo,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(AuditLogInfo);
    }
    else
    {
        *PolicyInformation = (PLSAPR_POLICY_INFORMATION)AuditLogInfo;
    }

    return Status;
}


NTSTATUS
LsarQueryAuditEvents(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSAP_POLICY_AUDIT_EVENTS_DATA AuditData = NULL;
    PLSAPR_POLICY_AUDIT_EVENTS_INFO p = NULL;
    ULONG AttributeSize;
    NTSTATUS Status = STATUS_SUCCESS;

    *PolicyInformation = NULL;

    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAdtEv",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
        return Status;

    TRACE("Attribute size: %lu\n", AttributeSize);
    if (AttributeSize > 0)
    {
        AuditData = MIDL_user_allocate(AttributeSize);
        if (AuditData == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolAdtEv",
                                        AuditData,
                                        &AttributeSize);
        if (!NT_SUCCESS(Status))
            goto done;
    }

    p = MIDL_user_allocate(sizeof(LSAPR_POLICY_AUDIT_EVENTS_INFO));
    if (p == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    if (AuditData == NULL)
    {
        p->AuditingMode = FALSE;
        p->MaximumAuditEventCount = 0;
        p->EventAuditingOptions = NULL;
    }
    else
    {
        p->AuditingMode = AuditData->AuditingMode;
        p->MaximumAuditEventCount = AuditData->MaximumAuditEventCount;

        p->EventAuditingOptions = MIDL_user_allocate(AuditData->MaximumAuditEventCount * sizeof(DWORD));
        if (p->EventAuditingOptions == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        memcpy(p->EventAuditingOptions,
               &(AuditData->AuditEvents[0]),
               AuditData->MaximumAuditEventCount * sizeof(DWORD));
    }

    *PolicyInformation = (PLSAPR_POLICY_INFORMATION)p;

done:
    TRACE("Status: 0x%lx\n", Status);

    if (!NT_SUCCESS(Status))
    {
        if (p != NULL)
        {
            if (p->EventAuditingOptions != NULL)
                MIDL_user_free(p->EventAuditingOptions);

            MIDL_user_free(p);
        }
    }

    if (AuditData != NULL)
        MIDL_user_free(AuditData);

    return Status;
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
        if (p != NULL)
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
LsarQueryPdAccount(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSAPR_POLICY_PD_ACCOUNT_INFO PdAccountInfo = NULL;

    *PolicyInformation = NULL;

    PdAccountInfo = MIDL_user_allocate(sizeof(LSAPR_POLICY_PD_ACCOUNT_INFO));
    if (PdAccountInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    PdAccountInfo->Name.Length = 0;
    PdAccountInfo->Name.MaximumLength = 0;
    PdAccountInfo->Name.Buffer = NULL;

    *PolicyInformation = (PLSAPR_POLICY_INFORMATION)PdAccountInfo;

    return STATUS_SUCCESS;
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
LsarQueryServerRole(PLSA_DB_OBJECT PolicyObject,
                    PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PPOLICY_LSA_SERVER_ROLE_INFO ServerRoleInfo = NULL;
    ULONG AttributeSize;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    AttributeSize = sizeof(POLICY_LSA_SERVER_ROLE_INFO);
    ServerRoleInfo = MIDL_user_allocate(AttributeSize);
    if (ServerRoleInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolSrvRo",
                                    ServerRoleInfo,
                                    &AttributeSize);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ServerRoleInfo->LsaServerRole = PolicyServerRolePrimary;
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(ServerRoleInfo);
    }
    else
    {
        *PolicyInformation = (PLSAPR_POLICY_INFORMATION)ServerRoleInfo;
    }

    return Status;
}


NTSTATUS
LsarQueryReplicaSource(PLSA_DB_OBJECT PolicyObject,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    FIXME("\n");
    *PolicyInformation = NULL;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsarQueryDefaultQuota(PLSA_DB_OBJECT PolicyObject,
                      PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PPOLICY_DEFAULT_QUOTA_INFO QuotaInfo = NULL;
    ULONG AttributeSize;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    AttributeSize = sizeof(POLICY_DEFAULT_QUOTA_INFO);
    QuotaInfo = MIDL_user_allocate(AttributeSize);
    if (QuotaInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"DefQuota",
                                    QuotaInfo,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(QuotaInfo);
    }
    else
    {
        *PolicyInformation = (PLSAPR_POLICY_INFORMATION)QuotaInfo;
    }

    return Status;
}


NTSTATUS
LsarQueryModification(PLSA_DB_OBJECT PolicyObject,
                      PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PPOLICY_MODIFICATION_INFO Info = NULL;
    ULONG AttributeSize;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    AttributeSize = sizeof(POLICY_MODIFICATION_INFO);
    Info = MIDL_user_allocate(AttributeSize);
    if (Info == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolMod",
                                    Info,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(Info);
    }
    else
    {
        *PolicyInformation = (PLSAPR_POLICY_INFORMATION)Info;
    }

    return Status;
}


NTSTATUS
LsarQueryAuditFull(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PPOLICY_AUDIT_FULL_QUERY_INFO AuditFullInfo = NULL;
    ULONG AttributeSize;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    AttributeSize = sizeof(POLICY_AUDIT_FULL_QUERY_INFO);
    AuditFullInfo = MIDL_user_allocate(AttributeSize);
    if (AuditFullInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAdtFl",
                                    AuditFullInfo,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(AuditFullInfo);
    }
    else
    {
        *PolicyInformation = (PLSAPR_POLICY_INFORMATION)AuditFullInfo;
    }

    return Status;
}


NTSTATUS
LsarQueryDnsDomain(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    PLSAPR_POLICY_DNS_DOMAIN_INFO p = NULL;
    PUNICODE_STRING DomainName;
    ULONG AttributeSize;
    NTSTATUS Status;

    *PolicyInformation = NULL;

    p = MIDL_user_allocate(sizeof(LSAPR_POLICY_DNS_DOMAIN_INFO));
    if (p == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Primary Domain Name */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolPrDmN",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    if (AttributeSize > 0)
    {
        DomainName = MIDL_user_allocate(AttributeSize);
        if (DomainName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
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
                goto done;
            }

            p->Name.Length = DomainName->Length;
            p->Name.MaximumLength = DomainName->MaximumLength;
            memcpy(p->Name.Buffer,
                   DomainName->Buffer,
                   DomainName->MaximumLength);
        }

        MIDL_user_free(DomainName);
    }

    /* Primary Domain SID */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolPrDmS",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    if (AttributeSize > 0)
    {
        p->Sid = MIDL_user_allocate(AttributeSize);
        if (p->Sid == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolPrDmS",
                                        p->Sid,
                                        &AttributeSize);
    }

    /* DNS Domain Name */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolDnDDN",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
        goto done;

    if (AttributeSize > 0)
    {
        DomainName = MIDL_user_allocate(AttributeSize);
        if (DomainName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolDnDDN",
                                        DomainName,
                                        &AttributeSize);
        if (Status == STATUS_SUCCESS)
        {
            DomainName->Buffer = (LPWSTR)((ULONG_PTR)DomainName + (ULONG_PTR)DomainName->Buffer);

            TRACE("DNS Domain Name: %wZ\n", DomainName);

            p->DnsDomainName.Buffer = MIDL_user_allocate(DomainName->MaximumLength);
            if (p->DnsDomainName.Buffer == NULL)
            {
                MIDL_user_free(DomainName);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            p->DnsDomainName.Length = DomainName->Length;
            p->DnsDomainName.MaximumLength = DomainName->MaximumLength;
            memcpy(p->DnsDomainName.Buffer,
                   DomainName->Buffer,
                   DomainName->MaximumLength);
        }

        MIDL_user_free(DomainName);
    }

    /* DNS Forest Name */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolDnTrN",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
        goto done;

    if (AttributeSize > 0)
    {
        DomainName = MIDL_user_allocate(AttributeSize);
        if (DomainName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolDnTrN",
                                        DomainName,
                                        &AttributeSize);
        if (Status == STATUS_SUCCESS)
        {
            DomainName->Buffer = (LPWSTR)((ULONG_PTR)DomainName + (ULONG_PTR)DomainName->Buffer);

            TRACE("DNS Forest Name: %wZ\n", DomainName);

            p->DnsForestName.Buffer = MIDL_user_allocate(DomainName->MaximumLength);
            if (p->DnsForestName.Buffer == NULL)
            {
                MIDL_user_free(DomainName);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            p->DnsForestName.Length = DomainName->Length;
            p->DnsForestName.MaximumLength = DomainName->MaximumLength;
            memcpy(p->DnsForestName.Buffer,
                   DomainName->Buffer,
                   DomainName->MaximumLength);
        }

        MIDL_user_free(DomainName);
    }

    /* DNS Domain GUID */
    AttributeSize = sizeof(GUID);
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolDnDmG",
                                    &(p->DomainGuid),
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
        goto done;

    *PolicyInformation = (PLSAPR_POLICY_INFORMATION)p;

done:
    if (!NT_SUCCESS(Status))
    {
        if (p)
        {
            if (p->Name.Buffer)
                MIDL_user_free(p->Name.Buffer);

            if (p->DnsDomainName.Buffer)
                MIDL_user_free(p->DnsDomainName.Buffer);

            if (p->DnsForestName.Buffer)
                MIDL_user_free(p->DnsForestName.Buffer);

            if (p->Sid)
                MIDL_user_free(p->Sid);

            MIDL_user_free(p);
        }
    }

    return Status;
}


NTSTATUS
LsarQueryDnsDomainInt(PLSA_DB_OBJECT PolicyObject,
                      PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    FIXME("\n");
    *PolicyInformation = NULL;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsarQueryLocalAccountDomain(PLSA_DB_OBJECT PolicyObject,
                            PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    FIXME("\n");
    *PolicyInformation = NULL;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsarSetAuditLog(PLSA_DB_OBJECT PolicyObject,
                PPOLICY_AUDIT_LOG_INFO Info)
{
    TRACE("(%p %p)\n", PolicyObject, Info);

    return LsapSetObjectAttribute(PolicyObject,
                                  L"PolAdtLg",
                                  Info,
                                  sizeof(POLICY_AUDIT_LOG_INFO));
}


NTSTATUS
LsarSetAuditEvents(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_AUDIT_EVENTS_INFO Info)
{
    PLSAP_POLICY_AUDIT_EVENTS_DATA AuditData = NULL;
    ULONG AttributeSize;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%p %p)\n", PolicyObject, Info);

    AttributeSize = sizeof(LSAP_POLICY_AUDIT_EVENTS_DATA) +
                    Info->MaximumAuditEventCount * sizeof(DWORD);

    AuditData = RtlAllocateHeap(RtlGetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                AttributeSize);
    if (AuditData == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    AuditData->AuditingMode = Info->AuditingMode;
    AuditData->MaximumAuditEventCount = Info->MaximumAuditEventCount;

    memcpy(&(AuditData->AuditEvents[0]),
           Info->EventAuditingOptions,
           Info->MaximumAuditEventCount * sizeof(DWORD));

    Status = LsapSetObjectAttribute(PolicyObject,
                                    L"PolAdtEv",
                                    AuditData,
                                    AttributeSize);

    RtlFreeHeap(RtlGetProcessHeap(), 0, AuditData);

    return Status;
}


NTSTATUS
LsarSetPrimaryDomain(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_PRIMARY_DOM_INFO Info)
{
    PUNICODE_STRING Buffer;
    ULONG Length = 0;
    NTSTATUS Status;
    LPWSTR Ptr;

    TRACE("(%p %p)\n", PolicyObject, Info);

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

    TRACE("(%p %p)\n", PolicyObject, Info);

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
LsarSetServerRole(PLSA_DB_OBJECT PolicyObject,
                  PPOLICY_LSA_SERVER_ROLE_INFO Info)
{
    TRACE("(%p %p)\n", PolicyObject, Info);

    return LsapSetObjectAttribute(PolicyObject,
                                  L"PolSrvRo",
                                  Info,
                                  sizeof(POLICY_LSA_SERVER_ROLE_INFO));
}


NTSTATUS
LsarSetReplicaSource(PLSA_DB_OBJECT PolicyObject,
                     PPOLICY_LSA_REPLICA_SRCE_INFO Info)
{
    FIXME("\n");
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsarSetDefaultQuota(PLSA_DB_OBJECT PolicyObject,
                    PPOLICY_DEFAULT_QUOTA_INFO Info)
{
    TRACE("(%p %p)\n", PolicyObject, Info);

    return LsapSetObjectAttribute(PolicyObject,
                                  L"DefQuota",
                                  Info,
                                  sizeof(POLICY_DEFAULT_QUOTA_INFO));
}


NTSTATUS
LsarSetModification(PLSA_DB_OBJECT PolicyObject,
                    PPOLICY_MODIFICATION_INFO Info)
{
    TRACE("(%p %p)\n", PolicyObject, Info);

    return LsapSetObjectAttribute(PolicyObject,
                                  L"PolMod",
                                  Info,
                                  sizeof(POLICY_MODIFICATION_INFO));
}


NTSTATUS
LsarSetAuditFull(PLSA_DB_OBJECT PolicyObject,
                 PPOLICY_AUDIT_FULL_QUERY_INFO Info)
{
    PPOLICY_AUDIT_FULL_QUERY_INFO AuditFullInfo = NULL;
    ULONG AttributeSize;
    NTSTATUS Status;

    TRACE("(%p %p)\n", PolicyObject, Info);

    AttributeSize = sizeof(POLICY_AUDIT_FULL_QUERY_INFO);
    AuditFullInfo = MIDL_user_allocate(AttributeSize);
    if (AuditFullInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAdtFl",
                                    AuditFullInfo,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
        goto done;

    AuditFullInfo->ShutDownOnFull = Info->ShutDownOnFull;

    Status = LsapSetObjectAttribute(PolicyObject,
                                    L"PolAdtFl",
                                    AuditFullInfo,
                                    AttributeSize);

done:
    if (AuditFullInfo != NULL)
        MIDL_user_free(AuditFullInfo);

    return Status;
}


NTSTATUS
LsarSetDnsDomain(PLSA_DB_OBJECT PolicyObject,
                 PLSAPR_POLICY_DNS_DOMAIN_INFO Info)
{
    FIXME("\n");
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsarSetDnsDomainInt(PLSA_DB_OBJECT PolicyObject,
                    PLSAPR_POLICY_DNS_DOMAIN_INFO Info)
{
    FIXME("\n");
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
LsarSetLocalAccountDomain(PLSA_DB_OBJECT PolicyObject,
                          PLSAPR_POLICY_ACCOUNT_DOM_INFO Info)
{
    FIXME("\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
