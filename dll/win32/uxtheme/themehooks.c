/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS uxtheme.dll
 * FILE:            dll/win32/uxtheme/themehooks.c
 * PURPOSE:         uxtheme user api hook functions
 * PROGRAMMER:      Giannis Adamopoulos
 */
 
#include <windows.h>
#include <undocuser.h>

#include "vfwmsgs.h"
#include "uxtheme.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);



extern HINSTANCE hDllInst;

LRESULT
ThemeDefWindowProcAW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, WNDPROC defWndProc, BOOL ANSI);

USERAPIHOOK user32ApiHook;
BYTE gabDWPmessages[UAHOWP_MAX_SIZE];

static LRESULT CALLBACK
ThemeDefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{      
    return ThemeDefWindowProcAW(hWnd, 
	                            Msg, 
								wParam, 
								lParam, 
								user32ApiHook.DefWindowProcW, 
								FALSE);
}

static LRESULT CALLBACK
ThemeDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return ThemeDefWindowProcAW(hWnd, 
	                            Msg, 
								wParam, 
								lParam, 
								user32ApiHook.DefWindowProcA, 
								TRUE);
}

BOOL CALLBACK 
ThemeInitApiHook(UAPIHK State, PUSERAPIHOOK puah)
{
    /* Sanity checks for the caller */
    if (!puah || State != uahLoadInit)
	{
	    return TRUE;
	}
    
	/* Store the original functions from user32 */
	user32ApiHook = *puah;
	
	puah->DefWindowProcA = ThemeDefWindowProcA;
	puah->DefWindowProcW = ThemeDefWindowProcW;
    puah->DefWndProcArray.MsgBitArray  = gabDWPmessages;
    puah->DefWndProcArray.Size = UAHOWP_MAX_SIZE;

	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CREATE);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_DESTROY);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_THEMECHANGED);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCPAINT);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCACTIVATE);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCMOUSEMOVE);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_MOUSEMOVE);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCMOUSELEAVE);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCHITTEST);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCLBUTTONDOWN);
	UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCLBUTTONDBLCLK);
    
    return TRUE;
}

typedef BOOL (WINAPI * PREGISTER_UAH_WINXP)(HINSTANCE hInstance, USERAPIHOOKPROC CallbackFunc);
typedef BOOL (WINAPI * PREGISTER_UUAH_WIN2003)(PUSERAPIHOOKINFO puah);

BOOL WINAPI
ThemeHooksInstall()
{
	PVOID lpFunc;
	OSVERSIONINFO osvi;
		
	lpFunc = GetProcAddress(GetModuleHandle("user32.dll"), "RegisterUserApiHook");
	
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	
	if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
	{
		PREGISTER_UAH_WINXP lpfuncxp = (PREGISTER_UAH_WINXP)lpFunc;
		return lpfuncxp(hDllInst, ThemeInitApiHook);
	}
	else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
	{
	    PREGISTER_UUAH_WIN2003 lpfunc2003 = (PREGISTER_UUAH_WIN2003)lpFunc;
	    USERAPIHOOKINFO uah;
	
		uah.m_size = sizeof(uah);
		uah.m_dllname1 = L"uxtheme.dll";
		uah.m_funname1 = L"ThemeInitApiHook";
		uah.m_dllname2 = NULL;
		uah.m_funname2 = NULL;
	
		return lpfunc2003(&uah);
	}
	else
	{
		UNIMPLEMENTED;
		return FALSE;
	}
}

BOOL WINAPI
ThemeHooksRemove()
{
	return UnregisterUserApiHook();
}
