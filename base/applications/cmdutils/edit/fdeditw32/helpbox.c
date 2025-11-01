/* ------------ helpbox.c ----------- */

#include "dflat.h"
#include "htree.h"

extern DBOX HelpBox;

/* Strings of D-Flat classes for calling default help text collections */
char *ClassNames[] = {
    #undef ClassDef
    #define ClassDef(c,b,p,a) #c,
    #include "classes.h"
    NULL
};

#define MAXHEIGHT       (SCREENHEIGHT-10)
#define MAXHELPKEYWORDS 50              /* Maximum keywords in a window */
#define MAXHELPSTACK    100

static struct helps *FirstHelp;
static struct helps *ThisHelp;
static char HelpFileName[9];
static char hline[160];
static int HelpCount;
static int HelpStack[MAXHELPSTACK];
static int stacked;
static int keywordcount;
static FILE *helpfp;
static BOOL Helping;

/* --- keywords in the current help text -------- */
static struct keywords {
	struct helps *hkey;
    int lineno;
    int off1, off2, off3;
    char isDefinition;
} KeyWords[MAXHELPKEYWORDS];
static struct keywords *thisword;

static void SelectHelp(WINDOW, struct helps *, BOOL);
static void ReadHelp(WINDOW);
static struct helps *FindHelp(char *);
static void DisplayDefinition(WINDOW, char *);
static void BestFit(WINDOW, DIALOGWINDOW *);

/* ------------- CREATE_WINDOW message ------------ */
static void CreateWindowMsg(WINDOW wnd)
{
    Helping=TRUE;
    GetClass(wnd)=HELPBOX;
    InitWindowColors(wnd);
    if (ThisHelp != NULL)
        ThisHelp->hwnd=wnd;

}

/* ------------- COMMAND message ------------ */
static BOOL CommandMsg(WINDOW wnd, PARAM p1)
{
    switch ((int)p1)
        {
        case ID_PREV:
            if (ThisHelp != NULL)
                SelectHelp(wnd, FirstHelp+(ThisHelp->prevhlp), TRUE);

            return TRUE;
        case ID_NEXT:
            if (ThisHelp != NULL)
                SelectHelp(wnd, FirstHelp+(ThisHelp->nexthlp), TRUE);

            return TRUE;
        case ID_BACK:
            if (stacked)
                SelectHelp(wnd, FirstHelp+HelpStack[--stacked], FALSE);

            return TRUE;
        default:
            break;

        }

    return FALSE;

}

/* ------------- KEYBOARD message ------------ */
static BOOL KeyboardMsg(WINDOW wnd, PARAM p1)
{
    WINDOW cwnd;

    cwnd=ControlWindow(wnd->extension, ID_HELPTEXT);
    if (cwnd == NULL || inFocus != cwnd)
        return FALSE;

    switch ((int)p1)
        {
        case '\r':
            if (keywordcount)
                if (thisword != NULL)
                    {
                    char *hp=thisword->hkey->hname;

                    if (thisword->isDefinition)
            	        DisplayDefinition(GetParent(wnd), hp);
                    else
                    	SelectHelp(wnd, thisword->hkey, TRUE);

	            }
            return TRUE;
        case '\t':
            if (!keywordcount)
                return TRUE;

            if (thisword == NULL || ++thisword == KeyWords+keywordcount)
                thisword=KeyWords;

            break;
        case SHIFT_HT:
            if (!keywordcount)
                return TRUE;

            if (thisword == NULL || thisword == KeyWords)
                thisword=KeyWords+keywordcount;

            --thisword;
            break;
        default:
            return FALSE;

        }

    if (thisword->lineno < cwnd->wtop || thisword->lineno >= cwnd->wtop+ClientHeight(cwnd))
        {
        int distance=ClientHeight(cwnd)/2;

        do
            {
            cwnd->wtop=thisword->lineno-distance;
            distance /= 2;
            }
        while (cwnd->wtop < 0);

        }

    SendMessage(cwnd, PAINT, 0, 0);
    return TRUE;

}

/* ---- window processing module for the HELPBOX ------- */
int HelpBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            CreateWindowMsg(wnd);
            break;
        case INITIATE_DIALOG:
            ReadHelp(wnd);
            break;
        case COMMAND:
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
            if (ThisHelp != NULL)
                ThisHelp->hwnd=NULL;

            Helping=FALSE;
            break;
        default:
            break;

        }

    return BaseWndProc(HELPBOX, wnd, msg, p1, p2);

}

/* ---- PAINT message for the helpbox text editbox ---- */
static int PaintMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn;

    if (thisword != NULL)
        {
        WINDOW pwnd=GetParent(wnd);
        char *cp;

        cp=TextLine(wnd, thisword->lineno);
        cp+=thisword->off1;
        *(cp+1)=(pwnd->WindowColors[SELECT_COLOR][FG] & 255) | 0x80;
        *(cp+2)=(pwnd->WindowColors[SELECT_COLOR][BG] & 255) | 0x80;
        rtn=DefaultWndProc(wnd, PAINT, p1, p2);
        *(cp+1)=(pwnd->WindowColors[HILITE_COLOR][FG] & 255) | 0x80;
        *(cp+2)=(pwnd->WindowColors[HILITE_COLOR][BG] & 255) | 0x80;
        return rtn;
        }

    return DefaultWndProc(wnd, PAINT, p1, p2);

}

/* ---- LEFT_BUTTON message for the helpbox text editbox ---- */
static int LeftButtonMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn,mx,my,i;

    rtn=DefaultWndProc(wnd, LEFT_BUTTON, p1, p2);
    mx=(int)p1-GetClientLeft(wnd);
    my=(int)p2-GetClientTop(wnd);
    my+=wnd->wtop;
    thisword=KeyWords;
    for (i=0;i<keywordcount;i++)
        {
        if (my == thisword->lineno)
            {
            if (mx >= thisword->off2 && mx < thisword->off3)
                {
                SendMessage(wnd, PAINT, 0, 0);
                if (thisword->isDefinition)
                    {
                    WINDOW pwnd=GetParent(wnd);
                    if (pwnd != NULL)
                        DisplayDefinition(GetParent(pwnd), thisword->hkey->hname);

                    }
                break;

                }

            }

        thisword++;
        }

    if (i == keywordcount)
        thisword=NULL;

    return rtn;

}

/* --- window processing module for HELPBOX's text EDITBOX -- */
int HelpTextProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case KEYBOARD:
            break;
        case PAINT:
            return PaintMsg(wnd, p1, p2);
        case LEFT_BUTTON:
            return LeftButtonMsg(wnd, p1, p2);
        case DOUBLE_CLICK:
            PostMessage(wnd, KEYBOARD, '\r', 0);
            break;
        default:
            break;

        }

    return DefaultWndProc(wnd, msg, p1, p2);

}

/* -------- read the help text into the editbox ------- */
static void ReadHelp(WINDOW wnd)
{
    WINDOW cwnd=ControlWindow(wnd->extension, ID_HELPTEXT);
    int linectr=0;

    if (cwnd == NULL)
        return;

    thisword=KeyWords;
    keywordcount=0;
    cwnd->wndproc=HelpTextProc;
    SendMessage(cwnd, CLEARTEXT, 0, 0);

    /* Read the help text */
    while (TRUE)
        {
        char *cp=hline, *cp1;
        int colorct=0;

        if (GetHelpLine(hline) == NULL)
            break;

        if (*hline == '<')
            break;

        hline[strlen(hline)-1]='\0';

        /* Add help text to the help window */
        while (cp != NULL)
            {
            if ((cp=strchr(cp, '[')) != NULL)
                {
                if (*(cp+1) != '.' && *(cp+1) != '*')
                    {
                    cp++;
                    continue;
                    }

                thisword->lineno=cwnd->wlines;
                thisword->off1=(int) (cp-hline);
                thisword->off2=thisword->off1-colorct * 4;
                thisword->isDefinition=*(cp+1) == '*';
                colorct++;
                *cp++ =CHANGECOLOR;
                *cp++ =(wnd->WindowColors [HILITE_COLOR] [FG] & 255) | 0x80;
                *cp++ =(wnd->WindowColors [HILITE_COLOR] [BG] & 255) | 0x80;
                cp1=cp;
                if ((cp=strchr(cp, ']')) != NULL)
                    {
                    if (thisword != NULL)
                        thisword->off3=thisword->off2+(int) (cp-cp1);

                    *cp++ = RESETCOLOR;
                    }

                if ((cp=strchr(cp, '<')) != NULL)
                    {
                    char *cp1=strchr(cp, '>');
                    if (cp1 != NULL)
                        {
                        char hname[80];
                        int len=(int) (cp1-cp);

                        memset(hname, 0, 80);
                        strncpy(hname, cp+1, len-1);
                        thisword->hkey=FindHelp(hname);
                        memmove(cp, cp1+1, strlen(cp1));
                        }

                    }

                thisword++;
                keywordcount++;
                }

            }

        PutItemText(wnd, ID_HELPTEXT, hline);

        /* Display help text as soon as window is full */
        if (++linectr == ClientHeight(cwnd))
            {
            struct keywords *holdthis=thisword;

            thisword=NULL;
            SendMessage(cwnd, PAINT, 0, 0);
            thisword=holdthis;
            }

        if (linectr > ClientHeight(cwnd) && !TestAttribute(cwnd, VSCROLLBAR))
            {
            AddAttribute(cwnd, VSCROLLBAR);
            SendMessage(cwnd, BORDER, 0, 0);
            }

        }

    thisword=NULL;

}

/* ---- compute the displayed length of a help text line --- */
/*
static int HelpLength(char *s)
{
    int len=strlen(s);
    char *cp=strchr(s, '[');

    while (cp != NULL)
        {
        len -= 4;
        cp=strchr(cp+1, '[');
        }

    cp=strchr(s, '<');
    while (cp != NULL)
        {
        char *cp1=strchr(cp, '>');

        if (cp1 != NULL)
            len -= (int) (cp1-cp)+1;

        cp=strchr(cp1, '<');
        }

    return len;

}
*/

/* ----------- load the help text file ------------ */
int LoadHelpFile(char *fname)
{
    long where;
    int i;

    if (Helping)
        return 0;

    UnLoadHelpFile();
    if ((helpfp=OpenHelpFile(fname, "rb")) == NULL)
        return 0;

    strcpy(HelpFileName, fname);
    fseek(helpfp, - (long) sizeof(long), SEEK_END);
    fread(&where, sizeof(long), 1, helpfp);
    fseek(helpfp, where, SEEK_SET);
    fread(&HelpCount, sizeof(HelpCount), 1, helpfp);
    FirstHelp=DFcalloc(sizeof(struct helps) * HelpCount, 1);
    for (i=0;i<HelpCount;i++)
        {
        int len;

        fread(&len, sizeof(len), 1, helpfp);
        if (len)
            {
            (FirstHelp+i)->hname=DFcalloc(len+1, 1);
            fread((FirstHelp+i)->hname, len+1, 1, helpfp);
            }

        fread(&len, sizeof(len), 1, helpfp);
        if (len)
            {
            (FirstHelp+i)->comment=DFcalloc(len+1, 1);
            fread((FirstHelp+i)->comment, len+1, 1, helpfp);
            }

        fread(&(FirstHelp+i)->hptr, sizeof(int)*5+sizeof(long), 1, helpfp);
	}

    fclose(helpfp);
    helpfp=NULL;

    return 1;
}

/* ------ free the memory used by the help file table ------ */
void UnLoadHelpFile(void)
{
    int i;

    if (FirstHelp != NULL) {
        for (i=0;i<HelpCount;i++) {
            free((FirstHelp+i)->comment);
            free((FirstHelp+i)->hname);
	    }
        free(FirstHelp);
        FirstHelp=NULL;
    }
    if (HelpTree != NULL) {
        free(HelpTree);
        HelpTree=NULL;
    }
}

static void BuildHelpBox(WINDOW wnd)
{
    int offset, i;

    /* Seek to the first line of the help text */
    SeekHelpLine(ThisHelp->hptr, ThisHelp->bit);

    /* Read the title */
    GetHelpLine(hline);
    hline[strlen(hline)-1]='\0';
    free(HelpBox.dwnd.title);
    HelpBox.dwnd.title=DFmalloc(strlen(hline)+1);
    strcpy(HelpBox.dwnd.title, hline);

    /* Set the height and width */
    HelpBox.dwnd.h=min(ThisHelp->hheight, MAXHEIGHT)+7;
    HelpBox.dwnd.w=max(45, ThisHelp->hwidth+6);

    /* Position the help window */
    if (wnd != NULL)
        BestFit(wnd, &HelpBox.dwnd);

    /* Position the command buttons */
    HelpBox.ctl[0].dwnd.w=max(40, ThisHelp->hwidth+2);
    HelpBox.ctl[0].dwnd.h=min(ThisHelp->hheight, MAXHEIGHT)+2;
    offset=(HelpBox.dwnd.w-40) / 2;
    for (i=1;i<5;i++)
        {
        HelpBox.ctl[i].dwnd.y=min(ThisHelp->hheight, MAXHEIGHT)+3;
        HelpBox.ctl[i].dwnd.x=(i-1) * 10+offset;
	}

    /* Disable ineffective buttons */
    if (ThisHelp->nexthlp == -1)
        DisableButton(&HelpBox, ID_NEXT);
    else
        EnableButton(&HelpBox, ID_NEXT);

    if (ThisHelp->prevhlp == -1)
        DisableButton(&HelpBox, ID_PREV);
    else 
        EnableButton(&HelpBox, ID_PREV);

}

/* ----- select a new help window from its name ----- */
static void SelectHelp(WINDOW wnd, struct helps *newhelp, BOOL recall)
{
    if (newhelp != NULL)
        {
        int i,x,y;

        SendMessage(wnd, HIDE_WINDOW, 0, 0);
        if (recall && stacked < MAXHELPSTACK)
            HelpStack[stacked++]=ThisHelp-FirstHelp;

        ThisHelp=newhelp;
        SendMessage(GetParent(wnd), DISPLAY_HELP, (PARAM) ThisHelp->hname, 0);
        if (stacked)
            EnableButton(&HelpBox, ID_BACK);
        else 
            DisableButton(&HelpBox, ID_BACK);

        BuildHelpBox(NULL);
        AddTitle(wnd, HelpBox.dwnd.title);

        /* Reposition and resize the help window */
        HelpBox.dwnd.x=(SCREENWIDTH-HelpBox.dwnd.w)/2;
        HelpBox.dwnd.y=(SCREENHEIGHT-HelpBox.dwnd.h)/2;
        SendMessage(wnd, MOVE, HelpBox.dwnd.x, HelpBox.dwnd.y);
        SendMessage(wnd, SIZE, HelpBox.dwnd.x+HelpBox.dwnd.w-1, HelpBox.dwnd.y+HelpBox.dwnd.h-1);

        /* Reposition the controls */
        for (i=0;i<5;i++)
            {
            WINDOW cwnd=HelpBox.ctl[i].wnd;

            x=HelpBox.ctl[i].dwnd.x+GetClientLeft(wnd);
            y=HelpBox.ctl[i].dwnd.y+GetClientTop(wnd);
            SendMessage(cwnd, MOVE, x, y);
            if (i == 0)
                {
                x+=HelpBox.ctl[i].dwnd.w-1;
                y+=HelpBox.ctl[i].dwnd.h-1;
                SendMessage(cwnd, SIZE, x, y);
                }

            }

        /* Read the help text into the help window */
        ReadHelp(wnd);
        ReFocus(wnd);
        SendMessage(wnd, SHOW_WINDOW, 0, 0);
        }

}
/* ---- strip tildes from the help name ---- */
static void StripTildes(char *fh, char *hp)
{
    while (*hp)
        {
        if (*hp != '~')
            *fh++ = *hp;

        hp++;
	}

    *fh='\0';

}
/* --- return the comment associated with a help window --- */
char *HelpComment(char *Help)
{
    char FixedHelp[30];

    StripTildes(FixedHelp, Help);
    if ((ThisHelp=FindHelp(FixedHelp)) != NULL)
        return ThisHelp->comment;

    return NULL;

}
/* ---------- display help text ----------- */
BOOL DisplayHelp(WINDOW wnd, char *Help)
{
    char FixedHelp[30];
    BOOL rtn=FALSE;

    if (Helping)
        return TRUE;

    StripTildes(FixedHelp, Help);
    stacked=0;
    wnd->isHelping++;
    if ((ThisHelp=FindHelp(FixedHelp)) != NULL)
        {
        if ((helpfp=OpenHelpFile(HelpFileName, "rb")) != NULL)
            {
            BuildHelpBox(wnd);
            DisableButton(&HelpBox, ID_BACK);

            /* Display the help window */
            DialogBox(NULL, &HelpBox, TRUE, HelpBoxProc);
            free(HelpBox.dwnd.title);
            HelpBox.dwnd.title=NULL;
            fclose(helpfp);
            helpfp=NULL;
            rtn=TRUE;
            }

        }

    --wnd->isHelping;
    return rtn;

}

/* ------- display a definition window --------- */
static void DisplayDefinition(WINDOW wnd, char *def)
{
    WINDOW hwnd=wnd,dwnd;
    int y;
    struct helps *HoldThisHelp;

    HoldThisHelp=ThisHelp;
    if (GetClass(wnd) == POPDOWNMENU)
        hwnd=GetParent(wnd);

    y=GetClass(hwnd) == MENUBAR ? 2 : 1;
    if ((ThisHelp=FindHelp(def)) != NULL)
        {
        dwnd=CreateWindow(TEXTBOX, NULL, GetClientLeft(hwnd), GetClientTop(hwnd)+y, min(ThisHelp->hheight, MAXHEIGHT)+3, ThisHelp->hwidth+2, NULL, wnd, NULL, HASBORDER | NOCLIP | SAVESELF);
        if (dwnd != NULL)
            {
            clearBIOSbuffer();

            /* Read the help text */
            SeekHelpLine(ThisHelp->hptr, ThisHelp->bit);
            while (TRUE)
                {
                clearBIOSbuffer();
                if (GetHelpLine(hline) == NULL)
                    break;

                if (*hline == '<')
                    break;

                hline[strlen(hline)-1]='\0';
                SendMessage(dwnd,ADDTEXT,(PARAM)hline,0);
                }

            SendMessage(dwnd, SHOW_WINDOW, 0, 0);
            SendMessage(NULL, WAITKEYBOARD, 0, 0);
            SendMessage(NULL, WAITMOUSE, 0, 0);
            SendMessage(dwnd, CLOSE_WINDOW, 0, 0);
            }

        }

    ThisHelp=HoldThisHelp;

}

/* ------ compare help names with wild cards ----- */
static BOOL wildcmp(char *s1, char *s2)
{
    while (*s1 || *s2)
        {
        if (tolower(*s1) != tolower(*s2))
            if (*s1 != '?' && *s2 != '?')
                return TRUE;

        s1++, s2++;
        }

    return FALSE;

}

/* --- ThisHelp=the help window matching specified name --- */
static struct helps *FindHelp(char *Help)
{
    int i;
    struct helps *thishelp=NULL;

    for (i=0;i<HelpCount;i++)
        {
        if (wildcmp(Help, (FirstHelp+i)->hname) == FALSE)
            {
            thishelp=FirstHelp+i;
            break;
            }

	}

    return thishelp;

}

static int OverLap(int a, int b)
{
    int ov=a-b;

    if (ov<0)
        ov=0;

    return ov;

}

/* ----- compute the best location for a help dialogbox ----- */
static void BestFit(WINDOW wnd, DIALOGWINDOW *dwnd)
{
    int above, below, right, left;

    if (GetClass(wnd) == MENUBAR || GetClass(wnd) == APPLICATION)
        {
        dwnd->x=dwnd->y=-1;
        return;
        }

    above=OverLap(dwnd->h, GetTop(wnd)); /* Compute above overlap */
    below=OverLap(GetBottom(wnd), SCREENHEIGHT-dwnd->h);  /* Compute below overlap */
    right=OverLap(GetRight(wnd), SCREENWIDTH-dwnd->w);  /* Compute right overlap */
    left=OverLap(dwnd->w, GetLeft(wnd)); /* Compute left overlap */
    if (above < below)
        dwnd->y=max(0, GetTop(wnd)-dwnd->h-2);
    else
        dwnd->y=min(SCREENHEIGHT-dwnd->h, GetBottom(wnd)+2);

    if (right < left)
        dwnd->x=min(GetRight(wnd)+2, SCREENWIDTH-dwnd->w);
    else
        dwnd->x=max(0, GetLeft(wnd)-dwnd->w-2);

    if (dwnd->x == GetRight(wnd)+2 || dwnd->x == GetLeft(wnd)-dwnd->w-2)
        dwnd->y=-1;

    if (dwnd->y ==GetTop(wnd)-dwnd->h-2 || dwnd->y == GetBottom(wnd)+2)
        dwnd->x=-1;

}
