/* ----------- watch.c ----------- */

#include "dflat32/dflat.h"

int WatchIconProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    switch (msg)    {
        case CREATE_WINDOW:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            DfSendMessage(wnd, CAPTURE_MOUSE, 0, 0);
            DfSendMessage(wnd, CAPTURE_KEYBOARD, 0, 0);
            return rtn;
        case PAINT:
            SetStandardColor(wnd);
            writeline(wnd, " R ", 1, 1, FALSE);
            return TRUE;
        case BORDER:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            writeline(wnd, "Í", 2, 0, FALSE);
            return rtn;
        case MOUSE_MOVED:
            DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
            DfSendMessage(wnd, MOVE, p1, p2);
            DfSendMessage(wnd, SHOW_WINDOW, 0, 0);
            return TRUE;
        case CLOSE_WINDOW:
            DfSendMessage(wnd, RELEASE_MOUSE, 0, 0);
            DfSendMessage(wnd, RELEASE_KEYBOARD, 0, 0);
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

DFWINDOW WatchIcon(void)
{
    int mx, my;
    DFWINDOW wnd;

/* this won't work !! */
//    DfSendMessage(NULL, DFM_CURRENT_MOUSE_CURSOR,
//                        (PARAM) &mx, (PARAM) &my);

    mx = 0;//SCREENWIDTH / 2;
    mx = 0;//SCREENHEIGHT / 2;
    wnd = DfCreateWindow(
                    BOX,
                    NULL,
                    mx, my, 3, 5,
                    NULL,NULL,
                    WatchIconProc,
                    VISIBLE | HASBORDER | SHADOW | SAVESELF);
    return wnd;
}

/* EOF */
