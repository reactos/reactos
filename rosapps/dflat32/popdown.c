/* ------------- popdown.c ----------- */

#include "dflat.h"

static int SelectionWidth(struct DfPopDown *);
static int py = -1;

/* ------------ DFM_CREATE_WINDOW Message ------------- */
static int CreateWindowMsg(DFWINDOW wnd)
{
	int rtn, adj;

	DfClearAttribute (wnd,
	                DF_HASTITLEBAR |
	                DF_VSCROLLBAR  |
	                DF_MOVEABLE    |
	                DF_SIZEABLE    |
	                DF_HSCROLLBAR);

	/* ------ adjust to keep popdown on screen ----- */
	adj = DfGetScreenHeight()-1-wnd->rc.bt;
	if (adj < 0)
	{
		wnd->rc.tp += adj;
		wnd->rc.bt += adj;
	}
	adj = DfGetScreenWidth()-1-wnd->rc.rt;
	if (adj < 0)
	{
		wnd->rc.lf += adj;
		wnd->rc.rt += adj;
	}

	rtn = DfBaseWndProc(DF_POPDOWNMENU, wnd, DFM_CREATE_WINDOW, 0, 0);
	DfSendMessage(wnd, DFM_CAPTURE_MOUSE, 0, 0);
	DfSendMessage(wnd, DFM_CAPTURE_KEYBOARD, 0, 0);
	DfSendMessage(NULL, DFM_SAVE_CURSOR, 0, 0);
	DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
	wnd->oldFocus = DfInFocus;
	DfInFocus = wnd;

	return rtn;
}

/* --------- DFM_LEFT_BUTTON Message --------- */
static void LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int my = (int) p2 - DfGetTop(wnd);
    if (DfInsideRect(p1, p2, DfClientRect(wnd)))    {
        if (my != py)    {
            DfSendMessage(wnd, DFM_LB_SELECTION,
                    (DF_PARAM) wnd->wtop+my-1, TRUE);
            py = my;
        }
    }
    else if ((int)p2 == DfGetTop(DfGetParent(wnd)))
        if (DfGetClass(DfGetParent(wnd)) == DF_MENUBAR)
            DfPostMessage(DfGetParent(wnd), DFM_LEFT_BUTTON, p1, p2);
}

/* -------- BUTTON_RELEASED Message -------- */
static BOOL ButtonReleasedMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    py = -1;
    if (DfInsideRect((int)p1, (int)p2, DfClientRect(wnd)))    {
        int sel = (int)p2 - DfGetClientTop(wnd);
        if (*DfTextLine(wnd, sel) != DF_LINE)
            DfSendMessage(wnd, DFM_LB_CHOOSE, wnd->selection, 0);
    }
    else    {
        DFWINDOW pwnd = DfGetParent(wnd);
        if (DfGetClass(pwnd) == DF_MENUBAR && (int)p2==DfGetTop(pwnd))
            return FALSE;
        if ((int)p1 == DfGetLeft(pwnd)+2)
            return FALSE;
        DfSendMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* --------- DFM_PAINT Message -------- */
static void PaintMsg(DFWINDOW wnd)
{
    int wd;
    unsigned char sep[80], *cp = sep;
    unsigned char sel[80];
    struct DfPopDown *ActivePopDown;
    struct DfPopDown *pd1;

    ActivePopDown = pd1 = wnd->mnu->Selections;
    wd = DfMenuWidth(ActivePopDown)-2;
    while (wd--)
        *cp++ = DF_LINE;
    *cp = '\0';
    DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
    wnd->selection = wnd->mnu->Selection;
    while (pd1->SelectionTitle != NULL)    {
        if (*pd1->SelectionTitle == DF_LINE)
            DfSendMessage(wnd, DFM_ADDTEXT, (DF_PARAM) sep, 0);
        else    {
            int len;
            memset(sel, '\0', sizeof sel);
            if (pd1->Attrib & DF_INACTIVE)
                /* ------ inactive menu selection ----- */
                sprintf(sel, "%c%c%c",
                    DF_CHANGECOLOR,
                    wnd->WindowColors [DF_HILITE_COLOR] [DF_FG]|0x80,
                    wnd->WindowColors [DF_STD_COLOR] [DF_BG]|0x80);
            strcat(sel, " ");
            if (pd1->Attrib & DF_CHECKED)
                /* ---- paint the toggle checkmark ---- */
                sel[strlen(sel)-1] = DF_CHECKMARK;
            len=DfCopyCommand(sel+strlen(sel),pd1->SelectionTitle,
                    pd1->Attrib & DF_INACTIVE,
                    wnd->WindowColors [DF_STD_COLOR] [DF_BG]);
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
            if (pd1->Attrib & DF_CASCADED)    {
                /* ---- paint cascaded menu token ---- */
                if (!pd1->Accelerator)    {
                    wd = DfMenuWidth(ActivePopDown)-len+1;
                    while (wd--)
                        strcat(sel, " ");
                }
                sel[strlen(sel)-1] = DF_CASCADEPOINTER;
            }
            else
                strcat(sel, " ");
            strcat(sel, " ");
            sel[strlen(sel)-1] = DF_RESETCOLOR;
            DfSendMessage(wnd, DFM_ADDTEXT, (DF_PARAM) sel, 0);
        }
        pd1++;
    }
}

/* ---------- DFM_BORDER Message ----------- */
static int BorderMsg(DFWINDOW wnd)
{
    int i, rtn = TRUE;
    DFWINDOW currFocus;
    if (wnd->mnu != NULL)    {
        currFocus = DfInFocus;
        DfInFocus = NULL;
        rtn = DfBaseWndProc(DF_POPDOWNMENU, wnd, DFM_BORDER, 0, 0);
        DfInFocus = currFocus;
        for (i = 0; i < DfClientHeight(wnd); i++)    {
            if (*DfTextLine(wnd, i) == DF_LINE)    {
                DfWPutch(wnd, DF_LEDGE, 0, i+1);
                DfWPutch(wnd, DF_REDGE, DfWindowWidth(wnd)-1, i+1);
            }
        }
    }
    return rtn;
}

/* -------------- DFM_LB_CHOOSE Message -------------- */
static void LBChooseMsg(DFWINDOW wnd, DF_PARAM p1)
{
    struct DfPopDown *ActivePopDown = wnd->mnu->Selections;
    if (ActivePopDown != NULL)    {
        int *attr = &(ActivePopDown+(int)p1)->Attrib;
        wnd->mnu->Selection = (int)p1;
        if (!(*attr & DF_INACTIVE))    {
            if (*attr & DF_TOGGLE)
                *attr ^= DF_CHECKED;
            DfPostMessage(DfGetParent(wnd), DFM_COMMAND,
                (ActivePopDown+(int)p1)->ActionId, p1);
        }
        else
            DfBeep();
    }
}

/* ---------- DFM_KEYBOARD Message --------- */
static BOOL KeyboardMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
	struct DfPopDown *ActivePopDown = wnd->mnu->Selections;

	if (wnd->mnu != NULL)
	{
		if (ActivePopDown != NULL)
		{
			int c = (int)p1;
			int sel = 0;
			int a;
			struct DfPopDown *pd = ActivePopDown;

			if ((c & DF_OFFSET) == 0)
				c = tolower(c);
			a = DfAltConvert(c);

			while (pd->SelectionTitle != NULL)
			{
				char *cp = strchr(pd->SelectionTitle, DF_SHORTCUTCHAR);

/* FIXME: DfAltConvert bug !! */
				if ((cp && tolower(*(cp+1)) == c) ||
//					(a && tolower(*(cp+1)) == a) ||
					pd->Accelerator == c)
				{
					DfPostMessage(wnd, DFM_LB_SELECTION, sel, 0);
					DfPostMessage(wnd, DFM_LB_CHOOSE, sel, TRUE);
					return TRUE;
				}
				pd++;
				sel++;
			}
		}
	}

	switch ((int)p1)
	{
        case DF_F1:
            if (ActivePopDown == NULL)
                DfSendMessage(DfGetParent(wnd), DFM_KEYBOARD, p1, p2);
            else 
                DfDisplayHelp(wnd,
                    (ActivePopDown+wnd->selection)->help);
            return TRUE;
        case DF_ESC:
            DfSendMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
            return TRUE;
        case DF_FWD:
        case DF_BS:
            if (DfGetClass(DfGetParent(wnd)) == DF_MENUBAR)
                DfPostMessage(DfGetParent(wnd), DFM_KEYBOARD, p1, p2);
            return TRUE;
        case DF_UP:
            if (wnd->selection == 0)    {
                if (wnd->wlines == DfClientHeight(wnd))    {
                    DfPostMessage(wnd, DFM_LB_SELECTION,
                                    wnd->wlines-1, FALSE);
                    return TRUE;
                }
            }
            break;
        case DF_DN:
            if (wnd->selection == wnd->wlines-1)    {
                if (wnd->wlines == DfClientHeight(wnd))    {
                    DfPostMessage(wnd, DFM_LB_SELECTION, 0, FALSE);
                    return TRUE;
                }
            }
            break;
        case DF_HOME:
        case DF_END:
        case '\r':
            break;
        default:
            return TRUE;
    }
    return FALSE;
}

/* ----------- DFM_CLOSE_WINDOW Message ---------- */
static int CloseWindowMsg(DFWINDOW wnd)
{
    int rtn;
	DFWINDOW pwnd = DfGetParent(wnd);
    DfSendMessage(wnd, DFM_RELEASE_MOUSE, 0, 0);
    DfSendMessage(wnd, DFM_RELEASE_KEYBOARD, 0, 0);
    DfSendMessage(NULL, DFM_RESTORE_CURSOR, 0, 0);
	DfInFocus = wnd->oldFocus;
    rtn = DfBaseWndProc(DF_POPDOWNMENU, wnd, DFM_CLOSE_WINDOW, 0, 0);
    DfSendMessage(pwnd, DFM_CLOSE_POPDOWN, 0, 0);
    return rtn;
}

/* - Window processing module for DF_POPDOWNMENU window class - */
int DfPopDownProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case DFM_LEFT_BUTTON:
            LeftButtonMsg(wnd, p1, p2);
            return FALSE;
        case DOUBLE_CLICK:
            return TRUE;
        case DFM_LB_SELECTION:
            if (*DfTextLine(wnd, (int)p1) == DF_LINE)
                return TRUE;
            wnd->mnu->Selection = (int)p1;
            break;
        case DFM_BUTTON_RELEASED:
            if (ButtonReleasedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_BUILD_SELECTIONS:
            wnd->mnu = (void *) p1;
            wnd->selection = wnd->mnu->Selection;
            break;
        case DFM_PAINT:
            if (wnd->mnu == NULL)
                return TRUE;
            PaintMsg(wnd);
            break;
        case DFM_BORDER:
            return BorderMsg(wnd);
        case DFM_LB_CHOOSE:
            LBChooseMsg(wnd, p1);
            return TRUE;
        case DFM_KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_CLOSE_WINDOW:
            return CloseWindowMsg(wnd);
        default:
            break;
    }
    return DfBaseWndProc(DF_POPDOWNMENU, wnd, msg, p1, p2);
}

/* --------- compute menu height -------- */
int DfMenuHeight(struct DfPopDown *pd)
{
    int ht = 0;
    while (pd[ht].SelectionTitle != NULL)
        ht++;
    return ht+2;
}

/* --------- compute menu width -------- */
int DfMenuWidth(struct DfPopDown *pd)
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
        if (pd->Attrib & DF_CASCADED)
            len = max(len, 2);
        pd++;
    }
    return wd+5+len;
}

/* ---- compute the maximum selection width in a menu ---- */
static int SelectionWidth(struct DfPopDown *pd)
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
int DfCopyCommand(unsigned char *dest, unsigned char *src,
                                        int skipcolor, int bg)
{
    unsigned char *d = dest;
    while (*src && *src != '\n')    {
        if (*src == DF_SHORTCUTCHAR)    {
            src++;
            if (!skipcolor)    {
                *dest++ = DF_CHANGECOLOR;
                *dest++ = DfCfg.clr[DF_POPDOWNMENU]
                            [DF_HILITE_COLOR] [DF_BG] | 0x80;
                *dest++ = bg | 0x80;
                *dest++ = *src++;
                *dest++ = DF_RESETCOLOR;
            }
        }
        else
            *dest++ = *src++;
    }
    return (int) (dest - d);
}

/* EOF */
