/* ----------- box.c ------------ */

#include "dflat.h"

int BoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    CTLWINDOW *ct = GetControl(wnd);

    if (ct != NULL)
        {
        switch (msg)
            {
            case SETFOCUS:
            case PAINT:
                return FALSE;
            case LEFT_BUTTON:
            case BUTTON_RELEASED:
                return SendMessage(GetParent(wnd), msg, p1, p2);
            case BORDER:
                rtn = BaseWndProc(BOX, wnd, msg, p1, p2);
                if (ct != NULL && ct->itext != NULL)
                    writeline(wnd, ct->itext, 1, 0, FALSE);

                return rtn;
            default:
                break;

            }

        }

    return BaseWndProc(BOX, wnd, msg, p1, p2);

}
