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
#include <richedit.h>
#include <commdlg.h>
#include <strsafe.h>

/* Missing RichEdit flags in our richedit.h */
#define AURL_ENABLEURL          1
#define AURL_ENABLEEMAILADDR    2
#define AURL_ENABLETELNO        4
#define AURL_ENABLEEAURLS       8
#define AURL_ENABLEDRIVELETTERS 16

#include "resource.h"

typedef struct _DETAILDATA
{
    BOOL bDisplayWords;
    HFONT hMonospaceFont;
} DETAILDATA, *PDETAILDATA;

static const WCHAR szWindowClass[]      = L"EVENTVWR"; /* the main window class name */
static const WCHAR EVENTLOG_BASE_KEY[]  = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\";

/* MessageFile message buffer size */
#define EVENT_MESSAGE_EVENTTEXT_BUFFER  1024*10
#define EVENT_MESSAGE_FILE_BUFFER       1024*10
#define EVENT_DLL_SEPARATOR             L";"
#define EVENT_CATEGORY_MESSAGE_FILE     L"CategoryMessageFile"
#define EVENT_MESSAGE_FILE              L"EventMessageFile"
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

LPWSTR lpComputerName  = NULL;
LPWSTR lpSourceLogName = NULL;

DWORD dwNumLogs  = 0;
LPWSTR* LogNames = NULL;

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
    HMODULE hRichEdit;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    /* Whenever any of the common controls are used in your app,
     * you must call InitCommonControlsEx() to register the classes
     * for those controls. */
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccx);

    /* Initialize global strings */
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadStringW(hInstance, IDS_APP_TITLE_EX, szTitleTemplate, ARRAYSIZE(szTitleTemplate));
    LoadStringW(hInstance, IDS_STATUS_MSG, szStatusBarTemplate, ARRAYSIZE(szStatusBarTemplate));
    MyRegisterClass(hInstance);

    /* Perform application initialization */
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    /* Load the RichEdit DLL to add support for RichEdit controls */
    hRichEdit = LoadLibraryW(L"riched20.dll");
    if (!hRichEdit)
    {
        return FALSE;
    }

    hAccelTable = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDA_EVENTVWR));

    /* Main message loop */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    FreeLibrary(hRichEdit);

    return (int)msg.wParam;
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
                       OUT PWCHAR ExpandedName) // TODO: Add IN DWORD BufLen
{
    DWORD dwSize;
    WCHAR szModuleName[MAX_PATH];
    WCHAR szKeyName[MAX_PATH];
    HKEY hAppKey = NULL;
    HKEY hSourceKey = NULL;
    BOOL bReturn = FALSE;

    StringCbCopyW(szKeyName, sizeof(szKeyName), EVENTLOG_BASE_KEY);
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
            dwSize = sizeof(szModuleName);
            if (RegQueryValueExW(hSourceKey,
                                 EntryName,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szModuleName,
                                 &dwSize) == ERROR_SUCCESS)
            {
                /* Returns a string containing the requested substituted environment variable */
                ExpandEnvironmentStringsW(szModuleName, ExpandedName, MAX_PATH);

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
                 OUT PWCHAR CategoryName) // TODO: Add IN DWORD BufLen
{
    BOOL Success = FALSE;
    HMODULE hLibrary = NULL;
    WCHAR szMessageDLL[MAX_PATH];
    LPWSTR lpMsgBuf = NULL;

    if (!GetEventMessageFileDLL(KeyName, SourceName, EVENT_CATEGORY_MESSAGE_FILE, szMessageDLL))
        goto Quit;

    hLibrary = LoadLibraryExW(szMessageDLL, NULL,
                              LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
    if (hLibrary == NULL)
        goto Quit;

    /* Retrieve the message string without appending extra newlines */
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_MAX_WIDTH_MASK,
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

        /* The ID was found and the message was formatted */
        Success = TRUE;
    }

    /* Free the buffer allocated by FormatMessage */
    if (lpMsgBuf)
        LocalFree(lpMsgBuf);

    FreeLibrary(hLibrary);

Quit:
    if (!Success)
    {
        if (pevlr->EventCategory != 0)
            StringCchPrintfW(CategoryName, MAX_PATH, L"(%lu)", pevlr->EventCategory);
        else
            LoadStringW(hInst, IDS_NONE, CategoryName, MAX_PATH);
    }

    return Success;
}


BOOL
GetEventMessage(IN LPCWSTR KeyName,
                IN LPCWSTR SourceName,
                IN EVENTLOGRECORD *pevlr,
                OUT PWCHAR EventText) // TODO: Add IN DWORD BufLen
{
    BOOL Success = FALSE;
    DWORD i;
    HMODULE hLibrary = NULL;
    WCHAR SourceModuleName[1000];
    WCHAR ParameterModuleName[1000];
    BOOL IsParamModNameCached = FALSE;
    LPWSTR lpMsgBuf = NULL;
    LPWSTR szDll;
    LPWSTR szStringArray, szMessage;
    LPWSTR *szArguments;

    /* Get the event string array */
    szStringArray = (LPWSTR)((LPBYTE)pevlr + pevlr->StringOffset);

    /* NOTE: GetEventMessageFileDLL can return a comma-separated list of DLLs */
    if (!GetEventMessageFileDLL(KeyName, SourceName, EVENT_MESSAGE_FILE, SourceModuleName))
        goto Quit;

    /* Allocate space for parameters */
    szArguments = HeapAlloc(GetProcessHeap(), 0, pevlr->NumStrings * sizeof(LPVOID));
    if (!szArguments)
        goto Quit;

    szMessage = szStringArray;
    for (i = 0; i < pevlr->NumStrings; i++)
    {
        if (wcsstr(szMessage, L"%%"))
        {
            if (!IsParamModNameCached)
            {
                /* Now that the parameter file list is loaded, no need to reload it at the next run! */
                IsParamModNameCached = GetEventMessageFileDLL(KeyName, SourceName, EVENT_PARAMETER_MESSAGE_FILE, ParameterModuleName);
                // FIXME: If the string loading failed the first time, no need to retry it just after???
            }

            if (IsParamModNameCached)
            {
                /* Not yet support for reading messages from parameter message DLL */
            }
        }

        szArguments[i] = szMessage;
        szMessage += wcslen(szMessage) + 1;
    }

    /* Loop through the list of event message DLLs */
    szDll = wcstok(SourceModuleName, EVENT_DLL_SEPARATOR);
    while ((szDll != NULL) && !Success)
    {
        hLibrary = LoadLibraryExW(szDll, NULL,
                                  LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
        if (hLibrary == NULL)
        {
            /* The DLL could not be loaded try the next one (if any) */
            szDll = wcstok(NULL, EVENT_DLL_SEPARATOR);
            continue;
        }

        /* Retrieve the message string without appending extra newlines */
        if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                           hLibrary,
                           pevlr->EventID,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)&lpMsgBuf,
                           0,
                           (va_list*)szArguments) == 0)
        {
            /* We haven't found the string, get next DLL (if any) */
            szDll = wcstok(NULL, EVENT_DLL_SEPARATOR);
        }
        else if (lpMsgBuf)
        {
            /* Trim the string */
            TrimNulls((LPWSTR)lpMsgBuf);

            /* Copy the event text */
            StringCchCopyW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, lpMsgBuf);

            /* The ID was found and the message was formatted */
            Success = TRUE;
        }

        /* Free the buffer allocated by FormatMessage */
        if (lpMsgBuf)
            LocalFree(lpMsgBuf);

        FreeLibrary(hLibrary);
    }

    HeapFree(GetProcessHeap(), 0, szArguments);

Quit:
    if (!Success)
    {
        /* Get a read-only pointer to the "event-not-found" string */
        lpMsgBuf = HeapAlloc(GetProcessHeap(), 0, EVENT_MESSAGE_EVENTTEXT_BUFFER * sizeof(WCHAR));
        LoadStringW(hInst, IDS_EVENTSTRINGIDNOTFOUND, lpMsgBuf, EVENT_MESSAGE_EVENTTEXT_BUFFER);
        StringCchPrintfW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, lpMsgBuf, (pevlr->EventID & 0xFFFF), SourceName);

        /* Append the strings */
        szMessage = szStringArray;
        for (i = 0; i < pevlr->NumStrings; i++)
        {
            StringCchCatW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, szMessage);
            StringCchCatW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, L"\n");
            szMessage += wcslen(szMessage) + 1;
        }
    }

    return Success;
}


VOID
GetEventType(IN WORD dwEventType,
             OUT PWCHAR eventTypeText) // TODO: Add IN DWORD BufLen
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

    /* Point to the SID */
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
    HWND* phWnd = (HWND*)lpParameter;
    HWND hWnd;
    MSG Msg;

    hWnd = CreateDialogW(hInst,
                         MAKEINTRESOURCEW(IDD_PROGRESSBOX),
                         GetDesktopWindow(), // hwndMainWindow,
                         StatusMessageWindowProc);
    if (!hWnd)
        return 0;

    /*
     * FIXME: With this technique, there is one problem, namely that if
     * for some reason, the call to CreateDialogW takes longer than the
     * whole event-loading code to execute, then it may happen that the
     * event-loading code tries to close this dialog *BEFORE* we had the
     * time to return the window handle, hence the progress dialog would
     * be created *AFTER* the event-loading code has finished its job and
     * as a result, we would have a orphan window floating around.
     */

    *phWnd = hWnd;

    ShowWindow(hWnd, SW_SHOW);

    /* Message loop for the Status window */
    while (GetMessageW(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }

    DestroyWindow(hWnd);

    return 0;
}


static VOID FreeRecords(VOID)
{
    DWORD iIndex;

    if (!g_RecordPtrs)
        return;

    for (iIndex = 0; iIndex < g_TotalRecords; iIndex++)
        HeapFree(GetProcessHeap(), 0, g_RecordPtrs[iIndex]);
    HeapFree(GetProcessHeap(), 0, g_RecordPtrs);
    g_RecordPtrs = NULL;
}

BOOL
QueryEventMessages(LPWSTR lpMachineName,
                   LPWSTR lpLogName)
{
    HWND hwndDlg = NULL;
    HANDLE hEventLog;
    EVENTLOGRECORD *pevlr;
    DWORD dwRead, dwNeeded, dwThisRecord, dwTotalRecords = 0, dwCurrentRecord = 0, dwRecordsToRead = 0, dwFlags, dwMaxLength;
    SIZE_T cchRemaining;
    LPWSTR lpszSourceName;
    LPWSTR lpszComputerName;
    BOOL bResult = TRUE; /* Read succeeded */

    WCHAR szWindowTitle[MAX_PATH];
    WCHAR szStatusText[MAX_PATH];
    WCHAR szLocalDate[MAX_PATH];
    WCHAR szLocalTime[MAX_PATH];
    WCHAR szEventID[MAX_PATH];
    WCHAR szEventTypeText[MAX_LOADSTRING];
    WCHAR szCategoryID[MAX_PATH];
    WCHAR szUsername[MAX_PATH];
    WCHAR szCategory[MAX_PATH];
    PWCHAR lpTitleTemplateEnd;

    SYSTEMTIME time;
    LVITEMW lviEventItem;

    dwFlags = EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ;

    /* Open the event log */
    hEventLog = OpenEventLogW(lpMachineName, lpLogName);
    if (hEventLog == NULL)
    {
        ShowLastWin32Error();
        return FALSE;
    }

    /* Save the current computer name and log name globally */
    lpComputerName  = lpMachineName;
    lpSourceLogName = lpLogName;

    /* Disable list view redraw */
    SendMessageW(hwndListView, WM_SETREDRAW, FALSE, 0);

    /* Clear the list view */
    (void)ListView_DeleteAllItems(hwndListView);
    FreeRecords();

    GetOldestEventLogRecord(hEventLog, &dwThisRecord);

    /* Get the total number of event log records */
    GetNumberOfEventLogRecords(hEventLog, &dwTotalRecords);
    g_TotalRecords = dwTotalRecords;

    if (dwTotalRecords > 0)
    {
        EnableMenuItem(hMainMenu, IDM_CLEAR_EVENTS, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hMainMenu, IDM_SAVE_EVENTLOG, MF_BYCOMMAND | MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hMainMenu, IDM_CLEAR_EVENTS, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMainMenu, IDM_SAVE_EVENTLOG, MF_BYCOMMAND | MF_GRAYED);
    }

    g_RecordPtrs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwTotalRecords * sizeof(*g_RecordPtrs));

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
        pevlr = HeapAlloc(GetProcessHeap(), 0, sizeof(*pevlr));
        g_RecordPtrs[dwCurrentRecord] = pevlr;

        bResult = ReadEventLogW(hEventLog,  // Event log handle
                                dwFlags,    // Sequential read
                                0,          // Ignored for sequential read
                                pevlr,      // Pointer to buffer
                                sizeof(*pevlr),   // Size of buffer
                                &dwRead,    // Number of bytes read
                                &dwNeeded); // Bytes in the next record
        if ((!bResult) && (GetLastError () == ERROR_INSUFFICIENT_BUFFER))
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
            LoadStringW(hInst, IDS_NOT_AVAILABLE, szUsername, ARRAYSIZE(szUsername));
            LoadStringW(hInst, IDS_NONE, szCategory, ARRAYSIZE(szCategory));

            /* Get the event source name */
            lpszSourceName = (LPWSTR)((LPBYTE)pevlr + sizeof(EVENTLOGRECORD));

            /* Get the computer name */
            lpszComputerName = (LPWSTR)((LPBYTE)pevlr + sizeof(EVENTLOGRECORD) + (wcslen(lpszSourceName) + 1) * sizeof(WCHAR));

            /* Compute the event type */
            EventTimeToSystemTime(pevlr->TimeWritten, &time);

            /* Get the username that generated the event */
            GetEventUserName(pevlr, szUsername);

            GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, szLocalDate, ARRAYSIZE(szLocalDate));
            GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &time, NULL, szLocalTime, ARRAYSIZE(szLocalTime));

            GetEventType(pevlr->EventType, szEventTypeText);
            GetEventCategory(lpLogName, lpszSourceName, pevlr, szCategory);

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
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 3, lpszSourceName);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 4, szCategory);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 5, szEventID);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 6, szUsername);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 7, lpszComputerName);

            dwRead -= pevlr->Length;
            pevlr = (EVENTLOGRECORD *)((LPBYTE) pevlr + pevlr->Length);
        }

        dwRecordsToRead--;
        dwCurrentRecord++;
    }

    /* All events loaded */
    if (hwndDlg)
        EndDialog(hwndDlg, 0);

    StringCchPrintfExW(szWindowTitle,
                       ARRAYSIZE(szWindowTitle),
                       &lpTitleTemplateEnd,
                       &cchRemaining,
                       0,
                       szTitleTemplate, szTitle, lpLogName); /* i = number of characters written */
    dwMaxLength = (DWORD)cchRemaining;
    if (!lpMachineName)
        GetComputerNameW(lpTitleTemplateEnd, &dwMaxLength);
    else
        StringCchCopyW(lpTitleTemplateEnd, dwMaxLength, lpMachineName);

    StringCbPrintfW(szStatusText, sizeof(szStatusText), szStatusBarTemplate, lpLogName, dwTotalRecords);

    /* Update the status bar */
    SendMessageW(hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)szStatusText);

    /* Set the window title */
    SetWindowTextW(hwndMainWindow, szWindowTitle);

    /* Resume list view redraw */
    SendMessageW(hwndListView, WM_SETREDRAW, TRUE, 0);

    /* Close the event log */
    CloseEventLog(hEventLog);

    return TRUE;
}


VOID
SaveEventLog(VOID)
{
    HANDLE hEventLog;
    WCHAR szFileName[MAX_PATH];

    ZeroMemory(szFileName, sizeof(szFileName));

    sfn.lpstrFile = szFileName;
    sfn.nMaxFile  = ARRAYSIZE(szFileName);

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

    LoadStringW(hInst, IDS_CLEAREVENTS_MSG, szMessage, ARRAYSIZE(szMessage));

    sfn.lpstrFile = szFileName;
    sfn.nMaxFile  = ARRAYSIZE(szFileName);

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
    QueryEventMessages(lpComputerName, lpSourceLogName);
}


ATOM
MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(wcex);
    wcex.style = 0;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_EVENTVWR));
    wcex.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDM_EVENTVWR);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = (HICON)LoadImageW(hInstance,
                                     MAKEINTRESOURCEW(IDI_EVENTVWR),
                                     IMAGE_ICON,
                                     16,
                                     16,
                                     LR_SHARED);

    return RegisterClassExW(&wcex);
}


BOOL
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
        return FALSE;
    }

    StringCbCopyW(KeyPath, cbKeyPath, EVENTLOG_BASE_KEY);
    StringCbCatW(KeyPath, cbKeyPath, lpLogName);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, KeyPath);
        return FALSE;
    }

    cbData = sizeof(szModuleName);
    if (RegQueryValueExW(hKey, L"DisplayNameFile", NULL, NULL, (LPBYTE)szModuleName, &cbData) == ERROR_SUCCESS)
    {
        ExpandEnvironmentStringsW(szModuleName, lpModuleName, MAX_PATH);
    }

    RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, KeyPath);

    return TRUE;
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
BuildLogList(VOID)
{
    HKEY hKey;
    DWORD dwIndex;
    DWORD dwMaxKeyLength;
    LPWSTR LogName = NULL;
    WCHAR szModuleName[MAX_PATH];
    DWORD lpcName;
    DWORD dwMessageID;
    LPWSTR lpDisplayName;
    HMODULE hLibrary = NULL;

    /* Open the EventLog key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, EVENTLOG_BASE_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    /* Retrieve the number of event logs enumerated as registry keys */
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

    /* Take the NULL terminator into account */
    ++dwMaxKeyLength;

    /* Allocate the temporary buffer */
    LogName = HeapAlloc(GetProcessHeap(), 0, dwMaxKeyLength * sizeof(WCHAR));
    if (!LogName)
    {
        RegCloseKey(hKey);
        return;
    }

    /* Allocate the list of event logs (add 1 to dwNumLogs in order to have one guard entry) */
    LogNames = HeapAlloc(GetProcessHeap(), 0, (dwNumLogs + 1) * sizeof(LPWSTR));
    if (!LogNames)
    {
        HeapFree(GetProcessHeap(), 0, LogName);
        RegCloseKey(hKey);
        return;
    }

    /* Enumerate and retrieve each event log name */
    for (dwIndex = 0; dwIndex < dwNumLogs; dwIndex++)
    {
        lpcName = dwMaxKeyLength;
        if (RegEnumKeyExW(hKey, dwIndex, LogName, &lpcName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        {
            LogNames[dwIndex] = NULL;
            continue;
        }

        /* Take the NULL terminator into account */
        ++lpcName;

        /* Allocate the event log name string and copy it from the temporary buffer */
        LogNames[dwIndex] = HeapAlloc(GetProcessHeap(), 0, lpcName * sizeof(WCHAR));
        if (LogNames[dwIndex] == NULL)
            continue;

        StringCchCopyW(LogNames[dwIndex], lpcName, LogName);

        /* Get the display name for the event log */
        lpDisplayName = NULL;

        ZeroMemory(szModuleName, sizeof(szModuleName));
        if (GetDisplayNameFile(LogNames[dwIndex], szModuleName))
        {
            dwMessageID = GetDisplayNameID(LogNames[dwIndex]);

            hLibrary = LoadLibraryExW(szModuleName, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
            if (hLibrary != NULL)
            {
                /* Retrieve the message string without appending extra newlines */
                FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                               FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                               hLibrary,
                               dwMessageID,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPWSTR)&lpDisplayName,
                               0,
                               NULL);
                FreeLibrary(hLibrary);
            }
        }

        if (lpDisplayName)
        {
            InsertMenuW(hMainMenu, IDM_SAVE_EVENTLOG, MF_BYCOMMAND | MF_STRING, ID_FIRST_LOG + dwIndex, lpDisplayName);
        }
        else
        {
            InsertMenuW(hMainMenu, IDM_SAVE_EVENTLOG, MF_BYCOMMAND | MF_STRING, ID_FIRST_LOG + dwIndex, LogNames[dwIndex]);
        }

        /* Free the buffer allocated by FormatMessage */
        if (lpDisplayName)
            LocalFree(lpDisplayName);
    }

    InsertMenuW(hMainMenu, IDM_SAVE_EVENTLOG, MF_BYCOMMAND | MF_SEPARATOR, ID_FIRST_LOG + dwIndex + 1, NULL);

    HeapFree(GetProcessHeap(), 0, LogName);
    RegCloseKey(hKey);

    return;
}


VOID
FreeLogList(VOID)
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
    LogNames  = NULL;
    dwNumLogs = 0;

    lpComputerName  = NULL;
    lpSourceLogName = NULL;

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
                                 STATUSCLASSNAMEW,                   // status bar
                                 L"",                                // no text
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

    /* After the ListView is created, we can add extended list view styles */
    (void)ListView_SetExtendedListViewStyle (hwndListView, LVS_EX_FULLROWSELECT);

    /* Create the ImageList */
    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              ILC_COLOR32 | ILC_MASK, // ILC_COLOR24
                              1, 1);

    /* Add event type icons to ImageList */
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_INFORMATIONICON)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WARNINGICON)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ERRORICON)));

    /* Assign ImageList to List View */
    (void)ListView_SetImageList(hwndListView, hSmall, LVSIL_SMALL);

    /* Now set up the listview with its columns */
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = 90;
    LoadStringW(hInstance,
                IDS_COLUMNTYPE,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 0, &lvc);

    lvc.cx = 70;
    LoadStringW(hInstance,
                IDS_COLUMNDATE,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 1, &lvc);

    lvc.cx = 70;
    LoadStringW(hInstance,
                IDS_COLUMNTIME,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 2, &lvc);

    lvc.cx = 150;
    LoadStringW(hInstance,
                IDS_COLUMNSOURCE,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 3, &lvc);

    lvc.cx = 100;
    LoadStringW(hInstance,
                IDS_COLUMNCATEGORY,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 4, &lvc);

    lvc.cx = 60;
    LoadStringW(hInstance,
                IDS_COLUMNEVENT,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 5, &lvc);

    lvc.cx = 120;
    LoadStringW(hInstance,
                IDS_COLUMNUSER,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 6, &lvc);

    lvc.cx = 100;
    LoadStringW(hInstance,
                IDS_COLUMNCOMPUTER,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    (void)ListView_InsertColumn(hwndListView, 7, &lvc);

    /* Initialize the save Dialog */
    ZeroMemory(&sfn, sizeof(sfn));
    ZeroMemory(szSaveFilter, sizeof(szSaveFilter));

    LoadStringW(hInst, IDS_SAVE_FILTER, szSaveFilter, ARRAYSIZE(szSaveFilter));

    sfn.lStructSize     = sizeof(sfn);
    sfn.hwndOwner       = hwndMainWindow;
    sfn.hInstance       = hInstance;
    sfn.lpstrFilter     = szSaveFilter;
    sfn.lpstrInitialDir = NULL;
    sfn.Flags           = OFN_HIDEREADONLY | OFN_SHAREAWARE;
    sfn.lpstrDefExt     = NULL;

    ShowWindow(hwndMainWindow, nCmdShow);
    UpdateWindow(hwndMainWindow);

    /* Retrieve the available event logs on this computer */
    // TODO: Implement connection to remote computer (lpComputerName)
    // At the moment we only support the user local computer.
    lpComputerName = NULL;
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

        case WM_DESTROY:
        {
            FreeRecords();
            FreeLogList();
            PostQuitMessage(0);
            break;
        }

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_DBLCLK:
                    hdr = (NMHDR FAR*)lParam;
                    if (hdr->hwndFrom == hwndListView)
                    {
                        LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;

                        if (lpnmitem->iItem != -1)
                        {
                            DialogBoxW(hInst,
                                       MAKEINTRESOURCEW(IDD_EVENTPROPERTIES),
                                       hWnd,
                                       EventDetails);
                        }
                    }
                    break;
            }
            break;

        case WM_COMMAND:
        {
            /* Parse the menu selections */
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
                case IDM_SAVE_EVENTLOG:
                    SaveEventLog();
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
                    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_ABOUTBOX), hWnd, About);
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
                    return DefWindowProcW(hWnd, message, wParam, lParam);
            }
            break;
        }

        case WM_SIZE:
        {
            GetClientRect(hWnd, &rect);

            MoveWindow(hwndListView,
                       0,
                       0,
                       rect.right,
                       rect.bottom - 20,
                       1);

            SendMessageW(hwndStatus, message, wParam, lParam);
            break;
        }

        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}


/* Message handler for About box */
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
    BOOL bEventData = FALSE;
    LVITEMW li;
    EVENTLOGRECORD* pevlr;
    int iIndex;

    /* Get index of selected item */
    iIndex = (int)SendMessageW(hwndListView, LVM_GETNEXTITEM, -1, LVNI_SELECTED | LVNI_FOCUSED);
    if (iIndex == -1)
    {
        MessageBoxW(hDlg,
                    L"No Items in ListView",
                    L"Error",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    li.mask = LVIF_PARAM;
    li.iItem = iIndex;
    li.iSubItem = 0;

    (void)ListView_GetItem(hwndListView, &li);

    pevlr = (EVENTLOGRECORD*)li.lParam;

    ListView_GetItemText(hwndListView, iIndex, 0, szEventType, ARRAYSIZE(szEventType));
    ListView_GetItemText(hwndListView, iIndex, 1, szDate, ARRAYSIZE(szDate));
    ListView_GetItemText(hwndListView, iIndex, 2, szTime, ARRAYSIZE(szTime));
    ListView_GetItemText(hwndListView, iIndex, 3, szSource, ARRAYSIZE(szSource));
    ListView_GetItemText(hwndListView, iIndex, 4, szCategory, ARRAYSIZE(szCategory));
    ListView_GetItemText(hwndListView, iIndex, 5, szEventID, ARRAYSIZE(szEventID));
    ListView_GetItemText(hwndListView, iIndex, 6, szUser, ARRAYSIZE(szUser));
    ListView_GetItemText(hwndListView, iIndex, 7, szComputer, ARRAYSIZE(szComputer));

    SetDlgItemTextW(hDlg, IDC_EVENTDATESTATIC, szDate);
    SetDlgItemTextW(hDlg, IDC_EVENTTIMESTATIC, szTime);
    SetDlgItemTextW(hDlg, IDC_EVENTUSERSTATIC, szUser);
    SetDlgItemTextW(hDlg, IDC_EVENTSOURCESTATIC, szSource);
    SetDlgItemTextW(hDlg, IDC_EVENTCOMPUTERSTATIC, szComputer);
    SetDlgItemTextW(hDlg, IDC_EVENTCATEGORYSTATIC, szCategory);
    SetDlgItemTextW(hDlg, IDC_EVENTIDSTATIC, szEventID);
    SetDlgItemTextW(hDlg, IDC_EVENTTYPESTATIC, szEventType);

    bEventData = (pevlr->DataLength > 0);
    EnableWindow(GetDlgItem(hDlg, IDC_BYTESRADIO), bEventData);
    EnableWindow(GetDlgItem(hDlg, IDC_WORDRADIO), bEventData);

    GetEventMessage(lpSourceLogName, szSource, pevlr, szEventText);
    SetDlgItemTextW(hDlg, IDC_EVENTTEXTEDIT, szEventText);
}

UINT
PrintByteDataLine(PWCHAR pBuffer, UINT uOffset, PBYTE pd, UINT uLength)
{
    PWCHAR p = pBuffer;
    UINT n, i, r = 0;

    if (uOffset != 0)
    {
        n = swprintf(p, L"\r\n");
        p = p + n;
        r += n;
    }

    n = swprintf(p, L"%04lx:", uOffset);
    p = p + n;
    r += n;

    for (i = 0; i < uLength; i++)
    {
        n = swprintf(p, L" %02x", pd[i]);
        p = p + n;
        r += n;
    }

    for (i = 0; i < 9 - uLength; i++)
    {
        n = swprintf(p, L"   ");
        p = p + n;
        r += n;
    }

    for (i = 0; i < uLength; i++)
    {
        n = swprintf(p, L"%c", iswprint(pd[i]) ? pd[i] : L'.');
        p = p + n;
        r += n;
    }

    return r;
}

UINT
PrintWordDataLine(PWCHAR pBuffer, UINT uOffset, PULONG pData, UINT uLength)
{
    PWCHAR p = pBuffer;
    UINT n, i, r = 0;

    if (uOffset != 0)
    {
        n = swprintf(p, L"\r\n");
        p = p + n;
        r += n;
    }

    n = swprintf(p, L"%04lx:", uOffset);
    p = p + n;
    r += n;

    for (i = 0; i < uLength / sizeof(ULONG); i++)
    {
        n = swprintf(p, L" %08lx", pData[i]);
        p = p + n;
        r += n;
    }

    return r;
}


VOID
DisplayEventData(HWND hDlg, BOOL bDisplayWords)
{
    LVITEMW li;
    EVENTLOGRECORD* pevlr;
    int iIndex;

    LPBYTE pData;
    UINT i, uOffset;
    UINT uBufferSize, uLineLength;
    PWCHAR pTextBuffer, pLine;

    /* Get index of selected item */
    iIndex = (int)SendMessageW(hwndListView, LVM_GETNEXTITEM, -1, LVNI_SELECTED | LVNI_FOCUSED);
    if (iIndex == -1)
    {
        MessageBoxW(hDlg,
                    L"No Items in ListView",
                    L"Error",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    li.mask = LVIF_PARAM;
    li.iItem = iIndex;
    li.iSubItem = 0;

    (void)ListView_GetItem(hwndListView, &li);

    pevlr = (EVENTLOGRECORD*)li.lParam;
    if (pevlr->DataLength == 0)
    {
        SetDlgItemTextW(hDlg, IDC_EVENTDATAEDIT, L"");
        return;
    }

    if (bDisplayWords)
        uBufferSize = ((pevlr->DataLength / 8) + 1) * 26 * sizeof(WCHAR);
    else
        uBufferSize = ((pevlr->DataLength / 8) + 1) * 43 * sizeof(WCHAR);

    pTextBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, uBufferSize);
    if (pTextBuffer)
    {
        pLine = pTextBuffer;
        uOffset = 0;

        for (i = 0; i < pevlr->DataLength / 8; i++)
        {
            pData = (LPBYTE)((LPBYTE)pevlr + pevlr->DataOffset + uOffset);

            if (bDisplayWords)
                uLineLength = PrintWordDataLine(pLine, uOffset, (PULONG)pData, 8);
            else
                uLineLength = PrintByteDataLine(pLine, uOffset, pData, 8);
            pLine = pLine + uLineLength;

            uOffset += 8;
        }

        if (pevlr->DataLength % 8 != 0)
        {
            pData = (LPBYTE)((LPBYTE)pevlr + pevlr->DataOffset + uOffset);

            if (bDisplayWords)
                PrintWordDataLine(pLine, uOffset, (PULONG)pData, pevlr->DataLength % 8);
            else
                PrintByteDataLine(pLine, uOffset, pData, pevlr->DataLength % 8);
        }

        SetDlgItemTextW(hDlg, IDC_EVENTDATAEDIT, pTextBuffer);

        HeapFree(GetProcessHeap(), 0, pTextBuffer);
    }
}

HFONT
CreateMonospaceFont(VOID)
{
    LOGFONTW tmpFont = {0};
    HFONT hFont;
    HDC hDc;

    hDc = GetDC(NULL);

    tmpFont.lfHeight = -MulDiv(8, GetDeviceCaps(hDc, LOGPIXELSY), 72);
    tmpFont.lfWeight = FW_NORMAL;
    wcscpy(tmpFont.lfFaceName, L"Courier New");

    hFont = CreateFontIndirectW(&tmpFont);

    ReleaseDC(NULL, hDc);

    return hFont;
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
    LoadStringW(hInst, IDS_COPY, tmpHeader, ARRAYSIZE(tmpHeader));

    /* Grab all the information and get it ready for the clipboard */
    GetDlgItemTextW(hWnd, IDC_EVENTTYPESTATIC, szEventType, ARRAYSIZE(szEventType));
    GetDlgItemTextW(hWnd, IDC_EVENTSOURCESTATIC, szSource, ARRAYSIZE(szSource));
    GetDlgItemTextW(hWnd, IDC_EVENTCATEGORYSTATIC, szCategory, ARRAYSIZE(szCategory));
    GetDlgItemTextW(hWnd, IDC_EVENTIDSTATIC, szEventID, ARRAYSIZE(szEventID));
    GetDlgItemTextW(hWnd, IDC_EVENTDATESTATIC, szDate, ARRAYSIZE(szDate));
    GetDlgItemTextW(hWnd, IDC_EVENTTIMESTATIC, szTime, ARRAYSIZE(szTime));
    GetDlgItemTextW(hWnd, IDC_EVENTUSERSTATIC, szUser, ARRAYSIZE(szUser));
    GetDlgItemTextW(hWnd, IDC_EVENTCOMPUTERSTATIC, szComputer, ARRAYSIZE(szComputer));
    GetDlgItemTextW(hWnd, IDC_EVENTTEXTEDIT, evtDesc, ARRAYSIZE(evtDesc));

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
    UNREFERENCED_PARAMETER(hwndDlg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}

static
VOID
InitDetailsDlg(HWND hDlg, PDETAILDATA pData)
{
    DWORD dwMask;

    HANDLE nextIcon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_NEXT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HANDLE prevIcon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_PREV), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HANDLE copyIcon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_COPY), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    SendDlgItemMessageW(hDlg, IDC_NEXT, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)nextIcon);
    SendDlgItemMessageW(hDlg, IDC_PREVIOUS, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)prevIcon);
    SendDlgItemMessageW(hDlg, IDC_COPY, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)copyIcon);

    /* Set the default read-only RichEdit color */
    SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_3DFACE));

    /* Enable RichEdit coloured and underlined links */
    dwMask = SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_GETEVENTMASK, 0, 0);
    SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_SETEVENTMASK, 0, dwMask | ENM_LINK | ENM_MOUSEEVENTS);

    /*
     * Activate automatic URL recognition by the RichEdit control. For more information, see:
     * https://blogs.msdn.microsoft.com/murrays/2009/08/31/automatic-richedit-hyperlinks/
     * https://blogs.msdn.microsoft.com/murrays/2009/09/24/richedit-friendly-name-hyperlinks/
     * https://msdn.microsoft.com/en-us/library/windows/desktop/bb787991(v=vs.85).aspx
     */
    SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_AUTOURLDETECT, AURL_ENABLEURL /* | AURL_ENABLEEAURLS */, 0);

    /* Note that the RichEdit control never gets themed under WinXP+. One would have to write code to simulate Edit-control theming */

    SendDlgItemMessageW(hDlg, pData->bDisplayWords ? IDC_WORDRADIO : IDC_BYTESRADIO, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessageW(hDlg, IDC_EVENTDATAEDIT, WM_SETFONT, (WPARAM)pData->hMonospaceFont, (LPARAM)TRUE);
}

/* Message handler for Event Details box */
INT_PTR CALLBACK
EventDetails(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PDETAILDATA pData;

    UNREFERENCED_PARAMETER(lParam);

    pData = (PDETAILDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message)
    {
        case WM_INITDIALOG:
            pData = (PDETAILDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DETAILDATA));
            if (pData)
            {
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pData);

                pData->bDisplayWords = FALSE;
                pData->hMonospaceFont = CreateMonospaceFont();

                InitDetailsDlg(hDlg, pData);

                /* Show event info on dialog box */
                DisplayEvent(hDlg);
                DisplayEventData(hDlg, pData->bDisplayWords);
            }
            return (INT_PTR)TRUE;

        case WM_DESTROY:
            if (pData->hMonospaceFont)
                DeleteObject(pData->hMonospaceFont);
            HeapFree(GetProcessHeap(), 0, pData);
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;

                case IDC_PREVIOUS:
                    SendMessageW(hwndListView, WM_KEYDOWN, VK_UP, 0);

                    /* Show event info on dialog box */
                    if (pData)
                    {
                        DisplayEvent(hDlg);
                        DisplayEventData(hDlg, pData->bDisplayWords);
                    }
                    return (INT_PTR)TRUE;

                case IDC_NEXT:
                    SendMessageW(hwndListView, WM_KEYDOWN, VK_DOWN, 0);

                    /* Show event info on dialog box */
                    if (pData)
                    {
                        DisplayEvent(hDlg);
                        DisplayEventData(hDlg, pData->bDisplayWords);
                    }
                    return (INT_PTR)TRUE;

                case IDC_COPY:
                    CopyEventEntry(hDlg);
                    return (INT_PTR)TRUE;

                case IDC_BYTESRADIO:
                    if (pData)
                    {
                        pData->bDisplayWords = FALSE;
                        DisplayEventData(hDlg, pData->bDisplayWords);
                    }
                    return (INT_PTR)TRUE;

                case IDC_WORDRADIO:
                    if (pData)
                    {
                        pData->bDisplayWords = TRUE;
                        DisplayEventData(hDlg, pData->bDisplayWords);
                    }
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

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case EN_LINK:
                    // TODO: Act on the activated RichEdit link!
                    break;
            }
            break;
    }

    return (INT_PTR)FALSE;
}
