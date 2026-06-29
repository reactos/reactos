///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998-2000 Paul Brannan
//Copyright (C) 1998 I.Ioannou
//Copyright (C) 1997 Brad Johnson
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//I.Ioannou
//roryt@hol.gr
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Module:		tnerror.cpp
//
// Contents:	error reporting
//
// Product:		telnet
//
// Revisions: June 15, 1998 Paul Brannan <pbranna@clemson.edu>
//            May 15, 1998 Paul Brannan
//            5.April.1997 jbj@nounname.com
//            5.Dec.1996 jbj@nounname.com
//            Version 2.0
//
//            02.Apr.1995	igor.milavec@uni-lj.si
//					  Original code
//
///////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include <time.h>

#ifndef LANG_USER_DEFAULT
#define LANG_USER_DEFAULT 400
#endif

// This has been moved to tnconfig.cpp
// int Telnet_Redir = 0;
// Telnet_Redir is set to the value of the environment variable TELNET_REDIR
// in main.

int printit(const char * it){
	DWORD numwritten;
	if (!ini.get_output_redir()) {
		if (!WriteConsole(
			GetStdHandle(STD_OUTPUT_HANDLE),	// handle of a console screen buffer
			it,	// address of buffer to write from
			strlen(it),	// number of characters to write
			&numwritten,	// address of number of characters written
			0 	// reserved
			)) return -1;
		// FIX ME!!! We need to tell the console that the cursor has moved.
		// Does this mean making Console global?
		// Paul Brannan 6/14/98
		// Console.sync();
	}else{
		if (!WriteFile(
			GetStdHandle(STD_OUTPUT_HANDLE),	// handle of a console screen buffer
			it,	// address of buffer to write from
			strlen(it),	// number of characters to write
			&numwritten,	// address of number of characters written
			NULL // no overlapped I/O
			)) return -1;
	}
	return 0;
}

#ifdef __REACTOS__
int wprintit(LPCWSTR it)
{
    DWORD numwritten;
    if (!ini.get_output_redir())
    {
        if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                           it, wcslen(it), &numwritten, NULL))
        {
            return -1;
        }
    }
    else
    {
        // calculate the number of bytes needed to store the UTF-8 string
        int cbMultibyte = WideCharToMultiByte(CP_UTF8, 0, it, -1, NULL, 0, NULL, NULL);
        if (cbMultibyte == 0)
            return 0;
        if (cbMultibyte < 0)
            return -1;
        // allocate the buffer for the UTF-8 string
        char* szBuffer = new char[cbMultibyte];
        if (!szBuffer)
            return -1;

        bool bSuccess = false;
        if (WideCharToMultiByte(CP_UTF8, 0, it, -1, szBuffer, cbMultibyte, NULL, NULL) &&
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
                      szBuffer, cbMultibyte, &numwritten, NULL))
        {
            bSuccess = true;
        }

        delete[] szBuffer;
        if (!bSuccess)
            return -1;
    }
    return 0;
}
#endif

int printm(LPTSTR szModule, BOOL fSystem, DWORD dwMessageId, ...)
{
	int Result = 0;

	HMODULE hModule = 0;
	if (szModule)
		hModule = LoadLibrary(szModule);

	va_list Ellipsis;
	va_start(Ellipsis, dwMessageId);
#ifdef __REACTOS__
	LPWSTR pszMessage = NULL;
	DWORD dwMessage = 0;

	if(fSystem) {
		dwMessage = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, hModule, dwMessageId,
			LANG_USER_DEFAULT, (LPWSTR)&pszMessage, 128, &Ellipsis);
	} else {
		// we will use a string table.
		WCHAR wszString[256];
		if(LoadStringW(0, dwMessageId, wszString, sizeof(wszString) / sizeof(*wszString)))
			dwMessage = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_STRING, wszString, dwMessageId,
				LANG_USER_DEFAULT, (LPWSTR)&pszMessage, sizeof(wszString) / sizeof(*wszString), &Ellipsis);
	}
#else
	LPTSTR pszMessage = 0;
	DWORD dwMessage = 0;
	if(fSystem) {
		dwMessage = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, hModule, dwMessageId,
			LANG_USER_DEFAULT, (LPTSTR)&pszMessage, 128, &Ellipsis);
	} else {
		// we will use a string table.
		char szString[256];
		if(LoadString(0, dwMessageId, szString, sizeof(szString)))
			dwMessage = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_STRING, szString, dwMessageId,
				LANG_USER_DEFAULT, (LPTSTR)&pszMessage, 256, &Ellipsis);
	}
#endif

	va_end(Ellipsis);

	if (szModule)
		FreeLibrary(hModule);

	if (dwMessage) {
#ifdef __REACTOS__
		Result = wprintit(pszMessage);
#else
		Result = printit(pszMessage);
#endif
		LocalFree(pszMessage);
	}

	return Result;
}


void LogErrorConsole(LPTSTR szError)
{
	DWORD dwLastError = GetLastError();

	const int cbLastError = 1024;
	TCHAR szLastError[cbLastError];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwLastError, LANG_USER_DEFAULT,
		szLastError, cbLastError, 0);

	LPTSTR lpszStrings[2];
	lpszStrings[0] = szError;
	lpszStrings[1] = szLastError;

	const int cbErrorString = 1024;
	TCHAR szErrorString[cbErrorString];
	FormatMessage(FORMAT_MESSAGE_FROM_HMODULE| FORMAT_MESSAGE_ARGUMENT_ARRAY,
		0, MSG_ERROR, LANG_USER_DEFAULT,
		szErrorString, cbErrorString, (va_list*)lpszStrings);

	time_t dwTime;
	time(&dwTime);
	char* szTime = ctime(&dwTime);
	szTime[19] = 0;

	//	printf("E %s %s", szTime + 11, szErrorString);
	char * buf;
	buf = new char [ 3 + strlen(szTime) - 11 + strlen(szErrorString) + 5 ];
	sprintf( buf,"E %s %s", szTime + 11, szErrorString);
	printit(buf);
	delete [] buf;
}


void LogWarningConsole(DWORD dwEvent, LPTSTR szWarning)
{
	DWORD dwLastError = GetLastError();

	const int cbLastError = 1024;
	TCHAR szLastError[cbLastError];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwLastError, LANG_USER_DEFAULT,
		szLastError, cbLastError, 0);

	LPTSTR lpszStrings[2];
	lpszStrings[0] = szWarning;
	lpszStrings[1] = szLastError;

	const int cbWarningString = 1024;
	TCHAR szWarningString[cbWarningString];
	FormatMessage(FORMAT_MESSAGE_FROM_HMODULE| FORMAT_MESSAGE_ARGUMENT_ARRAY,
		0, dwEvent, LANG_USER_DEFAULT,
		szWarningString, cbWarningString, (va_list*)lpszStrings);

	time_t dwTime;
	time(&dwTime);
	char* szTime = ctime(&dwTime);
	szTime[19] = 0;

	//	printf("W %s %s", szTime + 11, szWarningString);
	char * buf;
	buf = new char [ 3 + strlen(szTime) - 11 + strlen(szWarningString) + 5 ];
	sprintf(buf ,"W %s %s", szTime + 11, szWarningString);
	printit(buf);
	delete [] buf;

}


void LogInfoConsole(DWORD dwEvent, LPTSTR szInformation)
{
	LPTSTR lpszStrings[1];
	lpszStrings[0] = szInformation;

	const int cbInfoString = 1024;
	TCHAR szInfoString[cbInfoString];
	FormatMessage(FORMAT_MESSAGE_FROM_HMODULE| FORMAT_MESSAGE_ARGUMENT_ARRAY,
		0, dwEvent, LANG_USER_DEFAULT,
		szInfoString, cbInfoString, (va_list*)lpszStrings);

	time_t dwTime;
	time(&dwTime);
	char* szTime = ctime(&dwTime);
	szTime[19] = 0;

	//	printf("I %s %s", szTime + 11, szInfoString);
	char * buf;
	buf = new char [ 3 + strlen(szTime) - 11 + strlen(szInfoString) + 5 ];
	sprintf(buf,"I %s %s", szTime + 11, szInfoString);
	printit(buf);
	delete [] buf;

}

