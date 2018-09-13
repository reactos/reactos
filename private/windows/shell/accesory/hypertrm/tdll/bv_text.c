/*	File: C:\WACKER\TDLL\BV_TEXT.C (Created: 11-Jan-1994)
 *	Created from:
 *	File: C:\HA5G\ha5g\stxtproc.c (Created: 27-SEP-1991)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:37p $
 */

// #define	DEBUGSTR	1

#define	WE_DRAW_EDGE	1

#include <windows.h>
#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>

#include <term\xfer_dlg.h>

#include "bv_text.h"
#include "bv_text.hh"

/*
 * This stuff can probably go away as soon as the two following styles are
 * supported:
 *
 *	SS_SUNKEN	0x00001000L
 *	SS_RAISED	0x00002000L
 */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	RegisterBeveledTextClass
 *
 * DESCRIPTION:
 *	Registers the VU Meter window class.  (No kidding!)
 *
 * PARAMETERS:
 *	Hinstance -- the instance handle
 *
 * RETURNS:
 *	Whatever RegisterClass returns.
 */
BOOL RegisterBeveledTextClass(HANDLE hInstance)
	{
	BOOL            bRetVal = TRUE;
	WNDCLASS        wndclass;

	if (bRetVal)
		{
		wndclass.style          = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc    = BeveledTextWndProc;
		wndclass.cbClsExtra     = 0;
		wndclass.cbWndExtra     = sizeof(VOID FAR *);
		wndclass.hIcon          = NULL;
		wndclass.hInstance      = hInstance;
		wndclass.hCursor        = NULL;
		wndclass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wndclass.lpszMenuName   = NULL;
		wndclass.lpszClassName  = BV_TEXT_CLASS;

		bRetVal = RegisterClass(&wndclass);
		}

	return bRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: stxtDrawBeveledText
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 *
 */

VOID stxtDrawBeveledText(
						 HDC		hdc,
						 HFONT		hNewFont,
						 LPRECT 	lpRc,
						 USHORT 	usWidth,
						 DWORD		dwStyle,
						 ULONG FAR *pulColors,
						 LPTSTR		pszText
						)
	{
	//ULONG  ulOldBkColor;
	//ULONG  ulOldTextColor;
#if defined(WE_DRAW_EDGE)
	HBRUSH hBrush;
#endif
	HFONT  hFont;
	//WORD	 wFlags;
	WORD   wStyle;
	INT    nWidth;
	INT    nHeight;
	INT    nIndex;
	RECT   rcE;
	TEXTMETRIC tm;
	LPTSTR  lpstr;

	/*
	 * We redraw the edging completely, every time
	 */
	nHeight = (int)GetSystemMetrics(SM_CYBORDER) * (int)usWidth;
	nWidth	= (int)GetSystemMetrics(SM_CXBORDER) * (int)usWidth;

#if defined(WE_DRAW_EDGE)
	/* Draw the top edge */
	hBrush = CreateSolidBrush(pulColors[2]);
	for (nIndex = 0; nIndex < nHeight; nIndex += 1)
		{
		rcE = *lpRc;
		rcE.top = nIndex;
		rcE.bottom = nIndex + 1;
		rcE.right -= nIndex;
		FillRect(hdc, &rcE, hBrush);
		}
	/* Draw the left edge */
	for (nIndex = 0; nIndex < nWidth; nIndex += 1)
		{
		rcE = *lpRc;
		rcE.left = nIndex;
		rcE.right = nIndex + 1;
		rcE.bottom -= nIndex;
		FillRect(hdc, &rcE, hBrush);
		}
	DeleteObject(hBrush);
	/* Draw the bottom edge */
	hBrush = CreateSolidBrush(pulColors[3]);
	for (nIndex = 0; nIndex < nHeight; nIndex += 1)
		{
		rcE = *lpRc;
		rcE.top = rcE.bottom - nIndex - 1;
		rcE.bottom = rcE.bottom - nIndex;
		rcE.left += nIndex + 1;
		FillRect(hdc, &rcE, hBrush);
		}
	/* Draw the right edge */
	for (nIndex = 0; nIndex < nWidth; nIndex += 1)
		{
		rcE = *lpRc;
		rcE.left = rcE.right - nIndex - 1;
		rcE.right = rcE.right - nIndex;
		rcE.top += nIndex + 1;
		FillRect(hdc, &rcE, hBrush);
		}
	DeleteObject(hBrush);
#else

	DrawEdge(hdc, lpRc,
			 EDGE_SUNKEN,
			 BF_SOFT | BF_RECT);

#endif

	/*
	 * We redraw the text completely every time
	 */

	rcE = *lpRc;
#if defined(WE_DRAW_EDGE)
	InflateRect(&rcE, -nWidth, -nHeight);
#else
	InflateRect(&rcE, -2, -2);
#endif

	/* -------------- Must have something to paint ------------- */

	if (pszText)
		lpstr = pszText;

	else
		lpstr = " ";

	/* -------------- Figure where to place it ------------- */

	nIndex = (int)lstrlen(lpstr);
	wStyle = (WORD)dwStyle;

	if (wStyle & SS_RIGHT)
		{
		SetTextAlign(hdc, TA_RIGHT);
		nWidth = rcE.right - (2 * nWidth);
		}

	else if (wStyle & SS_CENTER)
		{
		SetTextAlign(hdc, TA_CENTER);
		nWidth = ((rcE.right - rcE.left) / 2) + nWidth;
		}

	else
		{
		nWidth = 3 * nWidth;
		}


	SetBkColor(hdc, pulColors[0]);
	SetTextColor(hdc, pulColors[1]);

	if (hNewFont != (HFONT)0)
		hFont = SelectObject(hdc, hNewFont);

	GetTextMetrics(hdc, &tm);
	nHeight += ((rcE.bottom - rcE.top) - tm.tmHeight - 1) / 2;

	ExtTextOut(hdc, nWidth, nHeight, ETO_OPAQUE | ETO_CLIPPED, &rcE,
		lpstr, nIndex, (LPINT)0);

	if (hNewFont != (HFONT)0)
		SelectObject(hdc, hFont);
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: StaticTextWndProc
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 *
 */

LONG CALLBACK BeveledTextWndProc(HWND hWnd,
								  UINT wMsg,
								  WPARAM wPar,
								  LPARAM lPar)
	{
	LPSTEXT psT;

	switch (wMsg)
		{
		case WM_CREATE:
			psT = (LPSTEXT)malloc(sizeof(STEXT));
			SetWindowLong(hWnd, 0, (LONG)psT);

			psT->ulCheck = STEXT_VALID;
			psT->pszText = NULL;
			psT->hFont	 = (HFONT)0;
			psT->fpOwnerDraw = (STXT_OWNERDRAW)0;

			switch (GetWindowLong(hWnd, GWL_STYLE) & 0x0000FF00)
				{
				case 0:
				default:
					psT->cBackGround = GetSysColor(COLOR_BTNFACE);
					psT->cTextColor  = GetSysColor(COLOR_BTNTEXT);
					psT->cUpperEdge  = GetSysColor(COLOR_BTNSHADOW);
					psT->cLowerEdge = GetSysColor(COLOR_BTNHIGHLIGHT);

					break;

				case BVS_ALTCLR:
				// case 0x100:
					psT->cBackGround = 0x00000000;
					psT->cTextColor  = 0x0000FF00;
					psT->cUpperEdge  = GetSysColor(COLOR_BTNSHADOW);
					psT->cLowerEdge  = GetSysColor(COLOR_BTNHIGHLIGHT);

					break;
				}

			psT->usDepth	 = STXT_DEF_DEPTH;

			break;

		case WM_DESTROY:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);

			if (STEXT_OK(psT))
				{
				if (psT->pszText != NULL)
					free(psT->pszText);

				if (psT->fpOwnerDraw)
					FreeProcInstance((FARPROC)psT->fpOwnerDraw);

				free(psT);
				}
			SetWindowLong(hWnd, 0, 0L);
			break;

		case WM_GETDLGCODE:
			/* Static controls don't want any of these */
			return DLGC_STATIC;

		case WM_GETTEXTLENGTH:
			{
			ULONG ulLength = 0;

			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				if (psT->pszText != NULL)
					ulLength = (ULONG)lstrlen(psT->pszText);
				}
			return ((LONG)ulLength);
			}

		case WM_GETTEXT:
			{
			WORD wLength = 0;

			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				if (psT->pszText != NULL)
					{
					wLength = (WORD)lstrlen(psT->pszText);
					if (wPar < wLength)
						wLength = wPar;
					memcpy((LPTSTR)lPar, psT->pszText, wLength);
					}
				}
			return ((LONG)wLength);
			}

		case WM_SETTEXT:
			{
			WORD wLength;
			LPTSTR pszStr;

			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				CHAR  ach[128];
				INT   i, len;
				SIZE  sz;
				RECT  rc;
				HDC   hDC;
				HFONT hFont;
				DWORD dwStyle;

				ach[0] = 0;
				if (psT->pszText != NULL)
					{
					lstrcpy(ach, psT->pszText);
					free(psT->pszText);
					}
				psT->pszText = NULL;

				pszStr = (LPTSTR)lPar;
				if (pszStr != NULL)
					{
					wLength = (WORD)lstrlen(pszStr);
					if (wLength > 0)
						{
						psT->pszText = (LPTSTR)malloc(wLength + 1);
						lstrcpy(psT->pszText, pszStr);

						dwStyle = (DWORD)GetWindowLong(hWnd, GWL_STYLE);
						GetClientRect(hWnd, &rc);

						// Microsoft defined SS_LEFT to be zero!  
						// So the only way I can test SS_LEFT is
						// to see if bit 1 is zero

						switch (dwStyle & 3)
							{
							default:
								break;

							case SS_LEFT:
								{
								// Invalidate only from the point where the
								// string has changed.

								for (i=0, len=lstrlen(ach) ; i < len  ; ++i)
									{
									if (ach[i] != *pszStr++)
										break;
									}

								ach[i] = '\0';

								hDC = GetDC(hWnd);
								hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L);
								if (hFont)
									hFont = SelectObject(hDC, hFont);
								GetTextExtentPoint(hDC, ach, i, &sz);
								if (hFont)
									SelectObject(hDC, hFont);
								ReleaseDC(hWnd, hDC);

								rc.left += sz.cx;

								/*
								 * We fudge because the font isn't fixed pitch
								 */
								rc.left -= min(sz.cx, 8);
								}
								break;

							case SS_RIGHT:
								{
								// Invalidate to the longer of the two strings.

								hDC = GetDC(hWnd);
								hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L);
								if (hFont)
									hFont = SelectObject(hDC, hFont);
								GetTextExtentPoint(hDC, pszStr, lstrlen(pszStr), &sz);
								len = sz.cx;
								GetTextExtentPoint(hDC, ach, lstrlen(ach), &sz);
								if (hFont)
									SelectObject(hDC, hFont);
								ReleaseDC(hWnd, hDC);

								// rc.left += max(0, rc.right - ((int)GetSystemMetrics(SM_CXBORDER) * (int)psT->usDepth) - max(len, sz.cx) - 1 - 8);
								}
								break;
							}

						InvalidateRect(hWnd, &rc, FALSE);
						}
					}
				}
			}
			break;

		case WM_PAINT:
			{
			RECT rcC;
			PAINTSTRUCT ps;

			BeginPaint(hWnd, &ps);

			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				GetClientRect(hWnd, &rcC);

				stxtDrawBeveledText(
									ps.hdc,
									psT->hFont,
									(LPRECT)&rcC,
									psT->usDepth,
									(DWORD)GetWindowLong(hWnd, GWL_STYLE),
									(ULONG FAR *)&psT->cBackGround,
									psT->pszText
									);

				if (psT->fpOwnerDraw)
					(*psT->fpOwnerDraw)(hWnd, ps.hdc);
				}

			EndPaint(hWnd, &ps);
			}
			break;

		case WM_STXT_SET_BK:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				psT->cBackGround = (ULONG)lPar;
				InvalidateRect(hWnd, NULL, FALSE);
				}
			break;

		case WM_STXT_SET_TXT:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				psT->cTextColor = (ULONG)lPar;
				InvalidateRect(hWnd, NULL, FALSE);
				}
			break;

		case WM_STXT_SET_UE:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				psT->cUpperEdge = (ULONG)lPar;
				InvalidateRect(hWnd, NULL, FALSE);
				}
			break;

		case WM_STXT_SET_LE:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				psT->cLowerEdge = (ULONG)lPar;
				InvalidateRect(hWnd, NULL, FALSE);
				}
			break;

		case WM_STXT_SET_DEPTH:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);
			if (STEXT_OK(psT))
				{
				if (wPar < 7)
					{
					psT->usDepth = wPar;
					InvalidateRect(hWnd, NULL, TRUE);
					}
				}
			break;

		case WM_STXT_OWNERDRAW:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);

			psT->fpOwnerDraw = (STXT_OWNERDRAW)lPar;
			break;

		case WM_SETFONT:
			psT = (LPSTEXT)GetWindowLong(hWnd, 0);

			if (STEXT_OK(psT))
				psT->hFont = (HFONT)wPar;

			return 0L;

		default:
			break;
		}

	return DefWindowProc(hWnd, wMsg, wPar, (LONG)lPar);
	}
