/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/lsasrv.c
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2006-2009 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

NT_PRODUCT_TYPE LsapProductType = NtProductWinNt;

/* FUNCTIONS ***************************************************************/

VOID
NTAPI
LsaIFree_LSAPR_ACCOUNT_ENUM_BUFFER(
    IN PLSAPR_ACCOUNT_ENUM_BUFFER Ptr)
{
    ULONG i;

    if (Ptr == NULL)
        return;

    if (Ptr->Information != NULL)
    {
        for (i = 0; i < Ptr->EntriesRead; i++)
            midl_user_free(Ptr->Information[i].Sid);

        midl_user_free(Ptr->Information);
    }

    midl_user_free(Ptr);
}


VOID
NTAPI
LsaIFree_LSAPR_CR_CIPHER_VALUE(
    IN PLSAPR_CR_CIPHER_VALUE Ptr)
{
    if (Ptr != NULL)
        midl_user_free(Ptr);
}


VOID
NTAPI
LsaIFree_LSAPR_POLICY_INFORMATION(IN POLICY_INFORMATION_CLASS InformationClass,
                                  IN PLSAPR_POLICY_INFORMATION PolicyInformation)
{
    if (PolicyInformation != NULL)
    {
        switch (InformationClass)
        {
            case PolicyAuditLogInformation:      /* 1 */
                break;

            case PolicyAuditEventsInformation:   /* 2 */
                if (PolicyInformation->PolicyAuditEventsInfo.EventAuditingOptions != NULL)
                    midl_user_free(PolicyInformation->PolicyAuditEventsInfo.EventAuditingOptions);
                break;

            case PolicyPrimaryDomainInformation: /* 3 */
                if (PolicyInformation->PolicyPrimaryDomInfo.Name.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyPrimaryDomInfo.Name.Buffer);

                if (PolicyInformation->PolicyPrimaryDomInfo.Sid != NULL)
                    midl_user_free(PolicyInformation->PolicyPrimaryDomInfo.Sid);
                break;

            case PolicyPdAccountInformation:     /* 4 */
                if (PolicyInformation->PolicyPdAccountInfo.Name.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyPdAccountInfo.Name.Buffer);
                break;

            case PolicyAccountDomainInformation: /* 5 */
                if (PolicyInformation->PolicyAccountDomainInfo.DomainName.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyAccountDomainInfo.DomainName.Buffer);

                if (PolicyInformation->PolicyAccountDomainInfo.Sid != NULL)
                    midl_user_free(PolicyInformation->PolicyAccountDomainInfo.Sid);
                break;

            case PolicyLsaServerRoleInformation: /* 6 */
                break;

            case PolicyReplicaSourceInformation: /* 7 */
                if (PolicyInformation->PolicyReplicaSourceInfo.ReplicaSource.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyReplicaSourceInfo.ReplicaSource.Buffer);

                if (PolicyInformation->PolicyReplicaSourceInfo.ReplicaAccountName.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyReplicaSourceInfo.ReplicaAccountName.Buffer);
                break;

            case PolicyDefaultQuotaInformation:  /* 8 */
                break;

            case PolicyModificationInformation:  /* 9 */
                break;

            case PolicyAuditFullSetInformation:  /* 10 (0xA) */
                break;

            case PolicyAuditFullQueryInformation: /* 11 (0xB) */
                break;

            case PolicyDnsDomainInformation:      /* 12 (0xC) */
                if (PolicyInformation->PolicyDnsDomainInfo.Name.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfo.Name.Buffer);

                if (PolicyInformation->PolicyDnsDomainInfo.DnsDomainName.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfo.DnsDomainName.Buffer);

                if (PolicyInformation->PolicyDnsDomainInfo.DnsForestName.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfo.DnsForestName.Buffer);

                if (PolicyInformation->PolicyDnsDomainInfo.Sid != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfo.Sid);
                break;

            case PolicyDnsDomainInformationInt:   /* 13 (0xD) */
                if (PolicyInformation->PolicyDnsDomainInfoInt.Name.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfoInt.Name.Buffer);

                if (PolicyInformation->PolicyDnsDomainInfoInt.DnsDomainName.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfoInt.DnsDomainName.Buffer);

                if (PolicyInformation->PolicyDnsDomainInfoInt.DnsForestName.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfoInt.DnsForestName.Buffer);

                if (PolicyInformation->PolicyDnsDomainInfoInt.Sid != NULL)
                    midl_user_free(PolicyInformation->PolicyDnsDomainInfoInt.Sid);
                break;

            case PolicyLocalAccountDomainInformation: /* 14 (0xE) */
                if (PolicyInformation->PolicyLocalAccountDomainInfo.DomainName.Buffer != NULL)
                    midl_user_free(PolicyInformation->PolicyLocalAccountDomainInfo.DomainName.Buffer);

                if (PolicyInformation->PolicyLocalAccountDomainInfo.Sid != NULL)
                    midl_user_free(PolicyInformation->PolicyLocalAccountDomainInfo.Sid);
                break;

            default:
                ERR("Invalid InformationClass: %lu\n", InformationClass);
                break;
        }

        midl_user_free(PolicyInformation);
    }
}


VOID
NTAPI
LsaIFree_LSAPR_PRIVILEGE_ENUM_BUFFER(
    IN PLSAPR_PRIVILEGE_ENUM_BUFFER Ptr)
{
    ULONG i;

    if (Ptr != NULL)
    {
        if (Ptr->Privileges != NULL)
        {
            for (i = 0; i < Ptr->Entries; i++)
            {
                if (Ptr->Privileges[i].Name.Buffer != NULL)
                    midl_user_free(Ptr->Privileges[i].Name.Buffer);
            }

            midl_user_free(Ptr->Privileges);
        }
    }
}


VOID
NTAPI
LsaIFree_LSAPR_PRIVILEGE_SET(IN PLSAPR_PRIVILEGE_SET Ptr)
{
    if (Ptr != NULL)
    {
        midl_user_free(Ptr);
    }
}


VOID
NTAPI
LsaIFree_LSAPR_REFERENCED_DOMAIN_LIST(
    IN PLSAPR_REFERENCED_DOMAIN_LIST Ptr)
{
    ULONG i;

    if (Ptr != NULL)
    {
        if (Ptr->Domains != NULL)
        {
            for (i = 0; i < Ptr->Entries; i++)
            {
                if (Ptr->Domains[i].Name.Buffer != NULL)
                     midl_user_free(Ptr->Domains[i].Name.Buffer);

                if (Ptr->Domains[i].Sid != NULL)
                    midl_user_free(Ptr->Domains[i].Sid);
            }

            midl_user_free(Ptr->Domains);
        }

        midl_user_free(Ptr);
    }
}


VOID
NTAPI
LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR(
    IN PLSAPR_SR_SECURITY_DESCRIPTOR Ptr)
{
    if (Ptr != NULL)
    {
        if (Ptr->SecurityDescriptor != NULL)
            midl_user_free(Ptr->SecurityDescriptor);

        midl_user_free(Ptr);
    }
}


VOID
NTAPI
LsaIFree_LSAPR_TRANSLATED_NAMES(
    IN PLSAPR_TRANSLATED_NAMES Ptr)
{
    ULONG i;

    if (Ptr != NULL)
    {
        if (Ptr->Names != NULL)
        {
            for (i = 0; i < Ptr->Entries; i++)
            {
                if (Ptr->Names[i].Name.Buffer != NULL)
                    midl_user_free(Ptr->Names[i].Name.Buffer);
            }

            midl_user_free(Ptr->Names);
        }

        midl_user_free(Ptr);
    }
}


VOID
NTAPI
LsaIFree_LSAPR_TRANSLATED_SIDS(
    IN PLSAPR_TRANSLATED_SIDS Ptr)
{
    if (Ptr != NULL)
    {
        if (Ptr->Sids != NULL)
            midl_user_free(Ptr->Sids);

        midl_user_free(Ptr);
    }
}


NTSTATUS
WINAPI
LsapInitLsa(VOID)
{
    NTSTATUS Status;
    BOOLEAN PrivilegeEnabled;

    TRACE("LsapInitLsa()\n");

    /* Get the product type */
    RtlGetNtProductType(&LsapProductType);

    /* Initialize the well known SIDs */
    LsapInitSids();

    /* Initialize the SRM server */
    Status = LsapRmInitializeServer();
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapRmInitializeServer() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Initialize the LSA database */
    LsapInitDatabase();

    /* Initialize logon sessions */
    LsapInitLogonSessions();

    /* Initialize the notification list */
    LsapInitNotificationList();

    /* Initialize registered authentication packages */
    Status = LsapInitAuthPackages();
    if (!NT_SUCCESS(Status))
    {
        ERR("LsapInitAuthPackages() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Enable the token creation privilege for the rest of our lifetime */
    Status = RtlAdjustPrivilege(SE_CREATE_TOKEN_PRIVILEGE, TRUE, FALSE, &PrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        ERR("RtlAdjustPrivilege(SE_CREATE_TOKEN_PRIVILEGE) failed, ignoring (Status 0x%08lx)\n", Status);
    }

    /* Start the authentication LPC port thread */
    Status = StartAuthenticationPort();
    if (!NT_SUCCESS(Status))
    {
        ERR("StartAuthenticationPort() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Start the RPC server */
    Status = LsarStartRpcServer();
    if (!NT_SUCCESS(Status))
    {
        ERR("LsarStartRpcServer() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, ptr);
}

/* EOF */
