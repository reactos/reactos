/* ------------- normal.c ------------ */

#include "dflat.h"

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
static struct DfWindow dwnd = {DF_DUMMY, NULL, DfNormalProc,
                                {-1,-1,-1,-1}};
static PCHAR_INFO Bsave;
static int Bht, Bwd;
BOOL DfWindowMoving;
BOOL DfWindowSizing;
/* -------- array of class definitions -------- */
DFCLASSDEFS DfClassDefs[] = {
    #undef DfClassDef
    #define DfClassDef(c,b,p,a) {b,p,a},
    #include "classes.h"
};
DFWINDOW HiddenWindow;

/* --------- DFM_CREATE_WINDOW Message ---------- */
static void CreateWindowMsg(DFWINDOW wnd)
{
	DfAppendWindow(wnd);
//	DfClearAttribute(wnd, DF_VSCROLLBAR | DF_HSCROLLBAR);
	if (DfTestAttribute(wnd, DF_SAVESELF) && DfIsVisible(wnd))
		GetVideoBuffer(wnd);
}

/* --------- DFM_SHOW_WINDOW Message ---------- */
static void ShowWindowMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
	if (DfGetParent(wnd) == NULL || DfIsVisible(DfGetParent(wnd)))
	{
		DFWINDOW cwnd;

		if (DfTestAttribute(wnd, DF_SAVESELF) && wnd->videosave == NULL)
			GetVideoBuffer(wnd);
		DfSetVisible(wnd);
		DfSendMessage(wnd, DFM_PAINT, 0, TRUE);
		DfSendMessage(wnd, DFM_BORDER, 0, 0);
		/* --- show the children of this window --- */
		cwnd = DfFirstWindow(wnd);
		while (cwnd != NULL)
		{
			if (cwnd->condition != DF_ISCLOSING)
				DfSendMessage(cwnd, DFM_SHOW_WINDOW, p1, p2);
			cwnd = DfNextWindow(cwnd);
		}
	}
}

/* --------- HIDE_WINDOW Message ---------- */
static void HideWindowMsg(DFWINDOW wnd)
{
	if (DfIsVisible(wnd))
	{
		DfClearVisible(wnd);
		/* --- paint what this window covered --- */
		if (DfTestAttribute(wnd, DF_SAVESELF))
			PutVideoBuffer(wnd);
#ifdef INCLUDE_MULTI_WINDOWS
		else
			PaintOverLappers(wnd);
#endif
    }
}

/* --------- DFM_KEYBOARD Message ---------- */
static BOOL KeyboardMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    if (DfWindowMoving || DfWindowSizing)    {
        /* -- move or size a window with keyboard -- */
        int x, y;
        x=DfWindowMoving?DfGetLeft(&dwnd):DfGetRight(&dwnd);
        y=DfWindowMoving?DfGetTop(&dwnd):DfGetBottom(&dwnd);
        switch ((int)p1)    {
            case DF_ESC:
                TerminateMoveSize();
                return TRUE;
            case DF_UP:
                if (y)
                    --y;
                break;
            case DF_DN:
                if (y < DfGetScreenHeight()-1)
                    y++;
                break;
            case DF_FWD:
                if (x < DfGetScreenWidth()-1)
                    x++;
                break;
            case DF_BS:
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
		case DF_F1:
			DfSendMessage(wnd, DFM_COMMAND, DF_ID_HELP, 0);
			return TRUE;

		case ' ':
			if ((int)p2 & DF_ALTKEY)
				if (DfTestAttribute(wnd, DF_HASTITLEBAR))
					if (DfTestAttribute(wnd, DF_CONTROLBOX))
						DfBuildSystemMenu(wnd);
			return TRUE;

		case DF_CTRL_F4:
			if (DfTestAttribute(wnd, DF_CONTROLBOX))
			{
				DfSendMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
				DfSkipApplicationControls();
				return TRUE;
			}
			break;

		default:
			break;
	}

	return FALSE;
}

/* --------- COMMAND Message ---------- */
static void CommandMsg(DFWINDOW wnd, DF_PARAM p1)
{
    switch ((int)p1)    {
        case DF_ID_HELP:
            DfDisplayHelp(wnd,DfClassNames[DfGetClass(wnd)]);
            break;
#ifdef INCLUDE_RESTORE
        case DF_ID_SYSRESTORE:
            DfSendMessage(wnd, DFM_RESTORE, 0, 0);
            break;
#endif
        case DF_ID_SYSMOVE:
            DfSendMessage(wnd, DFM_CAPTURE_MOUSE, TRUE,
                (DF_PARAM) &dwnd);
            DfSendMessage(wnd, DFM_CAPTURE_KEYBOARD, TRUE,
                (DF_PARAM) &dwnd);
            DfWindowMoving = TRUE;
            dragborder(wnd, DfGetLeft(wnd), DfGetTop(wnd));
            break;
        case DF_ID_SYSSIZE:
            DfSendMessage(wnd, DFM_CAPTURE_MOUSE, TRUE,
                (DF_PARAM) &dwnd);
            DfSendMessage(wnd, DFM_CAPTURE_KEYBOARD, TRUE,
                (DF_PARAM) &dwnd);
            DfWindowSizing = TRUE;
            dragborder(wnd, DfGetLeft(wnd), DfGetTop(wnd));
            break;
#ifdef INCLUDE_MINIMIZE
        case DF_ID_SYSMINIMIZE:
            DfSendMessage(wnd, DFM_MINIMIZE, 0, 0);
            break;
#endif
#ifdef INCLUDE_MAXIMIZE
        case DF_ID_SYSMAXIMIZE:
            DfSendMessage(wnd, DFM_MAXIMIZE, 0, 0);
            break;
#endif
        case DF_ID_SYSCLOSE:
            DfSendMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
			DfSkipApplicationControls();
            break;
        default:
            break;
    }
}

/* --------- DFM_SETFOCUS Message ---------- */
static void SetFocusMsg(DFWINDOW wnd, DF_PARAM p1)
{
	DFRECT rc = {0,0,0,0};

	if (p1 && wnd != NULL && DfInFocus != wnd)
	{
		DFWINDOW this, thispar;
		DFWINDOW that = NULL, thatpar = NULL;

		DFWINDOW cwnd = wnd, fwnd = DfGetParent(wnd);
		/* ---- post focus in ancestors ---- */
		while (fwnd != NULL)
		{
			fwnd->childfocus = cwnd;
			cwnd = fwnd;
			fwnd = DfGetParent(fwnd);
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
		that = thatpar = DfInFocus;

		/* ---- find common ancestor of prev focus and this window --- */
		while (thatpar != NULL)
		{
			thispar = wnd;
			while (thispar != NULL)
			{
				if (this == DfCaptureMouse || this == DfCaptureKeyboard)
				{
					/* ---- don't repaint if this window has capture ---- */
					that = thatpar = NULL;
					break;
				}
				if (thispar == thatpar)
				{
					/* ---- don't repaint if DF_SAVESELF window had focus ---- */
					if (this != that && DfTestAttribute(that, DF_SAVESELF))
						that = thatpar = NULL;
					break;
				}
				this = thispar;
				thispar = DfGetParent(thispar);
			}
			if (thispar != NULL)
				break;
			that = thatpar;
			thatpar = DfGetParent(thatpar);
		}
		if (DfInFocus != NULL)
			DfSendMessage(DfInFocus, DFM_SETFOCUS, FALSE, 0);
		DfInFocus = wnd;
		if (that != NULL && DfIsVisible(wnd))
		{
			rc = DfSubRectangle(DfWindowRect(that), DfWindowRect(this));
			if (!DfValidRect(rc))
			{
				if (DfApplicationWindow != NULL)
				{
					DFWINDOW fwnd = DfFirstWindow(DfApplicationWindow);
					while (fwnd != NULL)
					{
						if (!DfIsAncestor(wnd, fwnd))
						{
							rc = DfSubRectangle(DfWindowRect(wnd),DfWindowRect(fwnd));
							if (DfValidRect(rc))
								break;
						}
						fwnd = DfNextWindow(fwnd);
					}
				}
			}
		}
		if (that != NULL && !DfValidRect(rc) && DfIsVisible(wnd))
		{
			DfSendMessage(wnd, DFM_BORDER, 0, 0);
			this = NULL;
		}
		DfReFocus(wnd);
		if (this != NULL)
		DfSendMessage(this, DFM_SHOW_WINDOW, 0, 0);
	}
	else if (!p1 && DfInFocus == wnd)
	{
		/* clearing focus */
		DfInFocus = NULL;
		DfSendMessage(wnd, DFM_BORDER, 0, 0);
	}
}

/* --------- DOUBLE_CLICK Message ---------- */
static void DoubleClickMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int mx = (int) p1 - DfGetLeft(wnd);
    int my = (int) p2 - DfGetTop(wnd);
    if (!DfWindowSizing && !DfWindowMoving)	{
        if (DfHitControlBox(wnd, mx, my))	{
            DfPostMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
			DfSkipApplicationControls();
		}
	}
}

/* --------- DFM_LEFT_BUTTON Message ---------- */
static void LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int mx = (int) p1 - DfGetLeft(wnd);
    int my = (int) p2 - DfGetTop(wnd);
    if (DfWindowSizing || DfWindowMoving)
        return;
    if (DfHitControlBox(wnd, mx, my))    {
        DfBuildSystemMenu(wnd);
        return;
    }
    if (my == 0 && mx > -1 && mx < DfWindowWidth(wnd))  {
        /* ---------- hit the top border -------- */
        if (DfTestAttribute(wnd, DF_MINMAXBOX) &&
                DfTestAttribute(wnd, DF_HASTITLEBAR))  {
            if (mx == DfWindowWidth(wnd)-2)    {
                if (wnd->condition != DF_SRESTORED)
                    /* --- hit the restore box --- */
                    DfSendMessage(wnd, DFM_RESTORE, 0, 0);
#ifdef INCLUDE_MAXIMIZE
                else
                    /* --- hit the maximize box --- */
                    DfSendMessage(wnd, DFM_MAXIMIZE, 0, 0);
#endif
                return;
            }
#ifdef INCLUDE_MINIMIZE
            if (mx == DfWindowWidth(wnd)-3)    {
                /* --- hit the minimize box --- */
                if (wnd->condition != DF_ISMINIMIZED)
                    DfSendMessage(wnd, DFM_MINIMIZE, 0, 0);
                return;
            }
#endif
        }
#ifdef INCLUDE_MAXIMIZE
        if (wnd->condition == DF_ISMAXIMIZED)
            return;
#endif
        if (DfTestAttribute(wnd, DF_MOVEABLE))    {
            DfWindowMoving = TRUE;
            px = mx;
            py = my;
            diff = (int) mx;
            DfSendMessage(wnd, DFM_CAPTURE_MOUSE, TRUE,
                (DF_PARAM) &dwnd);
            dragborder(wnd, DfGetLeft(wnd), DfGetTop(wnd));
        }
        return;
    }
    if (mx == DfWindowWidth(wnd)-1 &&
            my == DfWindowHeight(wnd)-1)    {
        /* ------- hit the resize corner ------- */
#ifdef INCLUDE_MINIMIZE
        if (wnd->condition == DF_ISMINIMIZED)
            return;
#endif
        if (!DfTestAttribute(wnd, DF_SIZEABLE))
            return;
#ifdef INCLUDE_MAXIMIZE
        if (wnd->condition == DF_ISMAXIMIZED)    {
            if (DfGetParent(wnd) == NULL)
                return;
            if (DfTestAttribute(DfGetParent(wnd),DF_HASBORDER))
                return;
            /* ----- resizing a maximized window over a
                    borderless parent ----- */
            wnd = DfGetParent(wnd);
        }
#endif
        DfWindowSizing = TRUE;
        DfSendMessage(wnd, DFM_CAPTURE_MOUSE,
            TRUE, (DF_PARAM) &dwnd);
        dragborder(wnd, DfGetLeft(wnd), DfGetTop(wnd));
    }
}

/* --------- MOUSE_MOVED Message ---------- */
static BOOL MouseMovedMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    if (DfWindowMoving)    {
        int leftmost = 0, topmost = 0,
            bottommost = DfGetScreenHeight()-2,
            rightmost = DfGetScreenWidth()-2;
        int x = (int) p1 - diff;
        int y = (int) p2;
        if (DfGetParent(wnd) != NULL &&
                !DfTestAttribute(wnd, DF_NOCLIP))    {
            DFWINDOW wnd1 = DfGetParent(wnd);
            topmost    = DfGetClientTop(wnd1);
            leftmost   = DfGetClientLeft(wnd1);
            bottommost = DfGetClientBottom(wnd1);
            rightmost  = DfGetClientRight(wnd1);
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
    if (DfWindowSizing)    {
        sizeborder(wnd, (int) p1, (int) p2);
        return TRUE;
    }
    return FALSE;
}

#ifdef INCLUDE_MAXIMIZE
/* --------- DFM_MAXIMIZE Message ---------- */
static void MaximizeMsg(DFWINDOW wnd)
{
    DFRECT rc = {0, 0, 0, 0};
    DFRECT holdrc;
    holdrc = wnd->RestoredRC;
    rc.rt = DfGetScreenWidth()-1;
    rc.bt = DfGetScreenHeight()-1;
    if (DfGetParent(wnd))
        rc = DfClientRect(DfGetParent(wnd));
    wnd->oldcondition = wnd->condition;
    wnd->condition = DF_ISMAXIMIZED;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    DfSendMessage(wnd, DFM_MOVE,
        DfRectLeft(rc), DfRectTop(rc));
    DfSendMessage(wnd, DFM_DFM_SIZE,
        DfRectRight(rc), DfRectBottom(rc));
    if (wnd->restored_attrib == 0)
        wnd->restored_attrib = wnd->attrib;
    DfClearAttribute(wnd, DF_SHADOW);
    DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
    wnd->RestoredRC = holdrc;
}
#endif

#ifdef INCLUDE_MINIMIZE
/* --------- DFM_MINIMIZE Message ---------- */
static void MinimizeMsg(DFWINDOW wnd)
{
    DFRECT rc;
    DFRECT holdrc;

    holdrc = wnd->RestoredRC;
    rc = PositionIcon(wnd);
    wnd->oldcondition = wnd->condition;
    wnd->condition = DF_ISMINIMIZED;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    DfSendMessage(wnd, DFM_MOVE,
        DfRectLeft(rc), DfRectTop(rc));
    DfSendMessage(wnd, DFM_DFM_SIZE,
        DfRectRight(rc), DfRectBottom(rc));
	if (wnd == DfInFocus)
	    DfSetNextFocus();
    if (wnd->restored_attrib == 0)
        wnd->restored_attrib = wnd->attrib;
    DfClearAttribute(wnd,
        DF_SHADOW | DF_SIZEABLE | DF_HASMENUBAR |
        DF_VSCROLLBAR | DF_HSCROLLBAR);
    DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
    wnd->RestoredRC = holdrc;
}
#endif

#ifdef INCLUDE_RESTORE
/* --------- DFM_RESTORE Message ---------- */
static void RestoreMsg(DFWINDOW wnd)
{
    DFRECT holdrc;
    holdrc = wnd->RestoredRC;
    wnd->oldcondition = wnd->condition;
    wnd->condition = DF_SRESTORED;
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    wnd->attrib = wnd->restored_attrib;
    wnd->restored_attrib = 0;
    DfSendMessage(wnd, DFM_MOVE, wnd->RestoredRC.lf,
        wnd->RestoredRC.tp);
    wnd->RestoredRC = holdrc;
    DfSendMessage(wnd, DFM_DFM_SIZE, wnd->RestoredRC.rt,
        wnd->RestoredRC.bt);
	if (wnd != DfInFocus)
	    DfSendMessage(wnd, DFM_SETFOCUS, TRUE, 0);
	else
	    DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
}
#endif

/* --------- DFM_MOVE Message ---------- */
static void MoveMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    DFWINDOW cwnd;
    BOOL wasVisible = DfIsVisible(wnd);
    int xdif = (int) p1 - wnd->rc.lf;
    int ydif = (int) p2 - wnd->rc.tp;

    if (xdif == 0 && ydif == 0)
        return;
    if (wasVisible)
        DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    wnd->rc.lf = (int) p1;
    wnd->rc.tp = (int) p2;
    wnd->rc.rt = DfGetLeft(wnd)+DfWindowWidth(wnd)-1;
    wnd->rc.bt = DfGetTop(wnd)+DfWindowHeight(wnd)-1;
    if (wnd->condition == DF_SRESTORED)
        wnd->RestoredRC = wnd->rc;

	cwnd = DfFirstWindow(wnd);
	while (cwnd != NULL)	{
        DfSendMessage(cwnd, DFM_MOVE, cwnd->rc.lf+xdif, cwnd->rc.tp+ydif);
		cwnd = DfNextWindow(cwnd);
    }
    if (wasVisible)
        DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
}

/* --------- SIZE Message ---------- */
static void SizeMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    BOOL wasVisible = DfIsVisible(wnd);
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
    wnd->ht = DfGetBottom(wnd)-DfGetTop(wnd)+1;
    wnd->wd = DfGetRight(wnd)-DfGetLeft(wnd)+1;

    if (wnd->condition == DF_SRESTORED)
        wnd->RestoredRC = DfWindowRect(wnd);

#ifdef INCLUDE_MAXIMIZE
    rc = DfClientRect(wnd);

	cwnd = DfFirstWindow(wnd);
	while (cwnd != NULL)	{
        if (cwnd->condition == DF_ISMAXIMIZED)
            DfSendMessage(cwnd, DFM_DFM_SIZE, DfRectRight(rc), DfRectBottom(rc));
		cwnd = DfNextWindow(cwnd);
    }

#endif
    if (wasVisible)
        DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
}

/* --------- DFM_CLOSE_WINDOW Message ---------- */
static void CloseWindowMsg(DFWINDOW wnd)
{
    DFWINDOW cwnd;
    wnd->condition = DF_ISCLOSING;
    if (wnd->PrevMouse != NULL)
        DfSendMessage(wnd, DFM_RELEASE_MOUSE, 0, 0);
    if (wnd->PrevKeyboard != NULL)
        DfSendMessage(wnd, DFM_RELEASE_KEYBOARD, 0, 0);
    /* ----------- hide this window ------------ */
    DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
    /* --- close the children of this window --- */

	cwnd = DfLastWindow(wnd);
	while (cwnd != NULL)	{
        if (DfInFocus == cwnd)
            DfInFocus = wnd;
        DfSendMessage(cwnd,DFM_CLOSE_WINDOW,0,0);
		cwnd = DfLastWindow(wnd);
    }

    /* --- change focus if this window had it -- */
	if (wnd == DfInFocus)
	    DfSetPrevFocus();
    /* -- free memory allocated to this window - */
    if (wnd->title != NULL)
        free(wnd->title);
    if (wnd->videosave != NULL)
        free(wnd->videosave);
    /* -- remove window from parent's list of children -- */
	DfRemoveWindow(wnd);
    if (wnd == DfInFocus)
        DfInFocus = NULL;
    free(wnd);
}

/* ---- Window-processing module for DF_NORMAL window class ---- */
int DfNormalProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            CreateWindowMsg(wnd);
            break;
        case DFM_SHOW_WINDOW:
            ShowWindowMsg(wnd, p1, p2);
            break;
        case DFM_HIDE_WINDOW:
            HideWindowMsg(wnd);
            break;
        case DFM_DISPLAY_HELP:
            DfDisplayHelp(wnd, (char *)p1);
            break;
        case DFM_INSIDE_WINDOW:
            return InsideWindow(wnd, (int) p1, (int) p2);
        case DFM_KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            /* ------- fall through ------- */
        case DFM_ADDSTATUS:
        case DFM_SHIFT_CHANGED:
            if (DfGetParent(wnd) != NULL)
                DfPostMessage(DfGetParent(wnd), msg, p1, p2);
            break;
        case DFM_PAINT:
            if (DfIsVisible(wnd))
                DfClearWindow(wnd, (DFRECT *)p1, ' ');
            break;
        case DFM_BORDER:
            if (DfIsVisible(wnd))
            {
                if (DfTestAttribute(wnd, DF_HASBORDER))
                    DfRepaintBorder(wnd, (DFRECT *)p1);
                else if (DfTestAttribute(wnd, DF_HASTITLEBAR))
                    DfDisplayTitle(wnd, (DFRECT *)p1);
            }
            break;
        case DFM_COMMAND:
            CommandMsg(wnd, p1);
            break;
        case DFM_SETFOCUS:
            SetFocusMsg(wnd, p1);
            break;
        case DFM_DOUBLE_CLICK:
            DoubleClickMsg(wnd, p1, p2);
            break;
        case DFM_LEFT_BUTTON:
            LeftButtonMsg(wnd, p1, p2);
            break;
        case MOUSE_MOVED:
            if (MouseMovedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_BUTTON_RELEASED:
            if (DfWindowMoving || DfWindowSizing)
            {
                if (DfWindowMoving)
                    DfPostMessage(wnd,DFM_MOVE,dwnd.rc.lf,dwnd.rc.tp);
                else
                    DfPostMessage(wnd, DFM_DFM_SIZE,dwnd.rc.rt,dwnd.rc.bt);
                TerminateMoveSize();
            }
            break;
#ifdef INCLUDE_MAXIMIZE
        case DFM_MAXIMIZE:
            if (wnd->condition != DF_ISMAXIMIZED)
                MaximizeMsg(wnd);
            break;
#endif
#ifdef INCLUDE_MINIMIZE
        case DFM_MINIMIZE:
            if (wnd->condition != DF_ISMINIMIZED)
                MinimizeMsg(wnd);
            break;
#endif
#ifdef INCLUDE_RESTORE
        case DFM_RESTORE:
            if (wnd->condition != DF_SRESTORED)    {
#ifdef INCLUDE_MAXIMIZE
                if (wnd->oldcondition == DF_ISMAXIMIZED)
                    DfSendMessage(wnd, DFM_MAXIMIZE, 0, 0);
                else
#endif
                    RestoreMsg(wnd);
            }
            break;
#endif
        case DFM_MOVE:
            MoveMsg(wnd, p1, p2);
            break;
        case DFM_DFM_SIZE:    {
            SizeMsg(wnd, p1, p2);
            break;
        }
        case DFM_CLOSE_WINDOW:
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
    DfRectLeft(rc) = DfRectRight(prc) - DF_ICONWIDTH;
    DfRectTop(rc) = DfRectBottom(prc) - DF_ICONHEIGHT;
    DfRectRight(rc) = DfRectLeft(rc)+DF_ICONWIDTH-1;
    DfRectBottom(rc) = DfRectTop(rc)+DF_ICONHEIGHT-1;
    return rc;
}
/* ----- compute a position for a minimized window icon ---- */
static DFRECT PositionIcon(DFWINDOW wnd)
{
	DFWINDOW pwnd = DfGetParent(wnd);
    DFRECT rc;
    DfRectLeft(rc) = DfGetScreenWidth()-DF_ICONWIDTH;
    DfRectTop(rc) = DfGetScreenHeight()-DF_ICONHEIGHT;
    DfRectRight(rc) = DfGetScreenWidth()-1;
    DfRectBottom(rc) = DfGetScreenHeight()-1;
    if (pwnd != NULL)    {
        DFRECT prc = DfWindowRect(pwnd);
		DFWINDOW cwnd = DfFirstWindow(pwnd);
        rc = LowerRight(prc);
        /* - search for icon available location - */
		while (cwnd != NULL)	{
            if (cwnd->condition == DF_ISMINIMIZED)    {
                DFRECT rc1;
                rc1 = DfWindowRect(cwnd);
                if (DfRectLeft(rc1) == DfRectLeft(rc) &&
                        DfRectTop(rc1) == DfRectTop(rc))    {
                    DfRectLeft(rc) -= DF_ICONWIDTH;
                    DfRectRight(rc) -= DF_ICONWIDTH;
                    if (DfRectLeft(rc) < DfRectLeft(prc)+1)   {
                        DfRectLeft(rc) =
                            DfRectRight(prc)-DF_ICONWIDTH;
                        DfRectRight(rc) =
                            DfRectLeft(rc)+DF_ICONWIDTH-1;
                        DfRectTop(rc) -= DF_ICONHEIGHT;
                        DfRectBottom(rc) -= DF_ICONHEIGHT;
                        if (DfRectTop(rc) < DfRectTop(prc)+1)
                            return LowerRight(prc);
                    }
                    break;
                }
            }
			cwnd = DfNextWindow(cwnd);
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
    DfSendMessage(&dwnd, DFM_RELEASE_MOUSE, TRUE, 0);
    DfSendMessage(&dwnd, DFM_RELEASE_KEYBOARD, TRUE, 0);
    RestoreBorder(dwnd.rc);
    DfWindowMoving = DfWindowSizing = FALSE;
}
/* ---- build a dummy window border for moving or sizing --- */
static void dragborder(DFWINDOW wnd, int x, int y)
{
    RestoreBorder(dwnd.rc);
    /* ------- build the dummy window -------- */
    dwnd.rc.lf = x;
    dwnd.rc.tp = y;
    dwnd.rc.rt = dwnd.rc.lf+DfWindowWidth(wnd)-1;
    dwnd.rc.bt = dwnd.rc.tp+DfWindowHeight(wnd)-1;
    dwnd.ht = DfWindowHeight(wnd);
    dwnd.wd = DfWindowWidth(wnd);
    dwnd.parent = DfGetParent(wnd);
    dwnd.attrib = DF_VISIBLE | DF_HASBORDER | DF_NOCLIP;
    DfInitWindowColors(&dwnd);
    SaveBorder(dwnd.rc);
    DfRepaintBorder(&dwnd, NULL);
}
/* ---- write the dummy window border for sizing ---- */
static void sizeborder(DFWINDOW wnd, int rt, int bt)
{
    int leftmost = DfGetLeft(wnd)+10;
    int topmost = DfGetTop(wnd)+3;
    int bottommost = DfGetScreenHeight()-1;
    int rightmost  = DfGetScreenWidth()-1;
    if (DfGetParent(wnd))    {
        bottommost = min(bottommost,
            DfGetClientBottom(DfGetParent(wnd)));
        rightmost  = min(rightmost,
            DfGetClientRight(DfGetParent(wnd)));
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
        DfRepaintBorder(&dwnd, NULL);
    }
}
#ifdef INCLUDE_MULTI_WINDOWS
/* ----- adjust a rectangle to include the shadow ----- */
static DFRECT adjShadow(DFWINDOW wnd)
{
    DFRECT rc;
    rc = wnd->rc;
    if (DfTestAttribute(wnd, DF_SHADOW))    {
        if (DfRectRight(rc) < DfGetScreenWidth()-1)
            DfRectRight(rc)++;
        if (DfRectBottom(rc) < DfGetScreenHeight()-1)
            DfRectBottom(rc)++;
    }
    return rc;
}
/* --- repaint a rectangular subsection of a window --- */
static void PaintOverLap(DFWINDOW wnd, DFRECT rc)
{
    if (DfIsVisible(wnd))    {
        int isBorder, isTitle, isData;
        isBorder = isTitle = FALSE;
        isData = TRUE;
        if (DfTestAttribute(wnd, DF_HASBORDER))    {
            isBorder =  DfRectLeft(rc) == 0 &&
                        DfRectTop(rc) < DfWindowHeight(wnd);
            isBorder |= DfRectLeft(rc) < DfWindowWidth(wnd) &&
                        DfRectRight(rc) >= DfWindowWidth(wnd)-1 &&
                        DfRectTop(rc) < DfWindowHeight(wnd);
            isBorder |= DfRectTop(rc) == 0 &&
                        DfRectLeft(rc) < DfWindowWidth(wnd);
            isBorder |= DfRectTop(rc) < DfWindowHeight(wnd) &&
                        DfRectBottom(rc) >= DfWindowHeight(wnd)-1 &&
                        DfRectLeft(rc) < DfWindowWidth(wnd);
        }
        else if (DfTestAttribute(wnd, DF_HASTITLEBAR))
            isTitle = DfRectTop(rc) == 0 &&
                      DfRectRight(rc) > 0 &&
                      DfRectLeft(rc)<DfWindowWidth(wnd)-DfBorderAdj(wnd);

        if (DfRectLeft(rc) >= DfWindowWidth(wnd)-DfBorderAdj(wnd))
            isData = FALSE;
        if (DfRectTop(rc) >= DfWindowHeight(wnd)-DfBottomBorderAdj(wnd))
            isData = FALSE;
        if (DfTestAttribute(wnd, DF_HASBORDER))    {
            if (DfRectRight(rc) == 0)
                isData = FALSE;
            if (DfRectBottom(rc) == 0)
                isData = FALSE;
        }
        if (DfTestAttribute(wnd, DF_SHADOW))
            isBorder |= DfRectRight(rc) == DfWindowWidth(wnd) ||
                        DfRectBottom(rc) == DfWindowHeight(wnd);
        if (isData)
            DfSendMessage(wnd, DFM_PAINT, (DF_PARAM) &rc, TRUE);
        if (isBorder)
            DfSendMessage(wnd, DFM_BORDER, (DF_PARAM) &rc, 0);
        else if (isTitle)
            DfDisplayTitle(wnd, &rc);
    }
}
/* ------ paint the part of a window that is overlapped
            by another window that is being hidden ------- */
static void PaintOver(DFWINDOW wnd)
{
    DFRECT wrc, rc;
    wrc = adjShadow(HiddenWindow);
    rc = adjShadow(wnd);
    rc = DfSubRectangle(rc, wrc);
    if (DfValidRect(rc))
        PaintOverLap(wnd, DfRelativeWindowRect(wnd, rc));
}
/* --- paint the overlapped parts of all children --- */
static void PaintOverChildren(DFWINDOW pwnd)
{
    DFWINDOW cwnd = DfFirstWindow(pwnd);
    while (cwnd != NULL)    {
        if (cwnd != HiddenWindow)    {
            PaintOver(cwnd);
            PaintOverChildren(cwnd);
        }
        cwnd = DfNextWindow(cwnd);
    }
}
/* -- recursive overlapping paint of parents -- */
static void PaintOverParents(DFWINDOW wnd)
{
    DFWINDOW pwnd = DfGetParent(wnd);
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
    DFWINDOW hwnd = DfNextWindow(wnd);
    while (hwnd != NULL)    {
        /* ------- test only at document window level ------ */
        DFWINDOW pwnd = DfGetParent(hwnd);
/*        if (pwnd == NULL || DfGetClass(pwnd) == DF_APPLICATION)  */  {
            /* ---- don't bother testing self ----- */
            if (DfIsVisible(hwnd) && hwnd != wnd)    {
                /* --- see if other window is descendent --- */
                while (pwnd != NULL)    {
                    if (pwnd == wnd)
                        break;
                    pwnd = DfGetParent(pwnd);
                }
                /* ----- don't test descendent overlaps ----- */
                if (pwnd == NULL)    {
                    /* -- see if other window is ancestor --- */
                    pwnd = DfGetParent(wnd);
                    while (pwnd != NULL)    {
                        if (pwnd == hwnd)
                            break;
                        pwnd = DfGetParent(pwnd);
                    }
                    /* --- don't test ancestor overlaps --- */
                    if (pwnd == NULL)    {
                        HiddenWindow = DfGetAncestor(hwnd);
                        DfClearVisible(HiddenWindow);
                        PaintOver(wnd);
                        DfSetVisible(HiddenWindow);
                    }
                }
            }
        }
        hwnd = DfNextWindow(hwnd);
    }
    /* --------- repaint all children of this window
        the same way ----------- */
    hwnd = DfFirstWindow(wnd);
    while (hwnd != NULL)    {
        PaintUnderLappers(hwnd);
        hwnd = DfNextWindow(hwnd);
    }
}
#endif /* #ifdef INCLUDE_MULTI_WINDOWS */

/* --- save video area to be used by dummy window border --- */
static void SaveBorder(DFRECT rc)
{
	Bht = DfRectBottom(rc) - DfRectTop(rc) + 1;
	Bwd = DfRectRight(rc) - DfRectLeft(rc) + 1;
	Bsave = DfRealloc(Bsave, Bht * Bwd * sizeof(CHAR_INFO));

	DfGetVideo(rc,Bsave);
}
/* ---- restore video area used by dummy window border ---- */
static void RestoreBorder(DFRECT rc)
{
	if (Bsave != NULL)
	{
		DfStoreVideo(rc, Bsave);
		free(Bsave);
		Bsave = NULL;
	}
}
/* ----- test if screen coordinates are in a window ---- */
static BOOL InsideWindow(DFWINDOW wnd, int x, int y)
{
    DFRECT rc;
    rc = DfWindowRect(wnd);
    if (!DfTestAttribute(wnd, DF_NOCLIP))
    {
        DFWINDOW pwnd = DfGetParent(wnd);
        while (pwnd != NULL)
		{
            rc = DfSubRectangle(rc, DfClientRect(pwnd));
            pwnd = DfGetParent(pwnd);
        }
    }
    return DfInsideRect(x, y, rc);
}

BOOL DfIsDerivedFrom(DFWINDOW wnd, DFCLASS class)
{
    DFCLASS tclass = DfGetClass(wnd);
    while (tclass != -1)    {
        if (tclass == class)
            return TRUE;
        tclass = (DfClassDefs[tclass].base);
    }
    return FALSE;
}

/* -- find the oldest document window ancestor of a window -- */
DFWINDOW DfGetAncestor(DFWINDOW wnd)
{
    if (wnd != NULL)    {
        while (DfGetParent(wnd) != NULL)    {
            if (DfGetClass(DfGetParent(wnd)) == DF_APPLICATION)
                break;
            wnd = DfGetParent(wnd);
        }
    }
    return wnd;
}

BOOL DfIsVisible(DFWINDOW wnd)
{
    while (wnd != NULL)    {
        if (isHidden(wnd))
            return FALSE;
        wnd = DfGetParent(wnd);
    }
    return TRUE;
}

/* -- adjust a window's rectangle to clip it to its parent - */
static DFRECT ClipRect(DFWINDOW wnd)
{
    DFRECT rc;
    rc = DfWindowRect(wnd);
    if (DfTestAttribute(wnd, DF_SHADOW))    {
        DfRectBottom(rc)++;
        DfRectRight(rc)++;
    }
	return DfClipRectangle(wnd, rc);
}

/* -- get the video memory that is to be used by a window -- */
static void GetVideoBuffer(DFWINDOW wnd)
{
    DFRECT rc;
    int ht;
    int wd;

    rc = ClipRect(wnd);
    ht = DfRectBottom(rc) - DfRectTop(rc) + 1;
    wd = DfRectRight(rc) - DfRectLeft(rc) + 1;
    wnd->videosave = DfRealloc(wnd->videosave, (ht * wd * sizeof(CHAR_INFO)));
    DfGetVideo(rc, wnd->videosave);
}

/* -- put the video memory that is used by a window -- */
static void PutVideoBuffer(DFWINDOW wnd)
{
	if (wnd->videosave != NULL)
	{
		DFRECT rc;
		rc = ClipRect(wnd);
		DfStoreVideo(rc, wnd->videosave);
		free(wnd->videosave);
		wnd->videosave = NULL;
	}
}

/* ------- return TRUE if awnd is an ancestor of wnd ------- */
BOOL DfIsAncestor(DFWINDOW wnd, DFWINDOW awnd)
{
	while (wnd != NULL)	{
		if (wnd == awnd)
			return TRUE;
		wnd = DfGetParent(wnd);
	}
	return FALSE;
}

/* EOF */
