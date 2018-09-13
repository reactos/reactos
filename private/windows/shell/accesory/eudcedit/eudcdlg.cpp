/**************************************************/
/*						                          */
/*						                          */
/*	SelectCode... EUDC Editor Dialog	          */
/*						                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"eudcdlg.h"
#include 	"registry.h"
#include	"util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

typedef struct _tagRegistDef{
	TCHAR	RegFont[LF_FACESIZE];
	TCHAR	RegFile[MAX_PATH];
	BOOL	RegFlg;
} REGISTDEF;

static 	REGISTDEF	RegistDef;	
BEGIN_MESSAGE_MAP(CEudcDlg, CDialog)
	//{{AFX_MSG_MAP(CEudcDlg)
	ON_CBN_SELCHANGE(IDC_CHINARANGE, OnSelchangeChinarange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Constructor			*/
/*					*/
/****************************************/
CEudcDlg::CEudcDlg( CWnd* pParent)
	: CDialog(CEudcDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEudcDlg)
	//}}AFX_DATA_INIT
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_INITDIALOG"		*/
/*					*/
/****************************************/
BOOL
CEudcDlg::OnInitDialog()
{
	LOGFONT	LogFont;
	CString	DialogTitle;		
	TCHAR	SBuf[5], EBuf[5], CodeBuf[15];
	TCHAR	SysFace[LF_FACESIZE];
	long	WindowStyle;

	CDialog::OnInitDialog();

	WindowStyle = GetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE);
	WindowStyle |= WS_EX_CONTEXTHELP;
	SetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE, WindowStyle);

	m_EditList.SubclassDlgItem( IDC_CUSTOMEDIT, this);
	m_ViewChar.SubclassDlgItem( IDC_CUSTOMCHR, this);
#ifdef BUILD_ON_WINNT
	m_ColumnHeading.SubclassDlgItem( IDC_COLUMNHEADING, this);
#endif //BUILD_ON_WINNT
	m_Frame.SubclassDlgItem( IDC_CUSTOMFRAME, this);
	m_ListFrame.SubclassDlgItem( IDC_CUSTOMLISTFRAME, this);
	m_InfoFrame2.SubclassDlgItem( IDC_CUSTOMFRAME2, this);
	m_InfoFrame3.SubclassDlgItem( IDC_CUSTOMFRAME3, this);
	m_StaticRange.SubclassDlgItem( IDC_STATICRANGE, this);
	m_ComboRange.SubclassDlgItem( IDC_CHINARANGE, this);

	if( CountryInfo.LangID != EUDC_KRW || CountryInfo.bUnicodeMode){
		m_EditList.EnableScrollBar( SB_VERT, ESB_ENABLE_BOTH);
	}else	m_EditList.ShowScrollBar( SB_VERT, FALSE);

	AdjustFileName();
	AdjustFontName();
	this->SetDlgItemText( IDC_SETFONTNAME, FontName);
	this->SetDlgItemText( IDC_SETFILENAME, FileName);
	if( WorRFlag)
		DialogTitle.LoadString( IDS_REGISTAS_DLGTITLE);
	else	DialogTitle.LoadString( IDS_SELECTCODE_DLGTITLE);
	this->SetWindowText( DialogTitle);

	m_EditList.cRange = CountryInfo.CurrentRange;
	if( CountryInfo.nRange > 2 && !CountryInfo.bUnicodeMode){
	    m_StaticRange.ShowWindow( SW_SHOWNORMAL);
		m_ComboRange.ShowWindow( SW_SHOWNORMAL);
		m_StaticRange.UpdateWindow();
		m_ComboRange.UpdateWindow();

		for( int i = 0; i < CountryInfo.nRange-1; i++){
			wsprintf( SBuf, TEXT("%X"), CountryInfo.sRange[i]);
			wsprintf( EBuf, TEXT("%X"), CountryInfo.eRange[i]);
			lstrcpy( CodeBuf, SBuf);
			lstrcat( CodeBuf, TEXT(" - "));
			lstrcat( CodeBuf, EBuf);
			m_ComboRange.InsertString( i, CodeBuf);
		}
		m_ComboRange.DeleteString( CountryInfo.nRange-1);
		m_ComboRange.SetCurSel( CountryInfo.CurrentRange);
	}

//	Set font for output character code
	GetFont()->GetObject( sizeof( LOGFONT), &LogFont);
	m_EditList.SysFFont.CreateFontIndirect( &LogFont);
	m_EditList.CalcCharSize();
#ifdef BUILD_ON_WINNT
//  Set font for the column heading
	m_ColumnHeading.SysFFont.CreateFontIndirect( &LogFont);
#endif //BUILD_ON_WINNT

//	Set font for output character
	memset( &LogFont, 0, sizeof( LogFont));
	if( m_EditList.CharSize.cx >= m_EditList.CharSize.cy)
		LogFont.lfHeight = m_EditList.CharSize.cy-2;
	else	LogFont.lfHeight = m_EditList.CharSize.cx-2;
	LogFont.lfCharSet = (BYTE)CountryInfo.CharacterSet;
	
  GetStringRes((TCHAR *)SysFace, IDS_SYSTEMEUDCFONT_STR);
  if( !lstrcmp(SelectEUDC.m_Font, (const TCHAR *)SysFace) )
  {
    lstrcpy (LogFont.lfFaceName, TEXT("System"));
	}else{
    lstrcpy( LogFont.lfFaceName, SelectEUDC.m_Font);
	}
	if( !m_EditList.EUDCFont.CreateFontIndirect( &LogFont))
		return FALSE;
//fix for 261529
	if( !m_ViewChar.EUDCFont.CreateFontIndirect( &LogFont))
		return FALSE;

	return TRUE;
}

/****************************************/
/*		Deal with "OK"		*/
/****************************************/
void CEudcDlg::OnOK()
{
	if( !m_EditList.SelectCode){
		if( WorRFlag){
			OutputMessageBox( this->GetSafeHwnd(),
				IDS_REGISTAS_DLGTITLE,
				IDS_NOTSELCHARACTER_MSG, TRUE);
		}else{
			OutputMessageBox( this->GetSafeHwnd(),
				IDS_SELECTCODE_DLGTITLE,
				IDS_NOTSELCHARACTER_MSG, TRUE);
		}
		return;
	}
	RegistRegFont();
	EndDialog(IDOK);
}
void CEudcDlg::OnCancel()
{
	RegistRegFont();
	CDialog::OnCancel();
}
/****************************************/
/*	Take off ".TTF" from filename	*/
/****************************************/
void CEudcDlg::AdjustFileName()
{
	TCHAR	*FilePtr;

	lstrcpy(FileName, SelectEUDC.m_FileTitle);
	if(( FilePtr = Mytcsrchr( FileName, '.')) != NULL)
		*FilePtr = '\0';	

#ifdef BUILD_ON_WINNT
    /*
     * Trim the string more better way...
     */
    CClientDC   dc(this);
	CRect	    ViewFileRect;
	CSize       FileNameSize;
	CWnd       *cWnd = GetDlgItem( IDC_SETFILENAME);
    INT         MaxChars;
    INT         cchString = lstrlen(FileName);

	cWnd->GetClientRect( &ViewFileRect);

	GetTextExtentExPoint( dc.GetSafeHdc(), FileName,
		cchString, ViewFileRect.Width(), &MaxChars, NULL ,&FileNameSize);

    if( cchString > MaxChars) {
        INT Chars = 0;

        FilePtr = FileName;
        while (Chars < (MaxChars-3)) {
            FilePtr++; Chars++;
        }

        for (; Chars < MaxChars; Chars++) {
            FileName[Chars] = TEXT('.');
        }
        FileName[Chars] = TEXT('\0');
    }
#else
	if( strlen((const char *)FileName) > 11){
		FileName[11] = '.';
		FileName[12] = '.';
		FileName[13] = '\0';
	}
#endif // BUILD_ON_WINNT

	return;
}

/****************************************/
/*	Adjust EUDC Font name		*/
/****************************************/
void CEudcDlg::AdjustFontName()
{
CClientDC	dc(this);
	CRect	ViewFontRect;
	CSize	FontNameSize, CharSize;
	int	i;

	lstrcpy(FontName, SelectEUDC.m_Font);

	CWnd *cWnd = GetDlgItem( IDC_SETFONTNAME);
	cWnd->GetClientRect( &ViewFontRect);

	GetTextExtentPoint32( dc.GetSafeHdc(), FontName,
		lstrlen(FontName), &FontNameSize);

	if( ViewFontRect.Width() <= FontNameSize.cx){
		GetTextExtentPoint32( dc.GetSafeHdc(), TEXT("<<"), 2, &CharSize);
		i = ( ViewFontRect.Width() /CharSize.cx) * 2;
		FontName[i-2] = '.';
		FontName[i-1] = '.';
		FontName[i] = '\0';
	}
}

/****************************************/
/*	Change EUDC code range(CHINESE)	*/
/****************************************/
void CEudcDlg::OnSelchangeChinarange()
{
	int nIndex = m_ComboRange.GetCurSel();

	m_EditList.cRange = nIndex;
	m_EditList.SetEUDCCodeRange( nIndex);
	this->SetDlgItemText( IDC_CUSTOMCHR, TEXT(""));
	this->SetDlgItemText( IDC_CODECUST,  TEXT(""));
	m_EditList.Invalidate();
	m_EditList.UpdateWindow();

	CWnd *cWnd = this->GetDlgItem( IDC_CUSTOMEDIT);
	this->GotoDlgCtrl( cWnd);
}

void CEudcDlg::RegistRegFont()
{
	if( RegistDef.RegFlg){
		EnableEUDC(FALSE);
		RegistTypeFace( RegistDef.RegFont,  RegistDef.RegFile);
		EnableEUDC(TRUE);
	}
}

static DWORD aIds[] =
{
    IDC_CUSTOMEDIT, IDH_EUDC_SELLIST,
    IDC_COLUMNHEADING,  IDH_EUDC_SELLIST,
    IDC_CODECUST,   IDH_EUDC_SELCODE,
    IDC_CODE_NAME,  IDH_EUDC_SELCODE,
    IDC_CUSTOMFRAME,    IDH_EUDC_SELCODE,
    IDC_SETFONTNAME,IDH_EUDC_SELFONT,
    IDC_FONT_NAME, IDH_EUDC_SELFONT,
    IDC_CUSTOMFRAME3, IDH_EUDC_SELFONT,
    IDC_SETFILENAME,IDH_EUDC_SELFILE,
    IDC_FILE_NAME, IDH_EUDC_SELFILE,
    IDC_CUSTOMFRAME2, IDH_EUDC_SELFILE,
    IDC_CUSTOMCHR, IDH_EUDC_SELCHAR,
    IDC_STATICRANGE, IDH_EUDC_SELRANGE,
    IDC_CHINARANGE, IDH_EUDC_SELRANGE,
	0,0
};

static DWORD aIdsReg[] =
{
    IDC_CUSTOMEDIT, IDH_EUDC_REGLIST,
    IDC_COLUMNHEADING,  IDH_EUDC_REGLIST,
    IDC_CODECUST,   IDH_EUDC_REGCODE,
    IDC_CODE_NAME,  IDH_EUDC_REGCODE,
    IDC_CUSTOMFRAME,    IDH_EUDC_REGCODE,
    IDC_SETFONTNAME,IDH_EUDC_REGFONT,
    IDC_FONT_NAME, IDH_EUDC_REGFONT,
    IDC_CUSTOMFRAME3, IDH_EUDC_REGFONT,
    IDC_SETFILENAME,IDH_EUDC_REGFILE,
    IDC_FILE_NAME, IDH_EUDC_REGFILE,
    IDC_CUSTOMFRAME2, IDH_EUDC_REGFILE,
    IDC_CUSTOMCHR, IDH_EUDC_REGCHAR,
    IDC_STATICRANGE, IDH_EUDC_REGRANGE,
    IDC_CHINARANGE, IDH_EUDC_REGRANGE,
	0,0
};
/****************************************/
/*	Customdlg Window Procedure	*/
/****************************************/
LRESULT CEudcDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if( message == WM_VIEWUPDATE){
		m_ViewChar.Invalidate( FALSE);
		m_ViewChar.UpdateWindow();
		return(0);
	}
	if( message == WM_HELP){
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
			HelpPath, HELP_WM_HELP, (DWORD_PTR)(LPTSTR)(WorRFlag ? aIdsReg : aIds));
		return(0);
	}
	if( message == WM_CONTEXTMENU){
		::WinHelp((HWND)wParam, HelpPath,
			HELP_CONTEXTMENU, (DWORD_PTR)(LPTSTR) (WorRFlag ? aIdsReg : aIds));
		return(0);
	}
	return CDialog::WindowProc( message, wParam, lParam);
}


