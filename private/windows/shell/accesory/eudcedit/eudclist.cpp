/**************************************************/
/*						                          */
/*						                          */
/*   EUDC Character List ( Japan, China, Korea)	  */
/*						                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"eudclist.h"
#include	"util.h"

#ifdef BUILD_ON_WINNT
#include    "extfunc.h"
#endif // BUILD_ON_WINNT


/* Matrics of Characterlist */
#define		NUM_CHAR	16	// Row  of matrics
#define		NUM_LINE	6	// Line of matrics

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE( CEudcList, CEdit)
BEGIN_MESSAGE_MAP( CEudcList, CEdit)
	//{{AFX_MSG_MAP( CEudcList)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SETCURSOR()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#ifdef BUILD_ON_WINNT
static CRect	rcColumnHead[NUM_CHAR];
#endif //BUILD_ON_WINNT
static CRect	rcEditChar[NUM_LINE][NUM_CHAR];
static CRect	rcEditCode[NUM_LINE];
static BYTE	ViewCode[NUM_LINE];
static BOOL   bHasGlyph;
#define EUDCCODEBASE    ((unsigned short)0xe000)

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CEudcList::CEudcList()
{
//	Initialize parameter( Japan, China and Korea)
	SetInitEUDCCodeRange( CountryInfo.CurrentRange);
	FocusFlag = FALSE;
}

/****************************************/
/*					*/
/*	Set coderange of EUDC		*/
/*					*/
/****************************************/
void
CEudcList::SetInitEUDCCodeRange(
int 	nIndex)
{
	m_Index = nIndex;
	EUDC_SCode = CountryInfo.sRange[m_Index];
	EUDC_ECode = CountryInfo.eRange[m_Index];

    EUDC_EView =(CountryInfo.eRange[m_Index] & 0xfff0) -(0x10*(NUM_LINE-1));
	if( SelectEUDC.m_Code[0] != '\0'){
		SelectCode = (WORD)Mytcstol(SelectEUDC.m_Code, (TCHAR **)0, 16);
		EUDC_SView = (WORD)Mytcstol(SelectEUDC.m_Code, (TCHAR **)0, 16);
		EUDC_SView &= 0xfff0;
		if( EUDC_EView <= EUDC_SView)
			EUDC_SView = EUDC_EView;
	}else{
		EUDC_SView = CountryInfo.sRange[m_Index] & 0xfff0;
		SelectCode = EUDC_SCode;
		
	}
	
#ifdef BUILD_ON_WINNT
    //In case of CHS, each range will have a different trail byte range
    //after user selects a new range, we have to correct them with
    //sOrigTralByte and eOrigTralByte and form new trail byte range.
    if (CountryInfo.LangID == EUDC_CHS && !CountryInfo.bUnicodeMode)
        CorrectTrailByteRange(m_Index);
#endif // BUILD_ON_WINNT

	LButtonPt.x = LButtonPt.y = 0;
	ScrlBarPos = (short)GetBarPosition( EUDC_SView);
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);
}

/****************************************/
/*					*/
/*	Set coderange of EUDC		*/
/*					*/
/****************************************/
void
CEudcList::SetEUDCCodeRange(
int 	nIndex)
{
	m_Index = nIndex;
	EUDC_SView = CountryInfo.sRange[m_Index] & 0xfff0;
    EUDC_EView =(CountryInfo.eRange[m_Index] & 0xfff0) -(0x10*(NUM_LINE-1));
	EUDC_SCode = CountryInfo.sRange[m_Index];
	EUDC_ECode = CountryInfo.eRange[m_Index];

#ifdef BUILD_ON_WINNT
    //In case of CHS, each range will have a different trail byte range
    //after user selects a new range, we have to correct them with
    //sOrigTralByte and eOrigTralByte and form new trail byte range.
    if (CountryInfo.LangID == EUDC_CHS && !CountryInfo.bUnicodeMode)
        CorrectTrailByteRange(m_Index);
#endif // BUILD_ON_WINNT

	SelectCode = (WORD)EUDC_SCode;
	LButtonPt.x = LButtonPt.y = 0;
	ScrlBarPos = (short)0;
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);
}


/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CEudcList::~CEudcList()
{
    	SysFFont.DeleteObject();
   	EUDCFont.DeleteObject();
}

/****************************************/
/*					*/
/*	Correct it if code is illegal 	*/
/*					*/
/****************************************/
WORD
CEudcList::CorrectEUDCCode(
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

    //decide code falls in which trail byte range
	for( int i = 0; i < Info.nTralByte; i++){
		if( LByte >= (Info.sTralByte[i] & 0xf0) &&
		    LByte <= (Info.eTralByte[i] & 0xf0)){
		    	goto RET;
		}else if( LByte < (Info.sTralByte[i] & 0xf0)){
			TralPos = i;
			break;
		}else	TralPos = i+1;
	}

#ifdef BUILD_ON_WINNT
    // If we are going up on the code range, make the code in the first
    // first line of next valid trailbyte. Otherwise make the code in
    // the last line the preivous valid range.
	if( UporDown){
		if( TralPos == Info.nTralByte){
            TralPos =0;
			HByte += 0x1;
		}
        LByte = Info.sTralByte[TralPos] & 0xf0;
	}else{
		if( TralPos ==0 ){
			HByte -= 0x1;
            TralPos = Info.nTralByte;
        }
	    LByte = Info.eTralByte[TralPos-1] & 0xf0;
	}
#else
	if( UporDown)
		if( TralPos == Info.nTralByte){
			LByte = (Info.sTralByte[0] & 0xf0);
			HByte += 0x1;
		}else 	LByte = (Info.sTralByte[TralPos] & 0xf0);
	else{
		if( !TralPos){
			LByte = (Info.eTralByte[Info.nTralByte-1] & 0xf0);
			HByte -= 0x1;
		}else	LByte = (Info.eTralByte[TralPos-1] & 0xf0);
	}
#endif // BUILD_ON_WINNT

RET:
	wCode = MAKEWORD( LByte, HByte);
	return wCode;
}

/****************************************/
/*					*/
/*	Get position of scrollbar	*/
/*					*/		
/****************************************/
#ifdef BUILD_ON_WINNT
int
CEudcList::GetBarPosition(
WORD 	Code)
{
COUNTRYINFO	Info;
	WORD 	NumPage=0, NumCode=0;
	BYTE	Space[MAX_LEADBYTES];
	int	TotalSpace=0,TralPos=0;
	int	Err;
    BOOL ValidCode=FALSE;

	Info = CountryInfo;
	NumPage = HIBYTE( Code) - HIBYTE( EUDC_SCode);

    // Calculate each code space block for each trail byte range
    // and decide how many of each we count.

    /* space before first trailbyte range */
	Space[0] = ( Info.sTralByte[0] & 0xf0);
	for( int i = 1; i < Info.nTralByte; i++){
		Space[i] = (( Info.sTralByte[i]   & 0xf0)
			 -  ( Info.eTralByte[i-1] & 0xf0) - 0x10);
	}

    /* space after last trailbyte range */
	Space[i] = (0xff - Info.eTralByte[Info.nTralByte-1]) & 0xf0;

    for (i = 0; i < Info.nTralByte; i++)
        if( LOBYTE( Code) >= (Info.sTralByte[i] & 0xf0) &&
            LOBYTE( Code) <= (Info.eTralByte[i] & 0xff)){
            ValidCode=TRUE; //within our trail byte range.
            TralPos=i;
            break;
        }

    if (!ValidCode)
        return(0);  //not within our trail byte range.

    for (i = 0; i < Info.nTralByte; i++){
        if (Info.sTralByte[TralPos] >  LOBYTE( EUDC_SCode )){
            if((Info.sTralByte[i])  > LOBYTE(EUDC_SCode)  && i <= TralPos)
                TotalSpace += Space[i] * (NumPage + 1);
            else
                TotalSpace += Space[i] * NumPage ;
        }else{
            if((Info.sTralByte[i] & 0xf0) < LOBYTE(EUDC_SCode) && i > TralPos)
                TotalSpace += Space[i] * (NumPage - 1);
            else
                TotalSpace += Space[i] * NumPage ;
        }

	}
    TotalSpace += Space[i] * NumPage;

    Err = EUDC_SCode & 0x000f;
	NumCode = Code - EUDC_SCode - TotalSpace + Err;
	return( NumCode /NUM_CHAR);
}
#else
int
CEudcList::GetBarPosition(
WORD 	Code)
{
COUNTRYINFO	Info;
	WORD 	NumPage, NumCode;
	BYTE	Space[MAX_LEADBYTES];
	int	LoSpace, HiSpace;
	int	Err;

	Info = CountryInfo;
	NumPage = HIBYTE( Code) - HIBYTE( EUDC_SCode);
	Space[0] = ( Info.sTralByte[0] & 0xf0);
	Space[1] = 0x00;
	for( int i = 1; i < Info.nTralByte; i++){
		Space[i] = (( Info.sTralByte[i]   & 0xf0)
			 -  ( Info.eTralByte[i-1] & 0xf0) - 0x10);
	}

	for( i = 0; i < Info.nTralByte; i++){
		if( LOBYTE( Code) >= (Info.sTralByte[i] & 0xf0) &&
		    LOBYTE( Code) <= (Info.eTralByte[i] & 0xff)){
		    	if( LOBYTE( EUDC_SCode) > Info.sTralByte[0]){
				LoSpace = ( Space[0] * NumPage);
				HiSpace = ( Space[1] * (i + NumPage - 1));
			}else{
				LoSpace = ( Space[0] * NumPage);
				HiSpace = ( Space[1] * (i + NumPage));
			}
			Err = EUDC_SCode & 0x000f;
			NumCode = Code - EUDC_SCode - LoSpace - HiSpace + Err;
			break;
		}
	}
	return( NumCode /NUM_CHAR);
}
#endif // BUILD_ON_WINNT

/****************************************/
/*					*/
/*	Calculate bar pos from code	*/
/*					*/
/****************************************/
WORD
CEudcList::GetCodeScrPos(
int 	Pos)
{
	WORD	i;
	WORD	Code = 0;
	WORD	NumLine  = 0;
	WORD	PNumLine = 0;

	if( !Pos)
        	return( EUDC_SCode & 0xfff0);
#ifdef BUILD_ON_WINNT
    // we dont't need to go through each char, instead we can examine each
    // line to make this faster
	for( i = EUDC_SCode; i <= EUDC_ECode;  i+= NUM_CHAR){
#else
	for( i = EUDC_SCode; i <= EUDC_ECode;  ++i){
#endif // BUILD_ON_WINNT
    		NumLine = (WORD)GetBarPosition( i);
        	if( NumLine >= Pos){
        		NumLine = PNumLine;
            		break;
        	}
        	PNumLine = NumLine;
   	}
	Code = i;
	Pos -= NumLine;
	Code &= 0xfff0;

	return Code;
}

#define	FIX_SPACE	6
#define	LINEWIDTH	4
/****************************************/
/*					*/
/*	Calcurate character size 	*/
/*					*/
/****************************************/
void
CEudcList::CalcCharSize()
{
	TCHAR	Dummy[] = TEXT("FA40");
	int	Sx;
	int	OldMode;

	CClientDC	dc( this);

	this->GetClientRect( &EditListRect);
	OldMode = dc.SetMapMode( MM_TEXT);

	CFont	*OldFont = dc.SelectObject( &SysFFont);
	GetTextExtentPoint32( dc.GetSafeHdc(), Dummy, 4, &FixSize);
	FixSize.cx += FIX_SPACE;
	dc.SelectObject( OldFont);

	CharSize.cy = ( EditListRect.Height()
		- (NUM_LINE-1)*LINEWIDTH - 2) / NUM_LINE;
	CharSize.cx = ( EditListRect.Width()
		- FixSize.cx - 2 - (NUM_CHAR*LINEWIDTH)) / NUM_CHAR;

	for( int i = 0; i < NUM_LINE; i++){
		rcEditCode[i].left   = 1;
		rcEditCode[i].top    = 1 + i*( CharSize.cy + LINEWIDTH);
		rcEditCode[i].right  = rcEditCode[i].left + FixSize.cx;
		rcEditCode[i].bottom = rcEditCode[i].top  + CharSize.cy;
		Sx = rcEditCode[i].right + LINEWIDTH;
		for( int j = 0; j < NUM_CHAR; j++){
			rcEditChar[i][j].left   = Sx +j*(CharSize.cx+LINEWIDTH);
			rcEditChar[i][j].top    = rcEditCode[i].top;
			rcEditChar[i][j].right  = rcEditChar[i][j].left
						+ CharSize.cx;
			rcEditChar[i][j].bottom = rcEditChar[i][j].top
						+ CharSize.cy;
		}
	}
#ifdef BUILD_ON_WINNT
	for( int j = 0; j < NUM_CHAR; j++){
		rcColumnHead[j].left   = Sx +j*(CharSize.cx+LINEWIDTH);
		rcColumnHead[j].top    = 1;
		rcColumnHead[j].right  = rcColumnHead[j].left + CharSize.cx;
		rcColumnHead[j].bottom = 1 + FixSize.cy;
    }
#endif //BUILD_ON_WINNT
	dc.SetMapMode( MM_TEXT);
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CEudcList::OnPaint()
{
	register int	i, j, k;
	WORD    Code;
	CFont	*OldFont;
	BYTE    Glyph[800];
	BOOL    bGlyph = FALSE;

	CPaintDC	dc( this);
	int BottomCode = GetBarPosition((WORD)EUDC_EView);
	SetScrollRange( SB_VERT, 0, BottomCode, FALSE);
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);

	Code = EUDC_SView;
	int OldMapMode = dc.SetMapMode( MM_TEXT);
	dc.SetViewportOrg( 0, 0);
	OldFont = dc.SelectObject( &SysFFont);

	//
	// access *.euf to set bits in the array (800 * 8 = 6400)
	// to indicate a char has a glyph
	//
	bGlyph = GetGlyph(SelectEUDC.m_File, Glyph);
	for( i = 0; i < NUM_LINE; i++){
		BYTE	CodeArray[10];
		int	xOffset, yOffset;

//	Check character code
		dc.SelectObject( &SysFFont);
		Code = CorrectEUDCCode( Code, TRUE);
		wsprintf((LPTSTR)CodeArray, TEXT("%04X"), Code);

		dc.SetBkColor( COLOR_FACE);
		dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); // COLOR_BLACK);
		if( rcEditChar[i][0].Height() > FixSize.cy){
			yOffset = ( rcEditChar[i][0].Height() - FixSize.cy) /2;
		}else	yOffset = 0;

    ExtTextOut( dc.GetSafeHdc(), rcEditCode[i].left,
				rcEditCode[i].top  + yOffset,
				ETO_OPAQUE, &rcEditCode[i],
				(LPCTSTR)CodeArray, 4, NULL);

		dc.SelectObject( &EUDCFont);
		for( j = 0; j < NUM_CHAR; j++,Code++){
			BYTE	sOffset;
			CSize	cSize;
			BOOL	flg;
			int	wLength;

            //
            // see if the character has a glyph
            //
            bHasGlyph = TRUE;
            if(bGlyph == TRUE)
            {
                WORD  wIndex;
								if (CountryInfo.bUnicodeMode)
									wIndex = Code - EUDCCODEBASE;
								else
								{
									CHAR  pchar[3];
									WCHAR wpchar[2];
									pchar[0] = HIBYTE(Code);
									pchar[1] = LOBYTE(Code);
									pchar[2] = 0;
									MultiByteToWideChar(CP_ACP, 0, pchar, 2, wpchar, 1);
									wIndex = wpchar[0] - EUDCCODEBASE;
								}
                if((Glyph[wIndex>>3] & (0x80>>(wIndex%8))) == 0)
                    bHasGlyph = FALSE;
            }
            //fix for FontIsLinked
            else
              bHasGlyph = FALSE;
            //

			flg = FALSE;
			sOffset = LOBYTE( Code);
			for( k = 0; k < CountryInfo.nTralByte; k++){
				if( sOffset >= CountryInfo.sTralByte[k] &&
				    sOffset <= CountryInfo.eTralByte[k]){
				    	flg = TRUE;
				}
			}
			if((Code < CountryInfo.sRange[cRange])
			||(Code > CountryInfo.eRange[cRange]))
				flg = FALSE;


			if( !flg) 	continue;

			if (CountryInfo.bUnicodeMode){
                CodeArray[1] = HIBYTE(Code);
                CodeArray[0] = LOBYTE(Code);
                wLength = 1;
      }
			else{
                CodeArray[0] = HIBYTE(Code);
                CodeArray[1] = LOBYTE(Code);
								wLength = 2;
      }
    	CodeArray[2] = (BYTE)'\0';
			
			BOOL	PtIn;

			if(( rcEditChar[i][j].PtInRect( LButtonPt) ||
			     SelectCode == Code) && wLength != 0){
				TCHAR	CodeNum[10];

//				If character is selected by left clickking ,
//				Put it on dialog.
				PtIn = TRUE;
				SelectCode = Code;
				dc.SetBkColor(COLOR_FACE);
				dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); // COLOR_BLACK);
				wsprintf(CodeNum, TEXT("%04X"),Code);

				GetParent()->SetDlgItemText(IDC_CODECUST,CodeNum);

        if (CountryInfo.bUnicodeMode)
        {
           lstrcpyW((WCHAR *)ViewCode, (WCHAR *)CodeArray);
        }
        else
        {
           lstrcpyA((CHAR *)ViewCode, (CHAR *)CodeArray);
        }
				GetParent()->SendMessage( WM_VIEWUPDATE, 0, 0);
			}else{
				PtIn = FALSE;
				dc.SetBkColor( COLOR_FACE);
				dc.SetTextColor( GetSysColor(COLOR_WINDOWTEXT));  //COLOR_BLACK);
			}

      if (CountryInfo.bUnicodeMode)
			  GetTextExtentPoint32W( dc.GetSafeHdc(), (LPCWSTR)CodeArray, wLength, &cSize);
      else
    		GetTextExtentPoint32A( dc.GetSafeHdc(),	(LPCSTR)CodeArray, wLength, &cSize);

			if( rcEditChar[i][j].Width() > cSize.cx){
				xOffset = rcEditChar[i][j].Width() - cSize.cx;
				xOffset /= 2;
			}else	xOffset = 0;

			if( rcEditChar[i][j].Height() > cSize.cy){
				yOffset = rcEditChar[i][j].Height() - cSize.cy;
				yOffset /= 2;
			}else	yOffset = 0;

            if(bHasGlyph == TRUE)
            {
                if (CountryInfo.bUnicodeMode)
                {
#ifdef UNICODE
                    ExtTextOutW(dc.GetSafeHdc(),rcEditChar[i][j].left + xOffset,
                        rcEditChar[i][j].top + yOffset, ETO_OPAQUE,
                        &rcEditChar[i][j], (LPCWSTR)CodeArray, wLength, NULL);
		
#else
                    CHAR ViewTmp[2];
                    wLength = WideCharToMultiByte(CP_ACP, 0, (LPWSTR) CodeArray, 1, ViewTmp, 2, NULL, NULL);
                    ExtTextOutA(dc.GetSafeHdc(),rcEditChar[i][j].left + xOffset,
                        rcEditChar[i][j].top + yOffset, ETO_OPAQUE,
                        &rcEditChar[i][j], (LPCSTR)ViewTmp, wLength, NULL);
#endif
                }
                else
                {
                    ExtTextOutA(dc.GetSafeHdc(), rcEditChar[i][j].left + xOffset,
                        rcEditChar[i][j].top + yOffset, ETO_OPAQUE,
                        &rcEditChar[i][j], (LPCSTR)CodeArray,
                        wLength, NULL);
                }
            }
			DrawConcave( &dc, rcEditChar[i][j], PtIn);
		}
	}
	dc.SetMapMode( OldMapMode);
	dc.SelectObject( OldFont);

	LButtonPt.x = LButtonPt.y = 0;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_VSCROLL"		*/
/*					*/
/****************************************/
void
CEudcList::OnVScroll(
UINT 	nSBCode,
UINT 	nPos,
CScrollBar* pScrollBar)
{
	int	MoveScr;
	BOOL	ThumbTrkFlag;
	BOOL	ThumbPosFlag;

	MoveScr = 0;
	ThumbTrkFlag = FALSE;
	ThumbPosFlag = FALSE;
	BarRange = GetBarPosition((WORD)EUDC_EView);
	SetScrollRange( SB_VERT, 0, BarRange, FALSE);
	switch( nSBCode)
	{
		case SB_LINEDOWN:
			if(( EUDC_SView + NUM_CHAR) <= EUDC_EView){
				MoveScr =  0 - (CharSize.cy + LINEWIDTH);
				EUDC_SView += NUM_CHAR;
				EUDC_SView  = CorrectEUDCCode(EUDC_SView,TRUE);
				ScrlBarPos = (short)GetBarPosition( EUDC_SView);
			}
			break;

		case SB_LINEUP:
			if(( EUDC_SView - NUM_CHAR) >= ( EUDC_SCode & 0xfff0)){
				MoveScr = CharSize.cy + LINEWIDTH;
				EUDC_SView -= NUM_CHAR;
				EUDC_SView = CorrectEUDCCode(EUDC_SView,FALSE);
				ScrlBarPos = (short)GetBarPosition( EUDC_SView);
			}
			break;

		case SB_PAGEDOWN:
			if(( EUDC_SView + NUM_CHAR*NUM_LINE) <= EUDC_EView){
				MoveScr = ( 0-(CharSize.cy+LINEWIDTH))*NUM_LINE;
				ScrlBarPos = (short)GetBarPosition( EUDC_SView);
				ScrlBarPos += NUM_LINE;
				EUDC_SView = GetCodeScrPos( ScrlBarPos);
			}else{
				MoveScr = ( 0-(CharSize.cy+LINEWIDTH))*NUM_LINE;
				EUDC_SView = EUDC_EView;
				ScrlBarPos = (short)GetBarPosition( EUDC_SView);	
			}
			break;

		case SB_PAGEUP:
			if(( EUDC_SView - NUM_CHAR*NUM_LINE)
				>= ( EUDC_SCode & 0xfff0)){
				MoveScr = (CharSize.cy + LINEWIDTH)*NUM_LINE;
				ScrlBarPos = (short)GetBarPosition( EUDC_SView);
				ScrlBarPos -= NUM_LINE;
				EUDC_SView = GetCodeScrPos( ScrlBarPos);
			}else{
				MoveScr = (CharSize.cy + LINEWIDTH)*NUM_LINE;
				EUDC_SView = (EUDC_SCode & 0xfff0);
				ScrlBarPos = (short)GetBarPosition( EUDC_SView);
			}
			break;

		case SB_TOP:
			EUDC_SView = ( EUDC_SCode & 0xfff0);
			ScrlBarPos = 0;
			break;

		case SB_BOTTOM:
			EUDC_SView = EUDC_EView;
			ScrlBarPos = (short)BarRange;
			break;

		case SB_THUMBPOSITION:
			ThumbPosFlag = TRUE;
			ScrlBarPos = (short)nPos;
			EUDC_SView = GetCodeScrPos( nPos);
			break;

		case SB_THUMBTRACK:
			ThumbTrkFlag = TRUE;
			break;

		default:
			break;
	}

	if( abs( MoveScr) < (CharSize.cy + LINEWIDTH)*NUM_LINE &&
		abs( MoveScr) > 0 && !ThumbTrkFlag){
		CRect	ScrllRect;
		CRect	ClintRect;

		GetClientRect( &ClintRect);
		ScrllRect.CopyRect( &ClintRect);
		if( MoveScr < 0){
			ClintRect.top = 0 - MoveScr;
			this->ScrollWindow( 0, MoveScr, &ClintRect, NULL);
			ScrllRect.top = ( 0 - MoveScr)*(NUM_LINE-1);			
		}else{
			ClintRect.top = LINEWIDTH;
			ClintRect.bottom = MoveScr*(NUM_LINE - 1);
			this->ScrollWindow( 0, MoveScr, &ClintRect, NULL);
			ScrllRect.top = 0;
			ScrllRect.bottom = ScrllRect.top + MoveScr;
		}
		this->InvalidateRect( &ScrllRect, FALSE);
	}else if(!ThumbTrkFlag && ( MoveScr || ThumbPosFlag)){
		this->Invalidate( TRUE);
	}
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_LBUTTONDOWN"	*/
/*					*/
/****************************************/
void
CEudcList::OnLButtonDown(
UINT 	nFlags,
CPoint 	point)
{
	CRect	test;
	BOOL	PtIn;
unsigned int	i, j;

	PtIn = FALSE;
	this->SetFocus();
	for( i = 0; i < NUM_LINE; i++){
		for( j = 0; j < NUM_CHAR; j++){
			if( rcEditChar[i][j].PtInRect( point)){
				if( IsCorrectChar( i, j))
					PtIn = TRUE;
				break;
			}
		}
	}
	if( !PtIn){
		MessageBeep((UINT)-1);
		return;
	}

	LButtonPt = point;
	SearchSelectPosition();
	SelectCode = 0;


	for( i = 0; i < NUM_LINE; i++){
		for( j = 0; j < NUM_CHAR; j++){
			if( rcEditChar[i][j].PtInRect( LButtonPt)){

				test.SetRect( rcEditChar[i][j].left - 2,
						rcEditChar[i][j].top - 2,
						rcEditChar[i][j].right + 2,
						rcEditChar[i][j].bottom + 2);

				this->InvalidateRect( &test, FALSE);

				break;
			}
		}
	}


	this->UpdateWindow();

}

/****************************************/
/*					*/
/*	MESSAGE	"WM_LBUTTONDBLCLK"	*/
/*					*/
/****************************************/
void
CEudcList::OnLButtonDblClk(
UINT 	nFlags,
CPoint 	point)
{
	BOOL 	PtIn;
unsigned int	i, j;

	LButtonPt = point;
	this->Invalidate( FALSE);
	this->UpdateWindow();

	PtIn = FALSE;
	this->SetFocus();
	for( i = 0; i < NUM_LINE; i++){
		for( j = 0; j < NUM_CHAR; j++){
			if( rcEditChar[i][j].PtInRect( point)){
				if( IsCorrectChar( i, j))
					PtIn = TRUE;
				break;
			}
		}
	}
	if( !PtIn){
		MessageBeep((UINT)-1);
		return;
	}else{
		GetParent()->PostMessage( WM_COMMAND, IDOK, 0L);
	}
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_SETCURSOR"		*/
/*					*/
/****************************************/
BOOL
CEudcList::OnSetCursor(
CWnd* 	pWnd,
UINT 	nHitTest,
UINT 	message)
{
	::SetCursor( AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return TRUE;
}

/****************************************/
/*					*/
/*	Draw ConcaveRectangle		*/
/*					*/
/****************************************/
void
CEudcList::DrawConcave(
CDC 	*dc,
CRect 	rect,
BOOL 	PtIn)
{
	CBrush	ConBrush, *OldBrush;
	CRect	Rt, Rf;

	Rt.SetRect( rect.left-1, rect.top-1, rect.right+1, rect.bottom+1);
	Rf.CopyRect( &rect);

	if( !PtIn){
		ConBrush.CreateSolidBrush( COLOR_HLIGHT);
		OldBrush = dc->SelectObject( &ConBrush);
		dc->PatBlt( Rt.left, Rt.top, Rt.Width(), 1, PATCOPY);
		dc->PatBlt( Rt.left, Rt.top, 1, Rt.Height(), PATCOPY);
		dc->SelectObject( OldBrush);
		ConBrush.DeleteObject();

		ConBrush.CreateSolidBrush( COLOR_SHADOW);
		OldBrush = dc->SelectObject( &ConBrush);
		dc->PatBlt( Rt.left, Rt.bottom, Rt.Width(), 1, PATCOPY);
		dc->PatBlt( Rt.right, Rt.top, 1, Rt.Height()+1, PATCOPY);
		dc->SelectObject( OldBrush);
		ConBrush.DeleteObject();

		if( FocusFlag){
			CBrush	fBrush;
			CPen	fPen, *OldPen;

			fBrush.CreateStockObject( NULL_BRUSH);
			fPen.CreatePen( PS_SOLID, 1, COLOR_FACE);
			OldBrush = dc->SelectObject( &fBrush);
			OldPen   = dc->SelectObject( &fPen);
			dc->Rectangle( &Rf);
			dc->SelectObject( OldBrush);
			dc->SelectObject( OldPen);
			fBrush.DeleteObject();
			fPen.DeleteObject();
		}
	}else{
		ConBrush.CreateSolidBrush( COLOR_SHADOW);
		OldBrush = dc->SelectObject( &ConBrush);
	        dc->PatBlt( Rt.left, Rt.top, Rt.Width(), 1, PATCOPY);
		dc->PatBlt( Rt.left, Rt.top, 1, Rt.Height(), PATCOPY);
		dc->SelectObject( OldBrush);
		ConBrush.DeleteObject();

		ConBrush.CreateSolidBrush( COLOR_HLIGHT);
		OldBrush = dc->SelectObject( &ConBrush);
		dc->PatBlt( Rt.left, Rt.bottom, Rt.Width(), 1, PATCOPY);
		dc->PatBlt( Rt.right, Rt.top, 1, Rt.Height()+1, PATCOPY);
		dc->SelectObject( OldBrush);
		ConBrush.DeleteObject();

		if( FocusFlag){
			CBrush	fBrush;
			CPen	fPen, *OldPen;

			fBrush.CreateStockObject( NULL_BRUSH);
			fPen.CreatePen( PS_SOLID, 1, COLOR_SHADOW);
			OldBrush = dc->SelectObject( &fBrush);
			OldPen   = dc->SelectObject( &fPen);
			dc->Rectangle( &Rf);
			dc->SelectObject( OldBrush);
			dc->SelectObject( OldPen);
			fBrush.DeleteObject();
			fPen.DeleteObject();
		}
	}
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_KEYDOWN"		*/
/*					*/
/****************************************/
void
CEudcList::OnKeyDown(
UINT 	nChar,
UINT	nRepCnt,
UINT 	nFlags)
{
	int	sPos;
	int	ePos;
#ifdef BUILD_ON_WINNT
    int i;
	WORD	TmpCode;
#endif // BUILD_ON_WINNT

	if( nChar == VK_DOWN  || nChar == VK_UP  ||
	    nChar == VK_RIGHT || nChar == VK_LEFT){
		sPos = GetBarPosition( EUDC_SView);
		ePos = GetBarPosition( SelectCode);
		if( ePos - sPos >= NUM_LINE || ePos < sPos){
			EUDC_SView = SelectCode & 0xfff0;
			ScrlBarPos = (short)GetBarPosition( EUDC_SView);
			this->Invalidate(FALSE);
			this->UpdateWindow();
		}

		switch( nChar){

		case VK_DOWN:
			if( SelectCode + NUM_CHAR > EUDC_ECode)
				break;
#ifdef BUILD_ON_WINNT
			TmpCode = SelectCode + NUM_CHAR;
			TmpCode = CorrectEUDCCodeKey( TmpCode, NUM_CHAR, TRUE);
			sPos = GetBarPosition( EUDC_SView);
			ePos = GetBarPosition( TmpCode);
			for (i=0;i <= ePos - sPos - NUM_LINE; i++){
#else
			if( ePos - sPos >= NUM_LINE - 1){
#endif // BUILD_ON_WINNT
				this->SendMessage( WM_VSCROLL, SB_LINEDOWN, 0);
			}
			SearchSelectPosition();
			SelectCode += NUM_CHAR;
			SelectCode = CorrectEUDCCodeKey( SelectCode,
							NUM_CHAR, TRUE);
			SearchSelectPosition();
			this->UpdateWindow();
			break;

		case VK_UP:
			if( SelectCode - NUM_CHAR < EUDC_SCode)
				break;
#ifdef BUILD_ON_WINNT
			TmpCode = SelectCode - NUM_CHAR;
			TmpCode = CorrectEUDCCodeKey( TmpCode, NUM_CHAR, FALSE);
			sPos = GetBarPosition( EUDC_SView);
			ePos = GetBarPosition( TmpCode);
			for ( i=0; i < sPos - ePos; i++){
#else
			if( SelectCode - NUM_CHAR < EUDC_SView){
#endif // BUILD_ON_WINNT
				this->SendMessage( WM_VSCROLL, SB_LINEUP, 0);
			}

			SearchSelectPosition();
			SelectCode -= NUM_CHAR;
			SelectCode = CorrectEUDCCodeKey( SelectCode,
							NUM_CHAR, FALSE);
			SearchSelectPosition();
			this->UpdateWindow();
			break;

		case VK_LEFT:
			if( SelectCode - 1 < EUDC_SCode)
				break;
#ifdef BUILD_ON_WINNT
            TmpCode = SelectCode - 1;
            TmpCode = CorrectEUDCCodeKey( TmpCode, 1, FALSE);
            sPos = GetBarPosition( EUDC_SView);
            ePos = GetBarPosition( TmpCode);
            if( ePos < sPos){
#else
			if( SelectCode - 1 < EUDC_SView){
#endif // BUILD_ON_WINNT
                this->SendMessage( WM_VSCROLL, SB_LINEUP, 0);
            }
			SearchSelectPosition();
			SelectCode--;
			SelectCode = CorrectEUDCCodeKey( SelectCode, 1, FALSE);
			SearchSelectPosition();
			this->UpdateWindow();
			break;

		case VK_RIGHT:
#ifdef BUILD_ON_WINNT
            // Move to above...
#else
			WORD	TmpCode;
#endif // BUILD_ON_WINNT

			if( SelectCode + 1 > EUDC_ECode)
				break;
			TmpCode = SelectCode + 1;
			TmpCode = CorrectEUDCCodeKey( TmpCode, 1, TRUE);
			sPos = GetBarPosition( EUDC_SView);
			ePos = GetBarPosition( TmpCode);
			if( ePos - sPos >= NUM_LINE){
				this->SendMessage( WM_VSCROLL, SB_LINEDOWN, 0);
			}
			SearchSelectPosition();
			SelectCode++;
			SelectCode = CorrectEUDCCodeKey( SelectCode, 1, TRUE);
			SearchSelectPosition();
			this->UpdateWindow();
			break;
		}
	}else 	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_SETFOCUS"		*/
/*					*/
/****************************************/
void
CEudcList::OnSetFocus(
CWnd* 	pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);
	FocusFlag = TRUE;
	SearchSelectPosition();
	this->UpdateWindow();
	::HideCaret( NULL);
//	DestroyCaret();
}

/****************************************/
/*					*/
/*	Search select posistion		*/
/*					*/
/****************************************/
void
CEudcList::SearchSelectPosition()
{

	CRect	test;
	int	sViewPt, sCodePt;
unsigned int	i, j;


	sViewPt = GetBarPosition( EUDC_SView);
	sCodePt = GetBarPosition( SelectCode);

#ifdef BUILD_ON_WINNT
    // the new view does not contain previsouly selected char,
    // we don't need to redraw the concave.
    if (sCodePt < sViewPt || sCodePt > sViewPt+NUM_LINE)
        return;
#endif



	i = (unsigned int)(sCodePt - sViewPt);
	j = (unsigned int)(SelectCode & 0x000f);
	test.SetRect( rcEditChar[i][j].left - 2,
			rcEditChar[i][j].top - 2,
			rcEditChar[i][j].right + 2,
			rcEditChar[i][j].bottom + 2);
	this->InvalidateRect( &test, FALSE);

}

/****************************************/
/*					*/
/*	Correct it if code is illegal 	*/
/*					*/
/****************************************/
#ifdef BUILD_ON_WINNT
WORD
CEudcList::CorrectEUDCCodeKey(
WORD 	Code,
int 	MovePt,
BOOL 	UporDown)
{
COUNTRYINFO	Info;
	WORD	wCode;
	BYTE 	LByte, HByte, Tmp0;
	int	TralPos = 0;

	LByte = LOBYTE( Code);
	HByte = HIBYTE( Code);
	Info = CountryInfo;
	for( int i = 0; i < Info.nTralByte; i++){
		if( LByte >= Info.sTralByte[i] && LByte <= Info.eTralByte[i]){
		    	goto RET;
		}else if( LByte < Info.sTralByte[i]){
        /* decide which range of starting trailbyte we are less than */
			TralPos = i;
			break;
        }else	TralPos = i+1;
	}
	if( UporDown){  //code increasing
		if( TralPos == Info.nTralByte){ //greater than last eTrailByte
		    HByte += 0x1; //same as less than next sTraiByte
            TralPos=0;
        }

		if( MovePt < NUM_CHAR){  //
			LByte = Info.sTralByte[TralPos];
		}else{
            Tmp0 = LByte & 0x0f;
            LByte = Info.sTralByte[TralPos] & 0xf0;
            LByte |= Tmp0;
            if (LByte < Info.sTralByte[TralPos])
                LByte += NUM_CHAR;

		}
	}else{
		if( TralPos == 0){ //greater than last eTrailByte
            TralPos = Info.nTralByte;
		    HByte -= 0x1;
        }

		if( MovePt < NUM_CHAR){
			LByte = Info.eTralByte[TralPos-1];
	    }else{
            Tmp0 = LByte & 0x0f;
            LByte = Info.eTralByte[TralPos-1] & 0xf0;
            LByte |= Tmp0;
            if (LByte > Info.eTralByte[TralPos-1])
                LByte -= NUM_CHAR;
		}
	}
RET:
	wCode = MAKEWORD( LByte, HByte);
	return wCode;
}
#else
WORD
CEudcList::CorrectEUDCCodeKey(
WORD 	Code,
int 	MovePt,
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
		if( LByte >= Info.sTralByte[i] && LByte <= Info.eTralByte[i]){
		    	goto RET;
		}else if( LByte < Info.sTralByte[i]){
			TralPos = i;
			break;
		}else	TralPos = i+1;
	}
	if( UporDown)
		if( TralPos == Info.nTralByte){
			if( MovePt < NUM_CHAR){
				LByte = Info.sTralByte[0];
				HByte += 0x1;
			}else{
				LByte = Info.eTralByte[Info.nTralByte - 1];
			}
		}else if( MovePt < NUM_CHAR){
			LByte = Info.sTralByte[TralPos];
		}else{
			if( TralPos){
				BYTE	Tmp1;
				BYTE	Tmp2;

				Tmp1 = Info.eTralByte[TralPos-1] & 0xf0;
				Tmp2 = LByte & 0xf0;
				if( Tmp1 == Tmp2){
					LByte = Info.eTralByte[TralPos-1];
				}else{
					Tmp1 = LByte & 0x0f;
					LByte = Info.sTralByte[TralPos]	& 0xf0;
					LByte |= Tmp1;
				}
			}else{
				LByte += (Info.sTralByte[0] & 0xf0);
			}
		}
	else{
		if( !TralPos){
			if( MovePt < NUM_CHAR){
				LByte = Info.eTralByte[Info.nTralByte - 1];
				HByte -= 0x1;
			}else{
				BYTE	Tmp;

				Tmp = LByte & 0x0f;
				LByte = Info.eTralByte[Info.nTralByte - 1]&0xf0;
				LByte |= Tmp;
				HByte -= 0x1;
				if( LByte > Info.eTralByte[Info.nTralByte-1]){
					LByte = Info.eTralByte[Info.nTralByte-1];
				}
			}
		}else{
			BYTE	Tmp;

			Tmp = LByte & 0x0f;
			LByte = Info.eTralByte[TralPos-1] & 0xf0;
			LByte |= Tmp;
			if( LByte > Info.eTralByte[TralPos-1])
				LByte = Info.eTralByte[TralPos-1];

		}
	}
RET:
	wCode = MAKEWORD( LByte, HByte);
	return wCode;
}
#endif // BUILD_ON_WINNT

/****************************************/
/*					*/
/*	Whether correct or not?		*/
/*					*/
/****************************************/
BOOL
CEudcList::IsCorrectChar(
UINT 	i,
UINT 	j)
{
	int	sViewPt;
	WORD	wCode;
	BYTE	sOffset;
	BOOL	flg;

	flg = FALSE;
	BarRange = GetBarPosition((WORD)EUDC_EView);
	sViewPt = GetBarPosition( EUDC_SView);
	wCode = GetCodeScrPos( sViewPt + i);
	wCode |= j;

	sOffset = LOBYTE( wCode);
	for( int k = 0; k < CountryInfo.nTralByte; k++){
		if( sOffset >= CountryInfo.sTralByte[k] &&
		    sOffset <= CountryInfo.eTralByte[k]){
		    	flg = TRUE;
		}
	}
	return flg;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_KILLFOCUS"		*/
/*					*/
/****************************************/
void
CEudcList::OnKillFocus(
CWnd* 	pNewWnd)
{
	FocusFlag = FALSE;
	SearchSelectPosition();
	this->UpdateWindow();
	CEdit::OnKillFocus( pNewWnd);
	::HideCaret( NULL);
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_RBUTTONUP"		*/
/*					*/
/****************************************/
void
CEudcList::OnRButtonUp(
UINT 	nFlags,
CPoint 	point)
{
	GetParent()->SendMessage( WM_CONTEXTMENU, (WPARAM)this->GetSafeHwnd(), 0);
}

BEGIN_MESSAGE_MAP( CViewEdit, CEdit)
	//{{AFX_MSG_MAP( CViewEdit)
	ON_WM_PAINT()
	ON_WM_SETCURSOR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CViewEdit::CViewEdit()
{
}

/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CViewEdit::~CViewEdit()
{
//fix for 261529  
  EUDCFont.DeleteObject();
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CViewEdit::OnPaint()
{
	LOGFONT	LogFont;
	CFont	ViewFont, *OldFont;
	CSize	cSize;
	CRect	ViewRect;
	int	xOffset, yOffset;
	CPaintDC	dc( this);


	GetClientRect( &ViewRect);
	DrawConcave( &dc, ViewRect);

    if(bHasGlyph == FALSE) return;

	memset( &LogFont, 0, sizeof( LogFont));
//fix for 261529    
  EUDCFont.GetLogFont(&LogFont);

	if( ViewRect.Width() >= ViewRect.Height())
		LogFont.lfHeight = ViewRect.Height() - 10;
	else	LogFont.lfHeight = ViewRect.Width()  - 10;

  if( !ViewFont.CreateFontIndirect( &LogFont))
		return;
	OldFont = dc.SelectObject( &ViewFont);

#ifdef BUILD_ON_WINNT
    if (CountryInfo.bUnicodeMode)
	    GetTextExtentPoint32W(dc.GetSafeHdc(), (LPCWSTR)ViewCode, 1, &cSize);
    else
#endif //BUILD_ON_WINNT
	    GetTextExtentPoint32A(dc.GetSafeHdc(), (LPCSTR)ViewCode, 2, &cSize);

	if( ViewRect.Width() > cSize.cx){
		xOffset = ViewRect.Width() - cSize.cx;
		xOffset /= 2;
	}else	xOffset = 0;

	if( ViewRect.Height() > cSize.cy){
		yOffset = ViewRect.Height() - cSize.cy;
		yOffset /= 2;
	}else	yOffset = 0;

	dc.SetBkColor( COLOR_FACE);
	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); // COLOR_BLACK);

    if (CountryInfo.bUnicodeMode)
	{
#ifdef UNICODE
	    ExtTextOutW(dc.GetSafeHdc(), xOffset, yOffset, ETO_OPAQUE,
		    &ViewRect, (LPCWSTR)ViewCode, 1, NULL);
#else
		CHAR ViewTmp[2];
		WideCharToMultiByte(CP_ACP, 0, (LPWSTR) ViewCode, 1, ViewTmp, 2, NULL, NULL);
		ExtTextOutA(dc.GetSafeHdc(), xOffset, yOffset, ETO_OPAQUE,
						&ViewRect, (LPCSTR)ViewTmp, 2, NULL);
#endif

	}
    else
	{
		ExtTextOutA(dc.GetSafeHdc(), xOffset, yOffset, ETO_OPAQUE,
			&ViewRect, (LPCSTR)ViewCode, 2, NULL);
	}
	dc.SelectObject( OldFont);
	ViewFont.DeleteObject();
}

/****************************************/
/*					*/
/*	Draw Concave Rect		*/
/*					*/
/****************************************/
void
CViewEdit::DrawConcave(
CDC 	*dc,
CRect 	rect)
{
	CBrush	ConBrush, *OldBrush;
	CRect	Rt;

	Rt.SetRect( rect.left-1, rect.top-1, rect.right, rect.bottom);

 	ConBrush.CreateSolidBrush(COLOR_FACE);
	OldBrush = dc->SelectObject( &ConBrush);
	dc->PatBlt( Rt.left, Rt.top, Rt.right, Rt.bottom, PATCOPY);
	dc->SelectObject( OldBrush);
	ConBrush.DeleteObject();

	ConBrush.CreateSolidBrush( COLOR_HLIGHT);
	OldBrush = dc->SelectObject( &ConBrush);
	dc->PatBlt( Rt.left, Rt.top, Rt.Width(), 1, PATCOPY);
	dc->PatBlt( Rt.left, Rt.top, 1, Rt.Height(), PATCOPY);
	dc->SelectObject( OldBrush);
	ConBrush.DeleteObject();

	ConBrush.CreateSolidBrush( COLOR_SHADOW);
	OldBrush = dc->SelectObject( &ConBrush);
	dc->PatBlt( Rt.left, Rt.bottom, Rt.Width(), 1, PATCOPY);
	dc->PatBlt( Rt.right, Rt.top, 1, Rt.Height()+1, PATCOPY);
	dc->SelectObject( OldBrush);
	ConBrush.DeleteObject();
}

BEGIN_MESSAGE_MAP( CCustomListFrame, CStatic)
	//{{AFX_MSG_MAP( CCustomListFrame)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CCustomListFrame::CCustomListFrame()
{
}

/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CCustomListFrame::~CCustomListFrame()
{
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CCustomListFrame::OnPaint()
{
	CRect	FrameRect;
	CPaintDC	dc( this);

	this->GetClientRect( &FrameRect);
	this->DrawConcave( &dc, FrameRect);
}

/****************************************/
/*					*/
/*	Draw Concave Rect		*/
/*					*/
/****************************************/
void
CCustomListFrame::DrawConcave(
CDC 	*dc,
CRect 	rect)
{
	CBrush	ConBrush, *OldBrush;
	CRect	Rt;

	Rt.SetRect( rect.left-1, rect.top-1, rect.right, rect.bottom);

	ConBrush.CreateSolidBrush( COLOR_HLIGHT);
	OldBrush = dc->SelectObject( &ConBrush);
	dc->PatBlt( Rt.left, Rt.top, Rt.Width(), 1, PATCOPY);
	dc->PatBlt( Rt.left, Rt.top, 1, Rt.Height(), PATCOPY);
	dc->SelectObject( OldBrush);
	ConBrush.DeleteObject();

	ConBrush.CreateSolidBrush( COLOR_SHADOW);
	OldBrush = dc->SelectObject( &ConBrush);
	dc->PatBlt( Rt.left, Rt.bottom, Rt.Width(), 1, PATCOPY);
	dc->PatBlt( Rt.right, Rt.top, 1, Rt.Height()+1, PATCOPY);
	dc->SelectObject( OldBrush);
	ConBrush.DeleteObject();
}

BEGIN_MESSAGE_MAP( CCustomInfoFrame, CStatic)
	//{{AFX_MSG_MAP( CCustomInfoFrame)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CCustomInfoFrame::CCustomInfoFrame()
{
}

/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CCustomInfoFrame::~CCustomInfoFrame()
{
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CCustomInfoFrame::OnPaint()
{
	CRect	FrameRect;
	CPaintDC	dc( this);

	this->GetClientRect( &FrameRect);
	this->DrawConcave( &dc, FrameRect);
}

/****************************************/
/*					*/
/*	Draw Concave Rect		*/
/*					*/
/****************************************/
void
CCustomInfoFrame::DrawConcave(
CDC 	*dc,
CRect 	rect)
{
	CBrush	ConBrush, *OldBrush;
	CRect	Rt;

	Rt.SetRect( rect.left-1, rect.top-1, rect.right, rect.bottom);

	ConBrush.CreateSolidBrush( COLOR_SHADOW);
	OldBrush = dc->SelectObject( &ConBrush);
	dc->PatBlt( Rt.left, Rt.top, Rt.Width(), 1, PATCOPY);
	dc->PatBlt( Rt.left, Rt.top, 1, Rt.Height(), PATCOPY);
	dc->SelectObject( OldBrush);
	ConBrush.DeleteObject();

	ConBrush.CreateSolidBrush( COLOR_HLIGHT);
	OldBrush = dc->SelectObject( &ConBrush);
	dc->PatBlt( Rt.left, Rt.bottom, Rt.Width(), 1, PATCOPY);
	dc->PatBlt( Rt.right, Rt.top, 1, Rt.Height()+1, PATCOPY);
	dc->SelectObject( OldBrush);
	ConBrush.DeleteObject();
}

BOOL CViewEdit::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor( AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return TRUE;
}

void CViewEdit::OnSetFocus(CWnd* pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);
	::HideCaret( NULL);
}

void CViewEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	::HideCaret( NULL);
}

void CViewEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
}

void CViewEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
}

void CViewEdit::OnRButtonUp(UINT nFlags, CPoint point)
{
	GetParent()->SendMessage( WM_CONTEXTMENU, (WPARAM)this->GetSafeHwnd(), 0);
}

#ifdef BUILD_ON_WINNT

BEGIN_MESSAGE_MAP( CColumnHeading, CWnd)
    //{{AFX_MSG_MAP( CColumnHeading)
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CColumnHeading::CColumnHeading()
{
}

/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CColumnHeading::~CColumnHeading()
{
    SysFFont.DeleteObject();
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CColumnHeading::OnPaint()
{
    // column heading support
	CPaintDC	dc( this);
    TCHAR ColumnHead[2];
    CSize cSize;
    int	j,xOffset;

    dc.SetMapMode( MM_TEXT);
    dc.SelectObject( &SysFFont);
    dc.SetBkColor( COLOR_FACE);
    dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); // COLOR_BLACK);
    for( j = 0; j < NUM_CHAR; j++){
        wsprintf(ColumnHead, TEXT("%X"), j);
	    GetTextExtentPoint32(dc.GetSafeHdc(),
                ColumnHead, 1, &cSize);
        if( rcColumnHead[j].Width() > cSize.cx){
		    xOffset = rcColumnHead[j].Width() - cSize.cx;
	        xOffset /= 2;
	    }else	
            xOffset= 0;

        dc.ExtTextOut( rcColumnHead[j].left + xOffset,
             rcColumnHead[j].top,
             ETO_OPAQUE, &rcColumnHead[j],
             ColumnHead, 1, NULL);
    }
}


#endif //BUILD_ON_WINNT
