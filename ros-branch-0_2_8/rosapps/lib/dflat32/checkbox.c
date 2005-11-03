/* -------------- checkbox.c ------------ */

#include "dflat32/dflat.h"

int CheckBoxProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    CTLWINDOW *ct = GetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case SETFOCUS:
                if (!(int)p1)
                    DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
            case MOVE:
                rtn = BaseWndProc(CHECKBOX, wnd, msg, p1, p2);
                SetFocusCursor(wnd);
                return rtn;
            case PAINT:    {
                char cb[] = "[ ]";
                if (ct->setting)
                    cb[1] = 'X';
                DfSendMessage(wnd, CLEARTEXT, 0, 0);
                DfSendMessage(wnd, ADDTEXT, (PARAM) cb, 0);
                SetFocusCursor(wnd);
                break;
            }
            case KEYBOARD:
                if ((int)p1 != ' ')
                    break;
            case LEFT_BUTTON:
                ct->setting ^= ON;
                DfSendMessage(wnd, PAINT, 0, 0);
                return TRUE;
            default:
                break;
        }
    }
    return BaseWndProc(CHECKBOX, wnd, msg, p1, p2);
}

BOOL CheckBoxSetting(DBOX *db, enum commands cmd)
{
    CTLWINDOW *ct = FindCommand(db, cmd, CHECKBOX);
    if (ct != NULL)
        return (ct->isetting == ON);
    return FALSE;
}

/* EOF */
