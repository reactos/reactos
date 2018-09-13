/*	File: D:\WACKER\tdll\term.hh (Created: 07-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

/* --- Macros --- */

#define MAX_EMUCOLORS	16
#define BEZEL_SIZE		13
#define OUTDENT 		3

// MARK_XOR and MARK_ABS used to tell MarkText() to mark text in terminal
// window image buffer.

#define MARK_XOR	1
#define MARK_ABS	2

/* --- Private Messages --- */

#define WM_TERM_SCRLMARK	WM_USER+0x200
#define WM_TERM_KLUDGE_FONT WM_USER+0x201

/* --- Data Structures --- */

typedef struct stTerm *HHTERM;

struct stTerm
	{
	ECHAR	   **fplpstrTxt; 		// Text buffer
	PSTATTR    *fppstAttr;			// array of pointers to attribute structs.
	BYTE		abBlink[MAX_EMUROWS]; // array of chars used to check blinking

	int 		iTopline;			// top line of text buffer.

	int 		iRows,				// number of rows
				iCols;				// number of columns

	HBRUSH		hbrushBackScrl, 	// color of backscroll region.
				hbrushDivider,		// color of divider bar.
				hbrushTerminal, 	// emulator's clear attribute.
				hbrushHighlight,	// highlight brush
				hbrushBackHatch,	// backscroll hatched brush
				hbrushTermHatch;	// terminal hatched brush

	COLORREF	crBackScrl, 		// RGB background color of backscroll.
				crBackScrlTxt,		// RGB text color of backscroll.
				crTerm; 			// RGB terminal background (clear attr).

	HPEN		hBlackPen,			// Pen used to draw bezel border
				hWhitePen,			// Pen used to draw bezel highlight
				hDkGrayPen, 		// Pen used to draw bezel face
				hLtGrayPen; 		// Pen used to draw bezel shadow

	HWND		hwnd,				// terminal window handle.
				hwndSession;		// session window handle.
	HSESSION	hSession;			// session handle.

	HFONT		hFont,				// terminal font
				hDblHiFont, 		// double high
				hDblWiFont, 		// double wide
				hDblHiWiFont;		// double high wide

	HFONT		hSymFont,			// symbol terminal font
				hSymDblHiFont,		// symbol double high
				hSymDblWiFont,		// symbol double wide
				hSymDblHiWiFont;	// symbol double high wide

	LOGFONT 	lf, 				// terminal logfont structure
				lfSys,				// logfont read from system file
				lfHold; 			// for kludge font

	int 		xChar,				// width of font
				yChar,				// height of font
				cx, 				// width of client area
				cy, 				// height of client area
				iTermHite,			// terminal height expressed as rows
				iEvenFont;			// Flag indicating we have an even pixeled font

	int 		iBlink, 			// state of blink (-1=on,0=disabled,1=off)
				fBlink, 			// true if blinking host cursor set
				fFocus, 			// true if we have the focus
				fBump,				// used to position vertical thumb
				fCapture,			// mouse captured
				fHstCurOn,			// host cursor
				fSelectByWord,		// selecting text by word
				fScrolled,			// scrolling routines set this.
				iBtnOne,			// what a dbl clk button one does
				fLclCurOn,			// marking cursor
				fCursorsLinked, 	// cursors are the same
				fCursorTracking,	// terminal view tracks to cursor
				fMarkingLock,		// true if we've marked text
				fExtSelect, 		// true if extended text selection on
				fBackscrlLock,		// true if only backscroll visible
				fItalic;			// true if font is italic

	unsigned	uBlinkRate; 		// blink rate set on handle creation

	const COLORREF *pacrEmuColors;	// pointer array to emulator colors

	int 		iVScrlMin,			// scrollbar control variables
				iVScrlMax,
				iVScrlPos,
				iHScrlMin,
				iHScrlMax,
				iHScrlPos;

	int 		xBezel, 			// size of bezel in pixels
				xIndent,			// amount in pixels to indent from bezel
				yBrushOrg;			// brush orign used for hatchbrush

	POINT		ptHstCur,			// host cursor coordinates
				ptLclCur,			// local (marking) cursor coordinates
				ptBeg,				// beginning of marked region
				ptEnd;				// end of marked region

	int 		iCurType;			// Type of cursor defined in emu\emu.h
	int 		iHstCurSiz; 		// Vertical size of host cursor

	HTIMER		hCursorTimer,		// timer for blinking terminal cursor
				hMarkingTimer;		// used for scroll marking

	HUPDATE 	hUpdate;			// copy of info from termGetUpdate()

	TCHAR		underscores[MAX_EMUCOLS];	// used to do underlining

	/* --- Stuff from Client-side backscroll handle ---*/

	ECHAR	   **fplpstrBkTxt;		// bckscrl text buffer
	int 		iPhysicalBkRows,	// # of physical rows used by the buffer
				iMaxPhysicalBkRows, // Max size of buffer based on font.
				iNextBkLn,			// next line to receive text in buffer
				iBkLines;			// used in TP_WM_SIZE()
	};

/* --- Function prototypes --- */

/* --- termproc.c --- */

LRESULT CALLBACK TermProc(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar);

void TP_WM_SIZE(const HWND hwnd,
				const unsigned fwSizeType,
				const int iWidth,
				const int iHite);

/* --- termhdl.c --- */

HHTERM CreateTerminalHdl(const HWND hwndTerm);
void DestroyTerminalHdl(const HHTERM hhTerm);
BOOL termSysColorChng(const HHTERM hhTerm);

void termSetClrAttr(const HHTERM hhTerm);
BOOL termSetFont(const HHTERM hhTerm, const PLOGFONT plf);

HFONT termMakeFont(const HHTERM hhTerm, const BOOL fUnderline,
				   const BOOL fHigh, const BOOL fWide, const BOOL fSymbol);

/* --- termcpy.c --- */

int strlentrunc(const ECHAR *pach, const int iLen);

/* --- termmos.c --- */

void TP_WM_LBTNDN(const HWND hwnd, const unsigned uFlags, const int xPos, const int yPos);
void TP_WM_MOUSEMOVE(const HWND hwnd, const unsigned uFlags, const int xPos, const int yPos);
void TP_WM_LBTNUP(const HWND hwnd, const unsigned uFlags, const int xPos, const int yPos);
void TP_WM_LBTNDBLCLK(const HWND hwnd, const unsigned uFlags, const int xPos, const int yPos);

/* --- termcur.c --- */

void ShowCursors(const HHTERM hhTerm);
void HideCursors(const HHTERM hhTerm);
void PaintHostCursor(const HHTERM hhTerm, const BOOL fOn, const HDC hdc);
void PaintLocalCursor(const HHTERM hhTerm, const BOOL fOn, const HDC hdc);
void CALLBACK CursorTimerProc(void *pvhwnd, long ltime);
void LinkCursors(const HHTERM hhTerm);
void SetLclCurPos(const HHTERM hhTerm, const LPPOINT lpptCur);

void MoveSelectionCursor(const HHTERM hhTerm,
						 const HWND hwnd,
							   int	x,
							   int	y,
							   BOOL fMarking);

/* --- termutil.c --- */

BOOL termTranslateKey(const HHTERM hhTerm, const HWND hwnd, const KEY_T Key);
void BlinkText(const HHTERM hhTerm);
void termQuerySnapRect(const HHTERM hhTerm, LPRECT prc);

void UnmarkText(const HHTERM hhTerm);
void MarkTextAll(HHTERM hhTerm);
void TestForMarkingLock(const HHTERM hhTerm);
void CALLBACK MarkingTimerProc(void *pvhWnd, long lTime);

void MarkText(const HHTERM	   hhTerm,
			  const LPPOINT    ptBeg,
			  const LPPOINT    ptEnd,
			  const BOOL	   fMark,
			  const int 	   sMarkingMethod);

BOOL PointInSelectionRange(const PPOINT ppt,
						   const PPOINT pptBeg,
						   const PPOINT pptEnd,
						   const int	iCols);

#define	VP_NO_ADJUSTMENT		0
#define	VP_ADJUST_RIGHT			1
#define	VP_ADJUST_LEFT			2

BOOL termValidatePosition(const HHTERM	hhTerm,
						  const int		nAdjustmentMode,
						  		POINT	*pLocation);

/* --- termupd.c --- */

void termGetUpdate(const HHTERM hhTerm, const int fRedraw);
void termGetBkLines(const HHTERM hhTerm, const int iScrlInc, int yBeg, int sType);
void termFillBk(const HHTERM hhTerm, const int iScrlPos);

/* --- termpnt.c --- */

void termPaint(const HHTERM hhTerm, const HWND hwnd);
