/*  Little watch icon

*/

#include "dflat.h"

int WatchIconProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    static int tick = 0;
    static char *hands[] = { " À ", " Ú ", " ª ", " Ù " };
    switch (msg)
        {
        case CREATE_WINDOW:
            tick = 0;
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            SendMessage(wnd, CAPTURE_MOUSE, 0, 0);
            SendMessage(wnd, HIDE_MOUSE, 0, 0);
            SendMessage(wnd, CAPTURE_KEYBOARD, 0, 0);
            SendMessage(wnd, CAPTURE_CLOCK, 0, 0);
            return rtn;
        case CLOCKTICK:
            ++tick;
            tick &= 3;
            SendMessage(wnd->PrevClock, msg, p1, p2);
            /* (fall through and paint) */
        case PAINT:
            SetStandardColor(wnd);
            writeline(wnd, hands[tick], 1, 1, FALSE);
            return TRUE;
        case BORDER:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            writeline(wnd, "Í", 2, 0, FALSE);
            return rtn;
        case MOUSE_MOVED:
            SendMessage(wnd, HIDE_WINDOW, 0, 0);
            SendMessage(wnd, MOVE, p1, p2);
            SendMessage(wnd, SHOW_WINDOW, 0, 0);
            return TRUE;
        case CLOSE_WINDOW:
            SendMessage(wnd, RELEASE_CLOCK, 0, 0);
            SendMessage(wnd, RELEASE_MOUSE, 0, 0);
            SendMessage(wnd, RELEASE_KEYBOARD, 0, 0);
            SendMessage(wnd, SHOW_MOUSE, 0, 0);
            break;
        default:
            break;
        }

    return DefaultWndProc(wnd, msg, p1, p2);

}

WINDOW WatchIcon(void)
{
    int mx,my;
    WINDOW wnd;

    SendMessage(NULL, CURRENT_MOUSE_CURSOR, (PARAM) &mx, (PARAM) &my);
#ifdef __REACTOS__
    wnd = CreateWindow(BOX, NULL, mx, my, 3, 5, NULL,NULL,
                    WatchIconProc, VISIBLE /*| HASBORDER */| NOCLIP | SAVESELF);
#else
    wnd = CreateWindow(BOX, NULL, mx, my, 3, 5, NULL,NULL,
                    WatchIconProc, VISIBLE | HASBORDER | SHADOW | SAVESELF);
#endif
    return wnd;

}
