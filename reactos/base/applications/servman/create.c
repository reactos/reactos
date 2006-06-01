/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/create.c
 * PURPOSE:     Create a new service
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

extern HINSTANCE hInstance;
BOOL bHelpOpen = FALSE;


BOOL Create(LPTSTR ServiceName,
            LPTSTR DisplayName,
            LPTSTR BinPath,
            LPTSTR Description,
            LPTSTR Options)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    TCHAR Buf[32];

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    hSc = CreateService(hSCManager,
                        ServiceName,
                        DisplayName,
                        SERVICE_ALL_ACCESS,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        BinPath,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

    if (hSc == NULL)
    {
        GetError();
        return FALSE;
    }

    SetDescription(ServiceName, Description);

    LoadString(hInstance, IDS_CREATE_SUCCESS, Buf,
        sizeof(Buf) / sizeof(TCHAR));
	DisplayString(Buf);
	CloseServiceHandle(hSCManager);
    CloseServiceHandle(hSc);
    return TRUE;
}


#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
BOOL CALLBACK
CreateHelpDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hHelp;
    HICON hIcon = NULL;
    TCHAR Buf[1000];

    switch (message)
    {
    case WM_INITDIALOG:
        hIcon = LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM_ICON), IMAGE_ICON, 16, 16, 0);
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

        hHelp = GetDlgItem(hDlg, IDC_CREATE_HELP);

        LoadString(hInstance, IDS_HELP_OPTIONS, Buf,
            sizeof(Buf) / sizeof(TCHAR));

        SetWindowText(hHelp, Buf);

        return TRUE;

    case WM_COMMAND:
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            DestroyIcon(hIcon);
            DestroyWindow(hDlg);
            return TRUE;
        }
    break;

    case WM_DESTROY:
        bHelpOpen = FALSE;
    break;
    }

    return FALSE;
}


BOOL CALLBACK
CreateDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
        hIcon = LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM_ICON), IMAGE_ICON, 16, 16, 0);
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
            {
                LPTSTR ServiceName = NULL;
                LPTSTR DisplayName = NULL;
                LPTSTR BinPath = NULL;
                LPTSTR Description = NULL;
                LPTSTR Options = NULL;
                HWND hwnd;
                TCHAR Buf[32];
                INT iLen = 0;

                /* get service name */
                hwnd = GetDlgItem(hDlg, IDC_CREATE_SERVNAME);
                iLen = GetWindowTextLength(hwnd);
                if (iLen != 0)
                {
                    ServiceName = HeapAlloc(GetProcessHeap(), 0, iLen+1);
                    if (ServiceName != NULL)
                    {
                        GetWindowText(hwnd, ServiceName, iLen+1);
                    }

                }
                else
                {
                    LoadString(hInstance, IDS_CREATE_REQ, Buf,
                        sizeof(Buf) / sizeof(TCHAR));
                    DisplayString(Buf);
                    SetFocus(hwnd);
                    break;
                }

                /* get display name */
                iLen = 0;
                hwnd = GetDlgItem(hDlg, IDC_CREATE_DISPNAME);
                iLen = GetWindowTextLength(hwnd);
                if (iLen != 0)
                {
                    DisplayName = HeapAlloc(GetProcessHeap(), 0, iLen+1);
                    if (DisplayName != NULL)
                        GetWindowText(hwnd, DisplayName, iLen+1);

                }
                else
                {
                    LoadString(hInstance, IDS_CREATE_REQ, Buf,
                        sizeof(Buf) / sizeof(TCHAR));
                    DisplayString(Buf);
                    SetFocus(hwnd);
                    break;
                }

                /* get binary path */
                iLen = 0;
                hwnd = GetDlgItem(hDlg, IDC_CREATE_PATH);
                iLen = GetWindowTextLength(hwnd);
                if (iLen != 0)
                {
                    BinPath = HeapAlloc(GetProcessHeap(), 0, iLen+1);
                    if (BinPath != NULL)
                        GetWindowText(hwnd, BinPath, iLen+1);

                }
                else
                {
                    LoadString(hInstance, IDS_CREATE_REQ, Buf,
                        sizeof(Buf) / sizeof(TCHAR));
                    DisplayString(Buf);
                    SetFocus(hwnd);
                    break;
                }

                /* get description */
                iLen = 0;
                hwnd = GetDlgItem(hDlg, IDC_CREATE_DESC);
                iLen = GetWindowTextLength(hwnd);
                if (iLen != 0)
                {
                    Description = HeapAlloc(GetProcessHeap(), 0, iLen+1);
                    if (Description != NULL)
                        GetWindowText(hwnd, Description, iLen+1);

                }

                /* get options */
                iLen = 0;
                hwnd = GetDlgItem(hDlg, IDC_CREATE_PATH);
                iLen = GetWindowTextLength(hwnd);
                if (iLen != 0)
                {
                    Options = HeapAlloc(GetProcessHeap(), 0, iLen+1);
                    if (Options != NULL)
                        GetWindowText(hwnd, Options, iLen+1);

                }

                Create(ServiceName, DisplayName, BinPath, Description, Options);

                if (ServiceName != NULL)
                    HeapFree(GetProcessHeap(), 0, ServiceName);
                if (DisplayName != NULL)
                    HeapFree(GetProcessHeap(), 0, DisplayName);
                if (BinPath != NULL)
                    HeapFree(GetProcessHeap(), 0, BinPath);
                if (Description != NULL)
                    HeapFree(GetProcessHeap(), 0, Description);
                if (Options != NULL)
                    HeapFree(GetProcessHeap(), 0, Options);


                DestroyIcon(hIcon);
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }

            case IDCANCEL:
                DestroyIcon(hIcon);
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;

            case ID_CREATE_HELP:
            {
                HWND hHelp;

                if (! bHelpOpen)
                {
                    hHelp = CreateDialog(hInstance,
                                         MAKEINTRESOURCE(IDD_DLG_HELP_OPTIONS),
                                         hDlg,
                                         (DLGPROC)CreateHelpDialogProc);
                    if(hHelp != NULL)
                    {
                        ShowWindow(hHelp, SW_SHOW);
                        bHelpOpen = TRUE;
                    }
                }
            }
            break;
        }

    }

    return FALSE;
}
