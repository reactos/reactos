/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Precompiled header file
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#ifndef _CLEANMGR_PRECOMP_H
#define _CLEANMGR_PRECOMP_H
 
#define COBJMACROS
#define ONLY_DRIVE 3

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
    HWND hwndDlg;
    HWND hChoicePage;
    HWND hOptionsPage;
    HWND hSagesetPage;
    HWND hTab;
    HINSTANCE hInst;
} DLG_VAR;

typedef struct
{
    WCHAR TempDir[MAX_PATH];
    WCHAR AltTempDir[MAX_PATH];
    WCHAR RecycleBinDir[MAX_PATH];
    WCHAR RappsDir[MAX_PATH];
    WCHAR RecycleBin[MAX_PATH];
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

typedef enum
{
    OLD_CHKDSK_FILES = 0,
    RAPPS_FILES = 1,
    TEMPORARY_FILE = 2,
    RECYCLE_BIN = 3
} DIRECTORIES;

/* HACK: struct for gathering data from registry key */

typedef struct
{
    BOOL bSaveWndPos;
    BOOL bUpdateAtStart;
    BOOL bLogEnabled;
    WCHAR szDownloadDir[MAX_PATH];
    BOOL bDelInstaller;
    /* Window Pos */
    BOOL Maximized;
    INT Left;
    INT Top;
    INT Width;
    INT Height;
    /* Proxy settings */
    INT Proxy;
    WCHAR szProxyServer[MAX_PATH];
    WCHAR szNoProxyFor[MAX_PATH];
    /* Software source settings */
    BOOL bUseSource;
    WCHAR szSourceURL[INTERNET_MAX_URL_LENGTH];
} SETTINGS_INFO;


// For dialog.c

BOOL CALLBACK StartDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProgressDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ChoiceDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProgressEndDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SagesetDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

// For util.c
uint64_t DirSizeFunc(PWCHAR targetDir);
double SetOptimalSize(uint64_t size);
BOOL FindRecycleBin(PWCHAR drive);
void AddItem(HWND hList, PWCHAR string, PWCHAR subString, int iIndex);
HRESULT GetFolderCLSID(LPCWSTR path, SHDESCRIPTIONID* pdid);
PWCHAR SetOptimalUnit(uint64_t size);
void SetDetails(UINT stringID, UINT resourceID, HWND hwnd);
void SetTotalSize(long long size, UINT resourceID, HWND hwnd);
BOOL OnCreate(HWND hwnd);
BOOL OnCreateSageset(HWND hwnd);
void OnTabWndSelChange(void);
LRESULT APIENTRY ThemeHandler(HWND hDlg, NMCUSTOMDRAW* pNmDraw);
BOOL CreateImageLists(HWND hList);
void SelItem(HWND hwnd, int index);
long long CheckedItem(int index, HWND hwnd, HWND hList, long long size);
void SagesetCheckedItem(int index, HWND hwnd, HWND hList);
BOOL EnableDialogTheme(HWND hwnd);
DWORD WINAPI FolderRemoval(LPVOID lpParam);
DWORD WINAPI SizeCheck(LPVOID lpParam);
BOOL ArgCheck(LPWSTR* argList, int nArgs);
BOOL DriverunProc(LPWSTR* argList, PWCHAR LogicalDrives);
void SagesetProc(int nArgs, PWCHAR ArgReal, LPWSTR* argList);
void SagerunProc(int nArgs, PWCHAR ArgReal, LPWSTR* argList, PWCHAR LogicalDrives);
BOOL RegValSet(PWCHAR RegArg, PWCHAR SubKey, BOOL ArgBool);
DWORD RegQuery(PWCHAR RegArg, PWCHAR SubKey);
PWCHAR RealStageFlag(int nArgs, PWCHAR ArgReal, LPWSTR* argList);

// Dlg pages
INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SagesetPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

#endif /* !_CLEANMGR_PRECOMP_H */