/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   status.c: Status bar window
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
//#include <win32.h>
#include <mmsystem.h>
#include "status.h"

/* for compiling under win3.0 - we don't have this attribute */
#ifndef COLOR_BTNHIGHLIGHT
#define COLOR_BTNHIGHLIGHT 20
#endif

#ifdef _WIN32
typedef WNDPROC LPWNDPROC;
#else
typedef long (FAR PASCAL *LPWNDPROC)();
#endif



// class names for status bar and static text windows
TCHAR	szStatusClass[] = "StatusClass";
TCHAR	szText[]   = "SText";
int gStatusStdHeight;   // based on font metrics

static HBRUSH ghbrBackground;
static HANDLE ghFont;
static HBRUSH ghbrHL, ghbrShadow;


/* Function Prototypes */
LRESULT FAR PASCAL statusWndProc(HWND hwnd, unsigned msg,
						WPARAM wParam, LPARAM lParam);
LRESULT FAR PASCAL fnText(HWND, unsigned, WPARAM, LPARAM);
static VOID NEAR PASCAL PaintText(HWND hwnd, HDC hdc);




/*
 * create the brushes we need
 */
void
statusCreateTools(void)
{
    HDC hdc;
    TEXTMETRIC tm;
    HFONT hfont;

    ghbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    ghbrHL = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
    ghbrShadow = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));

    /* Create the font we'll use for the status bar - use system as default */
    ghFont = CreateFont(12, 0,		// height, width
                0, 0,			// escapement, orientation
                FW_NORMAL,		// weight,
                FALSE, FALSE, FALSE,	// attributes
                ANSI_CHARSET,		// charset
                OUT_DEFAULT_PRECIS,	// output precision
                CLIP_DEFAULT_PRECIS,	// clip precision
                DEFAULT_QUALITY,	// quality
                VARIABLE_PITCH | FF_MODERN,
                "Helv");

    if (ghFont == NULL) {
        ghFont = GetStockObject(SYSTEM_FONT);
    }

    // find the char size to calc standard status bar height
    hdc = GetDC(NULL);
    hfont = SelectObject(hdc, ghFont);
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, hfont);
    ReleaseDC(NULL, hdc);

    gStatusStdHeight = tm.tmHeight * 3 / 2;

}

void
statusDeleteTools(void)
{
    DeleteObject(ghbrBackground);
    DeleteObject(ghbrHL);
    DeleteObject(ghbrShadow);

    DeleteObject(ghFont);
}




/*--------------------------------------------------------------+
| statusInit - initialize for status window, register the	|
|	       Window's class.					|
|								|
+--------------------------------------------------------------*/
#pragma alloc_text(INIT_TEXT, statusInit)
BOOL statusInit(HANDLE hInst, HANDLE hPrev)
{
  WNDCLASS  cls;

  statusCreateTools();

  if (!hPrev){
	  cls.hCursor		= LoadCursor(NULL, IDC_ARROW);
	  cls.hIcon		= NULL;
	  cls.lpszMenuName	= NULL;
	  cls.lpszClassName	= szStatusClass;
	  cls.hbrBackground	= ghbrBackground;
	  cls.hInstance		= hInst;
	  cls.style		= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	  cls.lpfnWndProc	= statusWndProc;
	  cls.cbClsExtra	= 0;
	  cls.cbWndExtra	= 0;
	
	  if (!RegisterClass(&cls))
		  return FALSE;

	  cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
	  cls.hIcon          = NULL;
	  cls.lpszMenuName   = NULL;
	  cls.lpszClassName  = (LPSTR)szText;
	  cls.hbrBackground  = ghbrBackground;
	  cls.hInstance      = hInst;
	  cls.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	  cls.lpfnWndProc    = (LPWNDPROC)fnText;
	  cls.cbClsExtra     = 0;
	  cls.cbWndExtra     = 0;
	  if (!RegisterClass(&cls))
		return FALSE;
  }


  return TRUE;
}

/*
 * returns the recommended height for a status bar based on the
 * character dimensions of the font used
 */
int
statusGetHeight(void)
{
    return(gStatusStdHeight);
}


/*--------------------------------------------------------------+
| statusUpdateStatus - update the status line			|
|								|
| The argument can either be NULL, a string, or a resource ID	|
| cast to a LPCSTR with MAKEINTRESOURCE.			|
+--------------------------------------------------------------*/
void statusUpdateStatus(HWND hwnd, LPCTSTR lpsz)
{
    TCHAR	ach[80];
    HWND hwndtext;

    if ((lpsz != NULL) && (HIWORD((DWORD) (DWORD_PTR) lpsz) == 0)) {
	LoadString(GetWindowInstance(hwnd), LOWORD((DWORD) (DWORD_PTR) lpsz), ach, sizeof(ach));
	lpsz = ach;
    }

    hwndtext = GetDlgItem(hwnd, 1);
    if (!lpsz || *lpsz == '\0') {
	SetWindowText(hwndtext,"");
    } else {
	SetWindowText(hwndtext, lpsz);
    }
}

/*--------------------------------------------------------------+
| statusWndProc - window proc for status window			|
|								|
+--------------------------------------------------------------*/
LRESULT FAR PASCAL 
statusWndProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT	ps;
  HDC		hdc;
  HWND          hwndSText;

  switch(msg){
    case WM_CREATE:
	
	    /* we need to create the static text control for the status bar */
	    hwndSText = CreateWindow(
                            szText,
                            "",
                            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		            0, 0, 0, 0,
                            hwnd,
                            (HMENU) 1,  // child id
                            GetWindowInstance(hwnd),
                            NULL);

	    if (!hwndSText) {
		    return -1;
            }
	    break;
	
    case WM_DESTROY:
            statusDeleteTools();
	    break;
	
    case WM_SIZE:
        {
            RECT rc;

            GetClientRect(hwnd, &rc);

            MoveWindow(
                GetDlgItem(hwnd, 1),    // get child window handle
                2, 1,                   // xy just inside
                rc.right - 4,
                rc.bottom - 2,
                TRUE);

	    break;
        }

    case WM_PAINT:
	    hdc = BeginPaint(hwnd, &ps);

            // only the background and the child window need painting

	    EndPaint(hwnd, &ps);
	    break;

    case WM_SYSCOLORCHANGE:
        statusDeleteTools();
        statusCreateTools();
#ifdef _WIN32
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR) ghbrBackground);
#else
        SetClassWord(hwnd, GCW_HBRBACKGROUND, (WORD) ghbrBackground);
#endif
        break;

    case WM_ERASEBKGND:
        break;

  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*
 * window proc for static text window
 */
LRESULT FAR PASCAL 
fnText(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;

	switch (msg) {
	case WM_SETTEXT:
		DefWindowProc(hwnd, msg, wParam, lParam);
		InvalidateRect(hwnd,NULL,FALSE);
		UpdateWindow(hwnd);
		return 0L;

	case WM_ERASEBKGND:
		return 0L;

	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		PaintText(hwnd, ps.hdc);
		EndPaint(hwnd, &ps);
		return 0L;
        }

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*--------------------------------------------------------------+
| PaintText - paint the shadowed static text field.		|
|								|
+--------------------------------------------------------------*/
VOID NEAR PASCAL
PaintText(HWND hwnd, HDC hdc)
{
  RECT rc;
  TCHAR  ach[128];
  int  len;
  int	dx, dy;
  RECT	rcFill;
  HFONT	hfontOld;
  HBRUSH hbrSave;

  GetClientRect(hwnd, &rc);

  len = GetWindowText(hwnd,ach,sizeof(ach));

  SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
  SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

  hfontOld = SelectObject(hdc, ghFont);

  rcFill.left = rc.left + 1;
  rcFill.right = rc.right - 1;
  rcFill.top = rc.top + 1;
  rcFill.bottom = rc.bottom - 1;

  /* move in some and do background and text in one swoosh */
  ExtTextOut(hdc,4,1,ETO_OPAQUE,&rcFill,ach,len,NULL);

  dx = rc.right - rc.left;
  dy = rc.bottom - rc.top;

  hbrSave = SelectObject(hdc, ghbrShadow);
  PatBlt(hdc, rc.left, rc.top, 1, dy, PATCOPY);
  PatBlt(hdc, rc.left, rc.top, dx, 1, PATCOPY);

  SelectObject(hdc, ghbrHL);
  PatBlt(hdc, rc.right-1, rc.top+1, 1, dy-1, PATCOPY);
  PatBlt(hdc, rc.left+1, rc.bottom -1, dx-1, 1,  PATCOPY);

  if (hfontOld)
	  SelectObject(hdc, hfontOld);
  if (hbrSave)
	  SelectObject(hdc, hbrSave);
  return ;
}
