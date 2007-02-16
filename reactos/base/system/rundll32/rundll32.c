/*
 * ReactOS rundll32
 * Copyright (C) 2003-2004 ReactOS Team
 *
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
#include "resource.h"

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

/*
LPCTSTR DllNotLoaded = _T("LoadLibrary failed to load \"%s\"");
LPCTSTR MissingEntry = _T("Missing entry point:%s\nIn %s");
*/
LPCTSTR rundll32_wtitle = _T("rundll32");
LPCTSTR rundll32_wclass = _T("rundll32_window");

TCHAR ModuleFileName[MAX_PATH+1];
LPTSTR ModuleTitle;


// CommandLineToArgv converts a command-line string to argc and
// argv similar to the ones in the standard main function.
// This is a specialized version coded specifically for rundll32
// and is not intended to be used in any other program.
LPTSTR *WINAPI CommandLineToArgv(LPCTSTR lpCmdLine, int *lpArgc)
{
	LPTSTR *argv, lpSrc, lpDest, lpArg;
	int argc, nBSlash, nNames;
	BOOL bInQuotes, bFirstChar;

	// If null was passed in for lpCmdLine, there are no arguments
	if (!lpCmdLine) {
		if (lpArgc)
			*lpArgc = 0;
		return 0;
	}

	lpSrc = (LPTSTR)lpCmdLine;
	// Skip spaces at beginning
	while (*lpSrc == _T(' ') || *lpSrc == _T('\t'))
		lpSrc++;

	// If command-line starts with null, there are no arguments
	if (*lpSrc == 0) {
		if (lpArgc)
			*lpArgc = 0;
		return 0;
	}

	lpArg = lpSrc;
	argc = 0;
	nBSlash = 0;
	bInQuotes = FALSE;
	bFirstChar = TRUE;
	nNames = 0;

	// Count the number of arguments
	while (nNames < 4) {
		if (*lpSrc == 0 || (*lpSrc == _T(',') && nNames == 2) || ((*lpSrc == _T(' ') || *lpSrc == _T('\t')) && !bInQuotes)) {
			// Whitespace not enclosed in quotes signals the start of another argument
			argc++;

			// Skip whitespace between arguments
			while (*lpSrc == _T(' ') || *lpSrc == _T('\t') || (*lpSrc == _T(',') && nNames == 2))
				lpSrc++;
			if (*lpSrc == 0)
				break;
			if (nNames >= 3) {
				// Increment the count for the last argument
				argc++;
				break;
			}
			nBSlash = 0;
			bFirstChar = TRUE;
			continue;
		}
		else if (*lpSrc == _T('\\')) {
			// Count consecutive backslashes
			nBSlash++;
			bFirstChar = FALSE;
		}
		else if (*lpSrc == _T('\"') && !(nBSlash & 1)) {
			// Open or close quotes
			bInQuotes = !bInQuotes;
			nBSlash = 0;
		}
		else {
			// Some other character
			nBSlash = 0;
			if (bFirstChar && ((*lpSrc != _T('/') && nNames <= 1) || nNames > 1))
				nNames++;
			bFirstChar = FALSE;
		}
		lpSrc++;
	}

	// Allocate space for the pointers in argv and the strings in one block
	argv = (LPTSTR *)malloc(argc * sizeof(LPTSTR) + (_tcslen(lpArg) + 1) * sizeof(TCHAR));

	if (!argv) {
		// Memory allocation failed
		if (lpArgc)
			*lpArgc = 0;
		return 0;
	}

	lpSrc = lpArg;
	lpDest = lpArg = (LPTSTR)(argv + argc);
	argc = 0;
	nBSlash = 0;
	bInQuotes = FALSE;
	bFirstChar = TRUE;
	nNames = 0;

	// Fill the argument array
	while (nNames < 4) {
		if (*lpSrc == 0 || (*lpSrc == _T(',') && nNames == 2) || ((*lpSrc == _T(' ') || *lpSrc == _T('\t')) && !bInQuotes)) {
			// Whitespace not enclosed in quotes signals the start of another argument
			// Null-terminate argument
			*lpDest++ = 0;
			argv[argc++] = lpArg;

			// Skip whitespace between arguments
			while (*lpSrc == _T(' ') || *lpSrc == _T('\t') || (*lpSrc == _T(',') && nNames == 2))
				lpSrc++;
			if (*lpSrc == 0)
				break;
			lpArg = lpDest;
			if (nNames >= 3) {
				// Copy the rest of the command-line to the last argument
				argv[argc++] = lpArg;
				_tcscpy(lpArg,lpSrc);
				break;
			}
			nBSlash = 0;
			bFirstChar = TRUE;
			continue;
		}
		else if (*lpSrc == _T('\\')) {
			*lpDest++ = _T('\\');
			lpSrc++;

			// Count consecutive backslashes
			nBSlash++;
			bFirstChar = FALSE;
		}
		else if (*lpSrc == _T('\"')) {
			if (!(nBSlash & 1)) {
				// If an even number of backslashes are before the quotes,
				// the quotes don't go in the output
				lpDest -= nBSlash / 2;
				bInQuotes = !bInQuotes;
			}
			else {
				// If an odd number of backslashes are before the quotes,
				// output a quote
				lpDest -= (nBSlash + 1) / 2;
				*lpDest++ = _T('\"');
				bFirstChar = FALSE;
			}
			lpSrc++;
			nBSlash = 0;
		}
		else {
			// Copy other characters
			if (bFirstChar && ((*lpSrc != _T('/') && nNames <= 1) || nNames > 1))
				nNames++;
			*lpDest++ = *lpSrc++;
			nBSlash = 0;
			bFirstChar = FALSE;
		}
	}

	if (lpArgc)
		*lpArgc = argc;
	return argv;
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

// The macro ConvertToWideChar takes a tstring parameter and returns
// a pointer to a unicode string.  A conversion is performed if
// neccessary.  FreeConvertedWideChar string should be used on the
// return value of ConvertToWideChar when the string is no longer
// needed.  The original string or the string that is returned
// should not be modified until FreeConvertedWideChar has been called.
#ifdef UNICODE
#define ConvertToWideChar(lptString) (lptString)
#define FreeConvertedWideChar(lpwString)
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

#define FreeConvertedWideChar(lpwString) free(lpwString)
#endif

// The macro ConvertToMultiByte takes a tstring parameter and returns
// a pointer to an ansi string.  A conversion is performed if
// neccessary.  FreeConvertedMultiByte string should be used on the
// return value of ConvertToMultiByte when the string is no longer
// needed.  The original string or the string that is returned
// should not be modified until FreeConvertedMultiByte has been called.
#ifdef UNICODE
#define ConvertToMultiByte(lptString) DuplicateToMultiByte(lptString,0)
#define FreeConvertedMultiByte(lpaString) free(lpaString)
#else
#define ConvertToMultiByte(lptString) (lptString)
#define FreeConvertedMultiByte(lpaString)
#endif

// DuplicateToMultiByte takes a tstring parameter and always returns
// a pointer to a duplicate ansi string.  If nBufferSize is zero,
// the buffer length is the exact size of the string plus the
// terminating null.  If nBufferSize is nonzero, the buffer length
// is equal to nBufferSize.  As with strdup, free should be called
// for the returned string when it is no longer needed.
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

// Registers a minimal window class for passing to the dll function
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
	int argc;
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	LPTSTR *argv;
	LPTSTR lptCmdLine,lptDllName,lptFuncName,lptMsgBuffer;
	LPSTR lpFuncName,lpaCmdLine;
	LPWSTR lpwCmdLine;
	HMODULE hDll;
	DllWinMainW fnDllWinMainW;
	DllWinMainA fnDllWinMainA;
	HWND hWindow;
	int nRetVal,i;
	size_t nStrLen;

	// Get command-line in argc-argv format
	argv = CommandLineToArgv(GetCommandLine(),&argc);

	// Skip all beginning arguments starting with a slash (/)
	for (i = 1; i < argc; i++)
		if (*argv[i] != _T('/')) break;

	// If no dll was specified, there is nothing to do
	if (i >= argc) {
		if (argv) free(argv);
		return 0;
	}

	lptDllName = argv[i++];

	// The next argument, which specifies the name of the dll function,
	// can either have a comma between it and the dll filename or a space.
	// Using a comma here is the preferred method
	if (i < argc)
		lptFuncName = argv[i++];
	else
		lptFuncName = _T("");

	// If no function name was specified, nothing needs to be done
	if (!*lptFuncName) {
		if (argv) free(argv);
		return 0;
	}

	// The rest of the arguments will be passed to dll function
	if (i < argc)
		lptCmdLine = argv[i];
	else
		lptCmdLine = _T("");

	nRetVal = 0;

	// Everything is all setup, so load the dll now
	hDll = LoadLibrary(lptDllName);
	if (hDll) {
		nStrLen = _tcslen(lptFuncName);
		// Make a non-unicode version of the function name,
		// since that is all GetProcAddress accepts
		lpFuncName = DuplicateToMultiByte(lptFuncName,nStrLen + 2);

#ifdef UNICODE
		lpFuncName[nStrLen] = 'W';
		lpFuncName[nStrLen+1] = 0;
		// Get address of unicode version of the dll function if it exists
		fnDllWinMainW = (DllWinMainW)GetProcAddress(hDll,lpFuncName);
		fnDllWinMainA = 0;
		if (!fnDllWinMainW) {
			// If no unicode function was found, get the address of the non-unicode function
			lpFuncName[nStrLen] = 'A';
			fnDllWinMainA = (DllWinMainA)GetProcAddress(hDll,lpFuncName);
			if (!fnDllWinMainA) {
				// If first non-unicode function was not found, get the address
				// of the other non-unicode function
				lpFuncName[nStrLen] = 0;
				fnDllWinMainA = (DllWinMainA)GetProcAddress(hDll,lpFuncName);
			}
		}
#else
		// Get address of non-unicode version of the dll function if it exists
		fnDllWinMainA = (DllWinMainA)GetProcAddress(hDll,lpFuncName);
		fnDllWinMainW = 0;
		if (!fnDllWinMainA) {
			// If first non-unicode function was not found, get the address
			// of the other non-unicode function
			lpFuncName[nStrLen] = 'A';
			lpFuncName[nStrLen+1] = 0;
			fnDllWinMainA = (DllWinMainA)GetProcAddress(hDll,lpFuncName);
			if (!fnDllWinMainA) {
				// If non-unicode function was not found, get the address of the unicode function
				lpFuncName[nStrLen] = 'W';
				fnDllWinMainW = (DllWinMainW)GetProcAddress(hDll,lpFuncName);
			}
		}
#endif

		free(lpFuncName);

		RegisterBlankClass(hInstance);
		// Create a window so we can pass a window handle to
		// the dll function; this is required
		hWindow = CreateWindowEx(0,rundll32_wclass,rundll32_wtitle,0,CW_USEDEFAULT,0,CW_USEDEFAULT,0,0,0,hInstance,0);

		if (fnDllWinMainW) {
			// Convert the command-line string to unicode and call the dll function
			lpwCmdLine = ConvertToWideChar(lptCmdLine);
			nRetVal = fnDllWinMainW(hWindow,hInstance,lpwCmdLine,nCmdShow);
			FreeConvertedWideChar(lpwCmdLine);
		}
		else if (fnDllWinMainA) {
			// Convert the command-line string to ansi and call the dll function
			lpaCmdLine = ConvertToMultiByte(lptCmdLine);
			nRetVal = fnDllWinMainA(hWindow,hInstance,lpaCmdLine,nCmdShow);
			FreeConvertedMultiByte(lpaCmdLine);
		}
		else {
			// The specified dll function was not found; display an error message
			GetModuleTitle();
            LoadString( GetModuleHandle(NULL), IDS_MissingEntry, (LPTSTR) szMsg,RC_STRING_MAX_SIZE);

			lptMsgBuffer = (LPTSTR)malloc((_tcslen(szMsg) - 4 + _tcslen(lptFuncName) + _tcslen(lptDllName) + 1) * sizeof(TCHAR));
			_stprintf(lptMsgBuffer,szMsg,lptFuncName,lptDllName);
			MessageBox(0,lptMsgBuffer,ModuleTitle,MB_ICONERROR);
			free(lptMsgBuffer);
		}

		DestroyWindow(hWindow);
		UnregisterClass(rundll32_wclass,hInstance);

		// The dll function has finished executing, so unload it
		FreeLibrary(hDll);
	}
	else {
		// The dll could not be loaded; display an error message
		GetModuleTitle();
		LoadString( GetModuleHandle(NULL), IDS_DllNotLoaded, (LPTSTR) szMsg,RC_STRING_MAX_SIZE);

		lptMsgBuffer = (LPTSTR)malloc((_tcslen(szMsg) - 2 + _tcslen(lptDllName) + 1) * sizeof(TCHAR));
		_stprintf(lptMsgBuffer,szMsg,lptDllName);
		
		MessageBox(0,lptMsgBuffer,ModuleTitle,MB_ICONERROR);
		free(lptMsgBuffer);
	}

	if (argv) free(argv);
	return nRetVal;
}

