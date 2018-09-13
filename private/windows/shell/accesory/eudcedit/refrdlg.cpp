/**************************************************/
/*					                              */
/*					                              */
/*	Reference other characters	                  */
/*		(Dialog)		                          */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"refrdlg.h"
#include	"util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BOOL	CodeFocus;
BOOL	CharFocus;
BOOL	CompFinalized;
CHOOSEFONT	chf;
extern 	LOGFONT	ReffLogFont;
extern 	LOGFONT	EditLogFont;
extern 	BOOL	TitleFlag;
static 	BOOL 	CALLBACK ComDlg32DlgProc(HWND hDlg, UINT uMsg,
		WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditCharProc( HWND  hwnd, UINT  uMsg,
                                     WPARAM wParam, LPARAM  lParam);
HIMC hImcCode = NULL;

BEGIN_MESSAGE_MAP(CRefrDlg, CDialog)
	//{{AFX_MSG_MAP(CRefrDlg)
	ON_BN_CLICKED(IDC_BUTTOMFONT, OnClickedButtomfont)
	ON_EN_CHANGE(IDC_EDITCODE, OnChangeEditcode)
	ON_EN_SETFOCUS(IDC_EDITCODE, OnSetfocusEditcode)
	ON_EN_KILLFOCUS(IDC_EDITCODE, OnKillfocusEditcode)
	ON_EN_SETFOCUS(IDC_EDITCHAR, OnSetfocusEditchar)
	ON_EN_KILLFOCUS(IDC_EDITCHAR, OnKillfocusEditchar)
	ON_EN_CHANGE(IDC_EDITCHAR, OnChangeEditchar)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


LRESULT CALLBACK EditCharProc( HWND  hwnd, UINT  uMsg,
                               WPARAM wParam, LPARAM  lParam)
{
    switch (uMsg)
    {
        case WM_CHAR:
        {
        	//
	        // We always delete whatever in edit window before
	        // proceeding to avoid multiple characters in the
	        // window
	        //
		SetWindowText(hwnd, TEXT(""));
        	CompFinalized = TRUE;
            break;
        }
        case WM_IME_COMPOSITION:
        {
            if (lParam & CS_INSERTCHAR)
            {
                // This is KOR ime only.  We want to clear the edit
                // window when the first and only the first composition
                // char is entered.
                //

                if (CompFinalized)
                {
                    SetWindowText(hwnd, TEXT(""));
                }
        	    CompFinalized = FALSE;
            }
            break;
        }

	}

	return(AfxWndProc(hwnd, uMsg, wParam, lParam));
}

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CRefrDlg::CRefrDlg( CWnd* pParent)
	: CDialog(CRefrDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRefrDlg)
	//}}AFX_DATA_INIT
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_INITDIALOG"		*/
/*					*/
/****************************************/
BOOL
CRefrDlg::OnInitDialog()
{
	CString	DialogTitle;
	LOGFONT	LogFont;
	CWnd	*ViewWnd;
	CRect	CharViewRect;
	HWND 	hWndCode;
	HWND    hWndChar;

	CDialog::OnInitDialog();

//	Implement "?" in this dialog.
	LONG WindowStyle = GetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE);
	WindowStyle |= WS_EX_CONTEXTHELP;
	SetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE, WindowStyle);

//	Set Dialog title name.
	if( !TitleFlag)
		DialogTitle.LoadString( IDS_REFERENCE_DLGTITLE);
	else	DialogTitle.LoadString( IDS_CALL_DLGTITLE);
	this->SetWindowText( DialogTitle);

//	Subclass Dialog Control Item
	m_CodeList.SubclassDlgItem( ID_REFERCODE, this);
	m_RefListFrame1.SubclassDlgItem( IDC_LISTFRAME1, this);
	m_RefInfoFrame.SubclassDlgItem(  IDC_INFOFRAME, this);
	m_ColumnHeadingR.SubclassDlgItem( IDC_COLUMNHEADINGR, this);
	m_EditChar.SubclassDlgItem( IDC_EDITCHAR, this);
	hWndChar = this->GetDlgItem(IDC_EDITCHAR)->GetSafeHwnd();
	if (GetWindowLongPtr(hWndChar, GWLP_WNDPROC) == (LONG_PTR)AfxWndProc)
	{
		SetWindowLongPtr(hWndChar, GWLP_WNDPROC, (LONG_PTR)EditCharProc);
	}
	hWndCode = GetDlgItem(IDC_EDITCODE)->GetSafeHwnd();
	if (hWndCode && ::IsWindow(hWndCode))
	{
		hImcCode = ImmAssociateContext(hWndCode, NULL);
	}

	GetFont()->GetObject( sizeof(LOGFONT), &LogFont);
	m_CodeList.SysFFont.CreateFontIndirect( &LogFont);
	m_CodeList.CalcCharSize();
	m_CodeList.SendMessage( WM_SETFONT,
		(WPARAM)m_CodeList.SysFFont.GetSafeHandle(),
		MAKELPARAM( TRUE, 0));
	m_ColumnHeadingR.SysFFont.CreateFontIndirect( &LogFont);
	ViewWnd = this->GetDlgItem( IDC_EDITCHAR);
	ViewWnd->GetClientRect( &CharViewRect);
	if( CharViewRect.Width() >= CharViewRect.Height())
		vHeight = CharViewRect.Height() - 10;
	else	vHeight = CharViewRect.Width()  - 10;

	if( !TitleFlag){
		memcpy( &m_CodeList.rLogFont, &ReffLogFont, sizeof( LOGFONT));
		m_CodeList.rLogFont.lfHeight = vHeight;
		m_CodeList.ViewFont.CreateFontIndirect( &m_CodeList.rLogFont);
		
		if( m_CodeList.CharSize.cx >= m_CodeList.CharSize.cy)
			m_CodeList.rLogFont.lfHeight = m_CodeList.CharSize.cy-2;
		else	m_CodeList.rLogFont.lfHeight = m_CodeList.CharSize.cx-2;
		m_CodeList.CharFont.CreateFontIndirect( &m_CodeList.rLogFont);

		lstrcpy( (TCHAR *)FontName,
			(const TCHAR *)m_CodeList.rLogFont.lfFaceName);
		AdjustFontName();
		this->SetDlgItemText( IDC_EDITFONT, (LPTSTR)FontName);
	}else{
		memcpy( &m_CodeList.cLogFont, &EditLogFont, sizeof( LOGFONT));
		m_CodeList.cLogFont.lfHeight = vHeight;
		m_CodeList.ViewFont.CreateFontIndirect( &m_CodeList.cLogFont);

		if( m_CodeList.CharSize.cx >= m_CodeList.CharSize.cy)
			m_CodeList.cLogFont.lfHeight = m_CodeList.CharSize.cy-2;
		else	m_CodeList.cLogFont.lfHeight = m_CodeList.CharSize.cx-2;
		m_CodeList.CharFont.CreateFontIndirect( &m_CodeList.cLogFont);

		lstrcpy( (TCHAR *)FontName,
			(const TCHAR *)m_CodeList.cLogFont.lfFaceName);
		AdjustFontName();
		this->SetDlgItemText( IDC_EDITFONT, (LPTSTR)FontName);
	}
	m_CodeList.SetCodeRange();
	SetViewFont();
	m_CodeList.EnableScrollBar(SB_VERT, ESB_ENABLE_BOTH);
	m_CodeList.Invalidate( FALSE);
	m_CodeList.UpdateWindow();

	CodeFocus = FALSE;
	CharFocus = FALSE;
	CompFinalized=TRUE;
	this->SendDlgItemMessage(IDC_EDITCODE, EM_LIMITTEXT,
		(WPARAM)4, (LPARAM)0);
	this->SendDlgItemMessage(IDC_EDITCHAR, EM_LIMITTEXT,
		(WPARAM)1, (LPARAM)0);

	return TRUE;
}
void
CRefrDlg::OnDestroy()
{
	if (hImcCode)
	{
		HWND 	hWndCode;
		hWndCode = GetDlgItem(IDC_EDITCODE)->GetSafeHwnd();
		if (hWndCode && ::IsWindow(hWndCode))
		{
			ImmAssociateContext(hWndCode, hImcCode);
			hImcCode = NULL;
		}
	}
}

/****************************************/
/*					*/
/*	Set font on ViewEdit		*/
/*					*/
/****************************************/
void
CRefrDlg::SetViewFont()
{
	HWND	hWnd;

	hWnd = ::GetDlgItem( this->GetSafeHwnd(), IDC_EDITCHAR);
	::SendMessage( hWnd, WM_SETFONT,
		(WPARAM)m_CodeList.ViewFont.m_hObject, MAKELPARAM(TRUE,0));
}

/****************************************/
/*					*/
/*	COMMAND	"FONT"			*/
/*					*/
/****************************************/
void
CRefrDlg::OnClickedButtomfont()
{
	HDC hDC;

	hDC = ::GetDC( this->GetSafeHwnd());
	chf.hDC = ::CreateCompatibleDC( hDC);
	::ReleaseDC( this->GetSafeHwnd(), hDC);
	if( !TitleFlag){
		m_CodeList.rLogFont.lfHeight = 40;
		chf.lpLogFont = &m_CodeList.rLogFont;
	}else{
		m_CodeList.cLogFont.lfHeight = 40;
		chf.lpLogFont = &m_CodeList.cLogFont;
	}
	chf.lStructSize = sizeof(CHOOSEFONT);
	chf.hwndOwner = this->GetSafeHwnd();
	chf.rgbColors = GetSysColor(COLOR_WINDOWTEXT); //COLOR_BLACK;
	chf.lCustData = 0;
	chf.hInstance = AfxGetInstanceHandle();
	chf.lpszStyle = (LPTSTR)NULL;
	chf.nFontType = SCREEN_FONTTYPE;
	chf.lpfnHook = (LPCFHOOKPROC)(FARPROC)ComDlg32DlgProc;
	chf.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FORMATDLGORD31);
	chf.Flags = CF_SCREENFONTS | CF_NOSIMULATIONS | CF_ENABLEHOOK |
     			CF_ENABLETEMPLATE | CF_INITTOLOGFONTSTRUCT;

	if( ChooseFont( &chf ) == FALSE){
		::DeleteDC( chf.hDC);
        	return ;
   	}
	::DeleteDC( chf.hDC);
	m_CodeList.SetCodeRange();
	m_CodeList.ResetParam();
	this->SetDlgItemText( IDC_EDITCODE, TEXT(""));
	this->SetDlgItemText( IDC_EDITCHAR, TEXT(""));

	m_CodeList.CharFont.DeleteObject();
	m_CodeList.ViewFont.DeleteObject();
	if( !TitleFlag){
		lstrcpy( (TCHAR *)FontName,
			(const TCHAR *)m_CodeList.rLogFont.lfFaceName);
		AdjustFontName();
		this->SetDlgItemText( IDC_EDITFONT, FontName);

		m_CodeList.rLogFont.lfHeight  = vHeight;
		m_CodeList.rLogFont.lfQuality = PROOF_QUALITY;
		m_CodeList.ViewFont.CreateFontIndirect( &m_CodeList.rLogFont);

		if( m_CodeList.CharSize.cx >= m_CodeList.CharSize.cy)
			m_CodeList.rLogFont.lfHeight = m_CodeList.CharSize.cy-2;
		else	m_CodeList.rLogFont.lfHeight = m_CodeList.CharSize.cx-2;
		m_CodeList.CharFont.CreateFontIndirect( &m_CodeList.rLogFont);
	}else{
		lstrcpy(FontName,
			(const TCHAR *)m_CodeList.cLogFont.lfFaceName);
		AdjustFontName();
		this->SetDlgItemText( IDC_EDITFONT, FontName);

		m_CodeList.cLogFont.lfHeight = vHeight;
		m_CodeList.cLogFont.lfQuality = PROOF_QUALITY;
		m_CodeList.ViewFont.CreateFontIndirect( &m_CodeList.cLogFont);

		if( m_CodeList.CharSize.cx >= m_CodeList.CharSize.cy)
			m_CodeList.cLogFont.lfHeight = m_CodeList.CharSize.cy-2;
		else	m_CodeList.cLogFont.lfHeight = m_CodeList.CharSize.cx-2;
		m_CodeList.CharFont.CreateFontIndirect( &m_CodeList.cLogFont);
	}
	SetViewFont();
	m_CodeList.Invalidate( TRUE);
	m_CodeList.UpdateWindow();

	CWnd *cWnd = GetDlgItem( ID_REFERCODE);
	GotoDlgCtrl( cWnd);

   	return;
}

/****************************************/
/*					*/
/*	Adjust Font Name		*/
/*					*/
/****************************************/
void
CRefrDlg::AdjustFontName()
{
CClientDC	dc(this);
	CRect	ViewFontRect;
	CSize	FontNameSize, CharSize;
	int	i;

	CWnd *cWnd = GetDlgItem( IDC_EDITFONT);
	cWnd->GetClientRect( &ViewFontRect);

	GetTextExtentPoint32( dc.GetSafeHdc(), (const TCHAR *)FontName,
		lstrlen((const TCHAR *)FontName), &FontNameSize);

	if( ViewFontRect.Width() <= FontNameSize.cx){
		GetTextExtentPoint32( dc.GetSafeHdc(), TEXT("<<"), 2, &CharSize);
		i = ( ViewFontRect.Width() /CharSize.cx) * 2;
		FontName[i-2] = '.';
		FontName[i-1] = '.';
		FontName[i] = '\0';
	}	
}

/****************************************/
/*					*/
/*	jump Reference code		*/	
/*					*/
/****************************************/
void CRefrDlg::JumpReferCode()
{
	if( !m_CodeList.CodeButtonClicked())
	{
		if (CharFocus && !CompFinalized)
		{
			//
			// We want to cancel ime composition with wParam = 0, lParam
			// contains CS_INSERTCHAR.
			//
			this->SendDlgItemMessage(IDC_EDITCHAR,
                                     WM_IME_COMPOSITION,
                                     0,
                                     CS_INSERTCHAR | CS_NOMOVECARET |
                                     GCS_COMPSTR | GCS_COMPATTR);
		}
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_ILLEGALCODE_TITLE,
			IDS_ILLEGALCODE_MSG, TRUE);

	}else{

		if (CharFocus)
		{
		//
		// We don't want to highlight an interim KOR IME composition.
		//
			if (CompFinalized)
			{
				this->SendDlgItemMessage(IDC_EDITCHAR, EM_SETSEL, 0, -1);
			}
		}
		else
		{
			this->SendDlgItemMessage(IDC_EDITCODE, EM_SETSEL, 0, -1);
		}
	}
}

/****************************************/
/*					*/
/*	COMMAND "IDOK"			*/
/*					*/
/****************************************/
void
CRefrDlg::OnOK()
{
	if( !m_CodeList.SelectCode){
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_REFERENCE_DLGTITLE,
			IDS_NOTSELCHARACTER_MSG, TRUE);
		return;
	}
	if( !TitleFlag){
		memcpy( &ReffLogFont, &m_CodeList.rLogFont, sizeof( LOGFONT));
	}else{
		memcpy( &EditLogFont, &m_CodeList.cLogFont, sizeof( LOGFONT));
	}
	CDialog::OnOK();
}

/****************************************/
/*					*/
/*	COMMAND "IDCANCEL"		*/
/*					*/
/****************************************/
void
CRefrDlg::OnCancel()
{
 	m_CodeList.SelectCode = 0;		

	CDialog::OnCancel();
}

/****************************************/
/*					*/
/*	MESSAGE "EM_CHANGE"		*/
/*					*/
/****************************************/
void CRefrDlg::OnChangeEditcode()
{
	TCHAR	EditCode[5];
	int i;

	EditCode[0] = '\0';
	this->GetDlgItemText(IDC_EDITCODE, EditCode, sizeof(EditCode));
	
	for (i=0; i<lstrlen(EditCode); i++)
	{
		if ( EditCode[i] <  TEXT('0') ||
		     (EditCode[i] > TEXT('9') && EditCode[i] < TEXT('A')) ||
		     (EditCode[i] > TEXT('F') && EditCode[i] < TEXT('a')) ||
		     EditCode[i] > TEXT('f'))
		{
			OutputMessageBox( this->GetSafeHwnd(),
								IDS_ILLEGALCODE_TITLE,
								IDS_ILLEGALCODE_MSG, TRUE);
			this->SendDlgItemMessage(IDC_EDITCODE, EM_SETSEL, 0, -1);
			return;
		}
	}
		
	if( lstrlen( EditCode) == 4 && CodeFocus)
	{
		JumpReferCode();
	}
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_SETFOCUS"		*/
/*					*/
/****************************************/
void
CRefrDlg::OnSetfocusEditcode()
{
	this->SendDlgItemMessage(IDC_EDITCODE, EM_SETSEL, 0, -1);
	CodeFocus = TRUE;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_KILLFOCUS"		*/
/*					*/
/****************************************/
void
CRefrDlg::OnKillfocusEditcode()
{
	CodeFocus = FALSE;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_SETFOCUS"		*/
/*					*/
/****************************************/
void
CRefrDlg::OnSetfocusEditchar()
{
	if (CompFinalized)
	{
	    this->SendDlgItemMessage( IDC_EDITCHAR, EM_SETSEL, 0, -1);
	}
	CharFocus = TRUE;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_KILLFOCUS"		*/
/*					*/
/****************************************/
void
CRefrDlg::OnKillfocusEditchar()
{
	CharFocus = FALSE;
}

/****************************************/
/*					*/
/*	MESSAGE	"EM_CHANGE"		*/
/*					*/
/****************************************/
void
CRefrDlg::OnChangeEditchar()
{
	WCHAR	EditChar[5];

	EditChar[0]=TEXT('\0');
#ifdef UNICODE
  ::GetDlgItemTextW(this->GetSafeHwnd(),IDC_EDITCHAR, (LPWSTR)EditChar, sizeof(EditChar));
#else
  CHAR eChar[4];
  int nchar = ::GetDlgItemText(this->GetSafeHwnd(),IDC_EDITCHAR, (LPSTR)eChar, sizeof(eChar));
  MultiByteToWideChar(CP_ACP, 0, eChar, nchar, EditChar, sizeof(EditChar));
#endif

	if( CharFocus && EditChar[0] != TEXT('\0') )
	{
		int iPos = 1;
/*
#ifndef UNICODE
		if (IsDBCSLeadByte(EditChar[0]))
		{
			iPos = 2;
		}
#endif
*/
		EditChar[iPos]=TEXT('\0');
		JumpReferCode();
	}
}

/****************************************/
/*					*/
/*	Callback function		*/
/*					*/
/****************************************/
static BOOL CALLBACK
ComDlg32DlgProc(
HWND 	hDlg,
UINT 	uMsg,
WPARAM 	wParam,
LPARAM 	lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
 			long	WindowStyle;

			WindowStyle = GetWindowLong( hDlg, GWL_EXSTYLE);
			WindowStyle |= WS_EX_CONTEXTHELP;
			SetWindowLong( hDlg, GWL_EXSTYLE, WindowStyle);
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

static DWORD aIds[] =
{
	ID_REFERCODE,	IDH_EUDC_REFLIST,
	IDC_COLUMNHEADINGR,	IDH_EUDC_REFLIST,
	IDC_STATICC,	IDH_EUDC_REFCODE,
	IDC_EDITCODE,	IDH_EUDC_REFCODE,
	IDC_STATICS,	IDH_EUDC_REFCHAR,
	IDC_EDITCHAR, 	IDH_EUDC_REFCHAR,
	IDC_STATICF,	IDH_EUDC_REFFONT,
	IDC_EDITFONT,	IDH_EUDC_REFFONT,
	IDC_INFOFRAME,	IDH_EUDC_REFFONT,
	IDC_BUTTOMFONT,	IDH_EUDC_FONT,
//	IDOK, IDH_EUDC_OK,
//	IDCANCEL, IDH_EUDC_CANCEL,
	0,0
};

static DWORD aIdsCall[] =
{
	ID_REFERCODE,	IDH_EUDC_CALLLIST,
	IDC_COLUMNHEADINGR,	IDH_EUDC_CALLLIST,
	IDC_STATICC,	IDH_EUDC_CALLCODE,
	IDC_EDITCODE,	IDH_EUDC_CALLCODE,
	IDC_STATICS,	IDH_EUDC_CALLCHAR,
	IDC_EDITCHAR, 	IDH_EUDC_CALLCHAR,
	IDC_STATICF,	IDH_EUDC_CALLFONT,
	IDC_EDITFONT,	IDH_EUDC_CALLFONT,
	IDC_INFOFRAME,	IDH_EUDC_CALLFONT,
	IDC_BUTTOMFONT,	IDH_EUDC_FONT,
//	IDOK, IDH_EUDC_OK,
//	IDCANCEL, IDH_EUDC_CANCEL,
	0,0
};

/****************************************/
/*					*/
/*	Window procedure		*/
/*					*/
/****************************************/
LRESULT
CRefrDlg::WindowProc(
UINT 	message,
WPARAM 	wParam,
LPARAM 	lParam)
{
	if( message == WM_HELP){
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
			HelpPath, HELP_WM_HELP, (DWORD_PTR)(LPTSTR)(TitleFlag ? aIdsCall:aIds));
		return(0);
	}
	if( message == WM_CONTEXTMENU){
		::WinHelp((HWND)wParam, HelpPath,
			HELP_CONTEXTMENU, (DWORD_PTR)(LPTSTR)(TitleFlag ? aIdsCall : aIds));
		return(0);
	}
	return CDialog::WindowProc( message, wParam, lParam);
}
