/*
 * PROJECT:     ReactOS netapi32.dll API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Tests for DsRoleGetPrimaryDomainInformation
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
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
    ok(pInfo->MachineRole >= DsRole_RoleStandaloneWorkstation && pInfo->MachineRole <= DsRole_RolePrimaryDomainController, "pInfo->MachineRole is %lu!\n", pInfo->MachineRole);

    if (pInfo)
        DsRoleFreeMemory(pInfo);
}
