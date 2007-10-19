/*
 *  COLOR.C - color internal command.
 *
 *
 *  History:
 *
 *    13-Dec-1998 (Eric Kohl)
 *        Started.
 *
 *    19-Jan-1999 (Eric Kohl)
 *        Unicode ready!
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Redirection ready!
 *
 *    14-Oct-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        4nt's syntax implemented.
 *
 *    03-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Move all hardcoded strings to En.rc.
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_COLOR





VOID SetScreenColor (WORD wColor, BOOL bNoFill)
{
	DWORD dwWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coPos;


	if ((wColor & 0xF) == (wColor &0xF0) >> 4)
	{
		ConErrResPuts(STRING_COLOR_ERROR1);
	}
	else
	{
		if (bNoFill != TRUE)
		{
			GetConsoleScreenBufferInfo (hConsole, &csbi);

			coPos.X = 0;
			coPos.Y = 0;
			FillConsoleOutputAttribute (hConsole,
			                            (WORD)(wColor & 0x00FF),
			                            (csbi.dwSize.X)*(csbi.dwSize.Y),
			                            coPos,
			                            &dwWritten);
		}
		SetConsoleTextAttribute (hConsole, (WORD)(wColor & 0x00FF));
	}
}


/*
 * color
 *
 * internal dir command
 */
INT CommandColor (LPTSTR first, LPTSTR rest)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	if (_tcsncmp (rest, _T("/?"), 2) == 0)
	{
		ConOutResPaging(TRUE,STRING_COLOR_HELP1);
		return 0;
	}

	nErrorLevel = 0;

	if (rest[0] == _T('\0'))
	{
		/* set default color */
		wColor = wDefColor;
		SetScreenColor (wColor, FALSE);
		return 0;
	}


	if ( _tcslen(&rest[0])==1)
	{
	  if ( (_tcscmp(&rest[0], _T("0")) >=0 ) && (_tcscmp(&rest[0], _T("9")) <=0 ) )
	  {
        SetConsoleTextAttribute (hConsole, (WORD)_ttoi(rest));
		return 0;
	  }
	  else if ( (_tcscmp(&rest[0], _T("a")) >=0 ) && (_tcscmp(&rest[0], _T("f")) <=0 ) )
	  {
       SetConsoleTextAttribute (hConsole, (WORD) (rest[0] + 10 - _T('a')) );
	   return 0;
	  }
      else if ( (_tcscmp(&rest[0], _T("A")) >=0 ) && (_tcscmp(&rest[0], _T("F")) <=0 ) )
	  {
       SetConsoleTextAttribute (hConsole, (WORD) (rest[0] + 10 - _T('A')) );
	   return 0;
	  }
	  ConErrResPuts(STRING_COLOR_ERROR2);
	  nErrorLevel = 1;
	  return 1;
	}

	if (StringToColor(&wColor, &rest) == FALSE)
	{
		ConErrResPuts(STRING_COLOR_ERROR2);
		nErrorLevel = 1;
		return 1;
	}

	LoadString(CMD_ModuleHandle, STRING_COLOR_ERROR3, szMsg, RC_STRING_MAX_SIZE);
	ConErrPrintf(szMsg, wColor);

	if ((wColor & 0xF) == (wColor &0xF0) >> 4)
	{
		LoadString(CMD_ModuleHandle, STRING_COLOR_ERROR4, szMsg, RC_STRING_MAX_SIZE);
		ConErrPrintf(szMsg, wColor);
		nErrorLevel = 1;
		return 1;
	}

	/* set color */
	SetScreenColor(wColor,
	               (_tcsstr (rest,_T("/-F")) || _tcsstr (rest,_T("/-f"))));

	return 0;
}

#endif /* INCLUDE_CMD_COLOR */

/* EOF */
