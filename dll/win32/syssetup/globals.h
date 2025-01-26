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
    HWND hwndDlg;
} ITEMSDATA, *PITEMSDATA;

typedef struct _REGISTRATIONNOTIFY
{
    ULONG Progress;
    UINT ActivityID;
    LPCWSTR CurrentItem;
    LPCWSTR ErrorMessage;
    UINT MessageID;
    DWORD LastError;
} REGISTRATIONNOTIFY, *PREGISTRATIONNOTIFY;


#define PM_REGISTRATION_NOTIFY (WM_APP + 1)
/* Private Message used to communicate progress from the background
   registration thread to the main thread.
   wParam = 0 Registration in progress
          = 1 Registration completed
   lParam = Pointer to a REGISTRATIONNOTIFY structure */

#define PM_ITEM_START (WM_APP + 2)
/* Start of a new Item
   wParam = item number
   lParam = number of steps */

#define PM_ITEM_END   (WM_APP + 3)
/* End of a new Item
   wParam = unused
   lParam = Error Code */

#define PM_STEP_START (WM_APP + 4)
#define PM_STEP_END   (WM_APP + 5)
#define PM_ITEMS_DONE (WM_APP + 6)


extern HINSTANCE hDllInstance;
extern HINF hSysSetupInf;
extern ADMIN_INFO AdminInfo;

BOOL RegisterTypeLibraries (HINF hinf, LPCWSTR szSection);

/* install */

VOID
InstallStartMenuItems(
    _In_ PITEMSDATA pItemsData);

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
