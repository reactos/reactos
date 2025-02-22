/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig/msconfig.c
 * PURPOSE:     msconfig main dialog
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include "precomp.h"

#include "toolspage.h"
#include "srvpage.h"
#include "startuppage.h"
#include "freeldrpage.h"
#include "systempage.h"
#include "generalpage.h"

HINSTANCE hInst = 0;

HWND hMainWnd;                   /* Main Window */
HWND hTabWnd;                    /* Tab Control Window */
UINT uXIcon = 0, uYIcon = 0;     /* Icon sizes */
HICON hDialogIconBig = NULL;
HICON hDialogIconSmall = NULL;

void MsConfig_OnTabWndSelChange(void);

////////////////////////////////////////////////////////////////////////////////
// Taken from WinSpy++ 1.7
// http://www.catch22.net/software/winspy
// Copyright (c) 2002 by J Brown
//

//
//	Copied from uxtheme.h
//  If you have this new header, then delete these and
//  #include <uxtheme.h> instead!
//
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

//
typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

//
//	Try to call EnableThemeDialogTexture, if uxtheme.dll is present
//
BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc fnEnableThemeDialogTexture;

    hUXTheme = LoadLibrary(_T("uxtheme.dll"));

    if(hUXTheme)
    {
        fnEnableThemeDialogTexture =
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if(fnEnableThemeDialogTexture)
        {
            fnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

            FreeLibrary(hUXTheme);
            return TRUE;
        }
        else
        {
            // Failed to locate API!
            FreeLibrary(hUXTheme);
            return FALSE;
        }
    }
    else
    {
        // Not running under XP? Just fail gracefully
        return FALSE;
    }
}
BOOL OnCreate(HWND hWnd)
{
    TCHAR   szTemp[256];
    TCITEM  item;

    hTabWnd = GetDlgItem(hWnd, IDC_TAB);
    hGeneralPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_GENERAL_PAGE), hWnd,  GeneralPageWndProc); EnableDialogTheme(hGeneralPage);
    hSystemPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SYSTEM_PAGE), hWnd,  SystemPageWndProc); EnableDialogTheme(hSystemPage);
    hFreeLdrPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FREELDR_PAGE), hWnd,  FreeLdrPageWndProc); EnableDialogTheme(hFreeLdrPage);
    hServicesPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SERVICES_PAGE), hWnd,  ServicesPageWndProc); EnableDialogTheme(hServicesPage);
    hStartupPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_STARTUP_PAGE), hWnd,  StartupPageWndProc); EnableDialogTheme(hStartupPage);
    hToolsPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TOOLS_PAGE), hWnd,  ToolsPageWndProc); EnableDialogTheme(hToolsPage);

    LoadString(hInst, IDS_MSCONFIG, szTemp, 256);
    SetWindowText(hWnd, szTemp);

    // Insert Tab Pages
    LoadString(hInst, IDS_TAB_GENERAL, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 0, &item);

    LoadString(hInst, IDS_TAB_SYSTEM, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 1, &item);

    LoadString(hInst, IDS_TAB_FREELDR, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 2, &item);

    LoadString(hInst, IDS_TAB_SERVICES, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 3, &item);

    LoadString(hInst, IDS_TAB_STARTUP, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 4, &item);

    LoadString(hInst, IDS_TAB_TOOLS, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 5, &item);

    MsConfig_OnTabWndSelChange();

    return TRUE;
}


void MsConfig_OnTabWndSelChange(void)
{
    switch (TabCtrl_GetCurSel(hTabWnd)) {
    case 0: //General
        ShowWindow(hGeneralPage, SW_SHOW);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hGeneralPage);
        break;
    case 1: //SYSTEM.INI
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_SHOW);
        ShowWindow(hToolsPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        BringWindowToTop(hSystemPage);
        break;
    case 2: //Freeldr
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_SHOW);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hFreeLdrPage);
        break;
    case 3: //Services
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_SHOW);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hServicesPage);
        break;
    case 4: //startup
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_SHOW);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hStartupPage);
        break;
    case 5: //Tools
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_SHOW);
        BringWindowToTop(hToolsPage);
        break;
    }
}


static
VOID
SetDialogIcon(HWND hDlg)
{
    if (hDialogIconBig) DestroyIcon(hDialogIconBig);
    if (hDialogIconSmall) DestroyIcon(hDialogIconSmall);

    hDialogIconBig = LoadIconW(GetModuleHandle(NULL),
                               MAKEINTRESOURCE(IDI_APPICON));
    hDialogIconSmall = LoadImage(GetModuleHandle(NULL),
                                 MAKEINTRESOURCE(IDI_APPICON),
                                 IMAGE_ICON,
                                 uXIcon,
                                 uYIcon,
                                 0);
    SendMessage(hDlg,
                WM_SETICON,
                ICON_BIG,
                (LPARAM)hDialogIconBig);
    SendMessage(hDlg,
                WM_SETICON,
                ICON_SMALL,
                (LPARAM)hDialogIconSmall);
}


/* Message handler for dialog box. */
INT_PTR CALLBACK
MsConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR         pnmh;
    UINT            uXIconNew, uYIconNew;

    switch (message)
    {
        case WM_INITDIALOG:
            hMainWnd = hDlg;

            uXIcon = GetSystemMetrics(SM_CXSMICON);
            uYIcon = GetSystemMetrics(SM_CYSMICON);

            SetDialogIcon(hDlg);

            return OnCreate(hDlg);

        case WM_SETTINGCHANGE:
            uXIconNew = GetSystemMetrics(SM_CXSMICON);
            uYIconNew = GetSystemMetrics(SM_CYSMICON);

            if ((uXIcon != uXIconNew) || (uYIcon != uYIconNew))
            {
                uXIcon = uXIconNew;
                uYIcon = uYIconNew;
                SetDialogIcon(hDlg);
            }
            break;

        case WM_COMMAND:

            if (LOWORD(wParam) == IDOK)
            {
                //MsConfig_OnSaveChanges();
            }

            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            if ((pnmh->hwndFrom == hTabWnd) &&
                (pnmh->idFrom == IDC_TAB) &&
                (pnmh->code == TCN_SELCHANGE))
            {
                MsConfig_OnTabWndSelChange();
            }
            break;

        case WM_SYSCOLORCHANGE:
            /* Forward WM_SYSCOLORCHANGE to common controls */
            SendMessage(hServicesListCtrl, WM_SYSCOLORCHANGE, 0, 0);
            SendMessage(hStartupListCtrl, WM_SYSCOLORCHANGE, 0, 0);
            SendMessage(hToolsListCtrl, WM_SYSCOLORCHANGE, 0, 0);
            break;

        case WM_DESTROY:
            if (hToolsPage)
                DestroyWindow(hToolsPage);
            if (hGeneralPage)
                DestroyWindow(hGeneralPage);
            if (hServicesPage)
                DestroyWindow(hServicesPage);
            if (hStartupPage)
                DestroyWindow(hStartupPage);
            if (hFreeLdrPage)
                DestroyWindow(hFreeLdrPage);
            if (hSystemPage)
                DestroyWindow(hSystemPage);
            if (hDialogIconBig)
                DestroyIcon(hDialogIconBig);
            if (hDialogIconSmall)
                DestroyIcon(hDialogIconSmall);
            return DefWindowProc(hDlg, message, wParam, lParam);
    }

    return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{

    INITCOMMONCONTROLSEX InitControls;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&InitControls);

    hInst = hInstance;

    DialogBox(hInst, (LPCTSTR)IDD_MSCONFIG_DIALOG, NULL,  MsConfigWndProc);

    return 0;
}

/* EOF */
