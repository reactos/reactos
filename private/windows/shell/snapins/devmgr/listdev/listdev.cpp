// ListDev.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ListDev.h"
#include "devtree.h"
#include "ListDevDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListDevApp

BEGIN_MESSAGE_MAP(CListDevApp, CWinApp)
	//{{AFX_MSG_MAP(CListDevApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListDevApp construction

CListDevApp::CListDevApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CListDevApp object

CString     g_strStartupComputerName;
CListDevApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CListDevApp initialization

BOOL CListDevApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CMyCommandLineInfo  CmdLineInfo;
	ParseCommandLine(CmdLineInfo);
	if (CmdLineInfo.m_DisplayUsage)
	{
	    ::MessageBox(NULL, _T("Usage: ListDev [/computername <\\\\name>]"), _T("List Device"), MB_OK | MB_ICONINFORMATION);
	    return FALSE;
	}
	CListDevDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
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

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


const TCHAR*	COMPUTERNAME = _T("ComputerName");

void
CMyCommandLineInfo::ParseParam(
    LPCTSTR lpszParam,
    BOOL bFlag,
    BOOL bLast
    )
{
    if (bFlag)
    {
	if (!lstrcmpi(lpszParam, COMPUTERNAME))
	    m_WaitForName = TRUE;
	else if (_T('?') == *lpszParam)
	{
	    m_DisplayUsage = TRUE;
	}
    }
    else if (m_WaitForName)
    {
	g_strStartupComputerName = lpszParam;
	TCHAR Dbg[MAX_PATH];
	wsprintf(Dbg, _T("ComputerName = %s\n"), lpszParam);
	OutputDebugString(Dbg);
	m_WaitForName = FALSE;
    }
}
