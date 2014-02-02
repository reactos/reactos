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
 *    03-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Move all hardcoded strings in En.rc.
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_COLOR

BOOL SetScreenColor(WORD wColor, BOOL bNoFill)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coPos;

    /* Foreground and Background colors can't be the same */
    if ((wColor & 0x0F) == (wColor & 0xF0) >> 4)
        return FALSE;

    /* Fill the whole background if needed */
    if (bNoFill != TRUE)
    {
        GetConsoleScreenBufferInfo(hConsole, &csbi);

        coPos.X = 0;
        coPos.Y = 0;
        FillConsoleOutputAttribute(hConsole,
                                   wColor & 0x00FF,
                                   csbi.dwSize.X * csbi.dwSize.Y,
                                   coPos,
                                   &dwWritten);
    }

    /* Set the text attribute */
    SetConsoleTextAttribute(hConsole, wColor & 0x00FF);
    return TRUE;
}

/*
 * color
 *
 * internal dir command
 */
INT CommandColor(LPTSTR rest)
{
    WORD wColor = 0x00;

    /* The user asked for help */
    if (_tcsncmp(rest, _T("/?"), 2) == 0)
    {
        ConOutResPaging(TRUE, STRING_COLOR_HELP1);
        return 0;
    }

    /* Let's prepare %ERRORLEVEL% */
    nErrorLevel = 0;

    /* No parameter. Set the default colors */
    if (rest[0] == _T('\0'))
    {
        SetScreenColor(wDefColor, FALSE);
        return 0;
    }

    /* The parameter is just one character: Set color text */
    if (_tcslen(rest) == 1)
    {
        if ((rest[0] >= _T('0')) && (rest[0] <= _T('9')))
        {
            wColor = (WORD)_ttoi(rest);
        }
        else if ((rest[0] >= _T('a')) && (rest[0] <= _T('f')))
        {
            wColor = (WORD)(rest[0] + 10 - _T('a'));
        }
        else if ((rest[0] >= _T('A')) && (rest[0] <= _T('F')))
        {
            wColor = (WORD)(rest[0] + 10 - _T('A'));
        }
        else /* Invalid character */
        {
            ConOutResPaging(TRUE, STRING_COLOR_HELP1);
            nErrorLevel = 1;
            return 1;
        }
    }
    /* Color string: advanced choice: two-digits, "Color ON Color" , "Foreground ON Background" */
    else if (StringToColor(&wColor, &rest) == FALSE)
    {
        /* Invalid color string */
        ConOutResPaging(TRUE, STRING_COLOR_HELP1);
        nErrorLevel = 1;
        return 1;
    }

    /* Print the chosen color if we are in echo mode (NOTE: Not Windows-compliant) */
    if ((bc && bc->bEcho) || !bc)
    {
        ConErrResPrintf(STRING_COLOR_ERROR3, wColor);
    }

    /*
     * Set the chosen color. Use also the following advanced flag:
     * /-F to avoid changing already buffered foreground/background.
     */
    if (SetScreenColor(wColor, (_tcsstr(rest, _T("/-F")) || _tcsstr(rest, _T("/-f")))) == FALSE)
    {
        /* Failed because foreground and background colors were the same */
        ConErrResPuts(STRING_COLOR_ERROR1);
        nErrorLevel = 1;
        return 1;
    }

    /* Return success */
    return 0;
}

#endif /* INCLUDE_CMD_COLOR */

/* EOF */
