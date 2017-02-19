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
 * PROJECT:         ReactOS Event Log Viewer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/mscutils/eventvwr/eventvwr.c
 * PURPOSE:         Event Log Viewer main file
 * PROGRAMMERS:     Marc Piulachs (marc.piulachs at codexchange [dot] net)
 *                  Eric Kohl
 *                  Hermes Belusca-Maito
 */

#include "eventvwr.h"
#include "evtdetctl.h"

#include <sddl.h> // For ConvertSidToStringSidW
#include <shellapi.h>
#include <shlwapi.h>

// #include "resource.h"

#define LVM_PROGRESS    (WM_APP + 1)    // Used by the subclassed ListView

static const LPCWSTR szWindowClass       = L"EVENTVWR"; /* The main window class name */
static const WCHAR   EVENTLOG_BASE_KEY[] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\";

/* The 3 system logs that should always exist in the user's system */
static const LPCWSTR SystemLogs[] =
{
    L"Application",
    L"Security",
    L"System"
};

/* MessageFile message buffer size */
#define EVENT_MESSAGE_EVENTTEXT_BUFFER  1024*10                             // NOTE: Used by evtdetctl.c
#define EVENT_MESSAGE_FILE_BUFFER       1024*10
#define EVENT_DLL_SEPARATOR             L";"
#define EVENT_CATEGORY_MESSAGE_FILE     L"CategoryMessageFile"
#define EVENT_MESSAGE_FILE              L"EventMessageFile"
#define EVENT_PARAMETER_MESSAGE_FILE    L"ParameterMessageFile"

#define MAX_LOADSTRING 255

#define SPLIT_WIDTH 4

/* Globals */
HINSTANCE hInst;                            /* Current instance */
WCHAR szTitle[MAX_LOADSTRING];              /* The title bar text */
WCHAR szTitleTemplate[MAX_LOADSTRING];      /* The logged-on title bar text */
WCHAR szStatusBarTemplate[MAX_LOADSTRING];  /* The status bar text */
WCHAR szLoadingWait[MAX_LOADSTRING];        /* The "Loading, please wait..." text */
WCHAR szEmptyList[MAX_LOADSTRING];          /* The "There are no items to show in this view" text */
WCHAR szSaveFilter[MAX_LOADSTRING];         /* Filter Mask for the save Dialog */

INT  nVSplitPos;                            /* Vertical splitter (1) position */
INT  nHSplitPos;                            /* Horizontal splitter (2) position */
BYTE bSplit = 0;                            /* Splitter state:
                                             * 0: No splitting;
                                             * 1: Vertical splitting;
                                             * 2: Horizontal splitting.
                                             */

HWND hwndMainWindow;                        /* Main window */
HWND hwndTreeView;                          /* TreeView control */
HWND hwndListView;                          /* ListView control */          // NOTE: Used by evtdetctl.c
HWND hwndEventDetails;                      /* Event details pane */
HWND hwndStatus;                            /* Status bar */
HWND hwndStatusProgress;                    /* Progress bar in the status bar */
HMENU hMainMenu;                            /* The application's main menu */

HTREEITEM htiSystemLogs = NULL, htiAppLogs = NULL, htiUserLogs = NULL;


/* Global event records cache for the current active event log filter */
DWORD g_TotalRecords = 0;
PEVENTLOGRECORD *g_RecordPtrs = NULL;

/* Lists of event logs and event log filters */
LIST_ENTRY EventLogList;
LIST_ENTRY EventLogFilterList;
PEVENTLOGFILTER ActiveFilter = NULL;
BOOL NewestEventsFirst = TRUE;

HANDLE hEnumEventsThread = NULL;
HANDLE hStopEnumEvent    = NULL;

/*
 * Setting EnumFilter to a valid pointer and raising the hStartEnumEvent event
 * triggers the event-enumerator thread to perform a new enumeration.
 */
PEVENTLOGFILTER EnumFilter = NULL;
HANDLE hStartStopEnumEvent = NULL;  // End-of-application event
HANDLE hStartEnumEvent     = NULL;  // Command event

/* Default Open/Save-As dialog box */
OPENFILENAMEW sfn;


/* Forward declarations of functions included in this code module */

static DWORD WINAPI
StartStopEnumEventsThread(IN LPVOID lpParameter);

VOID BuildLogListAndFilterList(IN LPCWSTR lpComputerName);
VOID FreeLogList(VOID);
VOID FreeLogFilterList(VOID);

ATOM MyRegisterClass(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR EventLogProperties(HINSTANCE, HWND, PEVENTLOGFILTER);
INT_PTR CALLBACK EventDetails(HWND, UINT, WPARAM, LPARAM);


int APIENTRY
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR    lpCmdLine,
         int       nCmdShow)
{
    HANDLE hThread;
    INITCOMMONCONTROLSEX iccx;
    HMODULE hRichEdit;
    HACCEL hAccelTable;
    MSG msg;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    /* Whenever any of the common controls are used in your app,
     * you must call InitCommonControlsEx() to register the classes
     * for those controls. */
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccx);

    /* Load the RichEdit DLL to add support for RichEdit controls */
    hRichEdit = LoadLibraryW(L"riched20.dll");
    if (!hRichEdit)
        return -1;

    msg.wParam = (WPARAM)-1;

    /* Initialize global strings */
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadStringW(hInstance, IDS_APP_TITLE_EX, szTitleTemplate, ARRAYSIZE(szTitleTemplate));
    LoadStringW(hInstance, IDS_STATUS_MSG, szStatusBarTemplate, ARRAYSIZE(szStatusBarTemplate));
    LoadStringW(hInstance, IDS_LOADING_WAIT, szLoadingWait, ARRAYSIZE(szLoadingWait));
    LoadStringW(hInstance, IDS_NO_ITEMS, szEmptyList, ARRAYSIZE(szEmptyList));

    if (!MyRegisterClass(hInstance))
        goto Quit;

    /* Perform application initialization */
    if (!InitInstance(hInstance, nCmdShow))
        goto Quit;

    hAccelTable = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDA_EVENTVWR));

    /* Create the Start/Stop enumerator thread */
    // Manual-reset event
    hStartStopEnumEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!hStartStopEnumEvent)
        goto Cleanup;

    // Auto-reset event
    hStartEnumEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hStartEnumEvent)
        goto Cleanup;

    hThread = CreateThread(NULL, 0,
                           StartStopEnumEventsThread,
                           NULL, 0, NULL);
    if (!hThread)
        goto Cleanup;

    /* Retrieve the available event logs on this computer and create filters for them */
    InitializeListHead(&EventLogList);
    InitializeListHead(&EventLogFilterList);
    // TODO: Implement connection to remote computer...
    // At the moment we only support the user local computer.
    BuildLogListAndFilterList(NULL);

    // TODO: If the user wants to open an external event log with the Event Log Viewer
    // (via the command line), it's here that the log should be opened.

    /* Main message loop */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(hwndMainWindow, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    SetEvent(hStartStopEnumEvent);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    /* Free the filters list and the event logs list */
    FreeLogFilterList();
    FreeLogList();

Cleanup:
    /* Handle cleanup */
    if (hStartEnumEvent)
        CloseHandle(hStartEnumEvent);
    if (hStartStopEnumEvent)
        CloseHandle(hStartStopEnumEvent);

Quit:
    FreeLibrary(hRichEdit);

    return (int)msg.wParam;
}


/* GENERIC HELPER FUNCTIONS ***************************************************/

VOID
ShowLastWin32Error(VOID)
{
    DWORD dwError;
    LPWSTR lpMessageBuffer;

    dwError = GetLastError();
    if (dwError == ERROR_SUCCESS)
        return;

    if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dwError,
                        LANG_USER_DEFAULT,
                        (LPWSTR)&lpMessageBuffer,
                        0, NULL))
    {
        return;
    }

    MessageBoxW(hwndMainWindow, lpMessageBuffer, szTitle, MB_OK | MB_ICONERROR);
    LocalFree(lpMessageBuffer);
}

VOID
EventTimeToSystemTime(IN DWORD EventTime,
                      OUT PSYSTEMTIME pSystemTime)
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

/*
 * This function takes in entry a path to a single DLL, in which
 * the message string of ID dwMessageId has to be searched.
 * The other parameters are similar to those of the FormatMessageW API.
 */
LPWSTR
GetMessageStringFromDll(
    IN LPCWSTR  lpMessageDll,
    IN DWORD    dwFlags, // If we always use the same flags, just remove this param...
    IN DWORD    dwMessageId,
    IN DWORD    nSize,
    IN va_list* Arguments OPTIONAL)
{
    HMODULE hLibrary;
    DWORD dwLength;
    LPWSTR lpMsgBuf = NULL;

    hLibrary = LoadLibraryExW(lpMessageDll, NULL,
                              /* LOAD_LIBRARY_AS_IMAGE_RESOURCE | */ LOAD_LIBRARY_AS_DATAFILE);
    if (hLibrary == NULL)
        return NULL;

    /* Sanitize dwFlags */
    dwFlags &= ~FORMAT_MESSAGE_FROM_STRING;
    dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;

    _SEH2_TRY
    {
        /*
         * Retrieve the message string without appending extra newlines.
         * Wrap in SEH to protect from invalid string parameters.
         */
        _SEH2_TRY
        {
            dwLength = FormatMessageW(dwFlags,
                                   /* FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                                      FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, */
                                      hLibrary,
                                      dwMessageId,
                                      LANG_USER_DEFAULT,
                                      (LPWSTR)&lpMsgBuf,
                                      nSize,
                                      Arguments);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            dwLength = 0;

            /*
             * An exception occurred while calling FormatMessage, this is usually
             * the sign that a parameter was invalid, either 'lpMsgBuf' was NULL
             * but we did not pass the flag FORMAT_MESSAGE_ALLOCATE_BUFFER, or the
             * array pointer 'Arguments' was NULL or did not contain enough elements,
             * and we did not pass the flag FORMAT_MESSAGE_IGNORE_INSERTS, and the
             * message string expected too many inserts.
             * In this last case only, we can call again FormatMessage but ignore
             * explicitly the inserts. The string that we will return to the user
             * will not be pre-formatted.
             */
            if (((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) || lpMsgBuf) &&
                !(dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS))
            {
                /* Remove any possible harmful flags and always ignore inserts */
                dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;
                dwFlags |=  FORMAT_MESSAGE_IGNORE_INSERTS;

                /* If this call also throws an exception, we are really dead */
                dwLength = FormatMessageW(dwFlags,
                                          hLibrary,
                                          dwMessageId,
                                          LANG_USER_DEFAULT,
                                          (LPWSTR)&lpMsgBuf,
                                          nSize,
                                          NULL /* Arguments */);
            }
        }
        _SEH2_END;
    }
    _SEH2_FINALLY
    {
        FreeLibrary(hLibrary);
    }
    _SEH2_END;

    if (dwLength == 0)
    {
        ASSERT(lpMsgBuf == NULL);
        lpMsgBuf = NULL;
    }
    else
    {
        ASSERT(lpMsgBuf);
    }

    return lpMsgBuf;
}

/*
 * This function takes in entry a comma-separated list of DLLs, in which
 * the message string of ID dwMessageId has to be searched.
 * The other parameters are similar to those of the FormatMessageW API.
 */
LPWSTR
GetMessageStringFromDllList(
    IN LPCWSTR  lpMessageDllList,
    IN DWORD    dwFlags, // If we always use the same flags, just remove this param...
    IN DWORD    dwMessageId,
    IN DWORD    nSize,
    IN va_list* Arguments OPTIONAL)
{
    BOOL Success = FALSE;
    SIZE_T cbLength;
    LPWSTR szMessageDllList;
    LPWSTR szDll;
    LPWSTR lpMsgBuf = NULL;

    /* Allocate a local buffer for the DLL list that can be tokenized */
    // TODO: Optimize that!! Maybe we can cleverly use lpMessageDllList in read/write mode
    // and cleverly temporarily replace the ';' by UNICODE_NULL, do our job, then reverse the change.
    cbLength = (wcslen(lpMessageDllList) + 1) * sizeof(WCHAR);
    szMessageDllList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbLength);
    if (!szMessageDllList)
        return NULL;
    RtlCopyMemory(szMessageDllList, lpMessageDllList, cbLength);

    /* Loop through the list of message DLLs */
    szDll = wcstok(szMessageDllList, EVENT_DLL_SEPARATOR);
    while ((szDll != NULL) && !Success)
    {
        // Uses LANG_USER_DEFAULT
        lpMsgBuf = GetMessageStringFromDll(szDll,
                                           dwFlags,
                                           dwMessageId,
                                           nSize,
                                           Arguments);
        if (lpMsgBuf)
        {
            /* The ID was found and the message was formatted */
            Success = TRUE;
            break;
        }

        /*
         * The DLL could not be loaded, or the message could not be found,
         * try the next DLL, if any.
         */
        szDll = wcstok(NULL, EVENT_DLL_SEPARATOR);
    }

    HeapFree(GetProcessHeap(), 0, szMessageDllList);

    return lpMsgBuf;
}


typedef struct
{
    LPWSTR pStartingAddress;    // Pointer to the beginning of a parameter string in pMessage
    LPWSTR pEndingAddress;      // Pointer to the end of a parameter string in pMessage
    DWORD  pParameterID;        // Parameter identifier found in pMessage
    LPWSTR pParameter;          // Actual parameter string
} param_strings_format_data;

DWORD
ApplyParameterStringsToMessage(
    IN LPCWSTR  lpMessageDllList,
    IN BOOL     bMessagePreFormatted,
    IN CONST LPCWSTR pMessage,
    OUT LPWSTR* pFinalMessage)
{
    /*
     * This code is heavily adapted from the MSDN example:
     * https://msdn.microsoft.com/en-us/library/windows/desktop/bb427356.aspx
     * with bugs removed.
     */

    DWORD Status = ERROR_SUCCESS;
    DWORD dwParamCount = 0; // Number of insertion strings found in pMessage
    size_t cchBuffer = 0;   // Size of the buffer in characters
    size_t cchParams = 0;   // Number of characters in all the parameter strings
    size_t cch = 0;
    DWORD i = 0;
    param_strings_format_data* pParamData = NULL; // Array of pointers holding information about each parameter string in pMessage
    LPWSTR pTempMessage = (LPWSTR)pMessage;
    LPWSTR pTempFinalMessage = NULL;

    *pFinalMessage = NULL;

    /* Determine the number of parameter insertion strings in pMessage */
    if (bMessagePreFormatted)
    {
        while ((pTempMessage = wcschr(pTempMessage, L'%')))
        {
            pTempMessage++;
            if (isdigit(*pTempMessage))
            {
                dwParamCount++;
                while (isdigit(*++pTempMessage)) ;
            }
        }
    }
    else
    {
        while ((pTempMessage = wcsstr(pTempMessage, L"%%")))
        {
            pTempMessage += 2;
            if (isdigit(*pTempMessage))
            {
                dwParamCount++;
                while (isdigit(*++pTempMessage)) ;
            }
        }
    }

    /* If there are no parameter insertion strings in pMessage, just return */
    if (dwParamCount == 0)
    {
        // *pFinalMessage = NULL;
        goto Cleanup;
    }

    /* Allocate the array of parameter string format data */
    pParamData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwParamCount * sizeof(param_strings_format_data));
    if (!pParamData)
    {
        Status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    /*
     * Retrieve each parameter in pMessage and the beginning and end of the
     * insertion string, as well as the message identifier of the parameter.
     */
    pTempMessage = (LPWSTR)pMessage;
    if (bMessagePreFormatted)
    {
        while ((pTempMessage = wcschr(pTempMessage, L'%')) && (i < dwParamCount))
        {
            pTempMessage++;
            if (isdigit(*pTempMessage))
            {
                pParamData[i].pStartingAddress = pTempMessage-1;
                pParamData[i].pParameterID = (DWORD)_wtol(pTempMessage);

                while (isdigit(*++pTempMessage)) ;

                pParamData[i].pEndingAddress = pTempMessage;
                i++;
            }
        }
    }
    else
    {
        while ((pTempMessage = wcsstr(pTempMessage, L"%%")) && (i < dwParamCount))
        {
            pTempMessage += 2;
            if (isdigit(*pTempMessage))
            {
                pParamData[i].pStartingAddress = pTempMessage-2;
                pParamData[i].pParameterID = (DWORD)_wtol(pTempMessage);

                while (isdigit(*++pTempMessage)) ;

                pParamData[i].pEndingAddress = pTempMessage;
                i++;
            }
        }
    }

    /* Retrieve each parameter string */
    for (i = 0; i < dwParamCount; i++)
    {
        // pParamData[i].pParameter = GetMessageString(pParamData[i].pParameterID, 0, NULL);
        pParamData[i].pParameter =
            GetMessageStringFromDllList(lpMessageDllList,
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                        pParamData[i].pParameterID,
                                        0, NULL);
        if (!pParamData[i].pParameter)
        {
            /* Skip the insertion string */
            continue;
        }

        cchParams += wcslen(pParamData[i].pParameter);
    }

    /*
     * Allocate the final message buffer, the size of which is based on the
     * length of the original message and the length of each parameter string.
     */
    cchBuffer = wcslen(pMessage) + cchParams + 1;
    *pFinalMessage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cchBuffer * sizeof(WCHAR));
    if (!*pFinalMessage)
    {
        Status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    pTempFinalMessage = *pFinalMessage;

    /* Build the final message string */
    pTempMessage = (LPWSTR)pMessage;
    for (i = 0; i < dwParamCount; i++)
    {
        /* Append the segment from pMessage */
        cch = pParamData[i].pStartingAddress - pTempMessage;
        StringCchCopyNW(pTempFinalMessage, cchBuffer, pTempMessage, cch);
        pTempMessage = pParamData[i].pEndingAddress;
        cchBuffer -= cch;
        pTempFinalMessage += cch;

        /* Append the parameter string */
        if (pParamData[i].pParameter)
        {
            StringCchCopyW(pTempFinalMessage, cchBuffer, pParamData[i].pParameter);
            cch = wcslen(pParamData[i].pParameter); // pTempFinalMessage
        }
        else
        {
            /*
             * We failed to retrieve the parameter string before, so just
             * place back the original string placeholder.
             */
            cch = pParamData[i].pEndingAddress /* == pTempMessage */ - pParamData[i].pStartingAddress;
            StringCchCopyNW(pTempFinalMessage, cchBuffer, pParamData[i].pStartingAddress, cch);
            // cch = wcslen(pTempFinalMessage);
        }
        cchBuffer -= cch;
        pTempFinalMessage += cch;
    }

    /* Append the last segment from pMessage */
    StringCchCopyW(pTempFinalMessage, cchBuffer, pTempMessage);

Cleanup:

    // if (Status != ERROR_SUCCESS)
        // *pFinalMessage = NULL;

    if (pParamData)
    {
        for (i = 0; i < dwParamCount; i++)
        {
            if (pParamData[i].pParameter)
                LocalFree(pParamData[i].pParameter);
        }

        HeapFree(GetProcessHeap(), 0, pParamData);
    }

    return Status;
}


/*
 * The following functions were adapted from
 * shell32!dialogs/filedefext.cpp:``SH_...'' functions.
 */

UINT
FormatInteger(LONGLONG Num, LPWSTR pwszResult, UINT cchResultMax)
{
    WCHAR wszNumber[24];
    WCHAR wszDecimalSep[8], wszThousandSep[8];
    NUMBERFMTW nf;
    WCHAR wszGrouping[12];
    INT cchGrouping;
    INT cchResult;
    INT i;

    // Print the number in uniform mode
    swprintf(wszNumber, L"%I64u", Num);

    // Get system strings for decimal and thousand separators.
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, wszDecimalSep, _countof(wszDecimalSep));
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, wszThousandSep, _countof(wszThousandSep));

    // Initialize format for printing the number in bytes
    ZeroMemory(&nf, sizeof(nf));
    nf.lpDecimalSep = wszDecimalSep;
    nf.lpThousandSep = wszThousandSep;

    // Get system string for groups separator
    cchGrouping = GetLocaleInfoW(LOCALE_USER_DEFAULT,
                                 LOCALE_SGROUPING,
                                 wszGrouping,
                                 _countof(wszGrouping));

    // Convert grouping specs from string to integer
    for (i = 0; i < cchGrouping; i++)
    {
        WCHAR wch = wszGrouping[i];

        if (wch >= L'0' && wch <= L'9')
            nf.Grouping = nf.Grouping * 10 + (wch - L'0');
        else if (wch != L';')
            break;
    }

    if ((nf.Grouping % 10) == 0)
        nf.Grouping /= 10;
    else
        nf.Grouping *= 10;

    // Format the number
    cchResult = GetNumberFormatW(LOCALE_USER_DEFAULT,
                                 0,
                                 wszNumber,
                                 &nf,
                                 pwszResult,
                                 cchResultMax);

    if (!cchResult)
        return 0;

    // GetNumberFormatW returns number of characters including UNICODE_NULL
    return cchResult - 1;
}

UINT
FormatByteSize(LONGLONG cbSize, LPWSTR pwszResult, UINT cchResultMax)
{
    INT cchWritten;
    LPWSTR pwszEnd;
    size_t cchRemaining;

    /* Write formated bytes count */
    cchWritten = FormatInteger(cbSize, pwszResult, cchResultMax);
    if (!cchWritten)
        return 0;

    /* Copy " bytes" to buffer */
    pwszEnd = pwszResult + cchWritten;
    cchRemaining = cchResultMax - cchWritten;
    StringCchCopyExW(pwszEnd, cchRemaining, L" ", &pwszEnd, &cchRemaining, 0);
    cchWritten = LoadStringW(hInst, IDS_BYTES_FORMAT, pwszEnd, cchRemaining);
    cchRemaining -= cchWritten;

    return cchResultMax - cchRemaining;
}

LPWSTR
FormatFileSizeWithBytes(const PULARGE_INTEGER lpQwSize, LPWSTR pwszResult, UINT cchResultMax)
{
    UINT cchWritten;
    LPWSTR pwszEnd;
    size_t cchRemaining;

    /* Format bytes in KBs, MBs etc */
    if (StrFormatByteSizeW(lpQwSize->QuadPart, pwszResult, cchResultMax) == NULL)
        return NULL;

    /* If there is less bytes than 1KB, we have nothing to do */
    if (lpQwSize->QuadPart < 1024)
        return pwszResult;

    /* Concatenate " (" */
    cchWritten = wcslen(pwszResult);
    pwszEnd = pwszResult + cchWritten;
    cchRemaining = cchResultMax - cchWritten;
    StringCchCopyExW(pwszEnd, cchRemaining, L" (", &pwszEnd, &cchRemaining, 0);

    /* Write formated bytes count */
    cchWritten = FormatByteSize(lpQwSize->QuadPart, pwszEnd, cchRemaining);
    pwszEnd += cchWritten;
    cchRemaining -= cchWritten;

    /* Copy ")" to the buffer */
    StringCchCopyW(pwszEnd, cchRemaining, L")");

    return pwszResult;
}

/* Adapted from shell32!dialogs/filedefext.cpp:``CFileDefExt::GetFileTimeString'' */
BOOL
GetFileTimeString(LPFILETIME lpFileTime, LPWSTR pwszResult, UINT cchResult)
{
    FILETIME ft;
    SYSTEMTIME st;
    int cchWritten;
    size_t cchRemaining = cchResult;
    LPWSTR pwszEnd = pwszResult;

    if (!FileTimeToLocalFileTime(lpFileTime, &ft) || !FileTimeToSystemTime(&ft, &st))
        return FALSE;

    cchWritten = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, pwszEnd, cchRemaining);
    if (cchWritten)
        --cchWritten; // GetDateFormatW returns count with terminating zero
    // else
        // ERR("GetDateFormatW failed\n");

    cchRemaining -= cchWritten;
    pwszEnd += cchWritten;

    StringCchCopyExW(pwszEnd, cchRemaining, L", ", &pwszEnd, &cchRemaining, 0);

    cchWritten = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, pwszEnd, cchRemaining);
    if (cchWritten)
        --cchWritten; // GetTimeFormatW returns count with terminating zero
    // else
        // ERR("GetTimeFormatW failed\n");

    return TRUE;
}


HTREEITEM
TreeViewAddItem(IN HWND hTreeView,
                IN HTREEITEM hParent,
                IN LPWSTR lpText,
                IN INT Image,
                IN INT SelectedImage,
                IN LPARAM lParam)
{
    TV_INSERTSTRUCTW Insert;

    ZeroMemory(&Insert, sizeof(Insert));

    Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Insert.hInsertAfter = TVI_LAST;
    Insert.hParent = hParent;
    Insert.item.pszText = lpText;
    Insert.item.iImage = Image;
    Insert.item.iSelectedImage = SelectedImage;
    Insert.item.lParam = lParam;

    Insert.item.mask |= TVIF_STATE;
    Insert.item.stateMask = TVIS_OVERLAYMASK;
    Insert.item.state = INDEXTOOVERLAYMASK(1);

    return TreeView_InsertItem(hTreeView, &Insert);
}


/* LOG HELPER FUNCTIONS *******************************************************/

PEVENTLOG
AllocEventLog(IN PCWSTR ComputerName OPTIONAL,
              IN PCWSTR LogName,
              IN BOOL Permanent)
{
    PEVENTLOG EventLog;
    UINT cchName;

    /* Allocate a new event log entry */
    EventLog = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*EventLog));
    if (!EventLog)
        return NULL;

    /* Allocate the computer name string (optional) and copy it */
    if (ComputerName)
    {
        cchName = wcslen(ComputerName) + 1;
        EventLog->ComputerName = HeapAlloc(GetProcessHeap(), 0, cchName * sizeof(WCHAR));
        if (EventLog->ComputerName)
            StringCchCopyW(EventLog->ComputerName, cchName, ComputerName);
    }

    /* Allocate the event log name string and copy it */
    cchName = wcslen(LogName) + 1;
    EventLog->LogName = HeapAlloc(GetProcessHeap(), 0, cchName * sizeof(WCHAR));
    if (!EventLog->LogName)
    {
        HeapFree(GetProcessHeap(), 0, EventLog);
        return NULL;
    }
    StringCchCopyW(EventLog->LogName, cchName, LogName);

    EventLog->Permanent = Permanent;

    return EventLog;
}

VOID
EventLog_Free(IN PEVENTLOG EventLog)
{
    if (EventLog->LogName)
        HeapFree(GetProcessHeap(), 0, EventLog->LogName);

    if (EventLog->FileName)
        HeapFree(GetProcessHeap(), 0, EventLog->FileName);

    HeapFree(GetProcessHeap(), 0, EventLog);
}


PWSTR
AllocAndCopyMultiStr(IN PCWSTR MultiStr OPTIONAL)
{
    PWSTR pStr;
    ULONG Length;

    if (!MultiStr)
        return NULL;

    pStr = (PWSTR)MultiStr;
    while (*pStr) pStr += (wcslen(pStr) + 1);
    Length = MultiStr - pStr + 2;

    pStr = HeapAlloc(GetProcessHeap(), 0, Length * sizeof(WCHAR));
    // NOTE: If we failed allocating the string, then fall back into no filter!
    if (pStr)
        RtlCopyMemory(pStr, MultiStr, Length * sizeof(WCHAR));

    return pStr;
}

PEVENTLOGFILTER
AllocEventLogFilter(// IN PCWSTR FilterName,
                    IN BOOL Information,
                    IN BOOL Warning,
                    IN BOOL Error,
                    IN BOOL AuditSuccess,
                    IN BOOL AuditFailure,
                    IN PCWSTR Sources OPTIONAL,
                    IN PCWSTR Users OPTIONAL,
                    IN PCWSTR ComputerNames OPTIONAL,
                    IN ULONG NumOfEventLogs,
                    IN PEVENTLOG* EventLogs)
{
    PEVENTLOGFILTER EventLogFilter;

    /* Allocate a new event log filter entry, big enough to accommodate the list of logs */
    EventLogFilter = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               FIELD_OFFSET(EVENTLOGFILTER, EventLogs[NumOfEventLogs]));
    if (!EventLogFilter)
        return NULL;

    EventLogFilter->Information  = Information;
    EventLogFilter->Warning      = Warning;
    EventLogFilter->Error        = Error;
    EventLogFilter->AuditSuccess = AuditSuccess;
    EventLogFilter->AuditFailure = AuditFailure;

    /* Allocate and copy the sources, users, and computers multi-strings */
    EventLogFilter->Sources = AllocAndCopyMultiStr(Sources);
    EventLogFilter->Users   = AllocAndCopyMultiStr(Users);
    EventLogFilter->ComputerNames = AllocAndCopyMultiStr(ComputerNames);

    /* Copy the list of event logs */
    EventLogFilter->NumOfEventLogs = NumOfEventLogs;
    RtlCopyMemory(EventLogFilter->EventLogs, EventLogs, NumOfEventLogs * sizeof(PEVENTLOG));

    /* Initialize the filter reference count */
    EventLogFilter->ReferenceCount = 1;

    return EventLogFilter;
}

VOID
EventLogFilter_Free(IN PEVENTLOGFILTER EventLogFilter)
{
    if (EventLogFilter->Sources)
        HeapFree(GetProcessHeap(), 0, EventLogFilter->Sources);

    if (EventLogFilter->Users)
        HeapFree(GetProcessHeap(), 0, EventLogFilter->Users);

    if (EventLogFilter->ComputerNames)
        HeapFree(GetProcessHeap(), 0, EventLogFilter->ComputerNames);

    HeapFree(GetProcessHeap(), 0, EventLogFilter);
}

LONG EventLogFilter_AddRef(IN PEVENTLOGFILTER EventLogFilter)
{
    ASSERT(EventLogFilter);
    return InterlockedIncrement(&EventLogFilter->ReferenceCount);
}

LONG EventLogFilter_Release(IN PEVENTLOGFILTER EventLogFilter)
{
    LONG RefCount;

    ASSERT(EventLogFilter);

    /* When the reference count reaches zero, delete the filter */
    RefCount = InterlockedDecrement(&EventLogFilter->ReferenceCount);
    if (RefCount <= 0)
    {
        /* Remove the filter from the list */
        /** RemoveEntryList(&EventLogFilter->ListEntry); **/
        EventLogFilter_Free(EventLogFilter);
    }

    return RefCount;
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
                       OUT PWCHAR lpModuleName) // TODO: Add IN DWORD BufLen
{
    BOOL Success = FALSE;
    LONG Result;
    DWORD Type, dwSize;
    WCHAR szModuleName[MAX_PATH];
    WCHAR szKeyName[MAX_PATH];
    HKEY hLogKey = NULL;
    HKEY hSourceKey = NULL;

    StringCbCopyW(szKeyName, sizeof(szKeyName), EVENTLOG_BASE_KEY);
    StringCbCatW(szKeyName, sizeof(szKeyName), lpLogName);

    Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           szKeyName,
                           0,
                           KEY_READ,
                           &hLogKey);
    if (Result != ERROR_SUCCESS)
        return FALSE;

    Result = RegOpenKeyExW(hLogKey,
                           SourceName,
                           0,
                           KEY_QUERY_VALUE,
                           &hSourceKey);
    if (Result != ERROR_SUCCESS)
    {
        RegCloseKey(hLogKey);
        return FALSE;
    }

    dwSize = sizeof(szModuleName);
    Result = RegQueryValueExW(hSourceKey,
                              EntryName,
                              NULL,
                              &Type,
                              (LPBYTE)szModuleName,
                              &dwSize);
    if ((Result != ERROR_SUCCESS) || (Type != REG_EXPAND_SZ && Type != REG_SZ))
    {
        szModuleName[0] = UNICODE_NULL;
    }
    else
    {
        /* NULL-terminate the string and expand it */
        szModuleName[dwSize / sizeof(WCHAR) - 1] = UNICODE_NULL;
        ExpandEnvironmentStringsW(szModuleName, lpModuleName, ARRAYSIZE(szModuleName));
        Success = TRUE;
    }

    RegCloseKey(hSourceKey);
    RegCloseKey(hLogKey);

    return Success;
}

BOOL
GetEventCategory(IN LPCWSTR KeyName,
                 IN LPCWSTR SourceName,
                 IN PEVENTLOGRECORD pevlr,
                 OUT PWCHAR CategoryName) // TODO: Add IN DWORD BufLen
{
    BOOL Success = FALSE;
    WCHAR szMessageDLL[MAX_PATH];
    LPWSTR lpMsgBuf = NULL;

    if (!GetEventMessageFileDLL(KeyName, SourceName, EVENT_CATEGORY_MESSAGE_FILE, szMessageDLL))
        goto Quit;

    /* Retrieve the message string without appending extra newlines */
    lpMsgBuf =
    GetMessageStringFromDllList(szMessageDLL,
                                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                pevlr->EventCategory,
                                EVENT_MESSAGE_FILE_BUFFER,
                                NULL);
    if (lpMsgBuf)
    {
        /* Trim the string */
        TrimNulls(lpMsgBuf);

        /* Copy the category name */
        StringCchCopyW(CategoryName, MAX_PATH, lpMsgBuf);

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);

        /* The ID was found and the message was formatted */
        Success = TRUE;
    }

Quit:
    if (!Success)
    {
        if (pevlr->EventCategory != 0)
        {
            StringCchPrintfW(CategoryName, MAX_PATH, L"(%lu)", pevlr->EventCategory);
            Success = TRUE;
        }
    }

    return Success;
}


BOOL                                                                        // NOTE: Used by evtdetctl.c
GetEventMessage(IN LPCWSTR KeyName,
                IN LPCWSTR SourceName,
                IN PEVENTLOGRECORD pevlr,
                OUT PWCHAR EventText) // TODO: Add IN DWORD BufLen
{
    BOOL Success = FALSE;
    DWORD i;
    size_t cch;
    WCHAR SourceModuleName[1024];
    WCHAR ParameterModuleName[1024];
    BOOL IsParamModNameCached = FALSE;
    LPWSTR lpMsgBuf = NULL;
    LPWSTR szStringArray, szMessage;
    LPWSTR *szArguments;

    /* Get the event string array */
    szStringArray = (LPWSTR)((LPBYTE)pevlr + pevlr->StringOffset);

    /* NOTE: GetEventMessageFileDLL can return a comma-separated list of DLLs */
    if (!GetEventMessageFileDLL(KeyName, SourceName, EVENT_MESSAGE_FILE, SourceModuleName))
        goto Quit;

    /* Allocate space for insertion strings */
    szArguments = HeapAlloc(GetProcessHeap(), 0, pevlr->NumStrings * sizeof(LPVOID));
    if (!szArguments)
        goto Quit;

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

    szMessage = szStringArray;
    /*
     * HACK:
     * We do some hackish preformatting of the cached event strings...
     * That's because after we pass the string to FormatMessage
     * (via GetMessageStringFromDllList) with the FORMAT_MESSAGE_ARGUMENT_ARRAY
     * flag, instead of ignoring the insertion parameters and do the formatting
     * by ourselves. Therefore, the resulting string should have the parameter
     * string placeholders starting with a single '%' instead of a mix of one
     * and two '%'.
     */
    /* HACK part 1: Compute the full length of the string array */
    cch = 0;
    for (i = 0; i < pevlr->NumStrings; i++)
    {
        szMessage += wcslen(szMessage) + 1;
    }
    cch = szMessage - szStringArray;

    /* HACK part 2: Now do the HACK proper! */
    szMessage = szStringArray;
    for (i = 0; i < pevlr->NumStrings; i++)
    {
        lpMsgBuf = szMessage;
        while ((lpMsgBuf = wcsstr(lpMsgBuf, L"%%")))
        {
            if (isdigit(lpMsgBuf[2]))
            {
                RtlMoveMemory(lpMsgBuf, lpMsgBuf+1, ((szStringArray + cch) - lpMsgBuf - 1) * sizeof(WCHAR));
            }
        }

        szArguments[i] = szMessage;
        szMessage += wcslen(szMessage) + 1;
    }

    /* Retrieve the message string without appending extra newlines */
    lpMsgBuf =
    GetMessageStringFromDllList(SourceModuleName,
                                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                pevlr->EventID,
                                0,
                                (va_list*)szArguments);
    if (lpMsgBuf)
    {
        /* Trim the string */
        TrimNulls(lpMsgBuf);

        szMessage = NULL;
        Success = (ApplyParameterStringsToMessage(ParameterModuleName,
                                                  TRUE,
                                                  lpMsgBuf,
                                                  &szMessage) == ERROR_SUCCESS);
        if (Success && szMessage)
        {
            /* Free the buffer allocated by FormatMessage */
            LocalFree(lpMsgBuf);
            lpMsgBuf = szMessage;
        }

        /* Copy the event text */
        StringCchCopyW(EventText, EVENT_MESSAGE_EVENTTEXT_BUFFER, lpMsgBuf);

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);
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
        case EVENTLOG_SUCCESS:
            LoadStringW(hInst, IDS_EVENTLOG_SUCCESS, eventTypeText, MAX_LOADSTRING);
            break;
        case EVENTLOG_AUDIT_SUCCESS:
            LoadStringW(hInst, IDS_EVENTLOG_AUDIT_SUCCESS, eventTypeText, MAX_LOADSTRING);
            break;
        case EVENTLOG_AUDIT_FAILURE:
            LoadStringW(hInst, IDS_EVENTLOG_AUDIT_FAILURE, eventTypeText, MAX_LOADSTRING);
            break;
        default:
            LoadStringW(hInst, IDS_EVENTLOG_UNKNOWN_TYPE, eventTypeText, MAX_LOADSTRING);
            break;
    }
}

BOOL
GetEventUserName(IN PEVENTLOGRECORD pelr,
                 IN OUT PSID pLastSid,
                 OUT PWCHAR pszUser) // TODO: Add IN DWORD BufLen
{
    PSID pCurrentSid;
    PWSTR StringSid;
    WCHAR szName[1024];
    WCHAR szDomain[1024];
    SID_NAME_USE peUse;
    DWORD cchName = ARRAYSIZE(szName);
    DWORD cchDomain = ARRAYSIZE(szDomain);
    BOOL Success;

    /* Point to the SID */
    pCurrentSid = (PSID)((LPBYTE)pelr + pelr->UserSidOffset);

    if (!IsValidSid(pCurrentSid))
    {
        pLastSid = NULL;
        return FALSE;
    }
    else if (pLastSid && EqualSid(pLastSid, pCurrentSid))
    {
        return TRUE;
    }

    /* User SID */
    if (pelr->UserSidLength > 0)
    {
        /*
         * Try to retrieve the user account name and domain name corresponding
         * to the SID. If it cannot be retrieved, try to convert the SID to a
         * string-form. It should not be bigger than the user-provided buffer
         * 'pszUser', otherwise we return an error.
         */
        if (LookupAccountSidW(NULL, // FIXME: Use computer name? From the particular event?
                              pCurrentSid,
                              szName,
                              &cchName,
                              szDomain,
                              &cchDomain,
                              &peUse))
        {
            StringCchCopyW(pszUser, MAX_PATH, szName);
            Success = TRUE;
        }
        else if (ConvertSidToStringSidW(pCurrentSid, &StringSid))
        {
            /* Copy the string only if the user-provided buffer is big enough */
            if (wcslen(StringSid) + 1 <= MAX_PATH) // + 1 for NULL-terminator
            {
                StringCchCopyW(pszUser, MAX_PATH, StringSid);
                Success = TRUE;
            }
            else
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                Success = FALSE;
            }

            /* Free the allocated buffer */
            LocalFree(StringSid);
        }
    }

    pLastSid = Success ? pCurrentSid : NULL;

    return Success;
}


static VOID FreeRecords(VOID)
{
    DWORD iIndex;

    if (!g_RecordPtrs)
        return;

    for (iIndex = 0; iIndex < g_TotalRecords; iIndex++)
    {
        if (g_RecordPtrs[iIndex])
            HeapFree(GetProcessHeap(), 0, g_RecordPtrs[iIndex]);
    }
    HeapFree(GetProcessHeap(), 0, g_RecordPtrs);
    g_RecordPtrs = NULL;
    g_TotalRecords = 0;
}

BOOL
FilterByType(IN PEVENTLOGFILTER EventLogFilter,
             IN PEVENTLOGRECORD pevlr)
{
    if ((pevlr->EventType == EVENTLOG_SUCCESS          && !EventLogFilter->Information ) ||
        (pevlr->EventType == EVENTLOG_INFORMATION_TYPE && !EventLogFilter->Information ) ||
        (pevlr->EventType == EVENTLOG_WARNING_TYPE     && !EventLogFilter->Warning     ) ||
        (pevlr->EventType == EVENTLOG_ERROR_TYPE       && !EventLogFilter->Error       ) ||
        (pevlr->EventType == EVENTLOG_AUDIT_SUCCESS    && !EventLogFilter->AuditSuccess) ||
        (pevlr->EventType == EVENTLOG_AUDIT_FAILURE    && !EventLogFilter->AuditFailure))
    {
        return FALSE;
    }
    return TRUE;
}

BOOL
FilterByString(IN PCWSTR FilterString, // This is a multi-string
               IN PWSTR  String)
{
    PCWSTR pStr;

    /* The filter string is NULL so it does not filter anything */
    if (!FilterString)
        return TRUE;

    /*
     * If the filter string filters for an empty string AND the source string
     * is an empty string, we have a match (particular case of the last one).
     */
    if (!*FilterString && !*String)
        return TRUE;

    // if (*FilterString || *String)

    /*
     * If the filter string is empty BUT the source string is not empty,
     * OR vice-versa, we cannot have a match.
     */
    if ( (!*FilterString && *String) || (*FilterString && !*String) )
        return FALSE;

    /*
     * If the filter string filters for at least a non-empty string,
     * browse it and search for a string that matches the source string.
     */
    // else if (*FilterString && *String)
    {
        pStr = FilterString;
        while (*pStr)
        {
            if (wcsicmp(pStr, String) == 0)
            {
                /* We have a match, break the loop */
                break;
            }

            pStr += (wcslen(pStr) + 1);
        }
        if (!*pStr) // && *String
        {
            /* We do not have a match */
            return FALSE;
        }
    }

    /* We have a match */
    return TRUE;
}

/*
 * The events enumerator thread.
 */
static DWORD WINAPI
EnumEventsThread(IN LPVOID lpParameter)
{
    PEVENTLOGFILTER EventLogFilter = (PEVENTLOGFILTER)lpParameter;
    LPWSTR lpMachineName = NULL; // EventLogFilter->ComputerName;
    PEVENTLOG EventLog;

    ULONG LogIndex;
    HANDLE hEventLog;
    PEVENTLOGRECORD pEvlr = NULL;
    PBYTE pEvlrEnd;
    PBYTE pEvlrBuffer;
    DWORD dwWanted, dwRead, dwNeeded, dwStatus = ERROR_SUCCESS;
    DWORD dwTotalRecords = 0, dwCurrentRecord = 0;
    DWORD dwFlags, dwMaxLength;
    size_t cchRemaining;
    LPWSTR lpszSourceName;
    LPWSTR lpszComputerName;
    BOOL bResult = TRUE; /* Read succeeded */
    HANDLE hProcessHeap = GetProcessHeap();
    PSID pLastSid = NULL;

    UINT uStep = 0, uStepAt = 0, uPos = 0;

    WCHAR szWindowTitle[MAX_PATH];
    WCHAR szStatusText[MAX_PATH];
    WCHAR szLocalDate[MAX_PATH];
    WCHAR szLocalTime[MAX_PATH];
    WCHAR szEventID[MAX_PATH];
    WCHAR szEventTypeText[MAX_LOADSTRING];
    WCHAR szCategoryID[MAX_PATH];
    WCHAR szUsername[MAX_PATH];
    WCHAR szNoUsername[MAX_PATH];
    WCHAR szCategory[MAX_PATH];
    WCHAR szNoCategory[MAX_PATH];
    PWCHAR lpTitleTemplateEnd;

    SYSTEMTIME time;
    LVITEMW lviEventItem;

    /* Save the current event log filter globally */
    EventLogFilter_AddRef(EventLogFilter);
    ActiveFilter = EventLogFilter;

    /* Disable list view redraw */
    SendMessageW(hwndListView, WM_SETREDRAW, FALSE, 0);

    /* Clear the list view and free the cached records */
    ListView_DeleteAllItems(hwndListView);
    FreeRecords();

    SendMessageW(hwndListView, LVM_PROGRESS, 0, TRUE);
    ProgressBar_SetRange(hwndStatusProgress, 0);
    StatusBar_SetText(hwndStatus, 0, NULL);
    ShowWindow(hwndStatusProgress, SW_SHOW);

    /* Do a loop over the logs enumerated in the filter */
    // FIXME: For now we only support 1 event log per filter!
    LogIndex = 0;
    // for (LogIndex = 0; LogIndex < EventLogFilter->NumOfEventLogs; ++LogIndex)
    {

    EventLog = EventLogFilter->EventLogs[LogIndex];

    /* Open the event log */
    if (EventLog->Permanent)
        hEventLog = OpenEventLogW(EventLog->ComputerName, EventLog->LogName);
    else
        hEventLog = OpenBackupEventLogW(EventLog->ComputerName, EventLog->LogName); // FileName

    if (hEventLog == NULL)
    {
        ShowLastWin32Error();
        goto Cleanup;
    }

    // GetOldestEventLogRecord(hEventLog, &dwThisRecord);

    /* Get the total number of event log records */
    GetNumberOfEventLogRecords(hEventLog, &dwTotalRecords);

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

    /* Set up the event records cache */
    g_RecordPtrs = HeapAlloc(hProcessHeap, HEAP_ZERO_MEMORY, dwTotalRecords * sizeof(*g_RecordPtrs));
    if (!g_RecordPtrs)
    {
        // ShowLastWin32Error();
        goto Quit;
    }
    g_TotalRecords = dwTotalRecords;

    if (WaitForSingleObject(hStopEnumEvent, 0) == WAIT_OBJECT_0)
        goto Quit;

    LoadStringW(hInst, IDS_NOT_AVAILABLE, szNoUsername, ARRAYSIZE(szNoUsername));
    LoadStringW(hInst, IDS_NONE, szNoCategory, ARRAYSIZE(szNoCategory));

    ProgressBar_SetRange(hwndStatusProgress, MAKELPARAM(0, 100));
    uStepAt = (dwTotalRecords / 100) + 1;

    dwFlags = EVENTLOG_SEQUENTIAL_READ | (NewestEventsFirst ? EVENTLOG_FORWARDS_READ : EVENTLOG_BACKWARDS_READ);

    /* 0x7ffff is the maximum buffer size ReadEventLog will accept */
    dwWanted = 0x7ffff;
    pEvlr = HeapAlloc(hProcessHeap, 0, dwWanted);

    if (!pEvlr)
        goto Quit;

    while (dwStatus == ERROR_SUCCESS)
    {
        bResult =  ReadEventLogW(hEventLog, dwFlags, 0, pEvlr, dwWanted, &dwRead, &dwNeeded);
        dwStatus = GetLastError();

        if (!bResult && dwStatus == ERROR_INSUFFICIENT_BUFFER)
        {
            pEvlr = HeapReAlloc(hProcessHeap, 0, pEvlr, dwNeeded);
            dwWanted = dwNeeded;

            if (!pEvlr)
                break;

            bResult =  ReadEventLogW(hEventLog, dwFlags, 0, pEvlr, dwNeeded, &dwRead, &dwNeeded);

            if (!bResult)
                break;
        }
        else if (!bResult)
        {
            /* exit on other errors (ERROR_HANDLE_EOF) */
            break;
        }

        pEvlrBuffer = (LPBYTE)pEvlr;
        pEvlrEnd = pEvlrBuffer + dwRead;

        while (pEvlrBuffer < pEvlrEnd)
        {
            PEVENTLOGRECORD pEvlrTmp = (PEVENTLOGRECORD)pEvlrBuffer;
            PWSTR lpszUsername, lpszCategoryName;
            g_RecordPtrs[dwCurrentRecord] = NULL;

            // ProgressBar_StepIt(hwndStatusProgress);
            uStep++;
            if (uStep % uStepAt == 0)
            {
                ++uPos;
                ProgressBar_SetPos(hwndStatusProgress, uPos);
            }

           if (WaitForSingleObject(hStopEnumEvent, 0) == WAIT_OBJECT_0)
                goto Quit;

            /* Filter by event type */
            if (!FilterByType(EventLogFilter, pEvlrTmp))
                goto SkipEvent;

            /* Get the event source name and filter it */
            lpszSourceName = (LPWSTR)(pEvlrBuffer + sizeof(EVENTLOGRECORD));
            if (!FilterByString(EventLogFilter->Sources, lpszSourceName))
                goto SkipEvent;

            /* Get the computer name and filter it */
            lpszComputerName = (LPWSTR)(pEvlrBuffer + sizeof(EVENTLOGRECORD) + (wcslen(lpszSourceName) + 1) * sizeof(WCHAR));
            if (!FilterByString(EventLogFilter->ComputerNames, lpszComputerName))
                goto SkipEvent;

            /* Compute the event time */
            EventTimeToSystemTime(pEvlrTmp->TimeWritten, &time);
            GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, szLocalDate, ARRAYSIZE(szLocalDate));
            GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &time, NULL, szLocalTime, ARRAYSIZE(szLocalTime));

            /* Get the username that generated the event, and filter it */
            lpszUsername = GetEventUserName(pEvlrTmp, pLastSid, szUsername) ? szUsername : szNoUsername;

            if (!FilterByString(EventLogFilter->Users, lpszUsername))
                goto SkipEvent;

            // TODO: Filter by event ID and category
            GetEventType(pEvlrTmp->EventType, szEventTypeText);

            lpszCategoryName = GetEventCategory(EventLog->LogName, lpszSourceName, pEvlrTmp, szCategory) ? szCategory : szNoCategory;

            StringCbPrintfW(szEventID, sizeof(szEventID), L"%u", (pEvlrTmp->EventID & 0xFFFF));
            StringCbPrintfW(szCategoryID, sizeof(szCategoryID), L"%u", pEvlrTmp->EventCategory);

            g_RecordPtrs[dwCurrentRecord] = HeapAlloc(hProcessHeap, 0, pEvlrTmp->Length);
            RtlCopyMemory(g_RecordPtrs[dwCurrentRecord], pEvlrTmp, pEvlrTmp->Length);

            lviEventItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
            lviEventItem.iItem = 0;
            lviEventItem.iSubItem = 0;
            lviEventItem.lParam = (LPARAM)g_RecordPtrs[dwCurrentRecord];
            lviEventItem.pszText = szEventTypeText;

            switch (pEvlrTmp->EventType)
            {
                case EVENTLOG_SUCCESS:
                case EVENTLOG_INFORMATION_TYPE:
                    lviEventItem.iImage = 0;
                    break;

                case EVENTLOG_WARNING_TYPE:
                    lviEventItem.iImage = 1;
                    break;

                case EVENTLOG_ERROR_TYPE:
                    lviEventItem.iImage = 2;
                    break;

                case EVENTLOG_AUDIT_SUCCESS:
                    lviEventItem.iImage = 3;
                    break;

                case EVENTLOG_AUDIT_FAILURE:
                    lviEventItem.iImage = 4;
                    break;
            }

            lviEventItem.iItem = ListView_InsertItem(hwndListView, &lviEventItem);

            ListView_SetItemText(hwndListView, lviEventItem.iItem, 1, szLocalDate);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 2, szLocalTime);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 3, lpszSourceName);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 4, lpszCategoryName);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 5, szEventID);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 6, lpszUsername);
            ListView_SetItemText(hwndListView, lviEventItem.iItem, 7, lpszComputerName);

SkipEvent:
            pEvlrBuffer += pEvlrTmp->Length;
            dwCurrentRecord++;
        }
    }

Quit:

    if (pEvlr)
        HeapFree(hProcessHeap, 0, pEvlr);

    /* Close the event log */
    CloseEventLog(hEventLog);

    } // end-for (LogIndex)

    /* All events loaded */

Cleanup:

    ShowWindow(hwndStatusProgress, SW_HIDE);
    SendMessageW(hwndListView, LVM_PROGRESS, 0, FALSE);

    // FIXME: Use something else instead of EventLog->LogName !!

    /*
     * Use a different formatting, whether the event log filter holds
     * only one log, or many logs (the latter case is WIP TODO!)
     */
    if (EventLogFilter->NumOfEventLogs <= 1)
    {
        StringCchPrintfExW(szWindowTitle,
                           ARRAYSIZE(szWindowTitle),
                           &lpTitleTemplateEnd,
                           &cchRemaining,
                           0,
                           szTitleTemplate, szTitle, EventLog->LogName); /* i = number of characters written */
        dwMaxLength = (DWORD)cchRemaining;
        if (!lpMachineName)
            GetComputerNameW(lpTitleTemplateEnd, &dwMaxLength);
        else
            StringCchCopyW(lpTitleTemplateEnd, dwMaxLength, lpMachineName);

        StringCbPrintfW(szStatusText,
                        sizeof(szStatusText),
                        szStatusBarTemplate,
                        EventLog->LogName,
                        dwTotalRecords,
                        ListView_GetItemCount(hwndListView));
    }
    else
    {
        // TODO: Use a different title & implement filtering for multi-log filters !!
        // (EventLogFilter->NumOfEventLogs > 1)
        MessageBoxW(hwndMainWindow,
                    L"Many-logs filtering is not implemented yet!!",
                    L"Event Log",
                    MB_OK | MB_ICONINFORMATION);
    }

    /* Update the status bar */
    StatusBar_SetText(hwndStatus, 0, szStatusText);

    /* Set the window title */
    SetWindowTextW(hwndMainWindow, szWindowTitle);

    /* Resume list view redraw */
    SendMessageW(hwndListView, WM_SETREDRAW, TRUE, 0);

    EventLogFilter_Release(EventLogFilter);

    CloseHandle(hStopEnumEvent);
    InterlockedExchangePointer((PVOID*)&hStopEnumEvent, NULL);

    return 0;
}

/*
 * The purpose of this thread is to serialize the creation of the events
 * enumeration thread, since the Event Log Viewer currently only supports
 * one view, one event list, one enumeration.
 */
static DWORD WINAPI
StartStopEnumEventsThread(IN LPVOID lpParameter)
{
    HANDLE WaitHandles[2];
    DWORD  WaitResult;

    WaitHandles[0] = hStartStopEnumEvent; // End-of-application event
    WaitHandles[1] = hStartEnumEvent;     // Command event

    while (TRUE)
    {
        WaitResult = WaitForMultipleObjects(ARRAYSIZE(WaitHandles),
                                            WaitHandles,
                                            FALSE, // WaitAny
                                            INFINITE);
        switch (WaitResult)
        {
            case WAIT_OBJECT_0 + 0:
            {
                /* End-of-application event signaled, quit this thread */

                /* Stop the previous enumeration */
                if (hEnumEventsThread)
                {
                    if (hStopEnumEvent)
                    {
                        SetEvent(hStopEnumEvent);
                        WaitForSingleObject(hEnumEventsThread, INFINITE);
                        // NOTE: The following is done by the enumeration thread just before terminating.
                        // hStopEnumEvent = NULL;
                    }

                    CloseHandle(hEnumEventsThread);
                    hEnumEventsThread = NULL;
                }

                /* Clear the list view and free the cached records */
                ListView_DeleteAllItems(hwndListView);
                FreeRecords();

                /* Reset the active filter */
                ActiveFilter = NULL;

                return 0;
            }

            case WAIT_OBJECT_0 + 1:
            {
                /* Restart a new enumeration if needed */
                PEVENTLOGFILTER EventLogFilter;

                /* Stop the previous enumeration */
                if (hEnumEventsThread)
                {
                    if (hStopEnumEvent)
                    {
                        SetEvent(hStopEnumEvent);
                        WaitForSingleObject(hEnumEventsThread, INFINITE);
                        // NOTE: The following is done by the enumeration thread just before terminating.
                        // hStopEnumEvent = NULL;
                    }

                    CloseHandle(hEnumEventsThread);
                    hEnumEventsThread = NULL;
                }

                /* Clear the list view and free the cached records */
                ListView_DeleteAllItems(hwndListView);
                FreeRecords();

                /* Reset the active filter */
                ActiveFilter = NULL;

                EventLogFilter = InterlockedExchangePointer((PVOID*)&EnumFilter, NULL);
                if (!EventLogFilter)
                    break;

                // Manual-reset event
                hStopEnumEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
                if (!hStopEnumEvent)
                    break;

                hEnumEventsThread = CreateThread(NULL,
                                                 0,
                                                 EnumEventsThread,
                                                 (LPVOID)EventLogFilter,
                                                 CREATE_SUSPENDED,
                                                 NULL);
                if (!hEnumEventsThread)
                {
                    CloseHandle(hStopEnumEvent);
                    hStopEnumEvent = NULL;
                    break;
                }
                // CloseHandle(hEnumEventsThread);
                ResumeThread(hEnumEventsThread);

                break;
            }

            default:
            {
                /* Unknown command, must never go there! */
                return GetLastError();
            }
        }
    }

    return 0;
}

VOID
EnumEvents(IN PEVENTLOGFILTER EventLogFilter)
{
    /* Signal the enumerator thread we want to enumerate events */
    InterlockedExchangePointer((PVOID*)&EnumFilter, EventLogFilter);
    SetEvent(hStartEnumEvent);
    return;
}


PEVENTLOGFILTER
GetSelectedFilter(OUT HTREEITEM* phti OPTIONAL)
{
    TVITEMEXW tvItemEx;
    HTREEITEM hti;

    if (phti)
        *phti = NULL;

    /* Get index of selected item */
    hti = TreeView_GetSelection(hwndTreeView);
    if (hti == NULL)
        return NULL; // No filter

    tvItemEx.mask = TVIF_PARAM;
    tvItemEx.hItem = hti;

    TreeView_GetItem(hwndTreeView, &tvItemEx);

    if (phti)
        *phti = tvItemEx.hItem;

    return (PEVENTLOGFILTER)tvItemEx.lParam;
}


VOID
OpenUserEventLog(VOID)
{
    PEVENTLOG EventLog;
    PEVENTLOGFILTER EventLogFilter;
    HTREEITEM hItem = NULL;
    WCHAR szFileName[MAX_PATH];

    ZeroMemory(szFileName, sizeof(szFileName));

    sfn.lpstrFile = szFileName;
    sfn.nMaxFile  = ARRAYSIZE(szFileName);

    if (!GetOpenFileNameW(&sfn))
        return;
    sfn.lpstrFile[sfn.nMaxFile-1] = UNICODE_NULL;

    /* Allocate a new event log entry */
    EventLog = AllocEventLog(NULL, sfn.lpstrFile, FALSE);
    if (EventLog == NULL)
        return;

    /* Allocate a new event log filter entry for this event log */
    EventLogFilter = AllocEventLogFilter(// LogName,
                                         TRUE, TRUE, TRUE, TRUE, TRUE,
                                         NULL, NULL, NULL,
                                         1, &EventLog);
    if (EventLogFilter == NULL)
    {
        HeapFree(GetProcessHeap(), 0, EventLog);
        return;
    }

    /* Add the event log and the filter into their lists */
    InsertTailList(&EventLogList, &EventLog->ListEntry);
    InsertTailList(&EventLogFilterList, &EventLogFilter->ListEntry);

    /* Retrieve and cache the event log file */
    EventLog->FileName = HeapAlloc(GetProcessHeap(), 0, sfn.nMaxFile * sizeof(WCHAR));
    if (EventLog->FileName)
        StringCchCopyW(EventLog->FileName, sfn.nMaxFile, sfn.lpstrFile);

    hItem = TreeViewAddItem(hwndTreeView, htiUserLogs,
                            szFileName,
                            2, 3, (LPARAM)EventLogFilter);

    /* Select the event log */
    if (hItem)
    {
        // TreeView_Expand(hwndTreeView, htiUserLogs, TVE_EXPAND);
        TreeView_SelectItem(hwndTreeView, hItem);
        TreeView_EnsureVisible(hwndTreeView, hItem);
    }
    SetFocus(hwndTreeView);
}

VOID
SaveEventLog(IN PEVENTLOGFILTER EventLogFilter)
{
    PEVENTLOG EventLog;
    HANDLE hEventLog;
    WCHAR szFileName[MAX_PATH];

    /* Bail out if there is no available filter */
    if (!EventLogFilter)
        return;

    ZeroMemory(szFileName, sizeof(szFileName));

    sfn.lpstrFile = szFileName;
    sfn.nMaxFile  = ARRAYSIZE(szFileName);

    if (!GetSaveFileNameW(&sfn))
        return;

    EventLogFilter_AddRef(EventLogFilter);

    EventLog = EventLogFilter->EventLogs[0];
    hEventLog = OpenEventLogW(EventLog->ComputerName, EventLog->LogName);

    EventLogFilter_Release(EventLogFilter);

    if (!hEventLog)
    {
        ShowLastWin32Error();
        return;
    }

    if (!BackupEventLogW(hEventLog, szFileName))
        ShowLastWin32Error();

    CloseEventLog(hEventLog);
}

VOID
CloseUserEventLog(IN PEVENTLOGFILTER EventLogFilter, IN HTREEITEM hti)
{
    /* Bail out if there is no available filter */
    if (!EventLogFilter)
        return;

    if (InterlockedCompareExchangePointer((PVOID*)&ActiveFilter, NULL, NULL) == EventLogFilter)
    {
        /* Signal the enumerator thread we want to stop enumerating events */
        // EnumEvents(NULL);
        InterlockedExchangePointer((PVOID*)&EnumFilter, NULL);
        SetEvent(hStartEnumEvent);
    }

    /*
     * The deletion of the item automatically triggers a TVN_SELCHANGED
     * notification, that will reset the ActiveFilter (in case the item
     * selected is a filter). Otherwise we reset it there.
     */
    TreeView_DeleteItem(hwndTreeView, hti);

    /* Remove the filter from the list */
    RemoveEntryList(&EventLogFilter->ListEntry);
    EventLogFilter_Release(EventLogFilter);

    // /* Select the default event log */
    // // TreeView_Expand(hwndTreeView, htiUserLogs, TVE_EXPAND);
    // TreeView_SelectItem(hwndTreeView, hItem);
    // TreeView_EnsureVisible(hwndTreeView, hItem);
    SetFocus(hwndTreeView);
}


BOOL
ClearEvents(IN PEVENTLOGFILTER EventLogFilter)
{
    BOOL Success;
    PEVENTLOG EventLog;
    HANDLE hEventLog;
    WCHAR szFileName[MAX_PATH];
    WCHAR szMessage[MAX_LOADSTRING];

    /* Bail out if there is no available filter */
    if (!EventLogFilter)
        return FALSE;

    ZeroMemory(szFileName, sizeof(szFileName));
    ZeroMemory(szMessage, sizeof(szMessage));

    LoadStringW(hInst, IDS_CLEAREVENTS_MSG, szMessage, ARRAYSIZE(szMessage));

    sfn.lpstrFile = szFileName;
    sfn.nMaxFile  = ARRAYSIZE(szFileName);

    switch (MessageBoxW(hwndMainWindow, szMessage, szTitle, MB_YESNOCANCEL | MB_ICONINFORMATION))
    {
        case IDCANCEL:
            return FALSE;

        case IDNO:
            sfn.lpstrFile = NULL;
            break;

        case IDYES:
            if (!GetSaveFileNameW(&sfn))
                return FALSE;
            break;
    }

    EventLogFilter_AddRef(EventLogFilter);

    EventLog = EventLogFilter->EventLogs[0];
    hEventLog = OpenEventLogW(EventLog->ComputerName, EventLog->LogName);

    EventLogFilter_Release(EventLogFilter);

    if (!hEventLog)
    {
        ShowLastWin32Error();
        return FALSE;
    }

    Success = ClearEventLogW(hEventLog, sfn.lpstrFile);
    if (!Success)
        ShowLastWin32Error();

    CloseEventLog(hEventLog);
    return Success;
}


VOID
Refresh(IN PEVENTLOGFILTER EventLogFilter)
{
    /* Bail out if there is no available filter */
    if (!EventLogFilter)
        return;

    /* Reenumerate the events through the filter */
    EnumEvents(EventLogFilter);
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
    wcex.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1); // COLOR_WINDOW + 1
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
GetDisplayNameFileAndID(IN LPCWSTR lpLogName,
                        OUT PWCHAR lpModuleName, // TODO: Add IN DWORD BufLen
                        OUT PDWORD pdwMessageID)
{
    BOOL Success = FALSE;
    LONG Result;
    HKEY hLogKey;
    WCHAR *KeyPath;
    SIZE_T cbKeyPath;
    DWORD Type, cbData;
    DWORD dwMessageID = 0;
    WCHAR szModuleName[MAX_PATH];

    /* Use a default value for the message ID */
    *pdwMessageID = 0;

    cbKeyPath = (wcslen(EVENTLOG_BASE_KEY) + wcslen(lpLogName) + 1) * sizeof(WCHAR);
    KeyPath = HeapAlloc(GetProcessHeap(), 0, cbKeyPath);
    if (!KeyPath)
        return FALSE;

    StringCbCopyW(KeyPath, cbKeyPath, EVENTLOG_BASE_KEY);
    StringCbCatW(KeyPath, cbKeyPath, lpLogName);

    Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyPath, 0, KEY_QUERY_VALUE, &hLogKey);
    HeapFree(GetProcessHeap(), 0, KeyPath);
    if (Result != ERROR_SUCCESS)
        return FALSE;

    cbData = sizeof(szModuleName);
    Result = RegQueryValueExW(hLogKey,
                              L"DisplayNameFile",
                              NULL,
                              &Type,
                              (LPBYTE)szModuleName,
                              &cbData);
    if ((Result != ERROR_SUCCESS) || (Type != REG_EXPAND_SZ && Type != REG_SZ))
    {
        szModuleName[0] = UNICODE_NULL;
    }
    else
    {
        /* NULL-terminate the string and expand it */
        szModuleName[cbData / sizeof(WCHAR) - 1] = UNICODE_NULL;
        ExpandEnvironmentStringsW(szModuleName, lpModuleName, ARRAYSIZE(szModuleName));
        Success = TRUE;
    }

    /*
     * If we have a 'DisplayNameFile', query for 'DisplayNameID';
     * otherwise it's not really useful. 'DisplayNameID' is optional.
     */
    if (Success)
    {
        cbData = sizeof(dwMessageID);
        Result = RegQueryValueExW(hLogKey,
                                  L"DisplayNameID",
                                  NULL,
                                  &Type,
                                  (LPBYTE)&dwMessageID,
                                  &cbData);
        if ((Result != ERROR_SUCCESS) || (Type != REG_DWORD))
            dwMessageID = 0;

        *pdwMessageID = dwMessageID;
    }

    RegCloseKey(hLogKey);

    return Success;
}


VOID
BuildLogListAndFilterList(IN LPCWSTR lpComputerName)
{
    LONG Result;
    HKEY hEventLogKey, hLogKey;
    DWORD dwNumLogs = 0;
    DWORD dwIndex, dwMaxKeyLength;
    DWORD Type;
    PEVENTLOG EventLog;
    PEVENTLOGFILTER EventLogFilter;
    LPWSTR LogName = NULL;
    WCHAR szModuleName[MAX_PATH];
    DWORD lpcName;
    DWORD dwMessageID;
    LPWSTR lpDisplayName;
    HTREEITEM hRootNode = NULL, hItem = NULL, hItemDefault = NULL;

    /* Open the EventLog key */
    // FIXME: Use local or remote computer
    Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, EVENTLOG_BASE_KEY, 0, KEY_READ, &hEventLogKey);
    if (Result != ERROR_SUCCESS)
    {
        return;
    }

    /* Retrieve the number of event logs enumerated as registry keys */
    Result = RegQueryInfoKeyW(hEventLogKey, NULL, NULL, NULL, &dwNumLogs, &dwMaxKeyLength,
                              NULL, NULL, NULL, NULL, NULL, NULL);
    if (Result != ERROR_SUCCESS)
    {
        goto Quit;
    }
    if (!dwNumLogs)
        goto Quit;

    /* Take the NULL terminator into account */
    ++dwMaxKeyLength;

    /* Allocate the temporary buffer */
    LogName = HeapAlloc(GetProcessHeap(), 0, dwMaxKeyLength * sizeof(WCHAR));
    if (!LogName)
        goto Quit;

    /* Enumerate and retrieve each event log name */
    for (dwIndex = 0; dwIndex < dwNumLogs; dwIndex++)
    {
        lpcName = dwMaxKeyLength;
        Result = RegEnumKeyExW(hEventLogKey, dwIndex, LogName, &lpcName, NULL, NULL, NULL, NULL);
        if (Result != ERROR_SUCCESS)
            continue;

        /* Take the NULL terminator into account */
        ++lpcName;

        /* Allocate a new event log entry */
        EventLog = AllocEventLog(lpComputerName, LogName, TRUE);
        if (EventLog == NULL)
            continue;

        /* Allocate a new event log filter entry for this event log */
        EventLogFilter = AllocEventLogFilter(// LogName,
                                             TRUE, TRUE, TRUE, TRUE, TRUE,
                                             NULL, NULL, NULL,
                                             1, &EventLog);
        if (EventLogFilter == NULL)
        {
            HeapFree(GetProcessHeap(), 0, EventLog);
            continue;
        }

        /* Add the event log and the filter into their lists */
        InsertTailList(&EventLogList, &EventLog->ListEntry);
        InsertTailList(&EventLogFilterList, &EventLogFilter->ListEntry);

        EventLog->FileName = NULL;

        /* Retrieve and cache the event log file */
        Result = RegOpenKeyExW(hEventLogKey,
                               LogName,
                               0,
                               KEY_QUERY_VALUE,
                               &hLogKey);
        if (Result == ERROR_SUCCESS)
        {
            lpcName = 0;
            Result = RegQueryValueExW(hLogKey,
                                      L"File",
                                      NULL,
                                      &Type,
                                      NULL,
                                      &lpcName);
            if ((Result != ERROR_SUCCESS) || (Type != REG_EXPAND_SZ && Type != REG_SZ))
            {
                // Windows' EventLog uses some kind of default value, we do not.
                EventLog->FileName = NULL;
            }
            else
            {
                lpcName = ROUND_DOWN(lpcName, sizeof(WCHAR));
                EventLog->FileName = HeapAlloc(GetProcessHeap(), 0, lpcName);
                if (EventLog->FileName)
                {
                    Result = RegQueryValueExW(hLogKey,
                                              L"File",
                                              NULL,
                                              &Type,
                                              (LPBYTE)EventLog->FileName,
                                              &lpcName);
                    if (Result != ERROR_SUCCESS)
                    {
                        HeapFree(GetProcessHeap(), 0, EventLog->FileName);
                        EventLog->FileName = NULL;
                    }
                    else
                    {
                        EventLog->FileName[lpcName / sizeof(WCHAR) - 1] = UNICODE_NULL;
                    }
                }
            }

            RegCloseKey(hLogKey);
        }

        /* Get the display name for the event log */
        lpDisplayName = NULL;

        ZeroMemory(szModuleName, sizeof(szModuleName));
        if (GetDisplayNameFileAndID(LogName, szModuleName, &dwMessageID))
        {
            /* Retrieve the message string without appending extra newlines */
            lpDisplayName =
            GetMessageStringFromDll(szModuleName,
                                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                                    FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                    dwMessageID,
                                    0,
                                    NULL);
        }

        /*
         * Select the correct tree root node, whether the log is a System
         * or an Application log. Default to Application log otherwise.
         */
        hRootNode = htiAppLogs;
        for (lpcName = 0; lpcName < ARRAYSIZE(SystemLogs); ++lpcName)
        {
            /* Check whether the log name is part of the system logs */
            if (wcsicmp(LogName, SystemLogs[lpcName]) == 0)
            {
                hRootNode = htiSystemLogs;
                break;
            }
        }

        hItem = TreeViewAddItem(hwndTreeView, hRootNode,
                                (lpDisplayName ? lpDisplayName : LogName),
                                2, 3, (LPARAM)EventLogFilter);

        /* Try to get the default event log: "Application" */
        if ((hItemDefault == NULL) && (wcsicmp(LogName, SystemLogs[0]) == 0))
        {
            hItemDefault = hItem;
        }

        /* Free the buffer allocated by FormatMessage */
        if (lpDisplayName)
            LocalFree(lpDisplayName);
    }

    HeapFree(GetProcessHeap(), 0, LogName);

Quit:
    RegCloseKey(hEventLogKey);

    /* Select the default event log */
    if (hItemDefault)
    {
        // TreeView_Expand(hwndTreeView, hRootNode, TVE_EXPAND);
        TreeView_SelectItem(hwndTreeView, hItemDefault);
        TreeView_EnsureVisible(hwndTreeView, hItemDefault);
    }
    SetFocus(hwndTreeView);

    return;
}

VOID
FreeLogList(VOID)
{
    PLIST_ENTRY Entry;
    PEVENTLOG EventLog;

    while (!IsListEmpty(&EventLogList))
    {
        Entry = RemoveHeadList(&EventLogList);
        EventLog = (PEVENTLOG)CONTAINING_RECORD(Entry, EVENTLOG, ListEntry);
        EventLog_Free(EventLog);
    }

    return;
}

VOID
FreeLogFilterList(VOID)
{
    PLIST_ENTRY Entry;
    PEVENTLOGFILTER EventLogFilter;

    while (!IsListEmpty(&EventLogFilterList))
    {
        Entry = RemoveHeadList(&EventLogFilterList);
        EventLogFilter = (PEVENTLOGFILTER)CONTAINING_RECORD(Entry, EVENTLOGFILTER, ListEntry);
        EventLogFilter_Free(EventLogFilter);
    }

    ActiveFilter = NULL;

    return;
}

BOOL
InitInstance(HINSTANCE hInstance,
             int nCmdShow)
{
    RECT rcClient, rs;
    LONG StatusHeight;
    HIMAGELIST hSmall;
    LVCOLUMNW lvc = {0};
    WCHAR szTemp[256];

    hInst = hInstance; // Store instance handle in our global variable

    /* Create the main window */
    hwndMainWindow = CreateWindowW(szWindowClass,
                                   szTitle,
                                   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                                   NULL,
                                   NULL,
                                   hInstance,
                                   NULL);
    if (!hwndMainWindow)
        return FALSE;

    /* Create the status bar */
    hwndStatus = CreateWindowExW(0,                                  // no extended styles
                                 STATUSCLASSNAMEW,                   // status bar
                                 L"",                                // no text
                                 WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, // styles
                                 0, 0, 0, 0,                         // x, y, cx, cy
                                 hwndMainWindow,                     // parent window
                                 (HMENU)100,                         // window ID
                                 hInstance,                          // instance
                                 NULL);                              // window data

    GetClientRect(hwndMainWindow, &rcClient);
    GetWindowRect(hwndStatus, &rs);
    StatusHeight = rs.bottom - rs.top;

    /* Create a progress bar in the status bar (hidden by default) */
    StatusBar_GetItemRect(hwndStatus, 0, &rs);
    hwndStatusProgress = CreateWindowExW(0,                          // no extended styles
                                         PROGRESS_CLASSW,            // status bar
                                         NULL,                       // no text
                                         WS_CHILD | PBS_SMOOTH,      // styles
                                         rs.left, rs.top,            // x, y
                                         rs.right-rs.left, rs.bottom-rs.top, // cx, cy
                                         hwndStatus,                 // parent window
                                         NULL,                       // window ID
                                         hInstance,                  // instance
                                         NULL);                      // window data
    ProgressBar_SetStep(hwndStatusProgress, 1);

    /* Initialize the splitter default positions */
    nVSplitPos = 250;
    nHSplitPos = 250;

    /* Create the TreeView */
    hwndTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                   WC_TREEVIEWW,
                                   NULL,
                                   // WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_SHOWSELALWAYS,
                                   WS_CHILD | WS_VISIBLE | /* WS_TABSTOP | */ TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_EDITLABELS | TVS_SHOWSELALWAYS,
                                   0, 0,
                                   nVSplitPos - SPLIT_WIDTH/2,
                                   (rcClient.bottom - rcClient.top) - StatusHeight,
                                   hwndMainWindow,
                                   NULL,
                                   hInstance,
                                   NULL);

    /* Create the ImageList */
    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              ILC_COLOR32 | ILC_MASK, // ILC_COLOR24
                              1, 1);

    /* Add event type icons to the ImageList: closed/opened folder, event log (normal/viewed) */
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_CLOSED_CATEGORY)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_OPENED_CATEGORY)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_EVENTLOG)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_EVENTVWR)));

    /* Assign the ImageList to the Tree View */
    TreeView_SetImageList(hwndTreeView, hSmall, TVSIL_NORMAL);

    /* Add the event logs nodes */
    // "System Logs"
    LoadStringW(hInstance, IDS_EVENTLOG_SYSTEM, szTemp, ARRAYSIZE(szTemp));
    htiSystemLogs = TreeViewAddItem(hwndTreeView, NULL, szTemp, 0, 1, (LPARAM)NULL);
    // "Application Logs"
    LoadStringW(hInstance, IDS_EVENTLOG_APP, szTemp, ARRAYSIZE(szTemp));
    htiAppLogs = TreeViewAddItem(hwndTreeView, NULL, szTemp, 0, 1, (LPARAM)NULL);
    // "User Logs"
    LoadStringW(hInstance, IDS_EVENTLOG_USER, szTemp, ARRAYSIZE(szTemp));
    htiUserLogs = TreeViewAddItem(hwndTreeView, NULL, szTemp, 0, 1, (LPARAM)NULL);

    /* Create the Event details pane (optional) */
    hwndEventDetails = CreateEventDetailsCtrl(hInst, hwndMainWindow, (LPARAM)NULL);
    // hwndEventDetails = NULL;
    if (hwndEventDetails)
    {
    // SetWindowLongPtrW(hwndEventDetails, GWL_STYLE,
                      // GetWindowLongPtrW(hwndEventDetails, GWL_STYLE) | WS_BORDER);
    SetWindowLongPtrW(hwndEventDetails, GWL_EXSTYLE,
                      GetWindowLongPtrW(hwndEventDetails, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);
    SetWindowPos(hwndEventDetails, NULL,
                 nVSplitPos + SPLIT_WIDTH/2,
                 nHSplitPos + SPLIT_WIDTH/2,
                 (rcClient.right - rcClient.left) - nVSplitPos - SPLIT_WIDTH/2,
                 (rcClient.bottom - rcClient.top) - nHSplitPos - SPLIT_WIDTH/2 - StatusHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    /* Create the ListView */
    hwndListView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                   WC_LISTVIEWW,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | LVS_SHOWSELALWAYS | LVS_REPORT,
                                   nVSplitPos + SPLIT_WIDTH/2,
                                   0,
                                   (rcClient.right - rcClient.left) - nVSplitPos - SPLIT_WIDTH/2,
                                   hwndEventDetails ? nHSplitPos - SPLIT_WIDTH/2
                                                    : (rcClient.bottom - rcClient.top) - StatusHeight,
                                   hwndMainWindow,
                                   NULL,
                                   hInstance,
                                   NULL);

    /* Add the extended ListView styles */
    ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT |LVS_EX_LABELTIP);

    /* Create the ImageList */
    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              ILC_COLOR32 | ILC_MASK, // ILC_COLOR24
                              1, 1);

    /* Add event type icons to the ImageList */
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_INFORMATIONICON)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WARNINGICON)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ERRORICON)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_AUDITSUCCESSICON)));
    ImageList_AddIcon(hSmall, LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_AUDITFAILUREICON)));

    /* Assign the ImageList to the List View */
    ListView_SetImageList(hwndListView, hSmall, LVSIL_SMALL);

    /* Now set up the listview with its columns */
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = 90;
    LoadStringW(hInstance,
                IDS_COLUMNTYPE,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 0, &lvc);

    lvc.cx = 70;
    LoadStringW(hInstance,
                IDS_COLUMNDATE,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 1, &lvc);

    lvc.cx = 70;
    LoadStringW(hInstance,
                IDS_COLUMNTIME,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 2, &lvc);

    lvc.cx = 150;
    LoadStringW(hInstance,
                IDS_COLUMNSOURCE,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 3, &lvc);

    lvc.cx = 100;
    LoadStringW(hInstance,
                IDS_COLUMNCATEGORY,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 4, &lvc);

    lvc.cx = 60;
    LoadStringW(hInstance,
                IDS_COLUMNEVENT,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 5, &lvc);

    lvc.cx = 120;
    LoadStringW(hInstance,
                IDS_COLUMNUSER,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 6, &lvc);

    lvc.cx = 100;
    LoadStringW(hInstance,
                IDS_COLUMNCOMPUTER,
                szTemp,
                ARRAYSIZE(szTemp));
    lvc.pszText = szTemp;
    ListView_InsertColumn(hwndListView, 7, &lvc);

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

    return TRUE;
}

VOID ResizeWnd(INT cx, INT cy)
{
    HDWP hdwp;
    RECT rs;
    LONG StatusHeight;

    /* Resize the status bar -- now done in WM_SIZE */
    // SendMessageW(hwndStatus, WM_SIZE, 0, 0);
    GetWindowRect(hwndStatus, &rs);
    StatusHeight = rs.bottom - rs.top;

    /* Move the progress bar */
    StatusBar_GetItemRect(hwndStatus, 0, &rs);
    MoveWindow(hwndStatusProgress,
               rs.left, rs.top, rs.right-rs.left, rs.bottom-rs.top,
               IsWindowVisible(hwndStatusProgress) ? TRUE : FALSE);

    /*
     * TODO: Adjust the splitter positions:
     * - Vertical splitter (1)  : fixed position from the left window side.
     * - Horizontal splitter (2): fixed position from the bottom window side.
     */
    nVSplitPos = min(max(nVSplitPos, SPLIT_WIDTH/2), cx - SPLIT_WIDTH/2); // OK
    nHSplitPos = min(max(nHSplitPos, SPLIT_WIDTH/2), cy - SPLIT_WIDTH/2 - StatusHeight); // FIXME!

    hdwp = BeginDeferWindowPos(3);

    if (hdwp)
        hdwp = DeferWindowPos(hdwp,
                              hwndTreeView,
                              HWND_TOP,
                              0, 0,
                              nVSplitPos - SPLIT_WIDTH/2,
                              cy - StatusHeight,
                              SWP_NOZORDER | SWP_NOACTIVATE);

    if (hdwp)
        hdwp = DeferWindowPos(hdwp,
                              hwndListView,
                              HWND_TOP,
                              nVSplitPos + SPLIT_WIDTH/2, 0,
                              cx - nVSplitPos - SPLIT_WIDTH/2,
                              hwndEventDetails ? nHSplitPos - SPLIT_WIDTH/2
                                               : cy - StatusHeight,
                              SWP_NOZORDER | SWP_NOACTIVATE);

    if (hwndEventDetails && hdwp)
        hdwp = DeferWindowPos(hdwp,
                              hwndEventDetails,
                              HWND_TOP,
                              nVSplitPos + SPLIT_WIDTH/2,
                              nHSplitPos + SPLIT_WIDTH/2,
                              cx - nVSplitPos - SPLIT_WIDTH/2,
                              cy - nHSplitPos - SPLIT_WIDTH/2 - StatusHeight,
                              SWP_NOZORDER | SWP_NOACTIVATE);

    if (hdwp)
        EndDeferWindowPos(hdwp);
}


LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rect;

    switch (uMsg)
    {
        case WM_CREATE:
            hMainMenu = GetMenu(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == hwndListView)
            {
                switch (hdr->code)
                {
                    case LVN_ITEMCHANGED:
                    {
                        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

                        if ( (pnmv->uChanged  & LVIF_STATE) && /* The state has changed */
                             (pnmv->uNewState & LVIS_SELECTED) /* The item has been (de)selected */ )
                        {
                            if (hwndEventDetails)
                                SendMessageW(hwndEventDetails, EVT_DISPLAY, 0, 0);
                        }
                        break;
                    }

                    case NM_DBLCLK:
                    case NM_RETURN:
                        SendMessageW(hWnd, WM_COMMAND, IDM_EVENT_DETAILS, 0);
                        break;
                }
            }
            else if (hdr->hwndFrom == hwndTreeView)
            {
                switch (hdr->code)
                {
                    case TVN_BEGINLABELEDIT:
                    {
                        HTREEITEM hItem = ((LPNMTVDISPINFO)lParam)->item.hItem;

                        /* Disable label editing for root nodes */
                        return ((hItem == htiSystemLogs) ||
                                (hItem == htiAppLogs)    ||
                                (hItem == htiUserLogs));
                    }

                    case TVN_ENDLABELEDIT:
                    {
                        TVITEMW item = ((LPNMTVDISPINFO)lParam)->item;
                        HTREEITEM hItem = item.hItem;

                        /* Disable label editing for root nodes */
                        if ((hItem == htiSystemLogs) ||
                            (hItem == htiAppLogs)    ||
                            (hItem == htiUserLogs))
                        {
                            return FALSE;
                        }

                        if (item.pszText)
                        {
                            LPWSTR pszText = item.pszText;

                            /* Trim all whitespace */
                            while (*pszText && iswspace(*pszText))
                                ++pszText;

                            if (!*pszText)
                                return FALSE;

                            return TRUE;
                        }
                        else
                        {
                            return FALSE;
                        }
                    }

                    case TVN_SELCHANGED:
                    {
                        PEVENTLOGFILTER EventLogFilter =
                            (PEVENTLOGFILTER)((LPNMTREEVIEW)lParam)->itemNew.lParam;

                        // FIXME: It might be nice to reference here the filter,
                        // so that we don't have to reference/dereference it many times
                        // in the other functions???

                        // FIXME: This is a hack!!
                        if (hwndEventDetails && EventLogFilter)
                        {
                            SendMessageW(hwndEventDetails, EVT_SETFILTER, 0, (LPARAM)EventLogFilter);
                        }

                        if (EventLogFilter)
                        {
                            /*
                             * If we have selected a filter, enable the menu commands;
                             * they will possibly be updated after events enumeration.
                             */
                            EnableMenuItem(hMainMenu, IDM_SAVE_EVENTLOG, MF_BYCOMMAND | MF_ENABLED);
                            EnableMenuItem(hMainMenu, IDM_CLOSE_EVENTLOG, MF_BYCOMMAND | MF_ENABLED);
                            EnableMenuItem(hMainMenu, IDM_CLEAR_EVENTS, MF_BYCOMMAND | MF_ENABLED);
                            EnableMenuItem(hMainMenu, IDM_RENAME_EVENTLOG, MF_BYCOMMAND | MF_ENABLED);
                            EnableMenuItem(hMainMenu, IDM_EVENTLOG_SETTINGS, MF_BYCOMMAND | MF_ENABLED);
                        }
                        else
                        {
                            EnableMenuItem(hMainMenu, IDM_SAVE_EVENTLOG, MF_BYCOMMAND | MF_GRAYED);
                            EnableMenuItem(hMainMenu, IDM_CLOSE_EVENTLOG, MF_BYCOMMAND | MF_GRAYED);
                            EnableMenuItem(hMainMenu, IDM_CLEAR_EVENTS, MF_BYCOMMAND | MF_GRAYED);
                            EnableMenuItem(hMainMenu, IDM_RENAME_EVENTLOG, MF_BYCOMMAND | MF_GRAYED);
                            EnableMenuItem(hMainMenu, IDM_EVENTLOG_SETTINGS, MF_BYCOMMAND | MF_GRAYED);
                        }

                        /*
                         * The enumeration thread that is triggered by EnumEvents
                         * will set a new value for the 'ActiveFilter'.
                         */
                        if (EventLogFilter)
                            EnumEvents(EventLogFilter);

                        break;
                    }
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            /* Parse the menu selections */
            switch (LOWORD(wParam))
            {
                case IDM_OPEN_EVENTLOG:
                    OpenUserEventLog();
                    break;

                case IDM_SAVE_EVENTLOG:
                    SaveEventLog(GetSelectedFilter(NULL));
                    break;

                case IDM_CLOSE_EVENTLOG:
                {
                    HTREEITEM hti;
                    PEVENTLOGFILTER EventLogFilter = GetSelectedFilter(&hti);
                    CloseUserEventLog(EventLogFilter, hti);
                    break;
                }

                case IDM_CLEAR_EVENTS:
                {
                    PEVENTLOGFILTER EventLogFilter = GetSelectedFilter(NULL);
                    if (EventLogFilter && ClearEvents(EventLogFilter))
                        Refresh(EventLogFilter);
                    break;
                }

                case IDM_RENAME_EVENTLOG:
                    if (GetFocus() == hwndTreeView)
                        TreeView_EditLabel(hwndTreeView, TreeView_GetSelection(hwndTreeView));
                    break;

                case IDM_EVENTLOG_SETTINGS:
                {
                    PEVENTLOGFILTER EventLogFilter = GetSelectedFilter(NULL);
                    // TODO: Check the returned value?
                    if (EventLogFilter)
                        EventLogProperties(hInst, hWnd, EventLogFilter);
                    break;
                }

                case IDM_LIST_NEWEST:
                {
                    CheckMenuRadioItem(hMainMenu, IDM_LIST_NEWEST, IDM_LIST_OLDEST, IDM_LIST_NEWEST, MF_BYCOMMAND);
                    if (!NewestEventsFirst)
                    {
                        NewestEventsFirst = TRUE;
                        Refresh(GetSelectedFilter(NULL));
                    }
                    break;
                }

                case IDM_LIST_OLDEST:
                {
                    CheckMenuRadioItem(hMainMenu, IDM_LIST_NEWEST, IDM_LIST_OLDEST, IDM_LIST_OLDEST, MF_BYCOMMAND);
                    if (NewestEventsFirst)
                    {
                        NewestEventsFirst = FALSE;
                        Refresh(GetSelectedFilter(NULL));
                    }
                    break;
                }

                case IDM_EVENT_DETAILS:
                {
                    // LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
                    PEVENTLOGFILTER EventLogFilter = GetSelectedFilter(NULL);
                    if (/*lpnmitem->iItem != -1 &&*/ EventLogFilter)
                    {
                        EventLogFilter_AddRef(EventLogFilter);
                        DialogBoxParamW(hInst,
                                        MAKEINTRESOURCEW(IDD_EVENTDETAILS_DLG),
                                        hWnd,
                                        EventDetails,
                                        (LPARAM)EventLogFilter);
                        EventLogFilter_Release(EventLogFilter);
                    }
                    break;
                }

                case IDM_REFRESH:
                    Refresh(GetSelectedFilter(NULL));
                    break;

                case IDM_ABOUT:
                {
                    HICON hIcon;
                    WCHAR szCopyright[MAX_LOADSTRING];

                    hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_EVENTVWR));
                    LoadStringW(hInst, IDS_COPYRIGHT, szCopyright, ARRAYSIZE(szCopyright));
                    ShellAboutW(hWnd, szTitle, szCopyright, hIcon);
                    DeleteObject(hIcon);
                    break;
                }

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
                    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
            }
            break;
        }

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);

                /* Set the cursor for the vertical splitter */
                if (pt.x >= nVSplitPos - SPLIT_WIDTH/2 && pt.x < nVSplitPos + SPLIT_WIDTH/2 + 1)
                {
                    RECT rs;
                    GetClientRect(hWnd, &rect);
                    GetWindowRect(hwndStatus, &rs);
                    if (pt.y >= rect.top && pt.y < rect.bottom - (rs.bottom - rs.top))
                    {
                        SetCursor(LoadCursorW(NULL, IDC_SIZEWE));
                        return TRUE;
                    }
                }
                else
                /* Set the cursor for the horizontal splitter, if the Event details pane is displayed */
                if (hwndEventDetails &&
                    (pt.y >= nHSplitPos - SPLIT_WIDTH/2 && pt.y < nHSplitPos + SPLIT_WIDTH/2 + 1))
                {
                    // RECT rs;
                    GetClientRect(hWnd, &rect);
                    // GetWindowRect(hwndStatus, &rs);
                    if (pt.x >= nVSplitPos + SPLIT_WIDTH/2 + 1 /* rect.left + (rs.bottom - rs.top) */ &&
                        pt.x < rect.right)
                    {
                        SetCursor(LoadCursorW(NULL, IDC_SIZENS));
                        return TRUE;
                    }
                }
            }
            goto Default;

        case WM_LBUTTONDOWN:
        {
            INT x = GET_X_LPARAM(lParam);
            INT y = GET_Y_LPARAM(lParam);

            /* Reset the splitter state */
            bSplit = 0;

            /* Capture the cursor for the vertical splitter */
            if (x >= nVSplitPos - SPLIT_WIDTH/2 && x < nVSplitPos + SPLIT_WIDTH/2 + 1)
            {
                bSplit = 1;
                SetCapture(hWnd);
            }
            else
            /* Capture the cursor for the horizontal splitter, if the Event details pane is displayed */
            if (hwndEventDetails &&
                (y >= nHSplitPos - SPLIT_WIDTH/2 && y < nHSplitPos + SPLIT_WIDTH/2 + 1))
            {
                bSplit = 2;
                SetCapture(hWnd);
            }
            break;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
            if (GetCapture() == hWnd)
            {
                /* Adjust the correct splitter position */
                if (bSplit == 1)
                    nVSplitPos = GET_X_LPARAM(lParam);
                else if (bSplit == 2)
                    nHSplitPos = GET_Y_LPARAM(lParam);

                /* If we are splitting, resize the windows */
                if (bSplit != 0)
                {
                    GetClientRect(hWnd, &rect);
                    ResizeWnd(rect.right - rect.left, rect.bottom - rect.top);
                }

                /* Reset the splitter state */
                bSplit = 0;

                ReleaseCapture();
            }
            break;

        case WM_MOUSEMOVE:
            if (GetCapture() == hWnd)
            {
                /* Move the correct splitter */
                if (bSplit == 1)
                {
                    INT x = GET_X_LPARAM(lParam);

                    GetClientRect(hWnd, &rect);

                    x = min(max(x, SPLIT_WIDTH/2), rect.right - rect.left - SPLIT_WIDTH/2);
                    if (nVSplitPos != x)
                    {
                        nVSplitPos = x;
                        ResizeWnd(rect.right - rect.left, rect.bottom - rect.top);
                    }
                }
                else if (bSplit == 2)
                {
                    RECT rs;
                    INT y = GET_Y_LPARAM(lParam);

                    GetClientRect(hWnd, &rect);
                    GetWindowRect(hwndStatus, &rs);

                    y = min(max(y, SPLIT_WIDTH/2), rect.bottom - rect.top - SPLIT_WIDTH/2 - (rs.bottom - rs.top));
                    if (nHSplitPos != y)
                    {
                        nHSplitPos = y;
                        ResizeWnd(rect.right - rect.left, rect.bottom - rect.top);
                    }
                }
            }
            break;

        case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                SendMessageW(hwndStatus, WM_SIZE, 0, 0);
                ResizeWnd(LOWORD(lParam), HIWORD(lParam));
                break;
            }
            /* Fall through the default case */
        }

        default: Default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}


static
VOID
InitPropertiesDlg(HWND hDlg, PEVENTLOG EventLog)
{
    LPWSTR lpLogName = EventLog->LogName;

    DWORD Result, Type;
    DWORD dwMaxSize = 0, dwRetention = 0;
    BOOL Success;
    WIN32_FIND_DATAW FileInfo; // WIN32_FILE_ATTRIBUTE_DATA
    ULARGE_INTEGER FileSize;
    WCHAR wszBuf[MAX_PATH];
    WCHAR szTemp[MAX_LOADSTRING];
    LPWSTR FileName;

    HKEY hLogKey;
    WCHAR *KeyPath;
    DWORD cbData;
    SIZE_T cbKeyPath;

    if (EventLog->Permanent)
    {

    cbKeyPath = (wcslen(EVENTLOG_BASE_KEY) + wcslen(lpLogName) + 1) * sizeof(WCHAR);
    KeyPath = HeapAlloc(GetProcessHeap(), 0, cbKeyPath);
    if (!KeyPath)
    {
        goto Quit;
    }

    StringCbCopyW(KeyPath, cbKeyPath, EVENTLOG_BASE_KEY);
    StringCbCatW(KeyPath, cbKeyPath, lpLogName);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyPath, 0, KEY_QUERY_VALUE, &hLogKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, KeyPath);
        goto Quit;
    }
    HeapFree(GetProcessHeap(), 0, KeyPath);


    cbData = sizeof(dwMaxSize);
    Result = RegQueryValueExW(hLogKey,
                              L"MaxSize",
                              NULL,
                              &Type,
                              (LPBYTE)&dwMaxSize,
                              &cbData);
    if ((Result != ERROR_SUCCESS) || (Type != REG_DWORD))
    {
        // dwMaxSize = 512 * 1024; /* 512 kBytes */
        dwMaxSize = 0;
    }
    /* Convert in KB */
    dwMaxSize /= 1024;

    cbData = sizeof(dwRetention);
    Result = RegQueryValueExW(hLogKey,
                              L"Retention",
                              NULL,
                              &Type,
                              (LPBYTE)&dwRetention,
                              &cbData);
    if ((Result != ERROR_SUCCESS) || (Type != REG_DWORD))
    {
        /* On Windows 2003 it is 604800 (secs) == 7 days */
        dwRetention = 0;
    }
    /* Convert in days, rounded up */ // ROUND_UP
    // dwRetention = ROUND_UP(dwRetention, 24*3600) / (24*3600);
    dwRetention = (dwRetention + 24*3600 - 1) / (24*3600);


    RegCloseKey(hLogKey);

    }


Quit:

    SetDlgItemTextW(hDlg, IDC_DISPLAYNAME, lpLogName); // FIXME!
    SetDlgItemTextW(hDlg, IDC_LOGNAME, lpLogName);

    FileName = EventLog->FileName;
    if (FileName && *FileName)
    {
        ExpandEnvironmentStringsW(FileName, wszBuf, MAX_PATH);
        FileName = wszBuf;
    }
    SetDlgItemTextW(hDlg, IDC_LOGFILE, FileName);

    /*
     * The general problem here (and in the shell as well) is that
     * GetFileAttributesEx fails for files that are opened without
     * shared access. To retrieve file information for those we need
     * to use something else: FindFirstFile, on the full file name.
     */

    Success = GetFileAttributesExW(FileName,
                                   GetFileExInfoStandard,
                                   (LPWIN32_FILE_ATTRIBUTE_DATA)&FileInfo);
    if (!Success)
    {
        HANDLE hFind = FindFirstFileW(FileName, &FileInfo);
        Success = (hFind != INVALID_HANDLE_VALUE);
        if (Success)
            FindClose(hFind);
    }

    // Starting there, FileName is invalid (because it uses wszBuf)

    if (Success)
    {
        FileSize.u.LowPart = FileInfo.nFileSizeLow;
        FileSize.u.HighPart = FileInfo.nFileSizeHigh;
        if (FormatFileSizeWithBytes(&FileSize, wszBuf, ARRAYSIZE(wszBuf)))
            SetDlgItemTextW(hDlg, IDC_SIZE_LABEL, wszBuf);

        LoadStringW(hInst, IDS_NOT_AVAILABLE, szTemp, ARRAYSIZE(szTemp));

        if (GetFileTimeString(&FileInfo.ftCreationTime, wszBuf, ARRAYSIZE(wszBuf)))
            SetDlgItemTextW(hDlg, IDC_CREATED_LABEL, wszBuf);
        else
            SetDlgItemTextW(hDlg, IDC_CREATED_LABEL, szTemp);

        if (GetFileTimeString(&FileInfo.ftLastWriteTime, wszBuf, ARRAYSIZE(wszBuf)))
            SetDlgItemTextW(hDlg, IDC_MODIFIED_LABEL, wszBuf);
        else
            SetDlgItemTextW(hDlg, IDC_MODIFIED_LABEL, szTemp);

        if (GetFileTimeString(&FileInfo.ftLastAccessTime, wszBuf, ARRAYSIZE(wszBuf)))
            SetDlgItemTextW(hDlg, IDC_ACCESSED_LABEL, wszBuf);
        else
            SetDlgItemTextW(hDlg, IDC_MODIFIED_LABEL, szTemp);
    }
    else
    {
        LoadStringW(hInst, IDS_NOT_AVAILABLE, szTemp, ARRAYSIZE(szTemp));

        SetDlgItemTextW(hDlg, IDC_SIZE_LABEL, szTemp);
        SetDlgItemTextW(hDlg, IDC_CREATED_LABEL, szTemp);
        SetDlgItemTextW(hDlg, IDC_MODIFIED_LABEL, szTemp);
        SetDlgItemTextW(hDlg, IDC_ACCESSED_LABEL, szTemp);
    }

    if (EventLog->Permanent)
    {
        SendDlgItemMessageW(hDlg, IDC_UPDOWN_MAXLOGSIZE, UDM_SETRANGE32, (WPARAM)1, (LPARAM)0x3FFFC0);
        SendDlgItemMessageW(hDlg, IDC_UPDOWN_EVENTS_AGE, UDM_SETRANGE, 0, (LPARAM)MAKELONG(365, 1));

        SetDlgItemInt(hDlg, IDC_EDIT_MAXLOGSIZE, dwMaxSize, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_EVENTS_AGE, dwRetention, FALSE);

        if (dwRetention == 0)
        {
            CheckRadioButton(hDlg, IDC_OVERWRITE_AS_NEEDED, IDC_NO_OVERWRITE, IDC_OVERWRITE_AS_NEEDED);
            EnableDlgItem(hDlg, IDC_EDIT_EVENTS_AGE, FALSE);
            EnableDlgItem(hDlg, IDC_UPDOWN_EVENTS_AGE, FALSE);
        }
        else if (dwRetention == INFINITE)
        {
            CheckRadioButton(hDlg, IDC_OVERWRITE_AS_NEEDED, IDC_NO_OVERWRITE, IDC_NO_OVERWRITE);
            EnableDlgItem(hDlg, IDC_EDIT_EVENTS_AGE, FALSE);
            EnableDlgItem(hDlg, IDC_UPDOWN_EVENTS_AGE, FALSE);
        }
        else
        {
            CheckRadioButton(hDlg, IDC_OVERWRITE_AS_NEEDED, IDC_NO_OVERWRITE, IDC_OVERWRITE_OLDER_THAN);
            EnableDlgItem(hDlg, IDC_EDIT_EVENTS_AGE, TRUE);
            EnableDlgItem(hDlg, IDC_UPDOWN_EVENTS_AGE, TRUE);
        }
    }
    else
    {
        // TODO: Hide the unused controls! Or, just use another type of property sheet!
    }
}

/* Message handler for EventLog Properties dialog */
INT_PTR CALLBACK
EventLogPropProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PEVENTLOG EventLog;

    EventLog = (PEVENTLOG)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            EventLog = (PEVENTLOG)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)EventLog);

            InitPropertiesDlg(hDlg, EventLog);

            PropSheet_UnChanged(GetParent(hDlg), hDlg);
            return (INT_PTR)TRUE;
        }

        case WM_DESTROY:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;

                case IDC_OVERWRITE_AS_NEEDED:
                {
                    CheckRadioButton(hDlg, IDC_OVERWRITE_AS_NEEDED, IDC_NO_OVERWRITE, IDC_OVERWRITE_AS_NEEDED);
                    EnableDlgItem(hDlg, IDC_EDIT_EVENTS_AGE, FALSE);
                    EnableDlgItem(hDlg, IDC_UPDOWN_EVENTS_AGE, FALSE);
                    break;
                }

                case IDC_OVERWRITE_OLDER_THAN:
                {
                    CheckRadioButton(hDlg, IDC_OVERWRITE_AS_NEEDED, IDC_NO_OVERWRITE, IDC_OVERWRITE_OLDER_THAN);
                    EnableDlgItem(hDlg, IDC_EDIT_EVENTS_AGE, TRUE);
                    EnableDlgItem(hDlg, IDC_UPDOWN_EVENTS_AGE, TRUE);
                    break;
                }

                case IDC_NO_OVERWRITE:
                {
                    CheckRadioButton(hDlg, IDC_OVERWRITE_AS_NEEDED, IDC_NO_OVERWRITE, IDC_NO_OVERWRITE);
                    EnableDlgItem(hDlg, IDC_EDIT_EVENTS_AGE, FALSE);
                    EnableDlgItem(hDlg, IDC_UPDOWN_EVENTS_AGE, FALSE);
                    break;
                }

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

INT_PTR
EventLogProperties(HINSTANCE hInstance, HWND hWndParent, PEVENTLOGFILTER EventLogFilter)
{
    INT_PTR ret = 0;
    PROPSHEETHEADERW psh;
    PROPSHEETPAGEW   psp[1]; // 2

    /*
     * Bail out if there is no available filter, or if the filter
     * contains more than one log.
     */
    if (!EventLogFilter)
        return 0;

    EventLogFilter_AddRef(EventLogFilter);

    if (EventLogFilter->NumOfEventLogs > 1 ||
        EventLogFilter->EventLogs[0] == NULL)
    {
        goto Quit;
    }

    /* Header */
    psh.dwSize      = sizeof(psh);
    psh.dwFlags     = PSH_PROPSHEETPAGE /*| PSH_USEICONID */ | PSH_PROPTITLE | PSH_HASHELP /*| PSH_NOCONTEXTHELP */ /*| PSH_USECALLBACK */;
    psh.hInstance   = hInstance;
    psh.hwndParent  = hWndParent;
    // psh.pszIcon     = MAKEINTRESOURCEW(IDI_APPICON); // Disabled because it only sets the small icon; the big icon is a stretched version of the small one.
    psh.pszCaption  = EventLogFilter->EventLogs[0]->LogName;
    psh.nStartPage  = 0;
    psh.ppsp        = psp;
    psh.nPages      = ARRAYSIZE(psp);
    // psh.pfnCallback = PropSheetCallback;

    /* Log properties page */
    psp[0].dwSize      = sizeof(psp[0]);
    psp[0].dwFlags     = PSP_HASHELP;
    psp[0].hInstance   = hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCEW(IDD_LOGPROPERTIES_GENERAL);
    psp[0].pfnDlgProc  = EventLogPropProc;
    psp[0].lParam      = (LPARAM)EventLogFilter->EventLogs[0];

#if 0
    /* TODO: Log sources page */
    psp[1].dwSize      = sizeof(psp[1]);
    psp[1].dwFlags     = PSP_HASHELP;
    psp[1].hInstance   = hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCEW(IDD_GENERAL_PAGE);
    psp[1].pfnDlgProc  = GeneralPageWndProc;
    psp[0].lParam      = (LPARAM)EventLogFilter->EventLogs[0];
#endif

    /* Create the property sheet */
    ret = PropertySheetW(&psh);

Quit:
    EventLogFilter_Release(EventLogFilter);
    return ret;
}

/* Message handler for Event Details dialog */
static HWND hWndDetailsCtrl = NULL; // May go into the DWLP_USER
static HWND hWndGrip = NULL;
static INT cxMin, cyMin;    // In window coordinates
static INT cxOld, cyOld;    // In client coordinates

INT_PTR CALLBACK
EventDetails(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LONG_PTR dwStyle;
            INT sbVXSize, sbHYSize;
            RECT rcWnd, rect;

            hWndDetailsCtrl = CreateEventDetailsCtrl(hInst, hDlg, lParam);
            if (!hWndDetailsCtrl)
            {
                EndDialog(hDlg, 0);
                return (INT_PTR)TRUE;
            }

            /* Create a size grip if the dialog has a sizing border */
            GetClientRect(hDlg, &rcWnd);
            dwStyle  = GetWindowLongPtrW(hDlg, GWL_STYLE);
            sbVXSize = GetSystemMetrics(SM_CXVSCROLL);
            sbHYSize = GetSystemMetrics(SM_CYHSCROLL);
            if (dwStyle & WS_THICKFRAME /* == WS_SIZEBOX */)
            {
                hWndGrip = CreateWindowW(WC_SCROLLBARW,
                                         NULL,
                                         WS_CHILD | WS_VISIBLE | /**/ WS_CLIPSIBLINGS | /**/ SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
                                         rcWnd.right - sbVXSize,
                                         rcWnd.bottom - sbHYSize,
                                         sbVXSize, sbHYSize,
                                         hDlg,
                                         NULL,
                                         hInst,
                                         NULL);
            }

            // SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)hWndDetailsCtrl);

            /*
             * Compute the minimum window size (in window coordinates) by
             * adding the widths/heights of the "Help" and "Close" buttons,
             * together with the margins, and add some minimal spacing
             * between the buttons.
             */
            GetWindowRect(hDlg, &rcWnd);
            cxMin = cyMin = 0;

            GetWindowRect(GetDlgItem(hDlg, IDHELP), &rect);
            cxMin += (rect.right - rect.left) + (rect.left - rcWnd.left); // == (rect.right - rcWnd.left);
            cyMin += (rect.bottom - rect.top) + (rcWnd.bottom - rect.bottom); // == (rcWnd.bottom - rect.top);

            GetWindowRect(GetDlgItem(hDlg, IDOK), &rect);
            cxMin += (rect.right - rect.left) + (rcWnd.right - rect.right); // == (rcWnd.right - rect.left);
            cyMin += (rect.bottom - rect.top) + (rcWnd.bottom - rect.bottom); // == (rcWnd.bottom - rect.top);

            /*
             * Convert the window rect from window to client coordinates
             * in order to retrieve the sizes of the left and top margins,
             * and add some extra space.
             */
            MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rcWnd, sizeof(RECT)/sizeof(POINT));

            cxMin += -2*rcWnd.left;   // Minimal spacing between the buttons == 2 * left margin
            cyMin += -rcWnd.top + 12; // Add some space on top

            GetClientRect(hDlg, &rcWnd);
            cxOld = rcWnd.right - rcWnd.left;
            cyOld = rcWnd.bottom - rcWnd.top;

            /* Show event info on dialog control */
            SendMessageW(hWndDetailsCtrl, EVT_DISPLAY, 0, 0);

            // SetWindowPos(hWndDetailsCtrl, NULL,
                         // 0, 0,
                         // (rcWnd.right - rcWnd.left),
                         // (rcWnd.bottom - rcWnd.top),
                         // SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

            /*
             * Hide the placeholder static control and show the event details
             * control instead. Note that the placeholder is here so far just
             * to get the dimensions right in the dialog resource editor.
             * I plan to remove it and use a custom control with a suitable
             * window class for it, that would create the event details control
             * instead.
             */
            ShowWindow(GetDlgItem(hDlg, IDC_STATIC), SW_HIDE);
            ShowWindow(hWndDetailsCtrl, SW_SHOW);
            return (INT_PTR)TRUE;
        }

        case WM_DESTROY:
            if (IsWindow(hWndDetailsCtrl))
                DestroyWindow(hWndDetailsCtrl);
            hWndDetailsCtrl = NULL;
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
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

        case WM_SETCURSOR:
            if (((HWND)wParam == hWndGrip) && (LOWORD(lParam) == HTCLIENT))
            {
                SetCursor(LoadCursorW(NULL, IDC_SIZENWSE));
                SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, TRUE);
                return (INT_PTR)TRUE;
            }
            break;

        case WM_SIZING:
        {
            /* Forbid resizing the dialog smaller than its minimal size */
            PRECT dragRect = (PRECT)lParam;

            if ((wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_BOTTOMLEFT))
            {
                if (dragRect->right - dragRect->left < cxMin)
                    dragRect->left = dragRect->right - cxMin;
            }

            if ((wParam == WMSZ_RIGHT) || (wParam == WMSZ_TOPRIGHT) || (wParam == WMSZ_BOTTOMRIGHT))
            {
                if (dragRect->right - dragRect->left < cxMin)
                    dragRect->right = dragRect->left + cxMin;
            }

            if ((wParam == WMSZ_TOP) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_TOPRIGHT))
            {
                if (dragRect->bottom - dragRect->top < cyMin)
                    dragRect->top = dragRect->bottom - cyMin;
            }

            if ((wParam == WMSZ_BOTTOM) || (wParam == WMSZ_BOTTOMLEFT) || (wParam == WMSZ_BOTTOMRIGHT))
            {
                if (dragRect->bottom - dragRect->top < cyMin)
                    dragRect->bottom = dragRect->top + cyMin;
            }

            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, TRUE);
            return (INT_PTR)TRUE;
        }

        case WM_SIZE:
        {
            INT cx = LOWORD(lParam);
            INT cy = HIWORD(lParam);

            HDWP hdwp;
            HWND hItemWnd;
            RECT rect;

            hdwp = BeginDeferWindowPos(4);

            /* Resize the event details control window */

            hItemWnd = hWndDetailsCtrl;
            GetWindowRect(hItemWnd, &rect);
            MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

            if (hdwp)
                hdwp = DeferWindowPos(hdwp,
                                      hItemWnd,
                                      HWND_TOP,
                                      0, 0,
                                      (rect.right - rect.left) + (cx - cxOld),
                                      (rect.bottom - rect.top) + (cy - cyOld),
                                      SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

            /* Move the buttons */

            hItemWnd = GetDlgItem(hDlg, IDHELP);
            GetWindowRect(hItemWnd, &rect);
            MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

            if (hdwp)
                hdwp = DeferWindowPos(hdwp,
                                      hItemWnd,
                                      HWND_TOP,
                                      rect.left,
                                      rect.top + (cy - cyOld),
                                      0, 0,
                                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            hItemWnd = GetDlgItem(hDlg, IDOK);
            GetWindowRect(hItemWnd, &rect);
            MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

            if (hdwp)
                hdwp = DeferWindowPos(hdwp,
                                      hItemWnd,
                                      HWND_TOP,
                                      rect.left + (cx - cxOld),
                                      rect.top  + (cy - cyOld),
                                      0, 0,
                                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            /* Move the size grip */
            if (hWndGrip && hdwp)
            {
                GetWindowRect(hWndGrip, &rect);
                MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

                hdwp = DeferWindowPos(hdwp,
                                      hWndGrip,
                                      HWND_TOP,
                                      rect.left + (cx - cxOld),
                                      rect.top  + (cy - cyOld),
                                      0, 0,
                                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }

            if (hdwp)
                EndDeferWindowPos(hdwp);

            /* Hide the size grip if we are in maximized mode */
            if (hWndGrip)
                ShowWindow(hWndGrip, (wParam == SIZE_MAXIMIZED) ? SW_HIDE : SW_SHOW);

            cxOld = cx;
            cyOld = cy;

            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
            return (INT_PTR)TRUE;
        }
    }

    return (INT_PTR)FALSE;
}
