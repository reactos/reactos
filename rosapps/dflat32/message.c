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
} EventQueue[MAXMESSAGES];

/* ---------- message queue --------- */
static struct msgs
{
	DFWINDOW wnd;
	DFMESSAGE msg;
	PARAM p1;
	PARAM p2;
} MsgQueue[MAXMESSAGES];

static int EventQueueOnCtr;
static int EventQueueOffCtr;
static int EventQueueCtr;

static int MsgQueueOnCtr;
static int MsgQueueOffCtr;
static int MsgQueueCtr;


DFWINDOW CaptureMouse;
DFWINDOW CaptureKeyboard;
static BOOL NoChildCaptureMouse;
static BOOL NoChildCaptureKeyboard;

//static int doubletimer = -1;
//static int delaytimer  = -1;
static int clocktimer  = -1;

static DFWINDOW Cwnd;

//static char ermsg[] = "Error accessing drive x";


static void StopMsg(void)
{
	ClearClipboard();
	ClearDialogBoxes();
	restorecursor();
	unhidecursor();
}

SHORT DfGetScreenHeight (void)
{
	return sScreenHeight;
}

SHORT DfGetScreenWidth (void)
{
	return sScreenWidth;
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
	hInput = GetStdHandle (STD_INPUT_HANDLE);
	hOutput = GetStdHandle (STD_OUTPUT_HANDLE);

	/* get screen size */
	GetConsoleScreenBufferInfo (hOutput, &csbi);
	sScreenHeight = (csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
	sScreenWidth = (csbi.srWindow.Right - csbi.srWindow.Left) + 1;

	/* enable mouse events */
	SetConsoleMode (hInput, ENABLE_MOUSE_INPUT);

	savecursor();
	hidecursor();

	CaptureMouse = NULL;
	CaptureKeyboard = NULL;
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
    if (EventQueueCtr != MAXMESSAGES)    {
        EventQueue[EventQueueOnCtr].event = event;
        EventQueue[EventQueueOnCtr].mx = p1;
        EventQueue[EventQueueOnCtr].my = p2;
        if (++EventQueueOnCtr == MAXMESSAGES)
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

	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD ir;
	DWORD dwRead;
	int c;

#ifdef TIMER_AVAILABLE
    /* -------- test for a clock event (one/second) ------- */
    if (timed_out(clocktimer))
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
        set_timer(clocktimer, 1);
        /* -------- post the clock event -------- */
        PostEvent(CLOCKTICK, (PARAM)timestr, 0);
    }
#endif

//	WaitForSingleObject (hInput, INFINITE);
	ReadConsoleInput (hInput, &ir, 1, &dwRead);

	if ((ir.EventType == KEY_EVENT) &&
	    (ir.Event.KeyEvent.bKeyDown == TRUE))
	{
		/* handle key down events */

		/* handle shift state changes */
		if (ir.Event.KeyEvent.dwControlKeyState &
		    (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
		{
			sk |= ALTKEY;
		}
		if (ir.Event.KeyEvent.dwControlKeyState &
		    (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
		{
			sk |= CTRLKEY;
		}
		if (ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
		{
			sk |= LEFTSHIFT + RIGHTSHIFT;
		}

		if (sk != OldShiftKeys)
		{
			OldShiftKeys = sk;
			/* the shift status changed */
			PostEvent(SHIFT_CHANGED, sk, 0);
#if 0
			if (sk & ALTKEY)
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
					c = F1;
					break;

				case VK_F4:
					if (sk & ALTKEY)
						c = ALT_F4;
					else if (sk & CTRLKEY)
						c = CTRL_F4;
					else
						c = F4;

				case VK_F10:
					c = F10;
					break;

				case VK_UP:
					c = UP;
					break;

				case VK_DOWN:
					c = DN;
					break;

				case VK_LEFT:
					c = BS;
					break;

				case VK_RIGHT:
					c = FWD;
					break;

				case VK_INSERT:
					c = INS;
					break;

				case VK_DELETE:
					c = DEL;
					break;

				case VK_HOME:
					c = HOME;
					break;

				case VK_END:
					c = END;
					break;

				case VK_PRIOR:
					c = PGUP;
					break;

				case VK_NEXT:
					c = PGDN;
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
				c = SHIFT_HT;
			else
				c = ir.Event.KeyEvent.uChar.AsciiChar;
		}

		PostEvent (KEYBOARD, c, sk);
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
				PostEvent (LEFT_BUTTON,
				           ir.Event.MouseEvent.dwMousePosition.X,
				           ir.Event.MouseEvent.dwMousePosition.Y);
			}
			else if (ir.Event.MouseEvent.dwButtonState ==
			         RIGHTMOST_BUTTON_PRESSED)
			{
				PostEvent (RIGHT_BUTTON,
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
void DfPostMessage(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
	if (msg == ENDDIALOG)
	{
		msg++;
		--msg;
	}

	if (MsgQueueCtr != MAXMESSAGES)
	{
		MsgQueue[MsgQueueOnCtr].wnd = wnd;
		MsgQueue[MsgQueueOnCtr].msg = msg;
		MsgQueue[MsgQueueOnCtr].p1 = p1;
		MsgQueue[MsgQueueOnCtr].p2 = p2;
		if (++MsgQueueOnCtr == MAXMESSAGES)
			MsgQueueOnCtr = 0;
		MsgQueueCtr++;
	}
}

/* --------- send a message to a window ----------- */
int DfSendMessage(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn = TRUE, x, y;

#ifdef INCLUDE_LOGGING
	LogMessages(wnd, msg, p1, p2);
#endif
    if (wnd != NULL)
        switch (msg)    {
            case PAINT:
            case BORDER:
                /* ------- don't send these messages unless the
                    window is visible -------- */
                if (isVisible(wnd))
	                rtn = (*wnd->wndproc)(wnd, msg, p1, p2);
                break;
            case RIGHT_BUTTON:
            case LEFT_BUTTON:
            case DOUBLE_CLICK:
            case DFM_BUTTON_RELEASED:
                /* --- don't send these messages unless the
                    window is visible or has captured the mouse -- */
                if (isVisible(wnd) || wnd == CaptureMouse)
	                rtn = (*wnd->wndproc)(wnd, msg, p1, p2);
                break;
            case KEYBOARD:
            case SHIFT_CHANGED:
                /* ------- don't send these messages unless the
                    window is visible or has captured the keyboard -- */
                if (!(isVisible(wnd) || wnd == CaptureKeyboard))
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
            case CAPTURE_CLOCK:
                Cwnd = wnd;
                set_timer(clocktimer, 0);
                break;
            case RELEASE_CLOCK:
                Cwnd = NULL;
                disable_timer(clocktimer);
                break;
            /* -------- keyboard messages ------- */
            case KEYBOARD_CURSOR:
                if (wnd == NULL)
                    cursor((int)p1, (int)p2);
                else if (wnd == inFocus)
                    cursor(GetClientLeft(wnd)+(int)p1,
                                GetClientTop(wnd)+(int)p2);
                break;
            case CAPTURE_KEYBOARD:
                if (p2)
                    ((DFWINDOW)p2)->PrevKeyboard=CaptureKeyboard;
                else
                    wnd->PrevKeyboard = CaptureKeyboard;
                CaptureKeyboard = wnd;
                NoChildCaptureKeyboard = (int)p1;
                break;
			case RELEASE_KEYBOARD:
				if (wnd != NULL)
				{
					if (CaptureKeyboard == wnd || (int)p1)
						CaptureKeyboard = wnd->PrevKeyboard;
					else
					{
						DFWINDOW twnd = CaptureKeyboard;
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
							CaptureKeyboard = NULL;
					}
					wnd->PrevKeyboard = NULL;
				}
				else
					CaptureKeyboard = NULL;
				NoChildCaptureKeyboard = FALSE;
				break;
            case CURRENT_KEYBOARD_CURSOR:
                curr_cursor(&x, &y);
                *(int*)p1 = x;
                *(int*)p2 = y;
                break;
            case SAVE_CURSOR:
                savecursor();
                break;
            case RESTORE_CURSOR:
                restorecursor();
                break;
            case HIDE_CURSOR:
                normalcursor();
                hidecursor();
                break;
            case SHOW_CURSOR:
                if (p1)
                    set_cursor_size(100);
                else
                    set_cursor_size(5);
                unhidecursor();
                break;

			case CAPTURE_MOUSE:
				if (p2)
					((DFWINDOW)p2)->PrevMouse = CaptureMouse;
				else
					wnd->PrevMouse = CaptureMouse;
				CaptureMouse = wnd;
				NoChildCaptureMouse = (int)p1;
				break;

			case RELEASE_MOUSE:
				if (wnd != NULL)
				{
					if (CaptureMouse == wnd || (int)p1)
						CaptureMouse = wnd->PrevMouse;
					else
					{
						DFWINDOW twnd = CaptureMouse;
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
							CaptureMouse = NULL;
					}
					wnd->PrevMouse = NULL;
				}
				else
					CaptureMouse = NULL;
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
	DFRECT rc = WindowRect(wnd);
	if (!TestAttribute(wnd, NOCLIP))
	{
		DFWINDOW pwnd = GetParent(wnd);
		DFRECT prc;
		prc = ClientRect(pwnd);
		while (pwnd != NULL)
		{
			if (TestAttribute(pwnd, NOCLIP))
				break;
			rc = subRectangle(rc, prc);
			if (!ValidRect(rc))
				break;
			if ((pwnd = GetParent(pwnd)) != NULL)
				prc = ClientRect(pwnd);
		}
	}
	return rc;
}

/* ----- find window that mouse coordinates are in --- */
static DFWINDOW inWindow(DFWINDOW wnd, int x, int y)
{
	DFWINDOW Hit = NULL;
	while (wnd != NULL)	{
		if (isVisible(wnd))	{
			DFWINDOW wnd1;
			DFRECT rc = VisibleRect(wnd);
			if (InsideRect(x, y, rc))
				Hit = wnd;
			if ((wnd1 = inWindow(LastWindow(wnd), x, y)) != NULL)
				Hit = wnd1;
			if (Hit != NULL)
				break;
		}
		wnd = PrevWindow(wnd);
	}
	return Hit;
}

static DFWINDOW MouseWindow(int x, int y)
{
	/* get the window in which a mouse event occurred */
	DFWINDOW Mwnd = inWindow(ApplicationWindow, x, y);

	/* ---- process mouse captures ----- */
	if (CaptureMouse != NULL)
	{
		if (NoChildCaptureMouse ||
		    Mwnd == NULL 	||
		    !isAncestor(Mwnd, CaptureMouse))
			Mwnd = CaptureMouse;
	}
	return Mwnd;
}


void handshake(void)
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
		if (++EventQueueOffCtr == MAXMESSAGES)
			EventQueueOffCtr = 0;
		--EventQueueCtr;

		/* get the window in which a keyboard event occurred */
		Kwnd = inFocus;

		/* process keyboard captures */
		if (CaptureKeyboard != NULL)
		{
			if (Kwnd == NULL ||
			    NoChildCaptureKeyboard ||
			    !isAncestor(Kwnd, CaptureKeyboard))
				Kwnd = CaptureKeyboard;
		}

		/* send mouse and keyboard messages to the
		   window that should get them */
		switch (ev.event)
		{
			case SHIFT_CHANGED:
			case KEYBOARD:
				if (!handshaking)
					DfSendMessage(Kwnd, ev.event, ev.mx, ev.my);
				break;

			case LEFT_BUTTON:
				if (!handshaking)
				{
					Mwnd = MouseWindow(ev.mx, ev.my);
					if (!CaptureMouse ||
					    (!NoChildCaptureMouse &&
					     isAncestor(Mwnd, CaptureMouse)))
					{
						if (Mwnd != inFocus)
							DfSendMessage(Mwnd, SETFOCUS, TRUE, 0);
						DfSendMessage(Mwnd, LEFT_BUTTON, ev.mx, ev.my);
					}
				}
				break;

			case DFM_BUTTON_RELEASED:
			case DOUBLE_CLICK:
			case RIGHT_BUTTON:
				if (handshaking)
					break;

			case MOUSE_MOVED:
				Mwnd = MouseWindow(ev.mx, ev.my);
				DfSendMessage(Mwnd, ev.event, ev.mx, ev.my);
				break;

			case CLOCKTICK:
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

		if (++MsgQueueOffCtr == MAXMESSAGES)
			MsgQueueOffCtr = 0;
		--MsgQueueCtr;

		DfSendMessage (mq.wnd, mq.msg, mq.p1, mq.p2);
		if (mq.msg == ENDDIALOG)
			return FALSE;

		if (mq.msg == DFM_STOP)
			return FALSE;
	}

	return TRUE;
}

/* EOF */
