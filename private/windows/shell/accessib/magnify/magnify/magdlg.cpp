// MagDlg.cpp : implementation file
// 
// Copyright (c) 1997-1999 Microsoft Corporation
//  

#include "stdafx.h"
#include "Magnify.h"
#include "AppBar.h"
#include "MagBar.h"
#include "MagDlg.h"
#include "Registry.h"
#include <htmlhelp.h>

#include "..\Mag_Hook\Mag_Hook.h" // for WM_EVENT_MOUSEMOVE


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// JMC: HACK - REMOVE
const TCHAR m_szRegSubkey[] = __TEXT("Software\\Microsoft\\Magnify");
const TCHAR g_szSaveKey[]				= __TEXT("Magnify");
const TCHAR m_szMagSubkey[] = __TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
const TCHAR g_szSaveRoam[]				= __TEXT("Magnify.exe");

// Location of registry entries
// NOTE: If you change the default hot keys, you must change the 'Ver0' to 'Ver1', 'Ver2', etc.
// Otherwise, people with a new copy of the app will continue to use the old hot keys.
const TCHAR g_szRegStationaryTrackText[]				= __TEXT("StationaryTrackText");
const TCHAR g_szRegStationaryTrackSecondaryFocus[]		= __TEXT("StationaryTrackSecondaryFocus");
const TCHAR g_szRegStationaryTrackCursor[]				= __TEXT("StationaryTrackCursor");
const TCHAR g_szRegStationaryTrackFocus[]				= __TEXT("StationaryTrackFocus");
const TCHAR g_szRegStationaryInvertColors[]				= __TEXT("StationaryInvertColors");
const TCHAR g_szRegHotKeyModifiersToggleMouseTracking[]	= __TEXT("HotKeyVer0ModifiersToggleMouseTracking");
const TCHAR g_szRegHotKeyVirtKeyToggleMouseTracking[]	= __TEXT("HotKeyVer0VirtKeyToggleMouseTracking");
const TCHAR g_szRegHotKeyModifiersToggleInvertColors[]	= __TEXT("HotKeyVer0ModifiersToggleInvertColors");
const TCHAR g_szRegHotKeyVirtKeyToggleInvertColors[]	= __TEXT("HotKeyVer0VirtKeyToggleInvertColors");
const TCHAR g_szRegHotKeyModifiersCopyToClipboard[]		= __TEXT("HotKeyVer0ModifiersCopyToClipboard");
const TCHAR g_szRegHotKeyVirtKeyCopyToClipboard[]		= __TEXT("HotKeyVer0VirtKeyCopyToClipboard");
const TCHAR g_szRegHotKeyModifiersCopyToClipboard2[]	= __TEXT("HotKeyVer0ModifiersCopyToClipboard2");
const TCHAR g_szRegHotKeyVirtKeyCopyToClipboard2[]		= __TEXT("HotKeyVer0VirtKeyCopyToClipboard2");
const TCHAR g_szRegHotKeyModifiersHideMagnifier[]		= __TEXT("HotKeyVer0ModifiersHideMagnifier");
const TCHAR g_szRegHotKeyVirtKeyHideMagnifier[]			= __TEXT("HotKeyVer0VirtKeyHideMagnifier");
const TCHAR g_szRegStationaryMagLevel[]					= __TEXT("StationaryMagLevel");
const TCHAR g_szRegStationaryStartMinimized[]			= __TEXT("StationaryStartMinimized");
const TCHAR g_szRegUseHotKeys[]			                = __TEXT("UseHotKeys");
const TCHAR g_szRegShowWarning[]			            = __TEXT("ShowWarning");


// Hotkey identifiers for RegisterHotKey
#define HKID_ZOOMUP               1
#define HKID_ZOOMDOWN             2
#define HKID_TOGGLEMOUSETRACKING  3
#define HKID_TOGGLEINVERTCOLORS   4
#define HKID_COPYTOCLIPBOARD      5
#define HKID_COPYTOCLIPBOARD2     6
#define HKID_HIDEMAGNIFIER        7

#define DESKTOP_ACCESSDENIED 0
#define DESKTOP_DEFAULT      1
#define DESKTOP_SCREENSAVER  2
#define DESKTOP_WINLOGON     3
#define DESKTOP_TESTDISPLAY  4
#define DESKTOP_OTHER        5

#define WM_DELAYEDMINIMIZE        WM_APP
DWORD GetDesktop();

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult); 
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::PreTranslateMessage(MSG* pMsg) 
{
	// JMC: HACK TO GET THIS TO WORK WITH MouseHook
	switch(pMsg->message)
	{
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		FakeCursorMove(pMsg->pt);
		break;
	default:
		break;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CAboutDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	INT idCtl		= (INT)wParam;
	LPNMHDR pnmh	= (LPNMHDR)lParam;
	switch ( pnmh->code)
	{
		case NM_RETURN:
		case NM_CLICK:
		if ( (GetDesktop() == DESKTOP_DEFAULT) && (idCtl == IDC_ENABLEWEB2))
		{
			CString webAddr;
			webAddr.LoadString(IDS_ENABLEWEB);
			ShellExecute(m_hWnd, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
		}
		break;
	}
	return TRUE;
}


class CWarnDlg : public CDialog
{
public:
	CWarnDlg();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	BOOL m_warning;
// Dialog Data
	//{{AFX_DATA(CWarnDlg)
	enum { IDD = IDD_WARNING };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWarnDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult); 
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CWarnDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CWarnDlg::CWarnDlg() : CDialog(CWarnDlg::IDD)
{
	//{{AFX_DATA_INIT(CWarnDlg)
	//}}AFX_DATA_INIT
	m_warning = 0;
}

void CWarnDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWarnDlg)
		DDX_Check(pDX, IDC_SHOW, m_warning);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWarnDlg, CDialog)
	//{{AFX_MSG_MAP(CWarnDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWarnDlg::PreTranslateMessage(MSG* pMsg) 
{
	// JMC: HACK TO GET THIS TO WORK WITH MouseHook
	switch(pMsg->message)
	{
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		FakeCursorMove(pMsg->pt);
		break;
	default:
		break;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CWarnDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	INT idCtl		= (INT)wParam;
	LPNMHDR pnmh	= (LPNMHDR)lParam;
	switch ( pnmh->code)
	{
		case NM_RETURN:
		case NM_CLICK:
		if ( (GetDesktop() == DESKTOP_DEFAULT) && (idCtl == IDC_ENABLEWEB))
		{
			CString webAddr;
			webAddr.LoadString(IDS_ENABLEWEB);
			ShellExecute(m_hWnd, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
		}
		break;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMagnifyDlg dialog

#pragma warning(disable:4355)  // Disable warning C4355: 'this' : used in base member initializer list

CMagnifyDlg::CMagnifyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMagnifyDlg::IDD, pParent), m_wndStationary(this)
{
	m_fOnInitDialogCalledYet = FALSE;
	//{{AFX_DATA_INIT(CMagnifyDlg)
	m_fStationaryTrackText = TRUE;
	m_fStationaryTrackSecondaryFocus = TRUE;
	m_fStationaryTrackCursor = TRUE;
	m_fStationaryTrackFocus = TRUE;
	m_nStationaryMagLevel = 2;
	m_fStationaryInvertColors = FALSE;
	m_fStationaryUseHighContrast = FALSE;
	m_fStationaryStartMinimized = FALSE;
	m_fStationaryShowMagnifier = TRUE;
	//}}AFX_DATA_INIT
	m_fUseHotKeys = FALSE;
	m_fShowWarning = TRUE;
	idEvent = 6465;

	// Set default hotkey for toggling mouse tracking to Win+PageDown
	fuModifiersToggleMouseTracking = MOD_WIN;
	vkToggleMouseTracking = VK_NEXT;

	// Set default hotkey for toggling inverse colors to Win+PageUp
	fuModifiersToggleInvertColors = MOD_WIN;
	vkToggleInvertColors = VK_PRIOR;
	
	// Set default hotkey for copy to Clipboard to Win+PrintScreen
	fuModifiersCopyToClipboard = MOD_WIN;
	vkCopyToClipboard = VK_SNAPSHOT;
	
	// Set default hotkey for copy to Clipboard without mouse cursor to Win+Shift+PrintScreen
	fuModifiersCopyToClipboard2 = MOD_WIN;
	vkCopyToClipboard2 = VK_SCROLL;

	// Set default hotkey for hiding magnifier to Win+End
	fuModifiersHideMagnifier = MOD_WIN;
	vkHideMagnifier = VK_END;

	// Check the registry to see if we have been used before and if so,
	// reload our persistent settings.
	CRegSettings reg;
	if (reg.OpenSubkey(TRUE, HKEY_CURRENT_USER, m_szRegSubkey) == ERROR_SUCCESS) {
		reg.GetBOOL(g_szRegStationaryTrackText, &m_fStationaryTrackText);
		reg.GetBOOL(g_szRegStationaryTrackSecondaryFocus, &m_fStationaryTrackSecondaryFocus);
		reg.GetBOOL(g_szRegStationaryTrackCursor, &m_fStationaryTrackCursor);
		reg.GetBOOL(g_szRegStationaryTrackFocus, &m_fStationaryTrackFocus);
		reg.GetBOOL(g_szRegStationaryInvertColors, &m_fStationaryInvertColors);
		reg.GetDWORD(g_szRegHotKeyModifiersToggleMouseTracking, (PDWORD) &fuModifiersToggleMouseTracking);
		reg.GetDWORD(g_szRegHotKeyVirtKeyToggleMouseTracking, (PDWORD) &vkToggleMouseTracking);
		reg.GetDWORD(g_szRegHotKeyModifiersToggleInvertColors, (PDWORD) &fuModifiersToggleInvertColors);
		reg.GetDWORD(g_szRegHotKeyVirtKeyToggleInvertColors, (PDWORD) &vkToggleInvertColors);
		reg.GetDWORD(g_szRegHotKeyModifiersCopyToClipboard, (PDWORD) &fuModifiersCopyToClipboard);
		reg.GetDWORD(g_szRegHotKeyVirtKeyCopyToClipboard, (PDWORD) &vkCopyToClipboard);
		reg.GetDWORD(g_szRegHotKeyModifiersCopyToClipboard2, (PDWORD) &fuModifiersCopyToClipboard2);
		reg.GetDWORD(g_szRegHotKeyVirtKeyCopyToClipboard2, (PDWORD) &vkCopyToClipboard2);
		reg.GetDWORD(g_szRegHotKeyModifiersHideMagnifier, (PDWORD) &fuModifiersHideMagnifier);
		reg.GetDWORD(g_szRegHotKeyVirtKeyHideMagnifier, (PDWORD) &vkHideMagnifier);
		reg.GetDWORD(g_szRegStationaryMagLevel, (PDWORD) &m_nStationaryMagLevel);
		reg.GetBOOL(g_szRegStationaryStartMinimized, & m_fStationaryStartMinimized);
		reg.GetBOOL(g_szRegUseHotKeys, & m_fUseHotKeys);
		reg.GetBOOL(g_szRegShowWarning, & m_fShowWarning);
	}

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_fAccessTimeOutOverWritten = FALSE;
}

CMagnifyDlg::~CMagnifyDlg()
{
	// Save the current state of in the registry so that we'll
	// come up in the same state the next time the user runs us.
    SaveSettings();
}

void CMagnifyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMagnifyDlg)
	DDX_Control(pDX, IDC_STATIONARYMAGLEVEL, m_NumEdit);
	DDX_Control(pDX, IDC_STATIONARYMAGSPIN, m_wndMagLevSpin);
	DDX_Check(pDX, IDC_STATIONARYTRACKTEXT, m_fStationaryTrackText);
	DDX_Check(pDX, IDC_STATIONARYTRACKSECONDARYFOCUS, m_fStationaryTrackSecondaryFocus);
	DDX_Check(pDX, IDC_STATIONARYTRACKMOUSECURSOR, m_fStationaryTrackCursor);
	DDX_Check(pDX, IDC_STATIONARYTRACKKYBDFOCUS, m_fStationaryTrackFocus);
	DDX_Text(pDX, IDC_STATIONARYMAGLEVEL, m_nStationaryMagLevel);
	DDX_Check(pDX, IDC_STATIONARYINVERTCOLORS, m_fStationaryInvertColors);
	DDX_Check(pDX, IDC_STATIONARYHIGHCONTRAST, m_fStationaryUseHighContrast);
	DDX_Check(pDX, IDC_STATIONARYSTARTMINIMIZED, m_fStationaryStartMinimized);
	DDX_Check(pDX, IDC_STATIONARYSHOWMAGNIFIER, m_fStationaryShowMagnifier);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMagnifyDlg, CDialog)
	//{{AFX_MSG_MAP(CMagnifyDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
    ON_WM_ENDSESSION()
	ON_EN_CHANGE(IDC_STATIONARYMAGLEVEL, OnChangeStationarymaglevel)
	ON_BN_CLICKED(IDC_STATIONARYTRACKMOUSECURSOR, OnStationarytrackmousecursor)
	ON_BN_CLICKED(IDC_STATIONARYTRACKKYBDFOCUS, OnStationarytrackkybdfocus)
	ON_BN_CLICKED(IDC_STATIONARYTRACKSECONDARYFOCUS, OnStationarytracksecondaryfocus)
	ON_BN_CLICKED(IDC_STATIONARYTRACKTEXT, OnStationarytracktext)
	ON_BN_CLICKED(IDC_STATIONARYINVERTCOLORS, OnStationaryinvertcolors)
	ON_BN_CLICKED(IDC_STATIONARYHIGHCONTRAST, OnStationaryhighcontrast)
	ON_BN_CLICKED(ID_MHELP, OnHelp)
	ON_BN_CLICKED(IDC_EXIT, OnExit)
	ON_WM_SETTINGCHANGE()
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_STATIONARYSHOWMAGNIFIER, OnStationaryshowmagnifier)
	ON_BN_CLICKED(IDC_STATIONARYSTARTMINIMIZED, OnStationarystartminimized)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
	ON_MESSAGE(WM_DELAYEDMINIMIZE, OnDelayedMinimize)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMagnifyDlg message handlers

BOOL CMagnifyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	DWORD deskID = GetDesktop();

	if ( deskID != DESKTOP_DEFAULT )
		GetDlgItem(ID_MHELP)->EnableWindow(FALSE);
	else 
	{
	}
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_hEventAlreadyRunning = CreateEvent(NULL, FALSE, FALSE, __TEXT("JMR\\MSMagnifierAlreadyRunning"));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Prevent multiple instances from running
		EndDialog(1);
		return(FALSE);
	}

	m_fOnInitDialogCalledYet = TRUE;
	
	// Create the Stationary and Roaming windows
	m_wndStationary.Create(IDD_STATIONARY, NULL);
	//m_wndRoaming.Create;
	
	// Register the hotkeys
	if( m_fUseHotKeys )
	{
		::RegisterHotKey(m_hWnd, HKID_ZOOMUP,   MOD_WIN, VK_UP);
		::RegisterHotKey(m_hWnd, HKID_ZOOMDOWN, MOD_WIN, VK_DOWN);
		::RegisterHotKey(m_hWnd, HKID_TOGGLEMOUSETRACKING, fuModifiersToggleMouseTracking, vkToggleMouseTracking);
		::RegisterHotKey(m_hWnd, HKID_TOGGLEINVERTCOLORS,  fuModifiersToggleInvertColors,  vkToggleInvertColors);
		::RegisterHotKey(m_hWnd, HKID_COPYTOCLIPBOARD,     fuModifiersCopyToClipboard,     vkCopyToClipboard);
		::RegisterHotKey(m_hWnd, HKID_COPYTOCLIPBOARD2,    fuModifiersCopyToClipboard2,    vkCopyToClipboard2);
		::RegisterHotKey(m_hWnd, HKID_HIDEMAGNIFIER,       fuModifiersHideMagnifier,       vkHideMagnifier);
	}
	
	// Set the magnification spinner's range
	m_wndMagLevSpin.SetRange(nMinZoom, nMaxZoom);
	
	// Make this dialog box a topmost window too.
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// Set the high contrast check box
	CheckHighContrastSetting();
	CenterWindow();

	if(m_fStationaryStartMinimized || (GetDesktop() != DESKTOP_DEFAULT))
	{
		// Doing just a ShowWindow(SW_MINIMIZE) here doesn't quite
		// work - we end up as a minimized dialog *with the focus*.
		// - Windows always focuses dialogs after they're created.
		// This results in the tray icon flashing, and then remaining
		// inverted until the dialog is restored.
		// So we post ourselves this 'delayed minimize' message -
		// when we receive it - after we've been focused - we forcibly
		// minimize ourselves again, losing the focus in the process.
		// This allows us to start minimized 'unobtrusively'.
		// The ShowWindow(...) here just sets the internal window state
		// to minimized before we get displayed, so we are initially displayed
		// as minimized. If we were initially displayed as full, there'd
		// be flicker/animation as we were later minimized.
		ShowWindow(SW_SHOWMINIMIZED);
		::PostMessage(m_hWnd, WM_DELAYEDMINIMIZE, 0, 0);
	}

	// HACK: Make it work with utility Manager: Change post beta3
	m_wndStationary.SetHidden(FALSE);

	// Show the warning message after 100 ms...
	SetTimer(idEvent, 10, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMagnifyDlg::OnTimer(UINT idEvent)
{
	KillTimer(idEvent);

	if ( m_fShowWarning && (GetDesktop() == DESKTOP_DEFAULT))
	{
		CWarnDlg dlg;
		dlg.DoModal();
		m_fShowWarning = !(dlg.m_warning);
	}
}

void CMagnifyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMagnifyDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMagnifyDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMagnifyDlg::OnDestroy() 
{
	if( m_fUseHotKeys )
	{
		::UnregisterHotKey(m_hWnd, HKID_ZOOMUP);
		::UnregisterHotKey(m_hWnd, HKID_ZOOMDOWN);
		::UnregisterHotKey(m_hWnd, HKID_TOGGLEMOUSETRACKING);
		::UnregisterHotKey(m_hWnd, HKID_TOGGLEINVERTCOLORS);
		::UnregisterHotKey(m_hWnd, HKID_COPYTOCLIPBOARD);
		::UnregisterHotKey(m_hWnd, HKID_COPYTOCLIPBOARD2);
		::UnregisterHotKey(m_hWnd, HKID_HIDEMAGNIFIER);
	}
    
	CloseHandle(m_hEventAlreadyRunning); // JMC: Can't forget to close this handle

	ClearAccessTimeOut();

	CDialog::OnDestroy();
}

void CMagnifyDlg::OnOK() 
{
	// Load the values from the controls into the member variables
	if (!UpdateData(TRUE)) {
		TRACE0("UpdateData failed during dialog termination.\n");
		// the UpdateData routine will set focus to correct item
		return;
	} else {
		// Data in member variables OK
		// For stationary window to refresh using the new Zoom level
		m_wndStationary.InvalidateRect(NULL, FALSE);
	}
	// NOTE: Don't call CDialog::OnOK or the dialog box will be 
	// destroyed and the process will terminate.  The process should 
	// die when the user presses the Exit button (IDCANCEL)
	ShowWindow(SW_MINIMIZE);

	// Original MFC-produced code: if (CanExit()) CDialog::OnOK();
}

void CMagnifyDlg::OnExit() 
{
    if (CanExit()) {
        // Destroy the Stationary and Roaming windows
        // JMC: Don't send a WM_CLOSE, because its OnClose sends a WM_CLOSE to use
    // Closing this window will mean doing a proper cleanup
        m_wndStationary.DestroyWindow();
        CDialog::OnCancel();
    }
}

void CMagnifyDlg::OnClose() 
{
    if (CanExit()) {
        // Destroy the Stationary and Roaming windows
        // JMC: Don't send a WM_CLOSE, because its OnClose sends a WM_CLOSE to use
    // Closing this window will mean doing a proper cleanup
        m_wndStationary.DestroyWindow();
        CDialog::OnCancel();
    }
}

void CMagnifyDlg::OnCancel() 
{
	return;
}

BOOL CMagnifyDlg::CanExit() {
	return TRUE;
}

void CMagnifyDlg::UpdateState() {
	if (m_fOnInitDialogCalledYet) {
		// Load the values from the controls into the member variables
		if (!UpdateData(TRUE)) {
			TRACE0("UpdateData failed during dialog termination.\n");
			// the UpdateData routine will set focus to correct item
			return;
		} else {
			// Data in member variables OK
			// For stationary window to refresh using the new Zoom level
			m_wndStationary.ZoomChanged();
			m_wndStationary.InvalidateRect(NULL, FALSE);
		}
	}
}

void CMagnifyDlg::OnChangeStationarymaglevel() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.

	// Make sure we are in a valid range
	if(m_fOnInitDialogCalledYet)
	{
		UpdateData();
		m_nStationaryMagLevel = min(max(1, m_nStationaryMagLevel), 9);
		UpdateData(FALSE);
	}

	// TODO: Add your control notification handler code here
	UpdateState();
}

void CMagnifyDlg::OnStationarytrackmousecursor() 
{
	// TODO: Add your control notification handler code here
	UpdateState();
}

void CMagnifyDlg::OnStationarytrackkybdfocus() 
{
	// TODO: Add your control notification handler code here
	UpdateState();
}

void CMagnifyDlg::OnStationarytracksecondaryfocus() 
{
	// TODO: Add your control notification handler code here
	UpdateState();
}

void CMagnifyDlg::OnStationarytracktext() 
{
	// TODO: Add your control notification handler code here
	UpdateState();
}

void CMagnifyDlg::OnStationaryinvertcolors() 
{
	// TODO: Add your control notification handler code here
	UpdateState();
}

void CMagnifyDlg::OnStationarystartminimized() 
{
	// TODO: Add your control notification handler code here
	
}

void CMagnifyDlg::OnStationaryshowmagnifier() 
{
	// TODO: Add your control notification handler code here

	// Don't call update state - just call SetHidden
	UpdateData();
	m_wndStationary.SetHidden(!m_fStationaryShowMagnifier);
	m_wndStationary.InvalidateRect(NULL, FALSE);
}

LRESULT CMagnifyDlg::OnHotKey(WPARAM wParam, LPARAM lParam) {
	INT_PTR idHotKey = wParam;
	UpdateData();
	switch (idHotKey) {
	case HKID_ZOOMUP:
	case HKID_ZOOMDOWN:
		if (idHotKey == HKID_ZOOMUP) {
			if (m_nStationaryMagLevel < nMaxZoom) m_nStationaryMagLevel++;
		} else {
			if (m_nStationaryMagLevel > nMinZoom) m_nStationaryMagLevel--;
		}
		UpdateData(FALSE);
		m_wndStationary.ZoomChanged();
		m_wndStationary.InvalidateRect(NULL, FALSE);
		break;
		
	case HKID_TOGGLEMOUSETRACKING:
		m_fStationaryTrackCursor ^= TRUE;
		UpdateData(FALSE);   // Make sure the dialog box reflect the change.
		break;

	case HKID_TOGGLEINVERTCOLORS:
		m_wndStationary.Invalidate(FALSE);
		m_fStationaryInvertColors ^= TRUE;
		UpdateData(FALSE);   // Make sure the dialog box reflect the change.
		break;
		
	case HKID_COPYTOCLIPBOARD2:
		// make sure for this case we erase the window before copying it
		m_wndStationary.ForceHideCursor(TRUE);
		m_wndStationary.Invalidate(FALSE);
		m_wndStationary.UpdateWindow();
		m_wndStationary.ForceHideCursor(FALSE);
	case HKID_COPYTOCLIPBOARD:
		m_wndStationary.CopyToClipboard();
		break;

	case HKID_HIDEMAGNIFIER:
		// toggle hidden state of magnifier
		m_fStationaryShowMagnifier ^= TRUE;
		UpdateData(FALSE);
		m_wndStationary.SetHidden(!m_fStationaryShowMagnifier);
		m_wndStationary.ZoomChanged();
		m_wndStationary.Invalidate(FALSE);
		break;
	}
	return(0);
}

LRESULT CMagnifyDlg::OnDelayedMinimize(WPARAM wParam, LPARAM lParam)
{
	// Minimize alone isn't enough to shake off the focus!
	// Hiding forces the focus to go the the next window in line,
	// and then we re-show ourselves as minimized (without taking the
	// focus back).
	ShowWindow(SW_HIDE);
	ShowWindow(SW_MINIMIZE);
	return 0;
}

void CMagnifyDlg::OnStationaryhighcontrast() 
{
	UpdateData();
	HIGHCONTRAST hc;
	BOOL result;
	hc.cbSize = sizeof(hc);
	if(		SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, FALSE)
		&&	(hc.dwFlags & HCF_AVAILABLE))
	{
		if(m_fStationaryUseHighContrast)
		{
			hc.dwFlags |= HCF_HIGHCONTRASTON;
			OverWriteAccessTimeOut();
		}
		else
		{
			ClearAccessTimeOut();
			hc.dwFlags &= ~HCF_HIGHCONTRASTON;
		}

		result = SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof(hc), &hc, TRUE);
		ASSERT(result);
	}
#pragma message("FINISHFINISHFINISHFINISHFINISHFINISHFINISHFINISH - CHANGE FOR NT4")
	UpdateState();
}

BOOL CMagnifyDlg::PreTranslateMessage(MSG* pMsg) 
{
	// JMC: HACK TO GET THIS TO WORK WITH MouseHook
	switch(pMsg->message)
	{
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		FakeCursorMove(pMsg->pt);
		break;
	default:
		break;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CMagnifyDlg::OnSettingChange(UINT uFlags, LPCTSTR lpszSection) 
{
	CDialog::OnSettingChange(uFlags, lpszSection);
	
	CheckHighContrastSetting();
}

void CMagnifyDlg::CheckHighContrastSetting()
{
	// Set the high contrast check box
	UpdateData();
	HIGHCONTRAST hc;
	hc.cbSize = sizeof(hc);
	BOOL fUseHighContrast = FALSE;

	if(		SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, FALSE)
		&&	(hc.dwFlags & HCF_AVAILABLE))
	{
		fUseHighContrast = (hc.dwFlags & HCF_HIGHCONTRASTON)?TRUE:FALSE;
	}

	// Only update if the high contrast flag actually changed
	// - ie. ignore all other setting/ini changes.
	// Important, since otherwise we could get a change message as a result of the
	// appbar forcing a screen size change at magnifier startup or shutdown, and
	// we'd call through to UpdateState() before we've properly inited or after
	// we've cleaned up. Not good.
	if( m_fStationaryUseHighContrast != fUseHighContrast )
	{
		m_fStationaryUseHighContrast = fUseHighContrast;

		UpdateData(FALSE);

		UpdateState();
	}
}


DWORD g_rgHelpIds[] = {	1000, 8200,
						1002, 8202,
						1003, 8204,
						1013, 8206,
						1005, 8208,
						1006, 8210,
						IDOK, 8212,
						IDCANCEL, 8214,
						1007, 8216,
						1008, 8218,
						0, 0
					};


BOOL CMagnifyDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if ( GetDesktop() != DESKTOP_DEFAULT )
		return 0;

	::HtmlHelp(NULL , TEXT("magnify.chm"), HH_DISPLAY_TOPIC, 0);

	return 1;
//	return CDialog::OnHelpInfo(pHelpInfo);
}

void CMagnifyDlg::OnHelp()
{
	if ( GetDesktop() != DESKTOP_DEFAULT )
		return;

	::HtmlHelp(m_hWnd , TEXT("magnify.chm"), HH_DISPLAY_TOPIC, 0);
}

void CMagnifyDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if ( GetDesktop() != DESKTOP_DEFAULT )
		return;

	::WinHelp(pWnd->m_hWnd, __TEXT("magnify.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_rgHelpIds);

	return;
//	return CDialog::OnHelpInfo(pHelpInfo);
}

void CMagnifyDlg::OverWriteAccessTimeOut()
{
	if(m_fAccessTimeOutOverWritten)
		return;

	m_AccTimeOut.cbSize = sizeof(m_AccTimeOut);
	SystemParametersInfo(SPI_GETACCESSTIMEOUT, sizeof(m_AccTimeOut), &m_AccTimeOut, 0);
	ACCESSTIMEOUT AccTimeOutTemp = m_AccTimeOut;
	AccTimeOutTemp.dwFlags &= ~ATF_TIMEOUTON;
	SystemParametersInfo(SPI_SETACCESSTIMEOUT, sizeof(AccTimeOutTemp), &AccTimeOutTemp, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
	m_fAccessTimeOutOverWritten = TRUE;
}
void CMagnifyDlg::ClearAccessTimeOut()
{
	if(!m_fAccessTimeOutOverWritten)
		return;
	SystemParametersInfo(SPI_SETACCESSTIMEOUT, sizeof(m_AccTimeOut), &m_AccTimeOut, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
	m_fAccessTimeOutOverWritten = FALSE;
}


void CMagnifyDlg::SetStationaryHidden()
{
	// Hide the window
	UpdateData();
	m_fStationaryShowMagnifier = FALSE;
	UpdateData(FALSE);
	m_wndStationary.SetHidden(!m_fStationaryShowMagnifier);
}

// This method is going to save all values of the magnifier in the
// registry. And will write into RunOnce key to enable Magnifier to 
// come up, the next time.
void CMagnifyDlg::OnEndSession( BOOL bEnding )
{
    CDialog::OnEndSession(bEnding);
    

    if(bEnding)
    {
        HKEY  hSubkey;
        DWORD dwPosition;
        
        // a-anilk: Save the current settings of the Magnifier. This is required because
        // While processing WM_ENDSESSION, The destructor call is NOT gaurenteed to be
        // Exceuted properly. Infact in this case, It is NOT 
        SaveSettings();
        
	    
        CRegSettings reg;
        // This will save the registry values required for size
	    if (reg.OpenSubkey(FALSE, HKEY_CURRENT_USER, m_szRegSubkey) == ERROR_SUCCESS)
	    {
		    APPBARSTATE abs;
		    abs.m_cbSize = sizeof(abs);
		    m_wndStationary.GetState(&abs);
		    // Save the AppBar's state variables to the registry.
		    reg.PutBinary(__TEXT("AppBar"), (PBYTE) &abs, sizeof(abs));
	    }
        
        INT len;     // length of string with null in bytes
        len= (lstrlen( g_szSaveRoam )+1) * sizeof(TCHAR);

        // Save in RunOnce key
        RegCreateKeyEx(HKEY_CURRENT_USER, m_szMagSubkey,0, NULL, REG_OPTION_NON_VOLATILE , 
            KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hSubkey, &dwPosition );

        RegSetValueEx(hSubkey, g_szSaveKey, 0, REG_SZ, (CONST BYTE*)g_szSaveRoam, len);
    
        RegCloseKey(hSubkey);
    }
}


void CMagnifyDlg::SaveSettings()
{

    CRegSettings reg;
	if (reg.OpenSubkey(FALSE, HKEY_CURRENT_USER, m_szRegSubkey) == ERROR_SUCCESS) {
		reg.PutBOOL(g_szRegStationaryTrackText, m_fStationaryTrackText);
		reg.PutBOOL(g_szRegStationaryTrackSecondaryFocus, m_fStationaryTrackSecondaryFocus);
		reg.PutBOOL(g_szRegStationaryTrackCursor, m_fStationaryTrackCursor);
		reg.PutBOOL(g_szRegStationaryTrackFocus, m_fStationaryTrackFocus);
		reg.PutBOOL(g_szRegStationaryInvertColors, m_fStationaryInvertColors);
		reg.PutDWORD(g_szRegHotKeyModifiersToggleMouseTracking, fuModifiersToggleMouseTracking);
		reg.PutDWORD(g_szRegHotKeyVirtKeyToggleMouseTracking, vkToggleMouseTracking);
		reg.PutDWORD(g_szRegHotKeyModifiersToggleInvertColors, fuModifiersToggleInvertColors);
		reg.PutDWORD(g_szRegHotKeyVirtKeyToggleInvertColors, vkToggleInvertColors);
		reg.PutDWORD(g_szRegHotKeyModifiersCopyToClipboard, fuModifiersCopyToClipboard);
		reg.PutDWORD(g_szRegHotKeyVirtKeyCopyToClipboard, vkCopyToClipboard);
		reg.PutDWORD(g_szRegHotKeyModifiersCopyToClipboard2, fuModifiersCopyToClipboard2);
		reg.PutDWORD(g_szRegHotKeyVirtKeyCopyToClipboard2, vkCopyToClipboard2);
		reg.PutDWORD(g_szRegHotKeyModifiersHideMagnifier, fuModifiersHideMagnifier);
		reg.PutDWORD(g_szRegHotKeyVirtKeyHideMagnifier, vkHideMagnifier);
		reg.PutDWORD(g_szRegStationaryMagLevel, m_nStationaryMagLevel);
		reg.PutBOOL(g_szRegStationaryStartMinimized, m_fStationaryStartMinimized);
		reg.PutBOOL(g_szRegShowWarning, m_fShowWarning);
	}
}

// a-anilk added
// Returns the current desktop-ID
DWORD GetDesktop()
{
    HDESK hdesk;
    TCHAR name[300];
    DWORD value, nl, desktopID = DESKTOP_ACCESSDENIED;
    CRegSettings reg;

	reg.OpenSubkey(TRUE, HKEY_LOCAL_MACHINE, __TEXT("SYSTEM\\Setup") );
	reg.GetDWORD(__TEXT("SystemSetupInProgress"), &value);
	if ( value )
	{
		// Setup is in progress...
		return DESKTOP_ACCESSDENIED;
	}

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return DESKTOP_WINLOGON;
    }
    
	GetUserObjectInformation(hdesk, UOI_NAME, name, 300, &nl);
    
	if (!_tcsicmp(name, __TEXT("Default")))
        desktopID = DESKTOP_DEFAULT;

    else if (!_tcsicmp(name, __TEXT("Winlogon")))
        desktopID = DESKTOP_WINLOGON;

    else if (!_tcsicmp(name, __TEXT("screen-saver")))
        desktopID = DESKTOP_SCREENSAVER;

    else if (!_tcsicmp(name, __TEXT("Display.Cpl Desktop")))
        desktopID = DESKTOP_TESTDISPLAY;

    else
        desktopID = DESKTOP_OTHER;
    
	return desktopID;
}
