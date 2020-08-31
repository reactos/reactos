/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Precompiled header file
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#ifndef _CLEANMGR_PRECOMP_H
#define _CLEANMGR_PRECOMP_H
 
#define COBJMACROS
#define ONLY_PHYSICAL_DRIVE 3

#include <windows.h>
#include <windowsx.h>
#include <wchar.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strsafe.h>
#include <string.h>
#include <guiddef.h>
#include <inttypes.h>
#include <debug.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <wininet.h>
#include <cpl.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winreg.h>

typedef struct
{
    uint64_t TempASize;
    uint64_t TempBSize;
    uint64_t RecycleBinSize;
    uint64_t ChkDskSize;
    uint64_t RappsSize;
} DIRSIZE;

typedef struct
{
    HWND hChoicePage;
    HWND hOptionsPage;
    HWND hSagesetPage;
    HWND hTab;
} DLG_VAR;

typedef struct
{
    WCHAR RappsDir[MAX_PATH];
    WCHAR DriveLetter[MAX_PATH];
} WCHAR_VAR;

typedef struct
{
    BOOL TempClean;
    BOOL RecycleClean;
    BOOL ChkDskClean;
    BOOL RappsClean;
    BOOL SysDrive;
} BOOL_VAR;

DIRSIZE sz;
DLG_VAR dv;
WCHAR_VAR wcv;
BOOL_VAR bv;

/* HACK: struct for gathering data from registry key */

/*typedef struct
{
    BOOL bSaveWndPos;
    BOOL bUpdateAtStart;
    BOOL bLogEnabled;
    WCHAR szDownloadDir[MAX_PATH];
    BOOL bDelInstaller;

    BOOL Maximized;
    INT Left;
    INT Top;
    INT Width;
    INT Height;

    INT Proxy;
    WCHAR szProxyServer[MAX_PATH];
    WCHAR szNoProxyFor[MAX_PATH];

    BOOL bUseSource;
    WCHAR szSourceURL[INTERNET_MAX_URL_LENGTH];
} SETTINGS_INFO; */


// For dialog.c

INT_PTR CALLBACK StartDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgressDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChoiceDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgressEndDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SagesetDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// For util.c

BOOL ArgCheck(LPWSTR* argList, int nArgs);
BOOL CreateImageLists(HWND hList);
BOOL DrawItemCombobox(LPARAM lParam);
BOOL DriverunProc(LPWSTR* argList, PWCHAR LogicalDrives);
BOOL EnableDialogTheme(HWND hwnd);
BOOL OnCreate(HWND hwnd);
BOOL OnCreateSageset(HWND hwnd);
BOOL RegValSet(PWCHAR RegArg, PWCHAR SubKey, BOOL ArgBool);

double SetOptimalSize(uint64_t size);

DWORD WINAPI FolderRemoval(LPVOID lpParam);
DWORD WINAPI SizeCheck(LPVOID lpParam);
DWORD RegQuery(PWCHAR RegArg, PWCHAR SubKey);

HRESULT GetFolderCLSID(LPCWSTR path, SHDESCRIPTIONID* pdid);

long long CheckedItem(int index, HWND hwnd, HWND hList, long long size);

LRESULT APIENTRY ThemeHandler(HWND hDlg, NMCUSTOMDRAW* pNmDraw);

PWCHAR FindRecycleBin(PWCHAR TargetDrive);
PWCHAR RealStageFlag(int nArgs, PWCHAR ArgReal, LPWSTR* argList);
PWCHAR SetOptimalUnit(uint64_t size);

uint64_t DirSizeFunc(PWCHAR targetDir);

void AddItem(HWND hList, PWCHAR string, PWCHAR subString, int iIndex);
void CleanRequiredPath(PWCHAR Path);
void InitListViewControl(HWND hList);
void InitSagesetListViewControl(HWND hList);
void OnTabWndSelChange(void);
void SagesetCheckedItem(int index, HWND hwnd, HWND hList);
void SagesetProc(int nArgs, PWCHAR ArgReal, LPWSTR* argList);
void SagerunProc(int nArgs, PWCHAR ArgReal, LPWSTR* argList, PWCHAR LogicalDrives);
void SelItem(HWND hwnd, int index);
void SetDetails(UINT stringID, UINT resourceID, HWND hwnd);
void SetTotalSize(long long size, UINT resourceID, HWND hwnd);


// Dlg pages
INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsPageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SagesetPageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif /* !_CLEANMGR_PRECOMP_H */