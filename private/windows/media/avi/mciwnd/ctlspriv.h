//#define WINVER 0x400
#define _3DSTUFF

#define BUILDDLL

#ifndef STRICT
#define STRICT
#endif

/* disable "non-standard extension" warnings in our code
 */
#ifndef RC_INVOKED
#if 0
#pragma warning(disable:4001)
#endif
#endif

#include <windows.h>

#ifdef WIN32
#include <win32.h>
#else // WIN32
#define GETWINDOWID(hwnd)		GetWindowWord(hwnd, GWW_ID)
#endif

#define NOUPDOWN
#define NOSTATUSBAR
#define NOMENUHELP
#define NOBTNLIST
#define NODRAGLIST
#define NOPROGRESS
#include "commctrl.h"

#ifdef WIN32
#define SETWINDOWPOINTER(hwnd, name, p)	SetWindowLong(hwnd, 0, (LONG)p)
#define GETWINDOWPOINTER(hwnd, name)	((name)GetWindowLong(hwnd, 0))
#else // WIN32
#define SETWINDOWPOINTER(hwnd, name, p)	SetWindowWord(hwnd, 0, (WORD)p)
#define GETWINDOWPOINTER(hwnd, name)	((name)GetWindowWord(hwnd, 0))
#endif
#define ALLOCWINDOWPOINTER(name, size)	((name)LocalAlloc(LPTR, size))
#define FREEWINDOWPOINTER(p)		LocalFree((HLOCAL)p)

BOOL    WINAPI MyGetPrivateProfileStruct(LPCTSTR, LPCTSTR, LPVOID, UINT, LPCTSTR);
BOOL    WINAPI MyWritePrivateProfileStruct(LPCTSTR, LPCTSTR, LPVOID, UINT, LPCTSTR);


extern HINSTANCE hInst;

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitStatusClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitHeaderClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitButtonListBoxClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance);
BOOL FAR PASCAL InitUpDownClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitProgressClass(HINSTANCE hInstance);

void FAR PASCAL NewSize(HWND hWnd, int nClientHeight, LONG style,
      int left, int top, int width, int height);

#define IDS_SPACE	0x0400

/* System MenuHelp
 */
#define MH_SYSMENU	(0x8000 - MINSYSCOMMAND)
#define IDS_SYSMENU	(MH_SYSMENU-16)
#define IDS_HEADER	(MH_SYSMENU-15)
#define IDS_HEADERADJ	(MH_SYSMENU-14)
#define IDS_TOOLBARADJ	(MH_SYSMENU-13)

/* Cursor ID's
 */
#define IDC_SPLIT	100
#define IDC_MOVEBUTTON	102

#define IDC_STOP	103
#define IDC_COPY	104
#define IDC_MOVE	105

/* Icon ID's
 */
#define IDI_INSERT	150

/* AdjustDlgProc stuff
 */
#define ADJUSTDLG	200
#define IDC_BUTTONLIST	201
#define IDC_RESET	202
#define IDC_CURRENT	203
#define IDC_REMOVE	204
#define IDC_HELP	205
#define IDC_MOVEUP	206
#define IDC_MOVEDOWN	207

/* bitmap IDs
 */

#define IDB_THUMB       300

/* These are the internal structures used for a status bar.  The header
 * bar code needs this also
 */
typedef struct tagSTRINGINFO
  {
    DWORD dwString;
    UINT uType;
    int right;
  } STRINGINFO, *PSTRINGINFO;

typedef struct tagSTATUSINFO
  {
    HFONT hStatFont;
    BOOL bDefFont;

    int nFontHeight;
    int nMinHeight;
    int nBorderX, nBorderY, nBorderPart;

    STRINGINFO sSimple;

    int nParts;
    STRINGINFO sInfo[1];
  } STATUSINFO, *PSTATUSINFO;

#define SBT_NOSIMPLE	0x00ff	/* Flags to indicate normal status bar */

/* This is the default status bar face name
 */
extern TCHAR szSansSerif[];

/* Note that window procedures in protect mode only DLL's may be called
 * directly.
 */
void FAR PASCAL PaintStatusWnd(HWND hWnd, PSTATUSINFO pStatusInfo,
      PSTRINGINFO pStringInfo, int nParts, int nBorderX, BOOL bHeader);
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT uMessage, WPARAM wParam,
      LPARAM lParam);

/* toolbar.c */

typedef struct tagTBBMINFO {		/* info for recreating the bitmaps */
    int nButtons;
    HINSTANCE hInst;
    WORD wID;
    HBITMAP hbm;
} TBBMINFO, NEAR *PTBBMINFO;

typedef struct tagTBSTATE {		/* instance data for toolbar window */
    PTBBUTTON pCaptureButton;
    HWND hdlgCust;
    HWND hwndCommand;
    int nBitmaps;
    PTBBMINFO pBitmaps;
    HBITMAP hbmCache;
    PTSTR *pStrings;
    int nStrings;
    UINT uStructSize;
    int iDxBitmap;
    int iDyBitmap;
    int iButWidth;
    int iButHeight;
    int iYPos;
    int iBarHeight;
    int iNumButtons;
    int nSysColorChanges;
    WORD wButtonType;
    TBBUTTON Buttons[1];
} TBSTATE, NEAR *PTBSTATE;

typedef struct tagOLDTBBUTTON
{
/*REVIEW: index, command, flag words, resource ids should be UINT */
    int iBitmap;	/* index into bitmap of this button's picture */
    int idCommand;	/* WM_COMMAND menu ID that this button sends */
    BYTE fsState;	/* button's state */
    BYTE fsStyle;	/* button's style */
    int idsHelp;	/* string ID for button's status bar help */
} OLDTBBUTTON;
typedef OLDTBBUTTON FAR* LPOLDTBBUTTON;

static HBITMAP FAR PASCAL SelectBM(HDC hDC, PTBSTATE pTBState, int nButton);
static void FAR PASCAL DrawButton(HDC hdc, int x, int y, int dx, int dy,PTBSTATE pTBState, PTBBUTTON ptButton, BOOL bCache);
static int  FAR PASCAL TBHitTest(PTBSTATE pTBState, int xPos, int yPos);
static int  FAR PASCAL PositionFromID(PTBSTATE pTBState, int id);
static void FAR PASCAL BuildButtonTemplates(void);
static void FAR PASCAL TBInputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt);
static void FAR PASCAL TBOutputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt);

/* tbcust.c */
extern BOOL FAR PASCAL SaveRestore(HWND hWnd, PTBSTATE pTBState, BOOL bWrite,
      LPTSTR FAR *lpNames);
extern void FAR PASCAL CustomizeTB(HWND hWnd, PTBSTATE pTBState, int iPos);
extern void FAR PASCAL MoveButton(HWND hwndToolbar, PTBSTATE pTBState,
      int nSource);

/* cutils.c */
void FAR PASCAL NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height);
BOOL FAR PASCAL CreateDitherBrush(BOOL bIgnoreCount);	/* creates hbrDither */
BOOL FAR PASCAL FreeDitherBrush(void);
void FAR PASCAL CreateThumb(BOOL bIgnoreCount);
void FAR PASCAL DestroyThumb(void);
void FAR PASCAL CheckSysColors(void);

extern HBRUSH hbrDither;
extern HBITMAP hbmThumb;
extern int nSysColorChanges;
extern DWORD rgbFace;			// globals used a lot
extern DWORD rgbShadow;
extern DWORD rgbHilight;
extern DWORD rgbFrame;
