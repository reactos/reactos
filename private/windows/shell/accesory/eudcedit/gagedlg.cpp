/**************************************************/
/*					                              */
/*					                              */
/*	Gage when import bitmap		                  */ 
/*		(Dialog)		                          */
/*					                              */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"gagedlg.h"
#include	"extfunc.h"
#include	"util.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern TCHAR	UserFont[MAX_PATH];
extern TCHAR	EUDCTTF[MAX_PATH];
extern TCHAR	EUDCBMP[MAX_PATH];
static HWND	hDlg;
static UINT	nEUDC;
static UINT	cEUDC;
static BOOL	ExecuteFlag;
static BOOL	testtest;
extern BOOL 	g_bKeepEUDCLink;
BOOL	SendImportMessage( unsigned int cEudc, unsigned int nEudc);

/****************************************/
/*					*/
/*	Constructor			*/
/*					*/
/****************************************/
CGageDlg::CGageDlg( CWnd* pParent, LPTSTR szUserFont, LPTSTR szBmpFile, LPTSTR szTtfFile, BOOL bIsWin95EUDC)
	: CDialog(CGageDlg::IDD, pParent)
{

	lstrcpy (m_szTtfFile, szTtfFile? szTtfFile : EUDCTTF);
	lstrcpy (m_szBmpFile, szBmpFile? szBmpFile : EUDCBMP);
	lstrcpy (m_szUserFont, szUserFont? szUserFont : UserFont);
	m_bIsWin95EUDC = bIsWin95EUDC;


	//{{AFX_DATA_INIT(CGageDlg)
	//}}AFX_DATA_INIT
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_INITDIALOG"		*/
/*					*/
/****************************************/
BOOL 
CGageDlg::OnInitDialog()
{
	CString	DlgTitle;

	CDialog::OnInitDialog();

//	Implement "?" in this dialog.
	DlgTitle.LoadString( IDS_IMPORT_DLGTITLE);	
	this->SetWindowText( DlgTitle);

//	Set Dialog Title name.
	m_EditGage.SubclassDlgItem( IDC_GAGE, this);
	hDlg = this->GetSafeHwnd();

	testtest = FALSE;

	OutputMessageBox( this->GetSafeHwnd(),
                    IDS_MAINFRAMETITLE,
                    IDS_IMPORTFONT_MSG, TRUE);
	return TRUE;
}

/****************************************/
/*					*/
/*	COMMAND	"OK"			*/
/*					*/
/****************************************/
void 
CGageDlg::OnOK() 
{
	ExecuteFlag = FALSE;
	CDialog::OnOK();
}

/****************************************/
/*					*/
/*	Window procedure		*/
/*					*/
/****************************************/
LRESULT 
CGageDlg::WindowProc(
UINT 	message, 
WPARAM 	wParam, 
LPARAM 	lParam) 
{
	int	sts;

	if( message == WM_IMPORTGAGE){
		if( lParam){
			cEUDC = (UINT)wParam;
			nEUDC = (UINT)lParam;
			m_EditGage.Invalidate( FALSE);
			m_EditGage.UpdateWindow();
		}
		return (0L);
	}
	if( message == WM_IMPORTSTOP){
		ExecuteFlag = TRUE;
		EnableEUDC( FALSE);
		sts = Import(m_szUserFont, m_szBmpFile, m_szTtfFile,
			BITMAP_WIDTH, BITMAP_HEIGHT, SMOOTHLVL, m_bIsWin95EUDC);
		//
		// we import some glyphs and will not delete the link.
		//
		g_bKeepEUDCLink = TRUE;
		EnableEUDC( TRUE);

		if( sts)	return (0L);	// for debug
		return (0L);
	}
	return CDialog::WindowProc( message, wParam, lParam);
}

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CEditGage::CEditGage()
{
}

/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CEditGage::~CEditGage()
{
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void 
CEditGage::OnPaint()
{
	TCHAR	cEUDCBuf[10];
	TCHAR	nEUDCBuf[10];
	TCHAR	ViewBuf[30];
	CRect	EditGageRect;
	CRect	BrueGageRect, WhiteGageRect;
	CBrush	wBrush, bBrush;
	CPaintDC	dc( this);

	this->GetClientRect( &EditGageRect);

	if( !testtest){
		testtest = TRUE;
		::SendMessage( hDlg, WM_IMPORTSTOP, (WPARAM)0, (LPARAM)0);
	}else{
		if( nEUDC){
			wsprintf( cEUDCBuf, TEXT("%d"), cEUDC);
			wsprintf( nEUDCBuf, TEXT("%d"), nEUDC);
			lstrcpy(ViewBuf, cEUDCBuf);
			lstrcat(ViewBuf, TEXT(" /"));
			lstrcat(ViewBuf, nEUDCBuf);
			GetParent()->SetDlgItemText( IDC_IMPORT_STATIC, (LPCTSTR)ViewBuf);
			BrueGageRect.CopyRect( &EditGageRect);
			WhiteGageRect.CopyRect( &EditGageRect);
			BrueGageRect.right = (cEUDC*EditGageRect.Width()) / nEUDC;
			WhiteGageRect.left = BrueGageRect.right;

			bBrush.CreateSolidBrush(COLOR_BLUE);
			dc.FillRect( &BrueGageRect, &bBrush);
			bBrush.DeleteObject();

			wBrush.CreateStockObject( WHITE_BRUSH);
			dc.FillRect( &WhiteGageRect, &wBrush);
			wBrush.DeleteObject();
		}
	}
	if( cEUDC >= nEUDC){
		::SendMessage( hDlg, WM_COMMAND, (WPARAM)IDOK, (LPARAM)0);
	}
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_ONLBUTTONDOWN"	*/
/*					*/
/****************************************/
void 
CEditGage::OnLButtonDown(
UINT 	nFlags, 
CPoint 	point)
{
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_SETCURSOR"		*/
/*					*/
/****************************************/
BOOL 
CEditGage::OnSetCursor(
CWnd* 	pWnd, 
UINT 	nHitTest, 
UINT 	message)
{
	::SetCursor( AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return TRUE;
}

BEGIN_MESSAGE_MAP( CEditGage, CEdit)
	//{{AFX_MSG_MAP( CEditGage)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Send Import Message		*/
/*					*/
/****************************************/
BOOL 
SendImportMessage(
unsigned int 	cEudc,
unsigned int 	nEudc)
{
	MSG	msg;

	while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE)){
		if( msg.message == WM_QUIT) 
			break;
		TranslateMessage( &msg);
		DispatchMessage( &msg);
	}
	if( !ExecuteFlag){
		cEudc = 0;
		nEudc = 0;
		return FALSE;
	}
	SendMessage( hDlg, WM_IMPORTGAGE, (WPARAM)cEudc, (LPARAM)nEudc);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CGageDlg, CDialog)
	//{{AFX_MSG_MAP(CGageDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CEditGage::OnRButtonUp(UINT nFlags, CPoint point) 
{
}
