/* ------------- applicat.c ------------- */

#include "dflat.h"

static BOOL DisplayModified = FALSE;
DFWINDOW DfApplicationWindow;

extern DF_DBOX Display;
extern DF_DBOX Windows;

#ifdef INCLUDE_LOGGING
extern DF_DBOX Log;
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

/* --------------- DFM_CREATE_WINDOW Message -------------- */
static int CreateWindowMsg(DFWINDOW wnd)
{
	int rtn;

	DfApplicationWindow = wnd;
#ifdef INCLUDE_WINDOWOPTIONS
    if (DfCfg.Border)
        DfSetCheckBox(&Display, DF_ID_BORDER);
    if (DfCfg.Title)
        DfSetCheckBox(&Display, DF_ID_TITLE);
    if (DfCfg.StatusBar)
        DfSetCheckBox(&Display, DF_ID_STATUSBAR);
    if (DfCfg.Texture)
        DfSetCheckBox(&Display, DF_ID_TEXTURE);
#endif
    SelectColors(wnd);
#ifdef INCLUDE_WINDOWOPTIONS
    SelectBorder(wnd);
    SelectTitle(wnd);
    SelectStatusBar(wnd);
#endif
    rtn = DfBaseWndProc(DF_APPLICATION, wnd, DFM_CREATE_WINDOW, 0, 0);
    if (wnd->extension != NULL)
        DfCreateMenu(wnd);
    CreateStatusBar(wnd);
    return rtn;
}

/* --------- DFM_ADDSTATUS Message ---------- */
static void AddStatusMsg(DFWINDOW wnd, DF_PARAM p1)
{
    if (wnd->StatusBar != NULL)    {
        if (p1 && *(char *)p1)
            DfSendMessage(wnd->StatusBar, DFM_SETTEXT, p1, 0);
        else 
            DfSendMessage(wnd->StatusBar, DFM_CLEARTEXT, 0, 0);
        DfSendMessage(wnd->StatusBar, DFM_PAINT, 0, 0);
    }
}

/* -------- DFM_SETFOCUS Message -------- */
static void SetFocusMsg(DFWINDOW wnd, BOOL p1)
{
    if (p1)
        DfSendMessage(DfInFocus, DFM_SETFOCUS, FALSE, 0);
    DfInFocus = p1 ? wnd : NULL;
	if (DfIsVisible(wnd))
	    DfSendMessage(wnd, DFM_BORDER, 0, 0);
	else 
	    DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
}

/* ------- SIZE Message -------- */
static void SizeMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    BOOL WasVisible;
    WasVisible = DfIsVisible(wnd);
    if (WasVisible)
        DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    if (p1-DfGetLeft(wnd) < 30)
        p1 = DfGetLeft(wnd) + 30;
    DfBaseWndProc(DF_APPLICATION, wnd, DFM_DFM_SIZE, p1, p2);
    DfCreateMenu(wnd);
    CreateStatusBar(wnd);
    if (WasVisible)
        DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
}

/* ----------- DFM_KEYBOARD Message ------------ */
static int KeyboardMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    if (DfWindowMoving || DfWindowSizing || (int) p1 == DF_F1)
        return DfBaseWndProc(DF_APPLICATION, wnd, DFM_KEYBOARD, p1, p2);
    switch ((int) p1)    {
        case DF_ALT_F4:
            DfPostMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
            return TRUE;
#ifdef INCLUDE_MULTI_WINDOWS
        case DF_ALT_F6:
            DfSetNextFocus();
            return TRUE;
#endif
        case DF_ALT_HYPHEN:
            DfBuildSystemMenu(wnd);
            return TRUE;
        default:
            break;
    }
    DfPostMessage(wnd->MenuBarWnd, DFM_KEYBOARD, p1, p2);
    return TRUE;
}

/* --------- DFM_SHIFT_CHANGED Message -------- */
static void ShiftChangedMsg(DFWINDOW wnd, DF_PARAM p1)
{
	extern BOOL AltDown;
    if ((int)p1 & DF_ALTKEY)
        AltDown = TRUE;
    else if (AltDown)    {
        AltDown = FALSE;
        if (wnd->MenuBarWnd != DfInFocus)
            DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
        DfSendMessage(wnd->MenuBarWnd, DFM_KEYBOARD, DF_F10, 0);
    }
}

/* -------- COMMAND Message ------- */
static void CommandMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    switch ((int)p1)    {
        case DF_ID_HELP:
            DfDisplayHelp(wnd, DFlatApplication);
            break;
        case DF_ID_HELPHELP:
            DfDisplayHelp(wnd, "HelpHelp");
            break;
        case DF_ID_EXTHELP:
            DfDisplayHelp(wnd, "ExtHelp");
            break;
        case DF_ID_KEYSHELP:
            DfDisplayHelp(wnd, "KeysHelp");
            break;
        case DF_ID_HELPINDEX:
            DfDisplayHelp(wnd, "HelpIndex");
            break;
#ifdef TESTING_DFLAT
        case DF_ID_LOADHELP:
            DfLoadHelpFile();
            break;
#endif
#ifdef INCLUDE_LOGGING
        case DF_ID_LOG:
            DfMessageLog(wnd);
            break;
#endif
#ifdef INCLUDE_SHELLDOS
        case DF_ID_DOS:
            ShellDOS(wnd);
            break;
#endif
        case DF_ID_EXIT:
        case DF_ID_SYSCLOSE:
            DfPostMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
            break;
        case DF_ID_DISPLAY:
            if (DfDialogBox(wnd, &Display, TRUE, NULL))    {
				if (DfInFocus == wnd->MenuBarWnd || DfInFocus == wnd->StatusBar)
					oldFocus = DfApplicationWindow;
				else 
					oldFocus = DfInFocus;
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
                DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
			    DfSendMessage(oldFocus, DFM_SETFOCUS, TRUE, 0);
            }
            break;
        case DF_ID_SAVEOPTIONS:
            DfSaveConfig();
            break;
#ifdef INCLUDE_MULTI_WINDOWS
        case DF_ID_WINDOW:
            ChooseWindow(wnd, (int)p2-2);
            break;
        case DF_ID_CLOSEALL:
            CloseAll(wnd, FALSE);
            break;
        case DF_ID_MOREWINDOWS:
            MoreWindows(wnd);
            break;
#endif
#ifdef INCLUDE_RESTORE
        case DF_ID_SYSRESTORE:
#endif
        case DF_ID_SYSMOVE:
        case DF_ID_SYSSIZE:
#ifdef INCLUDE_MINIMIZE
        case DF_ID_SYSMINIMIZE:
#endif
#ifdef INCLUDE_MAXIMIZE
        case DF_ID_SYSMAXIMIZE:
#endif
            DfBaseWndProc(DF_APPLICATION, wnd, DFM_COMMAND, p1, p2);
            break;
        default:
            if (DfInFocus != wnd->MenuBarWnd && DfInFocus != wnd)
                DfPostMessage(DfInFocus, DFM_COMMAND, p1, p2);
            break;
    }
}

/* --------- DFM_CLOSE_WINDOW Message -------- */
static int CloseWindowMsg(DFWINDOW wnd)
{
    int rtn;
#ifdef INCLUDE_MULTI_WINDOWS
    CloseAll(wnd, TRUE);
	WindowSel = 0;
#endif
    DfPostMessage(NULL, DFM_STOP, 0, 0);
    rtn = DfBaseWndProc(DF_APPLICATION, wnd, DFM_CLOSE_WINDOW, 0, 0);
    DfUnLoadHelpFile();
	DisplayModified = FALSE;
	DfApplicationWindow = NULL;
    return rtn;
}

/* --- DF_APPLICATION Window Class window processing module --- */
int DfApplicationProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)
    {
        case DFM_CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case DFM_HIDE_WINDOW:
            if (wnd == DfInFocus)
                DfInFocus = NULL;
            break;
        case DFM_ADDSTATUS:
            AddStatusMsg(wnd, p1);
            return TRUE;
        case DFM_SETFOCUS:
            if ((int)p1 == (DfInFocus != wnd))    {
                SetFocusMsg(wnd, (BOOL) p1);
                return TRUE;
            }
            break;
        case DFM_DFM_SIZE:
            SizeMsg(wnd, p1, p2);
            return TRUE;
#ifdef INCLUDE_MINIMIZE
        case DFM_MINIMIZE:
            return TRUE;
#endif
        case DFM_KEYBOARD:
            return KeyboardMsg(wnd, p1, p2);
        case DFM_SHIFT_CHANGED:
            ShiftChangedMsg(wnd, p1);
            return TRUE;
        case DFM_PAINT:
            if (DfIsVisible(wnd))    {
#ifdef INCLUDE_WINDOWOPTIONS
                int cl = DfCfg.Texture ? DF_APPLCHAR : ' ';
#else
                int cl = DF_APPLCHAR;
#endif
                DfClearWindow(wnd, (DFRECT *)p1, cl);
            }
            return TRUE;
        case DFM_COMMAND:
            CommandMsg(wnd, p1, p2);
            return TRUE;
        case DFM_CLOSE_WINDOW:
            return CloseWindowMsg(wnd);
        default:
            break;
    }
    return DfBaseWndProc(DF_APPLICATION, wnd, msg, p1, p2);
}

#ifdef INCLUDE_SHELLDOS
static void SwitchCursor(void)
{
    DfSendMessage(NULL, DFM_SAVE_CURSOR, 0, 0);
    DfSwapCursorStack();
    DfSendMessage(NULL, DFM_RESTORE_CURSOR, 0, 0);
}

/* ------- Shell out to DOS ---------- */
static void ShellDOS(DFWINDOW wnd)
{
	oldFocus = DfInFocus;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    SwitchCursor();
    printf("To return to %s, execute the DOS exit command.",
                    DFlatApplication);
    fflush(stdout);
    _spawnl(P_WAIT, getenv("COMSPEC"), " ", NULL);
    SwitchCursor();
	DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
    DfSendMessage(oldFocus, DFM_SETFOCUS, TRUE, 0);
}
#endif

/* -------- Create the menu bar -------- */
static void DfCreateMenu(DFWINDOW wnd)
{
    DfAddAttribute(wnd, DF_HASMENUBAR);
    if (wnd->MenuBarWnd != NULL)
        DfSendMessage(wnd->MenuBarWnd, DFM_CLOSE_WINDOW, 0, 0);
    wnd->MenuBarWnd = DfDfCreateWindow(DF_MENUBAR,
                        NULL,
                        DfGetClientLeft(wnd),
                        DfGetClientTop(wnd)-1,
                        1,
                        DfClientWidth(wnd),
                        NULL,
                        wnd,
                        NULL,
                        0);
    DfSendMessage(wnd->MenuBarWnd,DFM_BUILDMENU,
        (DF_PARAM)wnd->extension,0);
    DfAddAttribute(wnd->MenuBarWnd, DF_VISIBLE);
}

/* ----------- Create the status bar ------------- */
static void CreateStatusBar(DFWINDOW wnd)
{
    if (wnd->StatusBar != NULL)    {
        DfSendMessage(wnd->StatusBar, DFM_CLOSE_WINDOW, 0, 0);
        wnd->StatusBar = NULL;
    }
    if (DfTestAttribute(wnd, DF_HASSTATUSBAR))    {
        wnd->StatusBar = DfDfCreateWindow(DF_STATUSBAR,
                            NULL,
                            DfGetClientLeft(wnd),
                            DfGetBottom(wnd),
                            1,
                            DfClientWidth(wnd),
                            NULL,
                            wnd,
                            NULL,
                            0);
        DfAddAttribute(wnd->StatusBar, DF_VISIBLE);
    }
}

#ifdef INCLUDE_MULTI_WINDOWS
/* -------- return the name of a document window ------- */
static char *WindowName(DFWINDOW wnd)
{
    if (DfGetTitle(wnd) == NULL)
    {
        if (DfGetClass(wnd) == DF_DIALOG)
            return ((DF_DBOX *)(wnd->extension))->HelpName;
        else 
            return "Untitled";
    }
    else
        return DfGetTitle(wnd);
}

/* ----------- Prepare the Window menu ------------ */
void DfPrepWindowMenu(void *w, struct DfMenu *mnu)
{
    DFWINDOW wnd = w;
    struct DfPopDown *p0 = mnu->Selections;
    struct DfPopDown *pd = mnu->Selections + 2;
    struct DfPopDown *ca = mnu->Selections + 13;
    int MenuNo = 0;
    DFWINDOW cwnd;

	mnu->Selection = 0;
	oldFocus = NULL;

	if (DfGetClass(wnd) != DF_APPLICATION)
	{
		oldFocus = wnd;

		/* point to the DF_APPLICATION window */
		if (DfApplicationWindow == NULL)
			return;

		cwnd = DfFirstWindow(DfApplicationWindow);
		/* get the first 9 document windows */
		while (cwnd != NULL && MenuNo < 9)
		{
			if (DfGetClass(cwnd) != DF_MENUBAR &&
			    DfGetClass(cwnd) != DF_STATUSBAR)
			{
				/* add the document window to the menu */
				strncpy (Menus[MenuNo]+4, WindowName(cwnd), 20);
				pd->SelectionTitle = Menus[MenuNo];
				if (cwnd == oldFocus)
				{
					/* mark the current document */
					pd->Attrib |= DF_CHECKED;
					mnu->Selection = MenuNo+2;
				}
				else
					pd->Attrib &= ~DF_CHECKED;
				pd++;
				MenuNo++;
			}
			cwnd = DfNextWindow(cwnd);
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
static int WindowPrep(DFWINDOW wnd,DFMESSAGE msg,DF_PARAM p1,DF_PARAM p2)
{
    switch (msg)    {
        case DFM_INITIATE_DIALOG:    {
            DFWINDOW wnd1;
            DFWINDOW cwnd = DfControlWindow(&Windows,DF_ID_WINDOWLIST);
            int sel = 0;
            if (cwnd == NULL)
                return FALSE;
			wnd1 = DfFirstWindow(DfApplicationWindow);
			while (wnd1 != NULL)	{
                if (wnd1 != wnd && DfGetClass(wnd1) != DF_MENUBAR &&
                        DfGetClass(wnd1) != DF_STATUSBAR)    {
                    if (wnd1 == oldFocus)
                        WindowSel = sel;
                    DfSendMessage(cwnd, DFM_ADDTEXT,
                        (DF_PARAM) WindowName(wnd1), 0);
                    sel++;
                }
				wnd1 = DfNextWindow(wnd1);
            }
            DfSendMessage(cwnd, DFM_LB_SETSELECTION, WindowSel, 0);
            DfAddAttribute(cwnd, DF_VSCROLLBAR);
            DfPostMessage(cwnd, DFM_SHOW_WINDOW, 0, 0);
            break;
        }
        case DFM_COMMAND:
            switch ((int) p1)    {
                case DF_ID_OK:
                    if ((int)p2 == 0)
                        WindowSel = DfSendMessage(
                                    DfControlWindow(&Windows,
                                    DF_ID_WINDOWLIST),
                                    DFM_LB_CURRENTSELECTION, 0, 0);
                    break;
                case DF_ID_WINDOWLIST:
                    if ((int) p2 == DFM_LB_CHOOSE)
                        DfSendMessage(wnd, DFM_COMMAND, DF_ID_OK, 0);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return DfDefaultWndProc(wnd, msg, p1, p2);
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
    DFWINDOW cwnd = DfFirstWindow(wnd);
	while (cwnd != NULL)
	{
        if (DfGetClass(cwnd) != DF_MENUBAR &&
                DfGetClass(cwnd) != DF_STATUSBAR)
            if (WindowNo-- == 0)
                break;
		cwnd = DfNextWindow(cwnd);
    }
    if (cwnd != NULL)    {
        DfSendMessage(cwnd, DFM_SETFOCUS, TRUE, 0);
        if (cwnd->condition == DF_ISMINIMIZED)
            DfSendMessage(cwnd, DFM_RESTORE, 0, 0);
    }
}

/* ----- Close all document windows ----- */
static void CloseAll(DFWINDOW wnd, int closing)
{
	DFWINDOW wnd1, wnd2;

	DfSendMessage(wnd, DFM_SETFOCUS, TRUE, 0);
	wnd1 = DfLastWindow(wnd);
	while (wnd1 != NULL)
	{
		wnd2 = DfPrevWindow(wnd1);
		if (DfGetClass(wnd1) != DF_MENUBAR && DfGetClass(wnd1) != DF_STATUSBAR)
		{
			DfClearVisible(wnd1);
			DfSendMessage(wnd1, DFM_CLOSE_WINDOW, 0, 0);
		}
		wnd1 = wnd2;
	}
	if (!closing)
		DfSendMessage(wnd, DFM_PAINT, 0, 0);
}

#endif    /* #ifdef INCLUDE_MULTI_WINDOWS */

static void DoWindowColors(DFWINDOW wnd)
{
	DFWINDOW cwnd;

	DfInitWindowColors(wnd);
	cwnd = DfFirstWindow(wnd);
	while (cwnd != NULL)
	{
		DoWindowColors(cwnd);
		if (DfGetClass(cwnd) == DF_TEXT && DfGetText(cwnd) != NULL)
			DfSendMessage(cwnd, DFM_CLEARTEXT, 0, 0);
		cwnd = DfNextWindow(cwnd);
	}
}

/* set up colors for the application window */
static void SelectColors(DFWINDOW wnd)
{
	memcpy(DfCfg.clr, DfColor, sizeof DfColor);
	DoWindowColors(wnd);
}


#ifdef INCLUDE_WINDOWOPTIONS

/* ----- select the screen texture ----- */
static void SelectTexture(void)
{
    DfCfg.Texture = DfCheckBoxSetting(&Display, DF_ID_TEXTURE);
}

/* -- select whether the application screen has a border -- */
static void SelectBorder(DFWINDOW wnd)
{
    DfCfg.Border = DfCheckBoxSetting(&Display, DF_ID_BORDER);
    if (DfCfg.Border)
        DfAddAttribute(wnd, DF_HASBORDER);
    else
        DfClearAttribute(wnd, DF_HASBORDER);
}

/* select whether the application screen has a status bar */
static void SelectStatusBar(DFWINDOW wnd)
{
    DfCfg.StatusBar = DfCheckBoxSetting(&Display, DF_ID_STATUSBAR);
    if (DfCfg.StatusBar)
        DfAddAttribute(wnd, DF_HASSTATUSBAR);
    else
        DfClearAttribute(wnd, DF_HASSTATUSBAR);
}

/* select whether the application screen has a title bar */
static void SelectTitle(DFWINDOW wnd)
{
    DfCfg.Title = DfCheckBoxSetting(&Display, DF_ID_TITLE);
    if (DfCfg.Title)
        DfAddAttribute(wnd, DF_HASTITLEBAR);
    else
        DfClearAttribute(wnd, DF_HASTITLEBAR);
}

#endif

/* EOF */
