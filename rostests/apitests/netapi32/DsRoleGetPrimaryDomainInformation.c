/*
 * PROJECT:     ReactOS netapi32.dll API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for DsRoleGetPrimaryDomainInformation
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <dsrole.h>

START_TEST(DsRoleGetPrimaryDomainInformation)
{
    DWORD dwErrorCode;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo = NULL;

    // Get information about the domain membership of this computer.
    dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE*)&pInfo);
    ok(dwErrorCode == ERROR_SUCCESS, "DsRoleGetPrimaryDomainInformation returns %lu!\n", dwErrorCode);
    ok(pInfo->MachineRole >= DsRole_RoleStandaloneWorkstation && pInfo->MachineRole <= DsRole_RolePrimaryDomainController, "pInfo->MachineRole is %u!\n", pInfo->MachineRole);

    if (pInfo)
        DsRoleFreeMemory(pInfo);
}
