/* --------- message.c ---------- */

#include "dflat.h"
#include "system.h"

static int handshaking = 0;

BOOL AllocTesting = FALSE;
jmp_buf AllocError;
BOOL AltDown = FALSE;

/* ---------- event queue ---------- */
static struct events
{
	DFMESSAGE event;
	int mx;
	int my;
} EventQueue[DF_MAXMESSAGES];

/* ---------- message queue --------- */
static struct msgs
{
	DFWINDOW wnd;
	DFMESSAGE msg;
	DF_PARAM p1;
	DF_PARAM p2;
} MsgQueue[DF_MAXMESSAGES];

static int EventQueueOnCtr;
static int EventQueueOffCtr;
static int EventQueueCtr;

static int MsgQueueOnCtr;
static int MsgQueueOffCtr;
static int MsgQueueCtr;


DFWINDOW DfCaptureMouse;
DFWINDOW DfCaptureKeyboard;
static BOOL NoChildCaptureMouse;
static BOOL NoChildCaptureKeyboard;

//static int doubletimer = -1;
//static int delaytimer  = -1;
static int clocktimer  = -1;

static DFWINDOW Cwnd;

//static char ermsg[] = "Error accessing drive x";


static void StopMsg(void)
{
	DfClearClipboard();
	DfClearDialogBoxes();
	DfRestoreCursor();
	DfUnhideCursor();
}

SHORT DfGetScreenHeight (void)
{
	return DfScreenHeight;
}

SHORT DfGetScreenWidth (void)
{
	return DfScreenWidth;
}

/* ------------ initialize the message system --------- */
BOOL DfInitialize (VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	AllocTesting = TRUE;
	if (setjmp(AllocError) != 0)
	{
		StopMsg();
		return FALSE;
	}

	/* get input and output handles */
	DfInput = GetStdHandle (STD_INPUT_HANDLE);
	DfOutput = GetStdHandle (STD_OUTPUT_HANDLE);

	/* get screen size */
	GetConsoleScreenBufferInfo (DfOutput, &csbi);
	DfScreenHeight = (csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
	DfScreenWidth = (csbi.srWindow.Right - csbi.srWindow.Left) + 1;

	/* enable mouse events */
	SetConsoleMode (DfInput, ENABLE_MOUSE_INPUT);

	DfSaveCursor();
	DfHideCursor();

	DfCaptureMouse = NULL;
	DfCaptureKeyboard = NULL;
	NoChildCaptureMouse = FALSE;
	NoChildCaptureKeyboard = FALSE;
	MsgQueueOnCtr = 0;
	MsgQueueOffCtr = 0;
	MsgQueueCtr = 0;
	EventQueueOnCtr = 0;
	EventQueueOffCtr = 0;
	EventQueueCtr = 0;
	DfPostMessage (NULL, DFM_START, 0, 0);

	return TRUE;
}


void DfTerminate (void)
{

}

/* ----- post an event and parameters to event queue ---- */
static void PostEvent(DFMESSAGE event, int p1, int p2)
{
    if (EventQueueCtr != DF_MAXMESSAGES)    {
        EventQueue[EventQueueOnCtr].event = event;
        EventQueue[EventQueueOnCtr].mx = p1;
        EventQueue[EventQueueOnCtr].my = p2;
        if (++EventQueueOnCtr == DF_MAXMESSAGES)
            EventQueueOnCtr = 0;
        EventQueueCtr++;
    }
}

/* ------ collect mouse, clock, and keyboard events ----- */
static void collect_events(void)
{
	static int OldShiftKeys = 0;
	int sk = 0;

#ifdef TIMER_AVAILABLE
	static BOOL flipflop = FALSE;
	static char timestr[9];
	struct tm *now;
	int hr;
#endif

	HANDLE DfInput = GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD ir;
	DWORD dwRead;
	int c;

#ifdef TIMER_AVAILABLE
    /* -------- test for a clock event (one/second) ------- */
    if (DfTimedOut(clocktimer))
    {
        /* ----- get the current time ----- */
        time_t t = time(NULL);
        now = localtime(&t);
        hr = now->tm_hour > 12 ?
             now->tm_hour - 12 :
             now->tm_hour;
        if (hr == 0)
            hr = 12;
        sprintf(timestr, "%2d:%02d", hr, now->tm_min);
        strcpy(timestr+5, now->tm_hour > 11 ? "pm " : "am ");
        /* ------- blink the : at one-second intervals ----- */
        if (flipflop)
            *(timestr+2) = ' ';
        flipflop ^= TRUE;
        /* -------- reset the timer -------- */
        DfSetTimer(clocktimer, 1);
        /* -------- post the clock event -------- */
        PostEvent(DFM_CLOCKTICK, (DF_PARAM)timestr, 0);
    }
#endif

//	WaitForSingleObject (DfInput, INFINITE);
	ReadConsoleInput (DfInput, &ir, 1, &dwRead);

	if ((ir.EventType == KEY_EVENT) &&
	    (ir.Event.KeyEvent.bKeyDown == TRUE))
	{
		/* handle key down events */

		/* handle shift state changes */
		if (ir.Event.KeyEvent.dwControlKeyState &
		    (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
		{
			sk |= DF_ALTKEY;
		}
		if (ir.Event.KeyEvent.dwControlKeyState &
		    (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
		{
			sk |= DF_CTRLKEY;
		}
		if (ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
		{
			sk |= DF_LEFTSHIFT + DF_RIGHTSHIFT;
		}

		if (sk != OldShiftKeys)
		{
			OldShiftKeys = sk;
			/* the shift status changed */
			PostEvent(DFM_SHIFT_CHANGED, sk, 0);
#if 0
			if (sk & DF_ALTKEY)
				AltDown = TRUE;
			else
				AltDown = FALSE;
#endif
		}

		if (ir.Event.KeyEvent.uChar.AsciiChar == 0)
		{
			switch (ir.Event.KeyEvent.wVirtualKeyCode)
			{
				case VK_F1:
					c = DF_F1;
					break;

				case VK_F4:
					if (sk & DF_ALTKEY)
						c = DF_ALT_F4;
					else if (sk & DF_CTRLKEY)
						c = DF_CTRL_F4;
					else
						c = DF_F4;

				case VK_F10:
					c = DF_F10;
					break;

				case VK_UP:
					c = DF_UP;
					break;

				case VK_DOWN:
					c = DF_DN;
					break;

				case VK_LEFT:
					c = DF_BS;
					break;

				case VK_RIGHT:
					c = DF_FWD;
					break;

				case VK_INSERT:
					c = DF_INS;
					break;

				case VK_DELETE:
					c = DF_DEL;
					break;

				case VK_HOME:
					c = DF_HOME;
					break;

				case VK_END:
					c = DF_END;
					break;

				case VK_PRIOR:
					c = DF_PGUP;
					break;

				case VK_NEXT:
					c = DF_PGDN;
					break;

				default:
					return;
			}
		}
		else
		{
			/* special handling of SHIFT+TAB */
			if (ir.Event.KeyEvent.uChar.AsciiChar == VK_TAB &&
			    (ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED))
				c = DF_SHIFT_HT;
			else
				c = ir.Event.KeyEvent.uChar.AsciiChar;
		}

		PostEvent (DFM_KEYBOARD, c, sk);
	}
	else if (ir.EventType == MOUSE_EVENT)
	{
		/* handle mouse events */
		if (ir.Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
		{
			PostEvent (MOUSE_MOVED,
			           ir.Event.MouseEvent.dwMousePosition.X,
			           ir.Event.MouseEvent.dwMousePosition.Y);
		}
		else if (ir.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)
		{
			if (ir.Event.MouseEvent.dwButtonState ==
			    FROM_LEFT_1ST_BUTTON_PRESSED)
			{
				PostEvent (DOUBLE_CLICK,
				           ir.Event.MouseEvent.dwMousePosition.X,
				           ir.Event.MouseEvent.dwMousePosition.Y);
			}
		}
		else if (ir.Event.MouseEvent.dwEventFlags == 0)
		{
			/* single click */
			if (ir.Event.MouseEvent.dwButtonState ==
			    FROM_LEFT_1ST_BUTTON_PRESSED)
			{
				PostEvent (DFM_LEFT_BUTTON,
				           ir.Event.MouseEvent.dwMousePosition.X,
				           ir.Event.MouseEvent.dwMousePosition.Y);
			}
			else if (ir.Event.MouseEvent.dwButtonState ==
			         RIGHTMOST_BUTTON_PRESSED)
			{
				PostEvent (DFM_RIGHT_BUTTON,
				           ir.Event.MouseEvent.dwMousePosition.X,
				           ir.Event.MouseEvent.dwMousePosition.Y);
			}
			else if (ir.Event.MouseEvent.dwButtonState == 0)
			{
				PostEvent (DFM_BUTTON_RELEASED,
						   ir.Event.MouseEvent.dwMousePosition.X,
						   ir.Event.MouseEvent.dwMousePosition.Y);
			}
		}
	}
}


/* ----- post a message and parameters to msg queue ---- */
void DfPostMessage(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
	if (msg == DFM_ENDDIALOG)
	{
		msg++;
		--msg;
	}

	if (MsgQueueCtr != DF_MAXMESSAGES)
	{
		MsgQueue[MsgQueueOnCtr].wnd = wnd;
		MsgQueue[MsgQueueOnCtr].msg = msg;
		MsgQueue[MsgQueueOnCtr].p1 = p1;
		MsgQueue[MsgQueueOnCtr].p2 = p2;
		if (++MsgQueueOnCtr == DF_MAXMESSAGES)
			MsgQueueOnCtr = 0;
		MsgQueueCtr++;
	}
}

/* --------- send a message to a window ----------- */
int DfSendMessage(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn = TRUE, x, y;

#ifdef INCLUDE_LOGGING
	DfLogMessages(wnd, msg, p1, p2);
#endif
    if (wnd != NULL)
        switch (msg)    {
            case DFM_PAINT:
            case DFM_BORDER:
                /* ------- don't send these messages unless the
                    window is visible -------- */
                if (DfIsVisible(wnd))
	                rtn = (*wnd->wndproc)(wnd, msg, p1, p2);
                break;
            case DFM_RIGHT_BUTTON:
            case DFM_LEFT_BUTTON:
            case DOUBLE_CLICK:
            case DFM_BUTTON_RELEASED:
                /* --- don't send these messages unless the
                    window is visible or has captured the mouse -- */
                if (DfIsVisible(wnd) || wnd == DfCaptureMouse)
	                rtn = (*wnd->wndproc)(wnd, msg, p1, p2);
                break;
            case DFM_KEYBOARD:
            case DFM_SHIFT_CHANGED:
                /* ------- don't send these messages unless the
                    window is visible or has captured the keyboard -- */
                if (!(DfIsVisible(wnd) || wnd == DfCaptureKeyboard))
	                break;
            default:
                rtn = (*wnd->wndproc)(wnd, msg, p1, p2);
                break;
        }
    /* ----- window processor returned true or the message was sent
        to no window at all (NULL) ----- */
    if (rtn != FALSE)    {
        /* --------- process messages that a window sends to the
            system itself ---------- */
        switch (msg)    {
            case DFM_STOP:
				StopMsg();
                break;
            /* ------- clock messages --------- */
            case DFM_CAPTURE_CLOCK:
                Cwnd = wnd;
                DfSetTimer(clocktimer, 0);
                break;
            case DFM_RELEASE_CLOCK:
                Cwnd = NULL;
                DfDisableTimer(clocktimer);
                break;
            /* -------- keyboard messages ------- */
            case DFM_KEYBOARD_CURSOR:
                if (wnd == NULL)
                    DfCursor((int)p1, (int)p2);
                else if (wnd == DfInFocus)
                    DfCursor(DfGetClientLeft(wnd)+(int)p1,
                                DfGetClientTop(wnd)+(int)p2);
                break;
            case DFM_CAPTURE_KEYBOARD:
                if (p2)
                    ((DFWINDOW)p2)->PrevKeyboard=DfCaptureKeyboard;
                else
                    wnd->PrevKeyboard = DfCaptureKeyboard;
                DfCaptureKeyboard = wnd;
                NoChildCaptureKeyboard = (int)p1;
                break;
			case DFM_RELEASE_KEYBOARD:
				if (wnd != NULL)
				{
					if (DfCaptureKeyboard == wnd || (int)p1)
						DfCaptureKeyboard = wnd->PrevKeyboard;
					else
					{
						DFWINDOW twnd = DfCaptureKeyboard;
						while (twnd != NULL)
						{
							if (twnd->PrevKeyboard == wnd)
							{
								twnd->PrevKeyboard = wnd->PrevKeyboard;
								break;
							}
							twnd = twnd->PrevKeyboard;
						}
						if (twnd == NULL)
							DfCaptureKeyboard = NULL;
					}
					wnd->PrevKeyboard = NULL;
				}
				else
					DfCaptureKeyboard = NULL;
				NoChildCaptureKeyboard = FALSE;
				break;
            case DFM_CURRENT_KEYBOARD_CURSOR:
                DfCurrCursor(&x, &y);
                *(int*)p1 = x;
                *(int*)p2 = y;
                break;
            case DFM_SAVE_CURSOR:
                DfSaveCursor();
                break;
            case DFM_RESTORE_CURSOR:
                DfRestoreCursor();
                break;
            case DFM_HIDE_CURSOR:
                DfNormalCursor();
                DfHideCursor();
                break;
            case DFM_SHOW_CURSOR:
                if (p1)
                    DfSetCursorSize(100);
                else
                    DfSetCursorSize(5);
                DfUnhideCursor();
                break;

			case DFM_CAPTURE_MOUSE:
				if (p2)
					((DFWINDOW)p2)->PrevMouse = DfCaptureMouse;
				else
					wnd->PrevMouse = DfCaptureMouse;
				DfCaptureMouse = wnd;
				NoChildCaptureMouse = (int)p1;
				break;

			case DFM_RELEASE_MOUSE:
				if (wnd != NULL)
				{
					if (DfCaptureMouse == wnd || (int)p1)
						DfCaptureMouse = wnd->PrevMouse;
					else
					{
						DFWINDOW twnd = DfCaptureMouse;
						while (twnd != NULL)
						{
							if (twnd->PrevMouse == wnd)
							{
								twnd->PrevMouse = wnd->PrevMouse;
								break;
							}
							twnd = twnd->PrevMouse;
						}
						if (twnd == NULL)
							DfCaptureMouse = NULL;
					}
					wnd->PrevMouse = NULL;
				}
				else
					DfCaptureMouse = NULL;
				NoChildCaptureMouse = FALSE;
				break;

			default:
				break;
		}
	}
	return rtn;
}

static DFRECT VisibleRect(DFWINDOW wnd)
{
	DFRECT rc = DfWindowRect(wnd);
	if (!DfTestAttribute(wnd, DF_NOCLIP))
	{
		DFWINDOW pwnd = DfGetParent(wnd);
		DFRECT prc;
		prc = DfClientRect(pwnd);
		while (pwnd != NULL)
		{
			if (DfTestAttribute(pwnd, DF_NOCLIP))
				break;
			rc = DfSubRectangle(rc, prc);
			if (!DfValidRect(rc))
				break;
			if ((pwnd = DfGetParent(pwnd)) != NULL)
				prc = DfClientRect(pwnd);
		}
	}
	return rc;
}

/* ----- find window that mouse coordinates are in --- */
static DFWINDOW inWindow(DFWINDOW wnd, int x, int y)
{
	DFWINDOW Hit = NULL;
	while (wnd != NULL)	{
		if (DfIsVisible(wnd))	{
			DFWINDOW wnd1;
			DFRECT rc = VisibleRect(wnd);
			if (DfInsideRect(x, y, rc))
				Hit = wnd;
			if ((wnd1 = inWindow(DfLastWindow(wnd), x, y)) != NULL)
				Hit = wnd1;
			if (Hit != NULL)
				break;
		}
		wnd = DfPrevWindow(wnd);
	}
	return Hit;
}

static DFWINDOW MouseWindow(int x, int y)
{
	/* get the window in which a mouse event occurred */
	DFWINDOW Mwnd = inWindow(DfApplicationWindow, x, y);

	/* ---- process mouse captures ----- */
	if (DfCaptureMouse != NULL)
	{
		if (NoChildCaptureMouse ||
		    Mwnd == NULL 	||
		    !DfIsAncestor(Mwnd, DfCaptureMouse))
			Mwnd = DfCaptureMouse;
	}
	return Mwnd;
}


void DfHandshake(void)
{
	handshaking++;
	DfDispatchMessage ();
	--handshaking;
}


/* ---- dispatch messages to the message proc function ---- */
BOOL DfDispatchMessage (void)
{
	DFWINDOW Mwnd, Kwnd;

	/* -------- collect mouse and keyboard events ------- */
	collect_events();

	/* --------- dequeue and process events -------- */
	while (EventQueueCtr > 0)
	{
		struct events ev;
			
		ev = EventQueue[EventQueueOffCtr];
		if (++EventQueueOffCtr == DF_MAXMESSAGES)
			EventQueueOffCtr = 0;
		--EventQueueCtr;

		/* get the window in which a keyboard event occurred */
		Kwnd = DfInFocus;

		/* process keyboard captures */
		if (DfCaptureKeyboard != NULL)
		{
			if (Kwnd == NULL ||
			    NoChildCaptureKeyboard ||
			    !DfIsAncestor(Kwnd, DfCaptureKeyboard))
				Kwnd = DfCaptureKeyboard;
		}

		/* send mouse and keyboard messages to the
		   window that should get them */
		switch (ev.event)
		{
			case DFM_SHIFT_CHANGED:
			case DFM_KEYBOARD:
				if (!handshaking)
					DfSendMessage(Kwnd, ev.event, ev.mx, ev.my);
				break;

			case DFM_LEFT_BUTTON:
				if (!handshaking)
				{
					Mwnd = MouseWindow(ev.mx, ev.my);
					if (!DfCaptureMouse ||
					    (!NoChildCaptureMouse &&
					     DfIsAncestor(Mwnd, DfCaptureMouse)))
					{
						if (Mwnd != DfInFocus)
							DfSendMessage(Mwnd, DFM_SETFOCUS, TRUE, 0);
						DfSendMessage(Mwnd, DFM_LEFT_BUTTON, ev.mx, ev.my);
					}
				}
				break;

			case DFM_BUTTON_RELEASED:
			case DOUBLE_CLICK:
			case DFM_RIGHT_BUTTON:
				if (handshaking)
					break;

			case MOUSE_MOVED:
				Mwnd = MouseWindow(ev.mx, ev.my);
				DfSendMessage(Mwnd, ev.event, ev.mx, ev.my);
				break;

			case DFM_CLOCKTICK:
				DfSendMessage(Cwnd, ev.event, ev.mx, ev.my);
				break;

			default:
				break;
		}
	}

	/* ------ dequeue and process messages ----- */
	while (MsgQueueCtr > 0)
	{
		struct msgs mq;

		mq = MsgQueue[MsgQueueOffCtr];

		if (++MsgQueueOffCtr == DF_MAXMESSAGES)
			MsgQueueOffCtr = 0;
		--MsgQueueCtr;

		DfSendMessage (mq.wnd, mq.msg, mq.p1, mq.p2);
		if (mq.msg == DFM_ENDDIALOG)
			return FALSE;

		if (mq.msg == DFM_STOP)
			return FALSE;
	}

	return TRUE;
}

/* EOF */
