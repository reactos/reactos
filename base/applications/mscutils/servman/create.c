/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/create.c
 * PURPOSE:     Create a new service
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

typedef struct _CREATE_DATA
{
    HWND hSelf;
    LPWSTR ServiceName;
    LPWSTR DisplayName;
    LPWSTR BinPath;
    LPWSTR Description;
    LPWSTR Options;

} CREATE_DATA, *PCREATE_DATA;

static BOOL bHelpOpen = FALSE;

static BOOL
DoCreate(PCREATE_DATA Data)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    BOOL bRet = FALSE;

    /* open handle to the SCM */
    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hSc = CreateServiceW(hSCManager,
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

        if (hSc)
        {
            LPWSTR lpSuccess;

            /* Set the service description as CreateService
               does not do this for us */
            SetServiceDescription(Data->ServiceName,
                                  Data->Description);

            /* report success to user */
            if (AllocAndLoadString(&lpSuccess,
                                   hInstance,
                                   IDS_CREATE_SUCCESS))
            {
                DisplayString(lpSuccess);

                LocalFree(lpSuccess);
            }

            CloseServiceHandle(hSc);
            bRet = TRUE;
        }

        CloseServiceHandle(hSCManager);
    }

    return bRet;
}

static LPWSTR
GetStringFromDialog(PCREATE_DATA Data,
                    UINT id)
{
    HWND hwnd;
    LPWSTR lpString = NULL;
    INT iLen = 0;

    hwnd = GetDlgItem(Data->hSelf,
                      id);
    if (hwnd)
    {
        iLen = GetWindowTextLengthW(hwnd);
        if (iLen)
        {
            lpString = (LPWSTR)HeapAlloc(ProcessHeap,
                                         0,
                                         (iLen + 1) * sizeof(WCHAR));
            if (lpString)
            {
                GetWindowTextW(hwnd,
                               lpString,
                               iLen + 1);
            }
        }
    }

    return lpString;
}

static BOOL
GetDataFromDialog(PCREATE_DATA Data)
{
    BOOL bRet = FALSE;

    if ((Data->ServiceName = GetStringFromDialog(Data, IDC_CREATE_SERVNAME)))
    {
        if ((Data->DisplayName = GetStringFromDialog(Data, IDC_CREATE_DISPNAME)))
        {
            if ((Data->BinPath = GetStringFromDialog(Data, IDC_CREATE_PATH)))
            {
                Data->Description = GetStringFromDialog(Data, IDC_CREATE_DESC);
                Data->Options = GetStringFromDialog(Data, IDC_CREATE_OPTIONS);

                bRet = TRUE;
            }
        }
    }

    return bRet;
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

INT_PTR CALLBACK
CreateHelpDialogProc(HWND hDlg,
                     UINT message,
                     WPARAM wParam,
                     LPARAM lParam)
{
    HWND hHelp;
    HICON hIcon = NULL;
    WCHAR Buf[1000];

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hIcon = (HICON) LoadImageW(hInstance,
                                       MAKEINTRESOURCE(IDI_SM_ICON),
                                       IMAGE_ICON,
                                       16,
                                       16,
                                       0);

            SendMessageW(hDlg,
                         WM_SETICON,
                         ICON_SMALL,
                         (LPARAM)hIcon);

            hHelp = GetDlgItem(hDlg,
                               IDC_CREATE_HELP);

            LoadStringW(hInstance,
                        IDS_HELP_OPTIONS,
                        Buf,
                        sizeof(Buf) / sizeof(WCHAR));

            SetWindowTextW(hHelp, Buf);

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

INT_PTR CALLBACK
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
            hIcon = (HICON)LoadImage(hInstance,
                                     MAKEINTRESOURCE(IDI_SM_ICON),
                                     IMAGE_ICON,
                                     16,
                                     16,
                                     0);
            if (hIcon)
            {
                SendMessage(hDlg,
                            WM_SETICON,
                            ICON_SMALL,
                            (LPARAM)hIcon);
                DestroyIcon(hIcon);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    PCREATE_DATA Data;

                    Data = (PCREATE_DATA)HeapAlloc(ProcessHeap,
                                                   HEAP_ZERO_MEMORY,
                                                   sizeof(CREATE_DATA));
                    if (Data)
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

                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }

                case IDCANCEL:
                {
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
                                             CreateHelpDialogProc);
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
