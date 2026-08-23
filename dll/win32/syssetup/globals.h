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


typedef struct _ITEMSDATA
{
    PSETUPDATA pSetupData;
    HWND hwndDlg;
} ITEMSDATA, *PITEMSDATA;


/*
 * Private messages used to communicate progress from the
 * background installation thread to the main thread.
 */

typedef struct _INSTALLITEM_NOTIFY
{
    ULONG Progress;
    UINT ActivityID;
    LPCWSTR CurrentItem;
    LPCWSTR ErrorMessage;
    UINT MessageID;
    DWORD LastError;
} INSTALLITEM_NOTIFY, *PINSTALLITEM_NOTIFY;

/* Start of a new Item.
 * wParam = item number.
 * lParam = number of steps. */
#define PM_ITEM_START (WM_APP + 1)

/* End of a new Item.
 * wParam = item number.
 * lParam = error code. */
#define PM_ITEM_END   (WM_APP + 2)

/* Start/End of an installation step for an Item.
 * wParam = unused (but usually set to the item number).
 * lParam = pointer to an INSTALLITEM_NOTIFY structure. */
#define PM_STEP_START (WM_APP + 3)
#define PM_STEP_END   (WM_APP + 4)

#define PM_ITEMS_DONE (WM_APP + 5)


extern HINSTANCE hDllInstance;
extern HINF hSysSetupInf;
extern ADMIN_INFO AdminInfo;

/* addons.c */
HRESULT
InstallOptionalComponents(
    _In_ PITEMSDATA pItemsData);

HRESULT
RunCommandAndWait(
    _In_ PWCHAR Command);

/* install.c */
BOOL
RegisterTypeLibraries(
    _In_ PITEMSDATA pItemsData,
    _In_ PINSTALLITEM_NOTIFY pNotify,
    _In_ HINF hinf,
    _In_ LPCWSTR szSection);

VOID
InstallStartMenuItems(
    _In_ PITEMSDATA pItemsData);

/* netinstall.c */
BOOL
InstallNetworkComponent(
    _In_ PWSTR pszComponentId);

/* security.c */
LONG
CountSecuritySteps(VOID);

DWORD
InstallSecurity(
    _In_ PITEMSDATA pItemsData,
    _In_ PINSTALLITEM_NOTIFY pNotify);

NTSTATUS
SetAdministratorPassword(LPCWSTR Password);

VOID
SetAutoAdminLogon(VOID);

/* wizard.c */
VOID
InstallWizard(VOID);

VOID
GetSetupInfPath(PWSTR szPath, UINT cchMax);

/* EOF */
