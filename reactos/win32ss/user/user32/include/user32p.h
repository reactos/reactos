/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/user32/include/user32p.h
 * PURPOSE:         Win32 User Library Private Headers
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#pragma once

/* Private User32 Headers */
#include "controls.h"
#include "dde_private.h"
#include "regcontrol.h"
#include "resource.h"
#include "ntwrapper.h"


typedef struct
{
    BOOL (WINAPI* pImmIsIME) (HKL);
    LRESULT (WINAPI* pImmEscapeA) (HKL, HIMC, UINT, LPVOID);
    LRESULT (WINAPI* pImmEscapeW) (HKL, HIMC, UINT, LPVOID);
    LONG (WINAPI* pImmGetCompositionStringA) (HIMC, DWORD, LPVOID, DWORD);
    LONG (WINAPI* pImmGetCompositionStringW) (HIMC, DWORD, LPVOID, DWORD);
    BOOL (WINAPI* pImmGetCompositionFontA) (HIMC, LPLOGFONTA);
    BOOL (WINAPI* pImmGetCompositionFontW) (HIMC, LPLOGFONTW);
    BOOL (WINAPI* pImmSetCompositionFontA)(HIMC, LPLOGFONTA);
    BOOL (WINAPI* pImmSetCompositionFontW)(HIMC, LPLOGFONTW);
    BOOL (WINAPI* pImmGetCompositionWindow) (HIMC, LPCOMPOSITIONFORM);
    BOOL (WINAPI* pImmSetCompositionWindow) (HIMC, LPCOMPOSITIONFORM);
    HIMC (WINAPI* pImmAssociateContext) (HWND, HIMC);
    BOOL (WINAPI* pImmReleaseContext) (HWND, HIMC);
    HIMC (WINAPI* pImmGetContext) (HWND);
    HWND (WINAPI* pImmGetDefaultIMEWnd) (HWND);
    BOOL (WINAPI* pImmNotifyIME) (HIMC, DWORD, DWORD, DWORD);
    BOOL (WINAPI* pImmRegisterClient) (PVOID, HINSTANCE);
    UINT (WINAPI* pImmProcessKey) (HWND, HKL, UINT, LPARAM, DWORD);

} Imm32ApiTable;


/* global variables */
extern HINSTANCE User32Instance;
#define user32_module User32Instance
extern PPROCESSINFO g_ppi;
extern ULONG_PTR g_ulSharedDelta;
extern PSERVERINFO gpsi;
extern SHAREDINFO gSharedInfo;
extern BOOLEAN gfLogonProcess;
extern BOOLEAN gfServerProcess;
extern PUSER_HANDLE_TABLE gHandleTable;
extern PUSER_HANDLE_ENTRY gHandleEntries;
extern CRITICAL_SECTION U32AccelCacheLock;
extern HINSTANCE ghImm32;
extern RTL_CRITICAL_SECTION gcsUserApiHook;
extern USERAPIHOOK guah;
extern HINSTANCE ghmodUserApiHook;
extern HICON hIconSmWindows, hIconWindows;
extern Imm32ApiTable gImmApiEntries;

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

/* FIXME: move to a correct header */
/* undocumented gdi32 definitions */
BOOL WINAPI GdiDllInitialize(HANDLE, DWORD, LPVOID);
LONG WINAPI GdiGetCharDimensions(HDC, LPTEXTMETRICW, LONG *);

/* definitions for spy.c */
#define SPY_DISPATCHMESSAGE       0x0101
#define SPY_SENDMESSAGE           0x0103
#define SPY_DEFWNDPROC            0x0105
#define SPY_RESULT_OK             0x0001
#define SPY_RESULT_INVALIDHWND    0x0003
#define SPY_RESULT_DEFWND         0x0005
extern const char *SPY_GetMsgName(UINT msg, HWND hWnd);
extern const char *SPY_GetVKeyName(WPARAM wParam);
extern void SPY_EnterMessage(INT iFlag, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void SPY_ExitMessage(INT iFlag, HWND hwnd, UINT msg, LRESULT lReturn, WPARAM wParam, LPARAM lParam);

/* definitions for usrapihk.c */
BOOL FASTCALL BeginIfHookedUserApiHook(VOID);
BOOL FASTCALL EndUserApiHook(VOID);
BOOL FASTCALL IsInsideUserApiHook(VOID);
VOID FASTCALL ResetUserApiHook(PUSERAPIHOOK);
BOOL FASTCALL IsMsgOverride(UINT,PUAHOWP);
BOOL WINAPI InitUserApiHook(HINSTANCE hInstance, USERAPIHOOKPROC pfn);
BOOL WINAPI ClearUserApiHook(HINSTANCE hInstance);

/* definitions for message.c */
BOOL FASTCALL MessageInit(VOID);
VOID FASTCALL MessageCleanup(VOID);

/* definitions for misc.c */
VOID WINAPI UserSetLastError(IN DWORD dwErrCode);
VOID WINAPI UserSetLastNTError(IN NTSTATUS Status);
PCALLPROCDATA FASTCALL ValidateCallProc(HANDLE hCallProc);
PWND FASTCALL ValidateHwnd(HWND hwnd);
PWND FASTCALL ValidateHwndOrDesk(HWND hwnd);
PWND FASTCALL GetThreadDesktopWnd(VOID);
PVOID FASTCALL ValidateHandleNoErr(HANDLE handle, UINT uType);
PWND FASTCALL ValidateHwndNoErr(HWND hwnd);
BOOL FASTCALL TestWindowProcess(PWND);
PVOID FASTCALL ValidateHandle(HANDLE, UINT);

/* definitions for menu.c */
BOOL MenuInit(VOID);
VOID MenuCleanup(VOID);
UINT MenuDrawMenuBar(HDC hDC, LPRECT Rect, HWND hWnd, BOOL Draw);
VOID MenuTrackMouseMenuBar(HWND hWnd, ULONG Ht, POINT Pt);
VOID MenuTrackKbdMenuBar(HWND hWnd, UINT wParam, WCHAR wChar);

/* definitions for logon.c */
VOID FASTCALL Logon(BOOL IsLogon);

/* misc definitions */
void mirror_rect( const RECT *window_rect, RECT *rect );
BOOL FASTCALL DefSetText(HWND hWnd, PCWSTR String, BOOL Ansi);
VOID FASTCALL ScrollTrackScrollBar(HWND Wnd, INT SBType, POINT Pt);
HCURSOR CursorIconToCursor(HICON hIcon, BOOL SemiTransparent);
BOOL get_icon_size(HICON hIcon, SIZE *size);
VOID FASTCALL IntNotifyWinEvent(DWORD, HWND, LONG, LONG, DWORD);
UINT WINAPI WinPosGetMinMaxInfo(HWND hWnd, POINT* MaxSize, POINT* MaxPos, POINT* MinTrack, POINT* MaxTrack);
VOID UserGetWindowBorders(DWORD, DWORD, SIZE *, BOOL);
void UserGetInsideRectNC(PWND Wnd, RECT *rect);
VOID FASTCALL GetConnected(VOID);
extern BOOL FASTCALL EnumNamesA(HWINSTA WindowStation, NAMEENUMPROCA EnumFunc, LPARAM Context, BOOL Desktops);
extern BOOL FASTCALL EnumNamesW(HWINSTA WindowStation, NAMEENUMPROCW EnumFunc, LPARAM Context, BOOL Desktops);
BOOL UserDrawSysMenuButton( HWND hWnd, HDC hDC, LPRECT, BOOL down );
HWND* WIN_ListChildren (HWND hWndparent);
VOID DeleteFrameBrushes(VOID);
BOOL WINAPI GdiValidateHandle(HGDIOBJ);
HANDLE FASTCALL UserGetProp(HWND hWnd, ATOM Atom, BOOLEAN SystemProp);
BOOL WINAPI InitializeImmEntryTable(VOID);

/* EOF */
