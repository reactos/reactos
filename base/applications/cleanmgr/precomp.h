/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Precompiled header file
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#ifndef _CLEANMGR_PRECOMP_H
#define _CLEANMGR_PRECOMP_H

#define ARR_MAX_SIZE 512

#include "resource.h"

#include <windows.h>
#include <windowsx.h>
#include <wchar.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <strsafe.h>
#include <inttypes.h>
#include <debug.h>
#include <shlobj.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <wininet.h>
#include <winreg.h>

typedef struct
{
    uint64_t TempSize;
    uint64_t RecycleBinSize;
    uint64_t ChkDskSize;
    uint64_t RappsSize;
    uint64_t CountSize;
} DIRSIZE;

typedef struct
{
    HWND hChoicePage;
    HWND hOptionsPage;
    HWND hSagesetPage;
    HWND hTab;
} DLG_HANDLE;

extern BOOL IsSystemDrive;
extern UINT CleanmgrWindowMsg;
extern WCHAR DriveLetter[ARR_MAX_SIZE];
extern WCHAR RappsDir[MAX_PATH];


// For dialog.c
INT_PTR CALLBACK StartDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgressDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChoiceDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgressEndDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SetStageFlagDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


// For util.c
BOOL ArgCheck(LPWSTR* ArgList, int nArgs);
BOOL DrawItemCombobox(LPARAM lParam);
BOOL InitTabControl(HWND hwnd);
BOOL InitStageFlagTabControl(HWND hwnd);

DWORD WINAPI RemoveRequiredFolder(LPVOID lpParam);
DWORD WINAPI GetRemovableDirSize(LPVOID lpParam);

uint64_t CheckedItem(int ItemIndex, HWND hwnd, HWND hList, long long size);

LRESULT APIENTRY ThemeHandler(HWND hDlg, NMCUSTOMDRAW* pNmDraw);

PWCHAR GetProperDriveLetter(HWND hComboCtrl, int ItemIndex);

void InitStartDlg(HWND hwnd, HBITMAP hBitmap);
void InitListViewControl(HWND hList);
void InitStageFlagListViewControl(HWND hList);
void SelItem(HWND hwnd, int ItemIndex);
void StageFlagCheckedItem(int ItemIndex, HWND hwnd, HWND hList);
void TabControlSelChange(void);


// Dlg pages
INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsPageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SetStageFlagPageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif /* !_CLEANMGR_PRECOMP_H */
