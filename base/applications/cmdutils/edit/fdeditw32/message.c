/* --------- message.c ---------- */

#include "dflat.h"

#ifndef _WIN32
#define USECBRKHNLDR 1
#endif

static int px = -1, py = -1;
static int pmx = -1, pmy = -1;
static int mx, my;
static int handshaking=0;
static volatile BOOL CriticalError;
BOOL AllocTesting=FALSE,AltDown=FALSE;
jmp_buf AllocError;

/* ---------- event queue ---------- */
static struct events    {
    MESSAGE event;
    int mx;
    int my;
} EventQueue[MAXMESSAGES];

/* ---------- message queue --------- */
static struct msgs {
    WINDOW wnd;
    MESSAGE msg;
    PARAM p1;
    PARAM p2;
} MsgQueue[MAXMESSAGES];

static int EventQueueOnCtr;
static int EventQueueOffCtr;
static int EventQueueCtr;
static int MsgQueueOnCtr;
static int MsgQueueOffCtr;
static int MsgQueueCtr;

static int lagdelay = FIRSTDELAY;

WINDOW CaptureMouse;
WINDOW CaptureKeyboard;
static BOOL NoChildCaptureMouse;
static BOOL NoChildCaptureKeyboard;
static int doubletimer = 0;
static int delaytimer  = 1;
static int clocktimer  = 2;
#ifndef _WIN32
static char timerused[3] = {0, 0, 0};
static long unsigned int timerend[3] = {0, 0, 0};
static long unsigned int timerstart[3] = {0, 0, 0};
volatile long unsigned int *biostimer = MK_FP(0x40,0x6c);
#endif
char time_string[] = "12:34pm    ";
static WINDOW Cwnd;

static void StopMsg(void);

/* ------- timer interrupt service routine ------- */
/* More complex countdown handling by Eric Auer */
/* Allows us to work without hooking intr. 0x08 */
#ifndef _WIN32
int timed_out(int timer)		/* was: countdown 0? */
{
    if ((timer > 2) || (timer < 0))
        return -1;			/* invalid -> always elapsed */

    if (timerused[timer] == 0)		/* not active at all? */
        return 0;

    if (timerused[timer] == 2)		/* timeout already known? */
        return 1;

    if ((biostimer[0] < timerstart[timer]) || (biostimer[0] >= timerend[timer]))
        {
        timerused[timer] = 2;		/* countdown elapsed */
        return 1;
        }

    return 0;				/* still waiting */

}

int timer_running(int timer)		/* was: countdown > 0? */
{
    if ((timer > 2) || (timer < 0))
        return 0;			/* invalid -> never running */

    if (timerused[timer] == 1)		/* running? */
        {
        return (1 - timed_out(timer));  /* if not elapsed, running */
        }
    else
        return 0;                       /* certainly not running */

}

int timer_disabled(int timer)		/* was: countdown -1? */
{
    if ((timer > 2) || (timer < 0))
        return 1;			/* invalid -> always disabled */

    return (timerused[timer] == 0);

}

void disable_timer(int timer)		/* was: countdown = -1 */
{
    if ((timer > 2) || (timer < 0))
        return;

    timerused[timer] = 0;

}

void set_timer(int timer, int secs)
{
    if ((timer > 2) || (timer < 0))
        return;

    timerstart[timer]=biostimer[0];
    timerend[timer]=timerstart[timer] + (secs*182UL/10) + 1;
    timerused[timer]=1;                 /* mark as running */

}

void set_timer_ticks(int timer, int ticks)
{
    if ((timer > 2) || (timer < 0))
        return;

    timerstart[timer]=biostimer[0];
    timerend[timer]=timerstart[timer] + ticks;
    timerused[timer]=1;                 /* mark as running */
}
#endif

#ifdef OLDCRITERR
static char ermsg[] = "Error accessing drive x:";
#else
static char ermsg[] = "Error accessing drive";
#endif

/* -------- test for critical errors --------- */
int TestCriticalError(void)
{
    int rtn = 0;

    if (CriticalError)
        {
    	beep();
        rtn = 1;
        CriticalError = FALSE;
        if (TestErrorMessage(ermsg) == FALSE)
            rtn = 2;

        }

    return rtn;

}

/* ------ critical error interrupt service routine ------ */
#ifdef OLDCRITERR
static void interrupt far newcrit(IREGS ir);

static void interrupt far newcrit(IREGS ir)
{
    if (!(ir.ax & 0x8000))              /* If any drive affected... */
        {
        ermsg[sizeof(ermsg)-2]=(ir.ax & 0xff) + 'A';
        CriticalError=TRUE;             /* ... only then we have a crit. error */
        }

    ir.ax = 0;

}

#else

int crit_error(void);

/* ----- critical error handler ----- */
int crit_error(void)
{
    CriticalError = TRUE;
#ifndef _WIN32
    hardretn(-1);		/* return an error!    */
#endif
    return 2;			/* is this correct???  */

    /* some possibilities: hardresume(1) is retry,     */
    /* hardresume(2) is abort. Or return to the caller */
    /* with a fake function result: hardretn(result)   */
}

#endif

static void StopMsg(void)
{
    ClearClipboard();
    ClearDialogBoxes();
    restorecursor(); 
    unhidecursor();
    hide_mousecursor();

}

#ifdef USECBRKHNLDR
/* ------ control break handler --------- */
#define ABORT		0
#define CONTINUE	1	/* any non-zero # will continue */
int c_break(void)
{
/*    PostMessage(NULL, STOP, 0, 0);
    StopMsg();
*/
    return CONTINUE;	/* JUST IGNORE CTRL BREAK ... */
}
#endif

/* ------------ initialize the message system --------- */
BOOL init_messages(void)
{
    AllocTesting = TRUE;
    if (setjmp(AllocError) != 0)
        {
        StopMsg();
        return FALSE;
	}

    resetmouse();
    set_mousetravel(0, SCREENWIDTH-1, 0, SCREENHEIGHT-1);
    savecursor();
    hidecursor();
    px = py = -1;
    pmx = pmy = -1;
    mx = my = 0;
    CaptureMouse = CaptureKeyboard = NULL;
    NoChildCaptureMouse = FALSE;
    NoChildCaptureKeyboard = FALSE;
    MsgQueueOnCtr = MsgQueueOffCtr = MsgQueueCtr = 0;
    EventQueueOnCtr = EventQueueOffCtr = EventQueueCtr = 0;

#ifdef OLDCRITERR		/* old style: hook intr 0x24 manually    */
    setvect(CRIT, newcrit);	/* (vector save/restore not needed here) */
#else
#ifndef _WIN32
    harderr(crit_error);	/* set critical error handler (dos.h)  */
#endif    				/* handler uses hardretn / hardresume  */
#endif

#ifdef USECBRKHNLDR
    ctrlbrk(c_break);		/* set ctrl break handler (dos.h) */
    				/* handler returns 0 to abort program */
    setcbrk(0);			/* 1 = all / 0 = only con calls check */
#endif

    PostMessage(NULL,START,0,0);
    lagdelay = FIRSTDELAY;
    return TRUE;

}

/* ----- post an event and parameters to event queue ---- */
void PostEvent(MESSAGE event, int p1, int p2)
{
    if (EventQueueCtr != MAXMESSAGES)
        {
        EventQueue[EventQueueOnCtr].event = event;
        EventQueue[EventQueueOnCtr].mx = p1;
        EventQueue[EventQueueOnCtr].my = p2;
        if (++EventQueueOnCtr == MAXMESSAGES)
            EventQueueOnCtr = 0;

        EventQueueCtr++;
        }

}

/* ------ collect mouse, clock, and keyboard events ----- */
static void near collect_events(void)
{
    static int ShiftKeys=0;
    int sk,hr;
    struct tm *now;

    update_key_buffer();

    /* Test for a clock event (one/second) */
    if (timed_out(clocktimer))
        {
        struct COUNTRY thiscountry;     /* Country support new 0.8 */
        time_t t=time(NULL);            /* The current time */
        char timesep=':';               /* Default 12:12am separator */
        int ampmflag=1;

        if (country(0, &thiscountry))   /* dos.h (int 21.38) */
            {
            timesep=thiscountry.co_tmsep[0];
            ampmflag=(thiscountry.co_time == 0);  /* 0 ampm 1 24h clock */
            }

        /* Get the current time */
        now=localtime(&t);
        hr=now->tm_hour;
        if (ampmflag && (hr > 12)) hr -= 12;
        if (ampmflag && (hr == 0)) hr  = 12;
        if (ampmflag)
            {
            sprintf(time_string, "%2d%c%02d%s", hr, timesep, now->tm_min, ((now->tm_hour > 11) ? "pm " : "am "));
            }
        else
            sprintf(time_string, "%2d%c%02d", hr, timesep, now->tm_min);

        set_timer(clocktimer, 1);       /* Reset the timer */
#ifdef _WIN32
        PostEvent(CLOCKTICK, (int)time_string, 0);
#else
        PostEvent(CLOCKTICK, FP_SEG(time_string), FP_OFF(time_string));
#endif
        }

    /* Keyboard events */
    if ((sk=getshift()) != ShiftKeys)
        {
        ShiftKeys=sk;

        /* The shift status changed */
        PostEvent(SHIFT_CHANGED, sk, 0);
    	if (sk & ALTKEY)
            AltDown=TRUE;

        }

    /* Test for keystroke */
    if (keyhit())
        {
        int c=getkey();

        AltDown=FALSE;
        PostEvent(KEYBOARD, c, sk);
        }

    /* Test for mouse events */
    if (button_releases())
        {
        /* The button was released */
        AltDown=FALSE;
        set_timer_ticks(doubletimer, DOUBLETICKS);
        PostEvent(BUTTON_RELEASED, mx, my);
        disable_timer(delaytimer);
        }

    get_mouseposition(&mx, &my);
    if (mx != px || my != py)
        {
        px=mx;
        py=my;
        PostEvent(MOUSE_MOVED, mx, my);
        }

    if (rightbutton())
        {
        AltDown=FALSE;
        PostEvent(RIGHT_BUTTON, mx, my);
	}

    if (leftbutton())
        {
        AltDown=FALSE;
        if (mx == pmx && my == pmy)
            {
            /* Same position as last left button */
            if (timer_running(doubletimer))
                {
                /* Second click before double timeout */
                disable_timer(doubletimer);
                PostEvent(DOUBLE_CLICK, mx, my);
                }
            else if (!timer_running(delaytimer))
                {
                /* Button held down a while */
                set_timer_ticks(delaytimer, lagdelay);
                lagdelay=DELAYTICKS;
                PostEvent(LEFT_BUTTON, mx, my);
                }

            }
        else
            {
            /* New button press */
            disable_timer(doubletimer);
            set_timer_ticks(delaytimer, FIRSTDELAY);
            lagdelay=DELAYTICKS;
            PostEvent(LEFT_BUTTON, mx, my);
            pmx=mx;
            pmy=my;
            }

        }
    else
        lagdelay=FIRSTDELAY;

}

/* ----- post a message and parameters to msg queue ---- */
void PostMessage(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    if (MsgQueueCtr != MAXMESSAGES)
        {
        MsgQueue[MsgQueueOnCtr].wnd=wnd;
        MsgQueue[MsgQueueOnCtr].msg=msg;
        MsgQueue[MsgQueueOnCtr].p1=p1;
        MsgQueue[MsgQueueOnCtr].p2=p2;
        if (++MsgQueueOnCtr == MAXMESSAGES)
            MsgQueueOnCtr=0;

        MsgQueueCtr++;
        }

}

/* --------- send a message to a window ----------- */
int SendMessage(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn=TRUE,x,y;

#ifdef INCLUDE_LOGGING
    LogMessages(wnd, msg, p1, p2);
#endif
    if (wnd != NULL)
        switch (msg)
            {
            case PAINT:
            case BORDER:
                /* Don't send these messages unless the window is visible */
                if (isVisible(wnd))
                    rtn=(*wnd->wndproc)(wnd, msg, p1, p2);

                break;
            case RIGHT_BUTTON:
            case LEFT_BUTTON:
            case DOUBLE_CLICK:
            case BUTTON_RELEASED:
                /* Don't send these messages unless the window is
                   visible or has captured the mouse */
                if (isVisible(wnd) || wnd == CaptureMouse)
                    rtn=(*wnd->wndproc)(wnd, msg, p1, p2);

                break;
            case KEYBOARD:
            case SHIFT_CHANGED:
                /* Don't send these messages unless the window is
                   visible or has captured the keyboard */
                if (!(isVisible(wnd) || wnd == CaptureKeyboard))
                    break;
            default:
                rtn=(*wnd->wndproc)(wnd, msg, p1, p2);
                break;

            }

    /* Window processor returned true or the message was sent to no window at all. */
    if (rtn != FALSE)
        {
        /* Process messages that a window sends to the system itself */
        switch (msg)
            {
            case STOP:
                StopMsg();
                break;

            /* Clock messages */
            case CAPTURE_CLOCK:
                if (Cwnd == NULL)
                    set_timer(clocktimer, 0);

                wnd->PrevClock = Cwnd;
                Cwnd = wnd;
                break;
            case RELEASE_CLOCK:
                Cwnd = wnd->PrevClock;
                if (Cwnd == NULL)
                    disable_timer(clocktimer);

                break;

            /* Keyboard messages */
            case KEYBOARD_CURSOR:
                if (wnd == NULL)
                    cursor((int)p1, (int)p2);
                else if (wnd == inFocus)
                    cursor(GetClientLeft(wnd)+(int)p1,GetClientTop(wnd)+(int)p2);

                break;
            case CAPTURE_KEYBOARD:
                if (p2)
                    ((WINDOW)p2)->PrevKeyboard=CaptureKeyboard;
                else
                    wnd->PrevKeyboard = CaptureKeyboard;

                CaptureKeyboard=wnd;
                NoChildCaptureKeyboard=(int)p1;
                break;
            case RELEASE_KEYBOARD:
                if (wnd != NULL)
                    {
                    if (CaptureKeyboard == wnd || (int)p1)
                        CaptureKeyboard=wnd->PrevKeyboard;
                    else
                        {
                        WINDOW twnd=CaptureKeyboard;

                        while (twnd != NULL)
                            {
                            if (twnd->PrevKeyboard == wnd)
                                {
                                twnd->PrevKeyboard=wnd->PrevKeyboard;
                                break;
                                }

                            twnd=twnd->PrevKeyboard;
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
                *(int*)p1=x;
                *(int*)p2=y;
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
                    set_cursor_type(0x0607);     /* Û */
                else
                    set_cursor_type(0x0106);     /* _ */

                unhidecursor();
                break;
            case WAITKEYBOARD:
                waitforkeyboard();
                break;

            /* Mouse messages */
            case RESET_MOUSE:
                resetmouse();
                set_mousetravel(0, SCREENWIDTH-1, 0, SCREENHEIGHT-1);
                break;
            case MOUSE_INSTALLED:
                rtn=mouse_installed();
                break;
            case MOUSE_TRAVEL:
                {
                RECT rc;

                if (!p1)
                    {
                    rc.lf=rc.tp=0;
                    rc.rt=SCREENWIDTH-1;
                    rc.bt=SCREENHEIGHT-1;
                    }
                else 
                    rc=*(RECT *)p1;

                set_mousetravel(rc.lf, rc.rt, rc.tp, rc.bt);
                break;
                }
            case SHOW_MOUSE:
                show_mousecursor();
                break;
            case HIDE_MOUSE:
                hide_mousecursor();
                break;
            case MOUSE_CURSOR:
                set_mouseposition((int)p1, (int)p2);
                break;
            case CURRENT_MOUSE_CURSOR:
                get_mouseposition((int*)p1,(int*)p2);
                break;
            case WAITMOUSE:
                waitformouse();
                break;
            case TESTMOUSE:
                rtn=mousebuttons();
                break;
            case CAPTURE_MOUSE:
                if (p2)
                    ((WINDOW)p2)->PrevMouse=CaptureMouse;
                else
                    wnd->PrevMouse=CaptureMouse;

                CaptureMouse=wnd;
                NoChildCaptureMouse=(int)p1;
                break;
            case RELEASE_MOUSE:
                if (wnd != NULL)
                    {
                    if (CaptureMouse == wnd || (int)p1)
                        CaptureMouse=wnd->PrevMouse;
                    else
                        {
                        WINDOW twnd=CaptureMouse;

                        while (twnd != NULL)
                            {
                            if (twnd->PrevMouse == wnd)
                                {
                                twnd->PrevMouse=wnd->PrevMouse;
                                break;
                                }

                            twnd=twnd->PrevMouse;
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

static RECT VisibleRect(WINDOW wnd)
{
    RECT rc=WindowRect(wnd);

    if (!TestAttribute(wnd, NOCLIP))
        {
        WINDOW pwnd=GetParent(wnd);
        RECT prc;

        if (pwnd != NULL) {
            prc=ClientRect(pwnd);
            while (pwnd != NULL)
                {
                if (TestAttribute(pwnd, NOCLIP))
                    break;

                rc=subRectangle(rc, prc);
                if (!ValidRect(rc))
                    break;

                if ((pwnd=GetParent(pwnd)) != NULL)
                    prc=ClientRect(pwnd);

                }
        }
	}

    return rc;

}

/* ----- find window that mouse coordinates are in --- */
static WINDOW inWindow(WINDOW wnd, int x, int y)
{
    WINDOW Hit=NULL;

    while (wnd != NULL)
        {
        if (isVisible(wnd))
            {
            WINDOW wnd1;
            RECT rc=VisibleRect(wnd);

            if (InsideRect(x, y, rc))
                Hit=wnd;

            if ((wnd1=inWindow(LastWindow(wnd), x, y)) != NULL)
                Hit=wnd1;

            if (Hit != NULL)
                break;

            }

        wnd=PrevWindow(wnd);
	}

    return Hit;

}

/* Get the window in which a mouse event occurred */
static WINDOW MouseWindow(int x, int y)
{
    WINDOW Mwnd=inWindow(ApplicationWindow, x, y);

    /* Process mouse captures ----- */
    if (CaptureMouse != NULL)
        {
        if (NoChildCaptureMouse || Mwnd == NULL || !isAncestor(Mwnd, CaptureMouse))
            Mwnd=CaptureMouse;

	}

    return Mwnd;

}

void handshake(void)
{
    handshaking++;
    dispatch_message();
    --handshaking;

}

/* ---- dispatch messages to the message proc function ---- */
BOOL dispatch_message(void)
{
    WINDOW Mwnd, Kwnd;

    collect_events();                   /* Collect mouse and keyboard events */

    /* Dequeue and process events */
    while (EventQueueCtr > 0)
        {
        struct events ev;

        ev = EventQueue[EventQueueOffCtr];
        if (++EventQueueOffCtr == MAXMESSAGES)
            EventQueueOffCtr = 0;

        --EventQueueCtr;

        /* Get the window in which a keyboard event occurred */
        Kwnd=inFocus;

        /* Process keyboard captures */
        if (CaptureKeyboard != NULL)
            if (Kwnd == NULL || NoChildCaptureKeyboard || !isAncestor(Kwnd, CaptureKeyboard))
                Kwnd = CaptureKeyboard;

        /* Send mouse and keyboard messages to the window that should get them */
        switch (ev.event)
            {
            case SHIFT_CHANGED:
            case KEYBOARD:
                if (!handshaking)
                    SendMessage(Kwnd, ev.event, ev.mx, ev.my);

                break;
            case LEFT_BUTTON:
                if (!handshaking)
                    {
                    Mwnd=MouseWindow(ev.mx, ev.my);
                    if (!CaptureMouse || (!NoChildCaptureMouse && isAncestor(Mwnd, CaptureMouse)))
                    	if (Mwnd != inFocus)
                            SendMessage(Mwnd, SETFOCUS, TRUE, 0);

                    SendMessage(Mwnd, LEFT_BUTTON, ev.mx, ev.my);
                    }
                break;
            case BUTTON_RELEASED:
            case DOUBLE_CLICK:
            case RIGHT_BUTTON:
                if (handshaking)
                    break;

            case MOUSE_MOVED:
                Mwnd=MouseWindow(ev.mx, ev.my);
                SendMessage(Mwnd, ev.event, ev.mx, ev.my);
                break;
            case CLOCKTICK:
#ifdef _WIN32
                SendMessage(Cwnd, ev.event,ev.mx, 0);
#else
                SendMessage(Cwnd, ev.event,(PARAM) MK_FP(ev.mx, ev.my), 0);
#endif
                break;
            default:
                break;

            }

        }

    /* Dequeue and process messages */
    while (MsgQueueCtr > 0)
        {
        struct msgs mq;

        mq=MsgQueue[MsgQueueOffCtr];
        if (++MsgQueueOffCtr == MAXMESSAGES)
            MsgQueueOffCtr=0;

        --MsgQueueCtr;
        SendMessage(mq.wnd, mq.msg, mq.p1, mq.p2);
        if (mq.msg == ENDDIALOG)
            return FALSE;

        if (mq.msg == STOP)
            {
            PostMessage(NULL, STOP, 0, 0);
            return FALSE;
            }

        }

    return TRUE;

}
