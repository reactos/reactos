/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/stop_dependencies.c
 * PURPOSE:     Routines related to stopping dependent services
 * COPYRIGHT:   Copyright 2006-2015 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

typedef struct _STOP_DATA
{
    LPWSTR ServiceName;
    LPWSTR DisplayName;
    LPWSTR ServiceList;

} STOP_DATA, *PSTOP_DATA;

static LPWSTR
AddServiceToList(LPWSTR *lpServiceList,
                 LPWSTR lpServiceToAdd)
{
    LPWSTR lpNewList = NULL;
    LPWSTR ptr;
    DWORD dwToAddSize;
    DWORD dwCurSize;

    dwToAddSize = wcslen(lpServiceToAdd) + 1;

    /* Is this is the first in the list? */
    if (!*lpServiceList)
    {
        /* Add another char for double null */
        dwToAddSize++;

        lpNewList = HeapAlloc(GetProcessHeap(),
                              0,
                              dwToAddSize * sizeof(WCHAR));
        if (lpNewList)
        {
            /* Copy the service name */
            StringCchCopy(lpNewList,
                          dwToAddSize,
                          lpServiceToAdd);

            /* Add the double null char */
            lpNewList[dwToAddSize - 1] = L'\0';
        }
    }
    else
    {
        ptr = *lpServiceList;
        dwCurSize = 0;

        /* Get the list size */
        for (;;)
        {
            /* Break when we hit the double null */
            if (*ptr == L'\0' && *(ptr + 1) == L'\0')
                break;

            ptr++;
            dwCurSize++;
        }
        dwCurSize++;

        /* Add another char for double null */
        dwCurSize++;

        /* Extend the list size */
        lpNewList = HeapReAlloc(GetProcessHeap(),
                                0,
                                *lpServiceList,
                                (dwCurSize + dwToAddSize) * sizeof(WCHAR));
        if (lpNewList)
        {
            /* Copy the service name */
            StringCchCopy(&lpNewList[dwCurSize - 1],
                          dwToAddSize,
                          lpServiceToAdd);

            /* Add the double null char */
            lpNewList[dwCurSize + dwToAddSize - 1] = L'\0';
        }
    }

    return lpNewList;
}

static BOOL
BuildListOfServicesToStop(LPWSTR *lpServiceList,
                          LPWSTR lpServiceName)
{
    LPENUM_SERVICE_STATUS lpServiceStatus;
    DWORD dwCount, i;
    BOOL bRet = FALSE;

    /* Get a list of service dependents */
    lpServiceStatus = TV2_GetDependants(lpServiceName, &dwCount);
    if (lpServiceStatus)
    {
        for (i = 0; i < dwCount; i++)
        {
            /* Does this service need stopping? */
            if (lpServiceStatus[i].ServiceStatus.dwCurrentState != SERVICE_STOPPED &&
                lpServiceStatus[i].ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
            {
                /* Add the service to the list */
                *lpServiceList = AddServiceToList(lpServiceList, lpServiceStatus[i].lpServiceName);

                /* We've got one */
                bRet = TRUE;
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpServiceStatus);
    }

    return bRet;
}

LPWSTR
GetListOfServicesToStop(LPWSTR lpServiceName)
{
    LPWSTR lpServiceList = NULL;

    /* Call recursive function to get our list */
    if (BuildListOfServicesToStop(&lpServiceList, lpServiceName))
        return lpServiceList;
    else
        return NULL;
}

static VOID
AddServiceNamesToStop(HWND hServiceListBox,
                      LPWSTR lpServiceList)
{
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    LPWSTR lpStr;

    lpStr = lpServiceList;

    /* Loop through all the services in the list */
    for (;;)
    {
        /* Break when we hit the double null */
        if (*lpStr == L'\0' && *(lpStr + 1) == L'\0')
            break;

        /* If this isn't our first time in the loop we'll
           have been left on a null char */
        if (*lpStr == L'\0')
            lpStr++;

        /* Get the service's display name */
        lpServiceConfig = GetServiceConfig(lpStr);
        if (lpServiceConfig)
        {
            /* Add the service to the listbox */
            SendMessageW(hServiceListBox,
                         LB_ADDSTRING,
                         0,
                         (LPARAM)lpServiceConfig->lpDisplayName);

            HeapFree(GetProcessHeap(), 0, lpServiceConfig);
        }

        /* Move onto the next string */
        while (*lpStr != L'\0')
            lpStr++;
    }
}

static BOOL
InitDialog(HWND hDlg,
           UINT Message,
           WPARAM wParam,
           LPARAM lParam)
{
    PSTOP_DATA StopData;
    HWND hServiceListBox;
    LPWSTR lpPartialStr, lpStr;
    DWORD fullLen;
    HICON hIcon = NULL;
    BOOL bRet = FALSE;

    StopData = (PSTOP_DATA)lParam;


    /* Load the icon for the window */
    hIcon = (HICON)LoadImageW(hInstance,
                                MAKEINTRESOURCE(IDI_SM_ICON),
                                IMAGE_ICON,
                                GetSystemMetrics(SM_CXSMICON),
                                GetSystemMetrics(SM_CXSMICON),
                                0);
    if (hIcon)
    {
        /* Set it */
        SendMessageW(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);
        DestroyIcon(hIcon);
    }

    /* Load the stop depends note */
    if (AllocAndLoadString(&lpPartialStr,
                            hInstance,
                            IDS_STOP_DEPENDS))
    {
        /* Get the length required */
        fullLen = wcslen(lpPartialStr) + wcslen(StopData->DisplayName) + 1;

        lpStr = HeapAlloc(ProcessHeap,
                          0,
                          fullLen * sizeof(WCHAR));
        if (lpStr)
        {
            /* Add the service name to the depends note */
            _snwprintf(lpStr,
                        fullLen,
                        lpPartialStr,
                        StopData->DisplayName);

            /* Add the string to the dialog */
            SendDlgItemMessageW(hDlg,
                                IDC_STOP_DEPENDS,
                                WM_SETTEXT,
                                0,
                                (LPARAM)lpStr);

            HeapFree(ProcessHeap,
                        0,
                        lpStr);

            bRet = TRUE;
        }

        LocalFree(lpPartialStr);
    }

    /* Display the list of services which need stopping */
    hServiceListBox = GetDlgItem(hDlg, IDC_STOP_DEPENDS_LB);
    if (hServiceListBox)
    {
        AddServiceNamesToStop(hServiceListBox,
                              (LPWSTR)StopData->ServiceList);
    }

    return bRet;
}

INT_PTR CALLBACK
StopDependsDialogProc(HWND hDlg,
                      UINT Message,
                      WPARAM wParam,
                      LPARAM lParam)
{

    switch (Message)
    {
        case WM_INITDIALOG:
        {
            return InitDialog(hDlg,
                              Message,
                              wParam,
                              lParam);
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                {
                    EndDialog(hDlg,
                              LOWORD(wParam));
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

BOOL
CreateStopDependsDialog(HWND hParent,
                        LPWSTR ServiceName,
                        LPWSTR DisplayName,
                        LPWSTR ServiceList)
{
    STOP_DATA StopData;
    INT_PTR Result;

    StopData.ServiceName = ServiceName;
    StopData.DisplayName = DisplayName;
    StopData.ServiceList = ServiceList;

    Result = DialogBoxParamW(hInstance,
                             MAKEINTRESOURCEW(IDD_DLG_DEPEND_STOP),
                             hParent,
                             StopDependsDialogProc,
                             (LPARAM)&StopData);
    if (Result == IDOK)
        return TRUE;

    return FALSE;
}
