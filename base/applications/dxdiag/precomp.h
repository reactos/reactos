#ifndef PRECOMP_H__
#define PRECOMP_H__

#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0800
#define D3D_OVERLOADS

#include <stdio.h>
#include <windows.h>
#include <limits.h>
#include <mmsystem.h>
#include <setupapi.h>
#include <commctrl.h>
#include <dinput.h>
#include <d3d9.h>
#include <ddraw.h>


#include <dsound.h>
#include <mmreg.h>
#include <wintrust.h>
#include <softpub.h>
#include <mscat.h>
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

/* dialog wnd proc */
INT_PTR CALLBACK SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DisplayPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MusicPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InputPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NetworkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HelpPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/* DirectDraw tests */
VOID DDTests();

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
#endif
