/* ------------- applicat.c ------------- */

#include "dflat.h"

static int ScreenHeight;
static BOOL DisplayModified=FALSE;
WINDOW ApplicationWindow;

extern DBOX Display;

#ifdef INCLUDE_MULTI_WINDOWS
extern DBOX Windows;
#endif

#ifdef INCLUDE_LOGGING
extern DBOX Log;
#endif

#ifdef INCLUDE_SHELLDOS
static void ShellDOS(WINDOW);
#endif
static void CreateMenu(WINDOW);
static void CreateStatusBar(WINDOW);
static void SelectColors(WINDOW);
static void SetScreenHeight(int);
static void SelectLines(WINDOW);
static void SelectLoadBlank(void);

#ifdef INCLUDE_WINDOWOPTIONS
static void SelectTexture(void);
static void SelectBorder(WINDOW);
static void SelectTitle(WINDOW);
static void SelectStatusBar(WINDOW);
#endif

static WINDOW oldFocus;
#ifdef INCLUDE_MULTI_WINDOWS
static void CloseAll(WINDOW, int);
static void MoreWindows(WINDOW);
static void ChooseWindow(WINDOW, int);
static int WindowSel;
/* avoid allocation into a read-only region! */
static char Menu_1[] = "~1.                      ";
static char Menu_2[] = "~2.                      ";
static char Menu_3[] = "~3.                      ";
static char Menu_4[] = "~4.                      ";
static char Menu_5[] = "~5.                      ";
static char Menu_6[] = "~6.                      ";
static char Menu_7[] = "~7.                      ";
static char Menu_8[] = "~8.                      ";
static char Menu_9[] = "~9.                      ";
static char *Menus[9] = {
    Menu_1,
    Menu_2,
    Menu_3,
    Menu_4,
    Menu_5,
    Menu_6,
    Menu_7,
    Menu_8,
    Menu_9,
};
#endif

static char Cwd[65];

#ifdef ENABLEGLOBALARGV
char **Argv;
#endif

/* --------------- CREATE_WINDOW Message -------------- */
static int CreateWindowMsg(WINDOW wnd)
{
    int rtn;

    ApplicationWindow=wnd;
    ScreenHeight=SCREENHEIGHT;
    getcwd(Cwd, 64);
    if (!DisplayModified)
        {
       	int i;
       	CTLWINDOW *ct, *ct1;

        ct=FindCommand(&Display, ID_SNOWY, CHECKBOX);
    	if (!isVGA())
            {
            /* ---- Modify Display Dialog Box for EGA, CGA ---- */
            if (isEGA())
                ct1=FindCommand(&Display,ID_50LINES,RADIOBUTTON);
            else
                {
            	CTLWINDOW *ct2;

                ct2=FindCommand(&Display,ID_COLOR,RADIOBUTTON)-1;
                if (ct2)
                    {
                    ct2->dwnd.w++;
                    for (i=0;i<7;i++)
                        (ct2+i)->dwnd.x += 8;

                    }

                ct1=FindCommand(&Display,ID_25LINES,RADIOBUTTON)-1;
        	}

            if (ct && ct1)
                for (i=0;i<6;i++)
                    *ct1++ = *ct++;

            }

    	if (isVGA() || isEGA())
            {
            /* Eliminate the snowy check box */
            ct=FindCommand(&Display, ID_SNOWY, CHECKBOX);
            if (ct != NULL)
                for (i=0;i<4;i++)
                    *(ct+i) = *(ct+2+i);

            }

        DisplayModified=TRUE;
        }
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
    if (cfg.mono==1)
        PushRadioButton(&Display, ID_MONO);
    else if (cfg.mono==2)
        PushRadioButton(&Display, ID_REVERSE);
    else
        PushRadioButton(&Display, ID_COLOR);
    if (cfg.ScreenLines==25)
        PushRadioButton(&Display, ID_25LINES);
    else if (cfg.ScreenLines==43)
        PushRadioButton(&Display, ID_43LINES);
    else if (cfg.ScreenLines==50)
        PushRadioButton(&Display, ID_50LINES);
    if (cfg.snowy)
        SetCheckBox(&Display, ID_SNOWY);
    if (cfg.loadblank)
        SetCheckBox(&Display, ID_LOADBLANK);
    if (SCREENHEIGHT != cfg.ScreenLines)
        {
        SetScreenHeight(cfg.ScreenLines);
        if (WindowHeight(wnd)==ScreenHeight || SCREENHEIGHT-1 < GetBottom(wnd))
            {
            WindowHeight(wnd)=SCREENHEIGHT;
            GetBottom(wnd)=GetTop(wnd)+WindowHeight(wnd)-1;
            wnd->RestoredRC=WindowRect(wnd);
            }

        }

    SelectColors(wnd);
#ifdef INCLUDE_WINDOWOPTIONS
    SelectBorder(wnd);
    SelectTitle(wnd);
    SelectStatusBar(wnd);
#endif
    rtn=BaseWndProc(APPLICATION, wnd, CREATE_WINDOW, 0, 0);
    if (wnd->extension != NULL)
        CreateMenu(wnd);

    CreateStatusBar(wnd);
    SendMessage(NULL, SHOW_MOUSE, 0, 0);
    return rtn;

}

/* --------- ADDSTATUS Message ---------- */
static void AddStatusMsg(WINDOW wnd, PARAM p1)
{
    if (wnd->StatusBar != NULL)
        {
        if (p1 && *(char *)p1)
            SendMessage(wnd->StatusBar, SETTEXT, p1, 0);
        else 
            SendMessage(wnd->StatusBar, CLEARTEXT, 0, 0);

        SendMessage(wnd->StatusBar, PAINT, 0, 0);
        }

}

/* -------- SETFOCUS Message -------- */
static void SetFocusMsg(WINDOW wnd, BOOL p1)
{
    if (p1)
        SendMessage(inFocus, SETFOCUS, FALSE, 0);

    inFocus=p1 ? wnd : NULL;
    SendMessage(NULL, HIDE_CURSOR, 0, 0);
    if (isVisible(wnd))
        SendMessage(wnd, BORDER, 0, 0);
    else 
        SendMessage(wnd, SHOW_WINDOW, 0, 0);

}

/* ------- SIZE Message -------- */
static void SizeMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    BOOL WasVisible;

    WasVisible=isVisible(wnd);
    if (WasVisible)
        SendMessage(wnd, HIDE_WINDOW, 0, 0);

    if (p1-GetLeft(wnd) < 30)
        p1=GetLeft(wnd)+30;

    BaseWndProc(APPLICATION, wnd, SIZE, p1, p2);
    CreateMenu(wnd);
    CreateStatusBar(wnd);
    if (WasVisible)
        SendMessage(wnd, SHOW_WINDOW, 0, 0);

}

/* ----------- KEYBOARD Message ------------ */
static int KeyboardMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    if (WindowMoving || WindowSizing || (int) p1==F1)
        return BaseWndProc(APPLICATION, wnd, KEYBOARD, p1, p2);

    switch ((int) p1)
        {
        case ALT_F4:
            if (TestAttribute(wnd, CONTROLBOX))
                PostMessage(wnd, CLOSE_WINDOW, 0, 0);

            return TRUE;
#ifdef INCLUDE_MULTI_WINDOWS
        case ALT_F6:
            SetNextFocus();
            return TRUE;
#endif
        case ALT_HYPHEN:
            if (TestAttribute(wnd, CONTROLBOX))
                BuildSystemMenu(wnd);

            return TRUE;
        default:
            break;

        }

    PostMessage(wnd->MenuBarWnd, KEYBOARD, p1, p2);
    return TRUE;

}

/* --------- SHIFT_CHANGED Message -------- */
static void ShiftChangedMsg(WINDOW wnd, PARAM p1)
{
    extern BOOL AltDown;
    if ((int)p1 & ALTKEY)
        AltDown=TRUE;
    else if (AltDown)
        {
        AltDown=FALSE;
        if (wnd->MenuBarWnd != inFocus)
            SendMessage(NULL, HIDE_CURSOR, 0, 0);

        SendMessage(wnd->MenuBarWnd, KEYBOARD, F10, 0);
        }

}

/* -------- COMMAND Message ------- */
static void CommandMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    switch ((int)p1)
        {
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
            PostMessage(wnd, CLOSE_WINDOW, 0, 0);
            break;
        case ID_DISPLAY:
            if (DialogBox(wnd, &Display, TRUE, NULL))
                {
                if (inFocus==wnd->MenuBarWnd || inFocus==wnd->StatusBar)
                    oldFocus=ApplicationWindow;
                else 
                    oldFocus=inFocus;

                SendMessage(wnd, HIDE_WINDOW, 0, 0);
                SelectColors(wnd);
                SelectLines(wnd);
                SelectLoadBlank();
#ifdef INCLUDE_WINDOWOPTIONS
                SelectBorder(wnd);
                SelectTitle(wnd);
                SelectStatusBar(wnd);
                SelectTexture();
#endif
                CreateMenu(wnd);
                CreateStatusBar(wnd);
                SendMessage(wnd, SHOW_WINDOW, 0, 0);
                SendMessage(oldFocus, SETFOCUS, TRUE, 0);
                }
            break;
        case ID_SAVEOPTIONS:
            SaveConfig();
            break;
#ifdef INCLUDE_MULTI_WINDOWS
        case ID_WINDOW:
            ChooseWindow(wnd, CurrentMenuSelection-2);
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
            BaseWndProc(APPLICATION, wnd, COMMAND, p1, p2);
            break;
        default:
            if (inFocus != wnd->MenuBarWnd && inFocus != wnd)
                PostMessage(inFocus, COMMAND, p1, p2);

            break;

        }

}

/* --------- CLOSE_WINDOW Message -------- */
static int CloseWindowMsg(WINDOW wnd)
{
    int rtn;
#ifdef INCLUDE_MULTI_WINDOWS
    CloseAll(wnd, TRUE);
    WindowSel=0;
#endif
    PostMessage(NULL, STOP, 0, 0);
    rtn=BaseWndProc(APPLICATION, wnd, CLOSE_WINDOW, 0, 0);
    if (ScreenHeight != SCREENHEIGHT)
        SetScreenHeight(ScreenHeight);

    UnLoadHelpFile();
    DisplayModified=FALSE;
    ApplicationWindow=NULL;
    setdisk(toupper(*Cwd) - 'A');
    chdir(Cwd+2);
    return rtn;

}

/* --- APPLICATION Window Class window processing module --- */
int ApplicationProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case HIDE_WINDOW:
            if (wnd==inFocus)
                inFocus=NULL;

            break;
        case ADDSTATUS:
            AddStatusMsg(wnd, p1);
            return TRUE;
        case SETFOCUS:
            if ((int)p1==(inFocus != wnd))
                {
                SetFocusMsg(wnd, (BOOL) p1);
                return TRUE;
                }

            break;
        case SIZE:
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
            if (isVisible(wnd))
                {
                int cl=cfg.Texture ? APPLCHAR : ' ';

                ClearWindow(wnd, (RECT *)p1, cl);
                }

            return TRUE;
        case COMMAND:
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
    SendMessage(NULL, SAVE_CURSOR, 0, 0);
    SwapCursorStack();
    SendMessage(NULL, RESTORE_CURSOR, 0, 0);
}

/* ------- Shell out to DOS ---------- */
static void ShellDOS(WINDOW wnd)
{
    oldFocus=inFocus;
    SendMessage(wnd, HIDE_WINDOW, 0, 0);
    SwitchCursor();
    if (ScreenHeight != SCREENHEIGHT)
        SetScreenHeight(ScreenHeight);

    SendMessage(NULL, HIDE_MOUSE, 0, 0);
    //printf("Type EXIT to return to %s.\n\n", DFlatApplication);
    //fflush(stdout);
    //spawnl(P_WAIT, getenv("COMSPEC"), NULL);
    if (SCREENHEIGHT != cfg.ScreenLines)
        SetScreenHeight(cfg.ScreenLines);

    SwitchCursor();
    SendMessage(wnd, SHOW_WINDOW, 0, 0);
    SendMessage(oldFocus, SETFOCUS, TRUE, 0);
    SendMessage(NULL, SHOW_MOUSE, 0, 0);

}
#endif

/* -------- Create the menu bar -------- */
static void CreateMenu(WINDOW wnd)
{
    AddAttribute(wnd, HASMENUBAR);
    if (wnd->MenuBarWnd != NULL)
        SendMessage(wnd->MenuBarWnd, CLOSE_WINDOW, 0, 0);

    wnd->MenuBarWnd=CreateWindow(MENUBAR, NULL, GetClientLeft(wnd), GetClientTop(wnd)-1, 1, ClientWidth(wnd), NULL, wnd, NULL, 0);
    SendMessage(wnd->MenuBarWnd,BUILDMENU, (PARAM)wnd->extension,0);
    AddAttribute(wnd->MenuBarWnd, VISIBLE);

}

/* ----------- Create the status bar ------------- */
static void CreateStatusBar(WINDOW wnd)
{
    if (wnd->StatusBar != NULL)
        {
        SendMessage(wnd->StatusBar, CLOSE_WINDOW, 0, 0);
        wnd->StatusBar=NULL;
        }

    if (TestAttribute(wnd, HASSTATUSBAR))
        {
        wnd->StatusBar=CreateWindow(STATUSBAR, NULL, GetClientLeft(wnd), GetBottom(wnd), 1, ClientWidth(wnd), NULL, wnd, NULL, 0);
        AddAttribute(wnd->StatusBar, VISIBLE);
        }

}

#ifdef INCLUDE_MULTI_WINDOWS
/* -------- return the name of a document window ------- */
static char *WindowName(WINDOW wnd)
{
    if (GetTitle(wnd)==NULL)
        {
        if (GetClass(wnd)==DIALOG)
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
    WINDOW wnd=w,cwnd;
    struct PopDown *p0=mnu->Selections;
    struct PopDown *pd=mnu->Selections+2;
    struct PopDown *ca=mnu->Selections+13;
    int MenuNo=0;

    mnu->Selection=0;
    oldFocus=NULL;
    if (GetClass(wnd) != APPLICATION)
        {
        oldFocus=wnd;
        if (ApplicationWindow==NULL)  /* Point to the APPLICATION window */
            return;

        cwnd=FirstWindow(ApplicationWindow);

        /* ----- get the first 9 document windows ----- */
        while (cwnd != NULL && MenuNo < 9)
            {
            if (isVisible(cwnd) && GetClass(cwnd) != MENUBAR && GetClass(cwnd) != STATUSBAR)
                {
                /* Add the document window to the menu */
                strncpy(Menus[MenuNo]+4, WindowName(cwnd), 20);
                pd->SelectionTitle=Menus[MenuNo];
                if (cwnd==oldFocus)
                    {
                    /* -- mark the current document -- */
                    pd->Attrib |= CHECKED;
                    mnu->Selection=MenuNo+2;
                    }
                else
                    pd->Attrib &= ~CHECKED;

                pd++;
                MenuNo++;
                }

            cwnd=NextWindow(cwnd);
            }

        }

    if (MenuNo)
        p0->SelectionTitle="~Close all";
    else
        p0->SelectionTitle=NULL;

    if (MenuNo >= 9)
        {
        *pd++=*ca;
        if (mnu->Selection==0)
            mnu->Selection=11;

        }

    pd->SelectionTitle=NULL;

}

/* window processing module for the More Windows dialog box */
static int WindowPrep(WINDOW wnd,MESSAGE msg,PARAM p1,PARAM p2)
{
    switch (msg)
        {
        case INITIATE_DIALOG:
            {
            WINDOW cwnd=ControlWindow(&Windows,ID_WINDOWLIST),wnd1;
            int sel=0;

            if (cwnd==NULL)
                return FALSE;

            wnd1=FirstWindow(ApplicationWindow);
            while (wnd1 != NULL)
                {
                if (isVisible(wnd1) && wnd1 != wnd && GetClass(wnd1) != MENUBAR && GetClass(wnd1) != STATUSBAR)
                    {
                    if (wnd1==oldFocus)
                        WindowSel=sel;

                    SendMessage(cwnd, ADDTEXT, (PARAM) WindowName(wnd1), 0);
                    sel++;
                    }

                wnd1=NextWindow(wnd1);
                }

            SendMessage(cwnd, LB_SETSELECTION, WindowSel, 0);
            AddAttribute(cwnd, VSCROLLBAR);
            PostMessage(cwnd, SHOW_WINDOW, 0, 0);
            break;
            }
        case COMMAND:
            switch ((int) p1)
                {
                case ID_OK:
                    if ((int)p2==0)
                        WindowSel=SendMessage(ControlWindow(&Windows, ID_WINDOWLIST), LB_CURRENTSELECTION, 0, 0);

                    break;
                case ID_WINDOWLIST:
                    if ((int) p2==LB_CHOOSE)
                        SendMessage(wnd, COMMAND, ID_OK, 0);

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
static void MoreWindows(WINDOW wnd)
{
    if (DialogBox(wnd, &Windows, TRUE, WindowPrep))
        ChooseWindow(wnd, WindowSel);

}

/* ----- user chose a window from the Window menu or the More Window dialog box ----- */
static void ChooseWindow(WINDOW wnd, int WindowNo)
{
    WINDOW cwnd=FirstWindow(wnd);

    while (cwnd != NULL)
        {
        if (isVisible(cwnd) && GetClass(cwnd) != MENUBAR && GetClass(cwnd) != STATUSBAR)
            if (WindowNo-- == 0)
                break;

        cwnd=NextWindow(cwnd);
        }

    if (cwnd != NULL)
        {
        SendMessage(cwnd, SETFOCUS, TRUE, 0);
        if (cwnd->condition==ISMINIMIZED)
            SendMessage(cwnd, RESTORE, 0, 0);

        }

}

/* ----- Close all document windows ----- */
static void CloseAll(WINDOW wnd, int closing)
{
    WINDOW wnd1, wnd2;

    SendMessage(wnd, SETFOCUS, TRUE, 0);
    wnd1=LastWindow(wnd);
    while (wnd1 != NULL)
        {
        wnd2=PrevWindow(wnd1);
        if (isVisible(wnd1) && GetClass(wnd1) != MENUBAR && GetClass(wnd1) != STATUSBAR)
            {
            ClearVisible(wnd1);
    	    SendMessage(wnd1, CLOSE_WINDOW, 0, 0);
            }

        wnd1=wnd2;
        }

    if (!closing)
        SendMessage(wnd, PAINT, 0, 0);

}

#endif    /* #ifdef INCLUDE_MULTI_WINDOWS */

static void DoWindowColors(WINDOW wnd)
{
    WINDOW cwnd;

    InitWindowColors(wnd);
    cwnd=FirstWindow(wnd);
    while (cwnd != NULL)
        {
        DoWindowColors(cwnd);
        if (GetClass(cwnd)==TEXT && GetText(cwnd) != NULL)
            SendMessage(cwnd, CLEARTEXT, 0, 0);

        cwnd=NextWindow(cwnd);
        }

}

/* ----- set up colors for the application window ------ */
static void SelectColors(WINDOW wnd)
{
    if (RadioButtonSetting(&Display, ID_MONO))
        cfg.mono=1;
    else if (RadioButtonSetting(&Display, ID_REVERSE))
        cfg.mono=2;
    else
        cfg.mono=0;

    cfg.snowy=CheckBoxSetting(&Display, ID_SNOWY);
    get_videomode();
    if ((ismono() || video_mode==2) && cfg.mono==0)
        cfg.mono=1;

    if (cfg.mono==1)
        memcpy(cfg.clr, bw, sizeof bw);
    else if (cfg.mono==2)
        memcpy(cfg.clr, reverse, sizeof reverse);
    else
        memcpy(cfg.clr, color, sizeof color);

    DoWindowColors(wnd);

}

static void SelectLoadBlank(void)
{
    cfg.loadblank=CheckBoxSetting(&Display, ID_LOADBLANK);

}

/* ---- select screen lines ---- */
static void SelectLines(WINDOW wnd)
{
    cfg.ScreenLines=25;
    if (isEGA() || isVGA())
        {
        if (RadioButtonSetting(&Display, ID_43LINES))
            cfg.ScreenLines=43;
        else if (RadioButtonSetting(&Display, ID_50LINES))
            cfg.ScreenLines=50;

        }

    if (SCREENHEIGHT != cfg.ScreenLines)
        {
        SetScreenHeight(cfg.ScreenLines);
        /* ---- re-maximize ---- */
        if (wnd->condition==ISMAXIMIZED)
            {
            SendMessage(wnd, SIZE, (PARAM) GetRight(wnd), SCREENHEIGHT-1);
            return;
            }

        /* --- adjust if current size does not fit --- */
        if (WindowHeight(wnd) > SCREENHEIGHT)
            SendMessage(wnd, SIZE, (PARAM) GetRight(wnd), (PARAM) GetTop(wnd)+SCREENHEIGHT-1);

        /* --- if window is off-screen, move it on-screen --- */
        if (GetTop(wnd) >= SCREENHEIGHT-1)
            SendMessage(wnd, MOVE, (PARAM) GetLeft(wnd), (PARAM) SCREENHEIGHT-WindowHeight(wnd));

        }

}

/* ---- set the screen height in the video hardware ---- */
static void SetScreenHeight(int height)
{
    if (isEGA() || isVGA())
        {
        SendMessage(NULL, SAVE_CURSOR, 0, 0);
        switch (height)
            {
            case 25:
                Set25();
                break;
            case 43:
                Set43();
                break;
            case 50:
                Set50();
                break;
            default:
                break;

            }

        SendMessage(NULL, RESTORE_CURSOR, 0, 0);
        SendMessage(NULL, RESET_MOUSE, 0, 0);
        SendMessage(NULL, SHOW_MOUSE, 0, 0);
        }

}

#ifdef INCLUDE_WINDOWOPTIONS
/* ----- select the screen texture ----- */
static void SelectTexture(void)
{
    cfg.Texture=CheckBoxSetting(&Display, ID_TEXTURE);
}

/* -- select whether the application screen has a border -- */
static void SelectBorder(WINDOW wnd)
{
    cfg.Border=CheckBoxSetting(&Display, ID_BORDER);
    if (cfg.Border)
        AddAttribute(wnd, HASBORDER);
    else
        ClearAttribute(wnd, HASBORDER);
}

/* select whether the application screen has a status bar */
static void SelectStatusBar(WINDOW wnd)
{
    cfg.StatusBar=CheckBoxSetting(&Display, ID_STATUSBAR);
    if (cfg.StatusBar)
        AddAttribute(wnd, HASSTATUSBAR);
    else
        ClearAttribute(wnd, HASSTATUSBAR);
}

/* select whether the application screen has a title bar */
static void SelectTitle(WINDOW wnd)
{
    cfg.Title=CheckBoxSetting(&Display, ID_TITLE);
    if (cfg.Title)
        AddAttribute(wnd, HASTITLEBAR);
    else
        ClearAttribute(wnd, HASTITLEBAR);
}

#endif

