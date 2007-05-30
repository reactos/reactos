/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/create.c
 * PURPOSE:     Create a new service
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

typedef struct _CREATE_DATA
{
    HWND hSelf;
    LPTSTR ServiceName;
    LPTSTR DisplayName;
    LPTSTR BinPath;
    LPTSTR Description;
    LPTSTR Options;

} CREATE_DATA, *PCREATE_DATA;

static BOOL bHelpOpen = FALSE;

static BOOL
DoCreate(PCREATE_DATA Data)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    TCHAR Buf[32];

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    hSc = CreateService(hSCManager,
                        Data->ServiceName,
                        Data->DisplayName,
                        SERVICE_ALL_ACCESS,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        Data->BinPath,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

    if (hSc == NULL)
    {
        GetError();
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    /* Set the service description in the registry
     * CreateService does not do this for us */
    SetDescription(Data->ServiceName,
                   Data->Description);

    /* report success to user */
    LoadString(hInstance,
               IDS_CREATE_SUCCESS,
               Buf,
               sizeof(Buf) / sizeof(TCHAR));
	DisplayString(Buf);

	CloseServiceHandle(hSCManager);
    CloseServiceHandle(hSc);

    return TRUE;
}


static BOOL
GetDataFromDialog(PCREATE_DATA Data)
{
    HWND hwnd;
    TCHAR Buf[64];
    INT iLen = 0;

    /* get service name */
    hwnd = GetDlgItem(Data->hSelf,
                      IDC_CREATE_SERVNAME);
    iLen = GetWindowTextLength(hwnd);
    if (iLen != 0)
    {
        Data->ServiceName = (TCHAR*) HeapAlloc(ProcessHeap,
                                      0,
                                      (iLen+1) * sizeof(TCHAR));
        if (Data->ServiceName != NULL)
        {
            GetWindowText(hwnd,
                          Data->ServiceName,
                          iLen+1);
        }
        else
            return FALSE;
    }
    else
    {
        LoadString(hInstance,
                   IDS_CREATE_REQ,
                   Buf,
                   sizeof(Buf));
        DisplayString(Buf);
        SetFocus(hwnd);
        return FALSE;
    }

    /* get display name */
    iLen = 0;
    hwnd = GetDlgItem(Data->hSelf,
                      IDC_CREATE_DISPNAME);
    iLen = GetWindowTextLength(hwnd);
    if (iLen != 0)
    {
        Data->DisplayName = (TCHAR*) HeapAlloc(ProcessHeap,
                                      0,
                                      (iLen+1) * sizeof(TCHAR));
        if (Data->DisplayName != NULL)
        {
            GetWindowText(hwnd,
                          Data->DisplayName,
                          iLen+1);
        }
        else
            return FALSE;
    }
    else
    {
        LoadString(hInstance,
                   IDS_CREATE_REQ,
                   Buf,
                   sizeof(Buf));
        DisplayString(Buf);
        SetFocus(hwnd);
        return FALSE;
    }

    /* get binary path */
    iLen = 0;
    hwnd = GetDlgItem(Data->hSelf,
                      IDC_CREATE_PATH);
    iLen = GetWindowTextLength(hwnd);
    if (iLen != 0)
    {
        Data->BinPath = (TCHAR*) HeapAlloc(ProcessHeap,
                                  0,
                                  (iLen+1) * sizeof(TCHAR));
        if (Data->BinPath != NULL)
        {
            GetWindowText(hwnd,
                          Data->BinPath,
                          iLen+1);
        }
        else
            return FALSE;
    }
    else
    {
        LoadString(hInstance,
                   IDS_CREATE_REQ,
                   Buf,
                   sizeof(Buf));
        DisplayString(Buf);
        SetFocus(hwnd);
        return FALSE;
    }

    /* get description */
    iLen = 0;
    hwnd = GetDlgItem(Data->hSelf,
                      IDC_CREATE_DESC);
    iLen = GetWindowTextLength(hwnd);
    if (iLen != 0)
    {
        Data->Description = (TCHAR*) HeapAlloc(ProcessHeap,
                                      0,
                                      (iLen+1) * sizeof(TCHAR));
        if (Data->Description != NULL)
        {
            GetWindowText(hwnd,
                          Data->Description,
                          iLen+1);
        }
        else
            return FALSE;
    }


    /* get options */
    iLen = 0;
    hwnd = GetDlgItem(Data->hSelf,
                      IDC_CREATE_PATH);
    iLen = GetWindowTextLength(hwnd);
    if (iLen != 0)
    {
        Data->Options = (TCHAR*) HeapAlloc(ProcessHeap,
                                  0,
                                  (iLen+1) * sizeof(TCHAR));
        if (Data->Options != NULL)
        {
            GetWindowText(hwnd,
                          Data->Options,
                          iLen+1);
        }
        else
            return FALSE;
    }

    return TRUE;
}

static VOID
FreeMemory(PCREATE_DATA Data)
{
    if (Data->ServiceName != NULL)
        HeapFree(ProcessHeap,
                 0,
                 Data->ServiceName);
    if (Data->DisplayName != NULL)
        HeapFree(ProcessHeap,
                 0,
                 Data->DisplayName);
    if (Data->BinPath != NULL)
        HeapFree(ProcessHeap,
                 0,
                 Data->BinPath);
    if (Data->Description != NULL)
        HeapFree(ProcessHeap,
                 0,
                 Data->Description);
    if (Data->Options != NULL)
        HeapFree(ProcessHeap,
                 0,
                 Data->Options);

    HeapFree(ProcessHeap,
             0,
             Data);
}


BOOL CALLBACK
CreateHelpDialogProc(HWND hDlg,
                     UINT message,
                     WPARAM wParam,
                     LPARAM lParam)
{
    HWND hHelp;
    HICON hIcon = NULL;
    TCHAR Buf[1000];

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hIcon = (HICON) LoadImage(hInstance,
                              MAKEINTRESOURCE(IDI_SM_ICON),
                              IMAGE_ICON,
                              16,
                              16,
                              0);

            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);

            hHelp = GetDlgItem(hDlg,
                               IDC_CREATE_HELP);

            LoadString(hInstance,
                       IDS_HELP_OPTIONS,
                       Buf,
                       sizeof(Buf) / sizeof(TCHAR));

            SetWindowText(hHelp,
                          Buf);

            return TRUE;
        }

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                bHelpOpen = FALSE;
                DestroyIcon(hIcon);
                DestroyWindow(hDlg);
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}


BOOL CALLBACK
CreateDialogProc(HWND hDlg,
                 UINT message,
                 WPARAM wParam,
                 LPARAM lParam)
{
    HICON hIcon = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hIcon = (HICON) LoadImage(hInstance,
                              MAKEINTRESOURCE(IDI_SM_ICON),
                              IMAGE_ICON,
                              16,
                              16,
                              0);

            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    PCREATE_DATA Data;

                    Data = (PCREATE_DATA) HeapAlloc(ProcessHeap,
                                     HEAP_ZERO_MEMORY,
                                     sizeof(CREATE_DATA));
                    if (Data != NULL)
                    {
                        Data->hSelf = hDlg;

                        if (GetDataFromDialog(Data))
                        {
                            DoCreate(Data);
                        }
                        else
                        {
                            /* Something went wrong, leave the dialog
                             * open so they can try again */
                            FreeMemory(Data);
                            break;
                        }

                        FreeMemory(Data);
                    }

                    DestroyIcon(hIcon);
                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }

                case IDCANCEL:
                {
                    DestroyIcon(hIcon);
                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }

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
                            bHelpOpen = TRUE;
                        }
                    }
                }
                break;
            }
        }
    }

    return FALSE;
}
