/********************************************************/
/*							                            */
/*							                            */
/*	EUDC EDITOR     ( Windows 95)			            */
/*							                            */
/*		* Japanese Version			                    */
/*		* Korea	   Version			                    */
/*		* Chinese  Version			                    */
/*							                            */
/*                                                      */
/* Copyright (c) 1997-1999 Microsoft Corporation.       */
/********************************************************/

#include 	"stdafx.h"
#include 	<afxpriv.h>
#include 	"eudcedit.h"
#include 	"mainfrm.h"
#include	"registry.h"
#include	"util.h"
#include  "assocdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char 	BASED_CODE THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CEudcApp, CWinApp)
//{{AFX_MSG_MAP(CEudcApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
//}}AFX_MSG_MAP
	ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
END_MESSAGE_MAP()

/* Global parameter */
INT	CAPTION_HEIGHT;		// height of caption
INT	BITMAP_WIDTH;		// width of bitmap
INT	BITMAP_HEIGHT;		// height of bitmap
TCHAR	HelpPath[MAX_PATH];	// help file path
TCHAR	ChmHelpPath[MAX_PATH];	// help file path for HtmlHelp
TCHAR	FontPath[MAX_PATH];	// font file path
DWORD	COLOR_GRID;		// grid color
DWORD	COLOR_FITTING;		// bitmap color on show outline
DWORD	COLOR_CURVE;		// color of outline
DWORD	COLOR_FACE;		// Win95 3D Face SystemColor
DWORD	COLOR_HLIGHT;		// Win95 3D HighLight System Color
DWORD	COLOR_SHADOW;		// Win95 3D Shadow SystemColor
DWORD	COLOR_WIN;		// Win95 Window System Color
CString	NotMemTtl;
CString	NotMemMsg;
HCURSOR	ToolCursor[NUMTOOL];	// cursor for tool
HCURSOR	ArrowCursor[NUMRESIZE];	// cursor for resize
COUNTRYINFO	CountryInfo;	// country information structure

/* Global function */
extern BOOL 	SetCountryInfo( UINT LocalCP);
BOOL     g_bKeepEUDCLink = TRUE;

extern "C" BOOL AnyLinkedFonts();


CEudcApp NEAR theApp;
/************************************************/
/*						*/
/*	Default Constructor			*/
/*						*/
/************************************************/
CEudcApp::CEudcApp()
{
}

/************************************************/
/*						*/
/*	Initialize Instance			*/
/*						*/
/************************************************/
BOOL
CEudcApp::InitInstance()
{
	CString	MainWndTitle;
	CRect	MainWndRect;
	UINT	MaxWndFlag;

//	Check whether EUDC editor can open or not
	if( !CheckPrevInstance())
		return FALSE;

/*------------------------------------------------
 * check if it's Administrator
 *------------------------------------------------*/
  TCHAR winpath[MAX_PATH];
  HANDLE nfh;

  GetSystemWindowsDirectory( winpath, MAX_PATH);
#ifdef IN_FONTS_DIR // CAssocDlg::OnOK()
		lstrcat( winpath, TEXT("\\FONTS\\"));
#else
		lstrcat( winpath, TEXT("\\"));
#endif // IN_FONTS_DIR
  lstrcat(winpath, _T("eudcadm.tte"));
	nfh = CreateFile(winpath,
					GENERIC_WRITE,
					FILE_SHARE_DELETE,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if ( nfh  == INVALID_HANDLE_VALUE)
  {
    HINSTANCE hInst = AfxGetInstanceHandle();
    TCHAR szMessage[256];
    LoadString(hInst, IDS_ACCESSDENIED, szMessage, sizeof(szMessage) / sizeof(TCHAR));
    AfxMessageBox(szMessage, MB_OK, 0);
		return FALSE;
  }
	else
  {
    CloseHandle(nfh);
    DeleteFile(winpath);
  }

//	Set background color for dialog
	COLOR_FACE   = ::GetSysColor( COLOR_3DFACE);
	COLOR_HLIGHT = ::GetSysColor( COLOR_3DHILIGHT);
	COLOR_SHADOW = ::GetSysColor( COLOR_3DSHADOW);
	COLOR_WIN    = ::GetSysColor( COLOR_WINDOW);
//	SetDialogBkColor( COLOR_FACE);

//	Set 3d controls
	Enable3dControls();

//	Create registry subkey
	if( !CreateRegistrySubkey())
		return FALSE;

//	Open "EUDCEDIT.INI", read data
	if( !GetProfileText( &MainWndRect, &MaxWndFlag))
		return FALSE;

//	Get Language ID with GetSystemDefaultLCID()
//      Get area of EUDC from registry and WideCharToMultiByte().
	if( !GetCountryInfo())
		return FALSE;

#if WINVER >= 0x0500
//	Remember original font link status before we do anything
//pliu  g_bKeepEUDCLink = AnyLinkedFonts();
#endif

//      Get Cursor from resource
	if( !GetCursorRes())
		return FALSE;

//      Get font and help file path
	if( !GetFilePath())
		return FALSE;

//	Create MDI mainFrame window
	MainWndTitle.LoadString( IDS_MAINFRAMETITLE);
	CMainFrame* pMainFrame = new CMainFrame;


	if (!pMainFrame->Create( MainWndTitle,
	    WS_OVERLAPPEDWINDOW , MainWndRect,
	    MAKEINTRESOURCE( IDR_MAINFRAME))){
		return FALSE;
	}

	pMainFrame->ShowWindow( m_nCmdShow);
	if( MaxWndFlag){
		pMainFrame->ShowWindow( SW_SHOWMAXIMIZED);
	}
	pMainFrame->UpdateWindow();
	m_pMainWnd = pMainFrame;

	CAssocDlg dlg(m_pMainWnd);

	if (!dlg.InitSystemFontAssoc())
	{
		return FALSE;
	}
	pMainFrame->m_wndGuideBar.PositionStatusPane();
	pMainFrame->SendMessage(WM_COMMAND, ID_READ_CHAR, NULL);

	return TRUE;
}

BOOL
CEudcApp::ExitInstance()
{
    EnableEUDC(FALSE);
    if (!g_bKeepEUDCLink && CountryInfo.bOnlyUnicode)
    {
        TCHAR szDefaultFace[LF_FACESIZE];
        TCHAR szFontPath[MAX_PATH];
        TCHAR *Ptr;

        GetStringRes(szDefaultFace, IDS_SYSTEMEUDCFONT_STR);

        if (InqTypeFace(szDefaultFace, szFontPath,MAX_PATH))
        {
            //
            // delete file eudc.tte
            //
            DeleteFile(szFontPath);
            if(( Ptr = Mytcsrchr( szFontPath, '.')) != NULL)
            {
                *Ptr = '\0';
                lstrcat( szFontPath, TEXT(".EUF"));
                //
                // delete file eudc.euf
                //
                DeleteFile(szFontPath);
            }
        }
        DeleteRegistrySubkey();
    }
    EnableEUDC(TRUE);
    return CWinApp::ExitInstance();
}

/************************************************/
/*						*/
/*	Check whether editor can open or not	*/
/*						*/
/************************************************/
BOOL
CEudcApp::CheckPrevInstance()
{
	HWND	hWnd;
	TCHAR	TitleBuf[50];

	GetStringRes(TitleBuf, IDS_MAINFRAMETITLE);

//	Search previous eudcedit mainframe.	
	hWnd = ::FindWindow( NULL, TitleBuf);
	if( hWnd == NULL)
		return TRUE;
	else 	::SetForegroundWindow( hWnd);

	return FALSE;
}

/************************************************/
/*						*/
/*	Correspond to waitting for Input	*/
/*						*/
/************************************************/
BOOL
CEudcApp::OnIdle(
LONG 	lCount)
{
	CWnd	*pWnd;

	if( !lCount){
		for( pWnd = m_pMainWnd->GetWindow( GW_HWNDFIRST); pWnd != NULL;
		     pWnd = pWnd->GetNextWindow( GW_HWNDNEXT)){
			if( m_pMainWnd == pWnd->GetParent()){
				if( pWnd == m_pMainWnd->GetActiveWindow() &&
				  ( ::GetCapture() == NULL))
					m_pMainWnd->SetActiveWindow();

				pWnd->SendMessage( WM_IDLEUPDATECMDUI,
						 (WPARAM)TRUE, 0L);
			}
		}
	}
	return CWinApp::OnIdle( lCount);
}
					
/************************************************/
/*						*/
/*   Open "EUDCEDIT.INI"			*/
/*   Set parameter of EUDC Editor		*/
/*						*/
/************************************************/
BOOL
CEudcApp::GetProfileText(
LPRECT 	MainWndRect,
UINT 	*MaxWndFlag)
{
	TCHAR	ProfileBuf[MAX_PATH], *pString;
	TCHAR	Separation[] = TEXT(" ,");
	INT	xScreen , yScreen;
	UINT	BitmapSiz;
	BYTE	Rcolor, Gcolor, Bcolor;
	CString	GridColor, CurvColor, FittColor, MainWnd;

//	Get system metrics
	CAPTION_HEIGHT = ::GetSystemMetrics( SM_CYCAPTION);
	xScreen = ::GetSystemMetrics( SM_CXSCREEN);
	yScreen = ::GetSystemMetrics( SM_CYSCREEN);

//	Read bitmapsize and maxflag
	BitmapSiz = this->GetProfileInt(TEXT("Bitmap"), TEXT("BitmapSize"), DEF_BITMAPSIZE);
	if( BitmapSiz <= 0)
		BitmapSiz = DEF_BITMAPSIZE;
	if( BitmapSiz > MAX_BITMAPSIZE)
		BitmapSiz = DEF_BITMAPSIZE;

	BitmapSiz = ((BitmapSiz + sizeof(WORD)-1)/sizeof(WORD))*sizeof(WORD);
	if( BitmapSiz > MAX_BITMAPSIZE)
		BitmapSiz = MAX_BITMAPSIZE;
	if( BitmapSiz < MIN_BITMAPSIZE)
		BitmapSiz = MIN_BITMAPSIZE;

	BITMAP_WIDTH  = BitmapSiz;
	BITMAP_HEIGHT = BitmapSiz;
	*MaxWndFlag = this->GetProfileInt(TEXT("WindowSize"), TEXT("MinMaxFlag"), 0);

//	Read color
	GridColor = this->GetProfileString(TEXT("Color"), TEXT("Grid"), TEXT("128 128 128"));
	CurvColor = this->GetProfileString(TEXT("Color"), TEXT("Curve"), TEXT("255 0 0"));
	FittColor = this->GetProfileString(TEXT("Color"), TEXT("Fitting"), TEXT("128 128 128"));

//	Read grid color
	ConvStringRes((TCHAR *)ProfileBuf, GridColor);
	if(( pString = Mytcstok( ProfileBuf, Separation)) == NULL)
		Rcolor = 0;
	else	Rcolor = (BYTE)Myttoi( pString);
	if(( pString = Mytcstok( NULL, Separation)) == NULL)
		Gcolor = 0;
	else	Gcolor = (BYTE)Myttoi( pString);
	if(( pString = Mytcstok( NULL, Separation)) == NULL)
		Bcolor = 0;
	else	Bcolor = (BYTE)Myttoi( pString);
	COLOR_GRID = RGB( Rcolor, Gcolor, Bcolor);

//	Read outline color
	ConvStringRes(ProfileBuf, CurvColor);
	if(( pString = Mytcstok( ProfileBuf, Separation)) == NULL)
		Rcolor = 0;
	else	Rcolor = (BYTE)Myttoi( pString);
	if(( pString = Mytcstok( NULL, Separation)) == NULL)
		Gcolor = 0;
	else	Gcolor = (BYTE)Myttoi( pString);
	if(( pString = Mytcstok( NULL, Separation)) == NULL)
		Bcolor = 0;
	else	Bcolor = (BYTE)Myttoi( pString);
	COLOR_CURVE = RGB( Rcolor, Gcolor, Bcolor);

//	Read bitmap color in show outline
	ConvStringRes(ProfileBuf, FittColor);
	if(( pString = Mytcstok( ProfileBuf, Separation)) == NULL)
		Rcolor = 0;
	else	Rcolor = (BYTE)Myttoi( pString);
	if(( pString = Mytcstok( NULL, Separation)) == NULL)
		Gcolor = 0;
	else	Gcolor = (BYTE)Myttoi( pString);
	if(( pString = Mytcstok( NULL, Separation)) == NULL)
		Bcolor = 0;
	else	Bcolor = (BYTE)Myttoi( pString);
	COLOR_FITTING = RGB( Rcolor, Gcolor, Bcolor);

//	Read main window size
	MainWnd = this->GetProfileString(TEXT("WindowSize"),TEXT("MainWindowSize"), TEXT(""));
	if( *MainWnd == '\0'){
		MainWndRect->left = 0;
		MainWndRect->top  = 0;
		MainWndRect->right = (xScreen/5)*4;
		MainWndRect->bottom =(yScreen/5)*4;
	}else{
		ConvStringRes(ProfileBuf, MainWnd);
		pString = Mytcstok( ProfileBuf, Separation);
		MainWndRect->left = Myttoi( pString);
		pString = Mytcstok( NULL, Separation);
		MainWndRect->top = Myttoi( pString);
		pString = Mytcstok( NULL, Separation);
		MainWndRect->right = Myttoi( pString);
		pString = Mytcstok( NULL, Separation);
		MainWndRect->bottom = Myttoi( pString);
	}
	return TRUE;
}

/************************************************/
/*						*/
/*	Get country information			*/
/*						*/
/************************************************/
BOOL
CEudcApp::GetCountryInfo()
{
	UINT	LocalCP;

	CountryInfo.CurrentRange = 0;
	CountryInfo.LangID = (int)GetSystemDefaultLCID();

	LocalCP = GetACP();

    CountryInfo.bUnicodeMode = FALSE;
	CountryInfo.bOnlyUnicode = FALSE;

	switch( CountryInfo.LangID){
	case EUDC_JPN:
		CountryInfo.CharacterSet = SHIFTJIS_CHARSET;
		break;
    case EUDC_HKG:
        CountryInfo.LangID = EUDC_CHT;
        //
        // fall through
        //
	case EUDC_CHT:
		CountryInfo.CharacterSet = CHINESEBIG5_CHARSET;
		break;
	case EUDC_KRW:
		CountryInfo.CharacterSet = HANGEUL_CHARSET;
		break;
    case EUDC_SIN:
        CountryInfo.LangID = EUDC_CHS;
        //
        // Fall through
        //
	case EUDC_CHS:
		CountryInfo.CharacterSet = GB2312_CHARSET;
		break;
	default:
    CHARSETINFO csi;
    if (TranslateCharsetInfo((DWORD*)LocalCP, &csi, TCI_SRCCODEPAGE))
       CountryInfo.CharacterSet = csi.ciCharset;
		CountryInfo.bOnlyUnicode = TRUE;
   	CountryInfo.bUnicodeMode = TRUE;
    lstrcpy(CountryInfo.szForceFont, _T("Microsoft Sans Serif"));
		break;
	}
	if( !SetCountryInfo( LocalCP))
		return FALSE;
	else 	return TRUE;
}

/************************************************/
/*						*/
/*	Get Cursor resource file		*/
/*						*/
/************************************************/
BOOL
CEudcApp::GetCursorRes()
{
	int	i;

//	For tool cursor
	ToolCursor[PEN]        = this->LoadCursor(IDC_PENCIL);
	ToolCursor[BRUSH]      = this->LoadCursor(IDC_BRUSH);
	ToolCursor[CIRCLE]     = this->LoadStandardCursor(IDC_CROSS);
	ToolCursor[CIRCLEFILL] = this->LoadStandardCursor(IDC_CROSS);
	ToolCursor[SLOPE]      = this->LoadStandardCursor(IDC_CROSS);
	ToolCursor[RECTBAND]   = this->LoadStandardCursor(IDC_CROSS);
	ToolCursor[RECTFILL]   = this->LoadStandardCursor(IDC_CROSS);
	ToolCursor[FREEFORM]   = this->LoadStandardCursor(IDC_CROSS);
	ToolCursor[RECTCLIP]   = this->LoadStandardCursor(IDC_CROSS);
	ToolCursor[ERASER]     = this->LoadCursor(IDC_ERASER);
	for( i = PEN; i <= ERASER; i++){
		if( ToolCursor[i] == NULL){
			return FALSE;
		}
	}
	
//	For select rectangle cursur
	ArrowCursor[VERTICAL]  = this->LoadStandardCursor(
				 MAKEINTRESOURCE(IDC_SIZEWE));
	ArrowCursor[RIGHTSLOPE]= this->LoadStandardCursor(
				 MAKEINTRESOURCE(IDC_SIZENESW));
	ArrowCursor[LEFTSLOPE] = this->LoadStandardCursor(
				 MAKEINTRESOURCE(IDC_SIZENWSE));
	ArrowCursor[HORIZONTAL]= this->LoadStandardCursor(
				 MAKEINTRESOURCE(IDC_SIZENS));
	ArrowCursor[ALLDIRECT] = this->LoadStandardCursor(
				 MAKEINTRESOURCE(IDC_SIZEALL));
	for( i = VERTICAL; i <= ALLDIRECT; i++){
		if( ArrowCursor[i] == NULL){
			return FALSE;
		}
	}
	return TRUE;
}

/************************************************/
/*						*/
/*	Get help file path			*/
/*						*/
/************************************************/
BOOL
CEudcApp::GetFilePath()
{
	if( !GetSystemWindowsDirectory( FontPath, MAX_PATH))
		return FALSE;
	lstrcat(FontPath, TEXT("\\"));

	lstrcpy(HelpPath, TEXT("EUDCEDIT.HLP"));
    lstrcpy(ChmHelpPath, TEXT("EUDCEDIT.CHM"));

	NotMemTtl.LoadString( IDS_MAINFRAMETITLE);
	NotMemMsg.LoadString( IDS_NOTENOUGHMEMORY_ERROR);

	return TRUE;
}

/************************************************/
/*						*/
/*	COMMAND 	"About"			*/
/*						*/
/************************************************/
void
CEudcApp::OnAppAbout()
{
	HICON	hIcon;
	TCHAR	TitleBuf[50];

	hIcon = LoadIcon( IDR_MAINFRAME);

	GetStringRes((TCHAR *)TitleBuf, IDS_MAINFRAMETITLE);
	ShellAbout( m_pMainWnd->GetSafeHwnd(), TitleBuf, TEXT(""), hIcon);
}
