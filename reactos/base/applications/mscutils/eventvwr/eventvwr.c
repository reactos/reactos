/*
 *  ReactOS Win32 Applications
 *  Copyright (C) 2007 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT : See COPYING in the top level directory
 * PROJECT   : Event Log Viewer
 * FILE      : eventvwr.c
 * PROGRAMMER: Marc Piulachs (marc.piulachs at codexchange [dot] net)
 */

#include <stdio.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winreg.h>
#include <commctrl.h>
#include <commdlg.h>
#include <strsafe.h>

#include "resource.h"

#if _MSC_VER
    #pragma warning(disable: 4996)   /* 'strdup' was declared deprecated */
    #define _CRT_SECURE_NO_DEPRECATE /* all deprecated unsafe string functions */
#endif

static const WCHAR szWindowClass[]           = L"EVENTVWR"; /* the main window class name*/
static const WCHAR EVENTLOG_BASE_KEY[]       = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\";

// MessageFile message buffer size
#define EVENT_MESSAGE_EVENTTEXT_BUFFER  1024*10
#define EVENT_MESSAGE_FILE_BUFFER       1024*10
#define EVENT_DLL_SEPARATOR             L";"
#define EVENT_MESSAGE_FILE              L"EventMessageFile"
#define EVENT_CATEGORY_MESSAGE_FILE     L"CategoryMessageFile"
#define EVENT_PARAMETER_MESSAGE_FILE    L"ParameterMessageFile"

#define MAX_LOADSTRING 255
#define ENTRY_SIZE 2056

/* Globals */
HINSTANCE hInst;                            /* current instance */
WCHAR szTitle[MAX_LOADSTRING];              /* The title bar text */
WCHAR szTitleTemplate[MAX_LOADSTRING];      /* The logged-on title bar text */
WCHAR szSaveFilter[MAX_LOADSTRING];         /* Filter Mask for the save Dialog */
HWND hwndMainWindow;                        /* Main window */
HWND hwndListView;                          /* ListView control */
HWND hwndStatus;                            /* Status bar */
HMENU hMainMenu;                            /* The application's main menu */
WCHAR szStatusBarTemplate[MAX_LOADSTRING];  /* The status bar text */
PEVENTLOGRECORD *g_RecordPtrs = NULL;
DWORD g_TotalRecords = 0;
OPENFILENAMEW sfn;

LPWSTR lpSourceLogName = NULL;
LPWSTR lpComputerName  = NULL;

DWORD dwNumLogs = 0;
WCHAR **LogNames;

/* Forward declarations of functions included in this code module: */
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK EventDetails(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK StatusMessageWindowProc (HWND, UINT, WPARAM, LPARAM);


int APIENTRY
wWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPWSTR    lpCmdLine,
          int       nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;
    INITCOMMONCONTROLSEX iccx;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    /* Whenever any of the common controls are used in your app,
     * you must call InitCommonControlsEx() to register the classes
     * for those controls. */
    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccx);

    /* Initialize global strings */
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDS_APP_TITLE_EX, szTitleTemplate, MAX_LOADSTRING);
    LoadStringW(hInstance, IDS_STATUS_MSG, szStatusBarTemplate, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    /* Perform application initialization: */
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_EVENTVWR));

    /* Main message loop: */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

static void FreeRecords(void)
{
    DWORD iIndex;

    if (!g_RecordPtrs)
        return;

    for (iIndex = 0; iIndex < g_TotalRecords; iIndex++)
        HeapFree(GetProcessHeap(), 0, g_RecordPtrs[iIndex]);
    HeapFree(GetProcessHeap(), 0, g_RecordPtrs);
    g_RecordPtrs = NULL;
}

VOID
ShowLastWin32Error(VOID)
{
    DWORD dwError;
    LPWSTR lpMessageBuffer;

    dwError = GetLastError();
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   dwError,
                   0,
                   (LPWSTR)&lpMessageBuffer,
                   0,
                   NULL);

    MessageBoxW(hwndMainWindow, lpMessageBuffer, szTitle, MB_OK | MB_ICONERROR);
    LocalFree(lpMessageBuffer);
}

VOID
EventTimeToSystemTime(DWORD EventTime,
                      SYSTEMTIME *pSystemTime)
{
    SYSTEMTIME st1970 = { 1970, 1, 0, 1, 0, 0, 0, 0 };
    FILETIME ftLocal;
    union
    {
        FILETIME ft;
        ULONGLONG ll;
    } u1970, uUCT;

    uUCT.ft.dwHighDateTime = 0;
    uUCT.ft.dwLowDateTime = EventTime;
    SystemTimeToFileTime(&st1970, &u1970.ft);
    uUCT.ll = uUCT.ll * 10000000 + u1970.ll;
    FileTimeToLocalFileTime(&uUCT.ft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, pSystemTime);
}


void
TrimNulls(LPWSTR s)
{
    WCHAR *c;

    if (s != NULL)
    {
        c = s + wcslen(s) - 1;
        while (c >= s && iswspace(*c))
            --c;
        *++c = L'\0';
    }
}


BOOL
GetEventMessageFileDLL(IN LPCWSTR lpLogName,
                       IN LPCWSTR SourceName,
                       IN LPCWSTR EntryName,
                       OUT PWCHAR ExpandedName)
{
    DWORD dwSize;
    BYTE szModuleName[MAX_PATH];
    WCHAR szKeyName[MAX_PATH];
    HKEY hAppKey = NULL;
    HKEY hSourceKey = NULL;
    BOOL bReturn = FALSE;

    StringCbCopyW(szKeyName, sizeof(szKeyName), L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\");
    StringCbCatW(szKeyName, sizeof(szKeyName), lpLogName);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     szKeyName,
                     0,
                     KEY_READ,
                     &hAppKey) == ERROR_SUCCESS)
    {
        if (RegOpenKeyExW(hAppKey,
                         SourceName,
                         0,
                         KEY_READ,
                         &hSourceKey) == ERROR_SUCCESS)
        {
            dwSize = MAX_PATH;
            if (RegQueryValueExW(hSourceKey,
                                EntryName,
                                NULL,
                                NULL,
                                (LPBYTE)szModuleName,
                                &dwSize) == ERROR_SUCCESS)
            {
                /* Returns a string containing the requested substituted environment variable. */
                ExpandEnvironmentStringsW((LPCWSTR)szModuleName, ExpandedName, MAX_PATH);

                /* Successful */
                bReturn = TRUE;
            }
        }
    }
    else
    {
        ShowLastWin32Error();
    }

    if (hSourceKey != NULL)
        RegCloseKey(hSourceKey);

    if (hAppKey != NULL)
        RegCloseKey(hAppKey);

    return bReturn;
}


BOOL
GetEventCategory(IN LPCWSTR KeyName,
                 IN LPCWSTR SourceName,
                 IN EVENTLOGRECORD *pevlr,
                 OUT PWCHAR CategoryName)
{
    HANDLE hLibrary = NULL;
    WCHAR szMessageDLL[MAX_PATH];
    LPVOID lpMsgBuf = NULL;

    if (GetEventMessageFileDLL (KeyName, SourceName, EVENT_CATEGORY_MESSAGE_FILE , szMessageDLL))
    {
        hLibrary = LoadLibraryExW(szMessageDLL,
                                 NULL,
                                 DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
        if (hLibrary != NULL)
        {
            /* Retrieve the message string. */
            if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                              hLibrary,
                              pevlr->EventCategory,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPWSTR)&lpMsgBuf,
                              EVENT_MESSAGE_FILE_BUFFER,
                              NULL) != 0)
            {
                /* Trim the string */
                TrimNulls(lpMsgBuf);

                /* Copy the category name */
                StringCchCopyW(CategoryName, MAX_PATH, lpMsgBuf);
            }
            else
            {
                LoadStringW(hInst, IDS_NONE, CategoryName, MAX_PATH);
            }

            if (hLibrary != NULL)
                FreeLibrary(hLibrary);

            /* Free the buffer allocated by FormatMessage */
            if (lpMsgBuf)
                LocalFree(lpMsgBuf);

            return TRUE;
        }
    }

    LoadStringW(hInst, IDS_NONE, CategoryName, MAX_PATH);

    return FALSE;
}


BOOL
GetEventMessage(IN LPCWSTR KeyName,
                IN LPCWSTR SourceName,
                IN EVENTLOGRECORD *pevlr,
                OUT PWCHAR EventText)
{
    DWORD i;
    HANDLE hLibrary = NULL;
    WCHAR SourceModuleName[1000];
    WCHAR ParameterModuleName[1000];
    LPWSTR lpMsgBuf = NULL;
    WCHAR szStringIDNotFound[MAX_LOADSTRING];
    LPWSTR szDll;
    LPWSTR szMessage;
    LPWSTR *szArguments;
    BOOL bDone = FALSE;

    /* TODO : GetEventMessageFileDLL can return a comma separated list of DLLs */
    if (GetEventMessageFileDLL (KeyName, SourceName, EVENT_MESSAGE_FILE, SourceModuleName))
    {
        /* Get the event message */
        szMessage = (LPWSTR)((LPBYTE)pevlr + pevlr->StringOffset);

        /* Allocate space for parameters */
        szArguments = malloc(sizeof(LPVOID) * pevlr->NumStrings);
        if (!szArguments)
        {
            return FALSE;
        }

        for (i = 0; i < pevlr->NumStrings ; i++)
        {
            if (wcsstr(szMessage , L"%%"))
            {
                if (GetEventMessageFileDLL(KeyName, SourceName, EVENT_PARAMETER_MESSAGE_FILE, ParameterModuleName))
                {
                    /* Not yet support for reading messages from parameter message DLL */
                }
            }

            szArguments[i] = szMessage;
            szMessage += wcslen(szMessage) + 1;
        }

        szDll = wcstok(SourceModuleName, EVENT_DLL_SEPARATOR);
        while ((szDll != NULL) && (!bDone))
        {
            hLibrary = LoadLibraryExW(szDll,
                                     NULL,
                                     DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
            if (hLibrary == NULL)
            {
                /* The DLL could not be loaded try the next one (if any) */
                szDll = wcstok(NULL, EVENT_DLL_SEPARATOR);
            }
            else
            {
                /* Retrieve the message string. */
                if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                  FORMAT_MESSAGE_FROM_HMODULE |
                                  FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                  hLibrary,
                                  pevlr->EventID,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPWSTR)&lpMsgBuf,
                                  0,
                                  (va_list*)szArguments) == 0)
                {
                    /* We haven't found the string , get next DLL (if any) */
                    szDll = wcstok(NULL, EVENT_DLL_SEPARATOR);
                }
                else
                {
                    if (lpMsgBuf)
                    {
                        /* The ID was found and the message was formated */
                        bDone = TRUE;

                        /* Trim the string */
                        TrimNulls((LPWSTR)lpMsgBuf);

                        /* Copy the event text */
                        StringCchCopyW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, lpMsgBuf);
                    }
                }

                FreeLibrary(hLibrary);
            }
        }

        if (!bDone)
        {
            LoadStringW(hInst, IDS_EVENTSTRINGIDNOTFOUND, szStringIDNotFound, MAX_LOADSTRING);
            StringCchPrintfW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, szStringIDNotFound, (pevlr->EventID & 0xFFFF), SourceName);
        }

        free(szArguments);

        /* No more dlls to try, return result */
        return bDone;
    }

    LoadStringW(hInst, IDS_EVENTSTRINGIDNOTFOUND, szStringIDNotFound, MAX_LOADSTRING);
    StringCchPrintfW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, szStringIDNotFound, (pevlr->EventID & 0xFFFF), SourceName);

    return FALSE;
}


VOID
GetEventType(IN WORD dwEventType,
             OUT PWCHAR eventTypeText)
{
    switch (dwEventType)
    {
        case EVENTLOG_ERROR_TYPE:
            LoadStringW(hInst, IDS_EVENTLOG_ERROR_TYPE, eventTypeText, MAX_LOADSTRING);
            break;
        case EVENTLOG_WARNING_TYPE:
            LoadStringW(hInst, IDS_EVENTLOG_WARNING_TYPE, eventTypeText, MAX_LOADSTRING);
            break;
        case EVENTLOG_INFORMATION_TYPE:
            LoadStringW(hInst, IDS_EVENTLOG_INFORMATION_TYPE, eventTypeText, MAX_LOADSTRING);
            break;
        case EVENTLOG_AUDIT_SUCCESS:
            LoadStringW(hInst, IDS_EVENTLOG_AUDIT_SUCCESS, eventTypeText, MAX_LOADSTRING);
            break;
        case EVENTLOG_AUDIT_FAILURE:
            LoadStringW(hInst, IDS_EVENTLOG_AUDIT_FAILURE, eventTypeText, MAX_LOADSTRING);
            break;
        case EVENTLOG_SUCCESS:
            LoadStringW(hInst, IDS_EVENTLOG_SUCCESS, eventTypeText, MAX_LOADSTRING);
            break;
        default:
            LoadStringW(hInst, IDS_EVENTLOG_UNKNOWN_TYPE, eventTypeText, MAX_LOADSTRING);
            break;
    }
}

BOOL
GetEventUserName(EVENTLOGRECORD *pelr,
                 OUT PWCHAR pszUser)
{
    PSID lpSid;
    WCHAR szName[1024];
    WCHAR szDomain[1024];
    SID_NAME_USE peUse;
    DWORD cbName = 1024;
    DWORD cbDomain = 1024;

    /* Point to the SID. */
    lpSid = (PSID)((LPBYTE)pelr + pelr->UserSidOffset);

    /* User SID */
    if (pelr->UserSidLength > 0)
    {
        if (LookupAccountSidW(NULL,
                             lpSid,
                             szName,
                             &cbName,
                             szDomain,
                             &cbDomain,
                             &peUse))
        {
            StringCchCopyW(pszUser, MAX_PATH, szName);
            return TRUE;
        }
    }

    return FALSE;
}


static DWORD WINAPI
ShowStatusMessageThread(IN LPVOID lpParameter)
{
    HWND *phWnd = (HWND *)lpParameter;
    HWND hWnd;
    MSG Msg;

    hWnd = CreateDialogParam(hInst,
                             MAKEINTRESOURCE(IDD_PROGRESSBOX),
                             GetDesktopWindow(),
                             StatusMessageWindowProc,
                             (LPARAM)NULL);
    if (!hWnd)
        return 0;

    *phWnd = hWnd;

    ShowWindow(hWnd, SW_SHOW);

    /* Message loop for the Status window */
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return 0;
}


BOOL
QueryEventMessages(LPWSTR lpMachineName,
                   LPWSTR lpLogName)
{
    HWND hwndDlg = NULL;
    HANDLE hEventLog;
    EVENTLOGRECORD *pevlr;
    DWORD dwRead, dwNeeded, dwThisRecord, dwTotalRecords = 0, dwCurrentRecord = 0, dwRecordsToRead = 0, dwFlags, dwMaxLength;
    size_t cchRemaining;
    LPWSTR lpSourceName;
    LPWSTR lpComputerName;
    LPSTR lpData;
    BOOL bResult = TRUE; /* Read succeeded. */

    WCHAR szWindowTitle[MAX_PATH];
    WCHAR szStatusText[MAX_PATH];
    WCHAR szLocalDate[MAX_PATH];
    WCHAR szLocalTime[MAX_PATH];
    WCHAR szEventID[MAX_PATH];
    WCHAR szEventTypeText[MAX_LOADSTRING];
    WCHAR szCategoryID[MAX_PATH];
    WCHAR szUsername[MAX_PATH];
    WCHAR szEventText[EVENT_MESSAGE_FILE_BUFFER];
    WCHAR szCategory[MAX_PATH];
    WCHAR szData[MAX_PATH];
    PWCHAR lpTitleTemplateEnd;

    SYSTEMTIME time;
    LVITEMW lviEventItem;

    dwFlags = EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ;

    /* Open the event log. */
    hEventLog = OpenEventLogW(lpMachineName,
                             lpLogName);
    if (hEventLog == NULL)
    {
        ShowLastWin32Error();
        return FALSE;
    }

    lpSourceLogName = lpLogName;
    lpComputerName = lpMachineName;

    /* Disable listview redraw */
    SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);

    /* Clear the list view */
    (void)ListView_DeleteAllItems (hwndListView);
    FreeRecords();

    GetOldestEventLogRecord(hEventLog, &dwThisRecord);

    /* Get the total number of event log records. */
    GetNumberOfEventLogRecords (hEventLog , &dwTotalRecords);
    g_TotalRecords = dwTotalRecords;

    if (dwTotalRecords > 0)
    {
        EnableMenuItem(hMainMenu, IDM_CLEAR_EVENTS, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hMainMenu, IDM_SAVE_PROTOCOL, MF_BYCOMMAND | MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hMainMenu, IDM_CLEAR_EVENTS, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMainMenu, IDM_SAVE_PROTOCOL, MF_BYCOMMAND | MF_GRAYED);
    }

    g_RecordPtrs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwTotalRecords * sizeof(PVOID));

    /* If we have at least 1000 records show the waiting dialog */
    if (dwTotalRecords > 1000)
    {
        CloseHandle(CreateThread(NULL,
                                 0,
                                 ShowStatusMessageThread,
                                 (LPVOID)&hwndDlg,
                                 0,
                                 NULL));
    }

    while (dwCurrentRecord < dwTotalRecords)
    {
        pevlr = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));
        g_RecordPtrs[dwCurrentRecord] = pevlr;

        bResult = ReadEventLog(hEventLog,  // Event log handle
                               dwFlags,    // Sequential read
                               0,          // Ignored for sequential read
                               pevlr,      // Pointer to buffer
                               sizeof(EVENTLOGRECORD),   // Size of buffer
                               &dwRead,    // Number of bytes read
                               &dwNeeded); // Bytes in the next record
        if((!bResult) && (GetLastError () == ERROR_INSUFFICIENT_BUFFER))
        {
            HeapFree(GetProcessHeap(), 0, pevlr);
            pevlr = HeapAlloc(GetProcessHeap(), 0, dwNeeded);
            g_RecordPtrs[dwCurrentRecord] = pevlr;

            ReadEventLogW(hEventLog,  // event log handle
                         dwFlags,    // read flags
                         0,          // offset; default is 0
                         pevlr,      // pointer to buffer
                         dwNeeded,   // size of buffer
                         &dwRead,    // number of bytes read
                         &dwNeeded); // bytes in next record
        }

        while (dwRead > 0)
        {
            LoadStringW(hInst, IDS_NOT_AVAILABLE, szUsername, MAX_PATH);
            LoadStringW(hInst, IDS_NOT_AVAILABLE, szEventText, MAX_PATH);
            LoadStringW(hInst, IDS_NONE, szCategory, MAX_PATH);

            // Get the event source name.
            lpSourceName = (LPWSTR)((LPBYTE)pevlr + sizeof(EVENTLOGRECORD));

            // Get the computer name
            lpComputerName = (LPWSTR)((LPBYTE)pevlr + sizeof(EVENTLOGRECORD) + (wcslen(lpSourceName) + 1) * sizeof(WCHAR));

            // This ist the data section of the current event
            lpData = (LPSTR)((LPBYTE)pevlr + pevlr->DataOffset);

            // Compute the event type
            EventTimeToSystemTime(pevlr->TimeWritten, &time);

            // Get the username that generated the event
            GetEventUserName(pevlr, szUsername);

            GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, szLocalDate, MAX_PATH);
            GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &time, NULL, szLocalTime, MAX_PATH);

            GetEventType(pevlr->EventType, szEventTypeText);
            GetEventCategory(lpLogName, lpSourceName, pevlr, szCategory);

            StringCbPrintfW(szEventID, sizeof(szEventID), L"%u", (pevlr->EventID & 0xFFFF));
            StringCbPrintfW(szCategoryID, sizeof(szCategoryID), L"%u", pevlr->EventCategory);

            lviEventItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
            lviEventItem.iItem = 0;
            lviEventItem.iSubItem = 0;
            lviEventItem.lParam = (LPARAM)pevlr;
            lviEventItem.pszText = szEventTypeText;

            switch (pevlr->EventType)
            {
                case EVENTLOG_ERROR_TYPE:
                    lviEventItem.iImage = 2;
                    break;

                case EVENTLOG_AUDIT_FAILURE:
                    lviEventItem.iImage = 2;
                    break;

                case EVENTLOG_WARNING_TYPE:
                    lviEventItem.iImage = 1;
                    break;

                case EVENTLOG_INFORMATION_TYPE:
                    lviEventItem.iImage = 0;
                    break;

                case EVENTLOG_AUDIT_SUCCESS:
                    lviEventItem.iImage = 0;
                    break;

                case EVENTLOG_SUCCESS:
                    lviEventItem.iImage = 0;
                    break;
            }

            lviEventItem.iItem = ListView_InsertItem(hwndListView, &lviEventItem);

            ListView_SetItemText(hwndListView, lviEventItem.iItem, 1, szLocalDate);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 2, szLocalTime);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 3, lpSourceName);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 4, szCategory);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 5, szEventID);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 6, szUsername); //User
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 7, lpComputerName); //Computer
            MultiByteToWideChar(CP_ACP,
                                0,
                                lpData,
                                pevlr->DataLength,
                                szData,
                                MAX_PATH);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 8, szData); //Event Text

            dwRead -= pevlr->Length;
            pevlr = (EVENTLOGRECORD *)((LPBYTE) pevlr + pevlr->Length);
        }

        dwRecordsToRead--;
        dwCurrentRecord++;
    }

    // All events loaded
    if(hwndDlg)
        EndDialog(hwndDlg, 0);

    StringCchPrintfExW(szWindowTitle,
                       sizeof(szWindowTitle) / sizeof(WCHAR),
                       &lpTitleTemplateEnd,
                       &cchRemaining,
                       0,
                       szTitleTemplate, szTitle, lpLogName); /* i = number of characters written */
    /* lpComputerName can be NULL here if no records was read */
    dwMaxLength = (DWORD)cchRemaining;
    if (!lpComputerName)
        GetComputerNameW(lpTitleTemplateEnd, &dwMaxLength);
    else
        StringCchCopyW(lpTitleTemplateEnd, dwMaxLength, lpComputerName);

    StringCbPrintfW(szStatusText, sizeof(szStatusText), szStatusBarTemplate, lpLogName, dwTotalRecords);

    // Update the status bar
    SendMessageW(hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)szStatusText);

    // Set the window title
    SetWindowTextW(hwndMainWindow, szWindowTitle);

    // Resume list view redraw
    SendMessageW(hwndListView, WM_SETREDRAW, TRUE, 0);

    // Close the event log.
    CloseEventLog(hEventLog);

    return TRUE;
}


VOID
SaveProtocol(VOID)
{
    HANDLE hEventLog;
    WCHAR szFileName[MAX_PATH];

    ZeroMemory(szFileName, sizeof(szFileName));

    sfn.lpstrFile = szFileName;
    sfn.nMaxFile  = MAX_PATH;

    if (!GetSaveFileNameW(&sfn))
    {
        return;
    }

    hEventLog = OpenEventLogW(lpComputerName, lpSourceLogName);
    if (!hEventLog)
    {
        ShowLastWin32Error();
        return;
    }

    if (!BackupEventLogW(hEventLog, szFileName))
    {
        ShowLastWin32Error();
    }

    CloseEventLog(hEventLog);
}


BOOL
ClearEvents(VOID)
{
    HANDLE hEventLog;
    WCHAR szFileName[MAX_PATH];
    WCHAR szMessage[MAX_LOADSTRING];

    ZeroMemory(szFileName, sizeof(szFileName));
    ZeroMemory(szMessage, sizeof(szMessage));

    LoadStringW(hInst, IDS_CLEAREVENTS_MSG, szMessage, MAX_LOADSTRING);

    sfn.lpstrFile = szFileName;
    sfn.nMaxFile  = MAX_PATH;

    switch (MessageBoxW(hwndMainWindow, szMessage, szTitle, MB_YESNOCANCEL | MB_ICONINFORMATION))
    {
        case IDCANCEL:
        {
            return FALSE;
        }

        case IDNO:
        {
            sfn.lpstrFile = NULL;
            break;
        }

        case IDYES:
        {
            if (!GetSaveFileNameW(&sfn))
            {
                return FALSE;
            }
            break;
        }
    }

    hEventLog = OpenEventLogW(lpComputerName, lpSourceLogName);
    if (!hEventLog)
    {
        ShowLastWin32Error();
        return FALSE;
    }

    if (!ClearEventLogW(hEventLog, sfn.lpstrFile))
    {
        ShowLastWin32Error();
        CloseEventLog(hEventLog);
        return FALSE;
    }

    CloseEventLog(hEventLog);

    return TRUE;
}


VOID
Refresh(VOID)
{
    QueryEventMessages(lpComputerName,
                       lpSourceLogName);
}


ATOM
MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EVENTVWR));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDM_EVENTVWR);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = (HICON)LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_EVENTVWR),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_SHARED);

    return RegisterClassExW(&wcex);
}


VOID
GetDisplayNameFile(IN LPCWSTR lpLogName,
                   OUT PWCHAR lpModuleName)
{
    HKEY hKey;
    WCHAR *KeyPath;
    WCHAR szModuleName[MAX_PATH];
    DWORD cbData;
    SIZE_T cbKeyPath;

    cbKeyPath = (wcslen(EVENTLOG_BASE_KEY) + wcslen(lpLogName) + 1) * sizeof(WCHAR);
    KeyPath = HeapAlloc(GetProcessHeap(), 0, cbKeyPath);
    if (!KeyPath)
    {
        return;
    }

    StringCbCopyW(KeyPath, cbKeyPath, EVENTLOG_BASE_KEY);
    StringCbCatW(KeyPath, cbKeyPath, lpLogName);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, KeyPath);
        return;
    }

    cbData = sizeof(szModuleName);
    if (RegQueryValueExW(hKey, L"DisplayNameFile", NULL, NULL, (LPBYTE)szModuleName, &cbData) == ERROR_SUCCESS)
    {
        ExpandEnvironmentStringsW(szModuleName, lpModuleName, MAX_PATH);
    }

    RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, KeyPath);
}


DWORD
GetDisplayNameID(IN LPCWSTR lpLogName)
{
    HKEY hKey;
    WCHAR *KeyPath;
    DWORD dwMessageID = 0;
    DWORD cbData;
    SIZE_T cbKeyPath;

    cbKeyPath = (wcslen(EVENTLOG_BASE_KEY) + wcslen(lpLogName) + 1) * sizeof(WCHAR);
    KeyPath = HeapAlloc(GetProcessHeap(), 0, cbKeyPath);
    if (!KeyPath)
    {
        return 0;
    }

    StringCbCopyW(KeyPath, cbKeyPath, EVENTLOG_BASE_KEY);
    StringCbCatW(KeyPath, cbKeyPath, lpLogName);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, KeyPath);
        return 0;
    }

    cbData = sizeof(dwMessageID);
    RegQueryValueExW(hKey, L"DisplayNameID", NULL, NULL, (LPBYTE)&dwMessageID, &cbData);

    RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, KeyPath);

    return dwMessageID;
}


VOID
BuildLogList(void)
{
    HKEY hKey;
    DWORD lpcName;
    DWORD dwIndex;
    DWORD dwMessageID;
    DWORD dwMaxKeyLength;
    WCHAR szModuleName[MAX_PATH];
    LPWSTR lpDisplayName;
    HANDLE hLibrary = NULL;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, EVENTLOG_BASE_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    if (RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwNumLogs, &dwMaxKeyLength, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    if (!dwNumLogs)
    {
        RegCloseKey(hKey);
        return;
    }

    LogNames = HeapAlloc(GetProcessHeap(), 0, (dwNumLogs + 1) * sizeof(WCHAR*));

    if (!LogNames)
    {
        RegCloseKey(hKey);
        return;
    }

    for (dwIndex = 0; dwIndex < dwNumLogs; dwIndex++)
    {
        LogNames[dwIndex] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((dwMaxKeyLength + 1) * sizeof(WCHAR)));

        if (LogNames[dwIndex] != NULL)
        {
            lpcName = dwMaxKeyLength + 1;

            if (RegEnumKeyExW(hKey, dwIndex, LogNames[dwIndex], &lpcName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                lpDisplayName = NULL;

                ZeroMemory(szModuleName, sizeof(szModuleName));
                GetDisplayNameFile(LogNames[dwIndex], szModuleName);
                dwMessageID = GetDisplayNameID(LogNames[dwIndex]);

                hLibrary = LoadLibraryExW(szModuleName, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
                if (hLibrary != NULL)
                {
                    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, hLibrary, dwMessageID, 0, (LPWSTR)&lpDisplayName, 0, NULL);
                    FreeLibrary(hLibrary);
                }

                if (lpDisplayName)
                {
                    InsertMenuW(hMainMenu, IDM_SAVE_PROTOCOL, MF_BYCOMMAND | MF_STRING, ID_FIRST_LOG + dwIndex, lpDisplayName);
                }
                else
                {
                    InsertMenuW(hMainMenu, IDM_SAVE_PROTOCOL, MF_BYCOMMAND | MF_STRING, ID_FIRST_LOG + dwIndex, LogNames[dwIndex]);
                }

                LocalFree(lpDisplayName);
            }
        }
    }

    InsertMenuW(hMainMenu, IDM_SAVE_PROTOCOL, MF_BYCOMMAND | MF_SEPARATOR, ID_FIRST_LOG + dwIndex + 1, NULL);

    RegCloseKey(hKey);

    return;
}


VOID
FreeLogList(void)
{
    DWORD dwIndex;

    if (!LogNames)
    {
        return;
    }

    for (dwIndex = 0; dwIndex < dwNumLogs; dwIndex++)
    {
        if (LogNames[dwIndex])
        {
            HeapFree(GetProcessHeap(), 0, LogNames[dwIndex]);
        }

        DeleteMenu(hMainMenu, ID_FIRST_LOG + dwIndex, MF_BYCOMMAND);
    }

    DeleteMenu(hMainMenu, ID_FIRST_LOG + dwIndex + 1, MF_BYCOMMAND);

    HeapFree(GetProcessHeap(), 0, LogNames);

    dwNumLogs = 0;

    return;
}


BOOL
InitInstance(HINSTANCE hInstance,
             int nCmdShow)
{
    HIMAGELIST hSmall;
    LVCOLUMNW lvc = {0};
    WCHAR szTemp[256];

    hInst = hInstance; // Store instance handle in our global variable

    hwndMainWindow = CreateWindowW(szWindowClass,
                                  szTitle,
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  NULL);
    if (!hwndMainWindow)
    {
        return FALSE;
    }

    hwndStatus = CreateWindowExW(0,                                  // no extended styles
                                STATUSCLASSNAMEW,                    // status bar
                                L"Done.",                     // no text
                                WS_CHILD | WS_BORDER | WS_VISIBLE,  // styles
                                0, 0, 0, 0,                         // x, y, cx, cy
                                hwndMainWindow,                     // parent window
                                (HMENU)100,                         // window ID
                                hInstance,                          // instance
                                NULL);                              // window data

    // Create our listview child window.  Note that I use WS_EX_CLIENTEDGE
    // and WS_BORDER to create the normal "sunken" look.  Also note that
    // LVS_EX_ styles cannot be set in CreateWindowEx().
    hwndListView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                  WC_LISTVIEWW,
                                  L"",
                                  LVS_SHOWSELALWAYS | WS_CHILD | WS_VISIBLE | LVS_REPORT,
                                  0,
                                  0,
                                  243,
                                  200,
                                  hwndMainWindow,
                                  NULL,
                                  hInstance,
                                  NULL);

    // After the ListView is created, we can add extended list view styles.
    (void)ListView_SetExtendedListViewStyle (hwndListView, LVS_EX_FULLROWSELECT);

    // Create the ImageList
    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              ILC_MASK,
                              1,
                              1);

    // Add event type icons to ImageList
    ImageList_AddIcon (hSmall, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INFORMATIONICON)));
    ImageList_AddIcon (hSmall, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WARNINGICON)));
    ImageList_AddIcon (hSmall, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ERRORICON)));

    // Assign ImageList to List View
    (void)ListView_SetImageList (hwndListView, hSmall, LVSIL_SMALL);

    // Now set up the listview with its columns.
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = 90;
    LoadStringW(hInstance,
                IDS_COLUMNTYPE,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 0, &lvc);

    lvc.cx = 70;
    LoadStringW(hInstance,
                IDS_COLUMNDATE,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 1, &lvc);

    lvc.cx = 70;
    LoadStringW(hInstance,
                IDS_COLUMNTIME,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 2, &lvc);

    lvc.cx = 150;
    LoadStringW(hInstance,
                IDS_COLUMNSOURCE,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 3, &lvc);

    lvc.cx = 100;
    LoadStringW(hInstance,
                IDS_COLUMNCATEGORY,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 4, &lvc);

    lvc.cx = 60;
    LoadStringW(hInstance,
                IDS_COLUMNEVENT,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 5, &lvc);

    lvc.cx = 120;
    LoadStringW(hInstance,
                IDS_COLUMNUSER,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 6, &lvc);

    lvc.cx = 100;
    LoadStringW(hInstance,
                IDS_COLUMNCOMPUTER,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 7, &lvc);

    lvc.cx = 0;
    LoadStringW(hInstance,
                IDS_COLUMNEVENTDATA,
                szTemp,
                sizeof(szTemp) / sizeof(WCHAR));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 8, &lvc);

    // Initialize the save Dialog
    ZeroMemory(&sfn, sizeof(sfn));
    ZeroMemory(szSaveFilter, sizeof(szSaveFilter));

    LoadStringW(hInst, IDS_SAVE_FILTER, szSaveFilter, MAX_LOADSTRING);

    sfn.lStructSize     = sizeof(sfn);
    sfn.hwndOwner       = hwndMainWindow;
    sfn.hInstance       = hInstance;
    sfn.lpstrFilter     = szSaveFilter;
    sfn.lpstrInitialDir = NULL;
    sfn.Flags           = OFN_HIDEREADONLY | OFN_SHAREAWARE;
    sfn.lpstrDefExt     = NULL;

    ShowWindow(hwndMainWindow, nCmdShow);
    UpdateWindow(hwndMainWindow);

    BuildLogList();

    QueryEventMessages(lpComputerName, LogNames[0]);

    CheckMenuRadioItem(GetMenu(hwndMainWindow), ID_FIRST_LOG, ID_FIRST_LOG + dwNumLogs, ID_FIRST_LOG, MF_BYCOMMAND);

    return TRUE;
}


LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    NMHDR *hdr;

    switch (message)
    {
        case WM_CREATE:
            hMainMenu = GetMenu(hWnd);
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_DBLCLK :
                    hdr = (NMHDR FAR*)lParam;
                    if (hdr->hwndFrom == hwndListView)
                    {
                        LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;

                        if (lpnmitem->iItem != -1)
                        {
                            DialogBox(hInst,
                                      MAKEINTRESOURCE(IDD_EVENTPROPERTIES),
                                      hWnd,
                                      EventDetails);
                        }
                    }
                    break;
            }
            break;

        case WM_COMMAND:
            // Parse the menu selections:

            if ((LOWORD(wParam) >= ID_FIRST_LOG) && (LOWORD(wParam) <= ID_FIRST_LOG + dwNumLogs))
            {
                if (LogNames[LOWORD(wParam) - ID_FIRST_LOG])
                {
                    if (QueryEventMessages(lpComputerName, LogNames[LOWORD(wParam) - ID_FIRST_LOG]))
                    {
                        CheckMenuRadioItem(GetMenu(hWnd), ID_FIRST_LOG, ID_FIRST_LOG + dwNumLogs, LOWORD(wParam), MF_BYCOMMAND);
                    }
                }
            }
            else

            switch (LOWORD(wParam))
            {
                case IDM_SAVE_PROTOCOL:
                    SaveProtocol();
                    break;

                case IDM_CLEAR_EVENTS:
                    if (ClearEvents())
                    {
                        Refresh();
                    }
                    break;

                case IDM_REFRESH:
                    Refresh();
                    break;

                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;

                case IDM_HELP:
                    MessageBoxW(hwndMainWindow,
                               L"Help not implemented yet!",
                               L"Event Log",
                               MB_OK | MB_ICONINFORMATION);
                               break;

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;

                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;

        case WM_SIZE:
            // Gets the window rectangle
            GetClientRect(hWnd, &rect);

            // Relocate the listview
            MoveWindow(hwndListView,
                       0,
                       0,
                       rect.right,
                       rect.bottom - 20,
                       1);

            // Resize the statusbar;
            SendMessage(hwndStatus, message, wParam, lParam);
            break;

        case WM_DESTROY:
            FreeRecords();
            FreeLogList();
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}


// Message handler for about box.
INT_PTR CALLBACK
About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
            {
                return (INT_PTR)TRUE;
            }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }

    return (INT_PTR)FALSE;
}

VOID
DisplayEvent(HWND hDlg)
{
    WCHAR szEventType[MAX_PATH];
    WCHAR szTime[MAX_PATH];
    WCHAR szDate[MAX_PATH];
    WCHAR szUser[MAX_PATH];
    WCHAR szComputer[MAX_PATH];
    WCHAR szSource[MAX_PATH];
    WCHAR szCategory[MAX_PATH];
    WCHAR szEventID[MAX_PATH];
    WCHAR szEventText[EVENT_MESSAGE_EVENTTEXT_BUFFER];
    WCHAR szEventData[MAX_PATH];
    BOOL bEventData = FALSE;
    LVITEMW li;
    EVENTLOGRECORD* pevlr;
    int iIndex;

    // Get index of selected item
    iIndex = (int)SendMessage (hwndListView, LVM_GETNEXTITEM, -1, LVNI_SELECTED | LVNI_FOCUSED);

    li.mask = LVIF_PARAM;
    li.iItem = iIndex;
    li.iSubItem = 0;

    (void)ListView_GetItem(hwndListView, &li);

    pevlr = (EVENTLOGRECORD*)li.lParam;

    if (iIndex != -1)
    {
        ListView_GetItemText(hwndListView, iIndex, 0, szEventType, sizeof(szEventType) / sizeof(WCHAR));
        ListView_GetItemText(hwndListView, iIndex, 1, szDate, sizeof(szDate) / sizeof(WCHAR));
        ListView_GetItemText(hwndListView, iIndex, 2, szTime, sizeof(szTime) / sizeof(WCHAR));
        ListView_GetItemText(hwndListView, iIndex, 3, szSource, sizeof(szSource) / sizeof(WCHAR));
        ListView_GetItemText(hwndListView, iIndex, 4, szCategory, sizeof(szCategory) / sizeof(WCHAR));
        ListView_GetItemText(hwndListView, iIndex, 5, szEventID, sizeof(szEventID) / sizeof(WCHAR));
        ListView_GetItemText(hwndListView, iIndex, 6, szUser, sizeof(szUser) / sizeof(WCHAR));
        ListView_GetItemText(hwndListView, iIndex, 7, szComputer, sizeof(szComputer) / sizeof(WCHAR));

        bEventData = !(pevlr->DataLength == 0);

        if (pevlr->DataLength > 0)
        {
            MultiByteToWideChar(CP_ACP,
                                0,
                                (LPCSTR)((LPBYTE)pevlr + pevlr->DataOffset),
                                pevlr->DataLength,
                                szEventData,
                                MAX_PATH);
        }

        GetEventMessage(lpSourceLogName, szSource, pevlr, szEventText);

        EnableWindow(GetDlgItem(hDlg, IDC_BYTESRADIO), bEventData);
        EnableWindow(GetDlgItem(hDlg, IDC_WORDRADIO), bEventData);

        SetDlgItemTextW(hDlg, IDC_EVENTDATESTATIC, szDate);
        SetDlgItemTextW(hDlg, IDC_EVENTTIMESTATIC, szTime);

        SetDlgItemTextW(hDlg, IDC_EVENTUSERSTATIC, szUser);
        SetDlgItemTextW(hDlg, IDC_EVENTSOURCESTATIC, szSource);
        SetDlgItemTextW(hDlg, IDC_EVENTCOMPUTERSTATIC, szComputer);
        SetDlgItemTextW(hDlg, IDC_EVENTCATEGORYSTATIC, szCategory);
        SetDlgItemTextW(hDlg, IDC_EVENTIDSTATIC, szEventID);
        SetDlgItemTextW(hDlg, IDC_EVENTTYPESTATIC, szEventType);
        SetDlgItemTextW(hDlg, IDC_EVENTTEXTEDIT, szEventText);
        SetDlgItemTextW(hDlg, IDC_EVENTDATAEDIT, szEventData);
    }
    else
    {
        MessageBoxW(NULL,
                   L"No Items in ListView",
                   L"Error",
                   MB_OK | MB_ICONINFORMATION);
    }
}

VOID
CopyEventEntry(HWND hWnd)
{
    WCHAR output[4130], tmpHeader[512];
    WCHAR szEventType[MAX_PATH];
    WCHAR szSource[MAX_PATH];
    WCHAR szCategory[MAX_PATH];
    WCHAR szEventID[MAX_PATH];
    WCHAR szDate[MAX_PATH];
    WCHAR szTime[MAX_PATH];
    WCHAR szUser[MAX_PATH];
    WCHAR szComputer[MAX_PATH];
    WCHAR evtDesc[ENTRY_SIZE];
    HGLOBAL hMem;

    if (!OpenClipboard(hWnd))
        return;

    /* First, empty the clipboard before we begin to use it */
    EmptyClipboard();

    /* Get the formatted text needed to place the content into */
    LoadStringW(hInst, IDS_COPY, tmpHeader, sizeof(tmpHeader) / sizeof(WCHAR));

    /* Grabs all the information and get it ready for the clipboard */
    GetDlgItemText(hWnd, IDC_EVENTTYPESTATIC, szEventType, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTSOURCESTATIC, szSource, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTCATEGORYSTATIC, szCategory, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTIDSTATIC, szEventID, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTDATESTATIC, szDate, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTTIMESTATIC, szTime, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTUSERSTATIC, szUser, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTCOMPUTERSTATIC, szComputer, MAX_PATH);
    GetDlgItemText(hWnd, IDC_EVENTTEXTEDIT, evtDesc, ENTRY_SIZE);

    /* Consolidate the information into on big piece */
    wsprintfW(output, tmpHeader, szEventType, szSource, szCategory, szEventID, szDate, szTime, szUser, szComputer, evtDesc);

    /* Sort out the memory needed to write to the clipboard */
    hMem = GlobalAlloc(GMEM_MOVEABLE, ENTRY_SIZE);
    memcpy(GlobalLock(hMem), output, ENTRY_SIZE);
    GlobalUnlock(hMem);

    /* Write the final content to the clipboard */
    SetClipboardData(CF_UNICODETEXT, hMem);

    /* Close the clipboard once we're done with it */
    CloseClipboard();
}

static
INT_PTR CALLBACK
StatusMessageWindowProc(IN HWND hwndDlg,
                        IN UINT uMsg,
                        IN WPARAM wParam,
                        IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            return TRUE;
        }
    }
    return FALSE;
}

static
VOID
InitDetailsDlg(HWND hDlg)
{
    HANDLE nextIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_NEXT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HANDLE prevIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_PREV), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HANDLE copyIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_COPY), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    SendMessage(GetDlgItem(hDlg, IDC_NEXT), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)nextIcon);
    SendMessage(GetDlgItem(hDlg, IDC_PREVIOUS), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)prevIcon);
    SendMessage(GetDlgItem(hDlg, IDC_COPY), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)copyIcon);
}

// Message handler for event details box.
INT_PTR CALLBACK
EventDetails(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            InitDetailsDlg(hDlg);

            // Show event info on dialog box
            DisplayEvent(hDlg);
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;

                case IDC_PREVIOUS:
                    SendMessage(hwndListView, WM_KEYDOWN, VK_UP, 0);

                    // Show event info on dialog box
                    DisplayEvent(hDlg);
                    return (INT_PTR)TRUE;

                case IDC_NEXT:
                    SendMessage(hwndListView, WM_KEYDOWN, VK_DOWN, 0);

                    // Show event info on dialog box
                    DisplayEvent(hDlg);
                    return (INT_PTR)TRUE;

                case IDC_COPY:
                    CopyEventEntry(hDlg);
                    return (INT_PTR)TRUE;

                case IDC_BYTESRADIO:
                    return (INT_PTR)TRUE;

                case IDC_WORDRADIO:
                    return (INT_PTR)TRUE;

                case IDHELP:
                    MessageBoxW(hDlg,
                               L"Help not implemented yet!",
                               L"Event Log",
                               MB_OK | MB_ICONINFORMATION);
                    return (INT_PTR)TRUE;

                default:
                    break;
            }
            break;
    }

    return (INT_PTR)FALSE;
}
