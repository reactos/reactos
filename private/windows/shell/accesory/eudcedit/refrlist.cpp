/**************************************************/
/*						                          */
/*						                          */
/*	Character List( Referrence Dialog)	          */
/*						                          */
/*						                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"refrlist.h"
#include    "util.h"

//	6 * 16 Matrics
#define		NUM_CHAR	16
#define		NUM_LINE	6

//	Type of Character Code
#define		CHAR_INIT	0	// Initial value
#define		CHAR_SBCS	1	// SBCS
#define		CHAR_DBCS1	2	// DBCS1
#define		CHAR_DBCS2	3	// DBCS2
#define		CHAR_EUDC	4	// EUDC
#define		CHAR_ETC	5	// Other

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE( CRefrList, CEdit)
BEGIN_MESSAGE_MAP( CRefrList, CEdit)
	//{{AFX_MSG_MAP( CRefrList)
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

//	Range of character code( high byte)
static WORD HiByteRange[][10] =
{
//    JPN             CHT             KRW             CHS             OTHER
	{ 0x00a0, 0xfca0, 0x00a0, 0xfea0, 0x00a0, 0xfea0, 0x00a0, 0xfea0, 0x00a0, 0x00a0}, // CHAR_INIT
	{ 0x0020, 0x00ff, 0x0020, 0x00ff, 0x0020, 0x00ff, 0x0020, 0x00ff, 0x0020, 0x00ff}, // CHAR_SBCS
	{ 0x8140, 0x9fff, 0x8140, 0xa0ff, 0x8140, 0x8fff, 0x8140, 0xafff, 0x0020, 0x00ff}, // CHAR_DBCS1
	{ 0xe040, 0xeaff, 0xa140, 0xf9ff, 0x9040, 0x9fff, 0xb040, 0xf7ff, 0x0020, 0x00ff}, // CHAR_DBCS2
	{ 0xf040, 0xfcff, 0xfa40, 0xfeff, 0xa040, 0xfeff, 0xf840, 0xfeff, 0x0020, 0x00ff}, // CHAR_EUDC
};

//	Range of character code( low byte)
static WORD LoByteRange[][10] =
{
	{ 0x0020, 0x00ff, 0x0020, 0x00ff, 0x0020, 0x00ff, 0x0020, 0x00ff, 0x0020, 0x00ff },
	{ 0x0040, 0x007e, 0x0040, 0x007e, 0x0041, 0x005a, 0x0040, 0x007e, 0x0040, 0x007e },
	{ 0x0080, 0x00fc, 0x0080, 0x00fe, 0x0061, 0x007a, 0x0080, 0x00fe, 0x0040, 0x007e },
	{ 0x0080, 0x00fc, 0x0080, 0x00fe, 0x0081, 0x00fe, 0x0080, 0x00fe, 0x0040, 0x007e },
	{ 0x0000, 0x003f, 0x0000, 0x003f, 0x0000, 0x003f, 0x0000, 0x003f, 0x0040, 0x007e },
};

extern LOGFONT	ReffLogFont;
extern LOGFONT	EditLogFont;
extern BOOL	TitleFlag;
extern BOOL	CodeFocus;
extern BOOL	CharFocus;
static CRect	rcReferChar[NUM_LINE][NUM_CHAR];
static CRect	rcReferCode[NUM_LINE];

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CRefrList::CRefrList()
{
/*
//	Initialize static parameter
	if( CountryInfo.LangID == EUDC_CHT)
		CHN = 2;
	else if( CountryInfo.LangID == EUDC_JPN)
		CHN = 0;
	else if( CountryInfo.LangID == EUDC_KRW)
		CHN = 4;
	else if( CountryInfo.LangID == EUDC_CHS)
		CHN = 6;
	else 	CHN = 8;
*/
	FocusFlag = FALSE;
}
								
/****************************************/
/*					*/
/*	Destructor( Virtual)		*/
/*					*/
/****************************************/
CRefrList::~CRefrList()
{
    SysFFont.DeleteObject();
    CharFont.DeleteObject();
		ViewFont.DeleteObject();
}
			      		
/****************************************/
/*					*/
/*	Set intitial code range		*/
/*					*/
/****************************************/
void
CRefrList::SetCodeRange()
{
    CHARSETINFO CharsetInfo;
    BYTE CharSet;

		SelectCode = 0x0020;
		ScrlBarPos = 0;
		StartCode  = 0x0020;
		EndCode    = 0xffff;
		ViewStart  = 0x0020;
		ViewEnd    = 0xffa0;
/*
	SelectCode = HiByteRange[CHAR_SBCS][CHN];
	ScrlBarPos = 0;
	StartCode  = HiByteRange[CHAR_SBCS][CHN];
	ViewStart  = HiByteRange[CHAR_SBCS][CHN];
	if( !TitleFlag){
		if( rLogFont.lfCharSet == CountryInfo.CharacterSet){
//			Correspond to DBCS		    	
			ViewEnd = HiByteRange[CHAR_INIT][CHN+1];
			EndCode	= HiByteRange[CHAR_EUDC][CHN+1];
		}else{
//			Correspond to SBCS
			ViewEnd = HiByteRange[CHAR_INIT][CHN];
			EndCode	= HiByteRange[CHAR_SBCS][CHN+1];
		}
        CharSet = rLogFont.lfCharSet;

	}else{
		if( cLogFont.lfCharSet == CountryInfo.CharacterSet){
			ViewEnd = HiByteRange[CHAR_INIT][CHN+1];
			EndCode	= HiByteRange[CHAR_EUDC][CHN+1];
		}else{
			ViewEnd = HiByteRange[CHAR_INIT][CHN];
			EndCode	= HiByteRange[CHAR_SBCS][CHN+1];
		}
        CharSet = cLogFont.lfCharSet;
	}

    if(CharSet == SYMBOL_CHARSET)
    {
        dwCodePage = 1252;
    }
    else if(TranslateCharsetInfo((DWORD *)CharSet,&CharsetInfo,TCI_SRCCHARSET))
    {
        dwCodePage = CharsetInfo.ciACP;
    }
    else
    {
        dwCodePage = CP_ACP;
    }
*/
}

/****************************************/
/*					*/
/*	Check type of character code	*/
/*					*/
/****************************************/
int
CRefrList::CheckCharType(
WORD 	Code)
{
	/*
	if( !( Code & 0xff00))
		return	CHAR_SBCS;
	else if(( Code >= HiByteRange[CHAR_DBCS1][CHN]) &&
	        ( Code <= HiByteRange[CHAR_DBCS1][CHN+1]))
		return  CHAR_DBCS1;
	else if(( Code >= HiByteRange[CHAR_DBCS2][CHN]) &&
		( Code <= HiByteRange[CHAR_DBCS2][CHN+1]))
	 	return  CHAR_DBCS2;
	else if(( Code >= HiByteRange[CHAR_EUDC][CHN])  &&
		( Code <= HiByteRange[CHAR_EUDC][CHN+1]))
		return  CHAR_EUDC;
	else	return  CHAR_ETC;
	*/
	return 0;
}

/****************************************/
/*					*/
/*	Increase character code		*/
/*					*/
/****************************************/
WORD
CRefrList::GetPlusCode(
WORD 	Code,
int 	ScrollNum)
{
	WORD	PrevCode;
	WORD 	LowByte, HighByte;
	int	CharType;
	int	CharType1, CharType2;
	int	Offset;

	Code += (WORD)ScrollNum;
/*
	PrevCode = (WORD)(Code - (WORD)ScrollNum);
	CharType1 = CheckCharType( PrevCode);
	CharType2 = CheckCharType( Code);
	if( CharType1 != CharType2){
		if( CharType1 == CHAR_EUDC)
			Code = PrevCode;		
		else{
			Offset = Code - HiByteRange[CharType1][CHN+1];
			Code = (WORD)( HiByteRange[CharType1+1][CHN] +Offset-1);
		}
	}
	CharType = CheckCharType( Code);
	if( CharType != CHAR_SBCS){
		LowByte  = Code & 0x00ff;
		HighByte = Code & 0xff00;
		if( LowByte <= LoByteRange[4][CHN+1] &&
		    LowByte >= LoByteRange[4][CHN] ){
#if 0
			if( CountryInfo.LangID == EUDC_CHS)
				LowByte = 0x00a0;
			else	LowByte = 0x0040;
#endif
			LowByte = 0x0040;
		}
		Code = ( HighByte | LowByte);
	}
*/
	return Code;
}
					
/****************************************/
/*					*/
/*	Decrease Character Code		*/
/*					*/
/****************************************/
WORD
CRefrList::GetMinusCode(
WORD 	Code,
int 	ScrollNum)
{
	WORD	PrevCode;
	int	CharType;
	int	CharType1, CharType2;
	int	Offset;

	Code -= (WORD)ScrollNum;
/*
	PrevCode = (WORD)( Code + (WORD)ScrollNum);
	CharType1 = CheckCharType( Code);
	CharType2 = CheckCharType( PrevCode);
	if( CharType1 != CharType2){
	 	if( CharType2 == CHAR_SBCS)
			return (WORD)HiByteRange[CHAR_SBCS][CHN];
		else{
			Offset = HiByteRange[CharType2][CHN] - Code;
			return (WORD)(HiByteRange[CharType2-1][CHN+1]-Offset+1);
		}	
	}
	CharType = CheckCharType( Code);
	if( CharType != CHAR_SBCS){
		WORD 	LowByte;
		WORD	HighByte;
		WORD	Tmp;

		LowByte  = Code & 0x00ff;
		HighByte = Code & 0xff00;
		if( LowByte <= LoByteRange[4][CHN+1] &&
		    LowByte >= LoByteRange[4][CHN] ){
			LowByte = 0xf0;
			Tmp = ( HighByte >> 8);
			Tmp -= 0x1;
			HighByte = Tmp << 8;
		}
		Code = ( HighByte | LowByte);
	}
*/
	return Code;
}
					
/****************************************/
/*					*/
/*	Calculate scroll position	*/
/*					*/	
/****************************************/
int
CRefrList::GetBarPosition(
WORD 	Code)
{
	short   i, StartType, EndType;
	int     Pos = 0;

	Pos = (Code - StartCode) /NUM_CHAR;
	return Pos;
/*
	StartType = CheckCharType( StartCode);
	EndType   = CheckCharType( Code);
	if( EndType == CHAR_SBCS){
		Pos = ( Code - HiByteRange[CHAR_SBCS][CHN]) /NUM_CHAR;
		return Pos;

	}

	for (i = StartType; i < EndType; i++){
		if( i == CHAR_SBCS){
			Pos += (HiByteRange[CHAR_SBCS][CHN+1]
				- HiByteRange[CHAR_SBCS][CHN] + 1) / NUM_CHAR;
		}else{
        		Pos += CalculateCode( HiByteRange[i][CHN],
        			HiByteRange[i][CHN+1]) / NUM_CHAR;
		}
	}
	Pos += CalculateCode( HiByteRange[i][CHN], Code) / NUM_CHAR;
	return Pos;
*/
}
					
/****************************************/
/*					*/
/*	Calculate character code	*/
/*					*/
/****************************************/
WORD
CRefrList::CalculateCode(
WORD 	Start,
WORD 	End)
{
	WORD 	PageNum = 0;
	WORD	CodeNum = 0;

    	if ( Start >= End )
         	return 0;

    	PageNum = HIBYTE(End) - HIBYTE(Start);
#if 0
	if( CountryInfo.LangID == EUDC_CHS){
		if( HIBYTE(End)){
			WORD	LoCode;
			WORD	HiCode;

			HiCode = End & 0xff00;
			LoCode = End & 0x00ff;
			if( LoCode < 0xa0)
				LoCode = 0xa0;
			End = HiCode | LoCode;
		}
		CodeNum	= End - Start - PageNum * 0x00a0;
	}else   CodeNum = End - Start - PageNum * 0x0040;
#endif
	CodeNum = End - Start - PageNum * 0x0040;

    	return CodeNum;
}
					
/****************************************/
/*					*/
/*  	Calculate code from scroll pos	*/
/*					*/
/****************************************/
WORD
CRefrList::GetCodeScrPos(
int 	Pos)
{
	short	i, StartType, EndType;
	WORD	Code = 0;
	WORD	NumLine = 0, PNumLine = 0;

	if (Pos == 0)
		return StartCode;
	if (Pos >= BottomCode)
	  return ViewEnd;
	Code = StartCode + Pos * NUM_CHAR;
	Code &= 0xfff0;
	return Code;

/*
	if( !Pos)
        	return HiByteRange[1][CHN];
	if( Pos >= BottomCode)
        	return ViewEnd;

	StartType = CheckCharType( HiByteRange[1][CHN]);
	EndType   = CheckCharType( HiByteRange[4][CHN+1]);
	for( i = StartType; i <= EndType; ++i){
    if( i == CHAR_SBCS )
      NumLine += (HiByteRange[i][CHN+1] - HiByteRange[i][CHN] + 1) /NUM_CHAR;
    else
    	NumLine += CalculateCode( HiByteRange[i][CHN],HiByteRange[i][CHN+1])/NUM_CHAR;
    if( NumLine > Pos){
			NumLine = PNumLine;
      break;
    }
    PNumLine = NumLine;
	}
	Code = HiByteRange[i][CHN];
	Pos -= NumLine;
  while( Code < HiByteRange[i][CHN+1]){
    NumLine = CalculateCode( HiByteRange[i][CHN], Code) /NUM_CHAR;
    if( Pos <= NumLine){
	        	break;
		}
    Code += NUM_CHAR;
  }

	Code &= 0xfff0;
	return Code;
*/
}
					
/****************************************/
/*					*/
/*	Jump view if editbox get focus	*/
/*					*/
/****************************************/
BOOL
CRefrList::CodeButtonClicked()
{
	WCHAR CodeValue[15];
	WCHAR	CharValue[15];
	WORD	Code1, Code2;

	Code1 = (WORD)0;
	Code2 = (WORD)0;
#ifdef UNICODE
  ::GetDlgItemTextW(GetParent()->GetSafeHwnd(), IDC_EDITCODE, CodeValue, 15);
  ::GetDlgItemTextW(GetParent()->GetSafeHwnd(), IDC_EDITCHAR, CharValue, 15);
#else
	CHAR CodeValueA[15];
	CHAR CharValueA[15];
  ::GetDlgItemText(GetParent()->GetSafeHwnd(), IDC_EDITCODE, CodeValueA, 15);
  int nchar = ::GetDlgItemText(GetParent()->GetSafeHwnd(), IDC_EDITCHAR, CharValueA, 15);
  MultiByteToWideChar(CP_ACP, 0, CodeValueA, 4, CodeValue, sizeof(CodeValue));
  MultiByteToWideChar(CP_ACP, 0, CharValueA, nchar, CharValue, sizeof(CharValue));
#endif

  /*	::GetDlgItemTextA( GetParent()->GetSafeHwnd(),IDC_EDITCHAR, CharValue, 15); */

	if( CodeValue[0] == '\0' && CharValue[0] == '\0')
		return TRUE;

	if( CodeValue[0] == '\0')
		Code2 = 0xffff;
	else	Code2 = (WORD)wcstol((LPWSTR)CodeValue, (WCHAR **)0, 16);
		
	if( CharValue[0] == '\0')
		Code1 = 0xffff;
	else
		Code1  = CharValue[0];
	
	if( CodeFocus){
		if( !IsCheckedCode( Code2))
			goto Error;
		SelectCode = Code2;		
	}

	if( CharFocus){
		if( !IsCheckedCode( Code1))
			goto Error;
		SelectCode = Code1;		
	}
		
	if( SelectCode >= ViewEnd)
		ViewStart = ViewEnd;
	else	ViewStart = SelectCode & 0xfff0;

 	BottomCode = (WORD)GetBarPosition((WORD)ViewEnd);
 	this->SetScrollRange( SB_VERT, 0, BottomCode, FALSE);
	ScrlBarPos = (short)GetBarPosition( ViewStart);
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);
	this->InvalidateRect( &CodeListRect, TRUE);
	this->UpdateWindow();
	return TRUE;
Error:
	return FALSE;
}
					
/****************************************/
/*					*/
/*	Check Character Code Range	*/
/*					*/
/****************************************/
BOOL
CRefrList::IsCheckedCode(
WORD 	CodeStock)
{
	WORD	Offset;
	int	CharType;
	int	CharType1;
/*
	if(( CharType = CheckCharType( CodeStock)) == CHAR_ETC)
		return FALSE;
	CharType1 = CheckCharType( ViewEnd);

	Offset = CodeStock & 0x00ff;
	if( CharType == CHAR_SBCS){
		if( Offset < LoByteRange[0][CHN])
			return FALSE;
	}else{
		if( CharType1 == CHAR_SBCS)
			return FALSE;
		if( Offset >= LoByteRange[4][CHN] &&
		    Offset <= LoByteRange[4][CHN+1] )
			return FALSE;
		if(( Offset >= LoByteRange[1][CHN]  &&
		     Offset <= LoByteRange[1][CHN+1]) ||
		   ( Offset >= LoByteRange[2][CHN]  &&
		     Offset <= LoByteRange[2][CHN+1]) ||
		   ( Offset >= LoByteRange[3][CHN]  &&
		     Offset <= LoByteRange[3][CHN+1])){
				;
		}else	return FALSE;
	}
*/
	return TRUE;
}

#define	FIX_SPACE	6
#define	LINEWIDTH	4
/****************************************/
/*					*
/*	Calcurate character size	*/
/*					*/
/****************************************/
void
CRefrList::CalcCharSize()
{
	char	Dummy[] = "FA40";
	int	Sx;

	CClientDC	dc( this);
	this->GetClientRect( &CodeListRect);

	CFont	*OldFont = dc.SelectObject( &SysFFont);
	GetTextExtentPoint32A( dc.GetSafeHdc(), Dummy, 4, &FixSize);
	FixSize.cx += FIX_SPACE;
	dc.SelectObject( OldFont);

	CharSize.cy = ( CodeListRect.Height()
			- ((NUM_LINE-1)*LINEWIDTH) - 2) /NUM_LINE;
	CharSize.cx = ( CodeListRect.Width()
			- FixSize.cx - 2 - (NUM_CHAR*LINEWIDTH)) / NUM_CHAR;

	for( int i = 0; i < NUM_LINE; i++){
		rcReferCode[i].left   = 1;
		rcReferCode[i].top    = 1 + i*(CharSize.cy + LINEWIDTH);
		rcReferCode[i].right  = rcReferCode[i].left + FixSize.cx;
		rcReferCode[i].bottom = rcReferCode[i].top  + CharSize.cy;
		Sx = rcReferCode[i].right + LINEWIDTH;
		for( int j = 0; j < NUM_CHAR; j++){
			rcReferChar[i][j].left   = Sx + j*( CharSize.cx
						 + LINEWIDTH);
			rcReferChar[i][j].top    = rcReferCode[i].top;
			rcReferChar[i][j].right  = rcReferChar[i][j].left
						+ CharSize.cx;
			rcReferChar[i][j].bottom = rcReferChar[i][j].top
						+ CharSize.cy;
		}
	}
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CRefrList::OnPaint()
{
register int	i, j;
	WORD	Code;

	CPaintDC	dc( this);

	int BottomCode = GetBarPosition((WORD)ViewEnd);
	this->SetScrollRange( SB_VERT, 0, BottomCode, FALSE);
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);

//	Initialize character code
	Code = ViewStart;

//	Change Mapping mode
	int OldMode = dc.SetMapMode(MM_TEXT);
	dc.SetViewportOrg( 0, 0);
	CFont	*OldFont = dc.SelectObject( &SysFFont);

	for( i = 0; i < NUM_LINE; i++){
		int	xOffset, yOffset;
		TCHAR	Work[5];
		int	wLength, CharType;

//		Draw character code on character list to display
		dc.SelectObject( &SysFFont);
		wsprintf(Work, _T("%04X"), Code);
		dc.SetBkColor( COLOR_FACE);
		dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));  //COLOR_BLACK);

		if( rcReferCode[i].Height() > FixSize.cy){
			yOffset = (rcReferCode[i].Height() - FixSize.cy) /2;
		}else	yOffset = 0;

        ::ExtTextOut(dc, rcReferCode[i].left + FIX_SPACE/2,
				      rcReferCode[i].top  + yOffset,
			        ETO_OPAQUE, &rcReferCode[i],
			        (TCHAR *)Work, 4, NULL);

		dc.SelectObject( &CharFont);
		for( j = 0; j < NUM_CHAR; j++ , Code = GetPlusCode( Code, 1)){
			WORD	Offset;
			CSize	cSize;
      WCHAR Work1[5];
			Work1[0] = Code;
			Work1[1] = 0;
			wLength = 1;
/*
			if(( CharType = CheckCharType( Code)) == CHAR_SBCS){
				Work[0] = LOBYTE(Code);
				wLength = 1;

			}else if( CharType == CHAR_DBCS1 ||
				  CharType == CHAR_DBCS2 ||
				  CharType == CHAR_EUDC  ){
				Offset = Code & 0x00ff;
				if(( Offset <= LoByteRange[1][CHN+1]  &&
				     Offset >= LoByteRange[1][CHN]) ||
				   ( Offset <= LoByteRange[2][CHN+1]  &&
				     Offset >= LoByteRange[2][CHN]) ||
				   ( Offset <= LoByteRange[3][CHN+1]  &&
				     Offset >= LoByteRange[3][CHN]) ){
	   					Work[0] = (BYTE)((Code>>8) & 0x00ff);
    	        		Work[1] = (BYTE) (Code & 0x00ff);
        	    		wLength = 2;
				}else{
					continue;				
				}
	   		}else	wLength = 0;
            		Work[wLength] = (BYTE)'\0';

#ifdef UNICODE
            WCHAR wszCodeTemp[2];
            wLength = MultiByteToWideChar(dwCodePage,
                                              0,
                                              (LPSTR) Work,
                                              wLength,
                                              wszCodeTemp,
                                              1);
                Work[0] = LOBYTE(wszCodeTemp[0]);
                Work[1] = HIBYTE(wszCodeTemp[0]);
                Work[2] = '\0';
#endif
*/
			BOOL	PtIn;
			if( rcReferChar[i][j].PtInRect( LButtonPt) ||
			    SelectCode == Code){
				TCHAR 	CodeNum[10];

//				If character is selected by clickking
//			 	left button, draw it on dialog
				PtIn = TRUE;
				SelectCode = Code;
				dc.SetBkColor( COLOR_FACE);
				dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); // COLOR_BLACK);
				wsprintf((TCHAR *)CodeNum, _T("%04X"), Code);

				if (!CodeFocus)
				{
          ::SetDlgItemText(GetParent()->GetSafeHwnd(), IDC_EDITCODE, (LPCTSTR)CodeNum);
				}

				if (!CharFocus)
				{
#ifdef UNICODE
          ::SetDlgItemTextW(GetParent()->GetSafeHwnd(), IDC_EDITCHAR, (LPCWSTR)Work1);
#else
          CHAR Work2[4];
          int nchar=WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)Work1, 1, (LPSTR)Work2, sizeof(Work2), 0,0);
          Work2[nchar]=0;
          ::SetDlgItemText(GetParent()->GetSafeHwnd(), IDC_EDITCHAR, (LPCSTR)Work2);
#endif        
        }

			}else{
				PtIn = FALSE;
				dc.SetBkColor( COLOR_FACE);
				dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT)); // COLOR_BLACK);
			}
			BOOL sts = GetTextExtentPoint32W( dc.GetSafeHdc(),
				(LPCWSTR)Work1, wLength, &cSize);

			if( rcReferChar[i][j].Width() > cSize.cx){
				xOffset = rcReferChar[i][j].Width() - cSize.cx;
				xOffset /= 2;
			}else	xOffset = 0;

			if( rcReferChar[i][j].Height() > cSize.cy){
				yOffset = rcReferChar[i][j].Height() - cSize.cy;
				yOffset /= 2;
			}else	yOffset = 0;

//			Draw character code on character list
			ExtTextOutW(dc.GetSafeHdc(), rcReferChar[i][j].left + xOffset,
				       rcReferChar[i][j].top  + yOffset,
				       ETO_OPAQUE, &rcReferChar[i][j],
				       (LPCWSTR)Work1, wLength, NULL);
			DrawConcave( &dc, rcReferChar[i][j], PtIn);
		}
    }
    dc.SelectObject( OldFont);
	dc.SetMapMode(OldMode);
	LButtonPt.x = 0;
	LButtonPt.y = 0;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_VSCROLL"		*/
/*					*/
/****************************************/
void
CRefrList::OnVScroll(
UINT 		nSBCode,
UINT 		nPos,
CScrollBar	*pScrollBar)
{
	int	MoveScr;
	BOOL 	ThumbTrkFlag, ThumbPosFlag;

	BottomCode = (WORD)GetBarPosition((WORD)ViewEnd);
	this->SetScrollRange( SB_VERT, 0, BottomCode, FALSE);

	MoveScr = 0;
	ThumbTrkFlag = ThumbPosFlag = FALSE;
	switch( nSBCode){
		case SB_LINEDOWN:
			if(( ViewStart + NUM_CHAR) <= ViewEnd){
				MoveScr = 0 - (CharSize.cy + LINEWIDTH);
				ViewStart  = GetPlusCode( ViewStart,NUM_CHAR);
				ScrlBarPos = (short)GetBarPosition( ViewStart);
			}
			break;

		case SB_LINEUP:
			if(( ViewStart - NUM_CHAR) >= StartCode){
				MoveScr = CharSize.cy + LINEWIDTH;
				ViewStart  = GetMinusCode(ViewStart,NUM_CHAR);
				ScrlBarPos = (short)GetBarPosition( ViewStart);
			}
			break;

		case SB_PAGEDOWN:
			if(( ViewStart + NUM_CHAR*NUM_LINE) <= ViewEnd){
				MoveScr = ( 0 -(CharSize.cy+LINEWIDTH)) *NUM_LINE;
				ScrlBarPos = (short)GetBarPosition( ViewStart);
				ScrlBarPos += NUM_LINE;
				ViewStart = GetCodeScrPos( ScrlBarPos);
 			}else{
				MoveScr = ( 0 -(CharSize.cy+LINEWIDTH)) *NUM_LINE;
				ViewStart = ViewEnd;
				ScrlBarPos = (short)GetBarPosition( ViewStart);
			}
			break;

		case SB_PAGEUP:
			if(( ViewStart - NUM_CHAR*NUM_LINE) >= StartCode &&
				 ViewStart >= NUM_CHAR*NUM_LINE){
				MoveScr =  (CharSize.cy + LINEWIDTH) *NUM_LINE;
				ScrlBarPos = (short)GetBarPosition( ViewStart);
				ScrlBarPos -= NUM_LINE;
				ViewStart = GetCodeScrPos( ScrlBarPos);
			}else{
				MoveScr = (CharSize.cy + LINEWIDTH) *NUM_LINE;
				ViewStart = StartCode;
				ScrlBarPos = (short)GetBarPosition( ViewStart);
			}
			break;

		case SB_THUMBPOSITION:
			ThumbPosFlag = TRUE;
			ScrlBarPos = (short)nPos;
			ViewStart = GetCodeScrPos( nPos);
			break;

		case SB_THUMBTRACK:
			ThumbTrkFlag = TRUE;
			break;

		case SB_TOP:
			ViewStart = StartCode;
			ScrlBarPos = 0;
			break;

		case SB_BOTTOM:
			ViewStart = ViewEnd;
			ScrlBarPos = BottomCode;
			break;

		default:
			break;
	}
	if( abs( MoveScr) <= (CharSize.cy + LINEWIDTH) &&
	    abs( MoveScr) > 0 && !ThumbTrkFlag){
		CRect	ScrllRect;
		CRect	ClintRect;

		GetClientRect( &ClintRect);
		ScrllRect.CopyRect( &ClintRect);
		if( MoveScr < 0){
			ClintRect.top = 0 - MoveScr;
			this->ScrollWindow( 0, MoveScr, &ClintRect, NULL);
			ScrllRect.top = ( 0-MoveScr)*(NUM_LINE -1);
		}else{
			ClintRect.top = 0;
			ClintRect.bottom = MoveScr*(NUM_LINE -1);
			this->ScrollWindow( 0, MoveScr, &ClintRect, NULL);
			ScrllRect.top = 0;
			ScrllRect.bottom = ScrllRect.top + MoveScr;
		}
		this->InvalidateRect( &ScrllRect, FALSE);
	}else if( !ThumbTrkFlag && ( MoveScr || ThumbPosFlag)){
		this->InvalidateRect( &CodeListRect, TRUE);
	}
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_LBUTTONDOWN"	*/
/*					*/
/****************************************/
void
CRefrList::OnLButtonDown(
UINT	,
CPoint 	point)
{
	CRect	test;
	BOOL	PtIn;
unsigned int	i, j;

	PtIn = FALSE;
	this->SetFocus();
	for( i = 0; i < NUM_LINE; i++){
		for( j = 0; j < NUM_CHAR; j++){
			if( rcReferChar[i][j].PtInRect( point)){
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
	SearchKeyPosition( TRUE);
	SelectCode = 0;

	for( i = 0; i < NUM_LINE; i++){
		for( j = 0; j < NUM_CHAR; j++){
			if( rcReferChar[i][j].PtInRect( LButtonPt)){
				test.SetRect( rcReferChar[i][j].left - 2,
					      rcReferChar[i][j].top - 2,
					      rcReferChar[i][j].right + 2,
					      rcReferChar[i][j].bottom + 2);
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
CRefrList::OnLButtonDblClk(
UINT 	nFlags,
CPoint 	point)
{
	BOOL	PtIn;
unsigned int	i, j;

	LButtonPt = point;
	this->InvalidateRect( &CodeListRect, FALSE);
	this->UpdateWindow();

 	PtIn = FALSE;
	this->SetFocus();
	for( i = 0; i < NUM_LINE; i++){
		for( j = 0; j < NUM_CHAR; j++){
			if( rcReferChar[i][j].PtInRect( point)){
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
CRefrList::OnSetCursor(
CWnd* 	pWnd,
UINT 	nHitTest,
UINT 	message)
{
	::SetCursor( AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	return TRUE;
}

/****************************************/
/*					*/
/*	Reset parameter			*/
/*					*/
/****************************************/
void
CRefrList::ResetParam()
{
	LButtonPt.x = LButtonPt.y = 0;
/*
	SelectCode = HiByteRange[CHAR_SBCS][CHN];
*/
	SelectCode = 0x0020;
	ScrlBarPos = 0;
	this->SetScrollPos( SB_VERT, ScrlBarPos, TRUE);
}

/****************************************/
/*					*/
/*	Draw Concave ractangle		*/
/*					*/
/****************************************/
void
CRefrList::DrawConcave(
CDC 	*dc,
CRect 	rect,
BOOL 	PtIn)
{
	CBrush	ConBrush, *OldBrush;
	CRect	Rt;

	Rt.SetRect( rect.left-1, rect.top-1, rect.right+1, rect.bottom+1);

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
			CPen 	fPen, *OldPen;

			fBrush.CreateStockObject( NULL_BRUSH);
			fPen.CreatePen( PS_SOLID, 1, COLOR_FACE);
			OldBrush = dc->SelectObject( &fBrush);
			OldPen   = dc->SelectObject( &fPen);
			dc->Rectangle( &rect);
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
			CPen 	fPen, *OldPen;

			fBrush.CreateStockObject( NULL_BRUSH);
			fPen.CreatePen( PS_SOLID, 1, COLOR_SHADOW);
			OldBrush = dc->SelectObject( &fBrush);
			OldPen   = dc->SelectObject( &fPen);
			dc->Rectangle( &rect);
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
CRefrList::OnKeyDown(
UINT 	nChar,
UINT 	nRepCnt,
UINT 	nFlags)
{
	int	sPos;
	int	ePos;

	if( nChar == VK_UP   || nChar == VK_DOWN ||
	    nChar == VK_LEFT || nChar == VK_RIGHT){
		sPos = GetBarPosition( ViewStart);
		ePos = GetBarPosition( SelectCode);
		if( ePos - sPos >= NUM_LINE || ePos < sPos){
			ViewStart = SelectCode & 0xfff0;
			ScrlBarPos = (short)GetBarPosition( ViewStart);
			this->Invalidate(FALSE);
			this->UpdateWindow();
		}

	    	switch( nChar){
		case VK_UP:
			if( SelectCode - NUM_CHAR < StartCode)
				break;
			if( SelectCode - NUM_CHAR < ViewStart){
				this->SendMessage(WM_VSCROLL, SB_LINEUP, 0);
			}
			SearchKeyPosition(FALSE);
			SelectCode = GetMinusCodeKey( SelectCode, NUM_CHAR);
			SearchKeyPosition(FALSE);
			break;
		case VK_DOWN:

			if( SelectCode + NUM_CHAR > EndCode)
				break;
			if( ePos - sPos >= NUM_LINE - 1){
				this->SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
			}
			SearchKeyPosition(FALSE);
			SelectCode = GetPlusCodeKey( SelectCode, NUM_CHAR);
			SearchKeyPosition(FALSE);
			break;
		case VK_LEFT:
			if( SelectCode - 1 < StartCode)
				break;
			if( SelectCode - 1 < ViewStart){
				this->SendMessage(WM_VSCROLL, SB_LINEUP, 0);
			}
			SearchKeyPosition(FALSE);
			SelectCode = GetMinusCodeKey( SelectCode, 1);
			SearchKeyPosition(FALSE);
			break;
		case VK_RIGHT:
			WORD 	TmpCode;
			int	TmpPos;

			if( SelectCode + 1 > EndCode)
				break;
			TmpCode = GetPlusCodeKey( SelectCode, 1);
			sPos = GetBarPosition( ViewStart);
			ePos = GetBarPosition( SelectCode);
			TmpPos = GetBarPosition( TmpCode);
			if( TmpPos - sPos >= NUM_LINE){
				this->SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
			}
			SearchKeyPosition(FALSE);
			SelectCode = GetPlusCodeKey( SelectCode, 1);
			SearchKeyPosition(FALSE);
			break;
		}

	}else 	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

/****************************************/
/*					*/
/*	Search focus position		*/
/*					*/
/****************************************/
void
CRefrList::SearchKeyPosition(
BOOL	Flg)
{
	CRect	test;
	int	sViewPt, sCodePt;
	int	sType, eType;
unsigned int	i, j;

	sViewPt = GetBarPosition( ViewStart);
	sCodePt = GetBarPosition( SelectCode);
	if(( sViewPt > sCodePt || abs( sCodePt - sViewPt) >= NUM_LINE) && Flg){
		this->Invalidate( FALSE);
		return;
	}
	i = (unsigned int)(sCodePt - sViewPt);
	j = (unsigned int)(SelectCode & 0x000f);
/*
	sType = CheckCharType( ViewStart);
	eType = CheckCharType( SelectCode);
	if( sType != eType && sType != CHAR_SBCS)
		i++;
*/
	test.SetRect( rcReferChar[i][j].left - 2,
		      rcReferChar[i][j].top - 2,
		      rcReferChar[i][j].right + 2,
		      rcReferChar[i][j].bottom + 2);
	this->InvalidateRect( &test, FALSE);
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_SETFOCUS"		*/
/*					*/
/****************************************/
void
CRefrList::OnSetFocus(
CWnd* 	pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);
	this->HideCaret();
	FocusFlag = TRUE;
	SearchKeyPosition(TRUE);
	this->UpdateWindow();
}

/****************************************/
/*					*/
/*	SearchCode			*/
/*					*/
/****************************************/
BOOL
CRefrList::IsCorrectChar(
UINT 	i,
UINT 	j)
{
	int	sViewPt, CharType;
	WORD	wCode;
	WORD	sOffset;
	BOOL	flg;

	flg = FALSE;
	BottomCode = (WORD)GetBarPosition((WORD)ViewEnd);
	sViewPt = GetBarPosition( ViewStart);
	wCode = GetCodeScrPos( sViewPt + i);
	wCode |= j;	

/*
	CharType = CheckCharType( wCode);	
	sOffset = wCode & 0x00ff;
	if( CharType == CHAR_SBCS){
		if( sOffset >= LoByteRange[0][CHN] &&
		    sOffset <= LoByteRange[0][CHN+1])
		    	flg = TRUE;
	}else{
		if(( sOffset >= LoByteRange[1][CHN] &&
		     sOffset <= LoByteRange[1][CHN+1]) ||
		   ( sOffset >= LoByteRange[2][CHN] &&
		     sOffset <= LoByteRange[2][CHN+1]) ||
		   ( sOffset >= LoByteRange[3][CHN] &&
		     sOffset <= LoByteRange[3][CHN+1]))
		     	flg = TRUE;
	}

	return flg;
*/
	return TRUE;
}

/****************************************/
/*					*/
/*	Increase key focus		*/
/*					*/
/****************************************/
WORD
CRefrList::GetPlusCodeKey(
WORD 	Code,
int 	ScrollNum)
{
  /*
	WORD	PrevCode;
	WORD 	LowByte, HighByte;
	WORD	LLByte;
	int	CharType;
	int	CharType1, CharType2;
	int	Offset;
  */
	Code += (WORD)ScrollNum;
	/*
  PrevCode = (WORD)(Code - (WORD)ScrollNum);
	CharType1 = CheckCharType( PrevCode);
	CharType2 = CheckCharType( Code);
	if( CharType1 != CharType2){
		if( CharType1 == CHAR_EUDC)
			Code = PrevCode;		
		else{
			Offset = Code - HiByteRange[CharType1][CHN+1];
			Code = (WORD)( HiByteRange[CharType1+1][CHN] +Offset-1);
		}
	}
	CharType = CheckCharType( Code);
	if( CharType != CHAR_SBCS){
		LowByte  = Code & 0x00ff;
		HighByte = Code & 0xff00;
		LLByte	 = Code & 0x000f;
		if( LowByte <= LoByteRange[4][CHN+1] &&
		    LowByte >= LoByteRange[4][CHN] ){
			LowByte = 0x0040 + LLByte;
		}
		Code = ( HighByte | LowByte);
	}*/
	return Code;
}
					
/****************************************/
/*					*/
/*	Decrease focus key		*/
/*					*/
/****************************************/
WORD
CRefrList::GetMinusCodeKey(
WORD 	Code,
int 	ScrollNum)
{
  /*
	WORD	PrevCode;
	WORD	LLByte;
	int	CharType;
	int	CharType1, CharType2;
	int	Offset;
  */
	Code -= (WORD)ScrollNum;
  /*
	PrevCode = (WORD)( Code + (WORD)ScrollNum);
	CharType1 = CheckCharType( Code);
	CharType2 = CheckCharType( PrevCode);
	if( CharType1 != CharType2){
	 	if( CharType2 == CHAR_SBCS)
			return (WORD)HiByteRange[CHAR_SBCS][CHN];
		else{
			Offset = HiByteRange[CharType2][CHN] - Code;
			return (WORD)(HiByteRange[CharType2-1][CHN+1]-Offset+1);
		}	
	}
	CharType = CheckCharType( Code);
	if( CharType != CHAR_SBCS){
		WORD 	LowByte;
		WORD	HighByte;
		WORD	Tmp;

		LowByte  = Code & 0x00ff;
		HighByte = Code & 0xff00;
		LLByte	 = Code & 0x000f;
		if( LowByte <= LoByteRange[4][CHN+1] &&
		    LowByte >= LoByteRange[4][CHN] ){
			LowByte = 0xf0 + LLByte;
			Tmp = ( HighByte >> 8);
			Tmp -= 0x1;
			HighByte = Tmp << 8;
		}
		Code = ( HighByte | LowByte);
	}*/
	return Code;
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_KILLFOCUS"		*/
/*					*/
/****************************************/
void
CRefrList::OnKillFocus(
CWnd* 	pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	FocusFlag = FALSE;
	SearchKeyPosition(TRUE);
	this->UpdateWindow();
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_RBUTTONUP"		*/
/*					*/
/****************************************/
void
CRefrList::OnRButtonUp(
UINT 	nFlags,
CPoint 	point)
{
	GetParent()->SendMessage( WM_CONTEXTMENU, (WPARAM)this->GetSafeHwnd(), 0);
}

BEGIN_MESSAGE_MAP( CRefListFrame, CStatic)
	//{{AFX_MSG_MAP( CRefListFrame)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CRefListFrame::CRefListFrame()
{
}

/****************************************/
/*					*/
/*		Destructor		*/
/*					*/
/****************************************/
CRefListFrame::~CRefListFrame()
{
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CRefListFrame::OnPaint()
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
CRefListFrame::DrawConcave(
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

BEGIN_MESSAGE_MAP( CRefInfoFrame, CStatic)
	//{{AFX_MSG_MAP( CRefInfoFrame)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************/
/*					*/
/*	Default Constructor		*/
/*					*/
/****************************************/
CRefInfoFrame::CRefInfoFrame()
{
}

/****************************************/
/*					*/
/*	Destructor			*/
/*					*/
/****************************************/
CRefInfoFrame::~CRefInfoFrame()
{
}

/****************************************/
/*					*/
/*	MESSAGE	"WM_PAINT"		*/
/*					*/
/****************************************/
void
CRefInfoFrame::OnPaint()
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
CRefInfoFrame::DrawConcave(
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
