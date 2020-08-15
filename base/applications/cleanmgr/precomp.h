/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Precompiled header file
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#define COBJMACROS

#include <windows.h>
#include <windowsx.h>
#include <wchar.h>
#include <Shlwapi.h>
#include <Commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strsafe.h>
#include <string.h>
#include <guiddef.h>
#include <inttypes.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <cpl.h>

typedef struct
{
	uint64_t tempASize;
	uint64_t tempBSize;
	uint64_t recyclebinSize;
	uint64_t downloadedSize;
	uint64_t rappsSize;
} DIRSIZE;

typedef struct
{
	HWND hwndDlg;
	HWND hChoicePage;
	HWND hOptionsPage;
	HWND hTab;
	HWND hTestCtrl;
	HINSTANCE hInst;
} DLG_VAR;

typedef struct
{
	WCHAR* unit;
	WCHAR tempDir[MAX_PATH];
	WCHAR alttempDir[MAX_PATH];
	WCHAR recyclebinDir[MAX_PATH];
	WCHAR downloadDir[MAX_PATH];
	WCHAR rappsDir[MAX_PATH];
	WCHAR recycleBin[MAX_PATH];
	WCHAR driveLetter[MAX_PATH];
} WCHAR_VAR;

typedef struct
{
	BOOL tempClean;
	BOOL recycleClean;
	BOOL downloadClean;
	BOOL rappsClean;
	BOOL sysDrive;
} BOOL_VAR;

typedef enum
{
	DOWNLOADED_FILES = 0,
	RAPPS_FILES = 1,
	TEMPORARY_FILE = 2,
	RECYCLE_BIN = 3
} DIRECTORIES;

// For dialog.c

BOOL CALLBACK StartDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProgressDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ChoiceDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProgressEndDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);


// For util.c
uint64_t dirSizeFunc(PWCHAR targetDir);
double setOptimalSize(uint64_t size);
BOOL findRecycleBin(PWCHAR drive);
void addItem(HWND hList, PWCHAR string, PWCHAR subString, int iIndex);
HRESULT GetFolderCLSID(LPCWSTR path, SHDESCRIPTIONID* pdid);
PWCHAR setOptimalUnit(uint64_t size);
void setDetails(UINT stringID, UINT resourceID, HWND hwnd);
void setTotalSize(long long size, UINT resourceID, HWND hwnd);
BOOL OnCreate(HWND hWnd);
void MsConfig_OnTabWndSelChange(void);
LRESULT APIENTRY ThemeHandler(HWND hDlg, NMCUSTOMDRAW* pNmDraw);
void createImageLists(HWND hList);
void selItem(int index, HWND hwnd);
long long checkedItem(int index, HWND hwnd, HWND hList, long long size);
BOOL EnableDialogTheme(HWND hwnd);
DWORD WINAPI folderRemoval(LPVOID lpParam);
DWORD WINAPI sizeCheck(LPVOID lpParam);
BOOL argCheck(LPWSTR* argList, int nArgs);

// Dlg pages
INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
