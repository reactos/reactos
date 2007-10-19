/*
 *	fontview display class
 *
 *	display.c
 *
 *	Copyright (C) 2007	Timo Kreuzer <timo <dot> kreuzer <at> reactos <dot> org>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <windows.h>
#include <stdio.h>

#include "display.h"

#define SPACING1 8
#define SPACING2 5

const WCHAR g_szFontDisplayClassName[] = L"FontDisplayClass";
LRESULT CALLBACK DisplayProc(HWND, UINT, WPARAM, LPARAM);

/* Internal data storage type */
typedef struct
{
	int nPageHeight;
	WCHAR szTypeFaceName[LF_FULLFACESIZE];
	WCHAR szFormat[MAX_FORMAT];
	WCHAR szString[MAX_STRING];

	HFONT hCaptionFont;
	HFONT hCharSetFont;
	HFONT hSizeFont;
	HFONT hFonts[MAX_SIZES];
	int nSizes[MAX_SIZES];
	int nHeights[MAX_SIZES];
} DISPLAYDATA;

/* This is the only public function, it registers the class */
BOOL
Display_InitClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wincl;

	/* Set the fontdisplay window class structure */
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.style = CS_DBLCLKS;
	wincl.lpfnWndProc = DisplayProc;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hInstance = hInstance;
	wincl.hIcon = NULL;
	wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
	wincl.hbrBackground = GetStockObject(WHITE_BRUSH);
	wincl.lpszMenuName = NULL;
	wincl.lpszClassName = g_szFontDisplayClassName;
	wincl.hIconSm = NULL;

	/* Register the window class, and if it fails return FALSE */
	if (!RegisterClassExW (&wincl))
	{
		return FALSE;
	}
	return TRUE;
}

static int
Display_DrawText(HDC hDC, DISPLAYDATA* pData, int nYPos)
{
	HFONT hOldFont;
	TEXTMETRIC tm;
	int i, y;
	WCHAR szSize[5];
	WCHAR szCaption[LF_FULLFACESIZE + 20];

	/* This is the location on the DC where we draw */
	y = -nYPos;

	hOldFont = SelectObject(hDC, pData->hCaptionFont);
	GetTextMetrics(hDC, &tm);

	swprintf(szCaption, L"%s%s", pData->szTypeFaceName, pData->szFormat);
	TextOutW(hDC, 0, y, szCaption, wcslen(szCaption));
	y += tm.tmHeight + SPACING1;

	/* Draw a seperation Line */
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	MoveToEx(hDC, 0, y, NULL);
	LineTo(hDC, 10000, y);
	y += SPACING2;

	/* TODO: Output font info */

	/* Output Character set */
	hOldFont = SelectObject(hDC, pData->hCharSetFont);
	GetTextMetrics(hDC, &tm);
	swprintf(szCaption, L"abcdefghijklmnopqrstuvwxyz");
	TextOutW(hDC, 0, y, szCaption, wcslen(szCaption));
	y += tm.tmHeight + 1;

	swprintf(szCaption, L"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	TextOutW(hDC, 0, y, szCaption, wcslen(szCaption));
	y += tm.tmHeight + 1;

	swprintf(szCaption, L"0123456789.:,;(\"~!@#$%^&*')");
	TextOutW(hDC, 0, y, szCaption, wcslen(szCaption));
	y += tm.tmHeight + 1;

	/* Draw a seperation Line */
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	MoveToEx(hDC, 0, y, NULL);
	LineTo(hDC, 10000, y);
	y += SPACING2;

	/* Output the strings for different sizes */
	for (i = 0; i < MAX_SIZES; i++)
	{
		SelectObject(hDC, pData->hFonts[i]);
		TextOutW(hDC, 20, y, pData->szString, wcslen(pData->szString));
		GetTextMetrics(hDC, &tm);
		y += tm.tmHeight + 1;
		SelectObject(hDC, pData->hSizeFont);
		swprintf(szSize, L"%d", pData->nSizes[i]);
		TextOutW(hDC, 0, y - 13 - tm.tmDescent, szSize, wcslen(szSize));
	}
	SelectObject(hDC, hOldFont);

	return y;
}

static LRESULT
Display_SetTypeFace(HWND hwnd, LPARAM lParam)
{
	DISPLAYDATA* pData;
	TEXTMETRIC tm;
	HDC hDC;
	RECT rect;
	SCROLLINFO si;
	int i;

	/* Set the new type face name */
	pData = (DISPLAYDATA*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	snwprintf(pData->szTypeFaceName, LF_FULLFACESIZE, (WCHAR*)lParam);

	/* Create the new fonts */
	hDC = GetDC(hwnd);
	DeleteObject(pData->hCharSetFont);
	pData->hCharSetFont = CreateFontW(-MulDiv(16, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
	                                  0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	                                  ANSI_CHARSET, OUT_DEFAULT_PRECIS,
	                                  CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
	                                  DEFAULT_PITCH , pData->szTypeFaceName);

	/* Get font format */
	// FIXME: Get the real font format (OpenType?)
	SelectObject(hDC, pData->hCharSetFont);
	GetTextMetrics(hDC, &tm);
	if ((tm.tmPitchAndFamily & TMPF_TRUETYPE) == TMPF_TRUETYPE)
	{
		swprintf(pData->szFormat, L" (TrueType)");
	}

	for (i = 0; i < MAX_SIZES; i++)
	{
		DeleteObject(pData->hFonts[i]);
		pData->hFonts[i] = CreateFontW(-MulDiv(pData->nSizes[i], GetDeviceCaps(hDC, LOGPIXELSY), 72),
		                               0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		                               ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		                               CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		                               DEFAULT_PITCH , pData->szTypeFaceName);
	}

	/* Calculate new page dimensions */
	pData->nPageHeight = Display_DrawText(hDC, pData, 0);
	ReleaseDC(hwnd, hDC);

	/* Set the vertical scrolling range and page size */
	GetClientRect(hwnd, &rect);
	si.cbSize = sizeof(si);
	si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS;
	si.nMin   = 0;
	si.nMax   = pData->nPageHeight;
	si.nPage  = rect.bottom;
	si.nPos   = 0;
	si.nTrackPos = 0;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

	return 0;
}

static LRESULT
Display_SetString(HWND hwnd, LPARAM lParam)
{
	DISPLAYDATA* pData;

	pData = (DISPLAYDATA*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	snwprintf(pData->szString, MAX_STRING, (WCHAR*)lParam);

	// FIXME: redraw the window

	return 0;
}

static LRESULT
Display_OnCreate(HWND hwnd)
{
	DISPLAYDATA* pData;
	const int nSizes[MAX_SIZES] = {12, 18, 24, 36, 48, 60, 72};
	int i;

	/* Create data structure */
	pData = malloc(sizeof(DISPLAYDATA));
	ZeroMemory(pData, sizeof(DISPLAYDATA));

	/* Set the window's GWLP_USERDATA to our data structure */
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData);

	for (i = 0; i < MAX_SIZES; i++)
	{
		pData->nSizes[i] = nSizes[i];
	}

	pData->hCaptionFont = CreateFontW(50, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	                                  ANSI_CHARSET, OUT_DEFAULT_PRECIS,
	                                  CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
	                                  DEFAULT_PITCH , L"Ms Shell Dlg");

	pData->hSizeFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	                               ANSI_CHARSET, OUT_DEFAULT_PRECIS,
	                               CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
	                               DEFAULT_PITCH , L"Ms Shell Dlg");

	Display_SetString(hwnd, (LPARAM)L"Jackdaws love my big sphinx of quartz. 1234567890");
	Display_SetTypeFace(hwnd, (LPARAM)L"Ms Shell Dlg");

	return 0;
}

static LRESULT
Display_OnPaint(HWND hwnd)
{
	DISPLAYDATA* pData;
	PAINTSTRUCT ps;
	SCROLLINFO si;

	pData = (DISPLAYDATA*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	/* Get the Scroll position */
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_VERT, &si);

	BeginPaint(hwnd, &ps);

	/* Erase background */
	FillRect(ps.hdc, &ps.rcPaint, GetStockObject(WHITE_BRUSH));

	/* Draw the text */
	Display_DrawText(ps.hdc, pData, si.nPos);

	EndPaint(hwnd, &ps);

	return 0;
}

static LRESULT
Display_OnSize(HWND hwnd)
{
	RECT rect;
	SCROLLINFO si;
	int nOldPos;

	GetClientRect(hwnd, &rect);

	/* Get the old scroll pos */
	si.cbSize = sizeof(si);
	si.fMask  = SIF_POS;
	GetScrollInfo(hwnd, SB_VERT, &si);
	nOldPos = si.nPos;

	/* Set the new page size */
	si.fMask  = SIF_PAGE;
	si.nPage  = rect.bottom;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

	/* Get the new scroll pos */
	si.fMask  = SIF_POS;
	GetScrollInfo(hwnd, SB_VERT, &si);

	/* If they don't match ... */
	if (nOldPos != si.nPos)
	{
		/* ... scroll the window */
		ScrollWindowEx(hwnd, 0, nOldPos - si.nPos, NULL, NULL, NULL, NULL, SW_INVALIDATE);
		UpdateWindow(hwnd);
	}

	return 0;
}

static LRESULT
Display_OnVScroll(HWND hwnd, WPARAM wParam)
{
	SCROLLINFO si;
	int nPos;

	si.cbSize = sizeof(si);
	si.fMask  = SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo(hwnd, SB_VERT, &si);

	switch(LOWORD(wParam))
	{
		case SB_PAGEUP:
			nPos = si.nPos - 50;
			break;
		case SB_PAGEDOWN:
			nPos = si.nPos + 50;
			break;
		case SB_LINEUP:
			nPos = si.nPos - 10;
			break;
		case SB_LINEDOWN:
			nPos = si.nPos + 10;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			nPos = si.nTrackPos;
			break;
		default:
			nPos = si.nPos;
	}

	nPos = max(nPos, si.nMin);
	nPos = min(nPos, si.nMax);
	if (nPos != si.nPos)
	{
		ScrollWindowEx(hwnd, 0, si.nPos - nPos, NULL, NULL, NULL, NULL, SW_INVALIDATE);
		si.cbSize = sizeof(si);
		si.nPos = nPos;
		si.fMask  = SIF_POS;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		UpdateWindow(hwnd);
	}

	return 0;
}

static LRESULT
Display_OnDestroy(HWND hwnd)
{
	DISPLAYDATA* pData;
	int i;

	pData = (DISPLAYDATA*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	/* Delete the fonts */
	DeleteObject(pData->hCaptionFont);
	DeleteObject(pData->hCharSetFont);
	DeleteObject(pData->hSizeFont);

	for (i = 0; i < MAX_SIZES; i++)
	{
		DeleteObject(pData->hFonts[i]);
	}

	/* Free the data structure */
	free(pData);

	return 0;
}

LRESULT CALLBACK
DisplayProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
			return Display_OnCreate(hwnd);

		case WM_PAINT:
			return Display_OnPaint(hwnd);

		case WM_SIZE:
			return Display_OnSize(hwnd);

		case WM_VSCROLL:
			return Display_OnVScroll(hwnd, wParam);

		case FVM_SETTYPEFACE:
			return Display_SetTypeFace(hwnd, lParam);

		case FVM_SETSTRING:
			return Display_SetString(hwnd, lParam);

		case WM_DESTROY:
			return Display_OnDestroy(hwnd);

		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

