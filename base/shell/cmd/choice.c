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
    lpBuffer.EventType = !KEY_EVENT;
    if (!ReadConsoleInput (hInput, &lpBuffer, 1, &dwRead) && GetLastError () == ERROR_INVALID_HANDLE)
    {
        *ch = '\0';
        if (ReadFile(hInput, ch, 1, &dwRead, NULL))
            return GC_KEYREAD;
    }

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
    LPTSTR lpText   = NULL, lpTextParam = NULL;
    UINT   iSkipTextIndex = 0, iTextIndex;
    BOOL   bNoPrompt = FALSE;
    BOOL   bCaseSensitive = FALSE;
    BOOL   bTimeout = FALSE;
    INT    nTimeout = 0;
    TCHAR  cDefault = _T('\0'), cDefParam = _T('\0');
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

    /* build parameter array */
    arg = split (param, &argc, FALSE, FALSE);

    /* evaluate arguments */
    nErrorLevel = 255;
    if (argc > 0)
    {
        for (i = 0; i < argc; i++)
        {
            if (_tcsnicmp (arg[i], _T("/s"), 2) == 0) /* DOS */
            {
                bCaseSensitive = TRUE;
            }
            else if (_tcsnicmp (arg[i], _T("/cs"), 3) == 0) /* NT */
            {
                bCaseSensitive = TRUE;
            }
            else if (_tcsnicmp (arg[i], _T("/c"), 2) == 0) /* Note: This eats /CS so it must come after */
            {
                if (arg[i][2] == _T(':'))
                    lpOptions = &arg[i][3];                 /* "/c:XYZ" (DOS and NT) */
                else if (arg[i][2])
                    lpOptions = &arg[i][2];                 /* "/cXYZ" (DOS) */
                else if (i + 1 < argc && *arg[i + 1] != _T('/'))
                    lpOptions = arg[iSkipTextIndex = ++i];  /* "/c XYZ" (NT) */

                if (!*lpOptions)
                {
                invalid_choice_characters:
                    ConErrResPuts(STRING_CHOICE_ERROR);
                    freep(arg);
                    return nErrorLevel;
                }
            }
            else if (_tcsnicmp (arg[i], _T("/m"), 2) == 0)  /* "/m msg" (NT) */
            {
                if (arg[i][2] == _T(':'))
                    lpText = lpTextParam = &arg[i][3];
                else if (i + 1 < argc && *arg[i + 1] != _T('/'))
                    lpText = lpTextParam = arg[++i];
                else
                {
                    ConErrResPrintf(STRING_CHOICE_ERROR_OPTION, arg[i]);
                    freep(arg);
                    return nErrorLevel;
                }
            }
            else if (_tcsnicmp (arg[i], _T("/n"), 2) == 0)
            {
                bNoPrompt = TRUE;
            }
            else if (_tcsnicmp (arg[i], _T("/d"), 2) == 0)  /* "/d:X" and "/d X" (NT) */
            {
                if (arg[i][2] == _T(':'))
                    cDefault = cDefParam = arg[i][3];
                else if (i + 1 < argc)
                    cDefault = cDefParam = arg[iSkipTextIndex = ++i][0];
                else
                    goto invalid_parameter_format;
            }
            else if (_tcsnicmp (arg[i], _T("/t"), 2) == 0)
            {
                LPTSTR s, end;
                TCHAR cSaveDefault = cDefault;

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
                    /* Just a number (NT syntax) */
                    cDefault = cSaveDefault;
                    if (arg[i][2] == _T(':'))
                        s = &arg[i][3];
                    else if (i + 1 < argc)
                        s = arg[iSkipTextIndex = ++i];

                    _tcstol(s, &end, 10);
                    if (end > s && !*end)
                        goto parse_timeout_seconds;

                failed_parse_timeout:
                    ConErrResPuts(STRING_CHOICE_ERROR_TXT);
                    freep (arg);
                    return nErrorLevel;
                }

                s++;
            parse_timeout_seconds:
                nTimeout = _ttoi(s);
                bTimeout = TRUE;
            }
            else if (arg[i][0] == _T('/'))
            {
            invalid_parameter_format:
                ConErrResPrintf(STRING_CHOICE_ERROR_OPTION, arg[i]);
                freep (arg);
                return nErrorLevel;
            }
        }
    }

    if (bTimeout)
    {
        if (!cDefault) /* NT /t syntax used without /d */
            goto failed_parse_timeout;
        if (nTimeout < 0 || (cDefParam && nTimeout > 9999)) /* DOS seems to be limited to 99, no ">" validation */
            goto failed_parse_timeout;
        if (IsKeyInString(lpOptions, cDefault, bCaseSensitive) < 0) /* The default must exist in the list of options */
            goto failed_parse_timeout;

        /* Duplicate choice characters are not allowed */
        for (p = lpOptions; *p; ++p)
        {
            if (IsKeyInString(p + 1, *p, bCaseSensitive) >= 0)
                goto invalid_choice_characters;
        }
    }

    /* The choice characters are printed uppercase when case-insensitive */
    if (!bCaseSensitive)
        _wcsupr(lpOptions);

    /* retrieve text */
    for (p = param, iTextIndex = 0; !lpTextParam; ++iTextIndex)
    {
        if (*p == _T('\0'))
            break;

        if (*p != _T('/') && (!iSkipTextIndex || iTextIndex > iSkipTextIndex))
        {
            lpText = p;
            break;
        }
        np = _tcschr (p, _T(' '));
        if (!np)
            break;
        p = np + 1;
    }

    /* print text */
    if (lpText)
        ConOutPrintf (_T("%s"), lpText);

    /* print options */
    if (bNoPrompt == FALSE)
    {
        LPCTSTR prefix = lpText && lpText == lpTextParam ? _T(" ") : _T("");
        ConOutPrintf (_T("%s[%c"), prefix, lpOptions[0]);

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
        return nErrorLevel;
    }

    clk = GetTickCount ();
    amount = nTimeout*1000;

loop:
    nTimeout = amount - (GetTickCount () - clk);
    if (nTimeout < 0)
        GCret = GC_TIMEOUT;
    else
        GCret = GetCharacterTimeout (&Ch, nTimeout);

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

    return nErrorLevel;
}
#endif /* INCLUDE_CMD_CHOICE */

/* EOF */
