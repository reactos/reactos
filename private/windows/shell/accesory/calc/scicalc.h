/****************************Module*Header***********************************\
* Module Name: SCICALC.H
*
* Module Descripton: Main header file
*
* Warnings:
*
* Created:
*
* Author:
\****************************************************************************/

#define CALC_COMPILE

/* To keep a buncha junk outa compiles */
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NODRAWFRAME
#define NOKEYSTATES
#define OEMRESOURCE
#define NOATOM
#define NOMETAFILE
#define NOOPENFILE
#define NOSOUND
#define NOWH
#define NOCOMM
#define NOKANJI

#include <windows.h>
#include <windowsx.h>
#include "scimath.h"
#include "resource.h"
#include "wassert.h"    // our own simple little assert
#include <htmlhelp.h>

#define CSTRMAX        256   /* Maximum length of any one string.         */
#define CCHSTRINGSMAX  1024  /* Initial bytes to allocate for strings.    */

#define CMS_CALC_TIMEOUT     (10 * 1000) // initial timeout == 10 secs
#define CMS_MAX_TIMEOUT      (40 * 1000) // Max timeout == 40 secs

#define xwParam(x,y) ((wParam >=x) && (wParam <=y))

#define RSHF        7

/* Error values.                                                          */
#define SCERR_DIVIDEZERO    0
#define SCERR_DOMAIN        1
#define SCERR_UNDEFINED     2
#define SCERR_POS_INFINITY  3
#define SCERR_NEG_INFINITY  4
#define SCERR_ABORTED       5


/* F_INTMATH()  returns TRUE if math should be intiger mode               */
//
// Do int math if we are not in base ten
//
#define F_INTMATH() (nRadix != 10)

////////////////////////////////////////////////////////////////////////////
//
// Function prototypes.
//
////////////////////////////////////////////////////////////////////////////

/* Exports.                                                               */
LRESULT APIENTRY CalcWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY StatBoxProc(HWND, UINT, WPARAM, LPARAM);

/* Functions called from several modules.  Must be FAR.                   */
VOID    APIENTRY DisplayError (INT);
VOID    APIENTRY EnableToggles(BOOL bEnable);
VOID    APIENTRY InitSciCalc (BOOL);
VOID    APIENTRY MenuFunctions(DWORD);
VOID    APIENTRY SciCalcFunctions (PHNUMOBJ phnoNum, DWORD wOp);
VOID    APIENTRY SetStat (BOOL);
VOID    APIENTRY StatFunctions (WPARAM);

VOID   DisplayNum (VOID);

/* Internal near calls.                                                   */
void        DoOperation (INT   nOperation, HNUMOBJ *phnoNum, HNUMOBJ hnoX);

VOID   NEAR ProcessCommands(WPARAM);
VOID   NEAR SetBox (int, BOOL);
VOID   NEAR SetRadix (DWORD);
LONG   NEAR StatAlloc (WORD, DWORD);
VOID   NEAR StatError (VOID);

void   SwitchModes(DWORD wRadix, int nDecMode, int nHexMode);

void RecalcNumObjConstants(void);
BOOL SetWaitCursor( BOOL fOn );

void KillTimeCalc( void );
void TimeCalc( BOOL fStart );

// these functions are from SciKeys.c and are used to access data stored 
// in the key array
COLORREF   GetKeyColor( int iID );
ULONG_PTR  GetHelpID( int iID );

#define  INDEXFROMID( x )    (x-IDC_FIRSTCONTROL)


////////////////////////////////////////////////////////////////////////////
//
// Global Variables
//
////////////////////////////////////////////////////////////////////////////

extern HWND         g_hwndDlg;
extern HINSTANCE    hInst;
extern ANGLE_TYPE   nDecMode;

extern long nRadix;
extern long nPrecision;
extern long dwWordBitWidth;

extern BOOL     bInv;
extern BOOL     bHyp;

extern int      nCalc;
extern int      nHexMode;

extern HNUMOBJ  g_ahnoChopNumbers[];

extern BOOL     bFarEast;
