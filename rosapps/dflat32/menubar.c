/* ---------------- menubar.c ------------------ */

#include "dflat.h"

static void reset_menubar(DFWINDOW);

static struct {
    int x1, x2;     /* position in menu bar */
    char sc;        /* shortcut key value   */
} menu[10];
static int mctr;

DF_MBAR *DfActiveMenuBar;
static DF_MENU *ActiveMenu;

static DFWINDOW mwnd;
static BOOL Selecting;

static DFWINDOW Cascaders[DF_MAXCASCADES];
static int casc;
static DFWINDOW GetDocFocus(void);

/* ----------- DFM_SETFOCUS Message ----------- */
static int SetFocusMsg(DFWINDOW wnd, DF_PARAM p1)
{
	int rtn;
	rtn = DfBaseWndProc(DF_MENUBAR, wnd, DFM_SETFOCUS, p1, 0);
	if (!(int)p1)
		DfSendMessage(DfGetParent(wnd), DFM_ADDSTATUS, 0, 0);
	return rtn;
}

/* --------- DFM_BUILDMENU Message --------- */
static void BuildMenuMsg(DFWINDOW wnd, DF_PARAM p1)
{
    int offset = 3;
    reset_menubar(wnd);
    mctr = 0;
    DfActiveMenuBar = (DF_MBAR *) p1;
    ActiveMenu = DfActiveMenuBar->PullDown;
    while (ActiveMenu->Title != NULL &&
            ActiveMenu->Title != (void*)-1)
    {
        char *cp;
        if (strlen(DfGetText(wnd)+offset) <
                strlen(ActiveMenu->Title)+3)
            break;
        DfGetText(wnd) = DfRealloc(DfGetText(wnd),
            strlen(DfGetText(wnd))+5);
        memmove(DfGetText(wnd) + offset+4, DfGetText(wnd) + offset,
                strlen(DfGetText(wnd))-offset+1);
        DfCopyCommand(DfGetText(wnd)+offset,ActiveMenu->Title,FALSE,
                wnd->WindowColors [DF_STD_COLOR] [DF_BG]);
        menu[mctr].x1 = offset;
        offset += strlen(ActiveMenu->Title) + (3+DF_MSPACE);
        menu[mctr].x2 = offset-DF_MSPACE;
        cp = strchr(ActiveMenu->Title, DF_SHORTCUTCHAR);
        if (cp)
            menu[mctr].sc = tolower(*(cp+1));
        mctr++;
        ActiveMenu++;
    }
    ActiveMenu = DfActiveMenuBar->PullDown;
}

/* ---------- DFM_PAINT Message ---------- */
static void PaintMsg(DFWINDOW wnd)
{
	if (Selecting)
		return;
    if (wnd == DfInFocus)
        DfSendMessage(DfGetParent(wnd), DFM_ADDSTATUS, 0, 0);
    DfSetStandardColor(wnd);
    DfWPuts(wnd, DfGetText(wnd), 0, 0);
    if (DfActiveMenuBar == NULL)
	return;
    if (DfActiveMenuBar->ActiveSelection != -1 &&
            (wnd == DfInFocus || mwnd != NULL))    {
        char *sel, *cp;
        int offset, offset1;

        sel = DfMalloc(200);
        offset=menu[DfActiveMenuBar->ActiveSelection].x1;
        offset1=menu[DfActiveMenuBar->ActiveSelection].x2;
        DfGetText(wnd)[offset1] = '\0';
        DfSetReverseColor(wnd);
        memset(sel, '\0', 200);
        strcpy(sel, DfGetText(wnd)+offset);
        cp = strchr(sel, DF_CHANGECOLOR);
        if (cp != NULL)
            *(cp + 2) = DfBackground | 0x80;
        DfWPuts(wnd, sel,
            offset-DfActiveMenuBar->ActiveSelection*4, 0);
        DfGetText(wnd)[offset1] = ' ';
        if (mwnd == NULL && wnd == DfInFocus) {
            char *st = ActiveMenu
                [DfActiveMenuBar->ActiveSelection].StatusText;
            if (st != NULL)
                DfSendMessage(DfGetParent(wnd), DFM_ADDSTATUS,
                    (DF_PARAM)st, 0);
        }
        free(sel);
    }
}

/* ------------ DFM_KEYBOARD Message ------------- */
static void KeyboardMsg(DFWINDOW wnd, DF_PARAM p1)
{
	DF_MENU *mnu;
	int sel;
    if (mwnd == NULL)
    {
        /* ----- search for menu bar shortcut keys ---- */
        int c = tolower((int)p1);
        int a = DfAltConvert((int)p1);
        int j;
        for (j = 0; j < mctr; j++)    {
            if ((DfInFocus == wnd && menu[j].sc == c) ||
                    (a && menu[j].sc == a))    {
				DfSendMessage(wnd, DFM_SETFOCUS, TRUE, 0);
                DfSendMessage(wnd, DFM_MB_SELECTION, j, 0);
                return;
            }
        }
    }
    /* -------- search for accelerator keys -------- */
    mnu = ActiveMenu;
    while (mnu->Title != (void *)-1)    {
        struct DfPopDown *pd = mnu->Selections;
        if (mnu->PrepMenu)
            (*(mnu->PrepMenu))(GetDocFocus(), mnu);
        while (pd->SelectionTitle != NULL)    {
            if (pd->Accelerator == (int) p1)    {
                if (pd->Attrib & DF_INACTIVE)
                    DfBeep();
                else    {
                    if (pd->Attrib & DF_TOGGLE)
                        pd->Attrib ^= DF_CHECKED;
                    DfSendMessage(GetDocFocus(),
                        DFM_SETFOCUS, TRUE, 0);
                    DfPostMessage(DfGetParent(wnd),
                        DFM_COMMAND, pd->ActionId, 0);
                }
                return;
            }
            pd++;
        }
        mnu++;
    }
	switch ((int)p1)
	{
		case DF_F1:
			if (ActiveMenu == NULL || DfActiveMenuBar == NULL)
				break;
			sel = DfActiveMenuBar->ActiveSelection;
			if (sel == -1)
			{
				DfBaseWndProc(DF_MENUBAR, wnd, DFM_KEYBOARD, DF_F1, 0);
				return;
			}
			mnu = ActiveMenu+sel;
			if (mwnd == NULL ||
				mnu->Selections[0].SelectionTitle == NULL)
			{
				DfDisplayHelp(wnd,mnu->Title+1);
				return;
			}
			break;

        case '\r':
            if (mwnd == NULL &&
                    DfActiveMenuBar->ActiveSelection != -1)
                DfSendMessage(wnd, DFM_MB_SELECTION,
                    DfActiveMenuBar->ActiveSelection, 0);
            break;
        case DF_F10:
            if (wnd != DfInFocus && mwnd == NULL)    {
                DfSendMessage(wnd, DFM_SETFOCUS, TRUE, 0);
			    if ( DfActiveMenuBar->ActiveSelection == -1)
			        DfActiveMenuBar->ActiveSelection = 0;
			    DfSendMessage(wnd, DFM_PAINT, 0, 0);
                break;
            }
            /* ------- fall through ------- */
        case DF_ESC:
            if (DfInFocus == wnd && mwnd == NULL)    {
                DfActiveMenuBar->ActiveSelection = -1;
                DfSendMessage(GetDocFocus(),DFM_SETFOCUS,TRUE,0);
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
            }
            break;
        case DF_FWD:
            DfActiveMenuBar->ActiveSelection++;
            if (DfActiveMenuBar->ActiveSelection == mctr)
                DfActiveMenuBar->ActiveSelection = 0;
            if (mwnd != NULL)
                DfSendMessage(wnd, DFM_MB_SELECTION,
                    DfActiveMenuBar->ActiveSelection, 0);
            else 
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
            break;
        case DF_BS:
            if (DfActiveMenuBar->ActiveSelection == 0 ||
					DfActiveMenuBar->ActiveSelection == -1)
                DfActiveMenuBar->ActiveSelection = mctr;
            --DfActiveMenuBar->ActiveSelection;
            if (mwnd != NULL)
                DfSendMessage(wnd, DFM_MB_SELECTION,
                    DfActiveMenuBar->ActiveSelection, 0);
            else 
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
            break;
        default:
            break;
    }
}

/* --------------- DFM_LEFT_BUTTON Message ---------- */
static void LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1)
{
    int i;
    int mx = (int) p1 - DfGetLeft(wnd);
    /* --- compute the selection that the left button hit --- */
    for (i = 0; i < mctr; i++)
        if (mx >= menu[i].x1-4*i &&
                mx <= menu[i].x2-4*i-5)
            break;
    if (i < mctr)
        if (i != DfActiveMenuBar->ActiveSelection || mwnd == NULL)
            DfSendMessage(wnd, DFM_MB_SELECTION, i, 0);
}

/* -------------- DFM_MB_SELECTION Message -------------- */
static void SelectionMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int wd, mx, my;
    DF_MENU *mnu;

	if (!p2)
	{
		DfActiveMenuBar->ActiveSelection = -1;
		DfSendMessage(wnd, DFM_PAINT, 0, 0);
	}
    Selecting = TRUE;
    mnu = ActiveMenu+(int)p1;
    if (mnu->PrepMenu != NULL)
        (*(mnu->PrepMenu))(GetDocFocus(), mnu);
    wd = DfMenuWidth(mnu->Selections);
    if (p2)
    {
		int brd = DfGetRight(wnd);
        mx = DfGetLeft(mwnd) + DfWindowWidth(mwnd) - 1;
		if (mx + wd > brd)
			mx = brd - wd;
        my = DfGetTop(mwnd) + mwnd->selection;
    }
    else
    {
        int offset = menu[(int)p1].x1 - 4 * (int)p1;
        if (mwnd != NULL)
            DfSendMessage(mwnd, DFM_CLOSE_WINDOW, 0, 0);
        DfActiveMenuBar->ActiveSelection = (int) p1;
        if (offset > DfWindowWidth(wnd)-wd)
            offset = DfWindowWidth(wnd)-wd;
        mx = DfGetLeft(wnd)+offset;
        my = DfGetTop(wnd)+1;
    }
    mwnd = DfDfCreateWindow(DF_POPDOWNMENU, NULL,
                mx, my,
                DfMenuHeight(mnu->Selections),
                wd,
                NULL,
                wnd,
                NULL,
                DF_SHADOW);
	if (!p2)
	{
		Selecting = FALSE;
		DfSendMessage(wnd, DFM_PAINT, 0, 0);
		Selecting = TRUE;
	}
	if (mnu->Selections[0].SelectionTitle != NULL)
	{
		DfSendMessage(mwnd, DFM_BUILD_SELECTIONS, (DF_PARAM) mnu, 0);
		DfSendMessage(mwnd, DFM_SETFOCUS, TRUE, 0);
		DfSendMessage(mwnd, DFM_SHOW_WINDOW, 0, 0);
	}
	Selecting = FALSE;
}

/* --------- COMMAND Message ---------- */
static void CommandMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
	if (p1 == DF_ID_HELP)
	{
		DfBaseWndProc(DF_MENUBAR, wnd, DFM_COMMAND, p1, p2);
		return;
	}

	if (DfIsCascadedCommand(DfActiveMenuBar, (int)p1))
	{
        /* find the cascaded menu based on command id in p1 */
        DF_MENU *mnu = ActiveMenu+mctr;
        while (mnu->Title != (void *)-1)    {
            if (mnu->CascadeId == (int) p1)    {
                if (casc < DF_MAXCASCADES)    {
                    Cascaders[casc++] = mwnd;
                    DfSendMessage(wnd, DFM_MB_SELECTION,
                        (DF_PARAM)(mnu-ActiveMenu), TRUE);
                }
                break;
            }
            mnu++;
        }
    }
    else     {
        if (mwnd != NULL)
            DfSendMessage(mwnd, DFM_CLOSE_WINDOW, 0, 0);
        DfSendMessage(GetDocFocus(), DFM_SETFOCUS, TRUE, 0);
        DfPostMessage(DfGetParent(wnd), DFM_COMMAND, p1, p2);
    }
}

/* --------------- DFM_CLOSE_POPDOWN Message --------------- */
static void ClosePopdownMsg(DFWINDOW wnd)
{
	if (casc > 0)
		DfSendMessage(Cascaders[--casc], DFM_CLOSE_WINDOW, 0, 0);
	else
	{
		mwnd = NULL;
		DfActiveMenuBar->ActiveSelection = -1;
		if (!Selecting)
		{
			DfSendMessage(GetDocFocus(), DFM_SETFOCUS, TRUE, 0);
			DfSendMessage(wnd, DFM_PAINT, 0, 0);
		}
	}
}

/* ---------------- DFM_CLOSE_WINDOW Message --------------- */
static void CloseWindowMsg(DFWINDOW wnd)
{
	if (DfGetText(wnd) != NULL)
	{
		free(DfGetText(wnd));
		DfGetText(wnd) = NULL;
	}
	mctr = 0;
	DfActiveMenuBar->ActiveSelection = -1;
	ActiveMenu = NULL;
	DfActiveMenuBar = NULL;
}

/* --- Window processing module for DF_MENUBAR window class --- */
int DfMenuBarProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn;

    switch (msg)    {
        case DFM_CREATE_WINDOW:
            reset_menubar(wnd);
            break;
        case DFM_SETFOCUS:
			return SetFocusMsg(wnd, p1);
        case DFM_BUILDMENU:
            BuildMenuMsg(wnd, p1);
            break;
        case DFM_PAINT:
            if (!DfIsVisible(wnd) || DfGetText(wnd) == NULL)
                break;
            PaintMsg(wnd);
            return FALSE;
        case DFM_BORDER:
		    if (mwnd == NULL)
				DfSendMessage(wnd, DFM_PAINT, 0, 0);
            return TRUE;
        case DFM_KEYBOARD:
            KeyboardMsg(wnd, p1);
            return TRUE;
        case DFM_LEFT_BUTTON:
            LeftButtonMsg(wnd, p1);
            return TRUE;
        case DFM_MB_SELECTION:
            SelectionMsg(wnd, p1, p2);
            break;
        case DFM_COMMAND:
            CommandMsg(wnd, p1, p2);
            return TRUE;
        case DFM_INSIDE_WINDOW:
            return DfInsideRect(p1, p2, DfWindowRect(wnd));
        case DFM_CLOSE_POPDOWN:
            ClosePopdownMsg(wnd);
            return TRUE;
        case DFM_CLOSE_WINDOW:
            CloseWindowMsg(wnd);
            rtn = DfBaseWndProc(DF_MENUBAR, wnd, msg, p1, p2);
            return rtn;
        default:
            break;
    }
    return DfBaseWndProc(DF_MENUBAR, wnd, msg, p1, p2);
}

/* ------------- reset the DF_MENUBAR -------------- */
static void reset_menubar(DFWINDOW wnd)
{
    DfGetText(wnd) = DfRealloc(DfGetText(wnd), DfGetScreenWidth()+5);
    memset(DfGetText(wnd), ' ', DfGetScreenWidth());
    *(DfGetText(wnd)+DfWindowWidth(wnd)) = '\0';
}

static DFWINDOW GetDocFocus(void)
{
	DFWINDOW wnd = DfApplicationWindow;
	if (wnd != NULL)
	{
		wnd = DfLastWindow(wnd);
		while (wnd != NULL &&
		       (DfGetClass(wnd) == DF_MENUBAR ||
		        DfGetClass(wnd) == DF_STATUSBAR))
			wnd = DfPrevWindow(wnd);
		if (wnd != NULL)
		{
			while (wnd->childfocus != NULL)
				wnd = wnd->childfocus;
		}
	}
	return wnd ? wnd : DfApplicationWindow;
}

/* EOF */
