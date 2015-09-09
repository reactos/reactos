/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/srvpage.c
 * PURPOSE:     Services page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include "precomp.h"

#include <winsvc.h>
#include <winver.h>

HWND hServicesPage;
HWND hServicesListCtrl;
HWND hServicesDialog;

void GetServices ( void );

INT_PTR CALLBACK
ServicesPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LV_COLUMN   column;
    TCHAR       szTemp[256];
    DWORD dwStyle;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (message) {
    case WM_INITDIALOG:

        hServicesListCtrl = GetDlgItem(hDlg, IDC_SERVICES_LIST);
        hServicesDialog = hDlg;

        dwStyle = (DWORD) SendMessage(hServicesListCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
        dwStyle = dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        SendMessage(hServicesListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

        SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

        // Initialize the application page's controls
        column.mask = LVCF_TEXT | LVCF_WIDTH;

        LoadString(hInst, IDS_SERVICES_COLUMN_SERVICE, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 200;
        (void)ListView_InsertColumn(hServicesListCtrl, 0, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_SERVICES_COLUMN_REQ, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 70;
        (void)ListView_InsertColumn(hServicesListCtrl, 1, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_SERVICES_COLUMN_VENDOR, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 200;
        (void)ListView_InsertColumn(hServicesListCtrl, 2, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_SERVICES_COLUMN_STATUS, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 70;
        (void)ListView_InsertColumn(hServicesListCtrl, 3, &column);

        GetServices();
        return TRUE;
    }

    return 0;
}

void
GetServices ( void )
{
    LV_ITEM item;
    WORD wCodePage;
    WORD wLangID;
    SC_HANDLE ScHandle;
    SC_HANDLE hService;
    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;
    DWORD NumServices = 0;
    DWORD dwHandle, dwLen;
    size_t Index;
    UINT BufLen;
    TCHAR szStatus[128];
    TCHAR* lpData;
    TCHAR* lpBuffer;
    TCHAR szStrFileInfo[80];
    TCHAR FileName[MAX_PATH];
    LPVOID pvData;

    LPSERVICE_FAILURE_ACTIONS pServiceFailureActions = NULL;
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;

    ScHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (ScHandle != NULL)
    {
        if (EnumServicesStatusEx(ScHandle, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (LPBYTE)pServiceStatus, 0, &BytesNeeded, &NumServices, &ResumeHandle, 0) == 0)
        {
            /* Call function again if required size was returned */
            if (GetLastError() == ERROR_MORE_DATA)
            {
                /* reserve memory for service info array */
                pServiceStatus = HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                if (!pServiceStatus)
                {
                    CloseServiceHandle(ScHandle);
                    return;
                }

                /* fill array with service info */
                if (EnumServicesStatusEx(ScHandle, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (LPBYTE)pServiceStatus, BytesNeeded, &BytesNeeded, &NumServices, &ResumeHandle, 0) == 0)
                {
                    HeapFree(GetProcessHeap(), 0, pServiceStatus);
                    CloseServiceHandle(ScHandle);
                    return;
                }
            }
            else /* exit on failure */
            {
                CloseServiceHandle(ScHandle);
                return;
            }
        }

        if (NumServices)
        {
            if (!pServiceStatus)
            {
                CloseServiceHandle(ScHandle);
                return;
            }

            for (Index = 0; Index < NumServices; Index++)
            {
                memset(&item, 0, sizeof(LV_ITEM));
                item.mask = LVIF_TEXT;
                item.iImage = 0;
                item.pszText = pServiceStatus[Index].lpDisplayName;
                item.iItem = ListView_GetItemCount(hServicesListCtrl);
                item.lParam = 0;
                item.iItem = ListView_InsertItem(hServicesListCtrl, &item);

                if (pServiceStatus[Index].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
                {
                    ListView_SetCheckState(hServicesListCtrl, item.iItem, TRUE);
                }

                BytesNeeded = 0;
                hService = OpenService(ScHandle, pServiceStatus[Index].lpServiceName, SC_MANAGER_CONNECT);
                if (hService != NULL)
                {
                    /* check if service is required by the system*/
                    if (!QueryServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, (LPBYTE)NULL, 0, &BytesNeeded))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            pServiceFailureActions = HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                            if (pServiceFailureActions == NULL)
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceStatus);
                                CloseServiceHandle(hService);
                                CloseServiceHandle(ScHandle);
                                return;
                            }

                            if (!QueryServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, (LPBYTE)pServiceFailureActions, BytesNeeded, &BytesNeeded))
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceFailureActions);
                                HeapFree(GetProcessHeap(), 0, pServiceStatus);
                                CloseServiceHandle(hService);
                                CloseServiceHandle(ScHandle);
                                return;
                            }
                        }
                        else /* exit on failure */
                        {
                            HeapFree(GetProcessHeap(), 0, pServiceStatus);
                            CloseServiceHandle(hService);
                            CloseServiceHandle(ScHandle);
                            return;
                        }
                    }

                    if (pServiceFailureActions != NULL)
                    {
                        if (pServiceFailureActions->cActions && pServiceFailureActions->lpsaActions[0].Type == SC_ACTION_REBOOT)
                        {
                                LoadString(hInst, IDS_SERVICES_YES, szStatus, 128);
                                item.pszText = szStatus;
                                item.iSubItem = 1;
                                SendMessage(hServicesListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                        }
                        HeapFree(GetProcessHeap(), 0, pServiceFailureActions);
                        pServiceFailureActions = NULL;
                    }

                    /* get vendor of service binary */
                    BytesNeeded = 0;
                    if (!QueryServiceConfig(hService, NULL, 0, &BytesNeeded))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            pServiceConfig = HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                            if (pServiceConfig == NULL)
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceStatus);
                                CloseServiceHandle(hService);
                                CloseServiceHandle(ScHandle);
                                return;
                            }
                            if (!QueryServiceConfig(hService, pServiceConfig, BytesNeeded, &BytesNeeded))
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceConfig);
                                HeapFree(GetProcessHeap(), 0, pServiceStatus);
                                CloseServiceHandle(hService);
                                CloseServiceHandle(ScHandle);
                                return;
                            }
                        }
                        else /* exit on failure */
                        {
                            HeapFree(GetProcessHeap(), 0, pServiceStatus);
                            CloseServiceHandle(hService);
                            CloseServiceHandle(ScHandle);
                            return;
                        }
                    }

                    memset(&FileName, 0, MAX_PATH);
                    if (_tcscspn(pServiceConfig->lpBinaryPathName, _T("\"")))
                    {
                        _tcsncpy(FileName, pServiceConfig->lpBinaryPathName, _tcscspn(pServiceConfig->lpBinaryPathName, _T(" ")) );
                    }
                    else
                    {
                        _tcscpy(FileName, pServiceConfig->lpBinaryPathName);
                    }

                    HeapFree(GetProcessHeap(), 0, pServiceConfig);
                    pServiceConfig = NULL;

                    dwLen = GetFileVersionInfoSize(FileName, &dwHandle);
                    if (dwLen)
                    {
                        lpData = HeapAlloc(GetProcessHeap(), 0, dwLen);
                        if (lpData == NULL)
                        {
                            HeapFree(GetProcessHeap(), 0, pServiceStatus);
                            CloseServiceHandle(hService);
                            CloseServiceHandle(ScHandle);
                            return;
                        }
                        if (!GetFileVersionInfo (FileName, dwHandle, dwLen, lpData))
                        {
                            HeapFree(GetProcessHeap(), 0, lpData);
                            HeapFree(GetProcessHeap(), 0, pServiceStatus);
                            CloseServiceHandle(hService);
                            CloseServiceHandle(ScHandle);
                            return;
                        }

                        if (VerQueryValue(lpData, _T("\\VarFileInfo\\Translation"), &pvData, (PUINT) &BufLen))
                        {
                            wCodePage = LOWORD(*(DWORD*) pvData);
                            wLangID = HIWORD(*(DWORD*) pvData);
                            wsprintf(szStrFileInfo, _T("StringFileInfo\\%04X%04X\\CompanyName"), wCodePage, wLangID);
                        }

                        if (VerQueryValue (lpData, szStrFileInfo, (void**) &lpBuffer, (PUINT) &BufLen))
                        {
                            item.pszText = lpBuffer;
                            item.iSubItem = 2;
                            SendMessage(hServicesListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                        }
                        HeapFree(GetProcessHeap(), 0, lpData);
                    }
                    else
                    {
                        LoadString(hInst, IDS_SERVICES_UNKNOWN, szStatus, 128);
                        item.pszText = szStatus;
                        item.iSubItem = 2;
                        SendMessage(hServicesListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                    }
                    CloseServiceHandle(hService);
                }

                LoadString(hInst, ((pServiceStatus[Index].ServiceStatusProcess.dwCurrentState == SERVICE_STOPPED) ? IDS_SERVICES_STATUS_STOPPED : IDS_SERVICES_STATUS_RUNNING), szStatus, 128);
                item.pszText = szStatus;
                item.iSubItem = 3;
                SendMessage(hServicesListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);

            }
        }

        HeapFree(GetProcessHeap(), 0, pServiceStatus);
        CloseServiceHandle(ScHandle);
    }

}
