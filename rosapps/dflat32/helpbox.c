/* ------------ helpbox.c ----------- */

#include "dflat.h"
#include "htree.h"

extern DF_DBOX HelpBox;

/* -------- strings of D-Flat classes for calling default
      help text collections -------- */
char *DfClassNames[] = {
    #undef DfClassDef
    #define DfClassDef(c,b,p,a) #c,
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
static void BestFit(DFWINDOW, DF_DIALOGWINDOW *);

/* ------------- DFM_CREATE_WINDOW message ------------ */
static void CreateWindowMsg(DFWINDOW wnd)
{
    Helping = TRUE;
    DfGetClass(wnd) = DF_HELPBOX;
    DfInitWindowColors(wnd);
    if (ThisHelp != NULL)
        ThisHelp->hwnd = wnd;
}

/* ------------- COMMAND message ------------ */
static BOOL CommandMsg(DFWINDOW wnd, DF_PARAM p1)
{
    switch ((int)p1)    {
        case DF_ID_CANCEL:
            ThisStack = LastStack;
            while (ThisStack != NULL)    {
                LastStack = ThisStack->PrevStack;
                if (ThisStack->hname != NULL)
                    free(ThisStack->hname);
                free(ThisStack);
                ThisStack = LastStack;
            }
            break;
        case DF_ID_PREV:
            FindHelpWindow(wnd);
            if (ThisHelp != NULL)
                SelectHelp(wnd, ThisHelp->PrevName);
            return TRUE;
        case DF_ID_NEXT:
            FindHelpWindow(wnd);
            if (ThisHelp != NULL)
                SelectHelp(wnd, ThisHelp->NextName);
            return TRUE;
        case DF_ID_BACK:
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

/* ------------- DFM_KEYBOARD message ------------ */
static BOOL KeyboardMsg(DFWINDOW wnd, DF_PARAM p1)
{
    DFWINDOW cwnd;
    struct keywords *thisword;
    static char HelpName[50];

    cwnd = DfControlWindow(wnd->extension, DF_ID_HELPTEXT);
    if (cwnd == NULL || DfInFocus != cwnd)
        return FALSE;
    thisword = cwnd->thisword;
    switch ((int)p1)    {
        case '\r':
            if (thisword != NULL)    {
                if (thisword->isDefinition)
                    DisplayDefinition(DfGetParent(wnd),
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
        case DF_SHIFT_HT:
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
                    cwnd->wtop + DfClientHeight(cwnd))  {
            int distance = DfClientHeight(cwnd)/2;
            do    {
                cwnd->wtop = thisword->lineno-distance;
                distance /= 2;
            }
            while (cwnd->wtop < 0);
        }
        DfSendMessage(cwnd, DFM_PAINT, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* ---- window processing module for the DF_HELPBOX ------- */
int DfHelpBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    DF_DBOX *db = wnd->extension;

    switch (msg)    {
        case DFM_CREATE_WINDOW:
            CreateWindowMsg(wnd);
            break;
        case DFM_INITIATE_DIALOG:
            ReadHelp(wnd);
            break;
        case DFM_COMMAND:
            if (p2 != 0)
                break;
            if (CommandMsg(wnd, p1))
                return TRUE;
            break;
        case DFM_KEYBOARD:
            if (DfWindowMoving)
                break;
            if (KeyboardMsg(wnd, p1))
                return TRUE;
            break;
        case DFM_CLOSE_WINDOW:
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
    return DfBaseWndProc(DF_HELPBOX, wnd, msg, p1, p2);
}

/* ----- select a new help window from its name ----- */
static void SelectHelp(DFWINDOW wnd, char *hname)
{
    if (hname != NULL)    {
        DFWINDOW pwnd = DfGetParent(wnd);
        DfPostMessage(wnd, DFM_ENDDIALOG, 0, 0);
        DfPostMessage(pwnd, DFM_DISPLAY_HELP, (DF_PARAM) hname, 0);
    }
}

/* ---- DFM_PAINT message for the helpbox text editbox ---- */
static int PaintMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    struct keywords *thisword;
    int rtn;
    if (wnd->thisword != NULL)    {
        DFWINDOW pwnd = DfGetParent(wnd);
        char *cp;
        thisword = wnd->thisword;
        cp = DfTextLine(wnd, thisword->lineno);
        cp += thisword->off1;
        *(cp+1) =
            (pwnd->WindowColors[DF_SELECT_COLOR][DF_FG] & 255) | 0x80;
        *(cp+2) =
            (pwnd->WindowColors[DF_SELECT_COLOR][DF_BG] & 255) | 0x80;
        rtn = DfDefaultWndProc(wnd, DFM_PAINT, p1, p2);
        *(cp+1) =
            (pwnd->WindowColors[DF_HILITE_COLOR][DF_FG] & 255) | 0x80;
        *(cp+2) =
            (pwnd->WindowColors[DF_HILITE_COLOR][DF_BG] & 255) | 0x80;
        return rtn;
    }
    return DfDefaultWndProc(wnd, DFM_PAINT, p1, p2);
}

/* ---- DFM_LEFT_BUTTON message for the helpbox text editbox ---- */
static int LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    struct keywords *thisword;
    int rtn, mx, my;

    rtn = DfDefaultWndProc(wnd, DFM_LEFT_BUTTON, p1, p2);
    mx = (int)p1 - DfGetClientLeft(wnd);
    my = (int)p2 - DfGetClientTop(wnd);
    my += wnd->wtop;
    thisword = wnd->firstword;
    while (thisword != NULL)    {
        if (my == thisword->lineno)    {
            if (mx >= thisword->off2 &&
                        mx < thisword->off3)    {
                wnd->thisword = thisword;
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
                if (thisword->isDefinition)    {
                    DFWINDOW pwnd = DfGetParent(wnd);
                    if (pwnd != NULL)
                        DisplayDefinition(DfGetParent(pwnd),
                            thisword->hname);
                }
                break;
            }
        }
        thisword = thisword->nextword;
    }
    return rtn;
}

/* --- window processing module for DF_HELPBOX's text DF_EDITBOX -- */
int HelpTextProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    struct keywords *thisword;
    switch (msg)    {
        case DFM_PAINT:
            return PaintMsg(wnd, p1, p2);
        case DFM_LEFT_BUTTON:
            return LeftButtonMsg(wnd, p1, p2);
        case DOUBLE_CLICK:
            DfPostMessage(wnd, DFM_KEYBOARD, '\r', 0);
            break;
        case DFM_CLOSE_WINDOW:
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
    return DfDefaultWndProc(wnd, msg, p1, p2);
}

/* -------- read the help text into the editbox ------- */
static void ReadHelp(DFWINDOW wnd)
{
    DFWINDOW cwnd = DfControlWindow(wnd->extension, DF_ID_HELPTEXT);
    int linectr = 0;
    if (cwnd == NULL)
        return;
    cwnd->wndproc = HelpTextProc;
    /* ----- read the help text ------- */
    while (TRUE)    {
        unsigned char *cp = hline, *cp1;
        int colorct = 0;
        if (DfGetHelpLine(hline) == NULL)
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
                thisword = DfCalloc(1, sizeof(struct keywords));
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
                *cp++ = DF_CHANGECOLOR;
                *cp++ =
            (wnd->WindowColors [DF_HILITE_COLOR] [DF_FG] & 255) | 0x80;
                *cp++ =
            (wnd->WindowColors [DF_HILITE_COLOR] [DF_BG] & 255) | 0x80;
                cp1 = cp;
                if ((cp = strchr(cp, ']')) != NULL)    {
                    if (thisword != NULL)
                        thisword->off3 =
                            thisword->off2 + (int) (cp - cp1);
                    *cp++ = DF_RESETCOLOR;
                }
                if ((cp = strchr(cp, '<')) != NULL)    {
                    char *cp1 = strchr(cp, '>');
                    if (cp1 != NULL)    {
                        int len = (int) ((int)cp1 - (int)cp);
                        thisword->hname = DfCalloc(1, len);
                        strncpy(thisword->hname, cp+1, len-1);
                        memmove(cp, cp1+1, strlen(cp1));
                    }
                }
            }
        }
        DfPutItemText(wnd, DF_ID_HELPTEXT, hline);
        /* -- display help text as soon as window is full -- */
        if (++linectr == DfClientHeight(cwnd))
            DfSendMessage(cwnd, DFM_PAINT, 0, 0);
        if (linectr > DfClientHeight(cwnd) &&
                !DfTestAttribute(cwnd, DF_VSCROLLBAR))    {
            DfAddAttribute(cwnd, DF_VSCROLLBAR);
            DfSendMessage(cwnd, DFM_BORDER, 0, 0);
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
void DfLoadHelpFile()
{
    char *cp;

    if (Helping)
        return;
    DfUnLoadHelpFile();
    if ((helpfp = DfOpenHelpFile()) == NULL)
        return;
    *hline = '\0';
    while (*hline != '<')    {
        if (DfGetHelpLine(hline) == NULL)    {
            fclose(helpfp);
            return;
        }
    }
    while (*hline == '<')   {
        if (strncmp(hline, "<end>", 5) == 0)
            break;

        /* -------- parse the help window's text name ----- */
        if ((cp = strchr(hline, '>')) != NULL)    {
            ThisHelp = DfCalloc(1, sizeof(struct helps));
            if (FirstHelp == NULL)
            FirstHelp = ThisHelp;
            *cp = '\0';
            ThisHelp->hname=DfMalloc(strlen(hline+1)+1);
            strcpy(ThisHelp->hname, hline+1);

            DfHelpFilePosition(&ThisHelp->hptr, &ThisHelp->bit);

            if (DfGetHelpLine(hline) == NULL)
                break;

            /* ------- build the help linked list entry --- */
            while (*hline == '[')    {
                DfHelpFilePosition(&ThisHelp->hptr,
                                            &ThisHelp->bit);
                /* ---- parse the <<prev button pointer ---- */
                if (strncmp(hline, "[<<]", 4) == 0)    {
                    char *cp = strchr(hline+4, '<');
                    if (cp != NULL)    {
                        char *cp1 = strchr(cp, '>');
                        if (cp1 != NULL)    {
                            int len = (int) (cp1-cp);
                            ThisHelp->PrevName=DfCalloc(1,len);
                            strncpy(ThisHelp->PrevName,
                                cp+1,len-1);
                        }
                    }
                    if (DfGetHelpLine(hline) == NULL)
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
                            ThisHelp->NextName=DfCalloc(1,len);
                            strncpy(ThisHelp->NextName,
                                            cp+1,len-1);
                        }
                    }
                    if (DfGetHelpLine(hline) == NULL)
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
        if (DfGetHelpLine(hline) == NULL)
            strcpy(hline, "<end>");
        while (*hline != '<')    {
            ThisHelp->hwidth =
                max(ThisHelp->hwidth, HelpLength(hline));
            ThisHelp->hheight++;
            if (DfGetHelpLine(hline) == NULL)
                strcpy(hline, "<end>");
        }
    }
    fclose(helpfp);
}

/* ------ free the memory used by the help file table ------ */
void DfUnLoadHelpFile(void)
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
    free(DfHelpTree);
	DfHelpTree = NULL;
}

/* ---------- display a specified help text ----------- */
BOOL DfDisplayHelp(DFWINDOW wnd, char *Help)
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
            ThisStack = DfCalloc(1,sizeof(struct HelpStack));
            ThisStack->hname = DfMalloc(strlen(Help)+1);
            if (ThisStack->hname != NULL)
                strcpy(ThisStack->hname, Help);
            ThisStack->PrevStack = LastStack;
            LastStack = ThisStack;
        }
        if ((helpfp = DfOpenHelpFile()) != NULL)    {
            DF_DBOX *db;
            int offset, i;

            db = DfCalloc(1,sizeof HelpBox);
            memcpy(db, &HelpBox, sizeof HelpBox);
            /* -- seek to the first line of the help text -- */
            DfSeekHelpLine(ThisHelp->hptr, ThisHelp->bit);
            /* ----- read the title ----- */
            DfGetHelpLine(hline);
            hline[strlen(hline)-1] = '\0';
            db->dwnd.title = DfMalloc(strlen(hline)+1);
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
                    DfDisableButton(db, DF_ID_BACK);
            if (ThisHelp->NextName == NULL)
                DfDisableButton(db, DF_ID_NEXT);
            if (ThisHelp->PrevName == NULL)
                DfDisableButton(db, DF_ID_PREV);
            /* ------- display the help window ----- */
            DfDialogBox(NULL, db, TRUE, DfHelpBoxProc);
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

    if (DfGetClass(wnd) == DF_POPDOWNMENU)
        hwnd = DfGetParent(wnd);
    y = DfGetClass(hwnd) == DF_MENUBAR ? 2 : 1;
    FindHelp(def);
    if (ThisHelp != NULL)    {
        if ((helpfp = DfOpenHelpFile()) != NULL)    {
            dwnd = DfDfCreateWindow(
                        DF_TEXTBOX,
                        NULL,
                        DfGetClientLeft(hwnd),
                        DfGetClientTop(hwnd)+y,
                        min(ThisHelp->hheight, MAXHEIGHT)+3,
                        ThisHelp->hwidth+2,
                        NULL,
                        wnd,
                        NULL,
                        DF_HASBORDER | DF_NOCLIP | DF_SAVESELF);
            if (dwnd != NULL)    {
                /* ----- read the help text ------- */
                DfSeekHelpLine(ThisHelp->hptr, ThisHelp->bit);
                while (TRUE)    {
                    if (DfGetHelpLine(hline) == NULL)
                        break;
                    if (*hline == '<')
                        break;
                    hline[strlen(hline)-1] = '\0';
                    DfSendMessage(dwnd,DFM_ADDTEXT,(DF_PARAM)hline,0);
                }
                DfSendMessage(dwnd, DFM_SHOW_WINDOW, 0, 0);
                DfSendMessage(NULL, DFM_WAITKEYBOARD, 0, 0);
                DfSendMessage(NULL, DFM_WAITMOUSE, 0, 0);
                DfSendMessage(dwnd, DFM_CLOSE_WINDOW, 0, 0);
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
static void BestFit(DFWINDOW wnd, DF_DIALOGWINDOW *dwnd)
{
    int above, below, right, left;
    if (DfGetClass(wnd) == DF_MENUBAR ||
                DfGetClass(wnd) == DF_APPLICATION)    {
        dwnd->x = dwnd->y = -1;
        return;
    }
    /* --- compute above overlap ---- */
    above = OverLap(dwnd->h, DfGetTop(wnd));
    /* --- compute below overlap ---- */
    below = OverLap(DfGetBottom(wnd), DfGetScreenHeight()-dwnd->h);
    /* --- compute right overlap ---- */
    right = OverLap(DfGetRight(wnd), DfGetScreenWidth()-dwnd->w);
    /* --- compute left  overlap ---- */
    left = OverLap(dwnd->w, DfGetLeft(wnd));

    if (above < below)
        dwnd->y = max(0, DfGetTop(wnd)-dwnd->h-2);
    else
        dwnd->y = min(DfGetScreenHeight()-dwnd->h, DfGetBottom(wnd)+2);
    if (right < left)
        dwnd->x = min(DfGetRight(wnd)+2, DfGetScreenWidth()-dwnd->w);
    else
        dwnd->x = max(0, DfGetLeft(wnd)-dwnd->w-2);

    if (dwnd->x == DfGetRight(wnd)+2 ||
            dwnd->x == DfGetLeft(wnd)-dwnd->w-2)
        dwnd->y = -1;
    if (dwnd->y ==DfGetTop(wnd)-dwnd->h-2 ||
            dwnd->y == DfGetBottom(wnd)+2)
        dwnd->x = -1;
}

/* EOF */
