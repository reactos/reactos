/**************************************************/
/*						                          */
/*						                          */
/*		Registry Process		                  */
/*		  (Dialog)			                      */
/*						                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"assocdlg.h"
#include 	"registry.h"
#include	"ttfstruc.h"
#include	"extfunc.h"
#include	"util.h"
#include	"gagedlg.h"

#define		LSPACE	2
#define		RSPACE	2
#define		LCSPACE 17
#define		NUMITEM	3
#define		DBCSCHK	0
#define		EUDCCHK 1

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

SELECTEUDC	SelectEUDC;

static HDC	hDC;
static HWND	hWnd;
static HICON	tIcon;
static HGLOBAL	hMem;
static int	nTypeFace, nIdx, CheckFlg;
static TCHAR	FontName[LF_FACESIZE];
static LPASSOCIATIONREG	lpAssociationReg;

static int 	CheckTTF( LOGFONT);
static BOOL	CheckCharSet( int CharSet);
static BOOL 	IsEUDCTTF( TCHAR *ttffile);
static BOOL 	CheckFileName( LPTSTR FileStr);

static int CALLBACK EnumFontFamProc(
	ENUMLOGFONT FAR*, NEWTEXTMETRIC FAR*, int, LPARAM);

static int CALLBACK EnumFontNumber(
	ENUMLOGFONT FAR*, NEWTEXTMETRIC FAR*, int, LPARAM);

extern BOOL 	SetCountryInfo( UINT LocalCP);



/****************************************/
/*					*/
/*	Constructor			*/
/*					*/
/****************************************/
CAssocDlg::CAssocDlg(
CWnd* 	pParent) : CDialog(CAssocDlg::IDD, pParent)
{
	m_pParent = pParent;
	//{{AFX_DATA_INIT(CAssocDlg)
	//}}AFX_DATA_INIT
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_INITDIALOG"		*/
/*					*/
/****************************************/
BOOL
CAssocDlg::OnInitDialog()
{
	CString	DlgTtl;
 	long	WindowStyle;

	CDialog::OnInitDialog();

	WindowStyle = GetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE);
	WindowStyle |= WS_EX_CONTEXTHELP;
	SetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE, WindowStyle);

	DlgTtl.LoadString( IDS_ASSOCIATE_DLGTITLE);
	this->SetWindowText( DlgTtl);

//	Set Dialog subclass
 	m_RegListBox.SubclassDlgItem( IDC_REGISTLIST, this);
	m_RegListBox.EnableScrollBar( SB_VERT, ESB_DISABLE_BOTH);

	CheckFlg = EUDCCHK;
	this->SendDlgItemMessage( IDC_RADIO_SYSTEM, BM_SETCHECK, (WPARAM)1, 0);
	hMem = NULL;
	if( !SetAssociationFontType()){
		this->MessageBox( NotMemMsg, NotMemTtl, MB_OK |
			MB_ICONHAND | MB_SYSTEMMODAL);

		return FALSE;
	}
	tIcon = AfxGetApp()->LoadIcon( IDI_TRUETYPE);
	return TRUE;
}

/****************************************/
/*					*/
/*	Set TTF and WIFE font		*/
/*					*/
/****************************************/
BOOL
CAssocDlg::SetAssociationFontType()
{
LPASSOCIATIONREG	lpAssociationRegTmp;
	int	aFontCount[] = {0,0,0};
	int	StartIdx = 0;
	int	sts;

	nTypeFace = nIdx = 0;
	hWnd = this->GetSafeHwnd();
	hDC  = ::GetDC( hWnd);

	sts = EnumFontFamilies( hDC, NULL,
	  	(FONTENUMPROC)EnumFontNumber, (LPARAM)aFontCount);
	if( hMem != NULL){
		GlobalUnlock( hMem);
		GlobalFree( hMem);
	}
	hMem = GlobalAlloc(GHND, sizeof(ASSOCIATIONREG) * nTypeFace);
	if( hMem == NULL){
		::ReleaseDC( hWnd, hDC);
		return FALSE;
	}

  lpAssociationReg = (LPASSOCIATIONREG)GlobalLock( hMem);
  lpAssociationRegTmp = lpAssociationReg;
  sts = EnumFontFamilies( hDC, NULL,
    (FONTENUMPROC)EnumFontFamProc, (LPARAM)aFontCount);
  ::ReleaseDC( hWnd, hDC);

  lpAssociationRegTmp = lpAssociationReg;
	for( int i = 0; i < nTypeFace; i++){
		sts = m_RegListBox.AddString(
			(LPCTSTR)lpAssociationRegTmp->szFaceName);
    m_RegListBox.SetItemData(sts, i);
		if( sts == LB_ERR || sts == LB_ERRSPACE){
			GlobalUnlock( hMem);
			GlobalFree( hMem);
			return FALSE;
		}
		if( !lstrcmp(lpAssociationRegTmp->szFaceName, SelectEUDC.m_Font)){
			StartIdx = i;
		}
		lpAssociationRegTmp++;
	}
	m_RegListBox.SetCurSel( StartIdx);

	return TRUE;
}

/****************************************/
/*					*/
/*		Callback 		*/
/*					*/
/****************************************/
static int
CALLBACK EnumFontNumber(
ENUMLOGFONT FAR	*lplf,
NEWTEXTMETRIC FAR* lptm,
int 	FontType,
LPARAM 	lParam)
{
	int 	sts;

	if( FontType == TRUETYPE_FONTTYPE){
		sts = CheckTTF( lplf->elfLogFont);
		if( sts == 1 && CheckCharSet( lplf->elfLogFont.lfCharSet)){
			nTypeFace++;
		}else if( sts == -1){
			return 0;
		}else	;
	}else if( FontType == 0x8 && lplf->elfLogFont.lfFaceName[0] != '@' &&
		  CheckCharSet( lplf->elfLogFont.lfCharSet)){
		nTypeFace++;
	}

	return 1;
}
/****************************************/
/*					*/
/*		Callback 		*/
/*					*/
/****************************************/
static int
CALLBACK EnumFontFamProc(
ENUMLOGFONT FAR	*lplf,
NEWTEXTMETRIC FAR* lptm,
int 	FontType,
LPARAM 	lParam)
{
  LPASSOCIATIONREG	lpAssociationRegTmp;
	TCHAR		FileTitle[MAX_PATH];
	TCHAR		*FilePtr;
	int		  sts;
  TCHAR   FileSbstName[LF_FACESIZE];

	if( FontType == TRUETYPE_FONTTYPE){
		sts = CheckTTF( lplf->elfLogFont);
		if( sts == 1 && CheckCharSet( lplf->elfLogFont.lfCharSet)){
      FindFontSubstitute(lplf->elfLogFont.lfFaceName, FileSbstName);
			lpAssociationRegTmp = lpAssociationReg + nIdx;
			lstrcpy((TCHAR *)lpAssociationRegTmp->szFaceName,
				FileSbstName);

			if(!InqTypeFace(lpAssociationRegTmp->szFaceName,
			    lpAssociationRegTmp->szFileName,
			    sizeof( lpAssociationRegTmp->szFileName))){
				GetStringRes(
				(TCHAR *)lpAssociationRegTmp->szFileName,
				IDS_NOTSELECT_STR);

				lstrcpy( lpAssociationRegTmp->szFileTitle,
				       lpAssociationRegTmp->szFileName);
			}else{
				lstrcpy(FileTitle,lpAssociationRegTmp->szFileName);
				if(( FilePtr=Mytcsrchr( FileTitle, '\\')) != NULL)
					FilePtr++;

				else{
					FilePtr = Mytcsrchr( FileTitle,':');
					if( FilePtr != NULL){
						FilePtr++;
					}else 	FilePtr = FileTitle;
				}
				lstrcpy(lpAssociationRegTmp->szFileTitle,
				       FilePtr);
			}
			lpAssociationRegTmp->FontTypeFlg = TRUE;
			lpAssociationRegTmp->UpdateFlg = FALSE;
			nIdx++;
		}else if( sts == -1){
			return 0;
		}else	;
	}else if( FontType == 0x8 && lplf->elfLogFont.lfFaceName[0] != '@' &&
		  CheckCharSet( lplf->elfLogFont.lfCharSet)){
    FindFontSubstitute(lplf->elfLogFont.lfFaceName, FileSbstName);
		lpAssociationRegTmp = lpAssociationReg + nIdx;

    lstrcpy((TCHAR *)lpAssociationRegTmp->szFaceName,
			FileSbstName);

		if( !InqTypeFace(lpAssociationRegTmp->szFaceName,
		   lpAssociationRegTmp->szFileName,
		    sizeof(lpAssociationRegTmp->szFileName))){
			GetStringRes((TCHAR *)lpAssociationRegTmp->szFileName,
				IDS_NOTSELECT_STR);
			lstrcpy(lpAssociationRegTmp->szFileTitle,
			       lpAssociationRegTmp->szFileName);
		}else{
			lstrcpy(FileTitle,
			       lpAssociationRegTmp->szFileName);
			if(( FilePtr=Mytcsrchr( FileTitle, '\\')) != NULL)
				FilePtr++;
			else{
				FilePtr = Mytcsrchr( FileTitle,':');
				if( FilePtr != NULL){
					FilePtr++;
				}else 	FilePtr = FileTitle;
			}
			lstrcpy(lpAssociationRegTmp->szFileTitle,
			       FilePtr);
		}
		lpAssociationRegTmp->FontTypeFlg = FALSE;
		lpAssociationRegTmp->UpdateFlg = FALSE;
		nIdx++;
	}
	return 1;
}

/****************************************/
/*					*/
/*	Check Character Set		*/
/*					*/
/****************************************/
static BOOL
CheckCharSet(
int 	CharSet)
{
	if( CountryInfo.CharacterSet != CharSet)
		return FALSE;
	else	return TRUE;
}

/****************************************/
/*					*/
/*	COMMAND 	"IDOK"		*/
/*					*/
/****************************************/
void
CAssocDlg::OnOK()
{
  LPASSOCIATIONREG	lpAssociationRegTmp;
	TCHAR	FileList[MAX_PATH];	
	TCHAR	TTFPath[MAX_PATH];
	TCHAR	BMPPath[MAX_PATH];
	TCHAR	*FilePtr;

  int	nIndex = m_RegListBox.GetCurSel();
	if( nIndex == -1){
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_ASSOCIATE_DLGTITLE,
			IDS_NOTSELTYPEFACE_MSG, TRUE);
		m_RegListBox.SetFocus();
		return;
	}
	EnableEUDC( FALSE);
	lpAssociationRegTmp = lpAssociationReg;
	GetStringRes( FileList, IDS_NOTSELECT_STR);
  nIndex = (int)m_RegListBox.GetItemData(nIndex);
	for( int i = 0; i < nTypeFace; i++)
  {
	  if( i == nIndex)
    {
      //
      // Treat as an error, if the eudc font file name is not
      // specified, yet.
      //
      if( !lstrcmp((LPCTSTR)lpAssociationRegTmp->szFileName,FileList) && 
				  (CheckFlg == DBCSCHK))
      {
        this->SendMessage(WM_COMMAND, IDC_MODIFY, NULL);
        if( !lstrcmp((LPCTSTR)lpAssociationRegTmp->szFileName,FileList))
        {
           m_RegListBox.SetFocus();
             EnableEUDC(TRUE);
		         return;
        }
      }

		  lstrcpy(SelectEUDC.m_Font, lpAssociationRegTmp->szFaceName);
			lstrcpy(SelectEUDC.m_File, lpAssociationRegTmp->szFileName);
			lstrcpy(SelectEUDC.m_FileTitle, lpAssociationRegTmp->szFileTitle);
			SelectEUDC.m_FontTypeFlg = lpAssociationRegTmp->FontTypeFlg;
		}
		if(!lstrcmp((LPCTSTR)lpAssociationRegTmp->szFileName,FileList)){
      if (lpAssociationRegTmp->UpdateFlg)
				  DeleteReg(lpAssociationRegTmp->szFaceName);
			lpAssociationRegTmp++;
			continue;
		}
		lstrcpy( (LPTSTR)TTFPath, (LPCTSTR)lpAssociationRegTmp->szFileName);
		lstrcpy( BMPPath, TTFPath);
		if(( FilePtr = Mytcsrchr( BMPPath, '.')) != NULL)
				*FilePtr = '\0';
		lstrcat( BMPPath, TEXT(".EUF"));
	
		if( OExistTTF( TTFPath))
    {
#if (WINVER >= 0x0500)
		  if (IsWin95EUDCBmp(BMPPath))
      {
			  if (!HandleImportWin95(TTFPath, BMPPath,nIndex ))
        {
//#for fix of 408428 -- allow user to proceed to register fontlink 
//even if there's no correct euf
//				  EnableEUDC( TRUE);
//				  return;
        }
      }
#endif
    }

		if( !lpAssociationRegTmp->UpdateFlg){
			lpAssociationRegTmp++;
			continue;
		}
 		if( !RegistTypeFace(
		    lpAssociationRegTmp->szFaceName,
		    lpAssociationRegTmp->szFileName))
    {
			EnableEUDC( TRUE);
			return;
		}

		lpAssociationRegTmp++;
	}
	EnableEUDC( TRUE);
	if( CheckFlg != DBCSCHK){
		if (!InitSystemFontAssoc())
		{
			return;
		}
	}

	GlobalUnlock( hMem);
 	GlobalFree( hMem);
	EndDialog(IDOK);
}

BOOL
CAssocDlg::InitSystemFontAssoc()
{
	TCHAR	DefaultFace[LF_FACESIZE];
	TCHAR	DefaultFile[MAX_PATH];
	TCHAR	TTFPath[MAX_PATH];
	TCHAR	BMPPath[MAX_PATH];
	TCHAR	*FilePtr;


	GetStringRes(DefaultFace, IDS_SYSTEMEUDCFONT_STR);
	GetStringRes(DefaultFile, IDS_DEFAULTFILENAME);

  // if the registry data for SystemEUDC font is already there, just use that.
  if (!InqTypeFace(DefaultFace,TTFPath,MAX_PATH)) {
    GetSystemWindowsDirectory( TTFPath, MAX_PATH);
#ifdef IN_FONTS_DIR // CAssocDlg::OnOK()
		lstrcat( TTFPath, TEXT("\\FONTS\\"));
#else
		lstrcat( TTFPath, TEXT("\\"));
#endif // IN_FONTS_DIR
 		lstrcat( TTFPath, DefaultFile);
  }

  lstrcpy( BMPPath, TTFPath);
  if(( FilePtr = Mytcsrchr( BMPPath, '.')) != NULL)
		*FilePtr = '\0';
	lstrcat( BMPPath, TEXT(".EUF"));
	
	EnableEUDC( FALSE);
	if(OExistTTF( TTFPath))
  {
#if (WINVER >= 0x0500)
		if (IsWin95EUDCBmp(BMPPath))
		{
			if (!HandleImportWin95(TTFPath, BMPPath, -1))
			{
				EnableEUDC( TRUE);
				return FALSE;
			}
		}
#endif
  }

	SelectEUDC.m_FontTypeFlg = TRUE;
	lstrcpy(SelectEUDC.m_Font,(const TCHAR *)DefaultFace);
	lstrcpy(SelectEUDC.m_File,(const TCHAR *)TTFPath);
	lstrcpy(SelectEUDC.m_FileTitle,(const TCHAR *)DefaultFile);

	if( !RegistTypeFace(DefaultFace, TTFPath)){
		EnableEUDC( TRUE);
		return FALSE;
	}

  EnableEUDC( TRUE);
    return TRUE;
}
/****************************************/
/*					*/
/*	MESSAGE	"WM_DBLCLKS"		*/
/*					*/
/****************************************/
void
CAssocDlg::OnDblclkRegistlist()
{
	this->SendMessage(WM_COMMAND, IDC_MODIFY, 0);

}

/****************************************/
/*					*/
/*	COMMAND	"IDCANCEL"		*/
/*					*/
/****************************************/
void
CAssocDlg::OnCancel()
{
	GlobalUnlock( hMem);
	GlobalFree( hMem);
	EndDialog( IDCANCEL);
}

/****************************************/
/*					*/
/*	COMMAND	 "Modify"		*/
/*					*/
/****************************************/
void
CAssocDlg::OnModify()
{
LPASSOCIATIONREG	lpAssociationRegTmp;
OPENFILENAME	ofn;
	CString	sFilter;
	CWnd	*cWnd;
	TCHAR	chReplace;
	CString	szDlgTtl;
	TCHAR 	szFilter[MAX_PATH];
	TCHAR	szFileName[MAX_PATH];
	TCHAR	szTitleName[MAX_PATH];
	TCHAR	szDirName[MAX_PATH];

	int	nIndex = m_RegListBox.GetCurSel();
	if( nIndex == -1){
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_ASSOCIATE_DLGTITLE,
			IDS_NOTSELTYPEFACE_MSG, TRUE);
		m_RegListBox.SetFocus();
		return;
	}
	//lpAssociationRegTmp = lpAssociationReg + nIndex;
	lpAssociationRegTmp = lpAssociationReg + m_RegListBox.GetItemData(nIndex);

//	Set filter of file( from string table)
	GetStringRes(szFilter, IDS_EUDCTTF_FILTER);
	int StringLength = lstrlen( szFilter);

	chReplace = szFilter[StringLength-1];
	for( int i = 0; szFilter[i]; i++){
		if( szFilter[i] == chReplace)
			szFilter[i] = '\0';
	}
	GetSystemWindowsDirectory( szDirName, sizeof(szDirName));
#ifdef IN_FONTS_DIR // CAssocDlg::OnModify()
    lstrcat( szDirName, TEXT("\\FONTS\\"));
#endif // IN_FONTS_DIR
	lstrcpy( szFileName, TEXT("*.TTE"));
	szDlgTtl.LoadString( IDS_MODIFY_DLGTITLE);

//	Set data in structure of OPENFILENAME
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
		| OFN_PATHMUSTEXIST ; 
  // there's no direct overwrite issue here, the file will be re-cycled.
  //| OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = TEXT("tte");
	ofn.lpstrTitle = szDlgTtl;
	if( !GetSaveFileName( &ofn)){
		return;
	}
  TCHAR tmpName[MAX_PATH];
  lstrcpy(tmpName, szDirName);
  lstrcat(tmpName, TEXT("eudc.tte"));
  if( !lstrcmpi(ofn.lpstrFile, tmpName) ) {
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_MODIFY_DLGTITLE,
			IDS_NOUSEEUDCFILE_MSG, TRUE);
		return;
  }
	if( !CheckFileName( ofn.lpstrFile)){
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_MODIFY_DLGTITLE,
			IDS_ILLEGALFILE_MSG, TRUE);
		return;
	}
	if( OExistTTF( ofn.lpstrFile)){
		if( !IsEUDCTTF( ofn.lpstrFile)){
			OutputMessageBox( this->GetSafeHwnd(),
			IDS_MODIFY_DLGTITLE,
			IDS_NOTEUDCFILE_MSG, TRUE);
			return;
		}
	}
	lstrcpy(lpAssociationRegTmp->szFileName, ofn.lpstrFile);
	lstrcpy(lpAssociationRegTmp->szFileTitle,
		ofn.lpstrFileTitle);
	lpAssociationRegTmp->UpdateFlg = TRUE;
	m_RegListBox.InsertString( nIndex,
		(LPCTSTR)lpAssociationRegTmp->szFaceName);
  m_RegListBox.SetItemData(nIndex, m_RegListBox.GetItemData(nIndex+1));
	m_RegListBox.DeleteString( nIndex + 1);
	m_RegListBox.SetCurSel( nIndex);

	cWnd = GetDlgItem( IDC_REGISTLIST);
	GotoDlgCtrl( cWnd);
}

/****************************************/
/*					*/
/*	COMMAND 	"Remove"	*/
/*					*/
/****************************************/
void
CAssocDlg::OnRemove()
{
LPASSOCIATIONREG	lpAssociationRegTmp;
	TCHAR	Tmp[MAX_PATH];

	int	nIndex = m_RegListBox.GetCurSel();
	if( nIndex == -1){
		OutputMessageBox( this->GetSafeHwnd(),
			IDS_ASSOCIATE_DLGTITLE,
			IDS_NOTSELTYPEFACE_MSG, TRUE);
		m_RegListBox.SetFocus();
		return;
	}

	GetStringRes( Tmp, IDS_NOTSELECT_STR);
	//lpAssociationRegTmp = lpAssociationReg + nIndex;
	lpAssociationRegTmp = lpAssociationReg + m_RegListBox.GetItemData(nIndex);
	lstrcpy(lpAssociationRegTmp->szFileName,  (const TCHAR *)Tmp);
	lstrcpy(lpAssociationRegTmp->szFileTitle, (const TCHAR *)Tmp);
	lpAssociationRegTmp->UpdateFlg = TRUE;
	m_RegListBox.InsertString( nIndex,
			lpAssociationRegTmp->szFaceName);
  m_RegListBox.SetItemData(nIndex, m_RegListBox.GetItemData(nIndex+1));
	m_RegListBox.DeleteString( nIndex + 1);
	m_RegListBox.SetCurSel( nIndex);
	
//	DeleteReg(lpAssociationRegTmp->szFaceName);
	CWnd *cWnd = GetDlgItem( IDC_REGISTLIST);
	GotoDlgCtrl( cWnd);	
}

/****************************************/
/*					*/
/*	Check .TTF 			*/
/*					*/
/****************************************/
static int
CheckTTF(
LOGFONT 	LogFont)
{
struct	NamingTable	*Ntbl;
struct	NameRecord	*NameRec;
	HGLOBAL	hglb;
	DWORD	dwSize = 0L;
	DWORD	dwTable = 0L;
	LPSTR	lpBuffer, lpTable;
	HFONT	hFont;
	HGDIOBJ	OldFont;
	short	nRec = 0;
  int sysLCID;

//	Check "tategaki" or not
	if( LogFont.lfFaceName[0] == '@')
		return 0;

//	Get current font to Inquire ttf file
	hFont = ::CreateFontIndirect( &LogFont);
	OldFont = ::SelectObject( hDC, hFont);
  

//	Get name table in ttf file
	lpTable = "name";
	dwTable = *(LPDWORD)lpTable;
	dwSize  = ::GetFontData( hDC, dwTable, 0L, NULL, 0L);
	if( dwSize == GDI_ERROR){
		::SelectObject(hDC, OldFont);
		::DeleteObject(hFont);
		return 0;
	}
	hglb = GlobalAlloc( GHND, dwSize);
	if( hglb == NULL){
		::SelectObject(hDC, OldFont);
		::DeleteObject(hFont);
		return -1;
	}
	lpBuffer = (LPSTR)GlobalLock( hglb);
	::GetFontData( hDC, dwTable, 0L, (LPVOID)lpBuffer, dwSize);
	::SelectObject(hDC, OldFont);
	::DeleteObject(hFont);

	Ntbl = (struct NamingTable *)lpBuffer;
	sitom( &Ntbl->NRecs);
	nRec = Ntbl->NRecs;
	lpBuffer += sizeof(struct NamingTable);
  sysLCID = (int) LANGIDFROMLCID(GetSystemDefaultLCID());
	while( nRec-- > 0){
		NameRec = (struct NameRecord *)lpBuffer;			
		sitom( &NameRec->PlatformID);
		sitom( &NameRec->PlatformSpecEncID);
		sitom( &NameRec->LanguageID);

#ifdef BUILD_ON_WINNT
//		Unicode TTF
		if( CountryInfo.bUnicodeMode ){
// 			if( NameRec->PlatformID == 3 &&
//		    	    NameRec->LanguageID == sysLCID){
        GlobalUnlock( hglb);
				GlobalFree( hglb);
				return 1;
//			}
		}
#endif //BUILD_ON_WINNT

//		Japanese TTF
		if( CountryInfo.LangID == EUDC_JPN){
			if( NameRec->PlatformID == 3 &&
		    	    NameRec->LanguageID == EUDC_JPN){
				GlobalUnlock( hglb);
				GlobalFree( hglb);
				return 1;
			}
		}
//		Chinese Taipei TTF
		if( CountryInfo.LangID == EUDC_CHT){
			if( NameRec->PlatformID == 3 &&
        NameRec->LanguageID == EUDC_CHT){
    		GlobalUnlock( hglb);
				GlobalFree( hglb);
				return 1;
			}
		}
//		Chinese GB TTF
		if( CountryInfo.LangID == EUDC_CHS){
			if( NameRec->PlatformID == 3 &&
        NameRec->LanguageID == EUDC_CHS){
        GlobalUnlock( hglb);
				GlobalFree( hglb);
				return 1;
			}
		}
//		Korea TTF(WANSUNG)
		if( CountryInfo.LangID == EUDC_KRW){
			if( NameRec->PlatformID == 3 &&
        NameRec->LanguageID == EUDC_KRW){
		    GlobalUnlock( hglb);
				GlobalFree( hglb);
				return 1;		
			}
		}	    	
/*
//    Hebrew TTF
    if( CountryInfo.LangID == EUDC_HEBREW) {
			if( NameRec->PlatformID == 1 &&
          NameRec->LanguageID == 0 ) {
		    		GlobalUnlock( hglb);
				GlobalFree( hglb);
				return 1;		
			}
		}	    	
*/

		lpBuffer += sizeof(struct NameRecord);
	}
	GlobalUnlock( hglb);
	GlobalFree( hglb);
	return 0;
}

/****************************************/
/*					*/
/*	Radio DBCS Clicked		*/
/*					*/
/****************************************/
void
CAssocDlg::OnRadioDbcs()
{
	CWnd 	*cWnd;

	if( CheckFlg != DBCSCHK){
		CheckFlg = DBCSCHK;
		m_RegListBox.EnableWindow( TRUE);
		m_RegListBox.EnableScrollBar( SB_VERT, ESB_ENABLE_BOTH);
		m_RegListBox.UpdateWindow();

		cWnd = this->GetDlgItem( IDC_MODIFY);
		cWnd->EnableWindow( TRUE);

		cWnd = this->GetDlgItem( IDC_REMOVE);
		cWnd->EnableWindow( TRUE);
	}
}

/****************************************/
/*					*/
/*	Radio SYSTEM Clicked		*/
/*					*/
/****************************************/
void
CAssocDlg::OnRadioSystem()
{
	CWnd	*cWnd;

	if( CheckFlg != EUDCCHK){
		CheckFlg = EUDCCHK;
		m_RegListBox.EnableWindow( FALSE);
		m_RegListBox.EnableScrollBar( SB_VERT, ESB_DISABLE_BOTH);

		cWnd = this->GetDlgItem( IDC_MODIFY);
		cWnd->EnableWindow( FALSE);

		cWnd = this->GetDlgItem( IDC_REMOVE);
		cWnd->EnableWindow( FALSE);
	}
}

/****************************************/
/*					*/
/*	Inquire into file( EUDC or not)	*/
/*					*/
/****************************************/
static BOOL
IsEUDCTTF(
TCHAR 	*ttffile)
{
struct	NamingTable	*nTbl;
struct	NameRecord	*nRec;
unsigned int	BufSiz;	
	char	*TableBuf, *SearchOfs;
	char	TTFName[MAX_CODE];


	HANDLE fHdl = CreateFile(ttffile,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( fHdl == INVALID_HANDLE_VALUE)
		return FALSE;

	if( TTFReadVarTable( fHdl, &TableBuf, &BufSiz, "name")){
		CloseHandle( fHdl);
		return FALSE;
	}

	SearchOfs = TableBuf;
	nTbl = (struct NamingTable *)TableBuf;

	sitom( &nTbl->OfsToStr);
	SearchOfs += sizeof(struct NamingTable);
	SearchOfs += sizeof(struct NameRecord);
	nRec = (struct NameRecord *)SearchOfs;

	sitom( &nRec->StringOfs);
	SearchOfs = TableBuf + nTbl->OfsToStr + nRec->StringOfs;
	SearchOfs += sizeof(char);
	for( short Inc = 0; Inc < MAX_CODE - 1; Inc++){
		TTFName[Inc] = (char)*SearchOfs;
		SearchOfs += sizeof(char)*2;
	}
	TTFName[Inc] = '\0';
	if( lstrcmpA( TTFName, "EUDC") == 0){
		free( TableBuf);
		CloseHandle( fHdl);
		return TRUE;
	}
	free( TableBuf);
	CloseHandle( fHdl);
	return FALSE;
}

/****************************************/
/*					*/
/*	Inquire into filename		*/
/*					*/
/****************************************/
static BOOL
CheckFileName(
LPTSTR	FileStr)
{
	TCHAR	FileTmp[MAX_PATH];
	TCHAR	Tmp[MAX_PATH];
	TCHAR	*FilePtr;

	FilePtr = FileStr;
	while( *FilePtr == ' ')
		FilePtr++;

	if( *FilePtr == '.' || *FilePtr == '\0')
		return FALSE;

	int i = 0;
	while( *FilePtr != '\0'){
		FileTmp[i] = *FilePtr;
		FilePtr++;
		i++;
	}
	FileTmp[i] = '\0';
	if(( FilePtr = Mytcsrchr( FileTmp, '.')) == NULL)
		lstrcat( FileTmp, TEXT(".tte"));
	else{
#ifdef BUILD_ON_WINNT
		if( lstrcmpi( FilePtr, TEXT(".TTE")))
#else
		if( stricmp( FilePtr, ".TTE"))
#endif // BUILD_ON_WINNT
			return FALSE;
	}

	lstrcpy(Tmp, FileTmp);
	if( lstrlen(Tmp) >= MAX_PATH)
		return FALSE;
	lstrcpy( FileStr, FileTmp);

	return TRUE;
}
static DWORD aIds[] =
{
	IDC_MODIFY, IDH_EUDC_MODIFY,
	IDC_REMOVE, IDH_EUDC_REMOVE,
	IDC_REGISTLIST,	IDH_EUDC_ASSO_LIST,
	IDC_RADIO_SYSTEM, IDH_EUDC_ASSO_STANDARD,
	IDC_RADIO_DBCS, IDH_EUDC_ASSO_TYPEFACE,
	0,0
};

/****************************************/
/*					*/
/*		Window Procedure	*/
/*					*/
/****************************************/
LRESULT
CAssocDlg::WindowProc(
UINT 	message,
WPARAM 	wParam,
LPARAM 	lParam)
{
	if( message == WM_HELP){
		::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
			HelpPath, HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);
		return(0);
	}	
	if( message == WM_CONTEXTMENU){
		::WinHelp((HWND)wParam, HelpPath,
			HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)aIds);
		return(0);
	}
	return CDialog::WindowProc(message, wParam, lParam);
}

BEGIN_MESSAGE_MAP(CAssocDlg, CDialog)
	//{{AFX_MSG_MAP(CAssocDlg)
	ON_LBN_DBLCLK(IDC_REGISTLIST, OnDblclkRegistlist)
	ON_BN_CLICKED(IDC_MODIFY, OnModify)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_RADIO_DBCS, OnRadioDbcs)
	ON_BN_CLICKED(IDC_RADIO_SYSTEM, OnRadioSystem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	MeasureItem			*/
/*					*/
/****************************************/
void
CRegistListBox::MeasureItem(
LPMEASUREITEMSTRUCT lpMIS)
{
	CRect	ListBoxRect;

	this->GetClientRect( &ListBoxRect);
	ItemHeight = ListBoxRect.Height() /NUMITEM;
	lpMIS->itemHeight = ItemHeight;
}

/****************************************/
/*					*/
/*	Draw Item			*/
/*					*/
/****************************************/
void
CRegistListBox::DrawItem(
LPDRAWITEMSTRUCT lpDIS)
{
LPASSOCIATIONREG	lpAssociationRegTmp;
	LOGFONT	LogFont;
	CSize	CharSize, FontSize, cSize;
	TCHAR	FileTmp[MAX_PATH], NotSel[MAX_PATH];
	int	Offset;

	CDC* pDC = CDC::FromHandle( lpDIS->hDC);
	if( lpDIS->itemAction & ODA_DRAWENTIRE){
	  	CBrush	fBrush;

		fBrush.CreateSolidBrush(GetSysColor(COLOR_WINDOW)); // COLOR_WIN);
		pDC->FillRect( &lpDIS->rcItem, &fBrush);
		fBrush.DeleteObject();
		pDC->SetBkColor( GetSysColor(COLOR_WINDOW)); //COLOR_WIN);

		//lpAssociationRegTmp = lpAssociationReg + lpDIS->itemID;
		lpAssociationRegTmp = lpAssociationReg + lpDIS->itemData;
    if( CheckFlg == DBCSCHK)
			pDC->SetTextColor( GetSysColor(COLOR_WINDOWTEXT)); //COLOR_BLACK);
		else 	pDC->SetTextColor( GetSysColor(COLOR_GRAYTEXT));  //COLOR_SHADOW);

		GetFont()->GetObject( sizeof(LOGFONT), &LogFont);
		if( abs( LogFont.lfHeight) < ItemHeight)
			Offset = ( ItemHeight - abs( LogFont.lfHeight)) /2;
		else	Offset = 0;

		if( lpAssociationRegTmp->FontTypeFlg){
			pDC->DrawIcon( lpDIS->rcItem.left + LSPACE,
				lpDIS->rcItem.top + Offset, tIcon);
		}

		lstrcpy(FileTmp,
		(const TCHAR *)lpAssociationRegTmp->szFileTitle);
		GetStringRes( NotSel, IDS_NOTSELECT_STR);
		if( FileTmp[0] == '\0' ||
		   !lstrcmp((const TCHAR *)FileTmp,(const TCHAR *)NotSel)){

		}else{
			TCHAR	*FilePtr;
			if(( FilePtr = Mytcsrchr( FileTmp, '.')) != NULL)
				*FilePtr = '\0';
			if( lstrlen((const TCHAR *)FileTmp) > 20){
				FileTmp[20] = '\0';
				lstrcat((TCHAR *)FileTmp, TEXT(".."));
			}
		}

		GetTextExtentPoint32( pDC->GetSafeHdc(),
			(const TCHAR *)FileTmp,
			lstrlen((const TCHAR *)FileTmp), &CharSize);

		pDC->ExtTextOut( lpDIS->rcItem.right - CharSize.cx - RSPACE,
			lpDIS->rcItem.top + Offset, 0, 0,
			(const TCHAR *)FileTmp,
			lstrlen((const TCHAR *)FileTmp), NULL);

		int	FontWidth;

		lstrcpy(FontName,
		(const TCHAR *)lpAssociationRegTmp->szFaceName);
		FontWidth = lpDIS->rcItem.right - lpDIS->rcItem.left
			- LCSPACE - RSPACE - CharSize.cx;

		GetTextExtentPoint32( pDC->GetSafeHdc(),
			(const TCHAR *)FontName,
			lstrlen((const TCHAR *)FontName), &FontSize);
			
		if( FontWidth <= FontSize.cx){
			int 	i;
			
 			GetTextExtentPoint32( pDC->GetSafeHdc(),TEXT("<<"),2, &cSize);
			i = ( FontWidth /cSize.cx) * 2;
			FontName[i-2] = '.';
			FontName[i-1] = '.';
			FontName[i] = '\0';
		}

		pDC->ExtTextOut(lpDIS->rcItem.left + LCSPACE,
			lpDIS->rcItem.top + Offset, 0, 0,
			FontName,
			lstrlen(FontName),	NULL);
	}

	if(( lpDIS->itemState & ODS_SELECTED) &&
	   ( lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE))){
	   	CBrush	fBrush;

		if( CheckFlg == DBCSCHK)
			fBrush.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		else	fBrush.CreateSolidBrush(GetSysColor(COLOR_WINDOW)); // COLOR_WIN);

		pDC->FillRect( &lpDIS->rcItem, &fBrush);
		fBrush.DeleteObject();

		//lpAssociationRegTmp = lpAssociationReg + lpDIS->itemID;
		lpAssociationRegTmp = lpAssociationReg + lpDIS->itemData;

		if( CheckFlg == DBCSCHK){
			pDC->SetBkColor(::GetSysColor( COLOR_HIGHLIGHT));
			pDC->SetTextColor(::GetSysColor( COLOR_HIGHLIGHTTEXT));
		}else{
			pDC->SetBkColor(GetSysColor(COLOR_WINDOW)); // COLOR_WIN);
			pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT)); // COLOR_SHADOW);
		}

		GetFont()->GetObject( sizeof(LOGFONT), &LogFont);
		if( abs( LogFont.lfHeight) < ItemHeight)
			Offset = ( ItemHeight - abs( LogFont.lfHeight)) /2;
		else	Offset = 0;

		if( lpAssociationRegTmp->FontTypeFlg){
			pDC->DrawIcon( lpDIS->rcItem.left + LSPACE,
				lpDIS->rcItem.top + Offset, tIcon);
		}

		lstrcpy(FileTmp,
			lpAssociationRegTmp->szFileTitle);
		GetStringRes( NotSel, IDS_NOTSELECT_STR);
		if( FileTmp[0] == '\0' ||
		   !lstrcmp(FileTmp,NotSel)){

		}else{
			TCHAR 	*FilePtr;
			if(( FilePtr = Mytcsrchr( FileTmp, '.')) != NULL)
				*FilePtr = '\0';
			if( lstrlen(FileTmp) > 20){
				FileTmp[20] = '\0';
				lstrcat(FileTmp, TEXT(".."));
			}
		}

		GetTextExtentPoint32( pDC->GetSafeHdc(),
			FileTmp,
			lstrlen(FileTmp), &CharSize);

		pDC->ExtTextOut( lpDIS->rcItem.right - CharSize.cx - RSPACE,
			lpDIS->rcItem.top + Offset, 0, 0,
			FileTmp,
			lstrlen(FileTmp), NULL);

		int	FontWidth;

		lstrcpy(FontName,
			lpAssociationRegTmp->szFaceName);
		FontWidth = lpDIS->rcItem.right - lpDIS->rcItem.left
			- LCSPACE - RSPACE - CharSize.cx;

		GetTextExtentPoint32( pDC->GetSafeHdc(),
			FontName,
			lstrlen(FontName), &FontSize);
			
		if( FontWidth <= FontSize.cx){
			int 	i;
			
 			GetTextExtentPoint32( pDC->GetSafeHdc(),TEXT("<<"),2, &cSize);
			i = ( FontWidth /cSize.cx) * 2;
			FontName[i-2] = '.';
			FontName[i-1] = '.';
			FontName[i] = '\0';
		}
				
		pDC->ExtTextOut( lpDIS->rcItem.left + LCSPACE,
			lpDIS->rcItem.top + Offset, 0, 0,
			FontName,
			lstrlen(FontName),	NULL);
	}

	if( !(lpDIS->itemState & ODS_SELECTED) &&
	  ( lpDIS->itemAction & ODA_SELECT)){
	  	CBrush	fBrush;

		fBrush.CreateSolidBrush(GetSysColor(COLOR_WINDOW)); // COLOR_WIN);
		pDC->FillRect( &lpDIS->rcItem, &fBrush);
		fBrush.DeleteObject();
		
		//lpAssociationRegTmp = lpAssociationReg + lpDIS->itemID;
		lpAssociationRegTmp = lpAssociationReg + lpDIS->itemData;

		pDC->SetBkColor(GetSysColor(COLOR_WINDOW)); // COLOR_WIN);
		if( CheckFlg == DBCSCHK)
			pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); // COLOR_BLACK);
		else	pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));  // COLOR_SHADOW);

		GetFont()->GetObject( sizeof(LOGFONT), &LogFont);
		if( abs( LogFont.lfHeight) < ItemHeight)
			Offset = ( ItemHeight - abs( LogFont.lfHeight)) /2;
		else	Offset = 0;

		if( lpAssociationRegTmp->FontTypeFlg){
			pDC->DrawIcon( lpDIS->rcItem.left + LSPACE,
				lpDIS->rcItem.top + Offset, tIcon);
		}

		lstrcpy( FileTmp,
			lpAssociationRegTmp->szFileTitle);
		GetStringRes( NotSel, IDS_NOTSELECT_STR);
		if( FileTmp[0] == '\0' ||
		   !lstrcmp(FileTmp,NotSel)){

		}else{
			TCHAR 	*FilePtr;
			if(( FilePtr = Mytcsrchr( FileTmp, '.')) != NULL)
				*FilePtr = '\0';
			if( lstrlen(FileTmp) > 20){
				FileTmp[20] = '\0';
				lstrcat(FileTmp, TEXT(".."));
			}
		}

		GetTextExtentPoint32( pDC->GetSafeHdc(),
			FileTmp,
			lstrlen(FileTmp), &CharSize);

		pDC->ExtTextOut( lpDIS->rcItem.right - CharSize.cx - RSPACE,
			lpDIS->rcItem.top + Offset, 0, 0,
			FileTmp,
			lstrlen(FileTmp), NULL);

		int	FontWidth;

		lstrcpy(FontName,
			lpAssociationRegTmp->szFaceName);
		FontWidth = lpDIS->rcItem.right - lpDIS->rcItem.left
			- LCSPACE - RSPACE - CharSize.cx;

		GetTextExtentPoint32( pDC->GetSafeHdc(),
			FontName,
			lstrlen(FontName), &FontSize);
			
		if( FontWidth <= FontSize.cx){
			int 	i;
			
 			GetTextExtentPoint32( pDC->GetSafeHdc(),TEXT("<<"),2, &cSize);
			i = ( FontWidth /cSize.cx) * 2;
			FontName[i-2] = '.';
			FontName[i-1] = '.';
			FontName[i] = '\0';
		}

		pDC->ExtTextOut( lpDIS->rcItem.left + LCSPACE,
			lpDIS->rcItem.top + Offset, 0, 0,
			FontName,	
			lstrlen(FontName),	NULL);
	}
}

int CRegistListBox::CompareItem(LPCOMPAREITEMSTRUCT lpCIS)
{
  TCHAR	TmpFontName1[LF_FACESIZE], TmpFontName2[LF_FACESIZE];
  LPASSOCIATIONREG	lpAssociationRegTmp1,lpAssociationRegTmp2;
    lpAssociationRegTmp1 = lpAssociationReg + lpCIS->itemData1;
    lpAssociationRegTmp2 = (LPASSOCIATIONREG) (lpCIS->itemData2);
    lstrcpy(TmpFontName1, lpAssociationRegTmp1->szFaceName);
    lstrcpy(TmpFontName2, lpAssociationRegTmp2->szFaceName);
    return (lstrcmp(TmpFontName1, TmpFontName2));
}

BOOL CAssocDlg::HandleImportWin95(
LPTSTR TTFPath,
LPTSTR BMPPath,
int Idx)
{

	TCHAR UserFont[MAX_PATH];
	LPTSTR FilePtr;
	TCHAR szTmpDir[MAX_PATH];

	GetTempPath(MAX_PATH, szTmpDir);

	if (!GetTempFileName(szTmpDir, TEXT("EUF"), 0, UserFont))
	{
		lstrcpy(UserFont, BMPPath);
		if(( FilePtr = Mytcsrchr( UserFont, '\\')) != NULL)
			*FilePtr = '\0';
		lstrcpy(UserFont, TEXT("EUF.tmp"));
	}

	if (!MoveFileEx(BMPPath, UserFont, MOVEFILE_REPLACE_EXISTING))
	{
		return FALSE;

	}

  CGageDlg dlg(this, UserFont, BMPPath, TTFPath, TRUE);
	dlg.DoModal();

	return TRUE;
}
