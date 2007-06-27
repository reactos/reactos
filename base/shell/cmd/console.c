/*
 *  CONSOLE.C - console input/output functions.
 *
 *
 *  History:
 *
 *    20-Jan-1999 (Eric Kohl)
 *        started
 *
 *    03-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 *    01-Jul-2005 (Brandon Turner) <turnerb7@msu.edu>)
 *        Added ConPrintfPaging and ConOutPrintfPaging
 *
 *    02-Feb-2007 (Paolo Devoti) <devotip at gmail.com>)
 *        Fixed ConPrintfPaging
 */



#include <precomp.h>
#include "resource.h"


#define OUTPUT_BUFFER_SIZE  4096


UINT InputCodePage;
UINT OutputCodePage;


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
	ReadConsoleInput (hInput, &dummy, 1, &dwRead);
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
		ReadConsoleInput (hInput, lpBuffer, 1, &dwRead);
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
	MultiByteToWideChar(  InputCodePage, 0, pBuf, dwLength + 1, lpInput, dwLength + 1);
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
	WideCharToMultiByte( OutputCodePage, 0, ws, 2, as, 2, NULL, NULL);
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
	len = WideCharToMultiByte( OutputCodePage, 0, szText, len + 1, pBuf, len + 1, NULL, NULL) - 1;
#else
	pBuf = szText;
#endif
	WriteFile (GetStdHandle (nStdHandle),
	           pBuf,
	           len,
	           &dwWritten,
	           NULL);
	WriteFile (GetStdHandle (nStdHandle),
	           _T("\n"),
	           1,
	           &dwWritten,
	           NULL);
#ifdef _UNICODE
	free(pBuf);
#endif
}

VOID ConOutResPaging(BOOL NewPage, UINT resID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LoadString(CMD_ModuleHandle, resID, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintfPaging(NewPage, szMsg);
}

VOID ConOutResPuts (UINT resID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LoadString(CMD_ModuleHandle, resID, szMsg, RC_STRING_MAX_SIZE);

	ConPuts(szMsg, STD_OUTPUT_HANDLE);
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
	len = WideCharToMultiByte( OutputCodePage, 0, szOut, len + 1, pBuf, len + 1, NULL, NULL) - 1;
#else
	pBuf = szOut;
#endif

	WriteFile (GetStdHandle (nStdHandle),
	           pBuf,
	           len,
	           &dwWritten,
	           NULL);


#ifdef _UNICODE
	free(pBuf);
#endif
}

INT ConPrintfPaging(BOOL NewPage, LPTSTR szFormat, va_list arg_ptr, DWORD nStdHandle)
{
	INT len;
	PCHAR pBuf;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	DWORD dwWritten;

	/* used to count number of lines since last pause */
	static int LineCount = 0;

	/* used to see how big the screen is */
	int ScreenLines = 0;  

	/* the number of chars in a roow */
	int ScreenCol = 0;  

	/* chars since start of line */
	int CharSL = 0; 

	int i = 0;

	if(NewPage == TRUE)
		LineCount = 0;

	/* rest LineCount and return if no string have been given */
	if (szFormat == NULL)
		return 0;


	//get the size of the visual screen that can be printed too
	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		// we assuming its a file handle
		ConPrintf(szFormat, arg_ptr, nStdHandle);
		return 0;
	}
	//subtract 2 to account for "press any key..." and for the blank line at the end of PagePrompt()
	ScreenLines = (csbi.srWindow.Bottom  - csbi.srWindow.Top) - 4;
	ScreenCol = (csbi.srWindow.Right - csbi.srWindow.Left) + 1;

	//make sure they didnt make the screen to small 
	if(ScreenLines<4)
	{
		ConPrintf(szFormat, arg_ptr, nStdHandle);
		return 0;
	}

	len = _vstprintf (szOut, szFormat, arg_ptr);
#ifdef _UNICODE
	pBuf = malloc(len + 1);
	len = WideCharToMultiByte( OutputCodePage, 0, szOut, len + 1, pBuf, len + 1, NULL, NULL) - 1;
#else
	pBuf = szOut;
#endif

	for(i = 0; i < len; i++)
	{
		// search 'end of string' '\n' or 'end of screen line'
		for(; (i < len) && (pBuf[i] != _T('\n') && (CharSL<ScreenCol)) ; i++)
			CharSL++;

		WriteFile (GetStdHandle (nStdHandle),&pBuf[i-CharSL],sizeof(CHAR)*(CharSL+1),&dwWritten,NULL);
		LineCount++; 
		CharSL=0;

		if(LineCount >= ScreenLines)
		{
			if(_strnicmp(&pBuf[i], "\n", 2)!=0)
				WriteFile (GetStdHandle (nStdHandle),_T("\n"),sizeof(CHAR),&dwWritten,NULL); 

			if(PagePrompt() != PROMPT_YES)
			{
#ifdef _UNICODE
				free(pBuf);
#endif
				return 1;
			}
			//reset the number of lines being printed         
			LineCount = 0;
			CharSL=0;
		}

	}

#ifdef _UNICODE
	free(pBuf);
#endif
	return 0;
}

VOID ConErrFormatMessage (DWORD MessageId, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
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
		LoadString(CMD_ModuleHandle, STRING_CONSOLE_ERROR, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg);
	}
}

VOID ConOutFormatMessage (DWORD MessageId, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
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
		LoadString(CMD_ModuleHandle, STRING_CONSOLE_ERROR, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg);
	}
}

VOID ConOutPrintf (LPTSTR szFormat, ...)
{
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	ConPrintf(szFormat, arg_ptr, STD_OUTPUT_HANDLE);
	va_end (arg_ptr);
}

INT ConOutPrintfPaging (BOOL NewPage, LPTSTR szFormat, ...)
{
	INT iReturn;
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	iReturn = ConPrintfPaging(NewPage, szFormat, arg_ptr, STD_OUTPUT_HANDLE);
	va_end (arg_ptr);
	return iReturn;
}

VOID ConErrChar (TCHAR c)
{
	ConChar(c, STD_ERROR_HANDLE);
}


VOID ConErrResPuts (UINT resID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LoadString(CMD_ModuleHandle, resID, szMsg, RC_STRING_MAX_SIZE);
	ConPuts(szMsg, STD_ERROR_HANDLE);
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
