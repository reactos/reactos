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
    RECYCLE_BIN = 2,
    TEMPORARY_FILE = 3
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

BOOL ArgCheck(LPWSTR* argList, int nArgs);
BOOL CreateImageLists(HWND hList);
BOOL DriverunProc(LPWSTR* argList, PWCHAR LogicalDrives);
BOOL EnableDialogTheme(HWND hwnd);
BOOL FindRecycleBin(PWCHAR drive);
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

PWCHAR RealStageFlag(int nArgs, PWCHAR ArgReal, LPWSTR* argList);
PWCHAR SetOptimalUnit(uint64_t size);

uint64_t DirSizeFunc(PWCHAR targetDir);

void AddItem(HWND hList, PWCHAR string, PWCHAR subString, int iIndex);
void CleanRequiredPath(PWCHAR Path);
void OnTabWndSelChange(void);
void SagesetCheckedItem(int index, HWND hwnd, HWND hList);
void SagesetProc(int nArgs, PWCHAR ArgReal, LPWSTR* argList);
void SagerunProc(int nArgs, PWCHAR ArgReal, LPWSTR* argList, PWCHAR LogicalDrives);
void SelItem(HWND hwnd, int index);
void SetDetails(UINT stringID, UINT resourceID, HWND hwnd);
void SetTotalSize(long long size, UINT resourceID, HWND hwnd);


// Dlg pages
INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SagesetPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

#endif /* !_CLEANMGR_PRECOMP_H */