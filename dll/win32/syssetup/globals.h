/*
 * Copyright (C) 2004 Eric Kohl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

typedef struct _ADMIN_INFO
{
    LPWSTR Name;
    LPWSTR Domain;
    LPWSTR Password;
} ADMIN_INFO, *PADMIN_INFO;

extern HINSTANCE hDllInstance;
extern HINF hSysSetupInf;
extern ADMIN_INFO AdminInfo;

BOOL RegisterTypeLibraries (HINF hinf, LPCWSTR szSection);

/* netinstall.c */

BOOL
InstallNetworkComponent(
    _In_ PWSTR pszComponentId);

/* security.c */

VOID InstallSecurity(VOID);
NTSTATUS
SetAdministratorPassword(LPCWSTR Password);

VOID
SetAutoAdminLogon(VOID);

/* wizard.c */
VOID InstallWizard (VOID);

/* EOF */
