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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT : See COPYING in the top level directory
 * PROJECT   : Event Log Viewer
 * FILE      : eventvwr.c
 * PROGRAMMER: Marc Piulachs (marc.piulachs at codexchange [dot] net)
 */

#include "eventvwr.h"
#include <windows.h>	// Standard windows include file
#include <commctrl.h>	// For ListView control APIs
#include <tchar.h>		// For TCHAR and string functions.
#include <stdio.h>
#include <time.h>

#if _MSC_VER
	#pragma warning(disable: 4996)   // 'strdup' was declared deprecated 
	#define _CRT_SECURE_NO_DEPRECATE // all deprecated 'unsafe string functions 
#endif

static const LPSTR EVENT_SOURCE_APPLICATION	= "Application";
static const LPSTR EVENT_SOURCE_SECURITY	= "Security";
static const LPSTR EVENT_SOURCE_SYSTEM		= "System";

//MessageFile message buffer size
#define EVENT_MESSAGE_FILE_BUFFER			1024*10      
#define EVENT_DLL_SEPARATOR		            ";" 
#define EVENT_MESSAGE_FILE					"EventMessageFile"
#define EVENT_CATEGORY_MESSAGE_FILE			"CategoryMessageFile"
#define EVENT_PARAMETER_MESSAGE_FILE		"ParameterMessageFile"

#define MAX_LOADSTRING 255

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Globals
HWND hwndMainWindow;         // Main window
HWND hwndListView;           // ListView control
HWND hwndStatus;			 // Status bar

LPTSTR lpSourceLogName	= NULL;
LPTSTR lpComputerName	= NULL;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc			(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About			(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	EventDetails	(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK StatusMessageWindowProc (HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	INITCOMMONCONTROLSEX iccx;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    // Whenever any of the common controls are used in your app,
    // you must call InitCommonControlsEx() to register the classes
    // for those controls.
    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccx);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_EVENTVWR, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EVENTVWR));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

VOID EventTimeToSystemTime (DWORD EventTime, SYSTEMTIME *pSystemTime)
{
	SYSTEMTIME st1970 = { 1970, 1, 0, 1, 0, 0, 0, 0 };
	FILETIME ftLocal;
	union {
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
TrimNulls ( LPSTR s ) 
{ 
    char *c; 

    if ( s != (char *) NULL ) 
    { 
        c = s + strlen ( s ) - 1; 
        while ( c >= s && isspace ( *c ) ) 
            --c; 
        *++c = '\0'; 
    } 
} 

BOOL GetEventMessageFileDLL(
	IN LPCTSTR lpLogName,
	IN LPCTSTR SourceName,
	IN LPCTSTR EntryName, 
	OUT LPSTR ExpandedName)
{
	DWORD dwSize;
	BYTE szModuleName[MAX_PATH];
	TCHAR szKeyName[MAX_PATH];
	HKEY hAppKey = NULL;
	HKEY hSourceKey = NULL;
	BOOL bReturn = FALSE; // Return

    _tcscpy(szKeyName, TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog"));
    _tcscat(szKeyName, _T("\\"));
    _tcscat(szKeyName, lpLogName);
	
	if (RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		szKeyName,
		0,
		KEY_READ,
		&hAppKey) == ERROR_SUCCESS) 
	{
		if (RegOpenKeyEx(
			hAppKey,
			SourceName,
			0,
			KEY_READ,
			&hSourceKey) == ERROR_SUCCESS)
		{
			dwSize = MAX_PATH;
			if (RegQueryValueEx(
				hSourceKey,
				EntryName,
				NULL,
				NULL,
				(LPBYTE)szModuleName,
				&dwSize) == ERROR_SUCCESS)
			{
				// Returns a string containing the requested substituted environment variable.
				ExpandEnvironmentStrings ((LPCTSTR)szModuleName, ExpandedName, MAX_PATH);

				// Succesfull
				bReturn = TRUE;
			}
		}
	}
	else
	{
		MessageBox (NULL ,
			_TEXT("Registry access failed!") , 
			_TEXT("Event Log") , 
			MB_OK | MB_ICONINFORMATION);
	}

	if (hSourceKey != NULL) 
		RegCloseKey(hSourceKey);
	
	if (hAppKey != NULL) 
		RegCloseKey(hAppKey);

	return bReturn;
}

BOOL GetEventCategory(
	IN LPCTSTR KeyName,
	IN LPCTSTR SourceName, 
	IN EVENTLOGRECORD *pevlr,
	OUT LPTSTR CategoryName)
{
	HANDLE hLibrary = NULL;
	TCHAR szMessageDLL[MAX_PATH];
	LPVOID lpMsgBuf = NULL;

	if(GetEventMessageFileDLL (KeyName, SourceName, EVENT_CATEGORY_MESSAGE_FILE , szMessageDLL))
	{
		hLibrary = LoadLibraryEx(
			szMessageDLL,
			NULL,
			DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);

		if(hLibrary != NULL) 
		{
			// Retrieve the message string. 
			if(FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				hLibrary,
				pevlr->EventCategory,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&lpMsgBuf,
				EVENT_MESSAGE_FILE_BUFFER,
				NULL) != 0)
			{
			if (lpMsgBuf)
			{
				//Trim the string
				TrimNulls ((LPSTR)lpMsgBuf);

				// Copy the category name
				strcpy (CategoryName, (LPCTSTR)lpMsgBuf);
			}
			else
			{
				strcpy (CategoryName, (LPCTSTR)lpMsgBuf);
			}
			}else{
				strcpy (CategoryName, "None");
			}

			if(hLibrary != NULL) 
				FreeLibrary(hLibrary);

			//  Free the buffer allocated by FormatMessage
			if (lpMsgBuf)
				LocalFree(lpMsgBuf);

			return TRUE;
		}
	}

	strcpy (CategoryName, "None");

	return FALSE;
}

BOOL GetEventMessage(
	IN LPCTSTR KeyName,
	IN LPCTSTR SourceName, 
	IN EVENTLOGRECORD *pevlr,
	OUT LPTSTR EventText)
{
	DWORD i;
	HANDLE hLibrary = NULL;
	char SourceModuleName[1000];
	char ParameterModuleName[1000];
	LPTSTR lpMsgBuf = NULL;
	TCHAR szStringIDNotFound[MAX_LOADSTRING];
	LPTSTR szDll;
	LPTSTR szMessage;
	LPTSTR *szArguments;
	BOOL bDone = FALSE;

	/* TODO : GetEventMessageFileDLL can return a comma separated list of DLLs */
	if (GetEventMessageFileDLL (KeyName , SourceName, EVENT_MESSAGE_FILE , SourceModuleName))
	{
		// Get the event message
		szMessage = (LPTSTR)((LPBYTE)pevlr + pevlr->StringOffset);

		// Allocate space for parameters
		szArguments = (LPTSTR*)malloc(sizeof(LPVOID)* pevlr->NumStrings);

		for (i = 0; i < pevlr->NumStrings ; i++)
		{
			if (strstr(szMessage , "%%")) 
			{
				if (GetEventMessageFileDLL (KeyName , SourceName, EVENT_PARAMETER_MESSAGE_FILE , ParameterModuleName))
				{
					//Not yet support for reading messages from parameter message DLL
				}

				szArguments[i] = szMessage;
				szMessage += strlen(szMessage) + 1;
			}
			else
			{
				szArguments[i] = szMessage;
				szMessage += strlen(szMessage) + 1;
			}
		}

		szDll = strtok(SourceModuleName, EVENT_DLL_SEPARATOR);
		while ((szDll != NULL) && (!bDone))
		{ 
			hLibrary = LoadLibraryEx(
				szDll,
				NULL,
				DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);

			if (hLibrary == NULL) 
			{
				// The DLL could not be loaded try the next one (if any)
				szDll = strtok (NULL, EVENT_DLL_SEPARATOR);
			}
			else
			{
				// Retrieve the message string. 
				if(FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_HMODULE | 
					FORMAT_MESSAGE_ARGUMENT_ARRAY,
					hLibrary,
					pevlr->EventID,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0,
					szArguments) == 0) 
				{
					// We haven't found the string , get next DLL (if any)
					szDll = strtok (NULL, EVENT_DLL_SEPARATOR); 
				}
				else
				{	
					if (lpMsgBuf)
					{
						// The ID was found and the message was formated
						bDone = TRUE;

						//Trim the string
						TrimNulls ((LPSTR)lpMsgBuf);

						// Copy the event text
						strcpy (EventText ,lpMsgBuf);
					}
				}

				FreeLibrary (hLibrary);
			}
		}

		if (!bDone)
		{
			LoadString(hInst, IDC_EVENTSTRINGIDNOTFOUND, szStringIDNotFound, MAX_LOADSTRING);
			wsprintf (EventText, szStringIDNotFound , (DWORD)(pevlr->EventID & 0xFFFF) , SourceName );
		}

		// No more dlls to try , return result
		return bDone;
	}

	LoadString(hInst, IDC_EVENTSTRINGIDNOTFOUND, szStringIDNotFound, MAX_LOADSTRING);
	wsprintf (EventText, szStringIDNotFound , (DWORD)(pevlr->EventID & 0xFFFF) , SourceName );

	return FALSE;
}

char* GetEventType (WORD dwEventType)
{
    switch(dwEventType)
    {
        case EVENTLOG_ERROR_TYPE:
            return "Error";
            break;
        case EVENTLOG_WARNING_TYPE:
            return "Warning";
            break;
        case EVENTLOG_INFORMATION_TYPE:
            return "Information";
            break;
        case EVENTLOG_AUDIT_SUCCESS:
            return "Audit Success";
            break;
        case EVENTLOG_AUDIT_FAILURE:
            return "Audit Failure";
            break;
        default:
            return "Unknown Event";
            break;
    }
}

BOOL 
GetEventUserName (EVENTLOGRECORD *pelr, OUT LPSTR pszUser)
{
    PSID lpSid;
	char szName[1024];
	char szDomain[1024];
    SID_NAME_USE peUse;
    DWORD cbName = 1024;
    DWORD cbDomain = 1024;

    // Point to the SID. 
    lpSid = (PSID)((LPBYTE) pelr + pelr->UserSidOffset); 

	 // User SID
	if(pelr->UserSidLength > 0)
	{
		if (LookupAccountSid(
				NULL, 
				lpSid, 
				szName, 
				&cbName, 
				szDomain, 
				&cbDomain, 
				&peUse))
		{
			strcpy (pszUser , szName);
			return TRUE;
		}
	}

    return FALSE;
}

static DWORD WINAPI
ShowStatusMessageThread(
    IN LPVOID lpParameter)
{
    HWND *phWnd = (HWND *)lpParameter;
    HWND hWnd;
    MSG Msg;

    hWnd = CreateDialogParam(
        hInst,
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

VOID QueryEventMessages (
	LPTSTR lpMachineName , 
	LPTSTR lpLogName)
{
	HWND hwndDlg;
    HANDLE hEventLog;
    EVENTLOGRECORD *pevlr;
    BYTE bBuffer[MAX_PATH];
    DWORD dwRead, dwNeeded, dwThisRecord , dwTotalRecords , dwCurrentRecord = 1 , dwRecordsToRead = 0 , dwFlags;
    LPSTR lpSourceName;
	LPSTR lpComputerName;
	LPSTR lpEventStr;
	LPSTR lpData;
	BOOL bResult = TRUE; // Read succeeded.

	char szWindowTitle[MAX_PATH];
	char szStatusText[MAX_PATH];
	char szLocalDate[MAX_PATH];
	char szLocalTime[MAX_PATH];
	char szEventID[MAX_PATH];
	char szCategoryID[MAX_PATH];
	char szUsername[MAX_PATH];
	char szEventText[EVENT_MESSAGE_FILE_BUFFER];
	char szCategory[MAX_PATH];

	SYSTEMTIME time;
	LVITEM lviEventItem;

	dwFlags = EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ;

	lpSourceLogName = lpLogName;
	lpComputerName = lpMachineName;

    // Open the event log.
    hEventLog = OpenEventLog( 
					lpMachineName,
					lpLogName);

    if (hEventLog == NULL)
    {
		MessageBox (NULL ,
			_TEXT("Could not open the event log.") ,
			_TEXT("Event Log") , 
			MB_OK | MB_ICONINFORMATION);
        return;
    }

	//Disable listview redraw
	SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);

	// Clear the list view
	(void)ListView_DeleteAllItems (hwndListView);
    
    // Initialize the event record buffer.
    pevlr = (EVENTLOGRECORD *)&bBuffer;

    // Get the record number of the oldest event log record.
    GetOldestEventLogRecord(hEventLog, &dwThisRecord);

	// Get the total number of event log records.
	GetNumberOfEventLogRecords (hEventLog , &dwTotalRecords);

	//If we have at least 1000 records show the waiting dialog
	if (dwTotalRecords > 1000)
	{
		CreateThread(
			NULL,
			0,
			ShowStatusMessageThread,
			(LPVOID)&hwndDlg,
			0,
			NULL);
	}

    while (dwCurrentRecord < dwTotalRecords)
    {
		pevlr = (EVENTLOGRECORD*)malloc(MAX_PATH);
              
		bResult = ReadEventLog(
			hEventLog,						// Event log handle
			dwFlags,						// Sequential read
			0,                              // Ignored for sequential read
			pevlr,                          // Pointer to buffer
			MAX_PATH,                    // Size of buffer
			&dwRead,                        // Number of bytes read
			&dwNeeded);						// Bytes in the next record

    	if((!bResult) && (GetLastError () == ERROR_INSUFFICIENT_BUFFER))
    	{
			pevlr = (EVENTLOGRECORD*)malloc (dwNeeded);

    		ReadEventLog(
				hEventLog,			// event log handle
                dwFlags,            // read flags
                0,					// offset; default is 0
                pevlr,              // pointer to buffer
                dwNeeded,           // size of buffer
                &dwRead,            // number of bytes read
                &dwNeeded);         // bytes in next record
    	}

        while (dwRead > 0)
        {
			strcpy (szUsername , "N/A");
			strcpy (szEventText , "N/A");
			strcpy (szCategory , "None");

            // Get the event source name.
            lpSourceName = (LPSTR) ((LPBYTE) pevlr + sizeof(EVENTLOGRECORD));

			// Get the computer name
			lpComputerName = (LPSTR) ((LPBYTE) pevlr + sizeof(EVENTLOGRECORD) + lstrlen(lpSourceName) + 1);

			// This ist the data section of the current event 
			lpData = (LPSTR) ((LPBYTE)pevlr + pevlr->DataOffset);

			// This is the text of the current event
			lpEventStr = (LPSTR) ((LPBYTE) pevlr + pevlr->StringOffset);

			// Compute the event type
			EventTimeToSystemTime(pevlr->TimeWritten, &time);  

			// Get the username that generated the event
			GetEventUserName (pevlr , szUsername);

			GetDateFormat( LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, szLocalDate, MAX_PATH );
			GetTimeFormat( LOCALE_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, szLocalTime, MAX_PATH );

			GetEventCategory (lpLogName , lpSourceName , pevlr , szCategory);
			//GetEventMessage (lpLogName , lpSourceName , pevlr , szEventText);

			wsprintf (szEventID, "%u", (DWORD)(pevlr->EventID & 0xFFFF));
			wsprintf (szCategoryID, "%u", (DWORD)(pevlr->EventCategory));

			lviEventItem.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM; 
			lviEventItem.iItem = 0;
			lviEventItem.iSubItem = 0;
			lviEventItem.lParam = (LPARAM)pevlr;

			switch(pevlr->EventType) 
			{
				case EVENTLOG_ERROR_TYPE: 
					lviEventItem.pszText = "Error";
					lviEventItem.iImage = 2;
					break;
				case EVENTLOG_AUDIT_FAILURE:
					lviEventItem.pszText = "Audit Failure";
					lviEventItem.iImage = 2;
					break;
				case EVENTLOG_WARNING_TYPE:
					lviEventItem.pszText = "Warning";
					lviEventItem.iImage = 1;
					break;
				case EVENTLOG_INFORMATION_TYPE:
					lviEventItem.pszText = "Information";
					lviEventItem.iImage = 0;
					break;
				case EVENTLOG_AUDIT_SUCCESS:
					lviEventItem.pszText = "Audit Success";
					lviEventItem.iImage = 0;
					break;
				case EVENTLOG_SUCCESS:
					lviEventItem.pszText = "Success";
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
			ListView_SetItemText(hwndListView, lviEventItem.iItem, 8, lpData); //Event Text
        
            dwRead -= pevlr->Length;
            pevlr = (EVENTLOGRECORD *) ((LPBYTE) pevlr + pevlr->Length);
        }

		dwRecordsToRead--;
		dwCurrentRecord++;

        pevlr = (EVENTLOGRECORD *) &bBuffer;
    }

	//All events loaded
	EndDialog(hwndDlg, 0);

	wsprintf (szWindowTitle, "%s - %s Log on \\\\%s", szTitle , lpLogName , lpComputerName);
	wsprintf (szStatusText, "%s has %d event(s)",  lpLogName , dwTotalRecords);

	//Update the status bar
	SendMessage (hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)szStatusText);

	//Set the window title
	SetWindowText ( hwndMainWindow , szWindowTitle);

	//Resume list view redraw
	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
   
    // Close the event log.
    CloseEventLog(hEventLog);
}

VOID 
Refresh (VOID)
{
	QueryEventMessages(
		lpComputerName , 
		lpSourceLogName);
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EVENTVWR));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_EVENTVWR);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HIMAGELIST hSmall;
	LVCOLUMN lvc = {0};

	hInst = hInstance; // Store instance handle in our global variable

	hwndMainWindow = CreateWindow(
		szWindowClass, 
		szTitle, 
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
		NULL, 
		NULL, 
		hInstance, 
		NULL);

	if (!hwndMainWindow)
	{
		return FALSE;
	}

	hwndStatus = CreateWindowEx( 
            0,									// no extended styles
            STATUSCLASSNAME,					// status bar
            "Done.",                            // no text 
            WS_CHILD | WS_BORDER | WS_VISIBLE,  // styles
            0, 0, 0, 0,							// x, y, cx, cy
            hwndMainWindow,                     // parent window
            (HMENU)100,							// window ID
            hInstance,                          // instance
            NULL);								// window data

    // Create our listview child window.  Note that I use WS_EX_CLIENTEDGE
    // and WS_BORDER to create the normal "sunken" look.  Also note that
    // LVS_EX_ styles cannot be set in CreateWindowEx().
    hwndListView = CreateWindowEx(
		WS_EX_CLIENTEDGE, 
		WC_LISTVIEW, 
		_T(""),
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
	hSmall = ImageList_Create(
		GetSystemMetrics(SM_CXSMICON),
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
	lvc.pszText = _T("Type");
    (void)ListView_InsertColumn(hwndListView, 0, &lvc);

	lvc.cx = 70;
	lvc.pszText = _T("Date");
    (void)ListView_InsertColumn(hwndListView, 1, &lvc);

	lvc.cx = 70;
    lvc.pszText = _T("Time");
    (void)ListView_InsertColumn(hwndListView, 2, &lvc);

	lvc.cx = 150;
	lvc.pszText = _T("Source");
    (void)ListView_InsertColumn(hwndListView, 3, &lvc);

	lvc.cx = 100;
	lvc.pszText = _T("Category");
    (void)ListView_InsertColumn(hwndListView, 4, &lvc);

	lvc.cx = 60;
	lvc.pszText = _T("Event");
    (void)ListView_InsertColumn(hwndListView, 5, &lvc);

	lvc.cx = 120;
	lvc.pszText = _T("User");
    (void)ListView_InsertColumn(hwndListView, 6, &lvc);

	lvc.cx = 100;
	lvc.pszText = _T("Computer");
    (void)ListView_InsertColumn(hwndListView, 7, &lvc);

	lvc.cx = 0;
	lvc.pszText = _T("Event Data");
    (void)ListView_InsertColumn(hwndListView, 8, &lvc);

	ShowWindow(hwndMainWindow, nCmdShow);
	UpdateWindow(hwndMainWindow);

	QueryEventMessages (
		lpComputerName,				// Use the local computer.
		EVENT_SOURCE_APPLICATION);	// The event log category

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	RECT rect;
	NMHDR       *hdr;
	HMENU       hMenu ;

	switch (message)
	{
		case WM_NOTIFY :
			switch(((LPNMHDR)lParam)->code)
			{
				case NM_DBLCLK :
					hdr = (NMHDR FAR*)lParam;
					if(hdr->hwndFrom == hwndListView)
					{
						LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;

						if(lpnmitem->iItem != -1)
						{
							DialogBox (hInst, MAKEINTRESOURCE(IDD_EVENTDETAILDIALOG), hWnd, EventDetails);
						}
					}
					break;
			}
		 break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		if ((wmId == ID_LOG_APPLICATION) ||
			(wmId == ID_LOG_SYSTEM) ||
			(wmId == ID_LOG_SECURITY))
		{
			hMenu = GetMenu (hWnd) ; // get the menu handle. Use it below

			CheckMenuItem (hMenu, ID_LOG_APPLICATION , MF_UNCHECKED) ;
			CheckMenuItem (hMenu, ID_LOG_SYSTEM , MF_UNCHECKED) ;
			CheckMenuItem (hMenu, ID_LOG_SECURITY , MF_UNCHECKED) ;

			if (hMenu)
			{
				CheckMenuItem (hMenu, wmId , MF_CHECKED) ;
			}
		}

		// Parse the menu selections:
		switch (wmId)
		{
			case ID_LOG_APPLICATION:
				QueryEventMessages (
					lpComputerName,				// Use the local computer.
					EVENT_SOURCE_APPLICATION);	// The event log category
				break;
			case ID_LOG_SYSTEM:
				QueryEventMessages (
					lpComputerName,				// Use the local computer.
					EVENT_SOURCE_SYSTEM);		// The event log category
				break;
			case ID_LOG_SECURITY:
				QueryEventMessages (
					lpComputerName,				// Use the local computer.
					EVENT_SOURCE_SECURITY);		// The event log category
				break;
			case IDM_REFRESH:
				Refresh ();
				break;
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_HELP:
				MessageBox (
					NULL ,
					_TEXT("Help not implemented yet!") , 
					_TEXT("Event Log") , 
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
		{
			//Gets the window rectangle
			GetClientRect(hWnd, &rect);
		
			//Relocate the listview
			MoveWindow(
				hwndListView,
				0, 
				0, 
				rect.right, 
				rect.bottom - 20, 
				1);

			// Resize the statusbar;
			SendMessage (hwndStatus, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
DisplayEvent (HWND hDlg)
{
	char szEventType[MAX_PATH];
	char szTime[MAX_PATH];
	char szDate[MAX_PATH];
	char szUser[MAX_PATH];
	char szComputer[MAX_PATH];
	char szSource[MAX_PATH];
	char szCategory[MAX_PATH];
	char szEventID[MAX_PATH];
	char szEventText[MAX_PATH*10];
	char szEventData[MAX_PATH];
	BOOL bEventData = FALSE;
	LVITEM li;
	EVENTLOGRECORD* pevlr;

	// Get index of selected item
	int iIndex = (int)SendMessage (hwndListView ,LVM_GETNEXTITEM, -1 , LVNI_SELECTED | LVNI_FOCUSED);

    li.mask = LVIF_PARAM;
    li.iItem = iIndex;
    li.iSubItem = 0;

	ListView_GetItem(hwndListView, &li);

	pevlr = (EVENTLOGRECORD*)li.lParam;

	if (iIndex != -1)
	{
		ListView_GetItemText (hwndListView , iIndex , 0 , szEventType , sizeof( szEventType ));
		ListView_GetItemText (hwndListView , iIndex , 1 , szDate , sizeof( szDate ));
		ListView_GetItemText (hwndListView , iIndex , 2 , szTime , sizeof( szTime ));
		ListView_GetItemText (hwndListView , iIndex , 3 , szSource , sizeof( szSource ));
		ListView_GetItemText (hwndListView , iIndex , 4 , szCategory , sizeof( szCategory ));
		ListView_GetItemText (hwndListView , iIndex , 5 , szEventID , sizeof( szEventID ));
		ListView_GetItemText (hwndListView , iIndex , 6 , szUser , sizeof( szUser ));
		ListView_GetItemText (hwndListView , iIndex , 7 , szComputer , sizeof( szComputer ));
		ListView_GetItemText (hwndListView , iIndex , 8 , szEventData , sizeof( szEventData ));

		bEventData = !(strlen(szEventData) == 0);

		GetEventMessage (lpSourceLogName , szSource , pevlr , szEventText);

		EnableWindow(GetDlgItem(hDlg , IDC_BYTESRADIO) , bEventData);
		EnableWindow(GetDlgItem(hDlg , IDC_WORDRADIO) , bEventData);

		SetDlgItemText (hDlg, IDC_EVENTDATESTATIC , szDate);
		SetDlgItemText (hDlg, IDC_EVENTTIMESTATIC , szTime);
		SetDlgItemText (hDlg, IDC_EVENTUSERSTATIC , szUser);
		SetDlgItemText (hDlg, IDC_EVENTSOURCESTATIC , szSource);
		SetDlgItemText (hDlg, IDC_EVENTCOMPUTERSTATIC , szComputer);
		SetDlgItemText (hDlg, IDC_EVENTCATEGORYSTATIC , szCategory);
		SetDlgItemText (hDlg, IDC_EVENTIDSTATIC , szEventID);
		SetDlgItemText (hDlg, IDC_EVENTTYPESTATIC , szEventType);
		SetDlgItemText (hDlg, IDC_EVENTTEXTEDIT , szEventText);
		SetDlgItemText (hDlg, IDC_EVENTDATAEDIT , szEventData);
	}
	else
	{
		MessageBox(
			NULL,
			"No Items in ListView",
			"Error",
			MB_OK | MB_ICONINFORMATION);
	}
}

static 
INT_PTR CALLBACK StatusMessageWindowProc(
    IN HWND hwndDlg,
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

// Message handler for event details box.
INT_PTR CALLBACK EventDetails(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
			{
				// Show event info on dialog box
				DisplayEvent (hDlg);
				return (INT_PTR)TRUE;
			}

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			if (LOWORD(wParam) == IDPREVIOUS)
			{
				SendMessage (hwndListView, WM_KEYDOWN, VK_UP, 0);

				// Show event info on dialog box
				DisplayEvent (hDlg);
				return (INT_PTR)TRUE;
			}

			if (LOWORD(wParam) == IDNEXT)
			{
				SendMessage (hwndListView, WM_KEYDOWN, VK_DOWN, 0);

				// Show event info on dialog box
				DisplayEvent (hDlg);
				return (INT_PTR)TRUE;
			}

			if (LOWORD(wParam) == IDC_BYTESRADIO)
			{
				return (INT_PTR)TRUE;
			}

			if (LOWORD(wParam) == IDC_WORDRADIO)
			{
				return (INT_PTR)TRUE;
			}

			if (LOWORD(wParam) == IDHELP)
			{
				MessageBox (NULL ,
					_TEXT("Help not implemented yet!") , 
					_TEXT("Event Log") , 
					MB_OK | MB_ICONINFORMATION);
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}
