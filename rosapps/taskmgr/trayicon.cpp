/*
 *  ReactOS Task Manager
 *
 *  trayicon.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
	
#include "taskmgr.h"
#include "trayicon.h"
#include "perfdata.h"
#include "shellapi.h"

HICON TrayIcon_GetProcessorUsageIcon(void)
{
	HICON		hTrayIcon = NULL;
	HDC			hScreenDC = NULL;
	HDC			hDC = NULL;
	HBITMAP		hBitmap = NULL;
	HBITMAP		hOldBitmap = NULL;
	HBITMAP		hBitmapMask = NULL;
	ICONINFO	iconInfo;
	ULONG		ProcessorUsage;
	int			nLinesToDraw;
	HBRUSH		hBitmapBrush = NULL;
	RECT		rc;

	//
	// Get a handle to the screen DC
	//
	hScreenDC = GetDC(NULL);
	if (!hScreenDC)
		goto done;
	
	//
	// Create our own DC from it
	//
	hDC = CreateCompatibleDC(hScreenDC);
	if (!hDC)
		goto done;

	//
	// Load the bitmaps
	//
	hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TRAYICON));
	hBitmapMask = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TRAYMASK));
	if (!hBitmap || !hBitmapMask)
		goto done;

	hBitmapBrush = CreateSolidBrush(RGB(0, 255, 0));
	if (!hBitmapBrush)
		goto done;
	
	//
	// Select the bitmap into our device context
	// so we can draw on it.
	//
	hOldBitmap = (HBITMAP) SelectObject(hDC, hBitmap);

	//
	// Get the cpu usage
	//
	ProcessorUsage = PerfDataGetProcessorUsage();

	//
	// Calculate how many lines to draw
	// since we have 11 rows of space
	// to draw the cpu usage instead of
	// just having 10.
	//
	nLinesToDraw = (ProcessorUsage + (ProcessorUsage / 10)) / 11;
	rc.left = 3;
	rc.top = 12 - nLinesToDraw;
	rc.right = 13;
	rc.bottom = 13;

	//
	// Now draw the cpu usage
	//
	if (nLinesToDraw)
		FillRect(hDC, &rc, hBitmapBrush);

	//
	// Now that we are done drawing put the
	// old bitmap back.
	//
	SelectObject(hDC, hOldBitmap);
	hOldBitmap = NULL;
	
	iconInfo.fIcon = TRUE;
	iconInfo.xHotspot = 0;
	iconInfo.yHotspot = 0;
	iconInfo.hbmMask = hBitmapMask;
	iconInfo.hbmColor = hBitmap;

	hTrayIcon = CreateIconIndirect(&iconInfo);

done:
	//
	// Cleanup
	//
	if (hScreenDC)
		ReleaseDC(NULL, hScreenDC);
	if (hOldBitmap)
		SelectObject(hDC, hOldBitmap);
	if (hDC)
		DeleteDC(hDC);
	if (hBitmapBrush)
		DeleteObject(hBitmapBrush);
	if (hBitmap)
		DeleteObject(hBitmap);
	if (hBitmapMask)
		DeleteObject(hBitmapMask);
	
	//
	// Return the newly created tray icon (if successful)
	//
	return hTrayIcon;
}

BOOL TrayIcon_ShellAddTrayIcon(void)
{
	NOTIFYICONDATA	nid;
	HICON			hIcon = NULL;
	BOOL			bRetVal;

	memset(&nid, 0, sizeof(NOTIFYICONDATA));

	hIcon = TrayIcon_GetProcessorUsageIcon();

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hMainWnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	//nid.uCallbackMessage = ??;
	nid.hIcon = hIcon;
	sprintf(nid.szTip, "CPU Usage: %d%%", PerfDataGetProcessorUsage());

	bRetVal = Shell_NotifyIcon(NIM_ADD, &nid);

	if (hIcon)
		DeleteObject(hIcon);

	return bRetVal;
}

BOOL TrayIcon_ShellRemoveTrayIcon(void)
{
	NOTIFYICONDATA	nid;
	BOOL			bRetVal;
	
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hMainWnd;
	nid.uID = 0;
	nid.uFlags = 0;
	//nid.uCallbackMessage = ??;
	
	bRetVal = Shell_NotifyIcon(NIM_DELETE, &nid);
	
	return bRetVal;
}

BOOL TrayIcon_ShellUpdateTrayIcon(void)
{
	NOTIFYICONDATA	nid;
	HICON			hIcon = NULL;
	BOOL			bRetVal;
	
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	
	hIcon = TrayIcon_GetProcessorUsageIcon();
	
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hMainWnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	//nid.uCallbackMessage = ??;
	nid.hIcon = hIcon;
	sprintf(nid.szTip, "CPU Usage: %d%%", PerfDataGetProcessorUsage());
	
	bRetVal = Shell_NotifyIcon(NIM_MODIFY, &nid);
	
	if (hIcon)
		DeleteObject(hIcon);
	
	return bRetVal;
}
