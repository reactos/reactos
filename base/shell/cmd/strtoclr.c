/*
 *  STRTOCLR.C - read color (for color command and other)
 *
 *
 *  History:
 *
 *    07-Oct-1999 (Paolo Pantaleo)
 *        Started.
 *
 */

/*
 * Only
 * BOOL StringToColor(LPWORD lpColor, LPTSTR*str)
 * has to be called.
 * Other are internal service functions.
 */

#include "precomp.h"

#define _B FOREGROUND_BLUE
#define _G FOREGROUND_GREEN
#define _R FOREGROUND_RED
#define _I FOREGROUND_INTENSITY


/* Return values for chop_blank */
#define CP_OK               0
#define CP_BLANK_NOT_FOUND  1
#define CP_END_OF_STRING    2

/* NOTE: See the description for these flags in the StringToColor()'s description */
#define SC_HEX 0x0100
#define SC_TXT 0x0200


typedef struct _CLRTABLE
{
    LPTSTR  name;
    WORD    val;
} CLRTABLE;


CLRTABLE clrtable[] =
{
    {_T("bla"), 0           },
    {_T("blu"), _B          },
    {_T("gre"), _G          },
    {_T("cya"), _B|_G       },
    {_T("red"), _R          },
    {_T("mag"), _B|_R       },
    {_T("yel"), _R|_G       },
    {_T("whi"), _R|_G|_B    },
    {_T("gra"), _I          },

    {_T("0")  , 0           },
    {_T("2")  , _G          },
    {_T("3")  , _B|_G       },
    {_T("4")  , _R          },
    {_T("5")  , _B|_R       },
    {_T("6")  , _R|_G       },
    {_T("7")  , _R|_G|_B    },

    {_T("8")  , _I          },
    {_T("9")  , _I|_B       },
    {_T("10") , _I|_G       },
    {_T("11") , _I|_B|_G    },
    {_T("12") , _I|_R       },
    {_T("13") , _I|_B|_R    },
    {_T("14") , _I|_R|_G    },
    {_T("15") , _I|_R|_G|_B },

    /*
     * Note that 1 is at the end of list
     * to avoid to confuse it with 10-15
     */
    {_T("1")  , _B          },

    /* Cyan synonym */
    {_T("aqu"), _B|_G       },
    /* Magenta synonym */
    {_T("pur"), _B|_R       },

    {_T("")   ,0},
};


/*
 * Move string pointer to next word (skip all spaces).
 * On error return nonzero value.
 */
static
INT chop_blank(LPTSTR *arg_str)
{
    LPTSTR str;
    str = _tcschr(*arg_str,_T(' '));
    if (!str)
    {
        str = _tcschr (*arg_str, _T('\0'));
        if (str != NULL)
            *arg_str=str;
        return CP_BLANK_NOT_FOUND;
    }

    while(_istspace(*str))
        str++;

    if (*str == _T('\0'))
    {
        *arg_str=str;
        return CP_END_OF_STRING;
    }

    *arg_str = str;

    return CP_OK;
}


/*
 * Read a color value in hex (like win nt's cmd syntax).
 * If an error occurs return -1.
 */
static
WORD hex_clr(LPTSTR str)
{
    WORD ret= (WORD)-1;
    TCHAR ch;

    ch = str[1];

    if (_istdigit(ch))
        ret = ch-_T('0');
    else
    {
        ch=_totupper(ch);

        if (  ch >= _T('A') && ch <= _T('F')  )
            ret = ch-_T('A')+10;
        else
            return (WORD)-1;
    }

    ch = str[0];

    if (_istdigit(ch))
        ret |= (ch-_T('0')) << 4;
    else
    {
        ch=_totupper(ch);

        if (  ch >= _T('A') && ch <= _T('F')  )
            ret |= (ch-_T('A')+10) <<4;
        else
            return (WORD)-1;
    }

    return ret;
}


/*
 * Read a color value from a string (like 4nt's syntax).
 * If an error occurs return -1.
 */
static
WORD txt_clr(LPTSTR str)
{
    INT i;

    for(i = 0; *(clrtable[i].name); i++)
        if (_tcsnicmp(str, clrtable[i].name, _tcslen(clrtable[i].name)) == 0)
            return clrtable[i].val;

    return (WORD)-1;
}


/* Search for "x on y" */
static
WORD str_to_color(LPTSTR* arg_str)
{
    LPTSTR str;
    BOOL bBri;

    WORD tmp_clr,ret_clr;

    str = *arg_str;

    if (!(*str))
        return (WORD)-1;

    /* foreground */
    bBri = FALSE;

    if (_tcsnicmp(str,_T("bri"),3) == 0)
    {
        bBri = TRUE;

        if (chop_blank(&str))
            return (WORD)-1;
    }

    if ((tmp_clr = txt_clr(str)) == (WORD)-1)
    {
        return (WORD)-1;
    }

    /* skip spaces and "on" keyword */
    if (chop_blank(&str) || chop_blank(&str))
        return (WORD)-1;

    ret_clr = tmp_clr | (bBri << 3);

    /* background */
    bBri = FALSE;

    if (_tcsnicmp(str,_T("bri"),3) == 0 )
    {
        bBri = TRUE;

        if (chop_blank(&str))
            return (WORD)-1;
    }

    if ( (tmp_clr = txt_clr(str)) == (WORD)-1 )
        return (WORD)-1;

    chop_blank(&str);

    *arg_str = str;

    /* NOTE: See the note on SC_HEX in the StringToColor()'s description */
    return /* SC_HEX | */ ret_clr | tmp_clr << 4 | bBri << 7;
}


/**** Main function ****/
/*
 * The only parameter is arg_str, a pointer to a string.
 * The string is modified so it will begin to first word after
 * color specification
 * (only the char* is moved, no chars in the string are modified).
 *
 * **** NOTE: The following functionality is deactivated ****
 * it returns the color in the l.o. byte, plus two flags in the
 * h.o. byte, they are:
 * SC_HEX win nt's cmd syntax (for example a0)
 * SC_TXT 4nt's syntax ( "bri gre on bla" or "10 on 0")
 * **********************************************************
 *
 * If success also move the LPTSTR to end of
 * string that specify color.
 */
BOOL StringToColor(LPWORD lpColor, LPTSTR*str)
{
    WORD wRet;

    wRet = str_to_color (str);
    if (wRet == (WORD)-1)
    {
        wRet=hex_clr (*str);
        chop_blank (str);
        if (wRet == (WORD)-1)
            return FALSE;
    }

    *lpColor = wRet;

    return TRUE;
}

/* EOF */
