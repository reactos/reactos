/**************************************************/
/*					                              */
/*					                              */
/*		Guide Bar 		                          */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/ 

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"guidebar.h"
#include	"registry.h"
#include	"util.h"




#define		GUIDEHIGH	50
#define		COMBOWIDTH	55
#define		COMBOHEIGHT	200



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CGuideBar, CStatusBar)
BEGIN_MESSAGE_MAP( CGuideBar, CStatusBar)
	//{{AFX_MSG_MAP(CGuideBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Constructor			*/
/*					*/
/****************************************/
CGuideBar::CGuideBar()
{
	m_comboBoxAdded = FALSE;
}

/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CGuideBar::~CGuideBar()
{
}

/****************************************/
/*					*/
/*	Create GuideBar 		*/
/*					*/
/****************************************/
BOOL 
CGuideBar::Create(
CWnd* 	pOwnerWnd, 
UINT 	pID)
{

	LONG lStyle;
	//
	// We don't want the sizegrip for this status bar because it is at
	// the top of the frame.  However MFC creates a sizegrip if the
	// parent window has a thickframe.  We temporarily turn off the bit
	// to fool MFC so that SBARS_GRIPSIZE is not set.
	//
	lStyle = ::SetWindowLong(pOwnerWnd->GetSafeHwnd(),
                             GWL_STYLE,
                             (pOwnerWnd->GetStyle() & ~WS_THICKFRAME));
	m_pOwnerWnd = pOwnerWnd;
	if (!CStatusBar::Create( pOwnerWnd, WS_CHILD | WS_VISIBLE
           | CBRS_TOP | CBRS_NOALIGN, AFX_IDW_STATUS_BAR))
	{
		return FALSE;
	}

	::SetWindowLong(pOwnerWnd->GetSafeHwnd(),  GWL_STYLE,  lStyle);
	return TRUE;
}

int
CGuideBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CRect rect;
	TCHAR	CharBuf[MAX_PATH];
	UINT nID = 0;

	if (CStatusBar::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_comboCharset.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP |
		CBS_DROPDOWNLIST | CBS_HASSTRINGS, rect, this, IDC_SELECTCHARSET)){
		return -1;
	}

	m_comboCharset.SendMessage(WM_SETFONT, (WPARAM) CStatusBar::GetFont()->GetSafeHandle());


	switch (CountryInfo.LangID){
	case EUDC_JPN:
		nID=IDS_SHIFTJIS;
		break;
	case EUDC_CHS:
		nID=IDS_GB2312;
		break;
	case EUDC_KRW:
		nID=IDS_HANGUL;
		break;
	case EUDC_CHT:
		nID=IDS_CHINESEBIG5;
		break;
	}

	if (nID){
		GetStringRes(CharBuf, nID);
		m_comboCharset.AddString(CharBuf);
	}

	GetStringRes(CharBuf, IDS_UNICODE);
	m_comboCharset.AddString(CharBuf);
	m_comboCharset.SetCurSel(0);
	return TRUE;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void 
CGuideBar::PositionStatusPane()
{

	TCHAR	CharBuf[MAX_PATH], BufTmp[MAX_PATH], *FilePtr;
	int	nWidth;
	UINT nStyle, nID;
	CRect rect;
	CSize StringSize;
	

	GetPaneInfo(0,nID,nStyle,nWidth);
	CDC* dc = this->GetDC();
	int nComboWidth;
	int nDlgBaseUnitX = LOWORD(GetDialogBaseUnits());
	nComboWidth = (nDlgBaseUnitX * COMBOWIDTH + 2) /4;

	if (!m_comboBoxAdded)
	{
		SetPaneInfo(0,nID, nStyle | SBPS_NOBORDERS,nWidth+nComboWidth);
		m_comboCharset.SetWindowPos( NULL, nWidth+nDlgBaseUnitX, 0,
			nComboWidth, COMBOHEIGHT, SWP_NOZORDER);
		m_comboBoxAdded = TRUE;
	}
	

//	Draw "Code:"
	if( SelectEUDC.m_Code[0] != '\0'){
		GetStringRes(CharBuf, IDS_CODE_STR);
		lstrcat( CharBuf, TEXT(" "));
		lstrcat( CharBuf, SelectEUDC.m_Code);
		SetPaneText (1, CharBuf, TRUE);	
		StringSize = dc->GetTextExtent(CharBuf,lstrlen(CharBuf)); 
  	GetPaneInfo(1,nID,nStyle,nWidth);
		SetPaneInfo(1,nID,nStyle,StringSize.cx + 3*nDlgBaseUnitX);
	}
	if( SelectEUDC.m_Font[0] != '\0'){
		GetStringRes(CharBuf, IDS_FONT_STR);
		lstrcat( CharBuf, TEXT(" "));
		lstrcat( CharBuf, SelectEUDC.m_Font);
		SetPaneText (2, CharBuf, TRUE);
		StringSize = dc->GetTextExtent(CharBuf,lstrlen(CharBuf)); 
		GetPaneInfo(2,nID,nStyle,nWidth);
		SetPaneInfo(2,nID,nStyle,StringSize.cx + 3*nDlgBaseUnitX);
	}
	

	if( SelectEUDC.m_Font[0] != TEXT('\0') && InqTypeFace(SelectEUDC.m_Font,
		SelectEUDC.m_File, sizeof( SelectEUDC.m_File))){
		GetStringRes(CharBuf, IDS_FILE_STR);
		lstrcat( CharBuf, TEXT(" "));
		lstrcpy( BufTmp, SelectEUDC.m_FileTitle);
		if(( FilePtr = Mytcsrchr( BufTmp, '.')) != NULL)
			*FilePtr = '\0';
		if( lstrlen((const TCHAR *)BufTmp) > 20){
			BufTmp[20] = '\0';
			lstrcat(BufTmp, TEXT(".."));
		}
		lstrcat( CharBuf, BufTmp);
		SetPaneText (3, CharBuf, TRUE);
		StringSize = dc->GetTextExtent(CharBuf,lstrlen(CharBuf)); 
		GetPaneInfo(3,nID,nStyle,nWidth);
		SetPaneInfo(3,nID,nStyle,StringSize.cx + 3*nDlgBaseUnitX);
	}
	UpdateWindow();
    this->ReleaseDC(dc);

}


#ifdef _DEBUG
void CGuideBar::AssertValid() const
{
	CStatusBar::AssertValid();
}
void CGuideBar::Dump(CDumpContext& dc) const
{
	CStatusBar::Dump(dc);
}
#endif //_DEBUG
