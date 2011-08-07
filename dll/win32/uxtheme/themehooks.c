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
#include "uxthemedll.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);



extern HINSTANCE hDllInst;

LRESULT CALLBACK ThemeWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, WNDPROC DefWndProc);

USERAPIHOOK user32ApiHook;
BYTE gabDWPmessages[UAHOWP_MAX_SIZE];
BYTE gabMSGPmessages[UAHOWP_MAX_SIZE];

static LRESULT CALLBACK
ThemeDefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{      
    if(!IsThemeActive())
    {
        return user32ApiHook.DefWindowProcW(hWnd, 
                                            Msg, 
                                            wParam, 
                                            lParam);
    }

    return ThemeWndProc(hWnd, 
                        Msg, 
                        wParam, 
                        lParam, 
                        user32ApiHook.DefWindowProcW);
}

static LRESULT CALLBACK
ThemeDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if(!IsThemeActive())
    {
        return user32ApiHook.DefWindowProcA(hWnd, 
                                            Msg, 
                                            wParam, 
                                            lParam);
    }

    return ThemeWndProc(hWnd, 
                        Msg, 
                        wParam, 
                        lParam, 
                        user32ApiHook.DefWindowProcA);
}

static LRESULT CALLBACK
ThemePreWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, ULONG_PTR ret,PDWORD unknown)
{
    switch(Msg)
    {
        case WM_THEMECHANGED:
            UXTHEME_LoadTheme();
            return 0;
    }

    return 0;
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
    puah->PreWndProc = ThemePreWindowProc;
    puah->DefWndProcArray.MsgBitArray  = gabDWPmessages;
    puah->DefWndProcArray.Size = UAHOWP_MAX_SIZE;
    puah->WndProcArray.MsgBitArray = gabMSGPmessages;
    puah->WndProcArray.Size = UAHOWP_MAX_SIZE;

    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCPAINT);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCACTIVATE);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCMOUSEMOVE);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCMOUSELEAVE);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCHITTEST);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCLBUTTONDOWN);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCUAHDRAWCAPTION);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCUAHDRAWFRAME);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_SETTEXT);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_WINDOWPOSCHANGED);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CONTEXTMENU);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_STYLECHANGED);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_SETICON);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCDESTROY);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_SYSCOMMAND);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CTLCOLORMSGBOX);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CTLCOLORBTN);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CTLCOLORSTATIC);

    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_CREATE);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_SETTINGCHANGE);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_DRAWITEM);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_MEASUREITEM);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_WINDOWPOSCHANGING);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_WINDOWPOSCHANGED);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_STYLECHANGING);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_STYLECHANGED);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_NCCREATE);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_NCDESTROY);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_NCPAINT);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_MENUCHAR);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_MDISETMENU);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_THEMECHANGED);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_UAHINIT);

    return TRUE;
}

typedef BOOL (WINAPI * PREGISTER_UAH_WINXP)(HINSTANCE hInstance, USERAPIHOOKPROC CallbackFunc);
typedef BOOL (WINAPI * PREGISTER_UUAH_WIN2003)(PUSERAPIHOOKINFO puah);

BOOL WINAPI
ThemeHooksInstall()
{
    PVOID lpFunc;
    OSVERSIONINFO osvi;
    BOOL ret;

    lpFunc = GetProcAddress(GetModuleHandle("user32.dll"), "RegisterUserApiHook");

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
    {
        PREGISTER_UAH_WINXP lpfuncxp = (PREGISTER_UAH_WINXP)lpFunc;
        ret = lpfuncxp(hDllInst, ThemeInitApiHook);
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

        ret = lpfunc2003(&uah);
    }
    else
    {
        UNIMPLEMENTED;
        ret = FALSE;
    }

    UXTHEME_broadcast_msg (NULL, WM_THEMECHANGED);

    return ret;
}

BOOL WINAPI
ThemeHooksRemove()
{
    return UnregisterUserApiHook();
}
