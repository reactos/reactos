/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    internat.h

Abstract:

    This module contains the header information for the Multilingual
    Language Indicator application.

Revision History:

--*/



//
//  Include Files.
//

#define STRICT
#define OEMRESOURCE
#define _INC_OLE
#define INITGUID

#include <windows.h>
#include <windowsx.h>
#include <docobj.h>
#include <shldisp.h>
#include <shlobj.h>
#include <winreg.h>
#include <commctrl.h>
#include <indicml.h>
#include <indicmlp.h>
#include <help.h>
#include "resource.h"
#include "..\share.h"
#include <immp.h>

#include <setupapi.h>



//
//  Constant Declarations.
//

#define MENUSTRLEN                64

#define CREATE_MLNGINFO           0x01
#define UPDATE_MLNGINFO           0x02
#define DESTROY_MLNGINFO          0x03

#define LANG_INDICATOR_ID         0xdf
#define IME_INDICATOR_ID          0xe0

#define SRCSTENCIL                0x00b8074aL

#define WM_LANGUAGE_INDICATOR     (WM_APP + 100)
#define WM_IME_INDICATOR          (WM_APP + 101)
#define WM_MYSETOPENSTATUS        (WM_APP + 102)
#define WM_IME_INDICATOR2         (WM_APP + 103)

#define TIMER_MYLANGUAGECHECK     1

#define MMIF_IME                  0x0001
#define MMIF_USE_ICON_INDEX       0x0002
#define MMIF_USE_ATOM_TIP         0x0004
#define MMIF_REMOVEDEFAULTMENUITEMS   0x0008
#define MMIF_REMOVEDEFAULTRIGHTMENUITEMS   0x0010



//
//  Typedef Declarations.
//

typedef struct
{
    HKL dwHkl;
    int nIconIndex;
    DWORD dwIMEFlag;
    ATOM atomTip;
    UINT uIMEIconIndex;
    TCHAR szTip[1];

} MLNGINFO, *PMLNGINFO;




//
//  Global Variables.
//

extern HKL        g_dwCurrentHkl;
extern BOOL       g_bIndicatorPresent;
extern HIMAGELIST g_himIndicatorList;
extern HDPA       g_hdpaMlngInfoList;
extern HWND       hWndTray;
extern HWND       hWndNotify;
extern HINSTANCE  g_hinst;

extern HINSTANCE  hInstLib;
extern REGHOOKPROC fpRegHookWindow;
extern PROC       fpStartShell;
extern PROC       fpStopShell;

extern BOOL       g_bInLangMenu;
extern HWND       g_hWndForLang;

extern int        nIMEIconIndex[8];    // eight states for now
extern BOOL       g_bIMEIndicator;

static TCHAR      szNotifyWindow[] = TEXT("TrayNotifyWnd");
static TCHAR      szDllName[]      = TEXT("INDICDLL");




//
//  Function Prototypes.
//

LRESULT CALLBACK
Internat_MainWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
Internat_InitApplication(
    HINSTANCE hInstance);

BOOL
Internat_InitInstance(
    HINSTANCE hInstance,
    int nCmdShow);

int APIENTRY
Internat_WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpAnsiCmdLine,
    int nCmdShow);

void
Internat_Destroy(
    HWND hwnd);

void
Internat_onSettingChange(
    HWND hwnd,
    BOOL fForce);

BOOL CALLBACK
Internat_EnumChildWndProc(
    HWND hwnd,
    LPARAM lParam);

int
Internat_OnCreate(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam);

void
Internat_SendLangIndicatorMsg(
    HWND hwnd,
    HKL dwHkl,
    DWORD dwMessage);

void
Internat_LanguageIndicator(
    HWND hwnd,
    DWORD dwFlag);

HICON
Internat_GetIconFromFile(
    HIMAGELIST himIndicators,
    LPTSTR lpszFileName,
    UINT uIconIndex);

HICON
Internat_CreateIcon(
    HWND hwnd,
    WORD langID);

void
Internat_ManageMlngInfo(
    HWND hwnd,
    WORD wFlag);

int
Internat_HandleLanguageMsg(
    HWND hwnd,
    HKL dwHkl);

BOOL
Internat_HandleLangMenuMeasure(
    HWND hwnd,
    LPMEASUREITEMSTRUCT lpmi);

BOOL
Internat_HandleLangMenuDraw(
    HWND hwnd,
    LPDRAWITEMSTRUCT lpdi);

void
Internat_CreateLanguageMenu(
    HWND hwnd,
    LPARAM lParam);

void
Internat_CreateOtherIndicatorMenu(
    HWND hwnd,
    LPARAM lParam);

HWND
Internat_GetCurrentFocusWnd(void);

void
Internat_LoadIMEIndicatorIcon(
    HINSTANCE hInstLib,
    int *ImeIcon);

void
Internat_SendIMEIndicatorMsg(
    HWND hwnd,
    HKL dwHkl,
    DWORD dwMessage);

void
Internat_CreateRightImeMenu(
    HWND hwnd);

void
Internat_CreateImeMenu(
    HWND hwnd);

BOOL
Internat_GetIMEShowStatus(void);

BOOL
Internat_SetIMEShowStatus(
    HWND hwnd,
    BOOL fShow);

int
Internat_GetIMEStatus(
    HWND *phwndFocus);

HKL
Internat_GetLayout(void);

HKL
Internat_GetKeyboardLayout(
    HWND hwnd);

void
Internat_BuildIMEMenu(
    HMENU hMenu,
    BOOL fRight,
    BOOL fRemoveDefault);

void
Internat_DestroyIMEMenu(void);

BOOL
Internat_GetIMEMenu(
    HWND hwndIMC,
    BOOL bRight);

UINT
Internat_GetIMEMenuItemID(
    int nMenuID);

void CALLBACK
Internat_TimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT_PTR idEvent,
    DWORD dwTime);

void
Internat_SetIMEOpenStatus(
    HWND hwnd,
    BOOL fopen,
    HWND hwndIMC);

BOOL
Internat_CallIMEHelp(
    HWND hwnd,
    BOOL fCallWinHelp);

void
Internat_CallConfigureIME(
    HWND hwnd,
    HKL dwhkl);

HWND
Internat_GetTopLevelWindow(
    HWND hwnd);
    
int 
Internat_GetDefaultImeMenuItem(void);

void 
Internat_SerializeActivate(
    HWND hwnd,
    HWND hwndIMC);
