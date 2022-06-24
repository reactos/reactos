/*
 *  CHOICE.C - internal command.
 *
 *
 *  History:
 *
 *    12 Aug 1999 (Eric Kohl)
 *        started.
 *
 *    01 Sep 1999 (Eric Kohl)
 *        Fixed help text.
 *
 *    26 Sep 1999 (Paolo Pantaleo)
 *        Fixed timeout.
 *
 *    02 Apr 2005 (Magnus Olsen)
 *        Remove hardcoded strings so that they can be translated.
 *
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_CHOICE


#define GC_TIMEOUT  -1
#define GC_NOKEY    0   //an event occurred but it wasn't a key pressed
#define GC_KEYREAD  1   //a key has been read


static INT
GetCharacterTimeout (LPTCH ch, DWORD dwMilliseconds)
{
//--------------------------------------------
//  Get a character from standard input but with a timeout.
//  The function will wait a limited amount
//  of time, then the function returns GC_TIMEOUT.
//
//  dwMilliseconds is the timeout value, that can
//  be set to INFINITE, so the function works like
//  stdio.h's getchar()

    HANDLE hInput;
    DWORD  dwRead;

    INPUT_RECORD lpBuffer;

    hInput = GetStdHandle (STD_INPUT_HANDLE);

    //if the timeout expired return GC_TIMEOUT
    if (WaitForSingleObject (hInput, dwMilliseconds) == WAIT_TIMEOUT)
        return GC_TIMEOUT;

    //otherwise get the event
    ReadConsoleInput (hInput, &lpBuffer, 1, &dwRead);

    //if the event is a key pressed
    if ((lpBuffer.EventType == KEY_EVENT) &&
        (lpBuffer.Event.KeyEvent.bKeyDown != FALSE))
    {
        //read the key
#ifdef _UNICODE
        *ch = lpBuffer.Event.KeyEvent.uChar.UnicodeChar;
#else
        *ch = lpBuffer.Event.KeyEvent.uChar.AsciiChar;
#endif
        return GC_KEYREAD;
    }

    //else return no key
    return GC_NOKEY;
}

static INT
IsKeyInString (LPTSTR lpString, TCHAR cKey, BOOL bCaseSensitive)
{
    LPTCH p = lpString;
    INT val = 0;

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
CommandChoice (LPTSTR param)
{
    LPTSTR lpOptions;
    TCHAR Options[6];
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

    INT GCret;
    TCHAR Ch;
    DWORD amount,clk;

    LoadString(CMD_ModuleHandle, STRING_CHOICE_OPTION, Options, 4);
    lpOptions = Options;

    if (_tcsncmp (param, _T("/?"), 2) == 0)
    {
        ConOutResPaging(TRUE,STRING_CHOICE_HELP);
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
    arg = split (param, &argc, FALSE, FALSE);

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
                    ConErrResPuts(STRING_CHOICE_ERROR);
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
                    s = &arg[i][4];
                }
                else
                {
                    cDefault = arg[i][2];
                    s = &arg[i][3];
                }

                if (*s != _T(','))
                {
                    ConErrResPuts(STRING_CHOICE_ERROR_TXT);
                    freep (arg);
                    return 1;
                }

                s++;
                nTimeout = _ttoi(s);
                bTimeout = TRUE;
            }
            else if (arg[i][0] == _T('/'))
            {
                ConErrResPrintf(STRING_CHOICE_ERROR_OPTION, arg[i]);
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

        for (i = 1; (unsigned)i < _tcslen (lpOptions); i++)
            ConOutPrintf (_T(",%c"), lpOptions[i]);

        ConOutPrintf (_T("]?"));
    }

    ConInFlush ();

    if (!bTimeout)
    {
        while (TRUE)
        {
            ConInKey (&ir);

            val = IsKeyInString (lpOptions,
#ifdef _UNICODE
                                 ir.Event.KeyEvent.uChar.UnicodeChar,
#else
                                 ir.Event.KeyEvent.uChar.AsciiChar,
#endif
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
        TRACE ("ErrorLevel: %d\n", nErrorLevel);
        return 0;
    }

    clk = GetTickCount ();
    amount = nTimeout*1000;

loop:
    GCret = GetCharacterTimeout (&Ch, amount - (GetTickCount () - clk));

    switch (GCret)
    {
        case GC_TIMEOUT:
            TRACE ("GC_TIMEOUT\n");
            TRACE ("elapsed %d msecs\n", GetTickCount () - clk);
            break;

        case GC_NOKEY:
            TRACE ("GC_NOKEY\n");
            TRACE ("elapsed %d msecs\n", GetTickCount () - clk);
            goto loop;

        case GC_KEYREAD:
            TRACE ("GC_KEYREAD\n");
            TRACE ("elapsed %d msecs\n", GetTickCount () - clk);
            TRACE ("read %c\n", Ch);
            if ((val=IsKeyInString(lpOptions,Ch,bCaseSensitive))==-1)
            {
                Beep (440, 50);
                goto loop;
            }
            cDefault=Ch;
            break;
    }

    TRACE ("exiting wait loop after %d msecs\n",
                GetTickCount () - clk);

    val = IsKeyInString (lpOptions, cDefault, bCaseSensitive);
    ConOutPrintf(_T("%c\n"), lpOptions[val]);

    nErrorLevel = val + 1;

    freep (arg);

    TRACE ("ErrorLevel: %d\n", nErrorLevel);

    return 0;
}
#endif /* INCLUDE_CMD_CHOICE */

/* EOF */
