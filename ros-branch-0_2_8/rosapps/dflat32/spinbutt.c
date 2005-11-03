/* ------------ spinbutt.c ------------- */

#include "dflat.h"

int DfSpinButtonProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn;
    DF_CTLWINDOW *ct = DfGetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case DFM_CREATE_WINDOW:
                wnd->wd -= 2;
                wnd->rc.rt -= 2;
                break;
            case DFM_SETFOCUS:
                rtn = DfBaseWndProc(DF_SPINBUTTON, wnd, msg, p1, p2);
                if (!(int)p1)
                    DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
                DfSetFocusCursor(wnd);
                return rtn;
            case DFM_PAINT:
                DfForeground = DfWndBackground(wnd);
                DfBackground = DfWndForeground(wnd);
                DfWPutch(wnd,DF_UPSCROLLBOX,DfWindowWidth(wnd), 0);
                DfWPutch(wnd,DF_DOWNSCROLLBOX,DfWindowWidth(wnd)+1,0);
                DfSetFocusCursor(wnd);
                break;
            case DFM_LEFT_BUTTON:
                if (p1 == DfGetRight(wnd) + 1)
                    DfSendMessage(wnd, DFM_KEYBOARD, DF_UP, 0);
                else if (p1 == DfGetRight(wnd) + 2)
                    DfSendMessage(wnd, DFM_KEYBOARD, DF_DN, 0);
                if (wnd != DfInFocus)
                    DfSendMessage(wnd, DFM_SETFOCUS, TRUE, 0);
                return TRUE;
            case DFM_LB_SETSELECTION:
                rtn = DfBaseWndProc(DF_SPINBUTTON, wnd, msg, p1, p2);
                wnd->wtop = (int) p1;
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
                return rtn;
            default:
                break;
        }
    }
    return DfBaseWndProc(DF_SPINBUTTON, wnd, msg, p1, p2);
}

/* EOF */
