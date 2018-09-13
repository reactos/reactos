
/****************************************************************************/
/*                                                                          */
/*  RCL.C -                                                                 */
/*                                                                          */
/*    Windows 3.0 Resource Compiler - Lexical analyzer                      */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "rc.h"


#define EOLCHAR L';'
#define STRCHAR L'"'
#define CHRCHAR L'\''
#define SGNCHAR L'-'
#define iswhite( c ) ((c != SYMUSESTART) && (c != SYMDEFSTART) &&\
        ((WCHAR)c <= L' ') ? TRUE : FALSE)

static WCHAR  curChar;
static WCHAR  curCharFTB;   /* Cur char From Token Buf */
static PWCHAR CurPtrTB;
static PFILE  inpfh;
static int    curLin, curCol;

extern BOOL bExternParse;


/* Must be sorted */
KEY keyList[] =
{
    { L"ALT",              TKALT },
    { L"ASCII",            TKASCII },
    { L"AUTO3STATE",       TKAUTO3 },
    { L"AUTOCHECKBOX",     TKAUTOCHECK },
    { L"AUTORADIOBUTTON",  TKAUTORADIO },
    { L"BEGIN",            BEGIN },
    { L"BEDIT",            TKBEDIT },
    { L"BITMAP",           TKBITMAP },
    { L"BLOCK",            TKBLOCK },
    { L"BUTTON",           TKBUTTON },
    { L"CAPTION",          TKCAPTION },
    { L"CHARACTERISTICS",  TKCHARACTERISTICS },
    { L"CHECKBOX",         TKCHECKBOX },
    { L"CHECKED",          TKCHECKED },
    { L"CLASS",            TKCLASS },
    { L"COMBOBOX",         TKCOMBOBOX },
    { L"CONTROL",          TKCONTROL },
    { L"CTEXT",            TKCTEXT },
    { L"DEFPUSHBUTTON",    TKDEFPUSHBUTTON },
    { L"DISCARDABLE",      TKDISCARD },
    { L"DLGINCLUDE",       TKDLGINCLUDE },
    { L"DLGINIT",          TKDLGINIT },
    { L"EDIT",             TKEDIT },
    { L"EDITTEXT",         TKEDITTEXT },
    { L"END",              END },
    { L"EXSTYLE",          TKEXSTYLE },
    { L"FILEFLAGS",        TKFILEFLAGS },
    { L"FILEFLAGSMASK",    TKFILEFLAGSMASK },
    { L"FILEOS",           TKFILEOS },
    { L"FILESUBTYPE",      TKFILESUBTYPE },
    { L"FILETYPE",         TKFILETYPE },
    { L"FILEVERSION",      TKFILEVERSION },
    { L"FIXED",            TKFIXED },
    { L"FONT",             TKFONT },
    { L"GRAYED",           TKGRAYED },
    { L"GROUPBOX",         TKGROUPBOX },
    { L"HEDIT",            TKHEDIT },
    { L"HELP",             TKHELP },
    { L"ICON",             TKICON },
    { L"IEDIT",            TKIEDIT },
    { L"IMPURE",           TKIMPURE },
    { L"INACTIVE",         TKINACTIVE },
    { L"LANGUAGE",         TKLANGUAGE },
    { L"LISTBOX",          TKLISTBOX },
    { L"LOADONCALL",       TKLOADONCALL },
    { L"LTEXT",            TKLTEXT },
    { L"MENU",             TKMENU },
    { L"MENUBARBREAK",     TKBREAKWBAR },
    { L"MENUBREAK",        TKBREAK },
    { L"MENUITEM",         TKMENUITEM },
    { L"MESAGETABLE",      TKMESSAGETABLE },
    { L"MOVEABLE",         TKMOVEABLE },
    { L"NOINVERT",         TKNOINVERT },
    { L"NONSHARED",        TKIMPURE },
    { L"NOT",              TKNOT },
    { L"OWNERDRAW",        TKOWNERDRAW },
    { L"POPUP",            TKPOPUP },
    { L"PRELOAD",          TKPRELOAD },
    { L"PRODUCTVERSION",   TKPRODUCTVERSION },
    { L"PURE",             TKPURE },
    { L"PUSHBOX",          TKPUSHBOX },
    { L"PUSHBUTTON",       TKPUSHBUTTON },
    { L"RADIOBUTTON",      TKRADIOBUTTON },
    { L"RCDATA",           TKRCDATA },
    { L"RTEXT",            TKRTEXT },
    { L"SCROLLBAR",        TKSCROLLBAR },
    { L"SEPARATOR",        TKSEPARATOR },
    { L"SHARED",           TKPURE },
    { L"SHIFT",            TKSHIFT },
    { L"STATE3",           TK3STATE },
    { L"STATIC",           TKSTATIC },
    { L"STYLE",            TKSTYLE },
    { L"USERBUTTON",       TKUSERBUTTON },
    { L"VALUE",            TKVALUE },
    { L"VERSION",          TKVERSION },
    { L"VIRTKEY",          TKVIRTKEY },
    { NULL,                0 }
};


SKEY skeyList[] =
{
    { L',', COMMA },
    { L'|', OR },
    { L'(', LPAREN },
    { L')', RPAREN },
    { L'{', BEGIN },
    { L'}', END },
    { L'~', TILDE },
    { L'+', TKPLUS },
    { L'-', TKMINUS },
    { L'&', AND },
    { L'=', EQUAL },
    { EOFMARK, EOFMARK },
    { L'\000', 0 }
};


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  LexInit() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
LexInit(
    PFILE fh
    )
{
    /* zero errors so far */
    errorCount = 0;
    curLin = 1;
    curCol = 0;
    inpfh = fh;

    /* Read initial character */
    OurGetChar();

    return TRUE;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetCharFTB() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
WCHAR
GetCharFTB(
    void
    )
{
    return(curCharFTB = *CurPtrTB++);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  OurGetChar() -                                                           */
/*                                                                           */
/*  Read a character, treating semicolon as an end of line comment char      */
/*                                                                           */
/*---------------------------------------------------------------------------*/

WCHAR
OurGetChar(
    void
    )
{
    if ((LitChar() != EOFMARK) && (curChar == CHCOMMENT))
        // if comment, HARD LOOP until EOLN
        while ((LitChar() != EOFMARK) && (curChar != CHNEWLINE));

    return(curChar);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  FileChar() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
FileChar(
    void
    )
{
    static WCHAR rgchLine[MAXSTR];
    static int   ibNext = MAXSTR;
    int          cch, ch;

    if (ibNext >= MAXSTR) {
        ibNext = 0;
        cch = MyRead (inpfh, rgchLine, MAXSTR * sizeof(WCHAR));
        if (cch < (MAXSTR * sizeof(WCHAR))) {
            fclose(inpfh);
            // NULL terminate the input buffer
            *(rgchLine + (cch / sizeof(WCHAR))) = L'\0';
        }
    }

    if ((ch = rgchLine[ibNext]) != 0)
        ibNext++;

    return(ch);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CopyToken() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void
CopyToken(
    PTOKEN ptgt_token,
    PTOKEN psrc_token
    )
{
    ptgt_token->longval  = psrc_token->longval;
    ptgt_token->row      = psrc_token->row;
    ptgt_token->col      = psrc_token->col;
    ptgt_token->flongval = psrc_token->flongval;
    ptgt_token->val      = psrc_token->val;
    ptgt_token->type     = psrc_token->type;
    ptgt_token->realtype = psrc_token->realtype;

    wcscpy(ptgt_token->sym.name, psrc_token->sym.name);
    wcscpy(ptgt_token->sym.file, psrc_token->sym.file);
    ptgt_token->sym.line = psrc_token->sym.line;
    ptgt_token->sym.nID  = psrc_token->sym.nID;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  LitChar() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* Read a literal character, without interpreting EOL comments */

WCHAR
LitChar(
    void
    )
{
    static int  fNewLine = TRUE;
    int fIgnore = FALSE;
    int fBackSlash = FALSE;
    int fDot;
    PWCHAR      pch;
    WCHAR buf[ _MAX_PATH ];
    TOKEN token_save;

    for (; ; ) {
        switch (curChar = (WCHAR)FileChar()) {
            case 0:
                curChar = EOFMARK;
                goto char_return;

            case 0xFEFF:     // skip Byte Order Mark
                continue;

            case SYMDEFSTART:
            {
                int fNewLineSave = fNewLine;
                GetSymbolDef(TRUE, curChar);
                fNewLine = fNewLineSave;
                break;
            }

            case CHCARRIAGE:
                curChar = CHSPACE;
                if (!fIgnore)
                    goto char_return;
                break;

            case CHNEWLINE:
                fNewLine = TRUE;
                curLin++;
                {
                    static long lTotalLin = 0;
                    if ((lTotalLin++ & RC_COMPILE_UPDATE) == 0)
                        UpdateStatus(2, lTotalLin);
                }

                if (!fIgnore)
                    goto char_return;
                break;

                /* skip whitespace before #line - don't clear fNewLine */
            case CHSPACE:
            case CHTAB:
                if (!fIgnore)
                    goto char_return;
                break;

            case CHDIRECTIVE:
                if (fNewLine) {
                    WCHAR tch;

                    fDot = FALSE;

                    /* also, leave fNewLine set, since we read thru \n */

                    /* read the 'line' part */
                    if ((tch = (WCHAR)FileChar()) != L'l') {
                        if (tch == L'p') {
                            if (FileChar() != L'r')
                                goto DirectiveError;
                            if (FileChar() != L'a')
                                goto DirectiveError;
                            if (FileChar() != L'g')
                                goto DirectiveError;
                            if (FileChar() != L'm')
                                goto DirectiveError;
                            if (FileChar() != L'a')
                                goto DirectiveError;

                            /*
                            ** This is very specific, as any #pragma will
                            ** be a code_page pragma written by p0prepro.c.
                            */
                            CopyToken( &token_save, &token );

                            GetToken(FALSE);        /* get #pragma and ignore */
                            GetToken(FALSE);        /* get code_page and ignore */
                            GetToken(TOKEN_NOEXPRESSION);   /* get codepage value only*/
                                            /* don't check return value */
                            uiCodePage = token.val;     /* assume ok */
                            /* read through end of line */
                            while (curChar != CHNEWLINE) {
                                curChar = (WCHAR)FileChar();
                            }
                            CopyToken( &token, &token_save );
                            continue;
                        } else {
                            goto DirectiveError;
                        }
                    }
                    if (FileChar() != L'i')
                        goto DirectiveError;
                    if (FileChar() != L'n')
                        goto DirectiveError;
                    if (FileChar() != L'e')
                        goto DirectiveError;

                    /* up to filename, grabbing line number as we go */
                    /* note that curChar first contains '#', because */
                    /* we don't read a new character into curChar */
                    curLin = 0;
                    do {
                        if (curChar >= L'0' && curChar <= L'9') {
                            curLin *= 10;
                            curLin += curChar - L'0';
                        }
                        curChar = (WCHAR)FileChar();
                    } while (curChar != CHQUOTE && curChar != CHNEWLINE);

                    /* don't change curFile or fIgnore if this is just a
                     * #line <lineno>
                     */
                    if (curChar == CHNEWLINE)
                        break;

                    /* read the filename.  detect the presence of .c or .h */
                    pch = buf;
                    do {
                        curChar = (WCHAR)FileChar();
                        switch (towlower(curChar)) {

                            /* treat backslash like normal char, set flag. */
                            case L'\\':
                                if (fBackSlash) {
                                    fBackSlash = FALSE;
                                } else {
                                    fBackSlash = TRUE;
                                    fIgnore = FALSE;
                                    fDot = FALSE;
                                    *pch++ = curChar;
                                }
                                break;

                                /* line format sanity check: no embedded newlines */
                            case CHNEWLINE:
                            case 0:
DirectiveError:
                                LexError1(2101);

                                /* stop reading filename when we hit a quote */
                            case CHQUOTE:
                                break;

                                /* if we see a ., prepare to find extension */
                            case CHEXTENSION:
                                fBackSlash = FALSE;
                                fDot = TRUE;
                                *pch++ = curChar;
                                break;

                                /* if there's a C or H after a '.', its not RCINCLUDE'd */
                            case CHCSOURCE:
                            case CHCHEADER:
                                fBackSlash = FALSE;
                                fIgnore = fDot;
                                fDot = FALSE;
                                *pch++ = curChar;
                                break;

                                /* any other character in a file means the next character
                                won't be after a dot, and the last char up to now
                                wasn't C or H.
                                */

                            default:
                                fIgnore = FALSE;
                                fDot = FALSE;
                                *pch++ = curChar;
                                break;
                        }
                    } while (curChar != CHQUOTE);
                    *pch = 0;
                    WideCharToMultiByte(uiCodePage, 0, buf, -1, (LPSTR) curFile, _MAX_PATH, NULL, NULL);

                    /* read through end of line */
                    do {
                        curChar = (WCHAR)FileChar();
                    } while (curChar != CHNEWLINE);

                    break;
                }
                /* else, fall through, treat like normal char */

            default:
                fNewLine = FALSE;
                if (!fIgnore)
                    goto char_return;
        }
    }

char_return:
    if (bExternParse)
        *((WCHAR*) GetSpace(sizeof(WCHAR))) = curChar;

    return curChar;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetStr() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
GetStr(
    void
    )
{
    PWCHAR  s;
    WCHAR   ch;
    WCHAR   temptok[ MAXSTR ];
    SHORT   i = 0;
    int     inc;
    UCHAR   Octal_Num;
    UCHAR   HexNum;

    /* token type is string literal */
    token.realtype = STRLIT;

    /*
    **  NB:  FloydR
    **  The use of token.realtype is a hack for RCDATA.
    **
    **  When we converted RC to be Unicode-based, all the
    **  separate "case STRLIT:" code was removed, and the LSTRLIT
    **  cases took over for them.  Alternatively, we could have
    **  left the STRLIT case, but removed the code it accessed
    **  and move the STRLIT case prior/after the LSTRLIT case,
    **  since they were now identical.  They were removed in favor
    **  of smaller/faster code.
    **
    **  However, RCDATA still had a need to discern the difference,
    **  so I added token.realtype, set it to STRLIT in GetStr(),
    **  set it to LSTRLIT in GetLStr() (below), and check it in
    **  GetRCData() in rctg.c.
    **
    */
    token.type = LSTRLIT;
    token.val = 0;
    s = tokenbuf;

    /* read string until " or EOF */
    while (LitChar() != EOFMARK)  {
        if (curChar == STRCHAR)
            if (OurGetChar() != STRCHAR)
                goto gotstr;

        if (token.val++ == MAXSTR)
            LexError1(2102); //"string literal too long"
        else
            *s++ = curChar;
    }
    if (curChar == EOFMARK)
        LexError1(2103); //"unexpected end of file in string literal"

gotstr:
    *s++ = 0;
    s = tokenbuf;

    /* process escape characters in the string */

    while (*s != 0)  {
        if (*s == L'\\')  {
            s++;
            if (*s == L'\\')
                temptok[i++] = L'\\';
            else if (*s == L'T' || *s == L't')
                temptok[i++] = L'\011';            /* Tab */
            else if (*s == 0x0a)                   /* continuation slash */
                ; /* ignore and let it go trough the s++ at the end so we skip the 0x0a char*/
            else if (*s == L'A' || *s == L'a')
                temptok[i++] = L'\010';            /* Right Align */
            else if (*s == L'n')
                temptok[i++] = fMacRsrcs ? 13 : 10;   /* linefeed */
            else if (*s == L'r')
                temptok[i++] = fMacRsrcs ? 10 : 13;   /* carriage return */
            else if (*s == L'"')
                temptok[i++] = L'"';               /* quote character */
            else if (*s == L'X' || *s == L'x')  {  /* Hexidecimal digit */
                USHORT wCount;

                HexNum = 0;
                ++s;
                for (wCount = 2 ;
                    wCount && iswxdigit((ch=(WCHAR)towupper(*s)));
                    --wCount)  {
                    if (ch >= L'A')
                        inc = ch - L'A' + 10;
                    else
                        inc = ch - L'0';
                    HexNum = HexNum * 16 + inc;
                    s++;
                }
                MultiByteToWideChar(uiCodePage, MB_PRECOMPOSED, (LPCSTR) &HexNum, 1, &temptok[i], 1);
                i++;
                s--;
            } else if (*s >= L'0' && *s <= L'7') {    /* octal character */
                USHORT wCount;

                Octal_Num = 0;
                for (wCount = 3; wCount && *s >= L'0' && *s <= L'7'; --wCount)  {
                    Octal_Num = (Octal_Num * 8 + (*s - L'0'));
                    s++;
                }
                MultiByteToWideChar(uiCodePage, MB_PRECOMPOSED, (LPCSTR) &Octal_Num, 1, &temptok[i], 1);
                i++;
                s--;
            }
            else {
                temptok[i++] = L'\\';
                s--;
            }
        } else
            temptok[i++] = *s;
        s++;
    }

    /* zero terminate */
    temptok[i] = L'\0';
    memcpy ( tokenbuf, temptok, sizeof(WCHAR)*(i + 1));
    token.val = (USHORT)i;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetLStr() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
GetLStr(
    void
    )
{
    PWCHAR  s;
    WCHAR   ch;
    WCHAR   temptok[ MAXSTR ];
    SHORT   i = 0;
    int     inc;
    int     Octal_Num;
    int     HexNum;

    /* token type is string literal */
    token.realtype = token.type = LSTRLIT;
    token.val = 0;
    s = tokenbuf;

    /* read string until " or EOF */
    while (LitChar() != EOFMARK)  {
        if (curChar == STRCHAR)
            if (OurGetChar() != STRCHAR)
                goto gotstr;

        if (token.val++ == MAXSTR)
            LexError1(2102); //"string literal too long"
        else
            *s++ = curChar;
    }
    if (curChar == EOFMARK)
        LexError1(2103); //"unexpected end of file in string literal"
    if (token.val >= 256) {
        SendError("\n");
        SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(4205), curFile, token.row);
        SendError(Msg_Text);
    }

gotstr:
    *s++ = 0;
    s = tokenbuf;

    /* process escape characters in the string */

    while (*s != 0)  {
        if (*s == L'\\')  {
            s++;
            if (*s == L'\\')
                temptok[i++] = L'\\';
            else if (*s == L'T' || *s == L't')
                temptok[i++] = L'\011';            /* Tab */
            else if (*s == L'A' || *s == L'a')
                temptok[i++] = L'\010';            /* Right Align */
            else if (*s == L'n')
                temptok[i++] = fMacRsrcs ? 13 : 10;   /* linefeed */
            else if (*s == L'r')
                temptok[i++] = fMacRsrcs ? 10 : 13;   /* carriage return */
            else if (*s == L'"')
                temptok[i++] = L'"';               /* quote character */
            else if (*s == L'X' || *s == L'x')  {  /* Hexidecimal digit */
                USHORT wCount;

                HexNum = 0;
                ++s;
                for (wCount = 4 ;
                    wCount && iswxdigit((ch=(WCHAR)towupper(*s)));
                    --wCount)  {
                    if (ch >= L'A')
                        inc = ch - L'A' + 10;
                    else
                        inc = ch - L'0';
                    HexNum = HexNum * 16 + inc;
                    s++;
                }
                temptok[i++] = (WCHAR)HexNum;
                s--;
            }
            else if (*s >= L'0' && *s <= L'7') {    /* octal character */
                USHORT wCount;

                Octal_Num = 0;
                for (wCount = 7; wCount && *s >= L'0' && *s <= L'7'; --wCount)  {
                    Octal_Num = (Octal_Num * 8 + (*s - L'0'));
                    s++;
                }
                temptok[i++] = (WCHAR)Octal_Num;
                s--;
            }

        }
        else
            temptok[i++] = *s;
        s++;
    }

    /* zero terminate */
    temptok[i] = L'\0';
    token.val = (USHORT)i;
    memcpy ( tokenbuf, temptok, sizeof(WCHAR)*(i + 1));
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetToken() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
GetToken(
    int fReportError
    )
{
    for (; ; )  {
        /* skip whitespace */
        while (iswhite( curChar))
            OurGetChar();

        /* take care of 'random' symbols use */
        if (curChar == SYMUSESTART)
            GetSymbol(fReportError, curChar);
        token.sym.name[0] = L'\0';

        /* remember location of token */
        token.row = curLin;
        token.col = curCol;

        /* determine if token is EOF, number, string, or keyword */
        token.type = EOFMARK;
        switch (curChar) {
            case EOFMARK:
                break;

            case SGNCHAR:
            case L'~':
                if (fReportError & TOKEN_NOEXPRESSION)
                    GetNumNoExpression();
                else
                    GetNum();
                break;

            case STRCHAR:
                GetStr();
                break;

            default:
                if (curChar == L'(' && !(fReportError & TOKEN_NOEXPRESSION))
                    GetNum();
                else if (iswdigit( curChar)) {
                    if (fReportError & TOKEN_NOEXPRESSION)
                        GetNumNoExpression();
                    else
                        GetNum();

                    if (curChar == SYMUSESTART)
                        GetSymbol(fReportError, curChar);
                } else {
                    if (!GetKwd( fReportError))
                        continue;
                    if (token.type == TKLSTR) {
                        GetLStr();
                        break;
                    }
                }
        }

        break;
    }

    return token.type;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetXNum() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* get hexadecimal number */

LONG
GetXNum(
    void
    )
{
    LONG n = 0;

    while (iswxdigit (GetCharFTB()))
        n = n * 16 + ( ((curCharFTB = (WCHAR)towupper(curCharFTB)) >= L'A') ?
            (WCHAR)(curCharFTB - L'A' + 10) :
            (WCHAR)(curCharFTB - L'0'));
    return (n);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetONum() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* get octal number */

LONG
GetONum(
    void
    )
{
    LONG n = 0;

    while (GetCharFTB() >= L'0' && curCharFTB <= L'7')
        n = n * 8 + (curCharFTB - L'0');
    return (n);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetDNum() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* get decimal number */

LONG
GetDNum(
    void
    )
{
    LONG n = 0;

    while (iswdigit(curCharFTB)) {
        n = n * 10 + (curCharFTB - L'0');
        GetCharFTB();
    }
    return (n);
}


PWSTR
GetWord(
    PWSTR pStr
    )
{
    WCHAR   ch;
    PSKEY   pskey;

    *pStr++ = curCharFTB = curChar;
    while (TRUE) {
        ch = OurGetChar();

        if (ch <= L' ')
            goto FoundBreak;

        switch (ch) {
            case EOFMARK:
            case EOLCHAR:
            case STRCHAR:
            case CHRCHAR:
                goto FoundBreak;

            default:
                for (pskey = skeyList; pskey->skwd; pskey++)
                    if (pskey->skwd == ch)
                        goto FoundBreak;
        }

        *pStr++ = ch;
    }

FoundBreak:
    *pStr = 0;

    return(pStr);
}


/*  GetNumFTB
 *      This function was previously added as a hack to handle converting
 *      radices.  I'm treating this as a (ugly) black box to read a number.
 */

VOID
GetNumFTB(
    void
    )
{
    int signFlag;
    USHORT wNotFlag;
    LONG n;

    /* Small hack to support NOT:  If we have a tilde, skip whitespace
     *  before the number.
     */
    if (curChar == L'~')
        while (iswhite(curChar))
            OurGetChar();

    /* Get the entire number in tokenbuf before computing radix */
    GetWord(tokenbuf);

    /* Skip the first char.  It is already in curCharFTB */
    CurPtrTB = tokenbuf + 1;

    /* mark token type as numeric literal */
    token.type = NUMLIT;

    /* find sign of number */
    if (curCharFTB == SGNCHAR)  {
        signFlag = TRUE;
        GetCharFTB();
    } else {
        signFlag = FALSE;
    }

    /* Check for a NOT (~) */
    if (curCharFTB == L'~') {
        wNotFlag = TRUE;
        GetCharFTB();
    } else {
        wNotFlag = FALSE;
    }

    /* determine radix of number */
    if (curCharFTB == L'0')  {
        GetCharFTB();
        if (curCharFTB == L'x')
            n = GetXNum();
        else if (curCharFTB == L'o')
            n = GetONum();
        else
            n = GetDNum();
    } else {
        n = GetDNum();
    }

    /* find size of number */
    if ((curCharFTB == L'L') || (curCharFTB == L'l'))  {
        token.flongval = TRUE;
        GetCharFTB();
    } else {
        token.flongval = FALSE;
    }

    /* account for sign */
    if (signFlag)
        n = -n;

    /* Account for the NOT */
    if (wNotFlag)
        n = ~n;

    /* Set longval regardless of flongval because Dialog Styles
     *  always have to be be long
     */
    token.longval = n;
    token.val = (USHORT)n;
}


/* ----- Static information needed for parsing ----- */
static int      wLongFlag;
static int      nParenCount;

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetNum() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
GetNum(
    void
    )
{
    LONG lValue;

    /* Initialize */
    wLongFlag = 0;
    nParenCount = 0;

    /* Return the number */
    lValue = GetExpression();

    /* Make sure we had matched parens */
    if (nParenCount)
        ParseError1(1013); //"Mismatched parentheses"

    /* Return as the proper token */
    if (wLongFlag)
        token.flongval = TRUE;
    token.type = NUMLIT;
    token.longval = lValue;
    token.val = (USHORT)lValue;
}


/*  GetNumNoExpression
 *      Gets a number without doing expression parsing on it.
 */

VOID
GetNumNoExpression(
    VOID
    )
{
    /* Call the single number parser */
    GetNumFTB();
}


/*  GetExpression
 *      Gets an expression, which is defined as any number of
 *      operators and operands inside one set of parens.
 */

LONG
GetExpression(
    VOID
    )
{
    LONG op1;
    LONG op2;
    WCHAR byOperator;
    UINT wFlags;

    /* Get the first operand */
    op1 = GetOperand();

    /* take care of symbol use */
    if (curChar == SYMUSESTART) {
        GetSymbol(TRUE, curChar);
        token.sym.nID = token.val;
    }

    /* Loop until end of expression */
    for (; ; ) {
        /* Get the operator */
        wFlags = GetOperator(&byOperator);

        /* If this is a right paren, dec the count */
        if (byOperator == L')') {
            /* Bring the paren count back down */
            --nParenCount;

            /* Skip the paren and any trailing whitespace */
            OurGetChar();
            SkipWhitespace();
        }

        /* If this isn't an operator, we're done with the expression */
        if (!wFlags) {
            token.sym.nID = (unsigned)op1;
            return op1;
        }
        token.sym.name[0] = L'\0';

        /* Get the second operand */
        op2 = GetOperand();

        /* Compute the value of the expression */
        switch (byOperator) {
            case L'+':
                op1 += op2;
                break;

            case L'-':
                op1 -= op2;
                break;

            case L'&':
                op1 &= op2;
                break;

            case L'|':
                op1 |= op2;
                break;
        }
    }
}


/*  GetOperand
 *      Gets an operand, which may either be a single number or may
 *      be an entire expression.
 */

LONG
GetOperand(
    VOID
    )
{

    /* Check to see if we need to descend a level */
    if (curChar == L'(') {
        /* Bump paren count so we can match them up */
        ++nParenCount;

        /* Skip past the paren char */
        OurGetChar();
        SkipWhitespace();

        /* Return the value of the computed expression for the operand */
        return GetExpression();
    }

    /* If this isn't a number, return an error */
    if (curChar != L'-' && curChar != L'~' && !iswdigit(curChar)) {
        GetKwd(FALSE);
        ParseError2(2237, tokenbuf);
        return 0;
    }

    /* Get the number in the token structure */
    GetNumFTB();

    /* See if we need to force the result long */
    if (token.flongval)
        wLongFlag = TRUE;

    /* Skip trailing whitespace */
    SkipWhitespace();

    /* Return the value */
    return token.longval;
}


/*  GetOperator
 *      Gets the next character and decides if it should be an operator.
 *      If it should, it returns TRUE, which causes the expression
 *      parser to continue.  Otherwise, it returns FALSE which causes
 *      the expression parser to pop up a level.
 */

int
GetOperator(
    PWCHAR pOperator
    )
{
    static WCHAR byOps[] = L"+-|&";
    PWCHAR pOp;

    /* take care of symbol use */
    if (curChar == SYMUSESTART)
        GetSymbol(TRUE, curChar);

    /* See if this character is an operator */
    pOp = wcschr(byOps, curChar);
    *pOperator = curChar;

    /* If we didn't find it, get out */
    if (!pOp)
        return FALSE;

    /* Otherwise, read trailing whitespace */
    OurGetChar();
    SkipWhitespace();

    /* Return the operator */
    return TRUE;
}


/*  SkipWhitespace
 *      Skips past whitespace in the current stream.
 */

VOID
SkipWhitespace(
    VOID
    )
{
    while (iswhite(curChar))
        OurGetChar();
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetKwd() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
GetKwd(
    int fReportError
    )
{
    PSKEY sk;

    /* see if a special character */

    for (sk = &skeyList[ 0 ]; sk->skwd; sk++) {
        if (curChar == sk->skwd)  {
            token.type = (UCHAR)sk->skwdval;
            token.val = 0;
            OurGetChar();
            return (token.type >= FIRSTKWD);
        }
    }

    /* else read characters up to the next seperator */
    GetWord(tokenbuf);

    // Check for TKLSTR -- new for NT
    if (!tokenbuf[1] && (towupper(tokenbuf[0]) == L'L') && (curChar == STRCHAR)) {
        token.type = TKLSTR;
        return TRUE;
    }

    /* look up keyword in table */
    if ((token.val = FindKwd( tokenbuf)) != (USHORT)-1) {
        token.type = (UCHAR)token.val;
    } else if (fReportError)  {
        LexError2(2104, (PCHAR)tokenbuf); //"undefined keyword or key name: %ws"
        return FALSE;
    }
    else
        token.type = 0;

    return TRUE;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  FindKwd() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

USHORT
FindKwd(
    PWCHAR str
    )
{
    PKEY   k;
    int    t;

    /* linear search the keyword table for the key */
    for (k = &keyList[0]; k->kwd; k++)
        if (!(t = _wcsicmp( str, k->kwd)))
            return k->kwdval;
        else if (t < 0)
            break;

    /* if not found, return -1 as keyword id */
    return (USHORT)-1;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  LexError1() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void
LexError1(
    int iMsg
    )
{
    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(iMsg), curFile, curLin);
    SendError(Msg_Text);
    quit("\n");
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  LexError2() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void
LexError2(
    int iMsg,
    PCHAR str
    )
{
    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(iMsg), curFile, curLin, str);
    SendError(Msg_Text);
    quit("\n");
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetNameOrd() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* For reading in resource names and types.  */
int
GetNameOrd(
    void
    )
{
    PWCHAR pch;
    int fString;

    /* get space delimited string */
    if (!GetGenText())
        return FALSE;

    /* convert to upper case */
    _wcsupr(tokenbuf);

    /* is it a string or number */
    for (pch=tokenbuf,fString=0 ; *pch ; pch++ )
    if (!iswdigit(*pch))
        fString = 1;

    /* determine if ordinal */
    if (tokenbuf[0] == L'0' && tokenbuf[1] == L'X') {
        int         HexNum;
        int         inc;
        USHORT      wCount;
        PWCHAR      s;

        HexNum = 0;
        s = &tokenbuf[2];
        for (wCount = 4 ; wCount && iswxdigit(*s) ; --wCount)  {
            if (*s >= L'A')
                inc = *s - L'A' + 10;
            else
                inc = *s - L'0';
            HexNum = HexNum * 16 + inc;
            s++;
        }
        token.val = (USHORT)HexNum;
    } else if (fString) {
        token.val = 0;
    } else {
       token.val = (USHORT)wcsatoi(tokenbuf);
    }

    return TRUE;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetGenText() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* returns a pointer to a string of generic text */

PWCHAR
GetGenText(
    void
    )
{
    PWCHAR  s;

    s = tokenbuf;

    /* skip white space */
    while (iswhite(curChar))
        OurGetChar();

    if (curChar == EOFMARK)  {
        token.type = EOFMARK;
        return NULL;
    }

    /* random symbol */
    if (curChar == SYMUSESTART)
        GetSymbol(TRUE, curChar);
    token.sym.name[0] = L'\0';

    /* read space delimited string */
    *s++ = curChar;
    while (( LitChar() != EOFMARK) && ( !iswhite(curChar)))
        *s++ = curChar;
    *s++ = 0;     /* put a \0 on the end of the string */

    OurGetChar();    /* read in the next character        */
    if (curChar == EOFMARK)
        token.type = EOFMARK;

    if (curChar == SYMUSESTART) {
        GetSymbol(TRUE, curChar);
        token.sym.nID = token.val;
    }

    return (tokenbuf);
}
