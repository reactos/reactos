/* ------------- normal.c ------------ */

#include "dflat32/dflat.h"

#ifdef INCLUDE_MULTI_WINDOWS
static void PaintOverLappers(DFWINDOW wnd);
static void PaintUnderLappers(DFWINDOW wnd);
#endif

static BOOL InsideWindow(DFWINDOW, int, int);
static void TerminateMoveSize(void);
static void SaveBorder(DFRECT);
static void RestoreBorder(DFRECT);
static void GetVideoBuffer(DFWINDOW);
static void PutVideoBuffer(DFWINDOW);
#ifdef INCLUDE_MINIMIZE
static DFRECT PositionIcon(DFWINDOW);
#endif
static void dragborder(DFWINDOW, int, int);
static void sizeborder(DFWINDOW, int, int);
static int px = -1, py = -1;
static int diff;
static struct window dwnd = {DUMMY, NULL, NormalProc,
                                {-1,-1,-1,-1}};
static PCHAR_INFO Bsave;
static int Bht, Bwd;
BOOL WindowMoving;
BOOL WindowSizing;
/* -------- array of class definitions -------- */
CLASSDEFS classdefs[] = {
    #undef ClassDef
    #define ClassDef(c,b,p,a) {b,p,a},
    #include "dflat32/classes.h"
};
DFWINDOW HiddenWindow;

/* --------- CREATE_WINDOW Message ---------- */
static void CreateWindowMsg(DFWINDOW wnd)
{
	AppendWindow(wnd);
//	ClearAttribute(wnd, VSCROLLBAR | HSCROLLBAR);
	if (TestAttribute(wnd, SAVESELF) && isVisible(wnd))
		GetVideoBuffer(wnd);
}

/* --------- SHOW_WINDOW Message ---------- */
static void ShowWindowMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
	if (GetParent(wnd) == NULL || isVisible(GetParent(wnd)))
	{
		DFWINDOW cwnd;

		if (TestAttribute(wnd, SAVESELF) && wnd->videosave == NULL)
			GetVideoBuffer(wnd);
		SetVisible(wnd);
		DfSendMessage(wnd, PAINT, 0, TRUE);
		DfSendMessage(wnd, BORDER, 0, 0);
		/* --- show the children of this window --- */
		cwnd = FirstWindow(wnd);
		while (cwnd != NULL)
		{
			if (cwnd->condition != ISCLOSING)
				DfSendMessage(cwnd, SHOW_WINDOW, p1, p2);
			cwnd = NextWindow(cwnd);
		}
	}
}

/* --------- HIDE_WINDOW Message ---------- */
static void HideWindowMsg(DFWINDOW wnd)
{
	if (isVisible(wnd))
	{
		ClearVisible(wnd);
		/* --- paint what this window covered --- */
		if (TestAttribute(wnd, SAVESELF))
			PutVideoBuffer(wnd);
#ifdef INCLUDE_MULTI_WINDOWS
		else
			PaintOverLappers(wnd);
#endif
    }
}

/* --------- KEYBOARD Message ---------- */
static BOOL KeyboardMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    if (WindowMoving || WindowSizing)    {
        /* -- move or size a window with keyboard -- */
        int x, y;
        x=WindowMoving?GetLeft(&dwnd):GetRight(&dwnd);
        y=WindowMoving?GetTop(&dwnd):GetBottom(&dwnd);
        switch ((int)p1)    {
            case ESC:
                TerminateMoveSize();
                return TRUE;
            case UP:
                if (y)
                    --y;
                break;
            case DN:
                if (y < DfGetScreenHeight()-1)
                    y++;
                break;
            case FWD:
                if (x < DfGetScreenWidth()-1)
                    x++;
                break;
            case BS:
                if (x)
                    --x;
                break;
            case '\r':
                DfSendMessage(wnd, DFM_BUTTON_RELEASED,x,y);
            default:
                return TRUE;
        }
        /* -- use the mouse functions to move/size - */
        DfSendMessage(wnd, MOUSE_MOVED, x, y);
        return TRUE;
    }

	switch ((int)p1)
	{
		case F1:
			DfSendMessage(wnd, DFM_COMMAND, ID_HELP, 0);
			return TRUE;

		case ' ':
			if ((int)p2 & ALTKEY)
				if (TestAttribute(wnd, HASTITLEBAR))
					if (TestAttribute(wnd, CONTROLBOX))
						BuildSystemMenu(wnd);
			return TRUE;

		case CTRL_F4:
			if (TestAttribute(wnd, CONTROLBOX))
			{
				DfSendMessage(wnd, CLOSE_WINDOW, 0, 0);
				SkipApplicationControls();
				return TRUE;
			}
			break;

		default:
			break;
	}

	return FALSE;
}

/* --------- COMMAND Message ---------- */
static void CommandMsg(DFWINDOW wnd, PARAM p1)
{
    switch ((int)p1)    {
        case ID_HELP:
            DisplayHelp(wnd,ClassNames[GetClass(wnd)]);
            break;
#ifdef INCLUDE_RESTORE
        case ID_SYSRESTORE:
            DfSendMessage(wnd, RESTORE, 0, 0);
            break;
#endif
        case ID_SYSMOVE:
            DfSendMessage(wnd, CAPTURE_MOUSE, TRUE,
                (PARAM) &dwnd);
            DfSendMessage(wnd, CAPTURE_KEYBOARD, TRUE,
                (PARAM) &dwnd);
            WindowMoving = TRUE;
            dragborder(wnd, GetLeft(wnd), GetTop(wnd));
            break;
        case ID_SYSSIZE:
            DfSendMessage(wnd, CAPTURE_MOUSE, TRUE,
                (PARAM) &dwnd);
            DfSendMessage(wnd, CAPTURE_KEYBOARD, TRUE,
                (PARAM) &dwnd);
            WindowSizing = TRUE;
            dragborder(wnd, GetLeft(wnd), GetTop(wnd));
            break;
#ifdef INCLUDE_MINIMIZE
        case ID_SYSMINIMIZE:
            DfSendMessage(wnd, MINIMIZE, 0, 0);
            break;
#endif
#ifdef INCLUDE_MAXIMIZE
        case ID_SYSMAXIMIZE:
            DfSendMessage(wnd, MAXIMIZE, 0, 0);
            break;
#endif
        case ID_SYSCLOSE:
            DfSendMessage(wnd, CLOSE_WINDOW, 0, 0);
			SkipApplicationControls();
            break;
        default:
            break;
    }
}

/* --------- SETFOCUS Message ---------- */
static void SetFocusMsg(DFWINDOW wnd, PARAM p1)
{
	DFRECT rc = {0,0,0,0};

	if (p1 && wnd != NULL && inFocus != wnd)
	{
		DFWINDOW this, thispar;
		DFWINDOW that = NULL, thatpar = NULL;

		DFWINDOW cwnd = wnd, fwnd = GetParent(wnd);
		/* ---- post focus in ancestors ---- */
		while (fwnd != NULL)
		{
			fwnd->childfocus = cwnd;
			cwnd = fwnd;
			fwnd = GetParent(fwnd);
		}
		/* ---- de-post focus in self and children ---- */
		fwnd = wnd;
		while (fwnd != NULL)
		{
			cwnd = fwnd->childfocus;
			fwnd->childfocus = NULL;
			fwnd = cwnd;
		}

		this = wnd;
		that = thatpar = inFocus;

		/* ---- find common ancestor of prev focus and this window --- */
		while (thatpar != NULL)
		{
			thispar = wnd;
			while (thispar != NULL)
			{
				if (this == CaptureMouse || this == CaptureKeyboard)
				{
					/* ---- don't repaint if this window has capture ---- */
					that = thatpar = NULL;
					break;
				}
				if (thispar == thatpar)
				{
					/* ---- don't repaint if SAVESELF window had focus ---- */
					if (this != that && TestAttribute(that, SAVESELF))
						that = thatpar = NULL;
					break;
				}
				this = thispar;
				thispar = GetParent(thispar);
			}
			if (thispar != NULL)
				break;
			that = thatpar;
			thatpar = GetParent(thatpar);
		}
		if (inFocus != NULL)
			DfSendMessage(inFocus, SETFOCUS, FALSE, 0);
		inFocus = wnd;
		if (that != NULL && isVisible(wnd))
		{
			rc = subRectangle(WindowRect(that), WindowRect(this));
			if (!ValidRect(rc))
			{
				if (ApplicationWindow != NULL)
				{
					DFWINDOW fwnd = FirstWindow(ApplicationWindow);
					while (fwnd != NULL)
					{
						if (!isAncestor(wnd, fwnd))
						{
							rc = subRectangle(WindowRect(wnd),WindowRect(fwnd));
							if (ValidRect(rc))
								break;
						}
						fwnd = NextWindow(fwnd);
					}
				}
			}
		}
		if (that != NULL && !ValidRect(rc) && isVisible(wnd))
		{
			DfSendMessage(wnd, BORDER, 0, 0);
			this = NULL;
		}
		ReFocus(wnd);
		if (this != NULL)
		DfSendMessage(this, SHOW_WINDOW, 0, 0);
	}
	else if (!p1 && inFocus == wnd)
	{
		/* clearing focus */
		inFocus = NULL;
		DfSendMessage(wnd, BORDER, 0, 0);
	}
}

/* --------- DOUBLE_CLICK Message ---------- */
static void DoubleClickMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int mx = (int) p1 - GetLeft(wnd);
    int my = (int) p2 - GetTop(wnd);
    if (!WindowSizing && !WindowMoving)	{
        if (HitControlBox(wnd, mx, my))	{
            DfPostMessage(wnd, CLOSE_WINDOW, 0, 0);
			SkipApplicationControls();
		}
	}
}

/* --------- LEFT_BUTTON Message ---------- */
static void LeftButtonMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int mx = (int) p1 - GetLeft(wnd);
    int my = (int) p2 - GetTop(wnd);
    if (WindowSizing || WindowMoving)
        return;
    if (HitControlBox(wnd, mx, my))    {
        BuildSystemMenu(wnd);
        return;
    }
    if (my == 0 && mx > -1 && mx < WindowWidth(wnd))  {
        /* ---------- hit the top border -------- */
        if (TestAttribute(wnd, MINMAXBOX) &&
                TestAttribute(wnd, HASTITLEBAR))  {
            if (mx == WindowWidth(wnd)-2)    {
                if (wnd->condition != ISRESTORED)
                    /* --- hit the restore box --- */
                    DfSendMessage(wnd, RESTORE, 0, 0);
#ifdef INCLUDE_MAXIMIZE
                else
                    /* --- hit the maximize box --- */
                    DfSendMessage(wnd, MAXIMIZE, 0, 0);
#endif
                return;
            }
#ifdef INCLUDE_MINIMIZE
            if (mx == WindowWidth(wnd)-3)    {
                /* --- hit the minimize box --- */
                if (wnd->condition != ISMINIMIZED)
                    DfSendMessage(wnd, MINIMIZE, 0, 0);
                return;
            }
#endif
        }
#ifdef INCLUDE_MAXIMIZE
        if (wnd->condition == ISMAXIMIZED)
            return;
#endif
        if (TestAttribute(wnd, MOVEABLE))    {
            WindowMoving = TRUE;
            px = mx;
            py = my;
            diff = (int) mx;
            DfSendMessage(wnd, CAPTURE_MOUSE, TRUE,
                (PARAM) &dwnd);
            dragborder(wnd, GetLeft(wnd), GetTop(wnd));
        }
        return;
    }
    if (mx == WindowWidth(wnd)-1 &&
            my == WindowHeight(wnd)-1)    {
        /* ------- hit the resize corner ------- */
#ifdef INCLUDE_MINIMIZE
        if (wnd->condition == ISMINIMIZED)
            return;
#endif
        if (!TestAttribute(wnd, SIZEABLE))
            return;
#ifdef INCLUDE_MAXIMIZE
        if (wnd->condition == ISMAXIMIZED)    {
            if (GetParent(wnd) == NULL)
                return;
            if (TestAttribute(GetParent(wnd),HASBORDER))
                return;
            /* ----- resizing a maximized window over a
                    borderless parent ----- */
            wnd = GetParent(wnd);
        }
#endif
        WindowSizing = TRUE;
        DfSendMessage(wnd, CAPTURE_MOUSE,
            TRUE, (PARAM) &dwnd);
        dragborder(wnd, GetLeft(wnd), GetTop(wnd));
    }
}

/* --------- MOUSE_MOVED Message ---------- */
static BOOL MouseMovedMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    if (WindowMoving)    {
        int leftmost = 0, topmost = 0,
            bottommost = DfGetScreenHeight()-2,
            rightmost = DfGetScreenWidth()-2;
        int x = (int) p1 - diff;
        int y = (int) p2;
        if (GetParent(wnd) != NULL &&
                !TestAttribute(wnd, NOCLIP))    {
            DFWINDOW wnd1 = GetParent(wnd);
            topmost    = GetClientTop(wnd1);
            leftmost   = GetClientLeft(wnd1);
            bottommost = GetClientBottom(wnd1);
            rightmost  = GetClientRight(wnd1);
        }
        if (x < leftmost || x > rightmost ||
                y < topmost || y > bottommost)    {
            x = max(x, leftmost);
            x = min(x, rightmost);
            y = max(y, topmost);
            y = min(y, bottommost);
        }

        if (x != px || y != py)    {
            px = x;
            py = y;
            dragborder(wnd, x, y);
        }
        return TRUE;
    }
    if (WindowSizing)    {
        sizeborder(wnd, (int) p1, (int) p2);
        return TRUE;
    }
    return FALSE;
}

#ifdef INCLUDE_MAXIMIZE
/* --------- MAXIMIZE Message ---------- */
static void MaximizeMsg(DFWINDOW wnd)
{
    DFRECT rc = {0, 0, 0, 0};
    DFRECT holdrc;
    holdrc = wnd->RestoredRC;
    rc.rt = DfGetScreenWidth()-1;
    rc.bt = DfGetScreenHeight()-1;
    if (GetParent(wnd))
        rc = ClientRect(GetParent(wnd));
    wnd->oldcondition = wnd->condition;
    wnd->condition = ISMAXIMIZED;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    DfSendMessage(wnd, MOVE,
        RectLeft(rc), RectTop(rc));
    DfSendMessage(wnd, DFM_SIZE,
        RectRight(rc), RectBottom(rc));
    if (wnd->restored_attrib == 0)
        wnd->restored_attrib = wnd->attrib;
    ClearAttribute(wnd, SHADOW);
    DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
    wnd->RestoredRC = holdrc;
}
#endif

#ifdef INCLUDE_MINIMIZE
/* --------- MINIMIZE Message ---------- */
static void MinimizeMsg(DFWINDOW wnd)
{
    DFRECT rc;
    DFRECT holdrc;

    holdrc = wnd->RestoredRC;
    rc = PositionIcon(wnd);
    wnd->oldcondition = wnd->condition;
    wnd->condition = ISMINIMIZED;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    DfSendMessage(wnd, MOVE,
        RectLeft(rc), RectTop(rc));
    DfSendMessage(wnd, DFM_SIZE,
        RectRight(rc), RectBottom(rc));
	if (wnd == inFocus)
	    SetNextFocus();
    if (wnd->restored_attrib == 0)
        wnd->restored_attrib = wnd->attrib;
    ClearAttribute(wnd,
        SHADOW | SIZEABLE | HASMENUBAR |
        VSCROLLBAR | HSCROLLBAR);
    DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
    wnd->RestoredRC = holdrc;
}
#endif

#ifdef INCLUDE_RESTORE
/* --------- RESTORE Message ---------- */
static void RestoreMsg(DFWINDOW wnd)
{
    DFRECT holdrc;
    holdrc = wnd->RestoredRC;
    wnd->oldcondition = wnd->condition;
    wnd->condition = ISRESTORED;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    wnd->attrib = wnd->restored_attrib;
    wnd->restored_attrib = 0;
    DfSendMessage(wnd, MOVE, wnd->RestoredRC.lf,
        wnd->RestoredRC.tp);
    wnd->RestoredRC = holdrc;
    DfSendMessage(wnd, DFM_SIZE, wnd->RestoredRC.rt,
        wnd->RestoredRC.bt);
	if (wnd != inFocus)
	    DfSendMessage(wnd, SETFOCUS, TRUE, 0);
	else
	    DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
}
#endif

/* --------- MOVE Message ---------- */
static void MoveMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    DFWINDOW cwnd;
    BOOL wasVisible = isVisible(wnd);
    int xdif = (int) p1 - wnd->rc.lf;
    int ydif = (int) p2 - wnd->rc.tp;

    if (xdif == 0 && ydif == 0)
        return;
    if (wasVisible)
        DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    wnd->rc.lf = (int) p1;
    wnd->rc.tp = (int) p2;
    wnd->rc.rt = GetLeft(wnd)+WindowWidth(wnd)-1;
    wnd->rc.bt = GetTop(wnd)+WindowHeight(wnd)-1;
    if (wnd->condition == ISRESTORED)
        wnd->RestoredRC = wnd->rc;

	cwnd = FirstWindow(wnd);
	while (cwnd != NULL)	{
        DfSendMessage(cwnd, MOVE, cwnd->rc.lf+xdif, cwnd->rc.tp+ydif);
		cwnd = NextWindow(cwnd);
    }
    if (wasVisible)
        DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
}

/* --------- SIZE Message ---------- */
static void SizeMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    BOOL wasVisible = isVisible(wnd);
    DFWINDOW cwnd;
    DFRECT rc;
    int xdif = (int) p1 - wnd->rc.rt;
    int ydif = (int) p2 - wnd->rc.bt;

    if (xdif == 0 && ydif == 0)
        return;
    if (wasVisible)
        DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    wnd->rc.rt = (int) p1;
    wnd->rc.bt = (int) p2;
    wnd->ht = GetBottom(wnd)-GetTop(wnd)+1;
    wnd->wd = GetRight(wnd)-GetLeft(wnd)+1;

    if (wnd->condition == ISRESTORED)
        wnd->RestoredRC = WindowRect(wnd);

#ifdef INCLUDE_MAXIMIZE
    rc = ClientRect(wnd);

	cwnd = FirstWindow(wnd);
	while (cwnd != NULL)	{
        if (cwnd->condition == ISMAXIMIZED)
            DfSendMessage(cwnd, DFM_SIZE, RectRight(rc), RectBottom(rc));
		cwnd = NextWindow(cwnd);
    }

#endif
    if (wasVisible)
        DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
}

/* --------- CLOSE_WINDOW Message ---------- */
static void CloseWindowMsg(DFWINDOW wnd)
{
    DFWINDOW cwnd;
    wnd->condition = ISCLOSING;
    if (wnd->PrevMouse != NULL)
        DfSendMessage(wnd, RELEASE_MOUSE, 0, 0);
    if (wnd->PrevKeyboard != NULL)
        DfSendMessage(wnd, RELEASE_KEYBOARD, 0, 0);
    /* ----------- hide this window ------------ */
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    /* --- close the children of this window --- */

	cwnd = LastWindow(wnd);
	while (cwnd != NULL)	{
        if (inFocus == cwnd)
            inFocus = wnd;
        DfSendMessage(cwnd,CLOSE_WINDOW,0,0);
		cwnd = LastWindow(wnd);
    }

    /* --- change focus if this window had it -- */
	if (wnd == inFocus)
	    SetPrevFocus();
    /* -- free memory allocated to this window - */
    if (wnd->title != NULL)
        free(wnd->title);
    if (wnd->videosave != NULL)
        free(wnd->videosave);
    /* -- remove window from parent's list of children -- */
	RemoveWindow(wnd);
    if (wnd == inFocus)
        inFocus = NULL;
    free(wnd);
}

/* ---- Window-processing module for NORMAL window class ---- */
int NormalProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)    {
        case CREATE_WINDOW:
            CreateWindowMsg(wnd);
            break;
        case SHOW_WINDOW:
            ShowWindowMsg(wnd, p1, p2);
            break;
        case DFM_HIDE_WINDOW:
            HideWindowMsg(wnd);
            break;
        case DISPLAY_HELP:
            DisplayHelp(wnd, (char *)p1);
            break;
        case INSIDE_WINDOW:
            return InsideWindow(wnd, (int) p1, (int) p2);
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            /* ------- fall through ------- */
        case ADDSTATUS:
        case SHIFT_CHANGED:
            if (GetParent(wnd) != NULL)
                DfPostMessage(GetParent(wnd), msg, p1, p2);
            break;
        case PAINT:
            if (isVisible(wnd))
                ClearWindow(wnd, (DFRECT *)p1, ' ');
            break;
        case BORDER:
            if (isVisible(wnd))
            {
                if (TestAttribute(wnd, HASBORDER))
                    RepaintBorder(wnd, (DFRECT *)p1);
                else if (TestAttribute(wnd, HASTITLEBAR))
                    DisplayTitle(wnd, (DFRECT *)p1);
            }
            break;
        case DFM_COMMAND:
            CommandMsg(wnd, p1);
            break;
        case SETFOCUS:
            SetFocusMsg(wnd, p1);
            break;
        case DFM_DOUBLE_CLICK:
            DoubleClickMsg(wnd, p1, p2);
            break;
        case LEFT_BUTTON:
            LeftButtonMsg(wnd, p1, p2);
            break;
        case MOUSE_MOVED:
            if (MouseMovedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_BUTTON_RELEASED:
            if (WindowMoving || WindowSizing)
            {
                if (WindowMoving)
                    DfPostMessage(wnd,MOVE,dwnd.rc.lf,dwnd.rc.tp);
                else
                    DfPostMessage(wnd, DFM_SIZE,dwnd.rc.rt,dwnd.rc.bt);
                TerminateMoveSize();
            }
            break;
#ifdef INCLUDE_MAXIMIZE
        case MAXIMIZE:
            if (wnd->condition != ISMAXIMIZED)
                MaximizeMsg(wnd);
            break;
#endif
#ifdef INCLUDE_MINIMIZE
        case MINIMIZE:
            if (wnd->condition != ISMINIMIZED)
                MinimizeMsg(wnd);
            break;
#endif
#ifdef INCLUDE_RESTORE
        case RESTORE:
            if (wnd->condition != ISRESTORED)    {
#ifdef INCLUDE_MAXIMIZE
                if (wnd->oldcondition == ISMAXIMIZED)
                    DfSendMessage(wnd, MAXIMIZE, 0, 0);
                else
#endif
                    RestoreMsg(wnd);
            }
            break;
#endif
        case MOVE:
            MoveMsg(wnd, p1, p2);
            break;
        case DFM_SIZE:    {
            SizeMsg(wnd, p1, p2);
            break;
        }
        case CLOSE_WINDOW:
            CloseWindowMsg(wnd);
            break;
        default:
            break;
    }
    return TRUE;
}
#ifdef INCLUDE_MINIMIZE
/* ---- compute lower right icon space in a rectangle ---- */
static DFRECT LowerRight(DFRECT prc)
{
    DFRECT rc;
    RectLeft(rc) = RectRight(prc) - ICONWIDTH;
    RectTop(rc) = RectBottom(prc) - ICONHEIGHT;
    RectRight(rc) = RectLeft(rc)+ICONWIDTH-1;
    RectBottom(rc) = RectTop(rc)+ICONHEIGHT-1;
    return rc;
}
/* ----- compute a position for a minimized window icon ---- */
static DFRECT PositionIcon(DFWINDOW wnd)
{
	DFWINDOW pwnd = GetParent(wnd);
    DFRECT rc;
    RectLeft(rc) = DfGetScreenWidth()-ICONWIDTH;
    RectTop(rc) = DfGetScreenHeight()-ICONHEIGHT;
    RectRight(rc) = DfGetScreenWidth()-1;
    RectBottom(rc) = DfGetScreenHeight()-1;
    if (pwnd != NULL)    {
        DFRECT prc = WindowRect(pwnd);
		DFWINDOW cwnd = FirstWindow(pwnd);
        rc = LowerRight(prc);
        /* - search for icon available location - */
		while (cwnd != NULL)	{
            if (cwnd->condition == ISMINIMIZED)    {
                DFRECT rc1;
                rc1 = WindowRect(cwnd);
                if (RectLeft(rc1) == RectLeft(rc) &&
                        RectTop(rc1) == RectTop(rc))    {
                    RectLeft(rc) -= ICONWIDTH;
                    RectRight(rc) -= ICONWIDTH;
                    if (RectLeft(rc) < RectLeft(prc)+1)   {
                        RectLeft(rc) =
                            RectRight(prc)-ICONWIDTH;
                        RectRight(rc) =
                            RectLeft(rc)+ICONWIDTH-1;
                        RectTop(rc) -= ICONHEIGHT;
                        RectBottom(rc) -= ICONHEIGHT;
                        if (RectTop(rc) < RectTop(prc)+1)
                            return LowerRight(prc);
                    }
                    break;
                }
            }
			cwnd = NextWindow(cwnd);
        }
    }
    return rc;
}
#endif
/* ----- terminate the move or size operation ----- */
static void TerminateMoveSize(void)
{
    px = py = -1;
    diff = 0;
    DfSendMessage(&dwnd, RELEASE_MOUSE, TRUE, 0);
    DfSendMessage(&dwnd, RELEASE_KEYBOARD, TRUE, 0);
    RestoreBorder(dwnd.rc);
    WindowMoving = WindowSizing = FALSE;
}
/* ---- build a dummy window border for moving or sizing --- */
static void dragborder(DFWINDOW wnd, int x, int y)
{
    RestoreBorder(dwnd.rc);
    /* ------- build the dummy window -------- */
    dwnd.rc.lf = x;
    dwnd.rc.tp = y;
    dwnd.rc.rt = dwnd.rc.lf+WindowWidth(wnd)-1;
    dwnd.rc.bt = dwnd.rc.tp+WindowHeight(wnd)-1;
    dwnd.ht = WindowHeight(wnd);
    dwnd.wd = WindowWidth(wnd);
    dwnd.parent = GetParent(wnd);
    dwnd.attrib = VISIBLE | HASBORDER | NOCLIP;
    InitWindowColors(&dwnd);
    SaveBorder(dwnd.rc);
    RepaintBorder(&dwnd, NULL);
}
/* ---- write the dummy window border for sizing ---- */
static void sizeborder(DFWINDOW wnd, int rt, int bt)
{
    int leftmost = GetLeft(wnd)+10;
    int topmost = GetTop(wnd)+3;
    int bottommost = DfGetScreenHeight()-1;
    int rightmost  = DfGetScreenWidth()-1;
    if (GetParent(wnd))    {
        bottommost = min(bottommost,
            GetClientBottom(GetParent(wnd)));
        rightmost  = min(rightmost,
            GetClientRight(GetParent(wnd)));
    }
    rt = min(rt, rightmost);
    bt = min(bt, bottommost);
    rt = max(rt, leftmost);
    bt = max(bt, topmost);

    if (rt != px || bt != py)
        RestoreBorder(dwnd.rc);

    /* ------- change the dummy window -------- */
    dwnd.ht = bt-dwnd.rc.tp+1;
    dwnd.wd = rt-dwnd.rc.lf+1;
    dwnd.rc.rt = rt;
    dwnd.rc.bt = bt;
    if (rt != px || bt != py)    {
        px = rt;
        py = bt;
        SaveBorder(dwnd.rc);
        RepaintBorder(&dwnd, NULL);
    }
}
#ifdef INCLUDE_MULTI_WINDOWS
/* ----- adjust a rectangle to include the shadow ----- */
static DFRECT adjShadow(DFWINDOW wnd)
{
    DFRECT rc;
    rc = wnd->rc;
    if (TestAttribute(wnd, SHADOW))    {
        if (RectRight(rc) < DfGetScreenWidth()-1)
            RectRight(rc)++;
        if (RectBottom(rc) < DfGetScreenHeight()-1)
            RectBottom(rc)++;
    }
    return rc;
}
/* --- repaint a rectangular subsection of a window --- */
static void PaintOverLap(DFWINDOW wnd, DFRECT rc)
{
    if (isVisible(wnd))    {
        int isBorder, isTitle, isData;
        isBorder = isTitle = FALSE;
        isData = TRUE;
        if (TestAttribute(wnd, HASBORDER))    {
            isBorder =  RectLeft(rc) == 0 &&
                        RectTop(rc) < WindowHeight(wnd);
            isBorder |= RectLeft(rc) < WindowWidth(wnd) &&
                        RectRight(rc) >= WindowWidth(wnd)-1 &&
                        RectTop(rc) < WindowHeight(wnd);
            isBorder |= RectTop(rc) == 0 &&
                        RectLeft(rc) < WindowWidth(wnd);
            isBorder |= RectTop(rc) < WindowHeight(wnd) &&
                        RectBottom(rc) >= WindowHeight(wnd)-1 &&
                        RectLeft(rc) < WindowWidth(wnd);
        }
        else if (TestAttribute(wnd, HASTITLEBAR))
            isTitle = RectTop(rc) == 0 &&
                      RectRight(rc) > 0 &&
                      RectLeft(rc)<WindowWidth(wnd)-BorderAdj(wnd);

        if (RectLeft(rc) >= WindowWidth(wnd)-BorderAdj(wnd))
            isData = FALSE;
        if (RectTop(rc) >= WindowHeight(wnd)-BottomBorderAdj(wnd))
            isData = FALSE;
        if (TestAttribute(wnd, HASBORDER))    {
            if (RectRight(rc) == 0)
                isData = FALSE;
            if (RectBottom(rc) == 0)
                isData = FALSE;
        }
        if (TestAttribute(wnd, SHADOW))
            isBorder |= RectRight(rc) == WindowWidth(wnd) ||
                        RectBottom(rc) == WindowHeight(wnd);
        if (isData)
            DfSendMessage(wnd, PAINT, (PARAM) &rc, TRUE);
        if (isBorder)
            DfSendMessage(wnd, BORDER, (PARAM) &rc, 0);
        else if (isTitle)
            DisplayTitle(wnd, &rc);
    }
}
/* ------ paint the part of a window that is overlapped
            by another window that is being hidden ------- */
static void PaintOver(DFWINDOW wnd)
{
    DFRECT wrc, rc;
    wrc = adjShadow(HiddenWindow);
    rc = adjShadow(wnd);
    rc = subRectangle(rc, wrc);
    if (ValidRect(rc))
        PaintOverLap(wnd, RelativeWindowRect(wnd, rc));
}
/* --- paint the overlapped parts of all children --- */
static void PaintOverChildren(DFWINDOW pwnd)
{
    DFWINDOW cwnd = FirstWindow(pwnd);
    while (cwnd != NULL)    {
        if (cwnd != HiddenWindow)    {
            PaintOver(cwnd);
            PaintOverChildren(cwnd);
        }
        cwnd = NextWindow(cwnd);
    }
}
/* -- recursive overlapping paint of parents -- */
static void PaintOverParents(DFWINDOW wnd)
{
    DFWINDOW pwnd = GetParent(wnd);
    if (pwnd != NULL)    {
        PaintOverParents(pwnd);
        PaintOver(pwnd);
        PaintOverChildren(pwnd);
    }
}
/* - paint the parts of all windows that a window is over - */
static void PaintOverLappers(DFWINDOW wnd)
{
    HiddenWindow = wnd;
    PaintOverParents(wnd);
}
/* --- paint those parts of a window that are overlapped --- */
static void PaintUnderLappers(DFWINDOW wnd)
{
    DFWINDOW hwnd = NextWindow(wnd);
    while (hwnd != NULL)    {
        /* ------- test only at document window level ------ */
        DFWINDOW pwnd = GetParent(hwnd);
/*        if (pwnd == NULL || GetClass(pwnd) == APPLICATION)  */  {
            /* ---- don't bother testing self ----- */
            if (isVisible(hwnd) && hwnd != wnd)    {
                /* --- see if other window is descendent --- */
                while (pwnd != NULL)    {
                    if (pwnd == wnd)
                        break;
                    pwnd = GetParent(pwnd);
                }
                /* ----- don't test descendent overlaps ----- */
                if (pwnd == NULL)    {
                    /* -- see if other window is ancestor --- */
                    pwnd = GetParent(wnd);
                    while (pwnd != NULL)    {
                        if (pwnd == hwnd)
                            break;
                        pwnd = GetParent(pwnd);
                    }
                    /* --- don't test ancestor overlaps --- */
                    if (pwnd == NULL)    {
                        HiddenWindow = GetAncestor(hwnd);
                        ClearVisible(HiddenWindow);
                        PaintOver(wnd);
                        SetVisible(HiddenWindow);
                    }
                }
            }
        }
        hwnd = NextWindow(hwnd);
    }
    /* --------- repaint all children of this window
        the same way ----------- */
    hwnd = FirstWindow(wnd);
    while (hwnd != NULL)    {
        PaintUnderLappers(hwnd);
        hwnd = NextWindow(hwnd);
    }
}
#endif /* #ifdef INCLUDE_MULTI_WINDOWS */

/* --- save video area to be used by dummy window border --- */
static void SaveBorder(DFRECT rc)
{
	Bht = RectBottom(rc) - RectTop(rc) + 1;
	Bwd = RectRight(rc) - RectLeft(rc) + 1;
	Bsave = DFrealloc(Bsave, Bht * Bwd * sizeof(CHAR_INFO));

	GetVideo(rc,Bsave);
}
/* ---- restore video area used by dummy window border ---- */
static void RestoreBorder(DFRECT rc)
{
	if (Bsave != NULL)
	{
		StoreVideo(rc, Bsave);
		free(Bsave);
		Bsave = NULL;
	}
}
/* ----- test if screen coordinates are in a window ---- */
static BOOL InsideWindow(DFWINDOW wnd, int x, int y)
{
    DFRECT rc;
    rc = WindowRect(wnd);
    if (!TestAttribute(wnd, NOCLIP))
    {
        DFWINDOW pwnd = GetParent(wnd);
        while (pwnd != NULL)
		{
            rc = subRectangle(rc, ClientRect(pwnd));
            pwnd = GetParent(pwnd);
        }
    }
    return InsideRect(x, y, rc);
}

BOOL isDerivedFrom(DFWINDOW wnd, DFCLASS class)
{
    DFCLASS tclass = GetClass(wnd);
    while (tclass != -1)    {
        if (tclass == class)
            return TRUE;
        tclass = (classdefs[tclass].base);
    }
    return FALSE;
}

/* -- find the oldest document window ancestor of a window -- */
DFWINDOW GetAncestor(DFWINDOW wnd)
{
    if (wnd != NULL)    {
        while (GetParent(wnd) != NULL)    {
            if (GetClass(GetParent(wnd)) == APPLICATION)
                break;
            wnd = GetParent(wnd);
        }
    }
    return wnd;
}

BOOL isVisible(DFWINDOW wnd)
{
    while (wnd != NULL)    {
        if (isHidden(wnd))
            return FALSE;
        wnd = GetParent(wnd);
    }
    return TRUE;
}

/* -- adjust a window's rectangle to clip it to its parent - */
static DFRECT ClipRect(DFWINDOW wnd)
{
    DFRECT rc;
    rc = WindowRect(wnd);
    if (TestAttribute(wnd, SHADOW))    {
        RectBottom(rc)++;
        RectRight(rc)++;
    }
	return ClipRectangle(wnd, rc);
}

/* -- get the video memory that is to be used by a window -- */
static void GetVideoBuffer(DFWINDOW wnd)
{
    DFRECT rc;
    int ht;
    int wd;

    rc = ClipRect(wnd);
    ht = RectBottom(rc) - RectTop(rc) + 1;
    wd = RectRight(rc) - RectLeft(rc) + 1;
    wnd->videosave = DFrealloc(wnd->videosave, (ht * wd * sizeof(CHAR_INFO)));
    GetVideo(rc, wnd->videosave);
}

/* -- put the video memory that is used by a window -- */
static void PutVideoBuffer(DFWINDOW wnd)
{
	if (wnd->videosave != NULL)
	{
		DFRECT rc;
		rc = ClipRect(wnd);
		StoreVideo(rc, wnd->videosave);
		free(wnd->videosave);
		wnd->videosave = NULL;
	}
}

/* ------- return TRUE if awnd is an ancestor of wnd ------- */
BOOL isAncestor(DFWINDOW wnd, DFWINDOW awnd)
{
	while (wnd != NULL)	{
		if (wnd == awnd)
			return TRUE;
		wnd = GetParent(wnd);
	}
	return FALSE;
}

/* EOF */
