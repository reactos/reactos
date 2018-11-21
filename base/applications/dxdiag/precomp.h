#ifndef PRECOMP_H__
#define PRECOMP_H__

#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0800
#define D3D_OVERLOADS

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <winuser.h>
#include <setupapi.h>
#include <ddraw.h>
#include <initguid.h>
#include <devguid.h>
#include <strsafe.h>
#include <udmihelp.h>

#include "resource.h"

typedef struct
{
    HWND hMainDialog;
    HWND hTabCtrl;
    ULONG NumDisplayAdapter;
    HWND * hDisplayWnd;
    ULONG NumSoundAdapter;
    HWND * hSoundWnd;
    HWND hDialogs[5];
}DXDIAG_CONTEXT, *PDXDIAG_CONTEXT;

/* globals */
extern HINSTANCE hInst;

/* theming hack */
BOOL EnableDialogTheme(HWND hwnd);

/* dialog wnd proc */
INT_PTR CALLBACK SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DisplayPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MusicPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InputPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NetworkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HelpPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/* DirectDraw tests */
VOID DDTests(VOID);

/* Direct3D tests */
VOID D3DTests(VOID);

/* DirectSound initialization */
void InitializeDirectSoundPage(PDXDIAG_CONTEXT pContext);

/* display adapter initialization */
void InitializeDisplayAdapters(PDXDIAG_CONTEXT pContext);

/* general functions */
BOOL GetFileVersion(LPCWSTR szAppName, WCHAR * szVer, DWORD szVerSize);
BOOL GetFileModifyTime(LPCWSTR pFullPath, WCHAR * szTime, int szTimeSize);
BOOL GetCatFileFromDriverPath(LPWSTR szFileName, LPWSTR szCatFileName);
BOOL GetRegValue(HKEY hBaseKey, LPWSTR SubKey, LPWSTR ValueName, DWORD Type, LPWSTR Result, DWORD Size);
VOID InsertTabCtrlItem(HWND hDlgCtrl, INT Position, LPWSTR uId);
VOID EnumerateDrivers(PVOID Context, HDEVINFO hList, PSP_DEVINFO_DATA pInfoData);

#endif /* PRECOMP_H__ */
