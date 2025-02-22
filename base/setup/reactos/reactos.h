/*
 *  ReactOS applications
 *  Copyright (C) 2004-2008 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS GUI first stage setup application
 * FILE:        base/setup/reactos/reactos.c
 * PROGRAMMERS: Matthias Kupfer
 *              Dmitry Chapyshev (dmitry@reactos.org)
 */

#ifndef _REACTOS_PCH_
#define _REACTOS_PCH_

/* C Headers */
#include <stdlib.h>
#include <stdarg.h>
#include <tchar.h>

/* PSDK/NDK */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>

#include <strsafe.h>

#include <commctrl.h>
#include <windowsx.h>

#define EnableDlgItem(hDlg, nID, bEnable)   \
    EnableWindow(GetDlgItem((hDlg), (nID)), (bEnable))

#define ShowDlgItem(hDlg, nID, nCmdShow)    \
    ShowWindow(GetDlgItem((hDlg), (nID)), (nCmdShow))

#define SetDlgItemFont(hDlg, nID, hFont, bRedraw)   \
    SetWindowFont(GetDlgItem((hDlg), (nID)), (hFont), (bRedraw))

/* These are public names and values determined from MFC, and compatible with Windows */
// Property Sheet control id's (determined with Spy++)
#define IDC_TAB_CONTROL                 0x3020
#define ID_APPLY_NOW                    0x3021
#define ID_WIZBACK                      0x3023
#define ID_WIZNEXT                      0x3024
#define ID_WIZFINISH                    0x3025


#include "treelist.h"

/**/#include <setupapi.h>/**/
#include <devguid.h>

#define NTOS_MODE_USER
#include <ndk/cmtypes.h> // For CM_DISK stuff
#include <ndk/iofuncs.h> // For NtCreate/OpenFile
#include <ndk/rtlfuncs.h>


/* Setup library headers */
// #include <reactos/rosioctl.h>
#include <../lib/setuplib.h>


/* UI elements */
typedef struct _UI_CONTEXT
{
    HWND hPartList; // Disks & partitions list
    HWND hwndDlg;   // Install progress page
    HWND hWndItem;  // Progress action
    HWND hWndProgress;  // Progress gauge
    LONG_PTR dwPbStyle; // Progress gauge style
} UI_CONTEXT, *PUI_CONTEXT;

extern UI_CONTEXT UiContext;


/*
 * A mapping entry that maps an NT path to a corresponding Win32 path.
 *
 * Example is:
 *   NT path:    "\Device\Harddisk0\Partition1\some\path1"
 *   Win32 path: "C:\some\path1"
 *
 * Here, the NT path prefix to be cached is only
 * "\Device\Harddisk0\Partition1\", to be mapped with "C:\".
 *
 * Then the same entry would be reused if one wants to convert
 * the NT path "\Device\Harddisk0\Partition1\another\path2",
 * which converts to the Win32 path "C:\another\path2" .
 */
typedef struct _NT_WIN32_PATH_MAPPING
{
    LIST_ENTRY ListEntry;
    WCHAR NtPath[MAX_PATH]; // MAX_PATH for both entries should be more than enough.
    WCHAR Win32Path[MAX_PATH];
} NT_WIN32_PATH_MAPPING, *PNT_WIN32_PATH_MAPPING;

/* The list of NT to Win32 path prefix mappings */
typedef struct _NT_WIN32_PATH_MAPPING_LIST
{
    LIST_ENTRY List;
    ULONG MappingsCount;
} NT_WIN32_PATH_MAPPING_LIST, *PNT_WIN32_PATH_MAPPING_LIST;


#if 0
typedef struct _KBLAYOUT
{
    TCHAR LayoutId[9];
    TCHAR LayoutName[128];
    TCHAR DllName[128];
} KBLAYOUT, *PKBLAYOUT;
#endif

typedef struct _SETUPDATA
{
    /* General */
    HINSTANCE hInstance;
    BOOL bUnattend;

    HFONT hTitleFont;
    HFONT hBoldFont;

    HANDLE hInstallThread;
    HANDLE hHaltInstallEvent;
    BOOL bStopInstall;

    NT_WIN32_PATH_MAPPING_LIST MappingList;

    USETUP_DATA USetupData;

    BOOLEAN RepairUpdateFlag; // flag for update/repair an installed reactos

    PPARTLIST PartitionList;
    PNTOS_INSTALLATION CurrentInstallation;
    PGENERIC_LIST NtOsInstallsList;


    /* Settings */
    LONG DestPartSize; // if partition doesn't exist, size of partition

    /* txtsetup.sif data */
    // LONG DefaultLang;     // default language (table index)
    // LONG DefaultKBLayout; // default keyboard layout (table index)
    PCWSTR SelectedLanguageId;
    WCHAR DefaultLanguage[20];   // Copy of string inside LanguageList
    WCHAR DefaultKBLayout[20];   // Copy of string inside KeyboardList

} SETUPDATA, *PSETUPDATA;

extern HANDLE ProcessHeap;
extern SETUPDATA SetupData;

extern PPARTENTRY InstallPartition;
extern PPARTENTRY SystemPartition;

/**
 * @brief   Data structure stored when a partition/volume needs to be formatted.
 **/
typedef struct _VOL_CREATE_INFO
{
    PVOLENTRY Volume;

    /* Volume-related parameters:
     * Cached input information that will be set to
     * the FORMAT_VOLUME_INFO structure given to the
     * 'FSVOLNOTIFY_STARTFORMAT' step */
    // PCWSTR FileSystemName;
    WCHAR FileSystemName[MAX_PATH+1];
    FMIFS_MEDIA_FLAG MediaFlag;
    PCWSTR Label;
    BOOLEAN QuickFormat;
    ULONG ClusterSize;
} VOL_CREATE_INFO, *PVOL_CREATE_INFO;

/* See drivepage.c */
PVOL_CREATE_INFO
FindVolCreateInTreeByVolume(
    _In_ HWND hTreeList,
    _In_ PVOLENTRY Volume);


/*
 * Attempts to convert a pure NT file path into a corresponding Win32 path.
 * Adapted from GetInstallSourceWin32() in dll/win32/syssetup/wizard.c
 */
BOOL
ConvertNtPathToWin32Path(
    IN OUT PNT_WIN32_PATH_MAPPING_LIST MappingList,
    OUT PWSTR pwszPath,
    IN DWORD cchPathMax,
    IN PCWSTR pwszNTPath);


/* drivepage.c */

INT_PTR
CALLBACK
DriveDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);


/* reactos.c */

BOOL
CreateListViewColumns(
    IN HINSTANCE hInstance,
    IN HWND hWndListView,
    IN const UINT* pIDs,
    IN const INT* pColsWidth,
    IN const INT* pColsAlign,
    IN UINT nNumOfColumns);

INT
DisplayMessageV(
    _In_opt_ HWND hWnd,
    _In_ UINT uType,
    _In_opt_ PCWSTR pszTitle,
    _In_opt_ PCWSTR pszFormatMessage,
    _In_ va_list args);

INT
__cdecl
DisplayMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT uType,
    _In_opt_ PCWSTR pszTitle,
    _In_opt_ PCWSTR pszFormatMessage,
    ...);

INT
__cdecl
DisplayError(
    _In_opt_ HWND hWnd,
    _In_ UINT uIDTitle,
    _In_ UINT uIDMessage,
    ...);

VOID
SetWindowResTextW(
    _In_ HWND hWnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ UINT uID);

VOID
SetWindowResPrintfVW(
    _In_ HWND hWnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ UINT uID,
    _In_ va_list args);

VOID
__cdecl
SetWindowResPrintfW(
    _In_ HWND hWnd,
    _In_opt_ HINSTANCE hInstance,
    _In_ UINT uID,
    ...);

#endif /* _REACTOS_PCH_ */

/* EOF */
