/*
 *  ReactOS rundll32
 *  Copyright (C) 2003 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS rundll32.exe
 * FILE:            apps/utils/rundll32/rundll32.c
 * PURPOSE:         Run a DLL as a program
 * PROGRAMMER:      ShadowFlare (blakflare@hotmail.com)
 */

#define WIN32_LEAN_AND_MEAN

// Both UNICODE and _UNICODE must be either defined or undefined
// because some headers use UNICODE and others use _UNICODE
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#ifdef _UNICODE
#define UNICODE
#endif
#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <tchar.h>

typedef int (WINAPI *DllWinMainW)(
  HWND hWnd,
  HINSTANCE hInstance,
  LPWSTR lpwCmdLine,
  int nCmdShow
);
typedef int (WINAPI *DllWinMainA)(
  HWND hWnd,
  HINSTANCE hInstance,
  LPSTR lpCmdLine,
  int nCmdShow
);

LPCTSTR DllNotLoaded = _T("LoadLibrary failed to load \"%s\"");
LPCTSTR MissingEntry = _T("Missing entry point:%s\nIn %s");
LPCTSTR rundll32_wtitle = _T("rundll32");
LPCTSTR rundll32_wclass = _T("rundll32_window");
TCHAR ModuleFileName[MAX_PATH+1];
LPTSTR ModuleTitle;

LPTSTR FindArgStart(LPTSTR lpStr)
{
	while (*lpStr) {
		switch (*lpStr) {
		case _T(' '):
		case _T('\t'):
		case _T('\r'):
		case _T('\n'):
			break;
		default:
			return lpStr;
		}
		lpStr++;
	}
	return lpStr;
}

LPTSTR FindArgEnd(LPTSTR lpStr)
{
	if (*lpStr != _T('\"')) {
		while (*lpStr) {
			switch (*lpStr) {
			case _T(' '):
			case _T('\t'):
			case _T('\r'):
			case _T('\n'):
				*(lpStr++) = 0;
				return lpStr;
			}
			lpStr++;
		}
	}
	else {
		for (++lpStr;*lpStr && *lpStr != _T('\"');++lpStr) {}
		if (*lpStr == _T('\"')) {
			*(lpStr++) = 0;
		}
	}

	return lpStr;
}

void GetModuleTitle(void)
{
	LPTSTR lpStr;

	GetModuleFileName(0,ModuleFileName,MAX_PATH);
	ModuleTitle = ModuleFileName;

	for (lpStr = ModuleFileName;*lpStr;lpStr++) {
		if (*lpStr == _T('\\'))
			ModuleTitle = lpStr+1;
	}

	for (lpStr = ModuleTitle;*lpStr;lpStr++) {
		if (_tcsicmp(lpStr,_T(".exe"))==0)
			break;
	}

	*lpStr = 0;
}

#ifdef UNICODE
#define ConvertToWideChar(lptString) (lptString)
#define FreeConvertedWideChar(lptString)
#else

LPWSTR ConvertToWideChar(LPCSTR lpString)
{
	LPWSTR lpwString;
	size_t nStrLen;

	nStrLen = strlen(lpString) + 1;

	lpwString = (LPWSTR)malloc(nStrLen * sizeof(WCHAR));
	MultiByteToWideChar(0,0,lpString,nStrLen,lpwString,nStrLen);

	return lpwString;
}

#define FreeConvertedWideChar(lptString) free(lptString)
#endif

#ifdef UNICODE
#define ConvertToMultiByte(lptString) DuplicateToMultiByte(lptString,0)
#define FreeConvertedMultiByte(lptString) free(lptString)
#else
#define ConvertToMultiByte(lptString) (lptString)
#define FreeConvertedMultiByte(lptString)
#endif

LPSTR DuplicateToMultiByte(LPCTSTR lptString, size_t nBufferSize)
{
	LPSTR lpString;
	size_t nStrLen;

	nStrLen = _tcslen(lptString) + 1;
	if (nBufferSize == 0) nBufferSize = nStrLen;

	lpString = (LPSTR)malloc(nBufferSize);
#ifdef UNICODE
	WideCharToMultiByte(0,0,lptString,nStrLen,lpString,nBufferSize,0,0);
#else
	strncpy(lpString,lptString,nBufferSize);
#endif

	return lpString;
}

LRESULT CALLBACK EmptyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

ATOM RegisterBlankClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style         = 0;
	wcex.lpfnWndProc   = EmptyWindowProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = hInstance;
	wcex.hIcon         = 0;
	wcex.hCursor       = 0;
	wcex.hbrBackground = 0;
	wcex.lpszMenuName  = 0;
	wcex.lpszClassName = rundll32_wclass;
	wcex.hIconSm       = 0;

	return RegisterClassEx(&wcex);
}

int WINAPI WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLineA,
  int nCmdShow
)
{
	LPTSTR lptCmdLineCopy,lptCmdLine,lptDllName,lptFuncName,lptMsgBuffer;
	LPSTR lpFuncName,lpaCmdLine;
	LPWSTR lpwCmdLine;
	HMODULE hDll;
	DllWinMainW fnDllWinMainW;
	DllWinMainA fnDllWinMainA;
	HWND hWindow;
	int nRetVal;
	size_t nStrLen;

	lptCmdLineCopy = _tcsdup(GetCommandLine());

	lptCmdLine = FindArgStart(lptCmdLineCopy);
	lptCmdLine = FindArgEnd(lptCmdLine);
	lptDllName = FindArgStart(lptCmdLine);
	while (*lptDllName == _T('/'))
		lptDllName = FindArgStart(FindArgEnd(++lptDllName));
	lptCmdLine = FindArgEnd(lptDllName);
	if (*lptDllName == _T('\"')) lptDllName++;

	if (!*lptDllName) {
		free(lptCmdLineCopy);
		return 0;
	}

	for (lptFuncName = lptDllName;*lptFuncName && *lptFuncName != _T(',');lptFuncName++) {}
	if (*lptFuncName == _T(',')) {
		*(lptFuncName++) = 0;
	}
	else {
		lptFuncName = FindArgStart(lptCmdLine);
		lptCmdLine = FindArgEnd(lptFuncName);
		if (*lptFuncName == _T('\"')) lptFuncName++;
	}

	if (!*lptFuncName) {
		free(lptCmdLineCopy);
		return 0;
	}

	if (*lptCmdLine) lptCmdLine = FindArgStart(lptCmdLine);

	nRetVal = 0;

	hDll = LoadLibrary(lptDllName);
	if (hDll) {
		nStrLen = _tcslen(lptFuncName);
		lpFuncName = DuplicateToMultiByte(lptFuncName,nStrLen + 2);

		lpFuncName[nStrLen] = 'W';
		lpFuncName[nStrLen+1] = 0;
		fnDllWinMainW = (DllWinMainW)GetProcAddress(hDll,lpFuncName);
		fnDllWinMainA = 0;
		if (!fnDllWinMainW) {
			lpFuncName[nStrLen] = 'A';
			fnDllWinMainA = (DllWinMainA)GetProcAddress(hDll,lpFuncName);
			if (!fnDllWinMainA) {
				lpFuncName[nStrLen] = 0;
				fnDllWinMainA = (DllWinMainA)GetProcAddress(hDll,lpFuncName);
			}
		}

		free(lpFuncName);

		RegisterBlankClass(hInstance);
		// Create a window to pass the handle to the dll function
		hWindow = CreateWindowEx(0,rundll32_wclass,rundll32_wtitle,0,CW_USEDEFAULT,0,CW_USEDEFAULT,0,0,0,hInstance,0);

		if (fnDllWinMainW) {
			lpwCmdLine = ConvertToWideChar(lptCmdLine);
			nRetVal = fnDllWinMainW(hWindow,hInstance,lpwCmdLine,nCmdShow);
			FreeConvertedWideChar(lpwCmdLine);
		}
		else if (fnDllWinMainA) {
			lpaCmdLine = ConvertToMultiByte(lptCmdLine);
			nRetVal = fnDllWinMainA(hWindow,hInstance,lpaCmdLine,nCmdShow);
			FreeConvertedMultiByte(lpaCmdLine);
		}
		else {
			GetModuleTitle();
			lptMsgBuffer = (LPTSTR)malloc((_tcslen(MissingEntry) - 4 + _tcslen(lptFuncName) + _tcslen(lptDllName) + 1) * sizeof(TCHAR));
			_stprintf(lptMsgBuffer,MissingEntry,lptFuncName,lptDllName);
			MessageBox(0,lptMsgBuffer,ModuleTitle,MB_ICONERROR);
			free(lptMsgBuffer);
		}

		DestroyWindow(hWindow);
		UnregisterClass(rundll32_wclass,hInstance);

		FreeLibrary(hDll);
	}
	else {
		GetModuleTitle();
		lptMsgBuffer = (LPTSTR)malloc((_tcslen(DllNotLoaded) - 2 + _tcslen(lptDllName) + 1) * sizeof(TCHAR));
		_stprintf(lptMsgBuffer,DllNotLoaded,lptDllName);
		MessageBox(0,lptMsgBuffer,ModuleTitle,MB_ICONERROR);
		free(lptMsgBuffer);
	}

	free(lptCmdLineCopy);
	return nRetVal;
}

