/* ------------- normal.c ------------ */

#include "dflat.h"

#ifdef INCLUDE_MULTI_WINDOWS
static void near PaintOverLappers(WINDOW wnd);
static void near PaintUnderLappers(WINDOW wnd);
#endif

static BOOL InsideWindow(WINDOW, int, int);
static void TerminateMoveSize(void);
static void SaveBorder(RECT);
static void RestoreBorder(RECT);
static void GetVideoBuffer(WINDOW);
static void PutVideoBuffer(WINDOW);
#ifdef INCLUDE_MINIMIZE
static RECT PositionIcon(WINDOW);
#endif
static void near dragborder(WINDOW, int, int);
static void near sizeborder(WINDOW, int, int);
static int px = -1, py = -1;
static int diff;
static struct window dwnd = {DUMMY, NULL, NormalProc,
                                {-1,-1,-1,-1}};
static unsigned short int *Bsave;
static int Bht, Bwd;
BOOL WindowMoving;
BOOL WindowSizing;
/* -------- array of class definitions -------- */
CLASSDEFS classdefs[] = {
    #undef ClassDef
    #define ClassDef(c,b,p,a) {b,p,a},
    #include "classes.h"
};
WINDOW HiddenWindow;

/* --------- CREATE_WINDOW Message ---------- */
static void CreateWindowMsg(WINDOW wnd)
{
    AppendWindow(wnd);
    if (!SendMessage(NULL, MOUSE_INSTALLED, 0, 0))
        ClearAttribute(wnd, VSCROLLBAR | HSCROLLBAR);
    if (TestAttribute(wnd, SAVESELF) && isVisible(wnd))
        GetVideoBuffer(wnd);
}

/* --------- SHOW_WINDOW Message ---------- */
static void ShowWindowMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    if (GetParent(wnd) == NULL || isVisible(GetParent(wnd)))    {
		WINDOW cwnd;
        if (TestAttribute(wnd, SAVESELF) &&
                        wnd->videosave == NULL)
            GetVideoBuffer(wnd);
        SetVisible(wnd);
        SendMessage(wnd, PAINT, 0, TRUE);
        SendMessage(wnd, BORDER, 0, 0);
        /* --- show the children of this window --- */
		cwnd = FirstWindow(wnd);
		while (cwnd != NULL)	{
            if (cwnd->condition != ISCLOSING)
                SendMessage(cwnd, SHOW_WINDOW, p1, p2);
			cwnd = NextWindow(cwnd);
        }
    }
}

/* --------- HIDE_WINDOW Message ---------- */
static void HideWindowMsg(WINDOW wnd)
{
    if (isVisible(wnd))    {
        ClearVisible(wnd);
        /* --- paint what this window covered --- */
	    if (TestAttribute(wnd, SAVESELF))
            PutVideoBuffer(wnd);
#ifdef INCLUDE_MULTI_WINDOWS
        else
            PaintOverLappers(wnd);
#endif
		wnd->wasCleared = FALSE;
    }
}

/* --------- KEYBOARD Message ---------- */
static BOOL KeyboardMsg(WINDOW wnd, PARAM p1, PARAM p2)
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
                if (y < SCREENHEIGHT-1)
                    y++;
                break;
            case FWD:
                if (x < SCREENWIDTH-1)
                    x++;
                break;
            case BS:
                if (x)
                    --x;
                break;
            case '\r':
                SendMessage(wnd,BUTTON_RELEASED,x,y);
            default:
                return TRUE;
        }
        /* -- use the mouse functions to move/size - */
        SendMessage(wnd, MOUSE_CURSOR, x, y);
        SendMessage(wnd, MOUSE_MOVED, x, y);
        return TRUE;
    }
    switch ((int)p1)    {
        case F1:
            SendMessage(wnd, COMMAND, ID_HELP, 0);
            return TRUE;
        case ' ':
            if ((int)p2 & ALTKEY)
                if (TestAttribute(wnd, HASTITLEBAR))
                    if (TestAttribute(wnd, CONTROLBOX))
                        BuildSystemMenu(wnd);
            return TRUE;
        case CTRL_F4:
            if (TestAttribute(wnd, CONTROLBOX))	{
            	SendMessage(wnd, CLOSE_WINDOW, 0, 0);
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
static void CommandMsg(WINDOW wnd, PARAM p1)
{
    switch ((int)p1)    {
        case ID_HELP:
            DisplayHelp(wnd,ClassNames[GetClass(wnd)]);
            break;
#ifdef INCLUDE_RESTORE
        case ID_SYSRESTORE:
            SendMessage(wnd, RESTORE, 0, 0);
            break;
#endif
        case ID_SYSMOVE:
            SendMessage(wnd, CAPTURE_MOUSE, TRUE,
                (PARAM) &dwnd);
            SendMessage(wnd, CAPTURE_KEYBOARD, TRUE,
                (PARAM) &dwnd);
            SendMessage(wnd, MOUSE_CURSOR,
                GetLeft(wnd), GetTop(wnd));
            WindowMoving = TRUE;
            dragborder(wnd, GetLeft(wnd), GetTop(wnd));
            break;
        case ID_SYSSIZE:
            SendMessage(wnd, CAPTURE_MOUSE, TRUE,
                (PARAM) &dwnd);
            SendMessage(wnd, CAPTURE_KEYBOARD, TRUE,
                (PARAM) &dwnd);
            SendMessage(wnd, MOUSE_CURSOR,
                GetRight(wnd), GetBottom(wnd));
            WindowSizing = TRUE;
            dragborder(wnd, GetLeft(wnd), GetTop(wnd));
            break;
#ifdef INCLUDE_MINIMIZE
        case ID_SYSMINIMIZE:
            SendMessage(wnd, MINIMIZE, 0, 0);
            break;
#endif
#ifdef INCLUDE_MAXIMIZE
        case ID_SYSMAXIMIZE:
            SendMessage(wnd, MAXIMIZE, 0, 0);
            break;
#endif
        case ID_SYSCLOSE:
            SendMessage(wnd, CLOSE_WINDOW, 0, 0);
			SkipApplicationControls();
            break;
        default:
            break;
    }
}

/* --------- SETFOCUS Message ---------- */
static void SetFocusMsg(WINDOW wnd, PARAM p1)
{
	RECT rc = {0,0,0,0};
    if (p1 && wnd != NULL && inFocus != wnd)    {
		WINDOW This, thispar;
		WINDOW that = NULL, thatpar = NULL;

		WINDOW cwnd = wnd, fwnd = GetParent(wnd);
		/* ---- post focus in ancestors ---- */
		while (fwnd != NULL)	{
			fwnd->childfocus = cwnd;
			cwnd = fwnd;
			fwnd = GetParent(fwnd);
		}
		/* ---- de-post focus in self and children ---- */
		fwnd = wnd;
		while (fwnd != NULL)	{
			cwnd = fwnd->childfocus;
			fwnd->childfocus = NULL;
			fwnd = cwnd;
		}

		This = wnd;
		that = thatpar = inFocus;

		/* ---- find common ancestor of prev focus and this window --- */
		while (thatpar != NULL)	{
			thispar = wnd;
			while (thispar != NULL)	{
				if (This == CaptureMouse || This == CaptureKeyboard)	{
					/* ---- don't repaint if this window has capture ---- */
					that = thatpar = NULL;
					break;
				}
				if (thispar == thatpar)	{
					/* ---- don't repaint if SAVESELF window had focus ---- */
					if (This != that && TestAttribute(that, SAVESELF))
						that = thatpar = NULL;
					break;
				}
				This = thispar;
				thispar = GetParent(thispar);
			}
			if (thispar != NULL)
				break;
			that = thatpar;
			thatpar = GetParent(thatpar);
		}
		if (inFocus != NULL)
	        SendMessage(inFocus, SETFOCUS, FALSE, 0);
        inFocus = wnd;
		if (that != NULL && isVisible(wnd))	{
			rc = subRectangle(WindowRect(that), WindowRect(This));
			if (!ValidRect(rc))	{
				if (ApplicationWindow != NULL)	{
					WINDOW fwnd = FirstWindow(ApplicationWindow);
					while (fwnd != NULL)	{
						if (!isAncestor(wnd, fwnd))	{
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
			This = NULL;
		ReFocus(wnd);
		if (This != NULL &&
				(!isVisible(This) || !TestAttribute(This, SAVESELF)))	{
			wnd->wasCleared = FALSE;
	        SendMessage(This, SHOW_WINDOW, 0, 0);
		}
		else if (!isVisible(wnd))
	        SendMessage(wnd, SHOW_WINDOW, 0, 0);
		else 
		    SendMessage(wnd, BORDER, 0, 0);
    }
    else if (!p1 && inFocus == wnd)    {
        /* -------- clearing focus --------- */
        inFocus = NULL;
        SendMessage(wnd, BORDER, 0, 0);
    }
}

/* --------- DOUBLE_CLICK Message ---------- */
static void DoubleClickMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int mx = (int) p1 - GetLeft(wnd);
    int my = (int) p2 - GetTop(wnd);
    if (!WindowSizing && !WindowMoving)	{
        if (HitControlBox(wnd, mx, my))	{
            PostMessage(wnd, CLOSE_WINDOW, 0, 0);
			SkipApplicationControls();
		}
	}
}

/* --------- LEFT_BUTTON Message ---------- */
static void LeftButtonMsg(WINDOW wnd, PARAM p1, PARAM p2)
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
                    SendMessage(wnd, RESTORE, 0, 0);
#ifdef INCLUDE_MAXIMIZE
                else
                    /* --- hit the maximize box --- */
                    SendMessage(wnd, MAXIMIZE, 0, 0);
#endif
                return;
            }
#ifdef INCLUDE_MINIMIZE
            if (mx == WindowWidth(wnd)-3)    {
                /* --- hit the minimize box --- */
                if (wnd->condition != ISMINIMIZED)
                    SendMessage(wnd, MINIMIZE, 0, 0);
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
            SendMessage(wnd, CAPTURE_MOUSE, TRUE,
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
	        if (!TestAttribute(wnd, SIZEABLE))
    	        return;
        }
#endif
        WindowSizing = TRUE;
        SendMessage(wnd, CAPTURE_MOUSE,
            TRUE, (PARAM) &dwnd);
        dragborder(wnd, GetLeft(wnd), GetTop(wnd));
    }
}

/* --------- MOUSE_MOVED Message ---------- */
static BOOL MouseMovedMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    if (WindowMoving)    {
        int leftmost = 0, topmost = 0,
            bottommost = SCREENHEIGHT-2,
            rightmost = SCREENWIDTH-2;
        int x = (int) p1 - diff;
        int y = (int) p2;
        if (GetParent(wnd) != NULL &&
                !TestAttribute(wnd, NOCLIP))    {
            WINDOW wnd1 = GetParent(wnd);
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
            SendMessage(NULL,MOUSE_CURSOR,x+diff,y);
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
static void MaximizeMsg(WINDOW wnd)
{
    RECT rc = {0, 0, 0, 0};
    RECT holdrc;
    holdrc = wnd->RestoredRC;
    rc.rt = SCREENWIDTH-1;
    rc.bt = SCREENHEIGHT-1;
    if (GetParent(wnd))
        rc = ClientRect(GetParent(wnd));
    wnd->oldcondition = wnd->condition;
    wnd->condition = ISMAXIMIZED;
	wnd->wasCleared = FALSE;
    SendMessage(wnd, HIDE_WINDOW, 0, 0);
    SendMessage(wnd, MOVE,
        RectLeft(rc), RectTop(rc));
    SendMessage(wnd, SIZE,
        RectRight(rc), RectBottom(rc));
    if (wnd->restored_attrib == 0)
        wnd->restored_attrib = wnd->attrib;
    ClearAttribute(wnd, SHADOW);
    SendMessage(wnd, SHOW_WINDOW, 0, 0);
    wnd->RestoredRC = holdrc;
}
#endif

#ifdef INCLUDE_MINIMIZE
/* --------- MINIMIZE Message ---------- */
static void MinimizeMsg(WINDOW wnd)
{
    RECT rc;
    RECT holdrc;

    holdrc = wnd->RestoredRC;
    rc = PositionIcon(wnd);
    wnd->oldcondition = wnd->condition;
    wnd->condition = ISMINIMIZED;
	wnd->wasCleared = FALSE;
    SendMessage(wnd, HIDE_WINDOW, 0, 0);
    SendMessage(wnd, MOVE,
        RectLeft(rc), RectTop(rc));
    SendMessage(wnd, SIZE,
        RectRight(rc), RectBottom(rc));
	if (wnd == inFocus)
	    SetNextFocus();
    if (wnd->restored_attrib == 0)
        wnd->restored_attrib = wnd->attrib;
    ClearAttribute(wnd,
        SHADOW | SIZEABLE | HASMENUBAR |
        VSCROLLBAR | HSCROLLBAR);
    SendMessage(wnd, SHOW_WINDOW, 0, 0);
    wnd->RestoredRC = holdrc;
}
#endif

#ifdef INCLUDE_RESTORE
/* --------- RESTORE Message ---------- */
static void RestoreMsg(WINDOW wnd)
{
    RECT holdrc;
    holdrc = wnd->RestoredRC;
    wnd->oldcondition = wnd->condition;
    wnd->condition = ISRESTORED;
	wnd->wasCleared = FALSE;
    SendMessage(wnd, HIDE_WINDOW, 0, 0);
    wnd->attrib = wnd->restored_attrib;
    wnd->restored_attrib = 0;
    SendMessage(wnd, MOVE, wnd->RestoredRC.lf,
        wnd->RestoredRC.tp);
    wnd->RestoredRC = holdrc;
    SendMessage(wnd, SIZE, wnd->RestoredRC.rt,
        wnd->RestoredRC.bt);
	if (wnd != inFocus)
	    SendMessage(wnd, SETFOCUS, TRUE, 0);
	else
	    SendMessage(wnd, SHOW_WINDOW, 0, 0);
}
#endif

/* --------- MOVE Message ---------- */
static void MoveMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    WINDOW cwnd;
    BOOL wasVisible = isVisible(wnd);
    int xdif = (int) p1 - wnd->rc.lf;
    int ydif = (int) p2 - wnd->rc.tp;

    if (xdif == 0 && ydif == 0)
        return;
	wnd->wasCleared = FALSE;
    if (wasVisible)
        SendMessage(wnd, HIDE_WINDOW, 0, 0);
    wnd->rc.lf = (int) p1;
    wnd->rc.tp = (int) p2;
    wnd->rc.rt = GetLeft(wnd)+WindowWidth(wnd)-1;
    wnd->rc.bt = GetTop(wnd)+WindowHeight(wnd)-1;
    if (wnd->condition == ISRESTORED)
        wnd->RestoredRC = wnd->rc;

	cwnd = FirstWindow(wnd);
	while (cwnd != NULL)	{
        SendMessage(cwnd, MOVE, cwnd->rc.lf+xdif, cwnd->rc.tp+ydif);
		cwnd = NextWindow(cwnd);
    }
    if (wasVisible)
        SendMessage(wnd, SHOW_WINDOW, 0, 0);
}

/* --------- SIZE Message ---------- */
static void SizeMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    BOOL wasVisible = isVisible(wnd);
    WINDOW cwnd;
    RECT rc;
    int xdif = (int) p1 - wnd->rc.rt;
    int ydif = (int) p2 - wnd->rc.bt;

    if (xdif == 0 && ydif == 0)
        return;
	wnd->wasCleared = FALSE;
    if (wasVisible)
        SendMessage(wnd, HIDE_WINDOW, 0, 0);
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
            SendMessage(cwnd, SIZE, RectRight(rc), RectBottom(rc));
		cwnd = NextWindow(cwnd);
    }

#endif
    if (wasVisible)
        SendMessage(wnd, SHOW_WINDOW, 0, 0);
}

/* --------- CLOSE_WINDOW Message ---------- */
static void CloseWindowMsg(WINDOW wnd)
{
    WINDOW cwnd;
    wnd->condition = ISCLOSING;
    /* ----------- hide this window ------------ */
    SendMessage(wnd, HIDE_WINDOW, 0, 0);

    /* --- close the children of this window --- */
	cwnd = LastWindow(wnd);
	while (cwnd != NULL)	{
        if (inFocus == cwnd)
            inFocus = wnd;
        SendMessage(cwnd,CLOSE_WINDOW,0,0);
		cwnd = LastWindow(wnd);
    }

	/* ----- release captured resources ------ */
    if (wnd->PrevClock != NULL)
        SendMessage(wnd, RELEASE_CLOCK, 0, 0);
    if (wnd->PrevMouse != NULL)
        SendMessage(wnd, RELEASE_MOUSE, 0, 0);
    if (wnd->PrevKeyboard != NULL)
        SendMessage(wnd, RELEASE_KEYBOARD, 0, 0);

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
int NormalProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)    {
        case CREATE_WINDOW:
            CreateWindowMsg(wnd);
            break;
        case SHOW_WINDOW:
            ShowWindowMsg(wnd, p1, p2);
            break;
        case HIDE_WINDOW:
            HideWindowMsg(wnd);
            break;
        case DISPLAY_HELP:
            return DisplayHelp(wnd, (char *)p1);
        case INSIDE_WINDOW:
            return InsideWindow(wnd, (int) p1, (int) p2);
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            /* ------- fall through ------- */
        case ADDSTATUS:
        case SHIFT_CHANGED:
            if (GetParent(wnd) != NULL)
                PostMessage(GetParent(wnd), msg, p1, p2);
            break;
        case PAINT:
            if (isVisible(wnd))	{
#ifdef INCLUDE_MULTI_WINDOWS
				if (wnd->wasCleared)
					PaintUnderLappers(wnd);
				else
#endif
				{
					wnd->wasCleared = TRUE;
	                ClearWindow(wnd, (RECT *)p1, ' ');
				}
			}
            break;
        case BORDER:
            if (isVisible(wnd))    {
                if (TestAttribute(wnd, HASBORDER))
                    RepaintBorder(wnd, (RECT *)p1);
                else if (TestAttribute(wnd, HASTITLEBAR))
                    DisplayTitle(wnd, (RECT *)p1);
            }
            break;
        case COMMAND:
            CommandMsg(wnd, p1);
            break;
        case SETFOCUS:
            SetFocusMsg(wnd, p1);
            break;
        case DOUBLE_CLICK:
            DoubleClickMsg(wnd, p1, p2);
            break;
        case LEFT_BUTTON:
            LeftButtonMsg(wnd, p1, p2);
            break;
        case MOUSE_MOVED:
            if (MouseMovedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case BUTTON_RELEASED:
            if (WindowMoving || WindowSizing)    {
                if (WindowMoving)
                    PostMessage(wnd,MOVE,dwnd.rc.lf,dwnd.rc.tp);
                else
                    PostMessage(wnd,SIZE,dwnd.rc.rt,dwnd.rc.bt);
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
                    SendMessage(wnd, MAXIMIZE, 0, 0);
                else
#endif
                    RestoreMsg(wnd);
            }
            break;
#endif
        case MOVE:
            MoveMsg(wnd, p1, p2);
            break;
        case SIZE:    {
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
static RECT LowerRight(RECT prc)
{
    RECT rc;
    RectLeft(rc) = RectRight(prc) - ICONWIDTH;
    RectTop(rc) = RectBottom(prc) - ICONHEIGHT;
    RectRight(rc) = RectLeft(rc)+ICONWIDTH-1;
    RectBottom(rc) = RectTop(rc)+ICONHEIGHT-1;
    return rc;
}
/* ----- compute a position for a minimized window icon ---- */
static RECT PositionIcon(WINDOW wnd)
{
	WINDOW pwnd = GetParent(wnd);
    RECT rc;
    RectLeft(rc) = SCREENWIDTH-ICONWIDTH;
    RectTop(rc) = SCREENHEIGHT-ICONHEIGHT;
    RectRight(rc) = SCREENWIDTH-1;
    RectBottom(rc) = SCREENHEIGHT-1;
    if (pwnd != NULL)    {
        RECT prc = WindowRect(pwnd);
		WINDOW cwnd = FirstWindow(pwnd);
        rc = LowerRight(prc);
        /* - search for icon available location - */
		while (cwnd != NULL)	{
            if (cwnd->condition == ISMINIMIZED)    {
                RECT rc1;
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
    SendMessage(&dwnd, RELEASE_MOUSE, TRUE, 0);
    SendMessage(&dwnd, RELEASE_KEYBOARD, TRUE, 0);
    RestoreBorder(dwnd.rc);
    WindowMoving = WindowSizing = FALSE;
}
/* ---- build a dummy window border for moving or sizing --- */
static void near dragborder(WINDOW wnd, int x, int y)
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
static void near sizeborder(WINDOW wnd, int rt, int bt)
{
    int leftmost = GetLeft(wnd)+10;
    int topmost = GetTop(wnd)+3;
    int bottommost = SCREENHEIGHT-1;
    int rightmost  = SCREENWIDTH-1;
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
    SendMessage(NULL, MOUSE_CURSOR, rt, bt);

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
static RECT adjShadow(WINDOW wnd)
{
    RECT rc;
    rc = wnd->rc;
    if (TestAttribute(wnd, SHADOW))    {
        if (RectRight(rc) < SCREENWIDTH-1)
            RectRight(rc)++;           
        if (RectBottom(rc) < SCREENHEIGHT-1)
            RectBottom(rc)++;
    }
    return rc;
}
/* --- repaint a rectangular subsection of a window --- */
static void near PaintOverLap(WINDOW wnd, RECT rc)
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
        if (isData)	{
			wnd->wasCleared = FALSE;
            SendMessage(wnd, PAINT, (PARAM) &rc, TRUE);
		}
        if (isBorder)
            SendMessage(wnd, BORDER, (PARAM) &rc, 0);
        else if (isTitle)
            DisplayTitle(wnd, &rc);
    }
}
/* ------ paint the part of a window that is overlapped
            by another window that is being hidden ------- */
static void PaintOver(WINDOW wnd)
{
    RECT wrc, rc;
    wrc = adjShadow(HiddenWindow);
    rc = adjShadow(wnd);
    rc = subRectangle(rc, wrc);
    if (ValidRect(rc))
        PaintOverLap(wnd, RelativeWindowRect(wnd, rc));
}
/* --- paint the overlapped parts of all children --- */
static void PaintOverChildren(WINDOW pwnd)
{
    WINDOW cwnd = FirstWindow(pwnd);
    while (cwnd != NULL)    {
        if (cwnd != HiddenWindow)    {
            PaintOver(cwnd);
            PaintOverChildren(cwnd);
        }
        cwnd = NextWindow(cwnd);
    }
}
/* -- recursive overlapping paint of parents -- */
static void PaintOverParents(WINDOW wnd)
{
    WINDOW pwnd = GetParent(wnd);
    if (pwnd != NULL)    {
        PaintOverParents(pwnd);
        PaintOver(pwnd);
        PaintOverChildren(pwnd);
    }
}
/* - paint the parts of all windows that a window is over - */
static void near PaintOverLappers(WINDOW wnd)
{
    HiddenWindow = wnd;
    PaintOverParents(wnd);
}
/* --- paint those parts of a window that are overlapped --- */
static void near PaintUnderLappers(WINDOW wnd)
{
    WINDOW hwnd = NextWindow(wnd);
    while (hwnd != NULL)    {
        /* ------- test only at document window level ------ */
        WINDOW pwnd = GetParent(hwnd);
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
static void SaveBorder(RECT rc)
{
    RECT lrc;
    int i;
    unsigned short int *cp;
    Bht = RectBottom(rc) - RectTop(rc) + 1;
    Bwd = RectRight(rc) - RectLeft(rc) + 1;
    Bsave = DFrealloc(Bsave, (Bht + Bwd) * 2 * sizeof(short int));

    lrc = rc;
    RectBottom(lrc) = RectTop(lrc);
    getvideo(lrc, Bsave);
    RectTop(lrc) = RectBottom(lrc) = RectBottom(rc);
    getvideo(lrc, Bsave + Bwd);
    cp = Bsave + Bwd * 2;
    for (i = 1; i < Bht-1; i++)    {
        *cp++ = GetVideoChar(RectLeft(rc),RectTop(rc)+i);
        *cp++ = GetVideoChar(RectRight(rc),RectTop(rc)+i);
    }
}
/* ---- restore video area used by dummy window border ---- */
static void RestoreBorder(RECT rc)
{
    if (Bsave != NULL)    {
        RECT lrc;
        int i;
        unsigned short int *cp;
        lrc = rc;
        RectBottom(lrc) = RectTop(lrc);
        storevideo(lrc, Bsave);
        RectTop(lrc) = RectBottom(lrc) = RectBottom(rc);
        storevideo(lrc, Bsave + Bwd);
        cp = Bsave + Bwd * 2;
        for (i = 1; i < Bht-1; i++)    {
            PutVideoChar(RectLeft(rc),RectTop(rc)+i, *cp++);
            PutVideoChar(RectRight(rc),RectTop(rc)+i, *cp++);
        }
        free(Bsave);
        Bsave = NULL;
    }
}
/* ----- test if screen coordinates are in a window ---- */
static BOOL InsideWindow(WINDOW wnd, int x, int y)
{
    RECT rc;
    rc = WindowRect(wnd);
    if (!TestAttribute(wnd, NOCLIP))    {
        WINDOW pwnd = GetParent(wnd);
        while (pwnd != NULL)    {
            rc = subRectangle(rc, ClientRect(pwnd));
            pwnd = GetParent(pwnd);
        }
    }
    return InsideRect(x, y, rc);
}

BOOL isDerivedFrom(WINDOW wnd, CLASS Class)
{
    CLASS tclass = GetClass(wnd);
    while (tclass != -1)    {
        if (tclass == Class)
            return TRUE;
        tclass = (classdefs[tclass].base);
    }
    return FALSE;
}

/* -- find the oldest document window ancestor of a window -- */
WINDOW GetAncestor(WINDOW wnd)
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

BOOL isVisible(WINDOW wnd)
{
    while (wnd != NULL)    {
        if (isHidden(wnd))
            return FALSE;
        wnd = GetParent(wnd);
    }
    return TRUE;
}

/* -- adjust a window's rectangle to clip it to its parent - */
static RECT near ClipRect(WINDOW wnd)
{
    RECT rc;
    rc = WindowRect(wnd);
    if (TestAttribute(wnd, SHADOW))    {
        RectBottom(rc)++;
        RectRight(rc)++;
    }
	return ClipRectangle(wnd, rc);
}

/* -- get the video memory that is to be used by a window -- */
static void GetVideoBuffer(WINDOW wnd)
{
    RECT rc;
    int ht;
    int wd;

    rc = ClipRect(wnd);
    ht = RectBottom(rc) - RectTop(rc) + 1;
    wd = RectRight(rc) - RectLeft(rc) + 1;
    wnd->videosave = DFrealloc(wnd->videosave, (ht * wd * 2));
    get_videomode();
    getvideo(rc, wnd->videosave);
}

/* -- put the video memory that is used by a window -- */
static void PutVideoBuffer(WINDOW wnd)
{
    if (wnd->videosave != NULL)    {
    	RECT rc;
    	rc = ClipRect(wnd);
    	get_videomode();
    	storevideo(rc, wnd->videosave);
    	free(wnd->videosave);
    	wnd->videosave = NULL;
	}
}

/* ------- return TRUE if awnd is an ancestor of wnd ------- */
BOOL isAncestor(WINDOW wnd, WINDOW awnd)
{
	while (wnd != NULL)	{
		if (wnd == awnd)
			return TRUE;
		wnd = GetParent(wnd);
	}
	return FALSE;
}

