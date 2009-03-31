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

	if (hInput == INVALID_HANDLE_VALUE)
		WARN ("Invalid input handle!!!\n");
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

	if (hInput == INVALID_HANDLE_VALUE)
		WARN ("Invalid input handle!!!\n");

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
	DWORD dwRead = 0;
	HANDLE hFile;

	LPTSTR p;
	PCHAR pBuf;

#ifdef _UNICODE
	pBuf = (PCHAR)cmd_alloc(dwLength - 1);
#else
	pBuf = lpInput;
#endif
	ZeroMemory (lpInput, dwLength * sizeof(TCHAR));
	hFile = GetStdHandle (STD_INPUT_HANDLE);
	GetConsoleMode (hFile, &dwOldMode);

	SetConsoleMode (hFile, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

	ReadFile (hFile, (PVOID)pBuf, dwLength - 1, &dwRead, NULL);

#ifdef _UNICODE
	MultiByteToWideChar(InputCodePage, 0, pBuf, dwRead, lpInput, dwLength - 1);
	cmd_free(pBuf);
#endif
	for (p = lpInput; *p; p++)
	{
		if (*p == _T('\x0d'))
		{
			*p = _T('\0');
			break;
		}
	}

	SetConsoleMode (hFile, dwOldMode);
}

static VOID ConWrite(TCHAR *str, DWORD len, DWORD nStdHandle)
{
	DWORD dwWritten;
	HANDLE hOutput = GetStdHandle(nStdHandle);

	if (WriteConsole(hOutput, str, len, &dwWritten, NULL))
		return;

	/* We're writing to a file or pipe instead of the console. Convert the
	 * string from TCHARs to the desired output format, if the two differ */
	if (bUnicodeOutput)
	{
#ifndef _UNICODE
		WCHAR buffer[len];
		len = MultiByteToWideChar(OutputCodePage, 0, str, len, buffer, len, NULL, NULL);
		str = (PVOID)buffer;
#endif
		WriteFile(hOutput, str, len * sizeof(WCHAR), &dwWritten, NULL);
	}
	else
	{
#ifdef _UNICODE
		CHAR buffer[len * MB_LEN_MAX];
		len = WideCharToMultiByte(OutputCodePage, 0, str, len, buffer, len * MB_LEN_MAX, NULL, NULL);
		str = (PVOID)buffer;
#endif
		WriteFile(hOutput, str, len, &dwWritten, NULL);
	}
}

VOID ConOutChar (TCHAR c)
{
	ConWrite(&c, 1, STD_OUTPUT_HANDLE);
}

VOID ConPuts(LPTSTR szText, DWORD nStdHandle)
{
	ConWrite(szText, _tcslen(szText), nStdHandle);
	ConWrite(_T("\n"), 1, nStdHandle);
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
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	ConWrite(szOut, _vstprintf(szOut, szFormat, arg_ptr), nStdHandle);
}

INT ConPrintfPaging(BOOL NewPage, LPTSTR szFormat, va_list arg_ptr, DWORD nStdHandle)
{
	INT len;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	TCHAR szOut[OUTPUT_BUFFER_SIZE];
	DWORD dwWritten;
	HANDLE hOutput = GetStdHandle(nStdHandle);

	/* used to count number of lines since last pause */
	static int LineCount = 0;

	/* used to see how big the screen is */
	int ScreenLines = 0;

	/* chars since start of line */
	int CharSL;

	int from = 0, i = 0;

	if(NewPage == TRUE)
		LineCount = 0;

	/* rest LineCount and return if no string have been given */
	if (szFormat == NULL)
		return 0;


	//get the size of the visual screen that can be printed too
	if (!GetConsoleScreenBufferInfo(hOutput, &csbi))
	{
		// we assuming its a file handle
		ConPrintf(szFormat, arg_ptr, nStdHandle);
		return 0;
	}
	//subtract 2 to account for "press any key..." and for the blank line at the end of PagePrompt()
	ScreenLines = (csbi.srWindow.Bottom  - csbi.srWindow.Top) - 4;
	CharSL = csbi.dwCursorPosition.X;

	//make sure they didnt make the screen to small
	if(ScreenLines<4)
	{
		ConPrintf(szFormat, arg_ptr, nStdHandle);
		return 0;
	}

	len = _vstprintf (szOut, szFormat, arg_ptr);

	while (i < len)
	{
		// Search until the end of a line is reached
		if (szOut[i++] != _T('\n') && ++CharSL < csbi.dwSize.X)
			continue;

		LineCount++;
		CharSL=0;

		if(LineCount >= ScreenLines)
		{
			WriteConsole(hOutput, &szOut[from], i-from, &dwWritten, NULL);
			from = i;

			if(PagePrompt() != PROMPT_YES)
			{
				return 1;
			}
			//reset the number of lines being printed
			LineCount = 0;
		}
	}

	WriteConsole(hOutput, &szOut[from], i-from, &dwWritten, NULL);

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

VOID ConOutResPrintf (UINT resID, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	va_list arg_ptr;

	va_start (arg_ptr, resID);
	LoadString(CMD_ModuleHandle, resID, szMsg, RC_STRING_MAX_SIZE);
	ConPrintf(szMsg, arg_ptr, STD_OUTPUT_HANDLE);
	va_end (arg_ptr);
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
	ConWrite(&c, 1, STD_ERROR_HANDLE);
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


VOID ConErrResPrintf (UINT resID, ...)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	va_list arg_ptr;

	va_start (arg_ptr, resID);
	LoadString(CMD_ModuleHandle, resID, szMsg, RC_STRING_MAX_SIZE);
	ConPrintf(szMsg, arg_ptr, STD_ERROR_HANDLE);
	va_end (arg_ptr);
}

VOID ConErrPrintf (LPTSTR szFormat, ...)
{
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	ConPrintf(szFormat, arg_ptr, STD_ERROR_HANDLE);
	va_end (arg_ptr);
}

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

	GetConsoleScreenBufferInfo (GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	*x = csbi.dwCursorPosition.X;
	*y = csbi.dwCursorPosition.Y;
}


SHORT GetCursorX (VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	return csbi.dwCursorPosition.X;
}


SHORT GetCursorY (VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	return csbi.dwCursorPosition.Y;
}


VOID GetScreenSize (PSHORT maxx, PSHORT maxy)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
	{
		csbi.dwSize.X = 80;
		csbi.dwSize.Y = 25;
	}

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
