/* ----------- watch.c ----------- */

#include "dflat.h"

int DfWatchIconProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn;
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            rtn = DfDefaultWndProc(wnd, msg, p1, p2);
            DfSendMessage(wnd, DFM_CAPTURE_MOUSE, 0, 0);
            DfSendMessage(wnd, DFM_CAPTURE_KEYBOARD, 0, 0);
            return rtn;
        case DFM_PAINT:
            DfSetStandardColor(wnd);
            DfWriteLine(wnd, " R ", 1, 1, FALSE);
            return TRUE;
        case DFM_BORDER:
            rtn = DfDefaultWndProc(wnd, msg, p1, p2);
            DfWriteLine(wnd, "Í", 2, 0, FALSE);
            return rtn;
        case MOUSE_MOVED:
            DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
            DfSendMessage(wnd, DFM_MOVE, p1, p2);
            DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
            return TRUE;
        case DFM_CLOSE_WINDOW:
            DfSendMessage(wnd, DFM_RELEASE_MOUSE, 0, 0);
            DfSendMessage(wnd, DFM_RELEASE_KEYBOARD, 0, 0);
            break;
        default:
            break;
    }
    return DfDefaultWndProc(wnd, msg, p1, p2);
}

DFWINDOW DfWatchIcon(void)
{
    int mx, my;
    DFWINDOW wnd;

/* this won't work !! */
//    DfSendMessage(NULL, DFM_CURRENT_MOUSE_CURSOR,
//                        (DF_PARAM) &mx, (DF_PARAM) &my);

    mx = 0;//DF_SCREENWIDTH / 2;
    mx = 0;//DF_SCREENHEIGHT / 2;
    wnd = DfDfCreateWindow(
                    DF_BOX,
                    NULL,
                    mx, my, 3, 5,
                    NULL,NULL,
                    DfWatchIconProc,
                    DF_VISIBLE | DF_HASBORDER | DF_SHADOW | DF_SAVESELF);
    return wnd;
}

/* EOF */
