/* ------------ helpbox.c ----------- */

#include "dflat.h"
#include "htree.h"

extern DBOX HelpBox;

/* -------- strings of D-Flat classes for calling default
      help text collections -------- */
char *ClassNames[] = {
    #undef ClassDef
    #define ClassDef(c,b,p,a) #c,
    #include "classes.h"
    NULL
};

#define MAXHEIGHT (DfGetScreenHeight()-10)

/* --------- linked list of help text collections -------- */
struct helps {
    char *hname;
    char *NextName;
    char *PrevName;
    long hptr;
    int bit;
    int hheight;
    int hwidth;
    DFWINDOW hwnd;
    struct helps *NextHelp;
};
static struct helps *FirstHelp;
static struct helps *LastHelp;
static struct helps *ThisHelp;

/* --- linked stack of help windows that have beed used --- */
struct HelpStack {
    char *hname;
    struct HelpStack *PrevStack;
};
static struct HelpStack *LastStack;
static struct HelpStack *ThisStack;

/* --- linked list of keywords in the current help
           text collection (listhead is in window) -------- */
struct keywords {
    char *hname;
    int lineno;
    int off1, off2, off3;
    int isDefinition;
    struct keywords *nextword;
    struct keywords *prevword;
};

static FILE *helpfp;
static char hline [160];
static BOOL Helping;

static void SelectHelp(DFWINDOW, char *);
static void ReadHelp(DFWINDOW);
static void FindHelp(char *);
static void FindHelpWindow(DFWINDOW);
static void DisplayDefinition(DFWINDOW, char *);
static void BestFit(DFWINDOW, DIALOGWINDOW *);

/* ------------- CREATE_WINDOW message ------------ */
static void CreateWindowMsg(DFWINDOW wnd)
{
    Helping = TRUE;
    GetClass(wnd) = HELPBOX;
    InitWindowColors(wnd);
    if (ThisHelp != NULL)
        ThisHelp->hwnd = wnd;
}

/* ------------- COMMAND message ------------ */
static BOOL CommandMsg(DFWINDOW wnd, PARAM p1)
{
    switch ((int)p1)    {
        case ID_CANCEL:
            ThisStack = LastStack;
            while (ThisStack != NULL)    {
                LastStack = ThisStack->PrevStack;
                if (ThisStack->hname != NULL)
                    free(ThisStack->hname);
                free(ThisStack);
                ThisStack = LastStack;
            }
            break;
        case ID_PREV:
            FindHelpWindow(wnd);
            if (ThisHelp != NULL)
                SelectHelp(wnd, ThisHelp->PrevName);
            return TRUE;
        case ID_NEXT:
            FindHelpWindow(wnd);
            if (ThisHelp != NULL)
                SelectHelp(wnd, ThisHelp->NextName);
            return TRUE;
        case ID_BACK:
            if (LastStack != NULL)    {
                if (LastStack->PrevStack != NULL)    {
                    ThisStack = LastStack->PrevStack;
                    if (LastStack->hname != NULL)
                        free(LastStack->hname);
                    free(LastStack);
                    LastStack = ThisStack;
                    SelectHelp(wnd, ThisStack->hname);
                }
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

/* ------------- KEYBOARD message ------------ */
static BOOL KeyboardMsg(DFWINDOW wnd, PARAM p1)
{
    DFWINDOW cwnd;
    struct keywords *thisword;
    static char HelpName[50];

    cwnd = ControlWindow(wnd->extension, ID_HELPTEXT);
    if (cwnd == NULL || inFocus != cwnd)
        return FALSE;
    thisword = cwnd->thisword;
    switch ((int)p1)    {
        case '\r':
            if (thisword != NULL)    {
                if (thisword->isDefinition)
                    DisplayDefinition(GetParent(wnd),
                                        thisword->hname);
                else    {
                    strncpy(HelpName, thisword->hname,
                        sizeof HelpName);
                    SelectHelp(wnd, HelpName);
                }
            }
            return TRUE;
        case '\t':
            if (thisword == NULL)
                thisword = cwnd->firstword;
            else {
                if (thisword->nextword == NULL)
                    thisword = cwnd->firstword;
                else
                    thisword = thisword->nextword;
            }
            break;
        case SHIFT_HT:
            if (thisword == NULL)
                thisword = cwnd->lastword;
            else {
                if (thisword->prevword == NULL)
                    thisword = cwnd->lastword;
                else
                    thisword = thisword->prevword;
            }
            break;
        default:
            thisword = NULL;
            break;
    }
    if (thisword != NULL)    {
        cwnd->thisword = thisword;
        if (thisword->lineno < cwnd->wtop ||
                thisword->lineno >=
                    cwnd->wtop + ClientHeight(cwnd))  {
            int distance = ClientHeight(cwnd)/2;
            do    {
                cwnd->wtop = thisword->lineno-distance;
                distance /= 2;
            }
            while (cwnd->wtop < 0);
        }
        DfSendMessage(cwnd, PAINT, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* ---- window processing module for the HELPBOX ------- */
int HelpBoxProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;

    switch (msg)    {
        case CREATE_WINDOW:
            CreateWindowMsg(wnd);
            break;
        case INITIATE_DIALOG:
            ReadHelp(wnd);
            break;
        case DFM_COMMAND:
            if (p2 != 0)
                break;
            if (CommandMsg(wnd, p1))
                return TRUE;
            break;
        case KEYBOARD:
            if (WindowMoving)
                break;
            if (KeyboardMsg(wnd, p1))
                return TRUE;
            break;
        case CLOSE_WINDOW:
            if (db != NULL)    {
                if (db->dwnd.title != NULL)    {
                    free(db->dwnd.title);
                    db->dwnd.title = NULL;
                }
            }
            FindHelpWindow(wnd);
            if (ThisHelp != NULL)
                ThisHelp->hwnd = NULL;
            Helping = FALSE;
            break;
        default:
            break;
    }
    return BaseWndProc(HELPBOX, wnd, msg, p1, p2);
}

/* ----- select a new help window from its name ----- */
static void SelectHelp(DFWINDOW wnd, char *hname)
{
    if (hname != NULL)    {
        DFWINDOW pwnd = GetParent(wnd);
        DfPostMessage(wnd, ENDDIALOG, 0, 0);
        DfPostMessage(pwnd, DISPLAY_HELP, (PARAM) hname, 0);
    }
}

/* ---- PAINT message for the helpbox text editbox ---- */
static int PaintMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    struct keywords *thisword;
    int rtn;
    if (wnd->thisword != NULL)    {
        DFWINDOW pwnd = GetParent(wnd);
        char *cp;
        thisword = wnd->thisword;
        cp = TextLine(wnd, thisword->lineno);
        cp += thisword->off1;
        *(cp+1) =
            (pwnd->WindowColors[SELECT_COLOR][FG] & 255) | 0x80;
        *(cp+2) =
            (pwnd->WindowColors[SELECT_COLOR][BG] & 255) | 0x80;
        rtn = DefaultWndProc(wnd, PAINT, p1, p2);
        *(cp+1) =
            (pwnd->WindowColors[HILITE_COLOR][FG] & 255) | 0x80;
        *(cp+2) =
            (pwnd->WindowColors[HILITE_COLOR][BG] & 255) | 0x80;
        return rtn;
    }
    return DefaultWndProc(wnd, PAINT, p1, p2);
}

/* ---- LEFT_BUTTON message for the helpbox text editbox ---- */
static int LeftButtonMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    struct keywords *thisword;
    int rtn, mx, my;

    rtn = DefaultWndProc(wnd, LEFT_BUTTON, p1, p2);
    mx = (int)p1 - GetClientLeft(wnd);
    my = (int)p2 - GetClientTop(wnd);
    my += wnd->wtop;
    thisword = wnd->firstword;
    while (thisword != NULL)    {
        if (my == thisword->lineno)    {
            if (mx >= thisword->off2 &&
                        mx < thisword->off3)    {
                wnd->thisword = thisword;
                DfSendMessage(wnd, PAINT, 0, 0);
                if (thisword->isDefinition)    {
                    DFWINDOW pwnd = GetParent(wnd);
                    if (pwnd != NULL)
                        DisplayDefinition(GetParent(pwnd),
                            thisword->hname);
                }
                break;
            }
        }
        thisword = thisword->nextword;
    }
    return rtn;
}

/* --- window processing module for HELPBOX's text EDITBOX -- */
int HelpTextProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    struct keywords *thisword;
    switch (msg)    {
        case PAINT:
            return PaintMsg(wnd, p1, p2);
        case LEFT_BUTTON:
            return LeftButtonMsg(wnd, p1, p2);
        case DOUBLE_CLICK:
            DfPostMessage(wnd, KEYBOARD, '\r', 0);
            break;
        case CLOSE_WINDOW:
            thisword = wnd->firstword;
            while (thisword != NULL)    {
                struct keywords *nextword = thisword->nextword;
                if (thisword->hname != NULL)
                    free(thisword->hname);
                free(thisword);
                thisword = nextword;
            }
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

/* -------- read the help text into the editbox ------- */
static void ReadHelp(DFWINDOW wnd)
{
    DFWINDOW cwnd = ControlWindow(wnd->extension, ID_HELPTEXT);
    int linectr = 0;
    if (cwnd == NULL)
        return;
    cwnd->wndproc = HelpTextProc;
    /* ----- read the help text ------- */
    while (TRUE)    {
        unsigned char *cp = hline, *cp1;
        int colorct = 0;
        if (GetHelpLine(hline) == NULL)
            break;
        if (*hline == '<')
            break;
        hline[strlen(hline)-1] = '\0';
        /* --- add help text to the help window --- */
        while (cp != NULL)    {
            if ((cp = strchr(cp, '[')) != NULL)    {
                /* ----- hit a new key word ----- */
                struct keywords *thisword;
                if (*(cp+1) != '.' && *(cp+1) != '*')    {
                    cp++;
                    continue;
                }
                thisword = DFcalloc(1, sizeof(struct keywords));
                if (cwnd->firstword == NULL)
                    cwnd->firstword = thisword;
                if (cwnd->lastword != NULL)    {
                    ((struct keywords *)
                        (cwnd->lastword))->nextword = thisword;
                    thisword->prevword = cwnd->lastword;
                }
                cwnd->lastword = thisword;
                thisword->lineno = cwnd->wlines;
                thisword->off1 = (int) ((int)cp - (int)hline);
                thisword->off2 = thisword->off1 - colorct * 4;
                thisword->isDefinition = *(cp+1) == '*';
                colorct++;
                *cp++ = CHANGECOLOR;
                *cp++ =
            (wnd->WindowColors [HILITE_COLOR] [FG] & 255) | 0x80;
                *cp++ =
            (wnd->WindowColors [HILITE_COLOR] [BG] & 255) | 0x80;
                cp1 = cp;
                if ((cp = strchr(cp, ']')) != NULL)    {
                    if (thisword != NULL)
                        thisword->off3 =
                            thisword->off2 + (int) (cp - cp1);
                    *cp++ = RESETCOLOR;
                }
                if ((cp = strchr(cp, '<')) != NULL)    {
                    char *cp1 = strchr(cp, '>');
                    if (cp1 != NULL)    {
                        int len = (int) ((int)cp1 - (int)cp);
                        thisword->hname = DFcalloc(1, len);
                        strncpy(thisword->hname, cp+1, len-1);
                        memmove(cp, cp1+1, strlen(cp1));
                    }
                }
            }
        }
        PutItemText(wnd, ID_HELPTEXT, hline);
        /* -- display help text as soon as window is full -- */
        if (++linectr == ClientHeight(cwnd))
            DfSendMessage(cwnd, PAINT, 0, 0);
        if (linectr > ClientHeight(cwnd) &&
                !TestAttribute(cwnd, VSCROLLBAR))    {
            AddAttribute(cwnd, VSCROLLBAR);
            DfSendMessage(cwnd, BORDER, 0, 0);
        }
    }
}

/* ---- compute the displayed length of a help text line --- */
static int HelpLength(char *s)
{
    int len = strlen(s);
    char *cp = strchr(s, '[');
    while (cp != NULL)    {
        len -= 4;
        cp = strchr(cp+1, '[');
    }
    cp = strchr(s, '<');
    while (cp != NULL)    {
        char *cp1 = strchr(cp, '>');
        if (cp1 != NULL)
            len -= (int) (cp1-cp)+1;
        cp = strchr(cp1, '<');
    }
    return len;
}

/* ----------- load the help text file ------------ */
void LoadHelpFile()
{
    char *cp;

    if (Helping)
        return;
    UnLoadHelpFile();
    if ((helpfp = OpenHelpFile()) == NULL)
        return;
    *hline = '\0';
    while (*hline != '<')    {
        if (GetHelpLine(hline) == NULL)    {
            fclose(helpfp);
            return;
        }
    }
    while (*hline == '<')   {
        if (strncmp(hline, "<end>", 5) == 0)
            break;

        /* -------- parse the help window's text name ----- */
        if ((cp = strchr(hline, '>')) != NULL)    {
            ThisHelp = DFcalloc(1, sizeof(struct helps));
            if (FirstHelp == NULL)
            FirstHelp = ThisHelp;
            *cp = '\0';
            ThisHelp->hname=DFmalloc(strlen(hline+1)+1);
            strcpy(ThisHelp->hname, hline+1);

            HelpFilePosition(&ThisHelp->hptr, &ThisHelp->bit);

            if (GetHelpLine(hline) == NULL)
                break;

            /* ------- build the help linked list entry --- */
            while (*hline == '[')    {
                HelpFilePosition(&ThisHelp->hptr,
                                            &ThisHelp->bit);
                /* ---- parse the <<prev button pointer ---- */
                if (strncmp(hline, "[<<]", 4) == 0)    {
                    char *cp = strchr(hline+4, '<');
                    if (cp != NULL)    {
                        char *cp1 = strchr(cp, '>');
                        if (cp1 != NULL)    {
                            int len = (int) (cp1-cp);
                            ThisHelp->PrevName=DFcalloc(1,len);
                            strncpy(ThisHelp->PrevName,
                                cp+1,len-1);
                        }
                    }
                    if (GetHelpLine(hline) == NULL)
                        break;
                    continue;
                }
                /* ---- parse the next>> button pointer ---- */
                else if (strncmp(hline, "[>>]", 4) == 0)    {
                    char *cp = strchr(hline+4, '<');
                    if (cp != NULL)    {
                        char *cp1 = strchr(cp, '>');
                        if (cp1 != NULL)    {
                            int len = (int) (cp1-cp);
                            ThisHelp->NextName=DFcalloc(1,len);
                            strncpy(ThisHelp->NextName,
                                            cp+1,len-1);
                        }
                    }
                    if (GetHelpLine(hline) == NULL)
                        break;
                    continue;
                }
                else
                    break;
            }
            ThisHelp->hheight = 0;
            ThisHelp->hwidth = 0;
            ThisHelp->NextHelp = NULL;

            /* ------ append entry to the linked list ------ */
            if (LastHelp != NULL)
                LastHelp->NextHelp = ThisHelp;
            LastHelp = ThisHelp;
        }
        /* -------- move to the next <helpname> token ------ */
        if (GetHelpLine(hline) == NULL)
            strcpy(hline, "<end>");
        while (*hline != '<')    {
            ThisHelp->hwidth =
                max(ThisHelp->hwidth, HelpLength(hline));
            ThisHelp->hheight++;
            if (GetHelpLine(hline) == NULL)
                strcpy(hline, "<end>");
        }
    }
    fclose(helpfp);
}

/* ------ free the memory used by the help file table ------ */
void UnLoadHelpFile(void)
{
    while (FirstHelp != NULL)    {
        ThisHelp = FirstHelp;
        if (ThisHelp->hname != NULL)
            free(ThisHelp->hname);
        if (ThisHelp->PrevName != NULL)
            free(ThisHelp->PrevName);
        if (ThisHelp->NextName != NULL)
            free(ThisHelp->NextName);
        FirstHelp = ThisHelp->NextHelp;
        free(ThisHelp);
    }
    ThisHelp = LastHelp = NULL;
    free(HelpTree);
	HelpTree = NULL;
}

/* ---------- display a specified help text ----------- */
BOOL DisplayHelp(DFWINDOW wnd, char *Help)
{
	BOOL rtn = FALSE;
    if (Helping)
        return TRUE;
	wnd->isHelping++;
    FindHelp(Help);
    if (ThisHelp != NULL)    {
        if (LastStack == NULL ||
                stricmp(Help, LastStack->hname))    {
            /* ---- add the window to the history stack ---- */
            ThisStack = DFcalloc(1,sizeof(struct HelpStack));
            ThisStack->hname = DFmalloc(strlen(Help)+1);
            if (ThisStack->hname != NULL)
                strcpy(ThisStack->hname, Help);
            ThisStack->PrevStack = LastStack;
            LastStack = ThisStack;
        }
        if ((helpfp = OpenHelpFile()) != NULL)    {
            DBOX *db;
            int offset, i;

            db = DFcalloc(1,sizeof HelpBox);
            memcpy(db, &HelpBox, sizeof HelpBox);
            /* -- seek to the first line of the help text -- */
            SeekHelpLine(ThisHelp->hptr, ThisHelp->bit);
            /* ----- read the title ----- */
            GetHelpLine(hline);
            hline[strlen(hline)-1] = '\0';
            db->dwnd.title = DFmalloc(strlen(hline)+1);
            strcpy(db->dwnd.title, hline);
            /* ----- set the height and width ----- */
            db->dwnd.h = min(ThisHelp->hheight, MAXHEIGHT)+7;
            db->dwnd.w = max(45, ThisHelp->hwidth+6);
            /* ------ position the help window ----- */
            BestFit(wnd, &db->dwnd);
            /* ------- position the command buttons ------ */
            db->ctl[0].dwnd.w = max(40, ThisHelp->hwidth+2);
            db->ctl[0].dwnd.h =
                        min(ThisHelp->hheight, MAXHEIGHT)+2;
            offset = (db->dwnd.w-40) / 2;
            for (i = 1; i < 5; i++)    {
                db->ctl[i].dwnd.y =
                        min(ThisHelp->hheight, MAXHEIGHT)+3;
                db->ctl[i].dwnd.x += offset;
            }
            /* ---- disable ineffective buttons ---- */
            if (ThisStack != NULL)
                if (ThisStack->PrevStack == NULL)
                    DisableButton(db, ID_BACK);
            if (ThisHelp->NextName == NULL)
                DisableButton(db, ID_NEXT);
            if (ThisHelp->PrevName == NULL)
                DisableButton(db, ID_PREV);
            /* ------- display the help window ----- */
            DfDialogBox(NULL, db, TRUE, HelpBoxProc);
            free(db);
            fclose(helpfp);
            rtn = TRUE;
        }
    }
	--wnd->isHelping;
    return rtn;
}

/* ------- display a definition window --------- */
static void DisplayDefinition(DFWINDOW wnd, char *def)
{
    DFWINDOW dwnd;
    DFWINDOW hwnd = wnd;
    int y;

    if (GetClass(wnd) == POPDOWNMENU)
        hwnd = GetParent(wnd);
    y = GetClass(hwnd) == MENUBAR ? 2 : 1;
    FindHelp(def);
    if (ThisHelp != NULL)    {
        if ((helpfp = OpenHelpFile()) != NULL)    {
            dwnd = DfCreateWindow(
                        TEXTBOX,
                        NULL,
                        GetClientLeft(hwnd),
                        GetClientTop(hwnd)+y,
                        min(ThisHelp->hheight, MAXHEIGHT)+3,
                        ThisHelp->hwidth+2,
                        NULL,
                        wnd,
                        NULL,
                        HASBORDER | NOCLIP | SAVESELF);
            if (dwnd != NULL)    {
                /* ----- read the help text ------- */
                SeekHelpLine(ThisHelp->hptr, ThisHelp->bit);
                while (TRUE)    {
                    if (GetHelpLine(hline) == NULL)
                        break;
                    if (*hline == '<')
                        break;
                    hline[strlen(hline)-1] = '\0';
                    DfSendMessage(dwnd,ADDTEXT,(PARAM)hline,0);
                }
                DfSendMessage(dwnd, SHOW_WINDOW, 0, 0);
                DfSendMessage(NULL, WAITKEYBOARD, 0, 0);
                DfSendMessage(NULL, WAITMOUSE, 0, 0);
                DfSendMessage(dwnd, CLOSE_WINDOW, 0, 0);
            }
            fclose(helpfp);
        }
    }
}

/* ------ compare help names with wild cards ----- */
static BOOL wildcmp(char *s1, char *s2)
{
    while (*s1 || *s2)    {
        if (tolower(*s1) != tolower(*s2))
            if (*s1 != '?' && *s2 != '?')
                return TRUE;
        s1++, s2++;
    }
    return FALSE;
}

/* --- ThisHelp = the help window matching specified name --- */
static void FindHelp(char *Help)
{
    ThisHelp = FirstHelp;
    while (ThisHelp != NULL)    {
        if (wildcmp(Help, ThisHelp->hname) == FALSE)
            break;
        ThisHelp = ThisHelp->NextHelp;
    }
}

/* --- ThisHelp = the help window matching specified wnd --- */
static void FindHelpWindow(DFWINDOW wnd)
{
    ThisHelp = FirstHelp;
    while (ThisHelp != NULL)    {
        if (wnd == ThisHelp->hwnd)
            break;
        ThisHelp = ThisHelp->NextHelp;
    }
}

static int OverLap(int a, int b)
{
    int ov = a - b;
    if (ov < 0)
        ov = 0;
    return ov;
}

/* ----- compute the best location for a help dialogbox ----- */
static void BestFit(DFWINDOW wnd, DIALOGWINDOW *dwnd)
{
    int above, below, right, left;
    if (GetClass(wnd) == MENUBAR ||
                GetClass(wnd) == APPLICATION)    {
        dwnd->x = dwnd->y = -1;
        return;
    }
    /* --- compute above overlap ---- */
    above = OverLap(dwnd->h, GetTop(wnd));
    /* --- compute below overlap ---- */
    below = OverLap(GetBottom(wnd), DfGetScreenHeight()-dwnd->h);
    /* --- compute right overlap ---- */
    right = OverLap(GetRight(wnd), DfGetScreenWidth()-dwnd->w);
    /* --- compute left  overlap ---- */
    left = OverLap(dwnd->w, GetLeft(wnd));

    if (above < below)
        dwnd->y = max(0, GetTop(wnd)-dwnd->h-2);
    else
        dwnd->y = min(DfGetScreenHeight()-dwnd->h, GetBottom(wnd)+2);
    if (right < left)
        dwnd->x = min(GetRight(wnd)+2, DfGetScreenWidth()-dwnd->w);
    else
        dwnd->x = max(0, GetLeft(wnd)-dwnd->w-2);

    if (dwnd->x == GetRight(wnd)+2 ||
            dwnd->x == GetLeft(wnd)-dwnd->w-2)
        dwnd->y = -1;
    if (dwnd->y ==GetTop(wnd)-dwnd->h-2 ||
            dwnd->y == GetBottom(wnd)+2)
        dwnd->x = -1;
}

/* EOF */
