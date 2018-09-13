/**************************************************/
/*					                              */
/*					                              */
/*	Convert from bmp to ttf		                  */
/*		(Dialogbox)		                          */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"imprtdlg.h"
#include	"ttfstruc.h"
#include	"extfunc.h"
#include	"util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

TCHAR	UserFont[MAX_PATH];
TCHAR	EUDCTTF[MAX_PATH];
TCHAR	EUDCBMP[MAX_PATH];

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CImportDlg::CImportDlg( CWnd* pParent)
	: CDialog(CImportDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportDlg)
	//}}AFX_DATA_INIT
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_INITDIALOG"		*/
/*					*/
/****************************************/
BOOL
CImportDlg::OnInitDialog()
{
	CString	DlgTitle;

	CDialog::OnInitDialog();

//	Implement "?" in this dialogbox.
//	LONG WindowStyle = GetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE);
//	WindowStyle |= WS_EX_CONTEXTHELP;
//	SetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE, WindowStyle);

//	Set dialog title name.
	DlgTitle.LoadString( IDS_IMPORT_DLGTITLE);
	this->SetWindowText( DlgTitle);

	return TRUE;
}

/****************************************/
/*					*/
/*	COMMAND	"BROWSE"		*/
/*					*/
/****************************************/
void
CImportDlg::OnFileBrowse()
{
OPENFILENAME	ofn;
	CString	DlgTtl, DlgMsg;
	CString	sFilter;
	CWnd	*cWnd;
	TCHAR	chReplace;
	TCHAR 	szFilter[MAX_PATH];
	TCHAR	szFileName[MAX_PATH];
	TCHAR	szTitleName[MAX_PATH];
	TCHAR	szDirName[MAX_PATH];

	if( CountryInfo.LangID == EUDC_JPN){
//		Set filter of file( from string table)
		GetStringRes(szFilter, IDS_IMPORT_JAPAN_FILTER);
		int StringLength = lstrlen( szFilter);

		chReplace = szFilter[StringLength-1];
		for( int i = 0; szFilter[i]; i++){
			if( szFilter[i] == chReplace)
				szFilter[i] = '\0';
		}
		GetSystemWindowsDirectory( szDirName, sizeof(szDirName));
		lstrcpy( szFileName, TEXT("USERFONT.FON"));
		DlgTtl.LoadString( IDS_BROWSEUSER_DLGTITLE);

//		Set data in structure of OPENFILENAME
		ofn.lStructSize = sizeof( OPENFILENAME);
		ofn.hInstance = AfxGetInstanceHandle();
		ofn.hwndOwner = this->GetSafeHwnd();
		ofn.lpstrFilter = szFilter;
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 0;
		ofn.lpstrFile = szFileName;
		ofn.lpstrFileTitle = szTitleName;
		ofn.nMaxFileTitle = sizeof( szTitleName);
		ofn.nMaxFile = sizeof( szFileName);
		ofn.lpstrInitialDir = szDirName;
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR
			 | OFN_PATHMUSTEXIST;
		ofn.lpstrDefExt = NULL;
		ofn.lpstrTitle = DlgTtl;

		if( !GetOpenFileName( &ofn)){
			return;
		}

		memcpy( UserFont, ofn.lpstrFile, sizeof( UserFont));
		this->SetDlgItemText( IDC_BMP_IMPUT, ofn.lpstrFile);
#ifdef BUILD_ON_WINNT
        if( OExistUserFont( UserFont) != 1){
			OutputMessageBoxEx( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				AFX_IDP_FAILED_INVALID_PATH, TRUE, UserFont);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
        }
#endif // BUILD_ON_WINNT
		if( isW31JEUDCBMP( UserFont) != 1){
			OutputMessageBox( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				IDS_NOTUSERFONT_MSG, TRUE);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
		}
		cWnd = GetDlgItem( IDOK);
		GotoDlgCtrl( cWnd);
	}else if( CountryInfo.LangID == EUDC_CHT ||
		  CountryInfo.LangID == EUDC_CHS ){
//		Set filter of file( from string table)
		GetStringRes(szFilter, IDS_IMPORT_CHINA_FILTER);
		int StringLength = lstrlen( szFilter);

		chReplace = szFilter[StringLength-1];
		for( int i = 0; szFilter[i]; i++){
			if( szFilter[i] == chReplace)
				szFilter[i] = '\0';
		}
		GetSystemWindowsDirectory( szDirName, sizeof(szDirName));
		lstrcpy( szFileName, TEXT("*.*"));
		DlgTtl.LoadString( IDS_BROWSEUSER_DLGTITLE);

//		Set data in structure of OPENFILENAME
		ofn.lStructSize = sizeof( OPENFILENAME);
		ofn.hwndOwner = this->GetSafeHwnd();
		ofn.hInstance = AfxGetInstanceHandle();
		ofn.lpstrFilter = szFilter;
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 0;
		ofn.lpstrFileTitle = szTitleName;
		ofn.nMaxFileTitle = sizeof( szTitleName);
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = sizeof( szFileName);
		ofn.lpstrInitialDir = szDirName;
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR
			 | OFN_PATHMUSTEXIST;
		ofn.lpstrDefExt = NULL;
		ofn.lpstrTitle = DlgTtl;

		if( !GetOpenFileName( &ofn))
			return;

		memcpy( UserFont, ofn.lpstrFile, sizeof( UserFont));
		this->SetDlgItemText( IDC_BMP_IMPUT, ofn.lpstrFile);
#ifdef BUILD_ON_WINNT
        if( OExistUserFont( UserFont) != 1){
			OutputMessageBoxEx( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				AFX_IDP_FAILED_INVALID_PATH, TRUE, UserFont);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
        }
#endif // BUILD_ON_WINNT
		if( isETENBMP( UserFont) != 1){
			OutputMessageBox( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				IDS_NOTUSERFONT_MSG, TRUE);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
		}
		cWnd = GetDlgItem( IDOK);
		GotoDlgCtrl( cWnd);
	}
}

/****************************************/
/*					*/
/*	COMMAND	"IDOK"			*/
/*					*/
/****************************************/
void
CImportDlg::OnOK()
{
	TCHAR 	*FilePtr;
	CWnd	*cWnd;

	lstrcpy(EUDCTTF,SelectEUDC.m_File);
	lstrcpy( EUDCBMP, EUDCTTF);
	if(( FilePtr = Mytcsrchr( EUDCBMP, '.')) != NULL)
		*FilePtr = '\0';
	lstrcat( EUDCBMP, TEXT(".EUF"));
	if( !this->GetDlgItemText(IDC_BMP_IMPUT, UserFont, MAX_PATH)){
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_IMPORT_DLGTITLE,
			IDS_NOTUSERFONT_MSG, TRUE);

		cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
		GotoDlgCtrl( cWnd);
		return;
	}
	if( CountryInfo.LangID == EUDC_JPN){
#ifdef BUILD_ON_WINNT
        if( OExistUserFont( UserFont) != 1){
			OutputMessageBoxEx( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				AFX_IDP_FAILED_INVALID_PATH, TRUE, UserFont);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
        }
#endif // BUILD_ON_WINNT
		if( isW31JEUDCBMP( UserFont) != 1){
			OutputMessageBox( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				IDS_NOTUSERFONT_MSG, TRUE);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
		}
	}else{
#ifdef BUILD_ON_WINNT
        if( OExistUserFont( UserFont) != 1){
			OutputMessageBoxEx( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				AFX_IDP_FAILED_INVALID_PATH, TRUE, UserFont);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
        }
#endif // BUILD_ON_WINNT
		if( isETENBMP( UserFont) != 1){
			OutputMessageBox( this->GetSafeHwnd(),
				IDS_IMPORT_DLGTITLE,
				IDS_NOTUSERFONT_MSG, TRUE);

			cWnd = this->GetDlgItem( IDC_BMP_IMPUT);
			GotoDlgCtrl( cWnd);
			return;
		}
	}
	EndDialog(IDOK);
}

static DWORD aIds[] =
{
  IDC_STATICBMP,    IDH_EUDC_IMPOBMP,
	IDC_BMP_IMPUT,		IDH_EUDC_IMPOBMP,
	IDC_FILE_BROWSE,	IDH_EUDC_BROWSE,
	0,0
};

/****************************************/
/*					*/
/*	Window procedure		*/
/*					*/
/****************************************/
LRESULT
CImportDlg::WindowProc(
UINT 	message,
WPARAM 	wParam,
LPARAM 	lParam)
{/*
	if( message == WM_HELP){
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
			HelpPath, HELP_WM_HELP, (DWORD_PTR)(LPTSTR)aIds);
		return(0);
	}
	if( message == WM_CONTEXTMENU){
		::WinHelp((HWND)wParam, HelpPath,
			HELP_CONTEXTMENU, (DWORD_PTR)(LPTSTR)aIds);
		return(0);
	}
 */
	return CDialog::WindowProc(message, wParam, lParam);
}

BEGIN_MESSAGE_MAP(CImportDlg, CDialog)
	//{{AFX_MSG_MAP(CImportDlg)
	ON_BN_CLICKED(IDC_FILE_BROWSE, OnFileBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
