// Magnify.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Magnify.h"
#include "MagDlg.h"
#include <ole2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname);
static BOOL InitMyProcessDesktopAccess(VOID);
static HWINSTA origWinStation = NULL;
static HWINSTA userWinStation = NULL;
#define DESKTOP_ACCESSDENIED 0
#define DESKTOP_DEFAULT      1
#define DESKTOP_SCREENSAVER  2
#define DESKTOP_WINLOGON     3
#define DESKTOP_TESTDISPLAY  4
#define DESKTOP_OTHER        5

// For Link Window
EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;

/////////////////////////////////////////////////////////////////////////////
// CMagnifyApp

BEGIN_MESSAGE_MAP(CMagnifyApp, CWinApp)
	//{{AFX_MSG_MAP(CMagnifyApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
    
// YX [ 99-10-12
// Commenting out to prevent any problems with unwanted help
//	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
// ] YX
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMagnifyApp construction

CMagnifyApp::CMagnifyApp()
{
    DWORD desktopID;
    TCHAR name[300];
    InitMyProcessDesktopAccess();
    AssignDesktop(&desktopID,name);
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMagnifyApp object
// For UM


CMagnifyApp theApp;


/////////////////////////////////////////////////////////////////////////////
// CMagnifyApp initialization

BOOL CMagnifyApp::InitInstance()
{   
    

	SCODE sc = CoInitialize(NULL);

	if (FAILED(sc))
	{
		// warn about non-NULL success codes
		TRACE(__TEXT("Warning: OleInitialize failed"));
		return FALSE;
	}

	// Make sure we are not already running
	HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, __TEXT("MSMagnifierAlreadyExistsEvent"));
	if(!hEvent || ERROR_ALREADY_EXISTS == GetLastError())
	{
		CloseHandle(hEvent);
		return FALSE;
	}

	// for the Link Window in finish page...
	LinkWindow_RegisterClass();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

    // UM
	
    CMagnifyDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	CoUninitialize();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

/*LRESULT CMagnifyApp::OnDeskTopChanged(WPARAM wParam, LPARAM lParam) 
{
    ::MessageBox(NULL, NULL, NULL, MB_OK|MB_ICONSTOP);

    return 0;
}




int CMagnifyApp::Run() 
{
    MSG msg;

	// TODO: Add your specialized code here and/or call the base class
	while ((GetMessage(&msg,NULL,0,0)))//support UM: you need to get PostThreadMessage
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
        if (msg.message == uDeskMag)
	    {
            ASSERT(FALSE);
            ::MessageBox(NULL, NULL, NULL, MB_OK|MB_ICONSTOP);
        }
    }

    ASSERT(FALSE);

    return 0;
	
	//return CWinApp::Run();
}*/


// AssignDeskTop() For UM
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname)
{
    HDESK hdesk;
    TCHAR name[300];
    DWORD nl;
    *desktopID = DESKTOP_ACCESSDENIED;
    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return FALSE;
    }
    GetUserObjectInformation(hdesk,UOI_NAME,name,300,&nl);
    if (pname)
        _tcscpy(pname,name);
    if (!_tcsicmp(name, __TEXT("Default")))
        *desktopID = DESKTOP_DEFAULT;
    else if (!_tcsicmp(name, __TEXT("Winlogon")))
        *desktopID = DESKTOP_WINLOGON;
    else if (!_tcsicmp(name, __TEXT("screen-saver")))
        *desktopID = DESKTOP_SCREENSAVER;
    else if (!_tcsicmp(name, __TEXT("Display.Cpl Desktop")))
        *desktopID = DESKTOP_TESTDISPLAY;
    else
        *desktopID = DESKTOP_OTHER;
    CloseDesktop(GetThreadDesktop(GetCurrentThreadId()));
    SetThreadDesktop(hdesk);
    return TRUE;
}// AssignDesktop


static BOOL InitMyProcessDesktopAccess(VOID)
{
  origWinStation = GetProcessWindowStation();
  userWinStation = OpenWindowStation(_TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
  if (!userWinStation)
    return FALSE;
  SetProcessWindowStation(userWinStation);
  return TRUE;
}// InitMyProcessDesktopAccess
