#include "stdafx.h"
#include "systray.h"

STICKYKEYS sk;
int skIconShown = -1; // either -1 or displacement of icon
HICON skIcon;

MOUSEKEYS mk;
DWORD mkStatus;
int mkIconShown = -1; // either -1 or equivalent to mkStatus
HICON mkIcon;

FILTERKEYS fk;
HICON fkIcon;

extern HINSTANCE g_hInstance;
void StickyKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon);
void StickyKeys_UpdateIcon(HWND hWnd, DWORD message);
void MouseKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon);
void MouseKeys_UpdateIcon(HWND hWnd, DWORD message);
void FilterKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon);
void FilterKeys_UpdateIcon(HWND hWnd, DWORD message);
void SysTray_NotifyIcon2(HWND hWnd, UINT uCallbackMessage, DWORD Message, HICON hIcon, LPCTSTR lpTip);

extern DWORD g_uiShellHook; //shell hook window message


BOOL StickyKeys_CheckEnable(HWND hWnd)
{
    BOOL bEnable;

    sk.cbSize = sizeof(sk);
    SystemParametersInfo(
      SPI_GETSTICKYKEYS,
      sizeof(sk),
      &sk,
      0);

    bEnable = sk.dwFlags & SKF_INDICATOR && sk.dwFlags & SKF_STICKYKEYSON;

    StickyKeys_UpdateStatus(hWnd, bEnable);

    return(bEnable);
}

void StickyKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon) {
    if (bShowIcon != (skIconShown!= -1)) {
        if (bShowIcon) {
            StickyKeys_UpdateIcon(hWnd, NIM_ADD);
        } else {
            SysTray_NotifyIcon(hWnd, STWM_NOTIFYSTICKYKEYS, NIM_DELETE, NULL, NULL);
            if (skIcon) {
                DestroyIcon(skIcon);
                skIcon = NULL;
                skIconShown = -1;
            }
        }
    }
    if (bShowIcon) {
        StickyKeys_UpdateIcon(hWnd, NIM_MODIFY);
    }
}

void StickyKeys_UpdateIcon(HWND hWnd, DWORD message)
{
    LPTSTR      lpsz;

    int iStickyOffset = 0;


    if (sk.dwFlags & SKF_LSHIFTLATCHED) iStickyOffset |= 1;
    if (sk.dwFlags & SKF_RSHIFTLATCHED) iStickyOffset |= 1;
    if (sk.dwFlags & SKF_LSHIFTLOCKED) iStickyOffset |= 1;
    if (sk.dwFlags & SKF_RSHIFTLOCKED) iStickyOffset |= 1;

    if (sk.dwFlags & SKF_LCTLLATCHED) iStickyOffset |= 2;
    if (sk.dwFlags & SKF_RCTLLATCHED) iStickyOffset |= 2;
    if (sk.dwFlags & SKF_LCTLLOCKED) iStickyOffset |= 2;
    if (sk.dwFlags & SKF_RCTLLOCKED) iStickyOffset |= 2;

    if (sk.dwFlags & SKF_LALTLATCHED) iStickyOffset |= 4;
    if (sk.dwFlags & SKF_RALTLATCHED) iStickyOffset |= 4;
    if (sk.dwFlags & SKF_LALTLOCKED) iStickyOffset |= 4;
    if (sk.dwFlags & SKF_RALTLOCKED) iStickyOffset |= 4;

    if (sk.dwFlags & SKF_LWINLATCHED) iStickyOffset |= 8;
    if (sk.dwFlags & SKF_RWINLATCHED) iStickyOffset |= 8;
    if (sk.dwFlags & SKF_LWINLOCKED) iStickyOffset |= 8;
    if (sk.dwFlags & SKF_RWINLOCKED) iStickyOffset |= 8;


    if ((!skIcon) || (iStickyOffset != skIconShown)) {
        if (skIcon) DestroyIcon(skIcon);
        skIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_STK000 + iStickyOffset),
                                        IMAGE_ICON, 16, 16, 0);
        skIconShown = iStickyOffset;
    }
    lpsz    = LoadDynamicString(IDS_STICKYKEYS);
    SysTray_NotifyIcon2(hWnd, STWM_NOTIFYSTICKYKEYS, message, skIcon, lpsz);
    DeleteDynamicString(lpsz);
}

void StickyKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WinExec("rundll32.exe Shell32.dll,Control_RunDLL access.cpl,,1",SW_SHOW);
        break;
    }
}

BOOL MouseKeys_CheckEnable(HWND hWnd)
{
    BOOL bEnable;

    mk.cbSize = sizeof(mk);
    SystemParametersInfo(
      SPI_GETMOUSEKEYS,
      sizeof(mk),
      &mk,
      0);

    bEnable = mk.dwFlags & MKF_INDICATOR && mk.dwFlags & MKF_MOUSEKEYSON;

    MouseKeys_UpdateStatus(hWnd, bEnable);

    return(bEnable);
}

void MouseKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon) {
    if (bShowIcon != (mkIconShown!= -1)) {
        if (bShowIcon) {
            MouseKeys_UpdateIcon(hWnd, NIM_ADD);

            g_uiShellHook = RegisterWindowMessage(L"SHELLHOOK");
            RegisterShellHook(hWnd, TRUE);
        } else {

            g_uiShellHook = 0;
            RegisterShellHook(hWnd, FALSE);
			
            SysTray_NotifyIcon(hWnd, STWM_NOTIFYMOUSEKEYS, NIM_DELETE, NULL, NULL);
            if (mkIcon) {
                DestroyIcon(mkIcon);
                mkIcon = NULL;
                mkIconShown = -1;
            }
        }
    }
    if (bShowIcon) {
        MouseKeys_UpdateIcon(hWnd, NIM_MODIFY);
    }
}

int MouseIcon[] = {
        IDI_MKPASS,           // 00 00  no buttons selected
        IDI_MKGT,             // 00 01  left selected, up
        IDI_MKTG,             // 00 10  right selected, up
        IDI_MKGG,             // 00 11  both selected, up
        IDI_MKPASS,           // 01 00  no buttons selected
        IDI_MKBT,             // 01 01  left selected, and down
        IDI_MKTG,             // 01 10  right selected, up
        IDI_MKBG,             // 01 11  both selected, left down, right up
        IDI_MKPASS,           // 10 00  no buttons selected
        IDI_MKGT,             // 10 01  left selected, right down
        IDI_MKTB,             // 10 10  right selected, down
        IDI_MKGB,             // 10 11  both selected, right down
        IDI_MKPASS,           // 11 00  no buttons selected
        IDI_MKBT,             // 11 01  left selected, down
        IDI_MKTB,             // 11 10  right selected, down
        IDI_MKBB};            // 11 11  both selected, down

void MouseKeys_UpdateIcon(HWND hWnd, DWORD message)
{
    LPTSTR      lpsz;
    int iMouseIcon = 0;

    if (!(mk.dwFlags & MKF_MOUSEMODE)) iMouseIcon = IDI_MKPASS;
    else {
        /*
         * Set up iMouseIcon as an index into the table first
         */

        if (mk.dwFlags & MKF_LEFTBUTTONSEL) iMouseIcon |= 1;
        if (mk.dwFlags & MKF_RIGHTBUTTONSEL) iMouseIcon |= 2;
        if (mk.dwFlags & MKF_LEFTBUTTONDOWN) iMouseIcon |= 4;
        if (mk.dwFlags & MKF_RIGHTBUTTONDOWN) iMouseIcon |= 8;
        iMouseIcon = MouseIcon[iMouseIcon];
    }

    if ((!mkIcon) || (iMouseIcon != mkIconShown)) {
        if (mkIcon) DestroyIcon(mkIcon);
        mkIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(iMouseIcon),
                                        IMAGE_ICON, 16, 16, 0);
        mkIconShown = iMouseIcon;
    }
    lpsz    = LoadDynamicString(IDS_MOUSEKEYS);
    SysTray_NotifyIcon2(hWnd, STWM_NOTIFYMOUSEKEYS, message, mkIcon, lpsz);
    DeleteDynamicString(lpsz);
}

void MouseKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WinExec("rundll32.exe Shell32.dll,Control_RunDLL access.cpl,,4",SW_SHOW);
        break;
    }
}


BOOL FilterKeys_CheckEnable(HWND hWnd)
{
    BOOL bEnable;

    fk.cbSize = sizeof(fk);
    SystemParametersInfo(
      SPI_GETFILTERKEYS,
      sizeof(fk),
      &fk,
      0);

    bEnable = fk.dwFlags & FKF_INDICATOR && fk.dwFlags & FKF_FILTERKEYSON;

    FilterKeys_UpdateStatus(hWnd, bEnable);

    return(bEnable);
}

void FilterKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon) {
    if (bShowIcon != (fkIcon!= NULL)) {
        if (bShowIcon) {
            FilterKeys_UpdateIcon(hWnd, NIM_ADD);
        } else {
            SysTray_NotifyIcon(hWnd, STWM_NOTIFYFILTERKEYS, NIM_DELETE, NULL, NULL);
            if (fkIcon) {
                DestroyIcon(fkIcon);
                fkIcon = NULL;
            }
        }
    }
}

void FilterKeys_UpdateIcon(HWND hWnd, DWORD message)
{
    LPTSTR      lpsz;

    if (!fkIcon) {
        fkIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_FILTER),
                                        IMAGE_ICON, 16, 16, 0);
    }
    lpsz    = LoadDynamicString(IDS_FILTERKEYS);
    SysTray_NotifyIcon2(hWnd, STWM_NOTIFYFILTERKEYS, message, fkIcon, lpsz);
    DeleteDynamicString(lpsz);
}

void FilterKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WinExec("rundll32.exe Shell32.dll,Control_RunDLL access.cpl,,1",SW_SHOW);
        break;
    }
}



//
// This function takes care of high contrast video mode which is set
// in Control Panel --> Accessibility Options --> Display --> Hi Contrast.
// The icon looks black in Normal video mode, and White in Hi-Contrast mode.
//
void SysTray_NotifyIcon2(	HWND hWnd,
							UINT uCallbackMessage,
							DWORD Message,
							HICON hIcon,
							LPCTSTR lpTip
							)

{
	HICON hRawIcon = hIcon;
	BITMAP BmpInfo;
	ICONINFO IconInfo;
	HBITMAP hCopyBmp = NULL;
	HDC hdcCopyBmp = NULL;
	HDC hdcIconBmp = NULL;
	ICONINFO ic;
	HICON hNewIcon = NULL;
	int i, j;
	COLORREF clr = GetSysColor(COLOR_WINDOWTEXT);

	GetIconInfo((HICON)hRawIcon, &IconInfo);

	GetObject(	IconInfo.hbmColor,
				sizeof(BITMAP),	
				&BmpInfo );	

	hCopyBmp = CreateBitmap(BmpInfo.bmWidth,
							BmpInfo.bmHeight,
							BmpInfo.bmPlanes,			// Planes
							BmpInfo.bmBitsPixel,		// BitsPerPel
							NULL);						// bits

	hdcCopyBmp = CreateCompatibleDC(NULL);
	if (!hdcCopyBmp) 
		OutputDebugString(TEXT("create compatible dc failed\n"));
	SelectObject(hdcCopyBmp, hCopyBmp);

	// Select Icon bitmap into a memoryDC so we can use it
	hdcIconBmp = CreateCompatibleDC(NULL);
	if (!hdcIconBmp)
		OutputDebugString(TEXT("create compatible DC failed\n"));
	SelectObject(hdcIconBmp, IconInfo.hbmColor);

	BitBlt(	hdcCopyBmp, 
			0,  
			0,  
			BmpInfo.bmWidth,  
			BmpInfo.bmHeight, 
			hdcIconBmp,  
			0,   
			0,   
			SRCCOPY  
			);

	ic.fIcon = TRUE;				// This is an icon
	ic.xHotspot = 0;
	ic.yHotspot = 0;
	ic.hbmMask = IconInfo.hbmMask;
			
	for (i=0; i < BmpInfo.bmWidth; i++)
		for (j=0; j < BmpInfo.bmHeight; j++)
		{
			COLORREF pel_value = GetPixel(hdcCopyBmp, i, j);
			if (pel_value == (COLORREF) RGB(0,0,128)) // The color on icon resource is BLUE!!
				SetPixel(hdcCopyBmp, i, j, clr);	// Window-Text icon
		}

	ic.hbmColor = hCopyBmp; // IconInfo.hbmColor;

	hNewIcon = CreateIconIndirect(&ic);

	DeleteObject(hdcIconBmp);
	DeleteDC(hdcCopyBmp);

	SysTray_NotifyIcon(hWnd, uCallbackMessage, Message, hNewIcon, lpTip);
}
