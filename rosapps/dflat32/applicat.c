/* ------------- applicat.c ------------- */

#include "dflat.h"

static BOOL DisplayModified = FALSE;
DFWINDOW ApplicationWindow;

extern DBOX Display;
extern DBOX Windows;

#ifdef INCLUDE_LOGGING
extern DBOX Log;
#endif

#ifdef INCLUDE_SHELLDOS
static void ShellDOS(DFWINDOW);
#endif
static void DfCreateMenu(DFWINDOW);
static void CreateStatusBar(DFWINDOW);
static void SelectColors(DFWINDOW);

#ifdef INCLUDE_WINDOWOPTIONS
static void SelectTexture(void);
static void SelectBorder(DFWINDOW);
static void SelectTitle(DFWINDOW);
static void SelectStatusBar(DFWINDOW);
#endif

static DFWINDOW oldFocus;
#ifdef INCLUDE_MULTI_WINDOWS
static void CloseAll(DFWINDOW, int);
static void MoreWindows(DFWINDOW);
static void ChooseWindow(DFWINDOW, int);
static int WindowSel;
static char Menus[9][26] =
{
	"~1.                      ",
	"~2.                      ",
	"~3.                      ",
	"~4.                      ",
	"~5.                      ",
	"~6.                      ",
	"~7.                      ",
	"~8.                      ",
	"~9.                      "
};
#endif

/* --------------- CREATE_WINDOW Message -------------- */
static int CreateWindowMsg(DFWINDOW wnd)
{
	int rtn;

	ApplicationWindow = wnd;
#ifdef INCLUDE_WINDOWOPTIONS
    if (cfg.Border)
        SetCheckBox(&Display, ID_BORDER);
    if (cfg.Title)
        SetCheckBox(&Display, ID_TITLE);
    if (cfg.StatusBar)
        SetCheckBox(&Display, ID_STATUSBAR);
    if (cfg.Texture)
        SetCheckBox(&Display, ID_TEXTURE);
#endif
    SelectColors(wnd);
#ifdef INCLUDE_WINDOWOPTIONS
    SelectBorder(wnd);
    SelectTitle(wnd);
    SelectStatusBar(wnd);
#endif
    rtn = BaseWndProc(APPLICATION, wnd, CREATE_WINDOW, 0, 0);
    if (wnd->extension != NULL)
        DfCreateMenu(wnd);
    CreateStatusBar(wnd);
    return rtn;
}

/* --------- ADDSTATUS Message ---------- */
static void AddStatusMsg(DFWINDOW wnd, PARAM p1)
{
    if (wnd->StatusBar != NULL)    {
        if (p1 && *(char *)p1)
            DfSendMessage(wnd->StatusBar, SETTEXT, p1, 0);
        else 
            DfSendMessage(wnd->StatusBar, CLEARTEXT, 0, 0);
        DfSendMessage(wnd->StatusBar, PAINT, 0, 0);
    }
}

/* -------- SETFOCUS Message -------- */
static void SetFocusMsg(DFWINDOW wnd, BOOL p1)
{
    if (p1)
        DfSendMessage(inFocus, SETFOCUS, FALSE, 0);
    inFocus = p1 ? wnd : NULL;
	if (isVisible(wnd))
	    DfSendMessage(wnd, BORDER, 0, 0);
	else 
	    DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
}

/* ------- SIZE Message -------- */
static void SizeMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    BOOL WasVisible;
    WasVisible = isVisible(wnd);
    if (WasVisible)
        DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    if (p1-GetLeft(wnd) < 30)
        p1 = GetLeft(wnd) + 30;
    BaseWndProc(APPLICATION, wnd, DFM_SIZE, p1, p2);
    DfCreateMenu(wnd);
    CreateStatusBar(wnd);
    if (WasVisible)
        DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
}

/* ----------- KEYBOARD Message ------------ */
static int KeyboardMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    if (WindowMoving || WindowSizing || (int) p1 == F1)
        return BaseWndProc(APPLICATION, wnd, KEYBOARD, p1, p2);
    switch ((int) p1)    {
        case ALT_F4:
            DfPostMessage(wnd, CLOSE_WINDOW, 0, 0);
            return TRUE;
#ifdef INCLUDE_MULTI_WINDOWS
        case ALT_F6:
            SetNextFocus();
            return TRUE;
#endif
        case ALT_HYPHEN:
            BuildSystemMenu(wnd);
            return TRUE;
        default:
            break;
    }
    DfPostMessage(wnd->MenuBarWnd, KEYBOARD, p1, p2);
    return TRUE;
}

/* --------- SHIFT_CHANGED Message -------- */
static void ShiftChangedMsg(DFWINDOW wnd, PARAM p1)
{
	extern BOOL AltDown;
    if ((int)p1 & ALTKEY)
        AltDown = TRUE;
    else if (AltDown)    {
        AltDown = FALSE;
        if (wnd->MenuBarWnd != inFocus)
            DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
        DfSendMessage(wnd->MenuBarWnd, KEYBOARD, F10, 0);
    }
}

/* -------- COMMAND Message ------- */
static void CommandMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    switch ((int)p1)    {
        case ID_HELP:
            DisplayHelp(wnd, DFlatApplication);
            break;
        case ID_HELPHELP:
            DisplayHelp(wnd, "HelpHelp");
            break;
        case ID_EXTHELP:
            DisplayHelp(wnd, "ExtHelp");
            break;
        case ID_KEYSHELP:
            DisplayHelp(wnd, "KeysHelp");
            break;
        case ID_HELPINDEX:
            DisplayHelp(wnd, "HelpIndex");
            break;
#ifdef TESTING_DFLAT
        case ID_LOADHELP:
            LoadHelpFile();
            break;
#endif
#ifdef INCLUDE_LOGGING
        case ID_LOG:
            MessageLog(wnd);
            break;
#endif
#ifdef INCLUDE_SHELLDOS
        case ID_DOS:
            ShellDOS(wnd);
            break;
#endif
        case ID_EXIT:
        case ID_SYSCLOSE:
            DfPostMessage(wnd, CLOSE_WINDOW, 0, 0);
            break;
        case ID_DISPLAY:
            if (DfDialogBox(wnd, &Display, TRUE, NULL))    {
				if (inFocus == wnd->MenuBarWnd || inFocus == wnd->StatusBar)
					oldFocus = ApplicationWindow;
				else 
					oldFocus = inFocus;
                DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
                SelectColors(wnd);
#ifdef INCLUDE_WINDOWOPTIONS
                SelectBorder(wnd);
                SelectTitle(wnd);
                SelectStatusBar(wnd);
                SelectTexture();
#endif
                DfCreateMenu(wnd);
                CreateStatusBar(wnd);
                DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
			    DfSendMessage(oldFocus, SETFOCUS, TRUE, 0);
            }
            break;
        case ID_SAVEOPTIONS:
            SaveConfig();
            break;
#ifdef INCLUDE_MULTI_WINDOWS
        case ID_WINDOW:
            ChooseWindow(wnd, (int)p2-2);
            break;
        case ID_CLOSEALL:
            CloseAll(wnd, FALSE);
            break;
        case ID_MOREWINDOWS:
            MoreWindows(wnd);
            break;
#endif
#ifdef INCLUDE_RESTORE
        case ID_SYSRESTORE:
#endif
        case ID_SYSMOVE:
        case ID_SYSSIZE:
#ifdef INCLUDE_MINIMIZE
        case ID_SYSMINIMIZE:
#endif
#ifdef INCLUDE_MAXIMIZE
        case ID_SYSMAXIMIZE:
#endif
            BaseWndProc(APPLICATION, wnd, DFM_COMMAND, p1, p2);
            break;
        default:
            if (inFocus != wnd->MenuBarWnd && inFocus != wnd)
                DfPostMessage(inFocus, DFM_COMMAND, p1, p2);
            break;
    }
}

/* --------- CLOSE_WINDOW Message -------- */
static int CloseWindowMsg(DFWINDOW wnd)
{
    int rtn;
#ifdef INCLUDE_MULTI_WINDOWS
    CloseAll(wnd, TRUE);
	WindowSel = 0;
#endif
    DfPostMessage(NULL, DFM_STOP, 0, 0);
    rtn = BaseWndProc(APPLICATION, wnd, CLOSE_WINDOW, 0, 0);
    UnLoadHelpFile();
	DisplayModified = FALSE;
	ApplicationWindow = NULL;
    return rtn;
}

/* --- APPLICATION Window Class window processing module --- */
int ApplicationProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
    {
        case CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case DFM_HIDE_WINDOW:
            if (wnd == inFocus)
                inFocus = NULL;
            break;
        case ADDSTATUS:
            AddStatusMsg(wnd, p1);
            return TRUE;
        case SETFOCUS:
            if ((int)p1 == (inFocus != wnd))    {
                SetFocusMsg(wnd, (BOOL) p1);
                return TRUE;
            }
            break;
        case DFM_SIZE:
            SizeMsg(wnd, p1, p2);
            return TRUE;
#ifdef INCLUDE_MINIMIZE
        case MINIMIZE:
            return TRUE;
#endif
        case KEYBOARD:
            return KeyboardMsg(wnd, p1, p2);
        case SHIFT_CHANGED:
            ShiftChangedMsg(wnd, p1);
            return TRUE;
        case PAINT:
            if (isVisible(wnd))    {
#ifdef INCLUDE_WINDOWOPTIONS
                int cl = cfg.Texture ? APPLCHAR : ' ';
#else
                int cl = APPLCHAR;
#endif
                ClearWindow(wnd, (DFRECT *)p1, cl);
            }
            return TRUE;
        case DFM_COMMAND:
            CommandMsg(wnd, p1, p2);
            return TRUE;
        case CLOSE_WINDOW:
            return CloseWindowMsg(wnd);
        default:
            break;
    }
    return BaseWndProc(APPLICATION, wnd, msg, p1, p2);
}

#ifdef INCLUDE_SHELLDOS
static void SwitchCursor(void)
{
    DfSendMessage(NULL, SAVE_CURSOR, 0, 0);
    SwapCursorStack();
    DfSendMessage(NULL, RESTORE_CURSOR, 0, 0);
}

/* ------- Shell out to DOS ---------- */
static void ShellDOS(DFWINDOW wnd)
{
	oldFocus = inFocus;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    SwitchCursor();
    printf("To return to %s, execute the DOS exit command.",
                    DFlatApplication);
    fflush(stdout);
    _spawnl(P_WAIT, getenv("COMSPEC"), " ", NULL);
    SwitchCursor();
	DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
    DfSendMessage(oldFocus, SETFOCUS, TRUE, 0);
}
#endif

/* -------- Create the menu bar -------- */
static void DfCreateMenu(DFWINDOW wnd)
{
    AddAttribute(wnd, HASMENUBAR);
    if (wnd->MenuBarWnd != NULL)
        DfSendMessage(wnd->MenuBarWnd, CLOSE_WINDOW, 0, 0);
    wnd->MenuBarWnd = DfCreateWindow(MENUBAR,
                        NULL,
                        GetClientLeft(wnd),
                        GetClientTop(wnd)-1,
                        1,
                        ClientWidth(wnd),
                        NULL,
                        wnd,
                        NULL,
                        0);
    DfSendMessage(wnd->MenuBarWnd,BUILDMENU,
        (PARAM)wnd->extension,0);
    AddAttribute(wnd->MenuBarWnd, VISIBLE);
}

/* ----------- Create the status bar ------------- */
static void CreateStatusBar(DFWINDOW wnd)
{
    if (wnd->StatusBar != NULL)    {
        DfSendMessage(wnd->StatusBar, CLOSE_WINDOW, 0, 0);
        wnd->StatusBar = NULL;
    }
    if (TestAttribute(wnd, HASSTATUSBAR))    {
        wnd->StatusBar = DfCreateWindow(STATUSBAR,
                            NULL,
                            GetClientLeft(wnd),
                            GetBottom(wnd),
                            1,
                            ClientWidth(wnd),
                            NULL,
                            wnd,
                            NULL,
                            0);
        AddAttribute(wnd->StatusBar, VISIBLE);
    }
}

#ifdef INCLUDE_MULTI_WINDOWS
/* -------- return the name of a document window ------- */
static char *WindowName(DFWINDOW wnd)
{
    if (GetTitle(wnd) == NULL)
    {
        if (GetClass(wnd) == DIALOG)
            return ((DBOX *)(wnd->extension))->HelpName;
        else 
            return "Untitled";
    }
    else
        return GetTitle(wnd);
}

/* ----------- Prepare the Window menu ------------ */
void PrepWindowMenu(void *w, struct Menu *mnu)
{
    DFWINDOW wnd = w;
    struct PopDown *p0 = mnu->Selections;
    struct PopDown *pd = mnu->Selections + 2;
    struct PopDown *ca = mnu->Selections + 13;
    int MenuNo = 0;
    DFWINDOW cwnd;

	mnu->Selection = 0;
	oldFocus = NULL;

	if (GetClass(wnd) != APPLICATION)
	{
		oldFocus = wnd;

		/* point to the APPLICATION window */
		if (ApplicationWindow == NULL)
			return;

		cwnd = FirstWindow(ApplicationWindow);
		/* get the first 9 document windows */
		while (cwnd != NULL && MenuNo < 9)
		{
			if (GetClass(cwnd) != MENUBAR &&
			    GetClass(cwnd) != STATUSBAR)
			{
				/* add the document window to the menu */
				strncpy (Menus[MenuNo]+4, WindowName(cwnd), 20);
				pd->SelectionTitle = Menus[MenuNo];
				if (cwnd == oldFocus)
				{
					/* mark the current document */
					pd->Attrib |= CHECKED;
					mnu->Selection = MenuNo+2;
				}
				else
					pd->Attrib &= ~CHECKED;
				pd++;
				MenuNo++;
			}
			cwnd = NextWindow(cwnd);
		}
	}

	if (MenuNo)
		p0->SelectionTitle = "~Close all";
	else
		p0->SelectionTitle = NULL;

	if (MenuNo >= 9)
	{
		*pd++ = *ca;
		if (mnu->Selection == 0)
			mnu->Selection = 11;
	}
	pd->SelectionTitle = NULL;
}

/* window processing module for the More Windows dialog box */
static int WindowPrep(DFWINDOW wnd,DFMESSAGE msg,PARAM p1,PARAM p2)
{
    switch (msg)    {
        case INITIATE_DIALOG:    {
            DFWINDOW wnd1;
            DFWINDOW cwnd = ControlWindow(&Windows,ID_WINDOWLIST);
            int sel = 0;
            if (cwnd == NULL)
                return FALSE;
			wnd1 = FirstWindow(ApplicationWindow);
			while (wnd1 != NULL)	{
                if (wnd1 != wnd && GetClass(wnd1) != MENUBAR &&
                        GetClass(wnd1) != STATUSBAR)    {
                    if (wnd1 == oldFocus)
                        WindowSel = sel;
                    DfSendMessage(cwnd, ADDTEXT,
                        (PARAM) WindowName(wnd1), 0);
                    sel++;
                }
				wnd1 = NextWindow(wnd1);
            }
            DfSendMessage(cwnd, LB_SETSELECTION, WindowSel, 0);
            AddAttribute(cwnd, VSCROLLBAR);
            DfPostMessage(cwnd, SHOW_WINDOW, 0, 0);
            break;
        }
        case DFM_COMMAND:
            switch ((int) p1)    {
                case ID_OK:
                    if ((int)p2 == 0)
                        WindowSel = DfSendMessage(
                                    ControlWindow(&Windows,
                                    ID_WINDOWLIST),
                                    LB_CURRENTSELECTION, 0, 0);
                    break;
                case ID_WINDOWLIST:
                    if ((int) p2 == LB_CHOOSE)
                        DfSendMessage(wnd, DFM_COMMAND, ID_OK, 0);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

/* ---- the More Windows command on the Window menu ---- */
static void MoreWindows(DFWINDOW wnd)
{
    if (DfDialogBox(wnd, &Windows, TRUE, WindowPrep))
        ChooseWindow(wnd, WindowSel);
}

/* ----- user chose a window from the Window menu
        or the More Window dialog box ----- */
static void ChooseWindow(DFWINDOW wnd, int WindowNo)
{
    DFWINDOW cwnd = FirstWindow(wnd);
	while (cwnd != NULL)
	{
        if (GetClass(cwnd) != MENUBAR &&
                GetClass(cwnd) != STATUSBAR)
            if (WindowNo-- == 0)
                break;
		cwnd = NextWindow(cwnd);
    }
    if (cwnd != NULL)    {
        DfSendMessage(cwnd, SETFOCUS, TRUE, 0);
        if (cwnd->condition == ISMINIMIZED)
            DfSendMessage(cwnd, RESTORE, 0, 0);
    }
}

/* ----- Close all document windows ----- */
static void CloseAll(DFWINDOW wnd, int closing)
{
	DFWINDOW wnd1, wnd2;

	DfSendMessage(wnd, SETFOCUS, TRUE, 0);
	wnd1 = LastWindow(wnd);
	while (wnd1 != NULL)
	{
		wnd2 = PrevWindow(wnd1);
		if (GetClass(wnd1) != MENUBAR && GetClass(wnd1) != STATUSBAR)
		{
			ClearVisible(wnd1);
			DfSendMessage(wnd1, CLOSE_WINDOW, 0, 0);
		}
		wnd1 = wnd2;
	}
	if (!closing)
		DfSendMessage(wnd, PAINT, 0, 0);
}

#endif    /* #ifdef INCLUDE_MULTI_WINDOWS */

static void DoWindowColors(DFWINDOW wnd)
{
	DFWINDOW cwnd;

	InitWindowColors(wnd);
	cwnd = FirstWindow(wnd);
	while (cwnd != NULL)
	{
		DoWindowColors(cwnd);
		if (GetClass(cwnd) == TEXT && GetText(cwnd) != NULL)
			DfSendMessage(cwnd, CLEARTEXT, 0, 0);
		cwnd = NextWindow(cwnd);
	}
}

/* set up colors for the application window */
static void SelectColors(DFWINDOW wnd)
{
	memcpy(cfg.clr, color, sizeof color);
	DoWindowColors(wnd);
}


#ifdef INCLUDE_WINDOWOPTIONS

/* ----- select the screen texture ----- */
static void SelectTexture(void)
{
    cfg.Texture = CheckBoxSetting(&Display, ID_TEXTURE);
}

/* -- select whether the application screen has a border -- */
static void SelectBorder(DFWINDOW wnd)
{
    cfg.Border = CheckBoxSetting(&Display, ID_BORDER);
    if (cfg.Border)
        AddAttribute(wnd, HASBORDER);
    else
        ClearAttribute(wnd, HASBORDER);
}

/* select whether the application screen has a status bar */
static void SelectStatusBar(DFWINDOW wnd)
{
    cfg.StatusBar = CheckBoxSetting(&Display, ID_STATUSBAR);
    if (cfg.StatusBar)
        AddAttribute(wnd, HASSTATUSBAR);
    else
        ClearAttribute(wnd, HASSTATUSBAR);
}

/* select whether the application screen has a title bar */
static void SelectTitle(DFWINDOW wnd)
{
    cfg.Title = CheckBoxSetting(&Display, ID_TITLE);
    if (cfg.Title)
        AddAttribute(wnd, HASTITLEBAR);
    else
        ClearAttribute(wnd, HASTITLEBAR);
}

#endif

/* EOF */
