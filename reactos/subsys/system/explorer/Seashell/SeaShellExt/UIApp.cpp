//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#include "stdafx.h"
#include "UIFrameWnd.h"
#include "UIApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIApp

IMPLEMENT_DYNAMIC(CUIApp,CWinApp)

BEGIN_MESSAGE_MAP(CUIApp, CWinApp)
	//{{AFX_MSG_MAP(CUIApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIApp construction

CUIApp::CUIApp()  
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_IDMyIcon = 0;
	m_bMyClassRegistered = false;
}

CDocTemplate *CUIApp::GetFirstDocTemplate()
{
	POSITION posTempl = GetFirstDocTemplatePosition();
	CDocTemplate *pTempl = GetNextDocTemplate(posTempl);
	ASSERT(pTempl);
	return pTempl;
}

CFrameWnd *CUIApp::GetMainFrame(CWnd *pWnd)
{
	if (pWnd)
		return pWnd->GetParentFrame();
	return (CFrameWnd*)m_pMainWnd;
}

CString CUIApp::GetRegAppKey()
{
	CString sKey;
	sKey += _T("Software");
	sKey += _T("\\");
	sKey += m_pszRegistryKey;
	sKey += _T("\\");
	sKey += m_pszProfileName;
	sKey += _T("\\");
	return sKey;
}

BOOL CUIApp::InitInstance()
{	
	if (!m_sMyClassName.IsEmpty() && !RegisterMyClass())
		return FALSE;
	return CWinApp::InitInstance();
}

int CUIApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
    if(m_bMyClassRegistered)
       ::UnregisterClass(m_sMyClassName,AfxGetInstanceHandle());

	return CWinApp::ExitInstance();
}
                                                                      
/////////////////////////////////////////////////////////////////////////////
// The one and only CUIApp object
bool CUIApp::RegisterMyClass()
{
	// Register our unique class name so only one instance can be started
	//////////////////////////////////////////////////////////////
	ASSERT(!m_sMyClassName.IsEmpty());
	if (m_sMyClassName.IsEmpty())
		return false;
	//////////////////////////////////////////////////////////////
    WNDCLASSEX wndcls; 
    memset(&wndcls, 0, sizeof(WNDCLASSEX));    
	wndcls.cbSize = sizeof(WNDCLASSEX);
    wndcls.style = CS_DBLCLKS;// | CS_HREDRAW | CS_VREDRAW;
    wndcls.lpfnWndProc = ::DefWindowProc;
    wndcls.hInstance = AfxGetInstanceHandle();
	if (m_IDMyIcon)
	{
		wndcls.hIcon = LoadIcon(m_IDMyIcon); 
		wndcls.hIconSm = (HICON)LoadImage(AfxGetInstanceHandle(), 
            MAKEINTRESOURCE(m_IDMyIcon),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            0);
			if (wndcls.hIconSm == NULL)
			{
				ErrorMessage(_T("LoadImage failed"));
			}
	}
	wndcls.hCursor = LoadStandardCursor(IDC_ARROW);
    wndcls.hbrBackground = NULL; //(HBRUSH) (COLOR_WINDOW + 1);
    wndcls.lpszMenuName = NULL; 
    // Specify our own class name for using FindWindow later to set focus to running app
    wndcls.lpszClassName = m_sMyClassName; 
	WNDCLASSEX wndclsEx;
	if (GetClassInfoEx(wndcls.hInstance, wndcls.lpszClassName, &wndclsEx))
	{
		// class already registered
		return true;
	}
    // Register new class and exit if it fails
    if(!::RegisterClassEx(&wndcls))
    {
		ErrorMessage(_T("Class Registration Failed"));
		return false;
    }
	else
		TRACE(_T("Window class %s was registered in CUIApp\n"),GetMyClass());
	m_bMyClassRegistered = true;
	return true;
}

BOOL CUIApp::FirstInstance()
{
	ASSERT(!m_sMyClassName.IsEmpty());
	if (m_sMyClassName.IsEmpty())
		return TRUE;

	CWnd *pWndPrev, *pWndChild;
	// Determine if another window with our class name exists...
	if (pWndPrev = CWnd::FindWindow(m_sMyClassName,NULL))
	{
		// if so, does it have any popups?
		pWndChild = pWndPrev->GetLastActivePopup();

		// If iconic, restore the main window
		if (pWndPrev->IsIconic())
		   pWndPrev->ShowWindow(SW_RESTORE);

		// Bring the main window or its popup to
		// the foreground
		pWndChild->SetForegroundWindow();

		// and we are done activating the previous one.
		return FALSE;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CUIApp initialization
void CUIApp::SetStatusBarText(UINT nPaneID,LPCTSTR Text,UINT nIconID)
{
	if (m_pMainWnd == NULL)
	{
		TRACE1("Warning: '%s' SetStatusBarText called before main frame created!\n",Text);
		return;
	}
	CUIFrameWnd *pMainFrame = (CUIFrameWnd*)m_pMainWnd;
	if (nIconID)
		pMainFrame->GetStatusBar().AddIcon(nPaneID,nIconID,false);
	pMainFrame->GetStatusBar().SetText(nPaneID,Text,true);     
}

void CUIApp::SetStatusBarText(LPCTSTR Text)
{
	SetStatusBarText(0,Text);
}

void CUIApp::SetStatusBarText(UINT nPaneID,LPCTSTR Text,CView *pView,UINT nIconID)
{
	if (m_pMainWnd == NULL)
		return;
	if (((CFrameWnd*)m_pMainWnd)->GetActiveView() == pView)
		SetStatusBarText(nPaneID,Text,nIconID);
}

void CUIApp::SetStatusBarText(UINT nPaneID,UINT nResID,UINT nIconID)
{
	CString str;
	str.LoadString(nResID);
	SetStatusBarText(nPaneID,str,nIconID);
}

void CUIApp::SetStatusBarIcon(UINT nPaneID,UINT nIconID,bool bAdd)
{
	if (m_pMainWnd == NULL)
		return;
	CUIFrameWnd *pMainFrame = (CUIFrameWnd*)m_pMainWnd;
	if (bAdd)
		pMainFrame->GetStatusBar().AddIcon(nPaneID,nIconID);
	else
		pMainFrame->GetStatusBar().RemoveIcon(nPaneID,nIconID);
	pMainFrame->GetStatusBar().RedrawWindow();
}

void CUIApp::SetStatusBarIdleMessage()
{
	if (m_pMainWnd == NULL)
		return;
	CUIFrameWnd *pMainFrame = (CUIFrameWnd*)m_pMainWnd;
	pMainFrame->GetStatusBar().RemoveAllIcons(0);
	SetStatusBarText(0,AFX_IDS_IDLEMESSAGE);
}

CDocument *CUIApp::GetDocument()
{
	CDocTemplate *pTempl = GetFirstDocTemplate();
	POSITION posDoc = pTempl->GetFirstDocPosition();
	CDocument *pDoc = pTempl->GetNextDoc(posDoc);
	ASSERT(pDoc);
	return pDoc; 
}

CView *CUIApp::GetView(CRuntimeClass *pClass)
{
	CDocTemplate *pTemplate = GetFirstDocTemplate();
	POSITION posDoc = pTemplate->GetFirstDocPosition();
	while (posDoc)
	{
		  CDocument *pDoc = pTemplate->GetNextDoc(posDoc); 
		  POSITION posView = pDoc->GetFirstViewPosition();
		  while (posView) 
		  {
				CView *pView = pDoc->GetNextView(posView); 
				if (pView && pView->IsKindOf(pClass))
					return pView;
		  }
    }
	return NULL;
}

void CUIApp::ChangeProfile(LPCTSTR szRegistryKey,LPCTSTR szProfileName)
{
	m_strOldProfileName = m_pszProfileName;
	m_strOldRegistryKey = m_pszRegistryKey;
	free((void*)m_pszRegistryKey);
	m_pszRegistryKey = _tcsdup(szRegistryKey);
	free((void*)m_pszProfileName);
	m_pszProfileName = _tcsdup(szProfileName);
}

void CUIApp::RestoreProfile()
{
	if (!m_strOldProfileName.IsEmpty())
	{
		free((void*)m_pszProfileName);
		m_pszProfileName = _tcsdup(m_strOldProfileName);
	}
	if (!m_strOldRegistryKey.IsEmpty()) 
	{
		free((void*)m_pszRegistryKey);
		m_pszRegistryKey = _tcsdup(m_strOldRegistryKey);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAppReg
// Tricks MFC into using another application key 
// useful if in HKEY_CURRENT_USER
CAppReg::CAppReg(CWinApp *pApp,LPCTSTR szRegistryKey,LPCTSTR szProfileName) 
{
	ASSERT(pApp);
	m_pApp = (CUIApp*)pApp;
	m_pApp->ChangeProfile(szRegistryKey,szProfileName);
}

CAppReg::~CAppReg()
{
	ASSERT(m_pApp);
	m_pApp->RestoreProfile();
}

//static 
bool CUIApp::COMMessage(HRESULT hr,UINT nID)
{
	CString sMess;
	sMess.LoadString(nID);
	return COMMessage(hr,sMess);
}

bool CUIApp::COMMessage(HRESULT hr,LPCTSTR szText)
{
	if (FAILED(hr))
	{
		ErrorMessage(szText,(DWORD)hr);
		return false;
	}
	return true;
}

void CUIApp::ErrorMessage(UINT nID,DWORD dwError)
{
	CString sMess;
	sMess.LoadString(nID);
	ErrorMessage(sMess);
}

void CUIApp::ErrorMessage(LPCTSTR szText,DWORD dwError)
{
	bool bError=true;
	if (dwError == 0)
	{
		dwError = GetLastError();
		bError = true;
	}
	else
	{
		bError = FAILED(dwError);
	}
	LPVOID lpMsgBuf=NULL;
	if (bError)
	{
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
	}
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
	CString sMess;
	if (lpMsgBuf)
	{
		sMess = (LPCTSTR)lpMsgBuf;
		sMess += _T("\n");
		LocalFree( lpMsgBuf );	
	}
	if (szText)
	{
		sMess += szText;
	}
	AfxMessageBox(sMess, MB_ICONSTOP );
}

