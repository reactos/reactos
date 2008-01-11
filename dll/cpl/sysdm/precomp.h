#ifndef __CPL_PRECOMP_H
#define __CPL_PRECOMP_H

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <powrprof.h>
#include <tchar.h>
#include <stdio.h>
#include <cpl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlguid.h>
#include <shlobj.h>
#include <cplext.h>
#include <regstr.h>
#include <setupapi.h>
#include "resource.h"

#define NUM_APPLETS (1)

typedef LONG (CALLBACK *APPLET_INITPROC)(VOID);

typedef struct _APPLET
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_INITPROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

/* prop sheet pages */
INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HardwarePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* dialogs */
INT_PTR CALLBACK HardProfDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UserProfileDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EnvironmentDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK StartRecDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK VirtMemDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LicenceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

typedef struct _PAGEFILE
{
    TCHAR szDrive[3];
    INT   InitialValue;
    INT   MaxValue;
    BOOL  bUsed;
} PAGEFILE, *PPAGEFILE;

typedef struct _VIRTMEM
{
    HWND   hSelf;
    HWND   hListBox;
    LPTSTR szPagingFiles;
    TCHAR  szDrive[10];
    INT    Count;
    BOOL   bSave;
    PAGEFILE  Pagefile[26];
} VIRTMEM, *PVIRTMEM;

typedef struct _BOOTRECORD
{
  DWORD BootType;
  TCHAR szSectionName[128];
  TCHAR szBootPath[MAX_PATH];
  TCHAR szOptions[512];

}BOOTRECORD, *PBOOTRECORD;

#endif /* __CPL_SYSDM_H */
