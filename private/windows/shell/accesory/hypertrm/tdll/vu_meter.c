/*  File: \WACKER\TDLL\VU_METER.C (Created: 10-JAN-1994)
 *	Created from:
 *	File: C:\HA5G\ha5g\stxtproc.c (Created: 27-SEP-1991)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#include <windows.h>
#pragma hdrstop

#define WE_DRAW_EDGE	1

#include <tdll\stdtyp.h>
#include <tdll\mc.h>
#include <tdll\assert.h>

#include <term\xfer_dlg.h>

#include "vu_meter.h"
#include "vu_meter.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	RegisterVuMeterClass
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
BOOL RegisterVuMeterClass(HANDLE hInstance)
	{
	BOOL            bRetVal = TRUE;
	WNDCLASS        wndclass;

	if (bRetVal)
		{
		wndclass.style          = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc    = VuMeterWndProc;
		wndclass.cbClsExtra     = 0;
		wndclass.cbWndExtra     = sizeof(VOID FAR *);
		wndclass.hIcon          = NULL;
		wndclass.hInstance      = hInstance;
		wndclass.hCursor        = NULL;
		wndclass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wndclass.lpszMenuName   = NULL;
		wndclass.lpszClassName  = VU_METER_CLASS;

		bRetVal = RegisterClass(&wndclass);
		}

	return bRetVal;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	VuMeterWndProc
 *
 * DESCRIPTION:
 *	Window procedure for implementing our own internal VU Meter.
 *
 * ARGUEMENTS:
 *	The standard window proc stuff.
 *
 * RETURNS:
 *
 */
LRESULT CALLBACK VuMeterWndProc(HWND hWnd,
	     	    			   UINT wMsg,
		    				   WPARAM wPar,
							   LPARAM lPar)
	{
	LPVM	psV;

	switch (wMsg)
		{
		case WM_CREATE:
			psV = (LPVM)malloc(sizeof(VUMETER));
			SetWindowLongPtr(hWnd, 0, (LONG_PTR)psV);

			psV->ulCheck = VUMETER_VALID;

#if FALSE
			/* Old colors */
			psV->cBackGround  = 0x00000000;
			psV->cFillColor   = 0x0000FF00;
			psV->cRefillColor = 0x00800080;
			psV->cMarkColor   = 0x0000FFFF;

			psV->cUpperEdge   = 0x00808080;
			psV->cLowerEdge   = 0x00FFFFFF;

#endif
			psV->cBackGround  = GetSysColor(COLOR_3DFACE);
			psV->cFillColor   = GetSysColor(COLOR_3DDKSHADOW);
			psV->cRefillColor = 0x00800080;		/* Unused */
			psV->cMarkColor   = GetSysColor(COLOR_3DSHADOW);

			psV->cUpperEdge   = GetSysColor(COLOR_3DSHADOW);
			psV->cLowerEdge   = GetSysColor(COLOR_3DHILIGHT);

			psV->ulMaxRange   = 0;
			psV->ulHiValue	  = 0;
			psV->ulCurValue   = 0;

			psV->usDepth	  = STXT_DEF_DEPTH;

			break;

		case WM_DESTROY:
			psV = (LPVM)GetWindowLongPtr(hWnd, 0);
			if (VUMETER_OK(psV))
				{
				free(psV);
				psV = NULL;
				}
			SetWindowLongPtr(hWnd, 0, 0L);
			break;

		case WM_GETDLGCODE:
			/* Static controls don't want any of these */
			return DLGC_STATIC;

		case WM_PAINT:
			{
			int nEnd;
			int nIndex;
			int nFill;
			int nReFill;
#if defined(WE_DRAW_EDGE)
			int nHeight;
			int nWidth;
#endif
			int nStep;
			RECT rcC;
			RECT rcE;
			RECT rcD;
			PAINTSTRUCT ps;
			HBRUSH hBrush;

			BeginPaint(hWnd, &ps);

			psV = (LPVM)GetWindowLongPtr(hWnd, 0);
			if (VUMETER_OK(psV))
				{
				/*
				 * We erase/fill in the invalid region
				 */
				hBrush = CreateSolidBrush(psV->cBackGround);
				FillRect(ps.hdc, &ps.rcPaint, hBrush);
				DeleteObject(hBrush);

#if defined(WE_DRAW_EDGE)
				/*
				 * We redraw the edging completely, every time
				 */
				nHeight = (int)GetSystemMetrics(SM_CYBORDER) * (int)psV->usDepth;
				nWidth	= (int)GetSystemMetrics(SM_CXBORDER) * (int)psV->usDepth;

				GetClientRect(hWnd, &rcC);

				/* Draw the top edge */
				hBrush = CreateSolidBrush(psV->cUpperEdge);
				for (nIndex = 0; nIndex < nHeight; nIndex += 1)
					{
					rcE = rcC;
					rcE.top = nIndex;
					rcE.bottom = nIndex + 1;
					rcE.right -= nIndex;
					FillRect(ps.hdc, &rcE, hBrush);
					}
				/* Draw the left edge */
				for (nIndex = 0; nIndex < nWidth; nIndex += 1)
					{
					rcE = rcC;
					rcE.left = nIndex;
					rcE.right = nIndex + 1;
					rcE.bottom -= nIndex;
					FillRect(ps.hdc, &rcE, hBrush);
					}
				DeleteObject(hBrush);
				/* Draw the bottom edge */
				hBrush = CreateSolidBrush(psV->cLowerEdge);
				for (nIndex = 0; nIndex < nHeight; nIndex += 1)
					{
					rcE = rcC;
					rcE.top = rcE.bottom - nIndex - 1;
					rcE.bottom = rcE.bottom - nIndex;
					rcE.left += nIndex + 1;
					FillRect(ps.hdc, &rcE, hBrush);
					}
				/* Draw the right edge */
				for (nIndex = 0; nIndex < nWidth; nIndex += 1)
					{
					rcE = rcC;
					rcE.left = rcE.right - nIndex - 1;
					rcE.right = rcE.right - nIndex;
					rcE.top += nIndex + 1;
					FillRect(ps.hdc, &rcE, hBrush);
					}
				DeleteObject(hBrush);
#else

				DrawEdge(ps.hdc, &ps.rcPaint,
						 EDGE_SUNKEN,
						 BF_SOFT | BF_RECT);

#endif

				/*
				 * Try and fill in the progress.  This is done by building
				 * a loop the would rewrite all of the lighted 'pixels' that
				 * would normally be light.  This is then modified by checking
				 * the intersection of the lighted region and the invalid area.
				 * If they interset, do the write, otherwise skip it.
				 */
				if ((psV->ulMaxRange > 0) &&
					(psV->ulCurValue > 0) &&
					(psV->ulHiValue > 0))
					{
					HBRUSH hbFill;
					HBRUSH hbRefill;

					/* These lines must match those in the WM_VU_SETCURVALUE */
					GetClientRect(hWnd, &rcC);
					InflateRect(&rcC,
								-((INT)psV->usDepth),
								-((INT)psV->usDepth));
					nEnd = rcC.right - rcC.left;

					nFill = (int)(((ULONG)nEnd * psV->ulCurValue) /
												psV->ulMaxRange);
					nReFill = (int)(((ULONG)nEnd * psV->ulHiValue) /
												psV->ulMaxRange);
					
					/* Build simulated LEDs */
					hbFill = CreateSolidBrush(psV->cFillColor);
					hbRefill = CreateSolidBrush(psV->cMarkColor);

					rcE = rcC;
					rcE.right = rcE.left + ((rcE.bottom - rcE.top) / 2);
					InflateRect(&rcE,
								(int)(rcE.left - rcE.right) / 3,
								(int)(rcE.top - rcE.bottom) / 3);
					nStep = rcE.right - rcE.left;
					nStep *= 2;

					/*
					 * TODO: figure out how to set the beginning and end of
					 * this loop to eliminate dead cycles
					 */
					for (nIndex = 0; nIndex < nEnd; nIndex += nStep)
						{
						if (IntersectRect(&rcD, &rcE, &ps.rcPaint) != 0)
							{
							hBrush = hbFill;
							if (nIndex > nFill)
								{
								hBrush = hbRefill;
								}
							if (nIndex > nReFill)
								{
								hBrush = NULL;
								}
							if (hBrush != NULL)
								{
								FillRect(ps.hdc, &rcE, hBrush);
								}
							}
						OffsetRect(&rcE, nStep, 0);
						}
					DeleteObject(hbFill);
					DeleteObject(hbRefill);
					hBrush = NULL;
					}
				}

			EndPaint(hWnd, &ps);
			}
			break;

		case WM_VU_SETMAXRANGE:
			psV = (LPVM)GetWindowLongPtr(hWnd, 0);
			if (VUMETER_OK(psV))
				{
				psV->ulMaxRange = (ULONG)lPar;
				psV->ulHiValue	= 0;
				psV->ulCurValue = 0;
				InvalidateRect(hWnd, NULL, FALSE);
				DbgOutStr("VUmeter max range %ld\r\n", lPar, 0, 0, 0, 0);
				}
			break;

		case WM_VU_SETHIGHVALUE:
			psV = (LPVM)GetWindowLongPtr(hWnd, 0);
			if (VUMETER_OK(psV))
				{
				if ((ULONG)lPar > psV->ulMaxRange)
					psV->ulHiValue = psV->ulMaxRange;
				else
					psV->ulHiValue = (ULONG)lPar;

				if (psV->ulCurValue > psV->ulHiValue)
					psV->ulCurValue = psV->ulHiValue;
				InvalidateRect(hWnd, NULL, FALSE);
				DbgOutStr("VUmeter high value %ld\r\n", lPar, 0, 0, 0, 0);
				}
			break;

		case WM_VU_SETCURVALUE:
			/*
			 * There are two separate tasks to be performed here.
			 * 1. Make sure the values are correctly updated.
			 * 2. Invalidate the correct region.
			 */
			psV = (LPVM)GetWindowLongPtr(hWnd, 0);
			if (VUMETER_OK(psV))
				{
				ULONG ulOldValue;
				ULONG ulLeft;
				ULONG ulRight;
				RECT rc;
				INT base;

				if (psV->ulMaxRange == 0)
					break;

				ulOldValue = psV->ulCurValue;

				if ((ULONG)lPar > psV->ulMaxRange)
					psV->ulCurValue = psV->ulMaxRange;
				else
					psV->ulCurValue = (ULONG)lPar;

				if (psV->ulCurValue > psV->ulHiValue)
					psV->ulHiValue = psV->ulCurValue;

				ulLeft = ulOldValue;			/* Get the low end */
				if (psV->ulCurValue < ulLeft)
					ulLeft = psV->ulCurValue;
				ulRight = ulOldValue;			/* Get the high end */
				if (psV->ulCurValue > ulRight)
					ulRight = psV->ulCurValue;

				/*
				 * Check for early exit
				 */
				if ((psV->ulCurValue == 0) && (psV->ulHiValue == 0))
					break;

				/* These lines must match those in WM_PAINT */
				GetClientRect(hWnd, &rc);
				InflateRect(&rc, -((INT)psV->usDepth), -((INT)psV->usDepth));
				base = rc.right - rc.left;

				ulLeft = ((ULONG)base * ulLeft) / psV->ulMaxRange;
				ulRight = ((ULONG)base * ulRight) / psV->ulMaxRange;

				base = rc.left;
				rc.left = base + (INT)ulLeft;
#if !defined(MIRRORS)
				rc.right = base + (INT)ulRight;
#endif

				InvalidateRect(hWnd, &rc, FALSE);
				DbgOutStr("VUmeter current value %ld\r\n", lPar, 0, 0, 0, 0);
				}
			break;

		case WM_VU_SET_DEPTH:
			psV = (LPVM)GetWindowLongPtr(hWnd, 0);
			if (VUMETER_OK(psV))
				{
				if (wPar < 7)
					{
					psV->usDepth = (USHORT)wPar;
					InvalidateRect(hWnd, NULL, FALSE);
					}
				}
			break;

		default:
			break;
		}

	return DefWindowProc(hWnd, wMsg, wPar, (LPARAM)lPar);
	}
