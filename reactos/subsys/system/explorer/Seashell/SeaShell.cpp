// SeaShell.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "SeaShell.h"

#include "MainFrm.h"
#include "SeaShellDoc.h"
#include "LeftView.h"
#include "UICtrl.h"
#include "TextProgressCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_APP_UPDATE_PROGRESS WM_APP+1
#define WM_APP_FINISH_PROGRESS WM_APP+2
/////////////////////////////////////////////////////////////////////////////
// CSeaShellApp

BEGIN_MESSAGE_MAP(CSeaShellApp, CUIApp)
	//{{AFX_MSG_MAP(CSeaShellApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CUIApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CUIApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CUIApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSeaShellApp construction

CSeaShellApp::CSeaShellApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSeaShellApp object

CSeaShellApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSeaShellApp initialization

BOOL CSeaShellApp::InitInstance()
{
	AfxEnableControlContainer();

	if (!AfxOleInit())
	{
		AfxMessageBox(_T("COM Failed to initialize"));
		return FALSE;
	}
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CSeaShellDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CLeftView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}

typedef struct THREADINFO
{
	int nRow;
	HWND hWnd;
	HANDLE hEvent;
} *LPTHREADINFO;
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };

	CUIODListCtrl	m_lcTest;
	//}}AFX_DATA
	enum Cols { COL_FILE_SIZE, COL_STATUS, COL_PROGRESS };
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CEvent m_Event;
	CWinThread *m_pThread;
	static UINT ThreadFunc(LPVOID pParam);
// Implementation
protected:
	virtual BOOL OnInitDialog();
	void PumpMessages();
	//{{AFX_MSG(CAboutDlg)
	afx_msg void OnButtProgress();
	afx_msg void OnButtNormal();
	afx_msg void OnButtDownload();
	afx_msg LRESULT OnAppUpdateProgress(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppFinishProgress(WPARAM wParam, LPARAM lParam );
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
	m_pThread = NULL;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_LIST_TEST, m_lcTest);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_BUTT_PROGRESS, OnButtProgress)
	ON_BN_CLICKED(IDC_BUTT_NORMAL, OnButtNormal)
	ON_BN_CLICKED(IDC_BUTT_DOWNLOAD, OnButtDownload)
	ON_MESSAGE(WM_APP_UPDATE_PROGRESS,OnAppUpdateProgress)
	ON_MESSAGE(WM_APP_FINISH_PROGRESS,OnAppFinishProgress)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CSeaShellApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CSeaShellApp message handlers


void CAboutDlg::OnButtProgress() 
{
	// TODO: Add your control notification handler code here
	int nRow = m_lcTest.AddTextItem();
	m_lcTest.AddString(nRow,COL_FILE_SIZE,nRow+1);
	m_lcTest.AddString(nRow,COL_STATUS,_T("Downloading"));
	CTextProgressCtrl *pPB = m_lcTest.AddProgressBar(nRow,COL_PROGRESS,0,100);
	if (pPB)
	{
		pPB->SetShowText(true);
		pPB->SetPos(0);
		pPB->SetRange(0,100);
		for (int i = 0; i < (nRow+1); i++)
		{
			pPB->StepIt();
		}
	}
}

void CAboutDlg::PumpMessages()
{
    // Must call Create() before using the dialog
    ASSERT(m_hWnd!=NULL);

    MSG msg;
    // Handle dialog messages
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if(!IsDialogMessage(&msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);  
      }
    }
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_lcTest.InitListCtrl(_T("File Size|Status|Progress"));
	m_lcTest.SetColumnFormat(COL_FILE_SIZE,LVCFMT_CENTER);	
	m_lcTest.SetColumnFormat(COL_STATUS,LVCFMT_CENTER);	
	m_lcTest.AddIcon(IDR_MAINFRAME);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnButtNormal() 
{
	// TODO: Add your control notification handler code here
	int nRow = m_lcTest.AddTextItem();
	m_lcTest.AddString(nRow,COL_FILE_SIZE,nRow+1);	
	m_lcTest.SetTextColor(nRow,COL_FILE_SIZE,RGB(0,0,128));
	m_lcTest.AddString(nRow,COL_STATUS,_T("Normal"));
	m_lcTest.SetTextColor(nRow,COL_STATUS,RGB(70,38,10));
	m_lcTest.AddString(nRow,COL_PROGRESS,_T("Progress"));
	if (nRow % 2)
	{
		m_lcTest.SetBkColor(nRow,-1,RGB(20,100,20));
		m_lcTest.SetRowBold(nRow,-1);
	}
}

void CAboutDlg::OnButtDownload() 
{
	// TODO: Add your control notification handler code here
	if (m_lcTest.GetCurSel() == -1)
		return;
	CTextProgressCtrl *pCtrl = m_lcTest.AddProgressBar(m_lcTest.GetCurSel(),COL_PROGRESS,0,120000);
	pCtrl->SetBgColor(CLR_DEFAULT,RGB(255,255,255));
	pCtrl->SetBarColor(CLR_DEFAULT,::GetSysColor(COLOR_MENU));
	LPTHREADINFO pth = new THREADINFO;
	pth->nRow = m_lcTest.GetCurSel();
	pth->hWnd = this->GetSafeHwnd();
	pth->hEvent = m_Event.m_hObject;
	m_pThread = AfxBeginThread(CAboutDlg::ThreadFunc,(LPVOID)pth);	
    m_pThread->m_bAutoDelete = FALSE;
}

UINT CAboutDlg::ThreadFunc(LPVOID pParam)
{
	LPTHREADINFO pth = (LPTHREADINFO)pParam;
	ASSERT(pth);
	DWORD dw;
	for(int i=1;i < 120;i++)
	{
		dw = WaitForMultipleObjects(1,&pth->hEvent,TRUE,1000);
        if ((dw - WAIT_OBJECT_0) == 0) 
			break;
		::PostMessage(pth->hWnd,WM_APP_UPDATE_PROGRESS,pth->nRow,i*1000);
	}
	if (::IsWindow(pth->hWnd))
		::PostMessage(pth->hWnd,WM_APP_FINISH_PROGRESS,pth->nRow,0);
	delete pth;
	TRACE0("Download thread is exiting\n");
	return 1;
}


LRESULT CAboutDlg::OnAppUpdateProgress(WPARAM wParam, LPARAM lParam)
{
	m_lcTest.AddString(wParam,COL_FILE_SIZE,lParam);
	m_lcTest.UpdateProgressBar(wParam,COL_PROGRESS,lParam);
	return 1;
}

LRESULT CAboutDlg::OnAppFinishProgress(WPARAM wParam, LPARAM lParam)
{
	delete m_pThread;
	m_pThread = NULL;
	m_lcTest.DeleteProgressBar(wParam,COL_PROGRESS);
	return 1;
}

void CAboutDlg::OnDestroy() 
{
	if (m_pThread)
	{
		m_Event.SetEvent();
		::WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		delete m_pThread;
		m_pThread = NULL;
	}
	CDialog::OnDestroy();	
	// TODO: Add your message handler code here	
}
