/* ------------ spinbutt.c ------------- */

#include "dflat32/dflat.h"

int SpinButtonProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    CTLWINDOW *ct = GetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case CREATE_WINDOW:
                wnd->wd -= 2;
                wnd->rc.rt -= 2;
                break;
            case SETFOCUS:
                rtn = BaseWndProc(SPINBUTTON, wnd, msg, p1, p2);
                if (!(int)p1)
                    DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
                SetFocusCursor(wnd);
                return rtn;
            case PAINT:
                foreground = WndBackground(wnd);
                background = WndForeground(wnd);
                wputch(wnd,UPSCROLLBOX,WindowWidth(wnd), 0);
                wputch(wnd,DOWNSCROLLBOX,WindowWidth(wnd)+1,0);
                SetFocusCursor(wnd);
                break;
            case LEFT_BUTTON:
                if (p1 == GetRight(wnd) + 1)
                    DfSendMessage(wnd, KEYBOARD, UP, 0);
                else if (p1 == GetRight(wnd) + 2)
                    DfSendMessage(wnd, KEYBOARD, DN, 0);
                if (wnd != inFocus)
                    DfSendMessage(wnd, SETFOCUS, TRUE, 0);
                return TRUE;
            case LB_SETSELECTION:
                rtn = BaseWndProc(SPINBUTTON, wnd, msg, p1, p2);
                wnd->wtop = (int) p1;
                DfSendMessage(wnd, PAINT, 0, 0);
                return rtn;
            default:
                break;
        }
    }
    return BaseWndProc(SPINBUTTON, wnd, msg, p1, p2);
}

/* EOF */
