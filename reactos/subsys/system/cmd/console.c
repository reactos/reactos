/* $Id: console.c,v 1.7 2004/06/23 19:32:17 weiden Exp $
 *
 *  CONSOLE.C - console input/output functions.
 *
 *
 *  History:
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        started
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cmd.h"


#define OUTPUT_BUFFER_SIZE  4096


VOID ConInDisable (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	DWORD dwMode;

	GetConsoleMode (hInput, &dwMode);
	dwMode &= ~ENABLE_PROCESSED_INPUT;
	SetConsoleMode (hInput, dwMode);
}


VOID ConInEnable (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	DWORD dwMode;

	GetConsoleMode (hInput, &dwMode);
	dwMode |= ENABLE_PROCESSED_INPUT;
	SetConsoleMode (hInput, dwMode);
}


VOID ConInDummy (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD dummy;
	DWORD  dwRead;

#ifdef _DEBUG
	if (hInput == INVALID_HANDLE_VALUE)
		DebugPrintf (_T("Invalid input handle!!!\n"));
#endif /* _DEBUG */
#ifdef __REACTOS__
	/* ReadConsoleInputW isn't implwmented within ROS. */
	ReadConsoleInputA (hInput, &dummy, 1, &dwRead);
#else
	ReadConsoleInput (hInput, &dummy, 1, &dwRead);
#endif
}

VOID ConInFlush (VOID)
{
	FlushConsoleInputBuffer (GetStdHandle (STD_INPUT_HANDLE));
}


VOID ConInKey (PINPUT_RECORD lpBuffer)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	DWORD  dwRead;

#ifdef _DEBUG
	if (hInput == INVALID_HANDLE_VALUE)
		DebugPrintf (_T("Invalid input handle!!!\n"));
#endif /* _DEBUG */

	do
	{
#ifdef __REACTOS__
		/* ReadConsoleInputW isn't implwmented within ROS. */
		ReadConsoleInputA (hInput, lpBuffer, 1, &dwRead);
#else
		ReadConsoleInput (hInput, lpBuffer, 1, &dwRead);
#endif
		if ((lpBuffer->EventType == KEY_EVENT) &&
			(lpBuffer->Event.KeyEvent.bKeyDown == TRUE))
			break;
	}
	while (TRUE);
}



VOID ConInString (LPTSTR lpInput, DWORD dwLength)
{
	DWORD dwOldMode;
	DWORD dwRead;
	HANDLE hFile;

	LPTSTR p;
	DWORD  i;
	PCHAR pBuf;

#ifdef _UNICODE
	pBuf = (PCHAR)malloc(dwLength);
#else
	pBuf = lpInput;
#endif
	ZeroMemory (lpInput, dwLength * sizeof(TCHAR));
	hFile = GetStdHandle (STD_INPUT_HANDLE);
	GetConsoleMode (hFile, &dwOldMode);

	SetConsoleMode (hFile, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

	ReadFile (hFile, (PVOID)pBuf, dwLength, &dwRead, NULL);

#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, 0, pBuf, dwLength + 1, lpInput, dwLength + 1);
#endif
	p = lpInput;
	for (i = 0; i < dwRead; i++, p++)
	{
		if (*p == _T('\x0d'))
		{
			*p = _T('\0');
			break;
		}
	}

#ifdef _UNICODE
	free(pBuf);
#endif

	SetConsoleMode (hFile, dwOldMode);
}

static VOID ConChar(TCHAR c, DWORD nStdHandle)
{
	DWORD dwWritten;
	CHAR cc;
#ifdef _UNICODE
	CHAR as[2];
	WCHAR ws[2];
	ws[0] = c;
	ws[1] = 0;
	WideCharToMultiByte(CP_ACP, 0, ws, 2, as, 2, NULL, NULL);
	cc = as[0];
#else
	cc = c;
#endif
	WriteFile (GetStdHandle (nStdHandle),
	           &cc,
	           1,
	           &dwWritten,
	           NULL);
}

VOID ConOutChar (TCHAR c)
{
	ConChar(c, STD_OUTPUT_HANDLE);
}

VOID ConPuts(LPTSTR szText, DWORD nStdHandle)
{
	DWORD dwWritten;
	PCHAR pBuf;
	INT len;

	len = _tcslen(szText);
#ifdef _UNICODE
	pBuf = malloc(len + 1);
	len = WideCharToMultiByte(CP_ACP, 0, szText, len + 1, pBuf, len + 1, NULL, NULL) - 1;
#else
	pBuf = szText;
#endif
	WriteFile (GetStdHandle (nStdHandle),
	           pBuf,
	           len,
	           &dwWritten,
	           NULL);
	WriteFile (GetStdHandle (nStdHandle),
	           "\n",
	           1,
	           &dwWritten,
	           NULL);
#ifdef UNICODE
	free(pBuf);
#endif
}

VOID ConOutPuts (LPTSTR szText)
{
	ConPuts(szText, STD_OUTPUT_HANDLE);
}


VOID ConPrintf(LPTSTR szFormat, va_list arg_ptr, DWORD nStdHandle)
{
	INT len;
	PCHAR pBuf;
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	DWORD dwWritten;

	len = _vstprintf (szOut, szFormat, arg_ptr);
#ifdef _UNICODE
	pBuf = malloc(len + 1);
	len = WideCharToMultiByte(CP_ACP, 0, szOut, len + 1, pBuf, len + 1, NULL, NULL) - 1;
#else
	pBuf = szOut;
#endif
	WriteFile (GetStdHandle (nStdHandle),
	           pBuf,
	           len,
	           &dwWritten,
	           NULL);
#ifdef UNICODE
	free(pBuf);
#endif
}

VOID ConOutFormatMessage (DWORD MessageId, ...)
{
	DWORD ret;
	LPTSTR text;
	va_list arg_ptr;
	
	va_start (arg_ptr, MessageId);
	ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	       NULL,
	       MessageId,
	       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	       (LPTSTR) &text,
	       0,
	       &arg_ptr);
	
	va_end (arg_ptr);
	if(ret > 0)
	{
		ConErrPuts (text);
		LocalFree(text);
	}
	else
	{
		ConErrPrintf (_T("Unknown error: %d\n"), MessageId);
	}
}

VOID ConOutPrintf (LPTSTR szFormat, ...)
{
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	ConPrintf(szFormat, arg_ptr, STD_OUTPUT_HANDLE);
	va_end (arg_ptr);
}

VOID ConErrChar (TCHAR c)
{
	ConChar(c, STD_ERROR_HANDLE);
}


VOID ConErrPuts (LPTSTR szText)
{
	ConPuts(szText, STD_ERROR_HANDLE);
}


VOID ConErrPrintf (LPTSTR szFormat, ...)
{
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	ConPrintf(szFormat, arg_ptr, STD_ERROR_HANDLE);
	va_end (arg_ptr);
}

#ifdef _DEBUG
VOID DebugPrintf (LPTSTR szFormat, ...)
{
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	ConPrintf(szFormat, arg_ptr, STD_ERROR_HANDLE);
	va_end (arg_ptr);
#if 0
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	OutputDebugString (szOut);
	va_end (arg_ptr);
#endif
}
#endif /* _DEBUG */

VOID SetCursorXY (SHORT x, SHORT y)
{
	COORD coPos;

	coPos.X = x;
	coPos.Y = y;
	SetConsoleCursorPosition (GetStdHandle (STD_OUTPUT_HANDLE), coPos);
}


VOID GetCursorXY (PSHORT x, PSHORT y)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (hConsole, &csbi);

	*x = csbi.dwCursorPosition.X;
	*y = csbi.dwCursorPosition.Y;
}


SHORT GetCursorX (VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (hConsole, &csbi);

	return csbi.dwCursorPosition.X;
}


SHORT GetCursorY (VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (hConsole, &csbi);

	return csbi.dwCursorPosition.Y;
}


VOID GetScreenSize (PSHORT maxx, PSHORT maxy)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (hConsole, &csbi);

	if (maxx)
		*maxx = csbi.dwSize.X;
	if (maxy)
		*maxy = csbi.dwSize.Y;
}


VOID SetCursorType (BOOL bInsert, BOOL bVisible)
{
	CONSOLE_CURSOR_INFO cci;

	cci.dwSize = bInsert ? 10 : 99;
	cci.bVisible = bVisible;

	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE), &cci);
}

/* EOF */
