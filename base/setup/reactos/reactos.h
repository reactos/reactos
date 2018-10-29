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
/**/#include <setupapi.h>/**/
#include <devguid.h>

#define NTOS_MODE_USER
#include <ndk/cmtypes.h> // For CM_DISK stuff
#include <ndk/iofuncs.h> // For NtCreate/OpenFile
#include <ndk/rtlfuncs.h>


/* Setup library headers */
// #include <reactos/rosioctl.h>
#include <../lib/setuplib.h>

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

    TCHAR szAbortMessage[512];
    TCHAR szAbortTitle[64];

    USETUP_DATA USetupData;

    BOOLEAN RepairUpdateFlag; // flag for update/repair an installed reactos

    PPARTLIST PartitionList;
    PNTOS_INSTALLATION CurrentInstallation;
    PGENERIC_LIST NtOsInstallsList;


    /* Settings */
    LONG DestPartSize; // if partition doesn't exist, size of partition
    LONG FSType; // file system type on partition 
    LONG FormatPart; // type of format the partition

    LONG SelectedLangId; // selected language (table index)
    LONG SelectedKBLayout; // selected keyboard layout (table index)
    LONG SelectedComputer; // selected computer type (table index)
    LONG SelectedDisplay; // selected display type (table index)
    LONG SelectedKeyboard; // selected keyboard type (table index)

    /* txtsetup.sif data */
    // LONG DefaultLang; // default language (table index)
    // LONG DefaultKBLayout; // default keyboard layout (table index)
    PCWSTR SelectedLanguageId;
    WCHAR DefaultLanguage[20];   // Copy of string inside LanguageList
    WCHAR DefaultKBLayout[20];   // Copy of string inside KeyboardList

} SETUPDATA, *PSETUPDATA;

extern HANDLE ProcessHeap;
extern BOOLEAN IsUnattendedSetup;


typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
} IMGINFO, *PIMGINFO;


/*
 * Attempts to convert a pure NT file path into a corresponding Win32 path.
 * Adapted from GetInstallSourceWin32() in dll/win32/syssetup/wizard.c
 */
BOOL
ConvertNtPathToWin32Path(
    OUT PWSTR pwszPath,
    IN DWORD cchPathMax,
    IN PCWSTR pwszNTPath);


/* drivepage.c */

BOOL
CreateListViewColumns(
    IN HINSTANCE hInstance,
    IN HWND hWndListView,
    IN const UINT* pIDs,
    IN const INT* pColsWidth,
    IN const INT* pColsAlign,
    IN UINT nNumOfColumns);

INT_PTR
CALLBACK
DriveDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

#endif /* _REACTOS_PCH_ */

/* EOP */
