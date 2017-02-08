/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/dssetup.c
 * PURPOSE:         Directory Service setup functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "lsasrv.h"
#include "dssetup_s.h"
#include "resources.h"

VOID
NTAPI
LsaIFree_LSAPR_POLICY_INFORMATION(IN POLICY_INFORMATION_CLASS InformationClass,
                                  IN PLSAPR_POLICY_INFORMATION PolicyInformation);

/* GLOBALS *****************************************************************/

VOID
DsSetupInit(VOID)
{
    RPC_STATUS Status;

    Status = RpcServerRegisterIf(dssetup_v0_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return;
    }
}


static
NET_API_STATUS
DsRolepGetBasicInfo(
    PDSROLER_PRIMARY_DOMAIN_INFORMATION *DomainInfo)
{
    LSAPR_OBJECT_ATTRIBUTES ObjectAttributes;
    PDSROLER_PRIMARY_DOMAIN_INFO_BASIC Buffer;
    PLSAPR_POLICY_INFORMATION PolicyInfo;
    LSA_HANDLE PolicyHandle;
    ULONG Size;
    NTSTATUS Status;

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    Status = LsarOpenPolicy(NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsarOpenPolicyFailed with NT status %x\n",
              LsaNtStatusToWinError(Status));
        return ERROR_OUTOFMEMORY;
    }

    Status = LsarQueryInformationPolicy(PolicyHandle,
                                        PolicyAccountDomainInformation,
                                        &PolicyInfo);
    LsarClose(&PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsarQueryInformationPolicy with NT status %x\n",
              LsaNtStatusToWinError(Status));
        return ERROR_OUTOFMEMORY;
    }

    Size = sizeof(DSROLER_PRIMARY_DOMAIN_INFO_BASIC) +
           PolicyInfo->PolicyAccountDomainInfo.DomainName.Length + sizeof(WCHAR);

    Buffer = midl_user_allocate(Size);
    if (Buffer == NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                          PolicyInfo);
        return ERROR_OUTOFMEMORY;
    }

    Buffer->MachineRole = DsRole_RoleStandaloneWorkstation;
    Buffer->DomainNameFlat = (LPWSTR)((LPBYTE)Buffer +
                                      sizeof(DSROLER_PRIMARY_DOMAIN_INFO_BASIC));
    wcscpy(Buffer->DomainNameFlat, PolicyInfo->PolicyAccountDomainInfo.DomainName.Buffer);

    LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                      PolicyInfo);

    *DomainInfo = (PDSROLER_PRIMARY_DOMAIN_INFORMATION)Buffer;

    return ERROR_SUCCESS;
}


static
NET_API_STATUS
DsRolepGetUpdateStatus(
    PDSROLER_PRIMARY_DOMAIN_INFORMATION *DomainInfo)
{
    PDSROLE_UPGRADE_STATUS_INFO Buffer;

    Buffer = midl_user_allocate(sizeof(DSROLE_UPGRADE_STATUS_INFO));
    if (Buffer == NULL)
        return ERROR_OUTOFMEMORY;

    Buffer->OperationState = 0;
    Buffer->PreviousServerState = 0;

    *DomainInfo = (PDSROLER_PRIMARY_DOMAIN_INFORMATION)Buffer;

    return ERROR_SUCCESS;
}


static
NET_API_STATUS
DsRolepGetOperationState(
    PDSROLER_PRIMARY_DOMAIN_INFORMATION *DomainInfo)
{
    PDSROLE_OPERATION_STATE_INFO Buffer;

    Buffer = midl_user_allocate(sizeof(DSROLE_OPERATION_STATE_INFO));
    if (Buffer == NULL)
        return ERROR_OUTOFMEMORY;

    Buffer->OperationState = DsRoleOperationIdle;

    *DomainInfo = (PDSROLER_PRIMARY_DOMAIN_INFORMATION)Buffer;

    return ERROR_SUCCESS;
}


DWORD
WINAPI
DsRolerGetPrimaryDomainInformation(
    handle_t hBinding,
    DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    PDSROLER_PRIMARY_DOMAIN_INFORMATION *DomainInfo)
{
    NET_API_STATUS ret;

    TRACE("DsRolerGetPrimaryDomainInformation(%p, %d, %p)\n", 
          hBinding, InfoLevel, DomainInfo);

    switch (InfoLevel)
    {
        case DsRolePrimaryDomainInfoBasic:
            ret = DsRolepGetBasicInfo(DomainInfo);
            break;

        case DsRoleUpgradeStatus:
            ret = DsRolepGetUpdateStatus(DomainInfo);
            break;

        case DsRoleOperationState:
            ret = DsRolepGetOperationState(DomainInfo);
            break;

        default:
            ret = ERROR_CALL_NOT_IMPLEMENTED;
    }

    return ret;
}

/* EOF */
