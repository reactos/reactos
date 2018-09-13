
//#ifdef WIN

/* BabakJ: The stuff in this ifdef is hack for using \\popcorn env */
//#define NOCOMM
/* #define _NTDEF_  to get NT defs, i.e. WORD unsigned short, not int */

#include <windows.h>
#include <port1632.h>


// Babakj: Set DEBUG based on DBG (1 or 0) to do FREE or CHECKED builds of Solitaire
#if DBG
#define DEBUG
#endif


/* #include <winkrnl.h>   To define OFSTRUCT */
/* OpenFile() Structure */
//typedef struct tagOFSTRUCT
//  {
//    BYTE        cBytes;
//    BYTE        fFixedDisk;
//    WORD        nErrCode;
//    BYTE        reserved[4];
//    BYTE        szPathName[128];
//  } OFSTRUCT;
//typedef OFSTRUCT            *POFSTRUCT;
//typedef OFSTRUCT NEAR       *NPOFSTRUCT;
//typedef OFSTRUCT FAR        *LPOFSTRUCT;
//#define OF_CREATE           0x1000
//#define OF_WRITE            0x0001
/* End if stuff taken from Winkrnl.h */
//#endif

#include <stdlib.h>
#include <time.h>
#include "std.h"
#include "crd.h"
#include "col.h"
#include "undo.h"
#include "solid.h"
#include "game.h"
#include "soldraw.h"
#include "back.h"
#include "stat.h"
#include "klond.h"
#include "debug.h"


VOID ChangeBack( INT );
VOID WriteIniFlags( INT );
BOOL FYesNoAlert( INT );
VOID DoOptions( VOID );
VOID DoBacks( VOID );
VOID NewGame( BOOL, BOOL );
BOOL APIENTRY cdtDraw( HDC, INT, INT, INT, INT, DWORD );
BOOL APIENTRY cdtDrawExt(HDC, INT, INT, INT, INT, INT, INT, DWORD);
BOOL FCreateStat( VOID );
BOOL FSetDrag( BOOL );
BOOL FInitGm( VOID );
BOOL APIENTRY cdtInit( INT FAR *, INT FAR * );
typedef INT (*COLCLSCREATEFUNC)();
COLCLS *PcolclsCreate(INT tcls, COLCLSCREATEFUNC lpfnColProc,
							DX dxUp, DY dyUp, DX dxDn, DY dyDn,
							INT dcrdUp, INT dcrdDn);
COL *PcolCreate(COLCLS *pcolcls, X xLeft, Y yTop, X xRight, Y yBot, INT icrdMax);
VOID SwapCards(CRD *pcrd1, CRD *pcrd2);
BOOL FCrdRectIsect(CRD *pcrd, RC *prc);
BOOL FRectIsect(RC *prc1, RC *prc2);
BOOL FPtInCrd(CRD *pcrd, PT pt);

VOID DrawCard(CRD *pcrd);
VOID DrawCardPt(CRD *pcrd, PT *ppt);
VOID DrawBackground(X xLeft, Y yTop, X xRight, Y yBot);
VOID DrawBackExcl(COL *pcol, PT *ppt);
VOID EraseScreen(VOID);
VOID OOM( VOID );

HDC HdcSet(HDC hdc, X xOrg, Y yOrg);
extern X xOrgCur;
extern Y yOrgCur;


#define AssertHdcCur() Assert(hdcCur != NULL)


BOOL FGetHdc( VOID );
VOID ReleaseHdc( VOID );

typedef union
	{
	struct _ini
		{
		BOOL fStatusBar : 1;
		BOOL fTimedGame : 1;
		BOOL fOutlineDrag : 1;
		BOOL fDrawThree : 1;
		unsigned fSMD: 2;
		BOOL fKeepScore : 1;
		BOOL unused:8;
		} grbit;
	DWORD w;
	} INI;


/* WriteIniFlags flags */

#define wifOpts   0x01
#define wifBitmap 0x02
#define wifBack   0x04

#define wifAll wifOpts|wifBitmap|wifBack




/* externals    */
/* sol.c        */
extern TCHAR   szAppName[]; // name of this application (solitaire)
extern TCHAR   szScore[];   // title 'score:' for internationalization
extern HWND   hwndApp;      // handle to main window of app
extern HANDLE hinstApp;     // handle to instance of app
extern BOOL   fBW;          // true if on monochrome video (not NT!)
extern HBRUSH hbrTable;     // handle to brush of table top
extern LONG   rgbTable;     // RGB value of table top
extern INT    modeFaceDown; // back of cards bmp id
extern BOOL   fIconic;      // true if app is iconic
extern INT    dyChar;       // tmHeight for textout
extern INT    dxChar;       // tmMaxCharWidth for textout
extern GM*    pgmCur;       // current game
extern DEL    delCrd;
extern DEL    delScreen;
extern PT     ptNil;        // no previous pt (nil)

#define dxCrd delCrd.dx
#define dyCrd delCrd.dy
#define dxScreen delScreen.dx
#define dyScreen delScreen.dy

extern RC     rcClient;     // client rectangle after resize
extern INT    igmCur;       // the current game #, srand seeded with this
#ifdef DEBUG
extern BOOL   fScreenShots;  // ???
#endif
extern HDC    hdcCur;       // current HDC to draw on (!)
extern INT    usehdcCur;    // hdcCur use count
extern X      xOrgCur;
extern Y      yOrgCur;

extern TCHAR   szOOM[50];   // "out of memory" error message

extern BOOL   fStatusBar;   // true if we are to show status
extern BOOL   fTimedGame;   // true if we are to time game
extern BOOL   fKeepScore;   // true if keeping score (vegas only)
extern SMD    smd;          // Score MoDe (std, vegas, none)
extern INT    ccrdDeal;
extern BOOL   fOutlineDrag;

extern BOOL   fHalfCards;
extern int    xCardMargin;


/* stat.c         */
extern HWND  hwndStat;      // hwnd of status window

/* col.c          */
extern BOOL  fMegaDiscardHack;  // true if called from DiscardMove
extern MOVE  move;              // move data

/* klond.c        */
BOOL PositionCols(void);
extern BOOL fKlondWinner;       // true if we needn't round card corners


#ifdef DEBUG
WORD ILogMsg( VOID *, INT, WPARAM, LPARAM, BOOL );
VOID LogMsgResult( INT, LRESULT );
#endif

