// MagDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Magnify.h"
#include "MagBar.h"
#include "MagDlg.h"
#include "Registry.h"

#include "..\Mag_Hook\Mag_Hook.h" // for WM_EVENT_MOUSEMOVE


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// JMC: HACK - REMOVE
const TCHAR m_szRegSubkey[] = __TEXT("Software\\Microsoft\\Magnify");

// Hotkey identifiers for RegisterHotKey
#define HKID_ZOOMUP               1
#define HKID_ZOOMDOWN             2
#define HKID_TOGGLEMOUSETRACKING  3
#define HKID_COPYTOCLIPBOARD      4


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
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
	//}}AFX_DATA_INIT
	// Set default hotkey for toggling mouse tracking to Ctrl+PageDown
	fuModifiersToggleMouseTracking = MOD_CONTROL;
	vkToggleMouseTracking = VK_NEXT;
	
	// Set default hotkey for copy to Clipboard to Ctrl+PrintScreen
	fuModifiersCopyToClipboard = MOD_ALT | MOD_SHIFT;// | MOD_CONTROL;
	vkCopyToClipboard = VK_SNAPSHOT;
	
	// Check the registry to see if we have been used before and if so,
	// reload our persistent settings.
	CRegSettings reg;
	if (reg.OpenSubkey(TRUE, HKEY_CURRENT_USER, m_szRegSubkey) == ERROR_SUCCESS) {
		reg.GetBOOL(__TEXT("StationaryTrackText"), &m_fStationaryTrackText);
		reg.GetBOOL(__TEXT("StationaryTrackSecondaryFocus"), &m_fStationaryTrackSecondaryFocus);
		reg.GetBOOL(__TEXT("StationaryTrackCursor"), &m_fStationaryTrackCursor);
		reg.GetBOOL(__TEXT("StationaryTrackFocus"), &m_fStationaryTrackFocus);
		reg.GetBOOL(__TEXT("StationaryInvertColors"), &m_fStationaryInvertColors);
		reg.GetDWORD(__TEXT("HotKeyModifiersToggleMouseTracking"), (PDWORD) &fuModifiersToggleMouseTracking);
		reg.GetDWORD(__TEXT("HotKeyVirtKeyToggleMouseTracking"), (PDWORD) &vkToggleMouseTracking);
		reg.GetDWORD(__TEXT("HotKeyModifiersCopyToClipboard"), (PDWORD) &fuModifiersCopyToClipboard);
		reg.GetDWORD(__TEXT("HotKeyVirtKeyCopyToClipboard"), (PDWORD) &vkCopyToClipboard);
		reg.GetDWORD(__TEXT("StationaryMagLevel"), (PDWORD) &m_nStationaryMagLevel);
	}

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMagnifyDlg::~CMagnifyDlg()
{
	// Save the current state of in the registry so that we'll
	// come up in the same state the next time the user runs us.
	CRegSettings reg;
	if (reg.OpenSubkey(FALSE, HKEY_CURRENT_USER, m_szRegSubkey) == ERROR_SUCCESS) {
		reg.PutBOOL(__TEXT("StationaryTrackText"), m_fStationaryTrackText);
		reg.PutBOOL(__TEXT("StationaryTrackSecondaryFocus"), m_fStationaryTrackSecondaryFocus);
		reg.PutBOOL(__TEXT("StationaryTrackCursor"), m_fStationaryTrackCursor);
		reg.PutBOOL(__TEXT("StationaryTrackFocus"), m_fStationaryTrackFocus);
		reg.PutBOOL(__TEXT("StationaryInvertColors"), m_fStationaryInvertColors);
		reg.PutDWORD(__TEXT("HotKeyModifiersToggleMouseTracking"), fuModifiersToggleMouseTracking);
		reg.PutDWORD(__TEXT("HotKeyVirtKeyToggleMouseTracking"), vkToggleMouseTracking);
		reg.PutDWORD(__TEXT("HotKeyModifiersCopyToClipboard"), fuModifiersCopyToClipboard);
		reg.PutDWORD(__TEXT("HotKeyVirtKeyCopyToClipboard"), vkCopyToClipboard);
		reg.PutDWORD(__TEXT("StationaryMagLevel"), m_nStationaryMagLevel);
	}
}

void CMagnifyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMagnifyDlg)
	DDX_Control(pDX, IDC_STATIONARYMAGSPIN, m_wndMagLevSpin);
	DDX_Check(pDX, IDC_STATIONARYTRACKTEXT, m_fStationaryTrackText);
	DDX_Check(pDX, IDC_STATIONARYTRACKSECONDARYFOCUS, m_fStationaryTrackSecondaryFocus);
	DDX_Check(pDX, IDC_STATIONARYTRACKMOUSECURSOR, m_fStationaryTrackCursor);
	DDX_Check(pDX, IDC_STATIONARYTRACKKYBDFOCUS, m_fStationaryTrackFocus);
	DDX_Text(pDX, IDC_STATIONARYMAGLEVEL, m_nStationaryMagLevel);
	DDV_MinMaxInt(pDX, m_nStationaryMagLevel, 1, 9);
	DDX_Check(pDX, IDC_STATIONARYINVERTCOLORS, m_fStationaryInvertColors);
	DDX_Check(pDX, IDC_STATIONARYHIGHCONTRAST, m_fStationaryUseHighContrast);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMagnifyDlg, CDialog)
	//{{AFX_MSG_MAP(CMagnifyDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_EN_CHANGE(IDC_STATIONARYMAGLEVEL, OnChangeStationarymaglevel)
	ON_BN_CLICKED(IDC_STATIONARYTRACKMOUSECURSOR, OnStationarytrackmousecursor)
	ON_BN_CLICKED(IDC_STATIONARYTRACKKYBDFOCUS, OnStationarytrackkybdfocus)
	ON_BN_CLICKED(IDC_STATIONARYTRACKSECONDARYFOCUS, OnStationarytracksecondaryfocus)
	ON_BN_CLICKED(IDC_STATIONARYTRACKTEXT, OnStationarytracktext)
	ON_BN_CLICKED(IDC_STATIONARYINVERTCOLORS, OnStationaryinvertcolors)
	ON_BN_CLICKED(IDC_STATIONARYHIGHCONTRAST, OnStationaryhighcontrast)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMagnifyDlg message handlers

BOOL CMagnifyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

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
	::RegisterHotKey(m_hWnd, HKID_ZOOMUP,   MOD_WIN, VK_UP);
	::RegisterHotKey(m_hWnd, HKID_ZOOMDOWN, MOD_WIN, VK_DOWN);
	::RegisterHotKey(m_hWnd, HKID_TOGGLEMOUSETRACKING, fuModifiersToggleMouseTracking, vkToggleMouseTracking);
	::RegisterHotKey(m_hWnd, HKID_COPYTOCLIPBOARD,     fuModifiersCopyToClipboard,     vkCopyToClipboard);
	
	// Set the magnification spinner's range
	m_wndMagLevSpin.SetRange(nMinZoom, nMaxZoom);
	
	// Make this dialog box a topmost window too.
//	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
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
	::UnregisterHotKey(m_hWnd, HKID_ZOOMUP);
	::UnregisterHotKey(m_hWnd, HKID_ZOOMDOWN);
	::UnregisterHotKey(m_hWnd, HKID_TOGGLEMOUSETRACKING);
	::UnregisterHotKey(m_hWnd, HKID_COPYTOCLIPBOARD);

	CDialog::OnDestroy();
}

void CMagnifyDlg::OnClose() 
{
	if (CanExit()) {
		// Destroy the Stationary and Roaming windows
		m_wndStationary.SendMessage(WM_CLOSE);
		CDialog::OnClose();
	}
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

void CMagnifyDlg::OnCancel() 
{
	if (CanExit()) {
		// Destroy the Stationary and Roaming windows
		m_wndStationary.SendMessage(WM_CLOSE);
		CDialog::OnCancel();
	}
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

LRESULT CMagnifyDlg::OnHotKey(WPARAM wParam, LPARAM lParam) {
	int idHotKey = wParam;
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
		
	case HKID_COPYTOCLIPBOARD:
		m_wndStationary.CopyToClipboard();
		break;
	}
	return(0);
}

void CMagnifyDlg::OnStationaryhighcontrast() 
{
	UpdateData();
	HIGHCONTRAST hc;
	hc.cbSize = sizeof(hc);
	if(		SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, FALSE)
		&&	(hc.dwFlags & HCF_AVAILABLE))
	{
		if(m_fStationaryUseHighContrast)
			hc.dwFlags |= HCF_HIGHCONTRASTON;
		else
			hc.dwFlags &= ~HCF_HIGHCONTRASTON;

		SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof(hc), &hc, TRUE);
	}
#pragma message("FINISHFINISHFINISHFINISHFINISHFINISHFINISHFINISH - CHANGE FOR NT4")

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
