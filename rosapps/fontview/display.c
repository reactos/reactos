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
	int nHeight;
	WCHAR szTypeFaceName[MAX_TYPEFACENAME];
	WCHAR szFormat[MAX_FORMAT];
	WCHAR szString[MAX_STRING];
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
Display_DrawText(HDC hDC, DISPLAYDATA* pData, int nYPos, BOOL bDraw)
{
	HFONT hOldFont, hFont, hFontNums;
	TEXTMETRIC tm;
	int i, y;
	const int nSizes[7] = {12, 18, 24, 36, 48, 60, 72};
	WCHAR szSize[5];
	WCHAR szCaption[MAX_TYPEFACENAME + 20];

	/* This is the location on the DC where we draw */
	y = -nYPos;

	/* Draw font name */
	hFont = CreateFontW(50, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	                   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
	                   DEFAULT_PITCH , L"Ms Shell Dlg");
	hOldFont = SelectObject(hDC, hFont);
	if (bDraw)
	{
		swprintf(szCaption, L"%s (%s)", pData->szTypeFaceName, pData->szFormat);
		TextOutW(hDC, 0, y, szCaption, wcslen(szCaption));
	}
	GetTextMetrics(hDC, &tm);
	y += tm.tmHeight + SPACING1;
	SelectObject(hDC, hOldFont);
	DeleteObject(hFont);

	/* Draw a seperation Line */
	if (bDraw)
	{
		SelectObject(hDC, GetStockObject(BLACK_PEN));
		MoveToEx(hDC, 0, y, NULL);
		LineTo(hDC, 10000, y);
	}
	y += SPACING2;

	/* Output font info */
	hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
						DEFAULT_PITCH , pData->szTypeFaceName);
	hOldFont = SelectObject(hDC, hFont);
	SelectObject(hDC, hOldFont);
	DeleteObject(hFont);

	/* Outout the lines for different sizes */
	hFontNums = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
						DEFAULT_PITCH , L"Ms Shell Dlg");

	for (i = 0; i < 7; i++)
	{
		hOldFont = SelectObject(hDC, hFontNums);
		if (bDraw)
		{
			swprintf(szSize, L"%d", nSizes[i]);
			TextOutW(hDC, 0, y, szSize, wcslen(szSize));
		}
		hFont = CreateFontW(nSizes[i], 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
						DEFAULT_PITCH , pData->szTypeFaceName);
		SelectObject(hDC, hFont);
		if (bDraw)
		{
			TextOutW(hDC, 20, y, pData->szString, wcslen(pData->szString));
		}
		GetTextMetrics(hDC, &tm);
		y += tm.tmHeight + 2;
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);
	}
	DeleteObject(hFontNums);

	return y;
}

static LRESULT
Display_OnCreate(HWND hwnd)
{
	DISPLAYDATA* pData;

	/* Initialize data structure */
	pData = malloc(sizeof(DISPLAYDATA));
	pData->nHeight = 0;
	swprintf(pData->szTypeFaceName, L"");
	swprintf(pData->szFormat, L"");
	swprintf(pData->szString, L"");

	/* Set the window's GWLP_USERDATA to our data structure */
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData);

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

	/* fill with white */
	FillRect(ps.hdc, &ps.rcPaint, GetStockObject(WHITE_BRUSH));

	/* Draw the text */
	Display_DrawText(ps.hdc, pData, si.nPos, TRUE);

	EndPaint(hwnd, &ps);

	return 0;
}

static LRESULT
Display_OnSize(HWND hwnd)
{
	RECT rect;
	SCROLLINFO si; 

	GetClientRect(hwnd, &rect);

	/* Set the new page size */
	si.cbSize = sizeof(si);
	si.fMask  = SIF_PAGE;
	si.nPage  = rect.bottom;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE); 
// FIXME: handle exceeding of current pos -> redraw entire window
// if new page size is < curent pos: current pos = maximum whatever and then redraw
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
//		case SB_THUMBTRACK:
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
		ScrollWindowEx(hwnd, 0, -(nPos - si.nPos), NULL, NULL, NULL, NULL, SW_INVALIDATE);
		si.cbSize = sizeof(si);
		si.nPos = nPos;
		si.fMask  = SIF_POS;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		UpdateWindow(hwnd);
	}
	return 0;
}

static LRESULT
Display_SetTypeFace(HWND hwnd, LPARAM lParam)
{
	DISPLAYDATA* pData;
	HDC hDC;
	RECT rect;
	SCROLLINFO si;

	/* Set the new type face name */
	pData = (DISPLAYDATA*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	snwprintf(pData->szTypeFaceName, MAX_TYPEFACENAME, (WCHAR*)lParam);

	/* Calculate new page dimensions */
	hDC = GetDC(hwnd);
	pData->nHeight = Display_DrawText(hDC, pData, 0, FALSE);
	ReleaseDC(hwnd, hDC);

	/* Set the vertical scrolling range and page size */
	GetClientRect(hwnd, &rect);
	si.cbSize = sizeof(si);
	si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS;
	si.nMin   = 0;
	si.nMax   = pData->nHeight;
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
Display_OnDestroy(HWND hwnd)
{
	DISPLAYDATA* pData;

	/* Free the data structure */
	pData = (DISPLAYDATA*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

