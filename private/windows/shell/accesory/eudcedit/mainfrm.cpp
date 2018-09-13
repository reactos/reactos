/**************************************************/
/*						                          */
/*						                          */
/*	MDI Mainframe window class		              */
/*						                          */
/*						                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"mainfrm.h"
#include 	"editwnd.h"
#include 	"refrwnd.h"
#include 	"eudcdlg.h"
#include 	"refrdlg.h"
#include	"registry.h"
#include	"assocdlg.h"
#include	"imprtdlg.h"
#include	"gagedlg.h"
#include	"blinkdlg.h"
#include	"util.h"
#include	"rotatdlg.h"
#include	"extfunc.h"
extern "C"
{
#include	"imelist.h"
}

#define		BUFFERMAX	800

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//	Global paramter
BOOL	TitleFlag;
extern CPoint	PalettePt;
CEditWnd	*pEditChild;
CRefrWnd *pRefrChild;
extern BOOL SetCountryInfo(UINT LocalCP);

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)
BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND_EX(IDW_STYLES, OnStylesBar)
	ON_UPDATE_COMMAND_UI(IDW_STYLES, OnUpdateStylesBar)
	ON_UPDATE_COMMAND_UI(ID_TOOLBAR, OnUpdateToolbar)
	ON_WM_DESTROY()
	ON_COMMAND(ID_REFFERFILE_NEW, OnRefferfileNew)
	ON_COMMAND(ID_READ_CHAR, OnReadChar)
	ON_UPDATE_COMMAND_UI(ID_REFFERFILE_NEW, OnUpdateRefferfileNew)
	ON_COMMAND(ID_TOOLBAR, OnToolbar)
	ON_COMMAND(ID_REFFER_CLOSE, OnRefferClose)
	ON_UPDATE_COMMAND_UI(ID_REFFER_CLOSE, OnUpdateRefferClose)
	ON_WM_CLOSE()
	ON_COMMAND(ID_SHOW_GRID, OnShowGrid)
	ON_UPDATE_COMMAND_UI(ID_SHOW_GRID, OnUpdateShowGrid)
	ON_COMMAND(ID_SELECTFAMILY, OnSelectfamily)
	ON_COMMAND(ID_SAVECHAR, OnSavechar)
	ON_UPDATE_COMMAND_UI(ID_SAVECHAR, OnUpdateSavechar)
	ON_COMMAND(ID_SAVE_CHARAS, OnSaveCharas)
	ON_UPDATE_COMMAND_UI(ID_SAVE_CHARAS, OnUpdateSaveCharas)
	ON_COMMAND(ID_LINKIME, OnLinkime)
	ON_WM_INITMENU()
	ON_UPDATE_COMMAND_UI(ID_READ_CHAR, OnUpdateReadChar)
	ON_UPDATE_COMMAND_UI(ID_LINKIME, OnUpdateLinkime)
	ON_COMMAND(ID_NEXTCODE, OnNextcode)
	ON_UPDATE_COMMAND_UI(ID_NEXTCODE, OnUpdateNextcode)
	ON_COMMAND(ID_PREVCODE, OnPrevcode)
	ON_UPDATE_COMMAND_UI(ID_PREVCODE, OnUpdatePrevcode)
	ON_COMMAND(ID_CALL_CHAR, OnCallChar)
	ON_UPDATE_COMMAND_UI(ID_CALL_CHAR, OnUpdateCallChar)
	ON_COMMAND(ID_IMPORT_FILE, OnImportFile)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_FILE, OnUpdateImportFile)
	ON_COMMAND(ID_LINKBATCHMODE, OnLinkbatchmode)
	ON_UPDATE_COMMAND_UI(ID_LINKBATCHMODE, OnUpdateLinkbatchmode)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_FIT_CURVE, OnFitCurve)
	ON_UPDATE_COMMAND_UI(ID_FIT_CURVE, OnUpdateFitCurve)
	ON_COMMAND(ID_ROTATE, OnRotate)
	ON_UPDATE_COMMAND_UI(ID_ROTATE, OnUpdateRotate)
	ON_COMMAND(ID_PALETTEBRUSH, OnPalettebrush)
	ON_UPDATE_COMMAND_UI(ID_PALETTEBRUSH, OnUpdatePalettebrush)
	ON_COMMAND(ID_PALETTECIRCLE, OnPalettecircle)
	ON_UPDATE_COMMAND_UI(ID_PALETTECIRCLE, OnUpdatePalettecircle)
	ON_COMMAND(ID_PALETTECIRCLEFILL, OnPalettecirclefill)
	ON_UPDATE_COMMAND_UI(ID_PALETTECIRCLEFILL, OnUpdatePalettecirclefill)
	ON_COMMAND(ID_PALETTEFREEFORM, OnPalettefreeform)
	ON_UPDATE_COMMAND_UI(ID_PALETTEFREEFORM, OnUpdatePalettefreeform)
	ON_COMMAND(ID_PALETTEERASER, OnPaletteeraser)
	ON_UPDATE_COMMAND_UI(ID_PALETTEERASER, OnUpdatePaletteeraser)
	ON_COMMAND(ID_PALETTEPEN, OnPalettepen)
	ON_UPDATE_COMMAND_UI(ID_PALETTEPEN, OnUpdatePalettepen)
	ON_COMMAND(ID_PALETTERECT, OnPaletterect)
	ON_UPDATE_COMMAND_UI(ID_PALETTERECT, OnUpdatePaletterect)
	ON_COMMAND(ID_PALETTERECTBAND, OnPaletterectband)
	ON_UPDATE_COMMAND_UI(ID_PALETTERECTBAND, OnUpdatePaletterectband)
	ON_COMMAND(ID_PALETTERECTFILL, OnPaletterectfill)
	ON_UPDATE_COMMAND_UI(ID_PALETTERECTFILL, OnUpdatePaletterectfill)
	ON_COMMAND(ID_PALETTESLOPE, OnPaletteslope)
	ON_UPDATE_COMMAND_UI(ID_PALETTESLOPE, OnUpdatePaletteslope)
	ON_CBN_SELCHANGE(IDC_SELECTCHARSET, OnSelectCharSet)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT BASED_CODE Palette[] =
{
//	Correspond to palette bitmap data
	ID_PALETTEPEN,
	ID_PALETTEBRUSH,
	ID_PALETTESLOPE,
	ID_PALETTERECT,
	ID_PALETTERECTFILL,
	ID_PALETTECIRCLE,
	ID_PALETTECIRCLEFILL,
	ID_PALETTERECTBAND,
	ID_PALETTEFREEFORM,
	ID_PALETTEERASER
};

/************************************************/
/*						*/
/*	Constructor				*/
/*						*/	
/************************************************/
CMainFrame::CMainFrame()
{
	CustomWndOpen = FALSE;		// Whether edit open or not
	RefferWndVisible = FALSE;	// Whether referrence visible or not
	ToolBarVisible = TRUE;		// Whether toolbox visible or not
	GuideBarVisible = TRUE;		// Whether guidebar visible or not
}

/************************************************/
/*						*/
/*	Destructor				*/
/*						*/
/************************************************/
CMainFrame::~CMainFrame()
{
}

/************************************************/
/*						*/
/*	Create MDI mainframe window 		*/
/*						*/
/************************************************/
BOOL
CMainFrame::Create(
LPCTSTR WndTitle,
DWORD 	WndStyle,
RECT 	MainWndRect,
LPCTSTR 	nID)
{
//	Register MDI mainframe window class
	const TCHAR *MainWndClass =
	AfxRegisterWndClass( 0, AfxGetApp()->LoadStandardCursor(IDC_ARROW),
		 	(HBRUSH)(COLOR_WINDOW+1),
		 	 AfxGetApp()->LoadIcon(IDR_MAINFRAME));

	if( !CFrameWnd::Create( MainWndClass,
		WndTitle, WndStyle, MainWndRect, NULL, nID))
		return FALSE;

	return TRUE;
}

/************************************************/
/*						*/
/*	Process before MDI mainframe create	*/
/*						*/
/************************************************/
int
CMainFrame::OnCreate(
LPCREATESTRUCT lpCreateStruct)
{
	if( CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	if( !LoadAccelTable( MAKEINTRESOURCE(IDR_MAINFRAME)))
		return -1;
	if( !CreateGuideBar())
		return -1;
	if( !CreateToolBar())
		return -1;
	EnableDocking(CBRS_ALIGN_ANY);

	if( !CreateReferWnd())
		return -1;
	m_wndToolBar.EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_LEFT);
	DockControlBar(&m_wndToolBar);
 	return 0;
}

/************************************************/
/*						*/
/*	Create client area			*/
/*						*/
/************************************************/
BOOL
CMainFrame::OnCreateClient(
LPCREATESTRUCT	lpcs,
CCreateContext 	*pContext)
{
	if( !CMDIFrameWnd::CreateClient( lpcs, NULL))
		return FALSE;

	return TRUE;
}

/************************************************/
/*						*/
/*	Create tool bar ( left side)		*/
/*						*/
/************************************************/
BOOL
CMainFrame::CreateToolBar()
{
	if( !m_wndToolBar.Create( this, WS_CHILD | WS_VISIBLE | CBRS_LEFT |
			CBRS_TOOLTIPS, 0)||
	    !m_wndToolBar.LoadBitmap( IDR_MAINFRAME)   ||
	    !m_wndToolBar.SetButtons( Palette,
	    sizeof( Palette)/sizeof( UINT)))
		return FALSE;
	else
		return TRUE;
}



/************************************************/
/*						*/
/*	Create Guideline bar ( upper side)	*/
/*						*/
/************************************************/
BOOL
CMainFrame::CreateGuideBar()
{
	const UINT nIndicators[] = {IDS_CHARSET_STR,
								IDS_CODE_STR,
								IDS_FONT_STR,
								IDS_FILE_STR};
	
	if( !m_wndGuideBar.Create( this, ID_TOOLBAR) ||
		!m_wndGuideBar.SetIndicators(nIndicators,
			sizeof(nIndicators)/sizeof(UINT)))
		return FALSE;
	else{
		m_wndGuideBar.PositionStatusPane();	
		return TRUE;
	}
}




/************************************************/
/*						*/
/*	COMMAND 	"ASSOCIATIONS"		*/
/*						*/
/************************************************/
void
CMainFrame::OnSelectfamily()
{
	CAssocDlg	dlg;

	if( CustomWndOpen){
		if( !OutputSaveMessage())
			return;
	}
	if( dlg.DoModal() == IDOK){
		if( CustomWndOpen){
			pEditChild->UpdateBitmap();
			pEditChild->SelectCodes();
		}
		m_wndGuideBar.PositionStatusPane();
	
	}
}

/************************************************/
/*						*/
/*	COMMAND 	"SELECTCODE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnReadChar()
{
	CEudcDlg	dlg;

	if( CustomWndOpen){
		if( !OutputSaveMessage())
			return;
	}
	dlg.WorRFlag = FALSE;
	if( dlg.DoModal() == IDOK){
		AfxGetApp()->DoWaitCursor(1);

		EudcWndRect.SetRectEmpty();
		if( !CustomWndOpen){
//			CalcEudcWndRect();
			CEditWnd *pEudcWnd = new CEditWnd;
			if( !pEudcWnd->Create( TEXT("EDITWINDOW"),
			    WS_CHILD | WS_VISIBLE | WS_BORDER,
			    EudcWndRect,this))
				return;
					
			pEditChild = pEudcWnd;
			SetEudcWndPos();			
			pEudcWnd->ShowWindow( SW_SHOWNORMAL);
			pEudcWnd->UpdateWindow();
			CustomWndOpen = TRUE;
		}
		pEditChild->UpdateBitmap();
		pEditChild->UpdateCode = dlg.m_EditList.SelectCode;
		pEditChild->SelectCodes();
		wsprintf( SelectEUDC.m_Code, TEXT("%X"), dlg.m_EditList.SelectCode);
	
		CountryInfo.CurrentRange = dlg.m_EditList.cRange;
		m_wndGuideBar.PositionStatusPane();
	
		AfxGetApp()->DoWaitCursor( -1);
	}
}

/************************************************/
/*						*/
/*	COMMAND		"SELECTCODE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateReadChar(
CCmdUI* pCmdUI)
{
	if( SelectEUDC.m_Font[0] != TEXT('\0') &&
		InqTypeFace((TCHAR *)SelectEUDC.m_Font,
	  ( TCHAR *)SelectEUDC.m_File, sizeof( SelectEUDC.m_File)))
		pCmdUI->Enable( TRUE);
	else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"REGISTER"		*/
/*						*/
/************************************************/
void
CMainFrame::OnSavechar()
{
	AfxGetApp()->DoWaitCursor(1);
	pEditChild->SaveEUDCCode(MB_OK);
	AfxGetApp()->DoWaitCursor(-1);
}

/************************************************/
/*						*/
/*	COMMAND 	"REGISTER" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateSavechar(
CCmdUI* pCmdUI)
{
	if( InqTypeFace((TCHAR *)SelectEUDC.m_Font,
	  ( TCHAR *)SelectEUDC.m_File,
	    sizeof( SelectEUDC.m_File )) && CustomWndOpen)
		pCmdUI->Enable( TRUE);
	else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"REGISTER AS"		*/
/*						*/
/************************************************/
void
CMainFrame::OnSaveCharas()
{
	CEudcDlg	dlg;

	dlg.WorRFlag = TRUE;
	if( dlg.DoModal() == IDOK){
		AfxGetApp()->DoWaitCursor(1);

		pEditChild->UpdateCode = dlg.m_EditList.SelectCode;
		pEditChild->SaveEUDCCode(MB_OK);
		wsprintf( SelectEUDC.m_Code, TEXT("%X"), dlg.m_EditList.SelectCode);
		CountryInfo.CurrentRange = dlg.m_EditList.cRange;
		m_wndGuideBar.PositionStatusPane();

		AfxGetApp()->DoWaitCursor(-1);
	}
}

/************************************************/
/*						*/
/*	COMMAND 	"REGISTER AS" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateSaveCharas(
CCmdUI* pCmdUI)
{		
	if( InqTypeFace((TCHAR *)SelectEUDC.m_Font,
	  ( TCHAR *)SelectEUDC.m_File,
	    sizeof( SelectEUDC.m_File )) && CustomWndOpen)
		pCmdUI->Enable( TRUE);
	else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"IMPORT"		*/
/*						*/
/************************************************/
void
CMainFrame::OnImportFile()
{
	CImportDlg	dlg1;
	BOOL		Tmp;

	if( CustomWndOpen){
		if( !OutputSaveMessage())
			return;
	}
  if (!OExistTTF(SelectEUDC.m_File))
  {
    OutputMessageBox(this->GetSafeHwnd(), IDS_MAINFRAMETITLE, IDS_NOTTE, TRUE);
    return;
  }
	if( dlg1.DoModal() == IDOK){
		if( CustomWndOpen){
			Tmp = FALSE;	
			if( pEditChild->FlagTmp){
				Tmp = TRUE;
				this->SendMessage( WM_COMMAND,
					ID_FIT_CURVE, 0L);
			}
		}
		CGageDlg	dlg2;
		if( dlg2.DoModal() == IDOK){
			;
		}
		if( CustomWndOpen){
			if( Tmp){
				this->SendMessage( WM_COMMAND,
					ID_FIT_CURVE, 0L);
			}
		}
	}else	;
}
/************************************************/
/*						*/
/*	COMMAND		"IMPORT" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateImportFile(
CCmdUI* pCmdUI)
{
	if(InqTypeFace((TCHAR *)SelectEUDC.m_Font,
	               (TCHAR *)SelectEUDC.m_File, 
                 sizeof( SelectEUDC.m_File)))
		pCmdUI->Enable( TRUE);
	else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND		"Select Charset"	*/
/*						*/
/************************************************/

void
CMainFrame::OnSelectCharSet()
{
	CHAR CPCode[2];
	WCHAR UCode[1];
	CEudcDlg dlg;
	UINT LocalCP =GetACP();
	if (m_wndGuideBar.m_comboCharset.GetCurSel() ==
		m_wndGuideBar.m_comboCharset.GetCount()-1){
		//We are switching to unicode.
		if ( CountryInfo.bUnicodeMode )
			// We are already in unicode mode.  No change.
			return;
		CountryInfo.CurrentRange = CountryInfo.nRange-1;
		CountryInfo.bUnicodeMode =TRUE;
		SetTrailByteRange(LocalCP);
		if( SelectEUDC.m_Code[0] != '\0'){
			CPCode[0]= HIBYTE((WORD)Mytcstol(SelectEUDC.m_Code, (TCHAR **)0, 16));
			CPCode[1]= LOBYTE((WORD)Mytcstol(SelectEUDC.m_Code, (TCHAR **)0, 16));
			MultiByteToWideChar(LocalCP, 0, CPCode,2, UCode,1);
			wsprintf( SelectEUDC.m_Code, TEXT("%X"),UCode[0]);
			m_wndGuideBar.PositionStatusPane();
			pEditChild->UpdateCode = UCode[0];
			dlg.m_EditList.SelectCode = UCode[0];
		}
	}else{
		//We are switching to code page
		if ( !CountryInfo.bUnicodeMode )
			// We are already in code page mode.  No change.
			return;

		CountryInfo.bUnicodeMode = FALSE;
		SetTrailByteRange(LocalCP);
		if( SelectEUDC.m_Code[0] != '\0'){
			BOOL bNotConverted;
			UCode[0] = (WCHAR)Mytcstol(SelectEUDC.m_Code, (TCHAR **)0, 16);
			WideCharToMultiByte(LocalCP, 0,UCode,1, CPCode,2, NULL, &bNotConverted);
			TCHAR CodeTmp[2];
			if (bNotConverted){
				OutputMessageBox( this->GetSafeHwnd(),
					IDS_MAINFRAMETITLE,
					IDS_INVALID_CODE_MSG, TRUE);
				
				if( CustomWndOpen){
					CountryInfo.bUnicodeMode = TRUE;
					OutputSaveMessage();
					CountryInfo.bUnicodeMode = FALSE;
				}
				CountryInfo.CurrentRange = 0;
				dlg.m_EditList.SetInitEUDCCodeRange(CountryInfo.CurrentRange);
				dlg.m_EditList.SelectCode=CountryInfo.sRange[CountryInfo.CurrentRange];
				pEditChild->SelectCodes();
				wsprintf( SelectEUDC.m_Code, TEXT("%X"), dlg.m_EditList.SelectCode);
				OnReadChar();
				return;
			}else{
				wsprintf( CodeTmp, TEXT("%X"), (BYTE)CPCode[0]);
				SelectEUDC.m_Code[0] = CodeTmp[0];
				SelectEUDC.m_Code[1] = CodeTmp[1];
				wsprintf( CodeTmp, TEXT("%X"), (BYTE)CPCode[1]);
				SelectEUDC.m_Code[2] = CodeTmp[0];
				SelectEUDC.m_Code[3] = CodeTmp[1];
				dlg.m_EditList.SelectCode = (WORD)Mytcstol(SelectEUDC.m_Code, (TCHAR **)0, 16);
				for (int i=0;i<CountryInfo.nRange - 1; i++)
				{
					if (dlg.m_EditList.SelectCode >= CountryInfo.sRange[i] &&
						dlg.m_EditList.SelectCode <= CountryInfo.eRange[i])
					{
							CountryInfo.CurrentRange = i;
					}
				}
			}
			m_wndGuideBar.PositionStatusPane();
			pEditChild->UpdateCode = dlg.m_EditList.SelectCode;

			
		}
        else
        {
            CountryInfo.CurrentRange = 0;
        }
	}
	dlg.m_EditList.SetInitEUDCCodeRange( CountryInfo.CurrentRange);
}

/************************************************/
/*						*/
/*	COMMAND  	"CALL"			*/
/*						*/
/************************************************/
void
CMainFrame::OnCallChar()
{
	UINT	Result;

	if( CustomWndOpen){
		if( !OutputSaveMessage())
			return;
	}
	if(( Result = SetReffCode( TRUE)) == IDOK){
		pEditChild->UpdateBitmap();
		pEditChild->CallCharTextOut();
	}
}

/************************************************/
/*						*/
/*	COMMAND 	"CALL" (Update)		*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateCallChar(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen)
		pCmdUI->Enable( TRUE);
	else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"LINK"			*/
/*						*/
/************************************************/
void
CMainFrame::OnLinkime()
{
#if 0
	CIMEDlg		dlg;
	dlg.EudcCode = pEditChild->UpdateCode;
	dlg.DoModal();
#endif
	ImeLink( this->GetSafeHwnd(),
		pEditChild->UpdateCode,
		CountryInfo.bUnicodeMode ? TRUE : FALSE,
		AfxGetInstanceHandle());
}

/************************************************/
/*						*/
/*	COMMAND 	"LINK" (Update)		*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateLinkime(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen &&
	  ( CountryInfo.LangID == EUDC_CHT ||
	    CountryInfo.LangID == EUDC_CHS )){
		pCmdUI->Enable(TRUE);
	}else	pCmdUI->Enable(FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"BATCHLINK"		*/
/*						*/
/************************************************/
void
CMainFrame::OnLinkbatchmode()
{
	CBLinkDlg	dlg;

	dlg.DoModal();
}

/************************************************/
/*						*/
/*	COMMAND 	"BATCHLINK" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateLinkbatchmode(
CCmdUI* pCmdUI)
{
#ifdef BUILD_ON_WINNT
//
//  IME batch link should be font independent feature
// and available after launch EUDCEDIT. The batch link
// menu item is grayed after launching EUDCEDIT.
//  It will be enabled after font association and EUDC
// code are selected.
//  But font association and EUDC code selection do not
// have any relationship with IME batch link.
//  The user does not need to select font association
// and EUDC code if they want to do IME batch link only.
//
// NTRaid #19424.
//
	if( CountryInfo.LangID == EUDC_CHT ||
	    CountryInfo.LangID == EUDC_CHS ){
		pCmdUI->Enable(TRUE);
	}else	pCmdUI->Enable(FALSE);
#else
	if( CustomWndOpen &&
	  ( CountryInfo.LangID == EUDC_CHT ||
	    CountryInfo.LangID == EUDC_CHS )){
		pCmdUI->Enable(TRUE);
	}else	pCmdUI->Enable(FALSE);
#endif // BUILD_ON_WINNT
}

/************************************************/
/*						*/
/*	COMMAND 	"GUIDEBAR"		*/
/*						*/
/************************************************/
BOOL
CMainFrame::OnStylesBar(
UINT 	nID)
{
	GuideBarVisible = !GuideBarVisible;


	if( GuideBarVisible)
		m_wndGuideBar.ShowWindow(SW_SHOWNA);
	else	m_wndGuideBar.ShowWindow(SW_HIDE);
	m_wndGuideBar.PositionStatusPane();
	this->SendMessage( WM_SIZE);

	return TRUE;
}

/************************************************/
/*						*/
/*	COMMAND 	"GUIDEBAR" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateStylesBar(
CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(GuideBarVisible);
}

/************************************************/
/*						*/
/*	COMMAND 	"TOOLBOX"		*/
/*						*/
/************************************************/
void
CMainFrame::OnToolbar()
{
    ToolBarVisible = !ToolBarVisible;
	if( ToolBarVisible)
    {
        DockControlBar(&m_wndToolBar);
		m_wndToolBar.ShowWindow( SW_SHOWNORMAL);
	}else{
        if (m_wndToolBar.IsFloating())
        {
		    m_wndToolBar.GetParentOwner()->ShowWindow( SW_HIDE);
        }
        else
        {
		    m_wndToolBar.ShowWindow( SW_HIDE);
        }
	}	
	this->SendMessage(WM_SIZE);
}

/************************************************/
/*						*/
/*	COMMAND		"TOOLBOX" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateToolbar(
CCmdUI* pCmdUI)
{
    ToolBarVisible = m_wndToolBar.IsWindowVisible();
	pCmdUI->SetCheck(ToolBarVisible);
}

/************************************************/
/*						*/
/*	COMMAND 	"GRID"			*/
/*						*/
/************************************************/
void
CMainFrame::OnShowGrid()
{
	pEditChild->GridShow = !pEditChild->GridShow;
	pEditChild->Invalidate( FALSE);
	pEditChild->UpdateWindow();
	if( RefferWndVisible){
		pRefrChild->GridShow = !pRefrChild->GridShow;
		pRefrChild->Invalidate(FALSE);
		pRefrChild->UpdateWindow();
	}
}

/************************************************/
/*						*/
/*	COMMAND 	"GRID" (Update)		*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateShowGrid(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable(1);
		pCmdUI->SetCheck( pEditChild->GridShow);
	}else 	pCmdUI->Enable(0);
}

/************************************************/
/*						*/
/*	COMMAND 	"NEXT CODE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnNextcode()
{
	if( CustomWndOpen){
		if( !OutputSaveMessage())
			return;
	}
	EudcWndRect.SetRectEmpty();
	pEditChild->UpdateBitmap();
	pEditChild->UpdateCode++;
	pEditChild->UpdateCode = CorrectCode( pEditChild->UpdateCode, TRUE);
	pEditChild->SelectCodes();
	wsprintf( SelectEUDC.m_Code, TEXT("%X"), pEditChild->UpdateCode);
	
	m_wndGuideBar.PositionStatusPane();
}

/************************************************/
/*						*/
/*	COMMAND 	"NEXT CODE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateNextcode(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen &&
	  ( InqTypeFace((TCHAR *)SelectEUDC.m_Font,
	  (TCHAR *)SelectEUDC.m_File, sizeof( SelectEUDC.m_File)))){
		if( CountryInfo.eRange[CountryInfo.CurrentRange]
				<= pEditChild->UpdateCode)
			pCmdUI->Enable( FALSE);
		else	pCmdUI->Enable( TRUE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"PREV CODE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnPrevcode()
{
	if( CustomWndOpen){
		if( !OutputSaveMessage())
			return;
	}
	EudcWndRect.SetRectEmpty();
	pEditChild->UpdateBitmap();
	pEditChild->UpdateCode--;
	pEditChild->UpdateCode = CorrectCode( pEditChild->UpdateCode, FALSE);
	pEditChild->SelectCodes();
	wsprintf( SelectEUDC.m_Code, TEXT("%X"), pEditChild->UpdateCode);

	m_wndGuideBar.PositionStatusPane();
}

/************************************************/
/*						*/
/*	COMMAND 	"PREV CODE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePrevcode(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen &&
	  ( InqTypeFace((TCHAR *)SelectEUDC.m_Font,
	  (TCHAR *)SelectEUDC.m_File, sizeof( SelectEUDC.m_File)))){
		if( CountryInfo.sRange[CountryInfo.CurrentRange]
				>= pEditChild->UpdateCode)
			pCmdUI->Enable( FALSE);
		else	pCmdUI->Enable( TRUE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"SHOW OUTLINE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnFitCurve()
{
	if( CustomWndOpen){
		pEditChild->FlagTmp = !pEditChild->FlagTmp;

		pEditChild->Invalidate(FALSE);
		pEditChild->UpdateWindow();
	}
}

/************************************************/
/*						*/
/*	COMMAND 	"SHOW OUTLINE"(Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateFitCurve(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck( pEditChild->FlagTmp);
	}else	pCmdUI->Enable(FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"PEN"			*/
/*						*/
/************************************************/
void
CMainFrame::OnPalettepen()
{
	pEditChild->SelectItem = PEN;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"PEN" (Update)		*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePalettepen(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == PEN)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"BRUSh"			*/
/*						*/
/************************************************/
void
CMainFrame::OnPalettebrush()
{
	pEditChild->SelectItem = BRUSH;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"BRUSH" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePalettebrush(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == BRUSH)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"STRAIGHT LINE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnPaletteslope()
{
	pEditChild->SelectItem = SLOPE;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"STRAIGHT LINE"(Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePaletteslope(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == SLOPE)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"RECTANGLE" 		*/
/*						*/
/************************************************/
void
CMainFrame::OnPaletterect()
{
	pEditChild->SelectItem = RECTBAND;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"RECTANGLE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePaletterect(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == RECTBAND)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"FILRECTANGLE" 		*/
/*						*/
/************************************************/
void
CMainFrame::OnPaletterectfill()
{
	pEditChild->SelectItem = RECTFILL;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"FILRECTANGLE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePaletterectfill(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == RECTFILL)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"CIRCLE" 		*/
/*						*/
/************************************************/
void
CMainFrame::OnPalettecircle()
{
	pEditChild->SelectItem = CIRCLE;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"CIRCLE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePalettecircle(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == CIRCLE)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"FILCIRCLE" 		*/
/*						*/
/************************************************/
void
CMainFrame::OnPalettecirclefill()
{
	pEditChild->SelectItem = CIRCLEFILL;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"FILCIRCLE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePalettecirclefill(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == CIRCLEFILL)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"FREEFORM" 		*/
/*						*/
/************************************************/
void
CMainFrame::OnPalettefreeform()
{
	pEditChild->SelectItem = FREEFORM;
	pRefrChild->SelectItems = FREEFORM;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"FREEFORM" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePalettefreeform(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == FREEFORM)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"ERASER" 		*/
/*						*/
/************************************************/
void
CMainFrame::OnPaletteeraser()
{
	pEditChild->SelectItem = ERASER;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"ERASER" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePaletteeraser(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == ERASER)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"RECTBAND" 		*/
/*						*/
/************************************************/
void
CMainFrame::OnPaletterectband()
{
	pEditChild->SelectItem = RECTCLIP;
	pRefrChild->SelectItems = RECTCLIP;
	pEditChild->WriteSelRectBitmap();
	pEditChild->MDIActivate();
}

/************************************************/
/*						*/
/*	COMMAND 	"RECTBAND"(Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdatePaletterectband(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable( TRUE);
		if( pEditChild->SelectItem == RECTCLIP)
			pCmdUI->SetRadio( TRUE);
		else	pCmdUI->SetRadio( FALSE);
	}else	pCmdUI->Enable( FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"FLIP/ROTATE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnRotate()
{
CRotateDlg	dlg;

	if( CustomWndOpen){
		if( dlg.DoModal() == IDOK){
			pEditChild->FlipRotate( dlg.RadioItem);
		}
	}
}

/************************************************/
/*						*/
/*	COMMAND 	"FLIP/ROTATE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateRotate(
CCmdUI* pCmdUI)
{
	if( CustomWndOpen){
		pCmdUI->Enable(TRUE);
	}else	pCmdUI->Enable(FALSE);
}

/************************************************/
/*						*/
/*	COMMAND 	"REFERRENCE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnRefferfileNew()
{
	UINT	Result;

	if( !RefferWndVisible){
		if(( Result = SetReffCode( FALSE)) == IDOK){
			SetReffWndPos();
			pRefrChild->UpdateBitmap();
			pRefrChild->GridShow = pEditChild->GridShow;
			pRefrChild->ShowWindow(SW_SHOWNORMAL);
			pRefrChild->UpdateWindow();
			RefferWndVisible = TRUE;
			if( CustomWndOpen)
				SetEudcWndPos();
		}
	}else{
		if(( Result = SetReffCode( FALSE)) == IDOK){
			pRefrChild->UpdateBitmap();
		}
	}
}

/************************************************/
/*						*/
/*	COMMAND 	"REFERRENCE" (Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateRefferfileNew(
CCmdUI* pCmdUI)
{
	pCmdUI->Enable( CustomWndOpen);	
}

/************************************************/
/*						*/
/*	COMMAND 	"CLOSE REF"		*/
/*						*/
/************************************************/
void
CMainFrame::OnRefferClose()
{
	RefferWndVisible = FALSE;
	pRefrChild->ShowWindow( SW_HIDE);
	pRefrChild->UpdateWindow();
	SetEudcWndPos();
}

/************************************************/
/*						*/
/*	COMMAND 	"CLOSE REF"(Update)	*/
/*						*/
/************************************************/
void
CMainFrame::OnUpdateRefferClose(
CCmdUI* pCmdUI)
{
	pCmdUI->Enable(RefferWndVisible);	
}

/************************************************/
/*						*/
/*	COMMAND 	"HELP TOPICS"		*/
/*						*/
/************************************************/
void
CMainFrame::OnHelp()
{
	if( this->IsWindowEnabled())
    {
        // BUGBUG: work around for bug in HtmlHelp API (unicode version doesn't work)
		//::HtmlHelp (this->GetSafeHwnd(), ChmHelpPath, HH_DISPLAY_TOPIC,(DWORD)0);
        ::HtmlHelpA (::GetDesktopWindow(), "eudcedit.chm", HH_DISPLAY_TOPIC,(DWORD)0);
    }
}

/************************************************/
/*						*/
/*	if bitmap is dirty, output message	*/
/*						*/
/************************************************/
BOOL
CMainFrame::OutputSaveMessage()
{
	int	Result;

	Result = IDYES;
	if( pEditChild->GetBitmapDirty()){
		Result = OutputMessageBox( this->GetSafeHwnd(),
			IDS_MAINFRAMETITLE,
			IDS_SAVEORNOT, FALSE);
		if( Result == IDYES){
			AfxGetApp()->DoWaitCursor(1);
			if (!pEditChild->SaveEUDCCode(MB_OKCANCEL))
        Result = IDCANCEL;
			AfxGetApp()->DoWaitCursor(-1);
		}else if( Result == IDNO){
			pEditChild->SetBitmapDirty( FALSE);
		}
	}
	if( Result == IDCANCEL)
		return FALSE;
	else	return TRUE;
}

/************************************************/
/*						*/
/*	Create refferrence window		*/
/*						*/
/************************************************/
BOOL
CMainFrame::CreateReferWnd()
{
	ReffWndRect.SetRectEmpty();
	CalcReffWndRect();

	CRefrWnd *pReffWnd = new CRefrWnd;
	if( !pReffWnd->Create( TEXT("REFERWINDOW"),
	    WS_CHILD | WS_BORDER, ReffWndRect, this))
		return FALSE;
				
	pRefrChild = pReffWnd;
	RefferWndVisible = FALSE;

	return TRUE;
}

/************************************************/
/*						*/
/*	Change size of edit window		*/
/*						*/
/************************************************/
void
CMainFrame::SetEudcWndPos()
{
	CalcEudcWndRect();
	pEditChild->MoveWindow( EudcWndRect.left, EudcWndRect.top,
		EudcWndRect.Width(), EudcWndRect.Height(), TRUE);
}

/************************************************/
/*						*/
/*	Calcurate size of edit window		*/
/*						*/
/************************************************/
void
CMainFrame::CalcEudcWndRect()
{
	CRect	CalcRect;
	CSize	Frame;
	int	Twice;
	int	Remain;

	Frame.cx = ( GetSystemMetrics( SM_CXFRAME)
		       - GetSystemMetrics( SM_CXBORDER)) *2;
	Frame.cy = ( GetSystemMetrics( SM_CYFRAME)
		       - GetSystemMetrics( SM_CYBORDER)) *2;

	GetClientRect( &EudcWndRect);
	if( GuideBarVisible){
		m_wndGuideBar.GetWindowRect( &CalcRect);
		EudcWndRect.bottom -= CalcRect.Height();
	}
	if( ToolBarVisible){
		m_wndToolBar.GetWindowRect( &CalcRect);
		EudcWndRect.right -= CalcRect.Width();
	}

	if( RefferWndVisible)
		EudcWndRect.right = EudcWndRect.right /2;

	if( EudcWndRect.Width() < EudcWndRect.Height()){
		Twice  = EudcWndRect.Width() /BITMAP_WIDTH;
		Remain = EudcWndRect.Width() %BITMAP_WIDTH;
		if( Remain < Frame.cx + 4)
			Twice -= 1;
		if( Twice <= 0)		Twice = 1;
		EudcWndRect.left   = 0;
		EudcWndRect.right  = EudcWndRect.left + BITMAP_WIDTH *Twice
				   + Frame.cx;
		EudcWndRect.bottom = BITMAP_HEIGHT*Twice + Frame.cy
				   + CAPTION_HEIGHT;
	}else{
		Twice  = EudcWndRect.Height() /BITMAP_HEIGHT;
		Remain = EudcWndRect.Height() %BITMAP_HEIGHT;
		if( Remain < CAPTION_HEIGHT + Frame.cy + 4)
			Twice -= 1;
		if( Twice <= 0)		Twice = 1;
		EudcWndRect.left   = 0;
		EudcWndRect.right  = EudcWndRect.left + BITMAP_WIDTH *Twice
				   + Frame.cx;
		EudcWndRect.bottom = BITMAP_HEIGHT*Twice + Frame.cy + CAPTION_HEIGHT;
	}
}

/************************************************/
/*						*/
/*	Change size of referrence window	*/
/*						*/
/************************************************/
void
CMainFrame::SetReffWndPos()
{
 	CalcReffWndRect();
	pRefrChild->MoveWindow( ReffWndRect.left, ReffWndRect.top,
		 ReffWndRect.Width(), ReffWndRect.Height(), TRUE);
}

/************************************************/
/*						*/
/*	Calcurate size of referrence window	*/
/*						*/
/************************************************/
void
CMainFrame::CalcReffWndRect()
{
	CRect	CalcRect;
	CSize	Frame;
	int	Twice;
	int	Remain;

	Frame.cx = ( GetSystemMetrics( SM_CXFRAME)
		   - GetSystemMetrics( SM_CXBORDER)) * 2;
  	Frame.cy = ( GetSystemMetrics( SM_CYFRAME)
		   - GetSystemMetrics( SM_CYBORDER)) * 2;
	GetClientRect( &ReffWndRect);
	if( GuideBarVisible){
		m_wndGuideBar.GetWindowRect( &CalcRect);
		ReffWndRect.bottom -= CalcRect.Height();
	}
	if( ToolBarVisible){
		m_wndToolBar.GetWindowRect( &CalcRect);
		ReffWndRect.right -= CalcRect.Width();
	}

	ReffWndRect.left  = ReffWndRect.right /2;
	if( ReffWndRect.Width() < ReffWndRect.Height()){
		Twice  = ReffWndRect.Width() /BITMAP_WIDTH;
		Remain = ReffWndRect.Width() %BITMAP_WIDTH;
		if( Remain < Frame.cx + 4)
			Twice -= 1;
		if( Twice <= 0){/*
			if( RefferWndVisible){
				pRefrChild->ShowWindow( SW_HIDE);
				pRefrChild->UpdateWindow();
				RefferWndVisible = FALSE;
			}*/
		}
		ReffWndRect.right  = ReffWndRect.left + BITMAP_WIDTH*Twice
				   + Frame.cx;
		ReffWndRect.bottom = BITMAP_HEIGHT*Twice + Frame.cy
				   + CAPTION_HEIGHT;
	}else{
		Twice  = ReffWndRect.Height() /BITMAP_HEIGHT;
		Remain = ReffWndRect.Height() %BITMAP_HEIGHT;
		if( Remain < CAPTION_HEIGHT + Frame.cy + 4)
			Twice -= 1;
		if( Twice <= 0)		Twice = 1;
		ReffWndRect.right  = ReffWndRect.left + BITMAP_WIDTH*Twice
				   + Frame.cx;
		ReffWndRect.bottom = BITMAP_HEIGHT*Twice + Frame.cy
				   + CAPTION_HEIGHT;
	}
}

/************************************************/
/*						*/
/*	MESSAGE		"WM_SIZE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnSize(
UINT 	nType,
int 	cx,
int 	cy)
{
	CMDIFrameWnd::OnSize(nType, cx, cy);
	if( RefferWndVisible){
		SetReffWndPos();
	}
	if( CustomWndOpen){
		SetEudcWndPos();
	}

	if( !ToolBarVisible){
		m_wndToolBar.Invalidate( FALSE);
		m_wndToolBar.UpdateWindow();
	}

}

/************************************************/
/*						*/
/*	MESSAGE	"WM_DESTROY"			*/
/*	Write "EUDCEDIT.INI"			*/
/*						*/
/************************************************/
void
CMainFrame::OnDestroy()
{
	TCHAR	ProfileBuf[BUFFERMAX];
	TCHAR	SelectName1[] = TEXT("WindowSize");
	TCHAR	SelectName2[] = TEXT("Bitmap");
	TCHAR	SelectName3[] = TEXT("Color");
	TCHAR	EntryName1[]  = TEXT("MainWindowSize");
	TCHAR	EntryName2[]  = TEXT("MinMaxFlag");
	TCHAR 	EntryName3[]  = TEXT("BitmapSize");
	TCHAR	EntryName4[]  = TEXT("Grid");
	TCHAR	EntryName5[]  = TEXT("Curve");
	TCHAR	EntryName6[]  = TEXT("Fitting");
	TCHAR 	EudcIniFile[] = TEXT("Eudcedit.ini");
	CRect	rect;
	short	MaxWndFlag;

 	if( IsZoomed())
		MaxWndFlag = 1;
	else	MaxWndFlag = 0;

	wsprintf( ProfileBuf,TEXT("%d %d %d"), GetRValue(COLOR_GRID),
		GetGValue(COLOR_GRID), GetBValue(COLOR_GRID));
	::WritePrivateProfileString( SelectName3, EntryName4,
		ProfileBuf, EudcIniFile);
						
	wsprintf( ProfileBuf,TEXT("%d %d %d"), GetRValue(COLOR_CURVE),
		GetGValue(COLOR_CURVE), GetBValue(COLOR_CURVE));
	::WritePrivateProfileString( SelectName3, EntryName5,
		ProfileBuf, EudcIniFile);

	wsprintf( ProfileBuf,TEXT("%d %d %d"), GetRValue(COLOR_FITTING),
		GetGValue(COLOR_FITTING), GetBValue(COLOR_FITTING));
	::WritePrivateProfileString( SelectName3, EntryName6,
		ProfileBuf, EudcIniFile);

	wsprintf( ProfileBuf, TEXT("%d"), BITMAP_WIDTH);
	::WritePrivateProfileString( SelectName2, EntryName3,
		ProfileBuf, EudcIniFile);

	wsprintf( ProfileBuf, TEXT("%d"), MaxWndFlag);
	::WritePrivateProfileString( SelectName1, EntryName2,
		ProfileBuf, EudcIniFile);

	if( !MaxWndFlag && !IsIconic()){	
		GetWindowRect( &rect);
		wsprintf( ProfileBuf, TEXT("%d %d %d %d"), rect.left, rect.top,
			rect.right, rect.bottom);

		::WritePrivateProfileString( SelectName1, EntryName1,
			ProfileBuf, EudcIniFile);
	}
	CMDIFrameWnd::OnDestroy();
}

/************************************************/
/*						*/
/*	MESSAGE		"WM_CLOSE"		*/
/*						*/
/************************************************/
void
CMainFrame::OnClose()
{
	CString	MsgTitle;
	CString Msg;
	int	result;

	if( CustomWndOpen){
		if(( pEditChild->GetBitmapDirty()) &&
		   ( InqTypeFace((TCHAR *)SelectEUDC.m_Font,
		   (TCHAR *)SelectEUDC.m_File, sizeof( SelectEUDC.m_File)))){
			MsgTitle.LoadString( IDS_MAINFRAMETITLE);
			Msg.LoadString( IDS_SAVEORNOT);
			result = MessageBox( Msg, MsgTitle,
					 MB_ICONQUESTION | MB_YESNOCANCEL);
			if( result == IDYES){
				AfxGetApp()->DoWaitCursor(1);
				if (!pEditChild->SaveEUDCCode(MB_OKCANCEL))
          result = IDCANCEL;
				AfxGetApp()->DoWaitCursor(-1);
			}
			if( result == IDCANCEL)
				return;
		}
	}		
	CMDIFrameWnd::OnClose();
}

/************************************************/
/*						*/
/*	MESSAGE		"WM_INITMENU"		*/
/*						*/
/************************************************/
void
CMainFrame::OnInitMenu(
CMenu* 	pMenu)
{
	CMDIFrameWnd::OnInitMenu(pMenu);

	if( CountryInfo.LangID != EUDC_CHT &&
	    CountryInfo.LangID != EUDC_CHS ){
		pMenu->DeleteMenu( ID_LINKIME, MF_BYCOMMAND);
		pMenu->DeleteMenu( ID_LINKBATCHMODE, MF_BYCOMMAND);
		if (CountryInfo.LangID != EUDC_JPN)
			pMenu->DeleteMenu( ID_IMPORT_FILE, MF_BYCOMMAND);
		this->DrawMenuBar();
	}
}

/************************************************/
/*						*/
/*	Correct it if code is illegal range	*/
/*						*/
/************************************************/
WORD
CMainFrame::CorrectCode(
WORD 	Code,
BOOL 	UporDown)
{
COUNTRYINFO	Info;
	WORD	wCode;
	BYTE 	LByte, HByte;
	int	TralPos = 0;

	LByte = LOBYTE( Code);
	HByte = HIBYTE( Code);
	Info = CountryInfo;
	for( int i = 0; i < Info.nTralByte; i++){
		if( LByte >= (Info.sTralByte[i]) &&
		    LByte <= (Info.eTralByte[i])){
		    	goto RET;
		}else if( LByte < (Info.sTralByte[i])){
			TralPos = i;
			break;
		}else	TralPos = i+1;
	}
	if( UporDown)
		if( TralPos == Info.nTralByte){
			LByte = (Info.sTralByte[0]);
			HByte += 0x1;
		}else 	LByte = (Info.sTralByte[TralPos]);
	else{
		if( !TralPos){
			LByte = (Info.eTralByte[Info.nTralByte-1]);
			HByte -= 0x1;
		}else	LByte = (Info.eTralByte[TralPos-1]);
	}
RET:
	wCode = MAKEWORD( LByte, HByte);
	return wCode;
}

/************************************************/
/*						*/
/*	Set Reference dialog			*/
/*						*/
/************************************************/
UINT
CMainFrame::SetReffCode( BOOL EditorRefer)
{
CRefrDlg	dlg;
	UINT	Result;

	TitleFlag = EditorRefer;
	if(( Result = (UINT)dlg.DoModal()) == IDOK){
		if( !EditorRefer){
			if( dlg.m_CodeList.SelectCode){
				pRefrChild->ReferCode =
					dlg.m_CodeList.SelectCode;
			}else	pRefrChild->ReferCode = 0;
		}else{
			if( dlg.m_CodeList.SelectCode)
				pEditChild->CallCode =
					dlg.m_CodeList.SelectCode;
			else	pEditChild->CallCode = 0;
		}
	}else{
		if( !EditorRefer)
			pRefrChild->ReferCode = 0;
		else 	pEditChild->CallCode = 0;
	}

	return Result;
}

/************************************************/
/*						*/
/*	Window procedure			*/
/*						*/
/************************************************/
LRESULT
CMainFrame::WindowProc(
UINT 	message,
WPARAM 	wParam,
LPARAM 	lParam)
{
	CRect	WorkRect;
	CRect	rect;
	CRect	Dummy;
	RECT	*Rect;
	POINT	*Point;
	int	Zm;

	if( message == WM_DUPLICATE){
		Zm = pEditChild->ZoomRate;
		pEditChild->GetClientRect( &WorkRect);
		pEditChild->ClientToScreen( &WorkRect);
		Point = (LPPOINT)lParam;
		Rect  = (LPRECT)wParam;
		pEditChild->ScreenToClient( Point);

		Rect->left = ((Point->x - Rect->left) /Zm) *Zm;
		Rect->top  = ((Point->y - Rect->top - CAPTION_HEIGHT)
				/Zm) *Zm + CAPTION_HEIGHT;
		Rect->right  = Rect->left + Rect->right;
		Rect->bottom = Rect->top  + Rect->bottom;

		pEditChild->ClientToScreen( Rect);
		if( Dummy.IntersectRect( &WorkRect, Rect)){
			if( Dummy.EqualRect( Rect)){
				pEditChild->ScreenToClient( Rect);
				pEditChild->SetDuplicateRect( Rect, Point);
			}else{
				int	Tmp;

				rect.SetRect( Rect->left, Rect->top,
						Rect->right, Rect->bottom);
				if( rect.left < WorkRect.left){
					Tmp = WorkRect.left - rect.left;
					rect.OffsetRect( Tmp, 0);
				}
				if( rect.top < WorkRect.top + CAPTION_HEIGHT){
					Tmp = WorkRect.top + CAPTION_HEIGHT
						- rect.top;
					rect.OffsetRect( 0, Tmp);
				}
				if( rect.right > WorkRect.right){
					Tmp = WorkRect.right - rect.right + 1;
					rect.OffsetRect( Tmp, 0);
				}
				if( rect.bottom > WorkRect.bottom){
					Tmp = WorkRect.bottom - rect.bottom + 1;
					rect.OffsetRect( 0, Tmp);
				}
				pEditChild->ScreenToClient( &rect);
				pEditChild->SetDuplicateRect( &rect, Point);
			}
			pRefrChild->RectVisible = FALSE;
			pEditChild->MDIActivate();
			if( pRefrChild->SelectItems == FREEFORM)
				pEditChild->SelectItem = FREEFORM;
			else	pEditChild->SelectItem = RECTCLIP;
		}else{
			pRefrChild->Invalidate( FALSE);
			pRefrChild->UpdateWindow();
		}
		return (0L);
	}else 	return CFrameWnd::WindowProc( message, wParam, lParam);
}

/************************************************/
/*						*/
/*	Activate Edit Window			*/
/*						*/
/************************************************/
BOOL CMainFrame::CustomActivate()
{
	pEditChild->MDIActivate();
	return TRUE;
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}
void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}
#endif //_DEBUG
