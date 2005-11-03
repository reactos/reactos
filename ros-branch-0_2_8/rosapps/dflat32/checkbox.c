/* -------------- checkbox.c ------------ */

#include "dflat.h"

int DfCheckBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn;
    DF_CTLWINDOW *ct = DfGetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case DFM_SETFOCUS:
                if (!(int)p1)
                    DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
            case DFM_MOVE:
                rtn = DfBaseWndProc(DF_CHECKBOX, wnd, msg, p1, p2);
                DfSetFocusCursor(wnd);
                return rtn;
            case DFM_PAINT:    {
                char cb[] = "[ ]";
                if (ct->setting)
                    cb[1] = 'X';
                DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
                DfSendMessage(wnd, DFM_ADDTEXT, (DF_PARAM) cb, 0);
                DfSetFocusCursor(wnd);
                break;
            }
            case DFM_KEYBOARD:
                if ((int)p1 != ' ')
                    break;
            case DFM_LEFT_BUTTON:
                ct->setting ^= DF_ON;
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
                return TRUE;
            default:
                break;
        }
    }
    return DfBaseWndProc(DF_CHECKBOX, wnd, msg, p1, p2);
}

BOOL DfCheckBoxSetting(DF_DBOX *db, enum DfCommands cmd)
{
    DF_CTLWINDOW *ct = DfFindCommand(db, cmd, DF_CHECKBOX);
    if (ct != NULL)
        return (ct->isetting == DF_ON);
    return FALSE;
}

/* EOF */
