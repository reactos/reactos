/*
 *  CHOICE.C - internal command.
 *
 *
 *  History:
 *
 *    12 Aug 1999 (Eric Kohl)
 *        started.
 */

#include "config.h"

#ifdef INCLUDE_CMD_CHOICE

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tchar.h>

#include "cmd.h"
#include "batch.h"


static INT
IsKeyInString (LPTSTR lpString, TCHAR cKey, BOOL bCaseSensitive)
{
        PTCHAR p = lpString;
        INT     val = 0;

        while (*p)
        {
                if (bCaseSensitive)
                {
                        if (*p == cKey)
                                return val;
                }
                else
                {
                        if (_totlower (*p) == _totlower (cKey))
                                return val;
                }

                val++;
                p++;
        }

        return -1;
}


INT
CommandChoice (LPTSTR cmd, LPTSTR param)
{
        LPTSTR lpOptions = "YN";
        LPTSTR lpText    = NULL;
        BOOL   bNoPrompt = FALSE;
        BOOL   bCaseSensitive = FALSE;
        BOOL   bTimeout = FALSE;
        INT    nTimeout = 0;
        TCHAR  cDefault = _T('\0');
        INPUT_RECORD ir;
        LPTSTR p, np;
	LPTSTR *arg;
	INT    argc;
	INT    i;
        INT    val;
        
	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
                ConOutPuts (_T("Prompts the user to select an option.\n"
                               "\n"
                               "CHOICE  [/C[:]options][/N][/S][/T[:]c,nn][text]\n"
                               "\n"
                               "  /C[:]options  Allowed option characters. Default is YN.\n"
                               "  /N            The option characters and the question mark will\n"
                               "                not be appended.\n"
                               "  /S            Input is case sensitive.\n"
                               "  /T[:]c,nn     Default to c after nn seconds.\n"
                               "  text          Displayed prompt.\n"
                               "\n"
                               "ERRORLEVEL ...\n"));
		return 0;
	}

        /* retrieve text */
        p = param;

        while (TRUE)
        {
                if (*p == _T('\0'))
                        break;
                if (*p != _T('/'))
                {
                        lpText = p;
                        break;
                }
                np = _tcschr (p, _T(' '));
                if (!np)
                        break;
                p = np + 1;
        }

	/* build parameter array */
	arg = split (param, &argc);

        /* evaluate arguments */
        if (argc > 0)
        {
                for (i = 0; i < argc; i++)
                {
                        if (_tcsnicmp (arg[i], _T("/c"), 2) == 0)
                        {
                                if (arg[i][2] == _T(':'))
                                        lpOptions = &arg[i][3];
                                else
                                        lpOptions = &arg[i][2];

                                if (_tcslen (lpOptions) == 0)
                                {
                                        ConErrPuts (_T("CHOICE: Error!!"));
                                        freep (arg);
                                        return 1;
                                }
                        }
                        else if (_tcsnicmp (arg[i], _T("/n"), 2) == 0)
                        {
                                bNoPrompt = TRUE;
                        }
                        else if (_tcsnicmp (arg[i], _T("/s"), 2) == 0)
                        {
                                bCaseSensitive = TRUE;
                        }
                        else if (_tcsnicmp (arg[i], _T("/t"), 2) == 0)
                        {
                                LPTSTR s;

                                if (arg[i][2] == _T(':'))
                                {
                                        cDefault = arg[i][3];
                                        s = & arg[i][4];
                                }
                                else
                                {
                                        cDefault = arg[i][2];
                                        s = & arg[i][3];
                                }

                                if (*s != _T(','))
                                {
                                        ConErrPrintf (_T("Format Error: /T!!\n"));
                                        freep (arg);
                                        return 1;
                                }

                                s++;
                                nTimeout = _ttoi(s);
                                bTimeout = TRUE;
                        }
                        else if (arg[i][0] == _T('/'))
                        {
                                ConErrPrintf (_T("Illegal Option: %s"), arg[i]);
                                freep (arg);
                                return 1;
                        }
                }
        } 

        /* print text */
        if (lpText)
                ConOutPrintf (_T("%s"), lpText);

        /* print options */
        if (bNoPrompt == FALSE)
        {
                ConOutPrintf (_T("[%c"), lpOptions[0]);

                for (i = 1; i < _tcslen (lpOptions); i++)
                        ConOutPrintf (_T(",%c"), lpOptions[i]);

                ConOutPrintf (_T("]?"));
        }

        ConInFlush ();

        if (bTimeout)
        {
                if (WaitForSingleObject (GetStdHandle(STD_INPUT_HANDLE),
                                         nTimeout * 1000) == WAIT_TIMEOUT)
                {
                        val = IsKeyInString (lpOptions,
                                             cDefault,
                                             bCaseSensitive);

                        if (val >= 0)
                        {
                                ConOutPrintf (_T("%c\n"), lpOptions[val]);

                                nErrorLevel = val + 1;

                                freep (arg);

#ifdef DEBUG
                                DebugPrintf (_T("ErrorLevel: %d\n"), nErrorLevel);
#endif /* DEBUG */

                                return 0;
                        }

                }
        }

        while (TRUE)
        {
                ConInKey (&ir);

                val = IsKeyInString (lpOptions,
#ifdef _UNICODE
                                     ir.Event.KeyEvent.uChar.UnicodeChar,
#else
                                     ir.Event.KeyEvent.uChar.AsciiChar,
#endif /* _UNICODE */
                                     bCaseSensitive);

                if (val >= 0)
                {
                        ConOutPrintf (_T("%c\n"), lpOptions[val]);

                        nErrorLevel = val + 1;

                        break;
                }

                Beep (440, 50);
        }

        freep (arg);

#ifdef DEBUG
        DebugPrintf (_T("ErrorLevel: %d\n"), nErrorLevel);
#endif /* DEBUG */

	return 0;
}
#endif /* INCLUDE_CMD_CHOICE */

/* EOF */
