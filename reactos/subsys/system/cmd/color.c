/* $Id$
 *
 *  COLOR.C - color internal command.
 *
 *
 *  History:
 *
 *    13-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection ready!
 *
 *    14-Oct-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        4nt's syntax implemented
 *
 *    03-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_COLOR


static VOID ColorHelp (VOID)
{
	WCHAR szMsg[RC_STRING_MAX_SIZE];
	LoadString( GetModuleHandle(NULL), STRING_COLOR_HELP1, (LPTSTR) szMsg,sizeof(szMsg));
    ConOutPuts (_T((LPTSTR)szMsg));

}


VOID SetScreenColor (WORD wColor, BOOL bFill)
{
	DWORD dwWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coPos;
    WCHAR szMsg[RC_STRING_MAX_SIZE];

	if ((wColor & 0xF) == (wColor &0xF0) >> 4)
	{
	  LoadString( GetModuleHandle(NULL), STRING_COLOR_ERROR1, (LPTSTR) szMsg,sizeof(szMsg));
      ConErrPuts (_T((LPTSTR)szMsg));
    }
    else 
    {
	    if (bFill == TRUE)
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
	WCHAR szMsg[RC_STRING_MAX_SIZE];

	if (_tcsncmp (rest, _T("/?"), 2) == 0)
	{
		ColorHelp ();
		return 0;
	}

	if (rest[0] == _T('\0'))
	{
		/* set default color */
		wColor = wDefColor;
		SetScreenColor (wColor, TRUE);
		return 0;
	}

	if (StringToColor (&wColor, &rest) == FALSE)
	{
		LoadString( GetModuleHandle(NULL), STRING_COLOR_ERROR2, (LPTSTR) szMsg,sizeof(szMsg));
        ConErrPuts (_T((LPTSTR)szMsg));
		return 1;
	}

	LoadString( GetModuleHandle(NULL), STRING_COLOR_ERROR3, (LPTSTR) szMsg,sizeof(szMsg));
    ConErrPrintf (_T((LPTSTR)szMsg), wColor);

	if ((wColor & 0xF) == (wColor &0xF0) >> 4)
	{
		LoadString( GetModuleHandle(NULL), STRING_COLOR_ERROR4, (LPTSTR) szMsg,sizeof(szMsg));
        ConErrPrintf (_T((LPTSTR)szMsg), wColor);		
		return 1;
	}

	/* set color */
	SetScreenColor (wColor,
	                (_tcsstr (rest,_T("/F")) || _tcsstr (rest,_T("/f"))));

	return 0;
}

#endif /* INCLUDE_CMD_COLOR */

/* EOF */
