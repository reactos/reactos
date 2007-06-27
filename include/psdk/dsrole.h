/*
 * Directory Services definitions
 *
 * Copyright 2005 Paul Vriens
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DSROLE_H
#define __WINE_DSROLE_H

#define DSROLE_PRIMARY_DS_RUNNING           0x00000001
#define DSROLE_PRIMARY_DS_MIXED_MODE        0x00000002
#define DSROLE_UPGRADE_IN_PROGRESS          0x00000004
#define DSROLE_PRIMARY_DOMAIN_GUID_PRESENT  0x01000000

typedef enum _DSROLE_PRIMARY_DOMAIN_INFO_LEVEL
{
    DsRolePrimaryDomainInfoBasic = 1,
    DsRoleUpgradeStatus,
    DsRoleOperationState
} DSROLE_PRIMARY_DOMAIN_INFO_LEVEL;

typedef enum _DSROLE_MACHINE_ROLE
{
  DsRole_RoleStandaloneWorkstation = 0,
  DsRole_RoleMemberWorkstation,
  DsRole_RoleStandaloneServer,
  DsRole_RoleMemberServer,
  DsRole_RoleBackupDomainController,
  DsRole_RolePrimaryDomainController
} DSROLE_MACHINE_ROLE;

typedef enum _DSROLE_SERVER_STATE
{
  DsRoleServerUnknown = 0,
  DsRoleServerPrimary,
  DsRoleServerBackup
} DSROLE_SERVER_STATE;

typedef enum _DSROLE_OPERATION_STATE
{
  DsRoleOperationIdle = 0,
  DsRoleOperationActive,
  DsRoleOperationNeedReboot
} DSROLE_OPERATION_STATE;

typedef struct _DSROLE_PRIMARY_DOMAIN_INFO_BASIC
{
    DSROLE_MACHINE_ROLE MachineRole;
    ULONG Flags;
    LPWSTR DomainNameFlat;
    LPWSTR DomainNameDns;
    LPWSTR DomainForestName;
    GUID DomainGuid;
} DSROLE_PRIMARY_DOMAIN_INFO_BASIC, *PDSROLE_PRIMARY_DOMAIN_INFO_BASIC;

typedef struct _DSROLE_UPGRADE_STATUS_INFO
{
    ULONG OperationState;
    DSROLE_SERVER_STATE PreviousServerState;
} DSROLE_UPGRADE_STATUS_INFO, *PDSROLE_UPGRADE_STATUS_INFO;

typedef struct _DSROLE_OPERATION_STATE_INFO
{
    DSROLE_OPERATION_STATE OperationState;
} DSROLE_OPERATION_STATE_INFO, *PDSROLE_OPERATION_STATE_INFO;

VOID WINAPI DsRoleFreeMemory(IN PVOID Buffer);
DWORD WINAPI DsRoleGetPrimaryDomainInformation(IN LPCWSTR lpServer OPTIONAL, IN DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel, OUT PBYTE *Buffer);

#endif /* __WINE_DSROLE_H */
