/**************************************************/
/*						                          */
/*						                          */
/*	EUDC EDITOR (MAIN HEADER)		              */
/*						                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"resource.h"
#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include <htmlhelp.h>

/* EUDC editor defined windows message */
#define		WM_IMPORTGAGE	(WM_USER)	// start import
#define		WM_IMPORTSTOP	(WM_USER + 1)	// stop import
#define		WM_DUPLICATE	(WM_USER + 2)	// duplicate bitmap
#define		WM_VIEWUPDATE	(WM_USER + 3)	// view update

/* Country ID */
#define		EUDC_JPN	0x411		// Japanese
#define		EUDC_CHT	0x404		// Chinese (Taipei)
#define		EUDC_CHS	0x804		// Chinese (General)
#define		EUDC_HKG	0xc04		// Chinese (Hong Kong)
#define		EUDC_SIN	0x1004		// Chinese (Singapore)
#define		EUDC_KRW	0x412		// Korea   (Wansung)
#define   EUDC_HEBREW 0x40d
#define   EUDC_HINDI  0x439
#define   EUDC_TAMIL  0x449

/*
#ifdef BUILD_ON_WINNT
#define		EUDC_UNICODE	0xFFFF		// Unicode
#endif
*/

/* Smoothing level */
#define		SMOOTHLVL	1		// show outline
						// regist truetype
/* Code string size */
#define		MAX_CODE	5		// size of code string 

/* Tool */
#define		NOTSELECT	0		// "Not Selected"
#define		PEN		1		// "Pen" 
#define		SLOPE		2		// "Straight Line"
#define		RECTBAND	3		// "Hollow Ractangle"
#define		RECTFILL	4		// "Filled Rectangle"
#define		CIRCLE		5		// "Hollow Circle"
#define		CIRCLEFILL	6		// "Filled Circle"
#define		RECTCLIP	7		// "Rectangular Selection"
#define		BRUSH		8		// "Brush"
#define		FREEFORM	9		// "Freeform Selection"
#define		ERASER		10		// "Eraser"

/* Cursor */
#define		VERTICAL	0		// vertical arrow for resize 
#define		RIGHTSLOPE	1		// right arrow for resize
#define		LEFTSLOPE	2		// left arrow for resize
#define		HORIZONTAL	3		// horizontal arrow for resize
#define		ALLDIRECT	4		// all direct arrow for resize
#define		NUMRESIZE	5		// the number of resize cursor
#define		NUMTOOL		11		// the number of tool cursor

/* Color */
#define		COLOR_WHITE	RGB(255,255,255) // white
#define		COLOR_BLACK	RGB(  0,  0,  0) // black
#define		COLOR_LGRAY	RGB(192,192,192) // light Gray
#define		COLOR_DGRAY	RGB(128,128,128) // dark Gray
#define		COLOR_YELLO	RGB(255,255,  0) // yellow
#define		COLOR_BLUE	RGB(  0,  0,255) // blue
#define		COLOR_RED	RGB(255,  0,  0) // red

/* Bitmap size */
#define		MAX_BITMAPSIZE	256		// maximun of bitmap size
#define		MIN_BITMAPSIZE	16		// minimun of bitmap size
#define		DEF_BITMAPSIZE	64		// default of bitmap size

/* Selected font, file and code structure */
typedef struct _tagSELECTEUDC{
	TCHAR	m_Font[LF_FACESIZE];		// selected EUDC typeFace
	TCHAR	m_File[MAX_PATH];		// selected EUDC fileName
	TCHAR	m_FileTitle[MAX_PATH];		// selected EUDC fileTitle
	TCHAR	m_Code[MAX_CODE];		// selected EUDC code
	BOOL	m_FontTypeFlg;			// flag whether TTF or not
}SELECTEUDC;

/* EUDC coderange and languageID structure */
typedef struct _tagCOUNTRYINFO{
	INT	nRange;				// the number of code range
	USHORT	sRange[MAX_LEADBYTES];		// start of code range
	USHORT	eRange[MAX_LEADBYTES];		// end of code range
	INT	nLeadByte;			// the number of lead byte
	UCHAR	sLeadByte[MAX_LEADBYTES];	// start of lead byte
	UCHAR	eLeadByte[MAX_LEADBYTES];	// end of lead byte
	INT	nTralByte;			// the number of trail byte
	UCHAR	sTralByte[MAX_LEADBYTES];	// start of trail byte
	UCHAR	eTralByte[MAX_LEADBYTES];	// end of trail byte
    TCHAR   szForceFont[LF_FACESIZE];   // default font facename;

#ifdef BUILD_ON_WINNT
    BOOL bUnicodeMode;                      // user select unicode mode
	BOOL bOnlyUnicode;                      // We only have unicode 


/*  For CHS to keep the original trail byte range in order to dynamically 
 *  calculate trailbyte range with EUDC selection range.
 */
    INT nOrigTralByte;
	UCHAR	sOrigTralByte[MAX_LEADBYTES];	// start of trail byte
	UCHAR	eOrigTralByte[MAX_LEADBYTES];	// end of trail byte
#endif // BUILD_ON_WINNT

	INT	LangID;				// language ID
	INT	CharacterSet;			// Character Set
	INT	CurrentRange; 			// selected currently range 
}COUNTRYINFO;

/* DBCS coderange except EUDC */
/*typedef struct _tagDBCSINFO{
	INT	nLeadByte;			// the number of lead byte
	UCHAR	sLeadByte[MAX_LEADBYTES];	// start of lead byte
	UCHAR	eLeadByte[MAX_LEADBYTES];	// end of lead byte
	INT	nTralByte;			// the number of trail byte
	UCHAR	sTralByte[MAX_LEADBYTES];	// start of trail byte
	UCHAR	eTralByte[MAX_LEADBYTES];	// end of trail byte
}DBCSINFO;
*/



/* Global parameter */
extern HCURSOR	ToolCursor[NUMTOOL];		// tool cursor
extern HCURSOR	ArrowCursor[NUMRESIZE];		// resize cursor
extern INT	CAPTION_HEIGHT;			// height of caption
extern INT	BITMAP_HEIGHT;			// height of bitmap
extern INT	BITMAP_WIDTH;			// width of bitmap
extern DWORD	COLOR_GRID;			// grid color
extern DWORD	COLOR_CURVE;			// outline color
extern DWORD	COLOR_FITTING;			// bitmap color in show outline
extern DWORD	COLOR_FACE;			// Win95 3D Face System Color
extern DWORD	COLOR_SHADOW;			// Win95 3D Shadow Color	
extern DWORD	COLOR_HLIGHT;			// Win95 3D HighLight
extern DWORD	COLOR_WIN;			// Win95 Window Color
extern TCHAR	HelpPath[MAX_PATH];		// Help file path
extern TCHAR	ChmHelpPath[MAX_PATH];	// Help file path
extern TCHAR	FontPath[MAX_PATH];		// Font file path
extern CString	NotMemTtl;
extern CString	NotMemMsg;
extern SELECTEUDC	SelectEUDC;
extern COUNTRYINFO	CountryInfo;


class CEudcApp : public CWinApp
{
public:
	CEudcApp();
	virtual BOOL	InitInstance();
	virtual BOOL	ExitInstance();
	virtual BOOL	OnIdle(LONG lCount);

private:
	BOOL	CheckPrevInstance();
	BOOL	GetProfileText( LPRECT MainWndRect, UINT *MaxWndFlag);
	BOOL	GetCountryInfo();
	BOOL	GetCursorRes();
	BOOL	GetFilePath();

public:
	//{{AFX_MSG(CEudcApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifdef BUILD_ON_WINNT
//
// Hard codeded font face name
//
//
// Public API prototype definition.
//  (should be in wingdi.h)
//
extern "C" BOOL APIENTRY EnableEUDC(BOOL bEnable);
#endif // BUILD_ON_WINNT

