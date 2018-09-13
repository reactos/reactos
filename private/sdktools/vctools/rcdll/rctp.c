/****************************************************************************/
/*                                                                          */
/*  RCTP.C -                                                                */
/*                                                                          */
/*    Windows 3.0 Resource Compiler - Resource Parser                       */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "rc.h"


static BOOL fComma;

/* Dialog template format :

        dialogName DIALOGEX x, y, cx, cy [, helpID]
        [style ...]
        [exStyle ...]
        [FONT height, name [, [weight] [, [italic [, [charset]]]]]]
        [caption ...]
        [menu ...]
        [memFlags [pure] [discard n] [preload]]
        BEGIN
            [CONTROL "text", id, BUTTON | STATIC | EDIT | LISTBOX | SCROLLBAR | COMBOBOX | "class", style, x, y, cx, cy]
            [FONT height, name [, [weight] [, [italic]]]]
            [BEGIN
                data-element-1 [,
                data-element-2 [,
                ... ]]
            END]

            [LTEXT     "text", id, x, y, cx, cy]
            [RTEXT     "text", id, x, y, cx, cy]
            [CTEXT     "text", id, x, y, cx, cy]

            [AUTO3STATE         "text", id, x, y, cx, cy]
            [AUTOCHECKBOX       "text", id, x, y, cx, cy]
            [AUTORADIOBUTTON    "text", id, x, y, cx, cy]
            [CHECKBOX           "text", id, x, y, cx, cy]
            [PUSHBOX            "text", id, x, y, cx, cy]
            [PUSHBUTTON         "text", id, x, y, cx, cy]
            [RADIOBUTTON        "text", id, x, y, cx, cy]
            [STATE3             "text", id, x, y, cx, cy]
            [USERBUTTON         "text", id, x, y, cx, cy]

            [EDITTEXT   id, x, y, cx, cy]
            [BEDIT      id, x, y, cx, cy]
            [HEDIT      id, x, y, cx, cy]
            [IEDIT      id, x, y, cx, cy]
            ...
        END

        MenuName MENUEX
        BEGIN
            [MENUITEM "text" [, [id] [, [type] [, [state]]]]]
            [POPUP    "text" [, [id] [, [type] [, [state] [, [help id]]]]]
            BEGIN
                [MENUITEM "text" [, [id] [, [type] [, [state]]]]]
                ...
            END]
            ...
        END

    Menu template format

                MenuName MENU
                BEGIN
                        [MENUITEM "text", id [option, ...]]
                        [POPUP    "text" [, option, ...]
                                BEGIN
                                   [MENUITEM "text", id [option, ...]]
                                   ...
                                END ]
                                ...
                END
*/

/* Dialog template format :

          dialogname DIALOG  x, y, cx, cy
          [language ...]
          [style ...]
          [caption ...  ]
          [menu ... ]
          [memflags [pure] [discard n] [preload]]
          begin
                [CONTROL "text", id, BUTTON | STATIC | EDIT | LISTBOX | SCROLLBAR | COMBOBOX | "class", style, x, y, cx, cy]

                [LTEXT     "text", id, x, y, cx, cy]
                [RTEXT     "text", id, x, y, cx, cy]
                [CTEXT     "text", id, x, y, cx, cy]

                [CHECKBOX     "text", id, x, y, cx, cy]
                [PUSHBUTTON   "text", id, x, y, cx, cy]
                [RADIOBUTTON  "text", id, x, y, cx, cy]

                [EDITTEXT  id, x, y, cx, cy]
                ...
          end

   Menu template format

        MenuName MENU
        BEGIN
            [MENUITEM "text", id [option, ...]]
            [POPUP    "text" [, option, ...]
                BEGIN
                   [MENUITEM "text", id [option, ...]]
                   ...
                END ]
                ...
        END
*/


#define CTLSTYLE(s) (WS_CHILD | WS_VISIBLE | (s))

/* list of control id's to check for duplicates */
PDWORD  pid;
int     cidMac;
int     cidMax;

BOOL
CheckStr(
    PWCHAR pStr
    )
{
    if (token.type == STRLIT || token.type == LSTRLIT) {
        if (token.val > MAXTOKSTR-1) {
            SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(4208), curFile, token.row);
            SendError(Msg_Text);
            tokenbuf[MAXTOKSTR-1] = TEXT('\0');
            token.val = MAXTOKSTR-2;
        }
        memcpy(pStr, tokenbuf, (token.val+1)*sizeof(WCHAR));

        return(TRUE);
    }
    return(FALSE);
}


// ----------------------------------------------------------------------------
//
//  GetDlgValue
//
// ----------------------------------------------------------------------------

SHORT
GetDlgValue(
    void
    )
{
    SHORT sVal;

    if (!GetFullExpression(&sVal, GFE_ZEROINIT | GFE_SHORT))
        ParseError1(2109); //"Expected Numerical Dialog constant"

    return(sVal);
}

void
GetCoords(
    PSHORT x,
    PSHORT y,
    PSHORT cx,
    PSHORT cy
    )
{
    *x = GetDlgValue();
    if (token.type == COMMA)
        GetToken(TOKEN_NOEXPRESSION);
    *y = GetDlgValue();
    if (token.type == COMMA)
        GetToken(TOKEN_NOEXPRESSION);
    *cx= GetDlgValue();
    if (token.type == COMMA)
        GetToken(TOKEN_NOEXPRESSION);
    *cy= GetDlgValue();
}

typedef struct tagCTRLTYPE {
    WORD    type;
    DWORD   dwStyle;
    BYTE    bCode;
    BYTE    fHasText;
}   CTRLTYPE;

CTRLTYPE ctrlTypes[] = {
    { TKGROUPBOX,       BS_GROUPBOX,                    BUTTONCODE,     TRUE  },
    { TKPUSHBUTTON,     BS_PUSHBUTTON | WS_TABSTOP,     BUTTONCODE,     TRUE  },
    { TKDEFPUSHBUTTON,  BS_DEFPUSHBUTTON | WS_TABSTOP,  BUTTONCODE,     TRUE  },
    { TKCHECKBOX,       BS_CHECKBOX | WS_TABSTOP,       BUTTONCODE,     TRUE  },
    { TKRADIOBUTTON,    BS_RADIOBUTTON,                 BUTTONCODE,     TRUE  },
    { TKAUTO3,          BS_AUTO3STATE | WS_TABSTOP,     BUTTONCODE,     TRUE  },
    { TKAUTOCHECK,      BS_AUTOCHECKBOX | WS_TABSTOP,   BUTTONCODE,     TRUE  },
    { TKAUTORADIO,      BS_AUTORADIOBUTTON,             BUTTONCODE,     TRUE  },
    { TKPUSHBOX,        BS_PUSHBOX | WS_TABSTOP,        BUTTONCODE,     TRUE  },
    { TK3STATE,         BS_3STATE | WS_TABSTOP,         BUTTONCODE,     TRUE  },
    { TKUSERBUTTON,     BS_USERBUTTON | WS_TABSTOP,     BUTTONCODE,     TRUE  },
    { TKLTEXT,          ES_LEFT | WS_GROUP,             STATICCODE,     TRUE  },
    { TKRTEXT,          ES_RIGHT | WS_GROUP,            STATICCODE,     TRUE  },
    { TKCTEXT,          ES_CENTER | WS_GROUP,           STATICCODE,     TRUE  },
    { TKICON,           SS_ICON,                        STATICCODE,     TRUE  },
    { TKBEDIT,          ES_LEFT | WS_BORDER | WS_TABSTOP, 0,            FALSE },
    { TKHEDIT,          ES_LEFT | WS_BORDER | WS_TABSTOP, 0,            FALSE },
    { TKIEDIT,          ES_LEFT | WS_BORDER | WS_TABSTOP, 0,            FALSE },
    { TKEDITTEXT,       ES_LEFT | WS_BORDER | WS_TABSTOP, EDITCODE,     FALSE },
    { TKLISTBOX,        WS_BORDER | LBS_NOTIFY,         LISTBOXCODE,    FALSE },
    { TKCOMBOBOX,       0,                              COMBOBOXCODE,   FALSE },
    { TKSCROLLBAR,      0,                              SCROLLBARCODE,  FALSE }
};

#define C_CTRLTYPES (sizeof(ctrlTypes) / sizeof(CTRLTYPE))

// ----------------------------------------------------------------------------
//
//  GetDlgItems(fDlgEx) -
//
// ----------------------------------------------------------------------------

int
GetDlgItems(
    BOOL fDlgEx
    )
{
    CTRL ctrl;
    int i;

    cidMac = 0;
    cidMax = 100;
    pid = (PDWORD) MyAlloc(sizeof(DWORD)*cidMax);
    if (!pid)
        return FALSE;

    GetToken(TRUE);

    /* read all the controls in the dialog */

    ctrl.id = 0L;  // initialize the control's id to 0

    while (token.type != END) {
        ctrl.dwHelpID = 0L;
        ctrl.dwExStyle = 0L;
        ctrl.dwStyle = WS_CHILD | WS_VISIBLE;
        ctrl.text[0] = 0;
        ctrl.fOrdinalText = FALSE;

        if (token.type == TKCONTROL) {
            ParseCtl(&ctrl, fDlgEx);
        } else {
            for (i = 0; i < C_CTRLTYPES; i++)
                if (token.type == ctrlTypes[i].type)
                    break;

            if (i == C_CTRLTYPES) {
                ParseError1(2111); //"Invalid Control type : ", tokenbuf
                return(FALSE);
            }

            ctrl.dwStyle |= ctrlTypes[i].dwStyle;
            if (fMacRsrcs &&
                (token.type == TKPUSHBUTTON ||
                token.type == TKDEFPUSHBUTTON ||
                token.type == TKCHECKBOX ||
                token.type == TKAUTO3 ||
                token.type == TKAUTOCHECK ||
                token.type == TKPUSHBOX ||
                token.type == TK3STATE ||
                token.type == TKUSERBUTTON))
            {
                ctrl.dwStyle &= ~WS_TABSTOP;
            }
            if (ctrlTypes[i].bCode) {
                ctrl.Class[0] = 0xFFFF;
                ctrl.Class[1] = ctrlTypes[i].bCode;
            } else {
                CheckStr(ctrl.Class);
            }

            if (ctrlTypes[i].fHasText)
                GetCtlText(&ctrl);

            // find the ID and the coordinates
            GetCtlID(&ctrl, fDlgEx);
            GetCoords(&ctrl.x, &ctrl.y, &ctrl.cx, &ctrl.cy);

            // get optional style, exstyle, and helpid
            if (token.type == COMMA) {
                GetToken(TOKEN_NOEXPRESSION);
                GetFullExpression(&ctrl.dwStyle, 0);
            }
        }

        if (token.type == COMMA) {
            GetToken(TOKEN_NOEXPRESSION);
            GetFullExpression(&ctrl.dwExStyle, 0);

            if (fDlgEx && (token.type == COMMA)) {
                GetToken(TOKEN_NOEXPRESSION);
                GetFullExpression(&ctrl.dwHelpID, GFE_ZEROINIT);
            }
        }

        SetUpItem(&ctrl, fDlgEx); /* gen the code for it  */

        if (fDlgEx && (token.type == BEGIN)) {
            /* align any CreateParams are there */
            //WriteAlign(); not yet!!!

            // we're ok passing NULL in for pRes here because PreBeginParse
            // won't have to use pRes
            // Note that passing fDlgEx is actually redundant since it
            // will always be TRUE here, but we'll do it in case someone
            // else ever calls SetItemExtraCount
            SetItemExtraCount(GetRCData(NULL), fDlgEx);
            GetToken(TOKEN_NOEXPRESSION);
        }
    }
    MyFree(pid);
    return TRUE;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetDlg() -                                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
GetDlg(
    PRESINFO pRes,
    PDLGHDR pDlg,
    BOOL fDlgEx
    )
{
    /* initialize and defaults */
    pDlg->dwExStyle = pRes->exstyleT;
    pDlg->dwStyle = WS_POPUPWINDOW | WS_SYSMENU;
    pDlg->MenuName[0] = 0;
    pDlg->Title[0] = 0;
    pDlg->Class[0] = 0;
    pDlg->fOrdinalMenu = FALSE;
    pDlg->fClassOrdinal = FALSE;
    pDlg->pointsize = 0;

    // get x, y, cx, cy
    GetCoords(&pDlg->x, &pDlg->y, &pDlg->cx, &pDlg->cy);

    /* get optional parameters */
    if (!DLexOptionalArgs(pRes, pDlg, fDlgEx))
        return FALSE;

    if (pDlg->pointsize)
        pDlg->dwStyle |= DS_SETFONT;
    else
        pDlg->dwStyle &= ~DS_SETFONT;

    /* output header to the resource buffer */
    SetUpDlg(pDlg, fDlgEx);

    /* make sure we have a BEGIN */
    if (token.type != BEGIN)
        ParseError1(2112); //"BEGIN expected in Dialog"

    /* get the dialog items */
    GetDlgItems(fDlgEx);

    if (fMacRsrcs)
        SwapItemCount();

    /* make sure this ended on an END */
    if (token.type != END)
        ParseError1(2113); //"END expected in Dialog"

    return (TRUE);
}



typedef struct tagCTRLNAME {
    BYTE    bCode;
    WORD    wType;
    PWCHAR  pszName;
} CTRLNAME;

CTRLNAME    ctrlNames[] = {
    { BUTTONCODE,    TKBUTTON,    L"button"    },
    { EDITCODE,      TKEDIT,      L"edit"      },
    { STATICCODE,    TKSTATIC,    L"static"    },
    { LISTBOXCODE,   TKLISTBOX,   L"listbox"   },
    { SCROLLBARCODE, TKSCROLLBAR, L"scrollbar" },
    { COMBOBOXCODE,  TKCOMBOBOX,  L"combobox"  }
};

#define C_CTRLNAMES (sizeof(ctrlNames) / sizeof(CTRLNAME))

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      ParseCtl() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

// for a control of the form CTL

void
ParseCtl(
    PCTRL LocCtl,
    BOOL fDlgEx
    )
{   /* by now we've read the CTL */
    int i;

    /* get the control text and identifier */
    GetCtlText(LocCtl);
    GetCtlID(LocCtl, fDlgEx);

    if (token.type == NUMLIT) {
        LocCtl->Class[0] = (char) token.val;
        LocCtl->Class[1] = 0;
    } else if (token.type == LSTRLIT) {
        // We will now convert class name strings to short form magic
        // numbers. These magic numbers are order dependent as defined in
        // USER. This provides some space savings in resource files.
        for (i = C_CTRLNAMES; i; ) {
            if (!_wcsicmp(tokenbuf, ctrlNames[--i].pszName))
                goto Found1;
        }
        CheckStr(LocCtl->Class);
    } else {
        for (i = C_CTRLNAMES; i; ) {
            if (token.type == ctrlNames[--i].wType)
                goto Found1;
        }
        ParseError1(2114); //"Expected control class name"

Found1:
        LocCtl->Class[0] = 0xFFFF;
        LocCtl->Class[1] = ctrlNames[i].bCode;
    }

    /* get the style bits */
    GetTokenNoComma(TOKEN_NOEXPRESSION);
    GetFullExpression(&LocCtl->dwStyle, 0);

    /* get the coordinates of the control */
    ICGetTok();
    GetCoords(&LocCtl->x, &LocCtl->y, &LocCtl->cx, &LocCtl->cy);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetCtlText() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
GetCtlText(
    PCTRL pLocCtl
    )
{
    GetTokenNoComma(TOKEN_NOEXPRESSION);
    if (CheckStr(pLocCtl->text)) {
        pLocCtl->fOrdinalText = FALSE;
        token.sym.name[0] = L'\0';
        token.sym.nID = 0;
    } else if (token.type == NUMLIT) {
        wcsitow(token.val, pLocCtl->text, 10);
        pLocCtl->fOrdinalText = TRUE;
        WriteSymbolUse(&token.sym);
    } else {
        ParseError1(2115); //"Text string or ordinal expected in Control"
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetCtlID() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
GetCtlID(
    PCTRL pLocCtl,
    BOOL fDlgEx
    )
{
    WORD    wGFE = GFE_ZEROINIT;
    int i;

    ICGetTok();

    WriteSymbolUse(&token.sym);

    if (!fDlgEx)
        wGFE |= GFE_SHORT;

    if (GetFullExpression(&pLocCtl->id, wGFE)) {
        if (!fDlgEx && pLocCtl->id != (DWORD)(WORD)-1 ||
             fDlgEx && pLocCtl->id != (DWORD)-1) {
            for (i=0 ; i<cidMac ; i++) {
                if (pLocCtl->id == *(pid+i)) {
                    i = (int)pLocCtl->id;
                    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(2182),
                            curFile, token.row, i);
                    SendError(Msg_Text);
                    break;
                }
            }
            if (cidMac == cidMax) {
                PDWORD pidNew;

                cidMax += 100;
                pidNew = (PDWORD) MyAlloc(cidMax*sizeof(DWORD));
                memcpy(pidNew, pid, cidMac*sizeof(DWORD));
                MyFree(pid);
                pid = pidNew;
            }
            *(pid+cidMac++) = pLocCtl->id;
        }
    } else {
        ParseError1(2116); //"Expecting number for ID"
    }

    if (token.type == COMMA)
        ICGetTok();
}


// ----------------------------------------------------------------------------
//
//  DLexOptionArgs(pRes, fDlgEx) -
//
// ----------------------------------------------------------------------------
BOOL
DLexOptionalArgs(
    PRESINFO pRes,
    PDLGHDR pDlg,
    BOOL fDlgEx
    )
{
    /* read all the optional dialog items */

    if (fDlgEx && (token.type == COMMA)) {
        GetToken(TOKEN_NOEXPRESSION);
        GetFullExpression(&pDlg->dwHelpID, GFE_ZEROINIT);
    }

    while (token.type != BEGIN) {
        switch (token.type) {
            case TKLANGUAGE:
                pRes->language = GetLanguage();
                GetToken(FALSE);
                break;

            case TKVERSION:
                GetToken(FALSE);
                if (token.type != NUMLIT)
                    ParseError1(2139);
                pRes->version = token.longval;
                GetToken(FALSE);
                break;

            case TKCHARACTERISTICS:
                GetToken(FALSE);
                if (token.type != NUMLIT)
                    ParseError1(2140);
                pRes->characteristics = token.longval;
                GetToken(FALSE);
                break;

            case TKSTYLE:
                // If CAPTION statement preceded STYLE statement, then we
                // already must have WS_CAPTION bits set in the "style"
                // field and we must not lose it;

                if ((pDlg->dwStyle & WS_CAPTION) == WS_CAPTION)
                    pDlg->dwStyle = WS_CAPTION;
                else
                    pDlg->dwStyle = 0;

                GetTokenNoComma(TOKEN_NOEXPRESSION);
                GetFullExpression(&pDlg->dwStyle, 0);
                break;

            case TKEXSTYLE:
                GetTokenNoComma(TOKEN_NOEXPRESSION);
                GetFullExpression(&pDlg->dwExStyle, 0);
                break;

            case TKCAPTION:
                DGetTitle(pDlg);
                break;

            case TKMENU:
                DGetMenuName(pDlg);
                break;

            case TKCLASS:
                DGetClassName(pDlg);
                break;

            case TKFONT:
                DGetFont(pDlg, fDlgEx);
                break;

            default:
                ParseError1(2112); //"BEGIN expected in dialog");
                return FALSE;
        }
    }
    return TRUE;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  DGetFont() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void
DGetFont(
    PDLGHDR pDlg,
    BOOL fDlgEx
    )
{
    WORD w;
    int i;

    GetToken(TRUE);
    if (!GetFullExpression(&pDlg->pointsize, GFE_ZEROINIT | GFE_SHORT))
        ParseError1(2117); //"Expected numeric point size"

    if (token.type == COMMA)
        ICGetTok();

    if (!CheckStr(pDlg->Font))
        ParseError1(2118); //"Expected font face name"

    if (_wcsicmp(pDlg->Font, L"System") &&
        szSubstituteFontName[0] != UNICODE_NULL) {
        for (i=0; i<nBogusFontNames; i++) {
            if (!_wcsicmp(pszBogusFontNames[i], pDlg->Font)) {
                GenWarning4(4510, (PCHAR)pDlg->Font, (PCHAR)szSubstituteFontName, 0 ); // Warning for hard coded fonts
                wcscpy(pDlg->Font, szSubstituteFontName);
            }
        }
    }

    GetToken(TRUE);

    pDlg->bCharSet = DEFAULT_CHARSET;

    if (fDlgEx && (token.type == COMMA)) {
        GetToken(TOKEN_NOEXPRESSION);
        if (GetFullExpression(&w, GFE_ZEROINIT | GFE_SHORT))
            pDlg->wWeight = w;

        if (token.type == COMMA) {
            GetToken(TOKEN_NOEXPRESSION);
            if (token.type == NUMLIT) {
                pDlg->bItalic = (token.val) ? TRUE : FALSE;
                GetToken(TOKEN_NOEXPRESSION);

                if (token.type == COMMA) {
                    GetToken(TOKEN_NOEXPRESSION);
                    if (GetFullExpression(&w, GFE_ZEROINIT | GFE_SHORT))
                        pDlg->bCharSet = (UCHAR) w;
                }
            }
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  DGetMenuName() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/*  gets the unquoted string of the name of the optional menu associated */
/*  with the dialog.  */

VOID
DGetMenuName(
    PDLGHDR pDlg
    )
{
    if (GetGenText()) {
        /* copy the menu name */
        token.type = LSTRLIT;
        CheckStr(pDlg->MenuName);

        /* check if menu name is an ordinal */
        if (wcsdigit(pDlg->MenuName[0]))
            pDlg->fOrdinalMenu = TRUE;
        GetToken(TRUE);
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  DGetTitle() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
DGetTitle(
 PDLGHDR pDlg
 )
{
    GetToken(TRUE);

    if (CheckStr(pDlg->Title))
        pDlg->dwStyle |= WS_CAPTION;
    else
        ParseError1(2119); //"Expecting quoted string in dialog title"

    GetToken(TRUE);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  DGetClassName() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
DGetClassName(
 PDLGHDR pDlg
 )
{
    GetToken(TRUE);
    if (!CheckStr(pDlg->Class)) {
        if (token.type == NUMLIT) {
            wcsitow(token.val, pDlg->Class, 10);
            pDlg->fClassOrdinal = TRUE;
        } else {
            ParseError1(2120); //"Expecting quoted string in dialog class"
        }
    }
    GetToken(TRUE);
}


/*---------------------------------------------------------------------------*/
/*      Gets a token, ignoring commas.  Returns the token type.              */
/*                                                                           */
/*  ICGetTok() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/*  Get token, but ignore commas  */

USHORT
ICGetTok(
    VOID
    )
{
    fComma = FALSE; // NT added the use of this fComma flag
    GetToken(TRUE);
    while (token.type == COMMA) {
        GetToken(TRUE);
        fComma = TRUE; // and they set it here
    }
    return (USHORT)token.type;
}


/*  GetTokenNoComma
 *      This function replaces ICGetTok() but has a flag to support
 *      the turning off of expression parsing.
 */

USHORT
GetTokenNoComma(
    USHORT wFlags
    )
{
    /* Get a token */
    GetToken(TRUE | wFlags);

    /* Ignore any commas */
    while (token.type == COMMA)
        GetToken(TRUE | wFlags);

    return (USHORT)token.type;
}


/*************  Menu Parsing Routines *********************/


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  IsmnOption() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
IsmnOption(
    UINT arg,
    PMENUITEM pmn
    )
{
    /* if we have a valid flag, or it into the menu flags */
    switch (arg) {
        case TKOWNERDRAW:
            pmn->OptFlags |= OPOWNERDRAW;
            break;

        case TKCHECKED:
            pmn->OptFlags |= OPCHECKED;
            break;

        case TKGRAYED:
            pmn->OptFlags |= OPGRAYED;
            break;

        case TKINACTIVE:
            pmn->OptFlags |= OPINACTIVE;
            break;

        case TKBREAKWBAR:
            pmn->OptFlags |= OPBREAKWBAR;
            break;

        case TKBREAK:
            pmn->OptFlags |= OPBREAK;
            break;

        case TKHELP:
            pmn->OptFlags |= OPHELP;
            break;

        case TKBITMAP:
            pmn->OptFlags |= OPBITMAP;
            break;

        default:
            return(FALSE);
    }
    return(TRUE);

#if 0
    if ((arg == OPBREAKWBAR)       || (arg == OPHELP   ) || (arg == OPGRAYED) ||
        (arg == OPUSECHECKBITMAPS) || (arg == OPCHECKED) || (arg == OPBITMAP) ||
        (arg == OPOWNERDRAW)       || (arg == OPBREAK  ) || (arg == OPINACTIVE))
    {
        pmn->OptFlags |= arg;
        return TRUE;
    }
#if 0
    if (arg == OPHELP) {
        pmn->OptFlags |= OPPOPHELP;
        return TRUE;
    }
#endif
    return FALSE;
#endif
}



// ----------------------------------------------------------------------------
//
//  DoOldMenuItem() -
//
// ----------------------------------------------------------------------------

WORD
DoOldMenuItem(
    int fPopup
    )
{
    MENUITEM mnTemp;

    mnTemp.PopFlag  = (UCHAR)fPopup;
    GetToken(TRUE);

    /* menu choice string */
    if (CheckStr(mnTemp.szText)) {
        mnTemp.OptFlags = OPPOPUP;
        ICGetTok();
        if (!fPopup) {
            /* change the flag and get the ID if not a popup */
            mnTemp.OptFlags = 0;

            WriteSymbolUse(&token.sym);
            if (!GetFullExpression(&mnTemp.id, GFE_ZEROINIT | GFE_SHORT))
                ParseError1(2125); //"Expected ID value for Menuitem"

            if (token.type == COMMA)
                GetToken(TOKEN_NOEXPRESSION);
        }

        /* read the menu option flags */
        while (IsmnOption(token.type, &mnTemp))
            ICGetTok();
    } else if (token.type == TKSEPARATOR) {
        mnTemp.szText[0] = 0;       // MENUITEM SEPARATOR
        mnTemp.id = 0;
        mnTemp.OptFlags = 0;
        ICGetTok();
    } else {
        ParseError1(2126); //"Expected Menu String"
    }

    /* set it up in the buffer (?) */
    return(SetUpOldMenu(&mnTemp));
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  ParseOldMenu() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

int
ParseOldMenu(
    int fRecursing,
    PRESINFO pRes           // 8 char proc name limitation!
    )
{
    BOOL    bItemRead = FALSE;
    WORD    wEndFlagLoc = 0;

    if (!fRecursing) {
        PreBeginParse(pRes, 2121);
    } else {
        /* make sure its really a menu */
        if (token.type != BEGIN)
            ParseError1(2121); //"BEGIN expected in menu"
        GetToken(TRUE);
    }

    /* get the individual menu items */
    while (token.type != END) {
        switch (token.type) {
            case TKMENUITEM:
                bItemRead = TRUE;
                wEndFlagLoc = DoOldMenuItem(FALSE);
                break;

            case TKPOPUP:
                bItemRead = TRUE;
                wEndFlagLoc = DoOldMenuItem(TRUE);
                ParseOldMenu(TRUE, pRes);
                break;

            default:
                ParseError1(2122); //"Unknown Menu SubType :"
                break;
        }
    }

    /* did we die on an END? */
    if (token.type != END)
        ParseError1(2123); //"END expected in menu"

    /* make sure we have a menu item */
    if (!bItemRead)
        ParseError1(2124); //"Empty menus not allowed"

    /* Get next token if this was NOT the last END*/
    if (fRecursing)
        GetToken(TRUE);

    /* mark the last item in the menu */
    FixOldMenuPatch(wEndFlagLoc);

    return (TRUE);
}


/* ----- Version resource stuff ----- */

/*  VersionParse
 *      Parses the VERSION resource and places it in the global buffer
 *      so it can be written out by SaveResFile().
 */

int
VersionParse(
    VOID
    )
{
    int Index;

    /* Get the fixed structure entries */
    /* Note that VersionParseFixed doesn't actually fail! */
    /* This is because VersionBlockStruct doesn't fail. */
    Index = VersionParseFixed();

    /* Put the following blocks in as sub-blocks.  Fix up the length when
     *  we're done.
     */
    SetItemCount(Index, (USHORT)(GetItemCount(Index) + VersionParseBlock()));

    /* The return data buffer is global */
    return TRUE;
}


/*  VersionParseFixed
 *      Parses the fixed portion of the version resource.  Returns a pointer
 *      to the length word of the block.  This word has the length of
 *      the fixed portion precomputed and remains to have the variable
 *      portion added in.
 */

int
VersionParseFixed(
    VOID
    )
{
    VS_FIXEDFILEINFO FixedInfo;

    /* Initialize the structure fields */
    memset((PCHAR)&FixedInfo, 0, sizeof(FixedInfo));
    FixedInfo.dwSignature = 0xfeef04bdL;
    FixedInfo.dwStrucVersion = 0x00010000L;
    FixedInfo.dwFileDateMS = 0L;
    FixedInfo.dwFileDateLS = 0L;

    /* Loop through tokens until we get the "BEGIN" token which
     *  must be present to terminate the fixed portion of the VERSIONINFO
     *  resource.
     */
    while (token.type != BEGIN) {
        switch (token.type) {
            /* The following have four WORDS scrambled into two DWORDS */
            case TKFILEVERSION:
                VersionGet4Words(&FixedInfo.dwFileVersionMS);
                break;

            case TKPRODUCTVERSION:
                VersionGet4Words(&FixedInfo.dwProductVersionMS);
                break;

                /* The following have just one DWORD */
            case TKFILEFLAGSMASK:
                VersionGetDWord(&FixedInfo.dwFileFlagsMask);
                break;

            case TKFILEFLAGS:
                VersionGetDWord(&FixedInfo.dwFileFlags);
                break;

            case TKFILEOS:
                VersionGetDWord(&FixedInfo.dwFileOS);
                break;

            case TKFILETYPE:
                VersionGetDWord(&FixedInfo.dwFileType);
                break;

            case TKFILESUBTYPE:
                VersionGetDWord(&FixedInfo.dwFileSubtype);
                break;

                /* Other tokens are unknown */
            default:
                ParseError1(2167); //"Unrecognized VERSIONINFO field;"
        }
    }

    /* Write the block out and return the pointer to the length */
    return VersionBlockStruct(L"VS_VERSION_INFO", (PCHAR)&FixedInfo,
        sizeof(FixedInfo));
}


/*  VersionGet4Words
 *      Reads a version number from the source file and scrambles them
 *      to fit in two DWORDs.  We force them to put commas in here so
 *      that if they don't put in enough values we can fill in zeros.
 */

VOID
VersionGet4Words(
    ULONG *pdw
    )
{
    //    static CHAR szParseError[] = "Version WORDs separated by commas expected";

    /* Get the first number */
    GetToken(TRUE);
    if (token.type != NUMLIT || token.flongval)
        ParseError1(2127); //szParseError
    *pdw = ((LONG)token.val) << 16;

    /* Get the comma.  If none, we're done, so fill the rest with zeros */
    GetToken(TRUE);
    if (token.type != COMMA) {
        *++pdw = 0L;
        return;
    }

    /* Get the second number */
    GetToken(TRUE);
    if (token.type != NUMLIT || token.flongval)
        ParseError1(2127); //szParseError
    *(PUSHORT)pdw = token.val;

    /* Get the comma.  If none, we're done, so fill the rest with zeros */
    GetToken(TRUE);
    if (token.type != COMMA) {
        *++pdw = 0L;
        return;
    }

    /* Get the third number */
    GetToken(TRUE);
    if (token.type != NUMLIT || token.flongval)
        ParseError1(2127); //szParseError
    *++pdw = ((LONG)token.val) << 16;

    /* Get the comma.  If none, we're done */
    GetToken(TRUE);
    if (token.type != COMMA)
        return;

    /* Get the fourth number */
    GetToken(TRUE);
    if (token.type != NUMLIT || token.flongval)
        ParseError1(2127); //szParseError
    *(PUSHORT)pdw = token.val;

    /* Get the next token for the loop */
    GetToken(TRUE);
}


/*  VersionGetDWord
 *      Reads a single DWORD from the source file into the given variable.
 */

VOID
VersionGetDWord(
    ULONG *pdw
    )
{
    /* Get the token */
    GetToken(TRUE);
    if (token.type != NUMLIT)
        ParseError1(2128); //"DWORD expected"
    *pdw = token.longval;

    /* Get the next token for the loop */
    GetToken(TRUE);
}


/*  VersionParseBlock
 *      Parses a block of version information.  Note that this block may
 *      contain one or more additional blocks, causing this function to
 *      be called recursively.  Returns the length of the block which can
 *      be added to the length of the current block.  Returns 0xffff on error.
 */

USHORT
VersionParseBlock(
    VOID
    )
{
    USHORT      wLen;
    int         IndexLen;
    USHORT      wType;

    /* Get the current position in the buffer */
    wLen = GetBufferLen();

    /* The token has already been read.  This should be a BEGIN */
    if (token.type != BEGIN)
        ParseError1(2129); //"BEGIN expected in VERSIONINFO resource"

    /* Get the first token.  From here on, the VersionBlockVariable()
     *  routine gets the tokens as it searches for the end of the value
     *  field.
     */
    GetToken(TRUE);

    /* Loop until we get to the END for this BEGIN */
    for (; ; ) {
        /* Get and decode the next line type */
        switch (token.type) {
            case TKVALUE:
            case TKBLOCK:
                /* Save the type of this token */
                wType = token.type;

                /* Get the key string */
                GetToken(TRUE);
                if (token.type != LSTRLIT)
                    ParseError1(2131); //"Expecting quoted string for key"

                /* Now feed in the key string and value items */
                IndexLen = VersionBlockVariable(tokenbuf);

                /* A "BLOCK" item causes recursion.  Current token should be
                 *  "BEGIN"
                 */
                if (wType == TKBLOCK) {
                    SetItemCount(IndexLen, (USHORT)(GetItemCount(IndexLen) + VersionParseBlock()));
                    GetToken(TRUE);
                }
                break;

            case END:
                /* We're done with this block.  Get the next token
                 *  (read past the "END") and return the length of the block.
                 */
                return GetBufferLen() - wLen;

            default:
                ParseError1(2132); //"Expected VALUE, BLOCK, or, END keyword."
        }
    }
}


#define DWORDALIGN(w) \
    (((w) + (sizeof(ULONG) - 1)) & ~(USHORT)(sizeof(ULONG) - 1))

/*  VersionBlockStruct
 *      Writes a version block without sub-blocks.  Sub-blocks are to
 *      be written directly after this header.  To facilitate this,
 *      a pointer to the block length is returned so that it can be modified.
 *      This call uses a pre-parsed value item.  Use VersionBlockVariable()
 *      to parse the value items instead.
 *      Note that this actually can't fail!
 */

int
VersionBlockStruct(
    PWCHAR pstrKey,
    PCHAR pstrValue,
    USHORT wLenValue
    )
{
    USHORT wLen;
    int Index;
    ULONG dwPadding = 0L;
    USHORT wAlign;

    /* Pad the block data to DWORD align */
    wAlign = DWORDALIGN(GetBufferLen()) - GetBufferLen();
    if (wAlign)
        WriteBuffer((PCHAR)&dwPadding, wAlign);

    /* Save the current length so we can compute the new block length later */
    wLen = GetBufferLen();

    /* Write a zero for the length for now */
    Index = GetBufferLen();
    WriteWord(0);

    /* Write the length of the value field */
    WriteWord(wLenValue);

    /* data is binary */
    WriteWord(0);

    /* Write the key string now */
    WriteString(pstrKey, TRUE);

    /* Write the value data if there is any */
    if (wLenValue) {
        /* Now we have to DWORD align the value data */
        wAlign = DWORDALIGN(GetBufferLen()) - GetBufferLen();
        if (wAlign)
            WriteBuffer((PSTR)&dwPadding, wAlign);

        /* Write it to the buffer */
        WriteBuffer((PSTR)pstrValue, wLenValue);
    }

    /* Now fix up the block length and return a pointer to it */
    SetItemCount(Index, (USHORT)(GetBufferLen() - wLen));

    return Index;
}



/*  VersionBlockVariable
 *      Writes a version block without sub-blocks.  Sub-blocks are to
 *      bre written directly after this header.  To facilitate this,
 *      a pointer to the block length is returned so that it can be modified.
 *      VersionBlockVariable() gets the value items by parsing the
 *      RC script as RCDATA.
 */

int
VersionBlockVariable(
    PWCHAR pstrKey
    )
{
    USHORT wLen;
    int IndexLen;
    int IndexType;
    int IndexValueLen;
    ULONG dwPadding = 0L;
    USHORT wAlign;

    /* Pad the block data to DWORD align */
    wAlign = DWORDALIGN(GetBufferLen()) - GetBufferLen();
    if (wAlign)
        WriteBuffer((PCHAR)&dwPadding, wAlign);

    /* Save the current length so we can compute the new block length later */
    wLen = GetBufferLen();

    /* Write a zero for the length for now */
    IndexLen = GetBufferLen();
    WriteWord(0);

    /* Write the length of the value field.  We fill this in later */
    IndexValueLen = GetBufferLen();
    WriteWord(0);

    /* Assume string data */
    IndexType = GetBufferLen();
    WriteWord(1);

    /* Write the key string now */
    WriteString(pstrKey, TRUE);

    /* Parse and write the value data if there is any */
    SetItemCount(IndexValueLen, VersionParseValue(IndexType));

    /* Now fix up the block length and return a pointer to it */
    SetItemCount(IndexLen, (USHORT)(GetBufferLen() - wLen));

    return IndexLen;
}



/*  VersionParseValue
 *      Parses the fields following either BLOCK or VALUE and following
 *      their key string which is parsed by VersionParseBlock().
 *      Before writing the first value item out, the field has to be
 *      DWORD aligned.  Returns the length of the value block.
 */

USHORT
VersionParseValue(
    int IndexType
    )
{
    USHORT wFirst = FALSE;
    USHORT wToken;
    USHORT wAlign;
    ULONG dwPadding = 0L;
    USHORT wLen = 0;

    /* Decode all tokens until we get to the end of this item */
    for (; ; ) {
        /* ICGetTok is GetToken(TRUE) ignoring commas */
        wToken =  ICGetTok();

        /* If this is the first item, DWORD align it.  Since empty value
         *  sections are legal, we have to wait until we actually have data
         *  to do this.
         */
        if (!wFirst) {
            wFirst = TRUE;
            wAlign = DWORDALIGN(GetBufferLen()) - GetBufferLen();
            if (wAlign)
                WriteBuffer((PCHAR)&dwPadding, wAlign);
        }

        switch (wToken) {
            case TKVALUE:
            case TKBLOCK:
            case BEGIN:
            case END:
                return wLen;

            case LSTRLIT:                   /* String, write characters */
                if (tokenbuf[0] == L'\0')   /* ignore null strings */
                    break;

                /* remove extra nuls */
                while (tokenbuf[token.val-1] == L'\0')
                    token.val--;

                wAlign = token.val + 1;     /* want the character count */
                wLen += wAlign;
                if (fComma) {
                    WriteString(tokenbuf, TRUE);
                } else {
                    AppendString(tokenbuf, TRUE);
                    wLen--;
                }
                break;

            case NUMLIT:            /* Write the computed number out */
                SetItemCount(IndexType, 0);        /* mark data binary */
                if (token.flongval) {
                    WriteLong(token.longval);
                    wLen += sizeof(LONG);
                } else {
                    WriteWord(token.val);
                    wLen += sizeof(WORD);
                }
                break;

            default:
                ParseError1(2133); //"Unexpected value in value data"
                return 0;
        }
    }
}


VOID
DlgIncludeParse(
    PRESINFO pRes
    )
{
    INT     i;
    INT     nbytes;
    char *  lpbuf;

    if (token.type != LSTRLIT) {
        ParseError1(2165);
        return;
    }

    // the DLGINCLUDE statement must be written in ANSI (8-bit) for compatibility
    //    WriteString(tokenbuf);
    nbytes = WideCharToMultiByte (CP_ACP, 0, tokenbuf, -1, NULL, 0, NULL, NULL);
    lpbuf = (char *) MyAlloc (nbytes);
    WideCharToMultiByte (CP_ACP, 0, tokenbuf, -1, lpbuf, nbytes, NULL, NULL);

    for (i = 0; i < nbytes; i++)
         WriteByte (lpbuf[i]);

    MyFree(lpbuf);
    return;
}
