/* ------------- popdown.c ----------- */

#include "dflat.h"

static int SelectionWidth(struct PopDown *);
static int py = -1;

/* ------------ CREATE_WINDOW Message ------------- */
static int CreateWindowMsg(DFWINDOW wnd)
{
    int rtn, adj;
    ClearAttribute(wnd, HASTITLEBAR     |
                        VSCROLLBAR     |
                        MOVEABLE     |
                        SIZEABLE     |
                        HSCROLLBAR);
	/* ------ adjust to keep popdown on screen ----- */
	adj = SCREENHEIGHT-1-wnd->rc.bt;
	if (adj < 0)	{
		wnd->rc.tp += adj;
		wnd->rc.bt += adj;
	}
	adj = SCREENWIDTH-1-wnd->rc.rt;
	if (adj < 0)	{
		wnd->rc.lf += adj;
		wnd->rc.rt += adj;
	}
    rtn = BaseWndProc(POPDOWNMENU, wnd, CREATE_WINDOW, 0, 0);
    DfSendMessage(wnd, CAPTURE_MOUSE, 0, 0);
    DfSendMessage(wnd, CAPTURE_KEYBOARD, 0, 0);
    DfSendMessage(NULL, SAVE_CURSOR, 0, 0);
    DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
	wnd->oldFocus = inFocus;
	inFocus = wnd;
    return rtn;
}

/* --------- LEFT_BUTTON Message --------- */
static void LeftButtonMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int my = (int) p2 - GetTop(wnd);
    if (InsideRect(p1, p2, ClientRect(wnd)))    {
        if (my != py)    {
            DfSendMessage(wnd, LB_SELECTION,
                    (PARAM) wnd->wtop+my-1, TRUE);
            py = my;
        }
    }
    else if ((int)p2 == GetTop(GetParent(wnd)))
        if (GetClass(GetParent(wnd)) == MENUBAR)
            DfPostMessage(GetParent(wnd), LEFT_BUTTON, p1, p2);
}

/* -------- BUTTON_RELEASED Message -------- */
static BOOL ButtonReleasedMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    py = -1;
    if (InsideRect((int)p1, (int)p2, ClientRect(wnd)))    {
        int sel = (int)p2 - GetClientTop(wnd);
        if (*TextLine(wnd, sel) != LINE)
            DfSendMessage(wnd, LB_CHOOSE, wnd->selection, 0);
    }
    else    {
        DFWINDOW pwnd = GetParent(wnd);
        if (GetClass(pwnd) == MENUBAR && (int)p2==GetTop(pwnd))
            return FALSE;
        if ((int)p1 == GetLeft(pwnd)+2)
            return FALSE;
        DfSendMessage(wnd, CLOSE_WINDOW, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* --------- PAINT Message -------- */
static void PaintMsg(DFWINDOW wnd)
{
    int wd;
    unsigned char sep[80], *cp = sep;
    unsigned char sel[80];
    struct PopDown *ActivePopDown;
    struct PopDown *pd1;

    ActivePopDown = pd1 = wnd->mnu->Selections;
    wd = MenuWidth(ActivePopDown)-2;
    while (wd--)
        *cp++ = LINE;
    *cp = '\0';
    DfSendMessage(wnd, CLEARTEXT, 0, 0);
    wnd->selection = wnd->mnu->Selection;
    while (pd1->SelectionTitle != NULL)    {
        if (*pd1->SelectionTitle == LINE)
            DfSendMessage(wnd, ADDTEXT, (PARAM) sep, 0);
        else    {
            int len;
            memset(sel, '\0', sizeof sel);
            if (pd1->Attrib & INACTIVE)
                /* ------ inactive menu selection ----- */
                sprintf(sel, "%c%c%c",
                    CHANGECOLOR,
                    wnd->WindowColors [HILITE_COLOR] [FG]|0x80,
                    wnd->WindowColors [STD_COLOR] [BG]|0x80);
            strcat(sel, " ");
            if (pd1->Attrib & CHECKED)
                /* ---- paint the toggle checkmark ---- */
                sel[strlen(sel)-1] = CHECKMARK;
            len=CopyCommand(sel+strlen(sel),pd1->SelectionTitle,
                    pd1->Attrib & INACTIVE,
                    wnd->WindowColors [STD_COLOR] [BG]);
            if (pd1->Accelerator)    {
                /* ---- paint accelerator key ---- */
                int i;
                int wd1 = 2+SelectionWidth(ActivePopDown) -
                                    strlen(pd1->SelectionTitle);
                for (i = 0; keys[i].keylabel; i++)    {
                    if (keys[i].keycode == pd1->Accelerator)   {
                        while (wd1--)
                            strcat(sel, " ");
                        sprintf(sel+strlen(sel), "[%s]",
                            keys[i].keylabel);
                        break;
                    }
                }
            }
            if (pd1->Attrib & CASCADED)    {
                /* ---- paint cascaded menu token ---- */
                if (!pd1->Accelerator)    {
                    wd = MenuWidth(ActivePopDown)-len+1;
                    while (wd--)
                        strcat(sel, " ");
                }
                sel[strlen(sel)-1] = CASCADEPOINTER;
            }
            else
                strcat(sel, " ");
            strcat(sel, " ");
            sel[strlen(sel)-1] = RESETCOLOR;
            DfSendMessage(wnd, ADDTEXT, (PARAM) sel, 0);
        }
        pd1++;
    }
}

/* ---------- BORDER Message ----------- */
static int BorderMsg(DFWINDOW wnd)
{
    int i, rtn = TRUE;
    DFWINDOW currFocus;
    if (wnd->mnu != NULL)    {
        currFocus = inFocus;
        inFocus = NULL;
        rtn = BaseWndProc(POPDOWNMENU, wnd, BORDER, 0, 0);
        inFocus = currFocus;
        for (i = 0; i < ClientHeight(wnd); i++)    {
            if (*TextLine(wnd, i) == LINE)    {
                wputch(wnd, LEDGE, 0, i+1);
                wputch(wnd, REDGE, WindowWidth(wnd)-1, i+1);
            }
        }
    }
    return rtn;
}

/* -------------- LB_CHOOSE Message -------------- */
static void LBChooseMsg(DFWINDOW wnd, PARAM p1)
{
    struct PopDown *ActivePopDown = wnd->mnu->Selections;
    if (ActivePopDown != NULL)    {
        int *attr = &(ActivePopDown+(int)p1)->Attrib;
        wnd->mnu->Selection = (int)p1;
        if (!(*attr & INACTIVE))    {
            if (*attr & TOGGLE)
                *attr ^= CHECKED;
            DfPostMessage(GetParent(wnd), DFM_COMMAND,
                (ActivePopDown+(int)p1)->ActionId, p1);
        }
        else
            beep();
    }
}

/* ---------- KEYBOARD Message --------- */
static BOOL KeyboardMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
	struct PopDown *ActivePopDown = wnd->mnu->Selections;

	if (wnd->mnu != NULL)
	{
		if (ActivePopDown != NULL)
		{
			int c = (int)p1;
			int sel = 0;
			int a;
			struct PopDown *pd = ActivePopDown;

			if ((c & OFFSET) == 0)
				c = tolower(c);
			a = AltConvert(c);

			while (pd->SelectionTitle != NULL)
			{
				char *cp = strchr(pd->SelectionTitle, SHORTCUTCHAR);

/* FIXME: AltConvert bug !! */
				if ((cp && tolower(*(cp+1)) == c) ||
//					(a && tolower(*(cp+1)) == a) ||
					pd->Accelerator == c)
				{
					DfPostMessage(wnd, LB_SELECTION, sel, 0);
					DfPostMessage(wnd, LB_CHOOSE, sel, TRUE);
					return TRUE;
				}
				pd++;
				sel++;
			}
		}
	}

	switch ((int)p1)
	{
        case F1:
            if (ActivePopDown == NULL)
                DfSendMessage(GetParent(wnd), KEYBOARD, p1, p2);
            else 
                DisplayHelp(wnd,
                    (ActivePopDown+wnd->selection)->help);
            return TRUE;
        case ESC:
            DfSendMessage(wnd, CLOSE_WINDOW, 0, 0);
            return TRUE;
        case FWD:
        case BS:
            if (GetClass(GetParent(wnd)) == MENUBAR)
                DfPostMessage(GetParent(wnd), KEYBOARD, p1, p2);
            return TRUE;
        case UP:
            if (wnd->selection == 0)    {
                if (wnd->wlines == ClientHeight(wnd))    {
                    DfPostMessage(wnd, LB_SELECTION,
                                    wnd->wlines-1, FALSE);
                    return TRUE;
                }
            }
            break;
        case DN:
            if (wnd->selection == wnd->wlines-1)    {
                if (wnd->wlines == ClientHeight(wnd))    {
                    DfPostMessage(wnd, LB_SELECTION, 0, FALSE);
                    return TRUE;
                }
            }
            break;
        case HOME:
        case END:
        case '\r':
            break;
        default:
            return TRUE;
    }
    return FALSE;
}

/* ----------- CLOSE_WINDOW Message ---------- */
static int CloseWindowMsg(DFWINDOW wnd)
{
    int rtn;
	DFWINDOW pwnd = GetParent(wnd);
    DfSendMessage(wnd, RELEASE_MOUSE, 0, 0);
    DfSendMessage(wnd, RELEASE_KEYBOARD, 0, 0);
    DfSendMessage(NULL, RESTORE_CURSOR, 0, 0);
	inFocus = wnd->oldFocus;
    rtn = BaseWndProc(POPDOWNMENU, wnd, CLOSE_WINDOW, 0, 0);
    DfSendMessage(pwnd, CLOSE_POPDOWN, 0, 0);
    return rtn;
}

/* - Window processing module for POPDOWNMENU window class - */
int PopDownProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)    {
        case CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case LEFT_BUTTON:
            LeftButtonMsg(wnd, p1, p2);
            return FALSE;
        case DOUBLE_CLICK:
            return TRUE;
        case LB_SELECTION:
            if (*TextLine(wnd, (int)p1) == LINE)
                return TRUE;
            wnd->mnu->Selection = (int)p1;
            break;
        case DFM_BUTTON_RELEASED:
            if (ButtonReleasedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case BUILD_SELECTIONS:
            wnd->mnu = (void *) p1;
            wnd->selection = wnd->mnu->Selection;
            break;
        case PAINT:
            if (wnd->mnu == NULL)
                return TRUE;
            PaintMsg(wnd);
            break;
        case BORDER:
            return BorderMsg(wnd);
        case LB_CHOOSE:
            LBChooseMsg(wnd, p1);
            return TRUE;
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case CLOSE_WINDOW:
            return CloseWindowMsg(wnd);
        default:
            break;
    }
    return BaseWndProc(POPDOWNMENU, wnd, msg, p1, p2);
}

/* --------- compute menu height -------- */
int MenuHeight(struct PopDown *pd)
{
    int ht = 0;
    while (pd[ht].SelectionTitle != NULL)
        ht++;
    return ht+2;
}

/* --------- compute menu width -------- */
int MenuWidth(struct PopDown *pd)
{
    int wd = 0, i;
    int len = 0;

    wd = SelectionWidth(pd);
    while (pd->SelectionTitle != NULL)    {
        if (pd->Accelerator)    {
            for (i = 0; keys[i].keylabel; i++)
                if (keys[i].keycode == pd->Accelerator)    {
                    len = max(len, (int)(2+strlen(keys[i].keylabel)));
                    break;
                }
        }
        if (pd->Attrib & CASCADED)
            len = max(len, 2);
        pd++;
    }
    return wd+5+len;
}

/* ---- compute the maximum selection width in a menu ---- */
static int SelectionWidth(struct PopDown *pd)
{
    int wd = 0;
    while (pd->SelectionTitle != NULL)    {
        int len = strlen(pd->SelectionTitle)-1;
        wd = max(wd, len);
        pd++;
    }
    return wd;
}

/* ----- copy a menu command to a display buffer ---- */
int CopyCommand(unsigned char *dest, unsigned char *src,
                                        int skipcolor, int bg)
{
    unsigned char *d = dest;
    while (*src && *src != '\n')    {
        if (*src == SHORTCUTCHAR)    {
            src++;
            if (!skipcolor)    {
                *dest++ = CHANGECOLOR;
                *dest++ = cfg.clr[POPDOWNMENU]
                            [HILITE_COLOR] [BG] | 0x80;
                *dest++ = bg | 0x80;
                *dest++ = *src++;
                *dest++ = RESETCOLOR;
            }
        }
        else
            *dest++ = *src++;
    }
    return (int) (dest - d);
}

/* EOF */
