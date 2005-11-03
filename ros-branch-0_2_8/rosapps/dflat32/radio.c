/* -------- radio.c -------- */

#include "dflat.h"

static DF_CTLWINDOW *rct[DF_MAXRADIOS];

int DfRadioButtonProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn;
    DF_DBOX *db = DfGetParent(wnd)->extension;
    DF_CTLWINDOW *ct = DfGetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case DFM_SETFOCUS:
                if (!(int)p1)
                    DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
            case DFM_MOVE:
                rtn = DfBaseWndProc(DF_RADIOBUTTON,wnd,msg,p1,p2);
                DfSetFocusCursor(wnd);
                return rtn;
            case DFM_PAINT:    {
                char rb[] = "( )";
                if (ct->setting)
                    rb[1] = 7;
                DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
                DfSendMessage(wnd, DFM_ADDTEXT, (DF_PARAM) rb, 0);
                DfSetFocusCursor(wnd);
                break;
            }
            case DFM_KEYBOARD:
                if ((int)p1 != ' ')
                    break;
            case DFM_LEFT_BUTTON:
                DfSetRadioButton(db, ct);
                break;
            default:
                break;
        }
    }
    return DfBaseWndProc(DF_RADIOBUTTON, wnd, msg, p1, p2);
}

static BOOL Setting = TRUE;

void DfSetRadioButton(DF_DBOX *db, DF_CTLWINDOW *ct)
{
	Setting = FALSE;
	DfPushRadioButton(db, ct->command);
	Setting = TRUE;
}

void DfPushRadioButton(DF_DBOX *db, enum DfCommands cmd)
{
    DF_CTLWINDOW *ctt = db->ctl;
    DF_CTLWINDOW *ct = DfFindCommand(db, cmd, DF_RADIOBUTTON);
    int i;

	if (ct == NULL)
		return;

    /* --- clear all the radio buttons
                in this group on the dialog box --- */

    /* -------- build a table of all radio buttons at the
            same x vector ---------- */
    for (i = 0; i < DF_MAXRADIOS; i++)
        rct[i] = NULL;
    while (ctt->class)    {
        if (ctt->class == DF_RADIOBUTTON)
            if (ct->dwnd.x == ctt->dwnd.x)
                rct[ctt->dwnd.y] = ctt;
        ctt++;
    }

    /* ----- find the start of the radiobutton group ---- */
    i = ct->dwnd.y;
    while (i >= 0 && rct[i] != NULL)
        --i;
    /* ---- ignore everthing before the group ------ */
    while (i >= 0)
        rct[i--] = NULL;

    /* ----- find the end of the radiobutton group ---- */
    i = ct->dwnd.y;
    while (i < DF_MAXRADIOS && rct[i] != NULL)
        i++;
    /* ---- ignore everthing past the group ------ */
    while (i < DF_MAXRADIOS)
        rct[i++] = NULL;

    for (i = 0; i < DF_MAXRADIOS; i++)    {
        if (rct[i] != NULL)    {
            int wason = rct[i]->setting;
            rct[i]->setting = DF_OFF;
			if (Setting)
	            rct[i]->isetting = DF_OFF;
            if (wason)
                DfSendMessage(rct[i]->wnd, DFM_PAINT, 0, 0);
        }
    }
	/* ----- set the specified radio button on ----- */
    ct->setting = DF_ON;
	if (Setting)
	    ct->isetting = DF_ON;
    DfSendMessage(ct->wnd, DFM_PAINT, 0, 0);
}

BOOL DfRadioButtonSetting(DF_DBOX *db, enum DfCommands cmd)
{
    DF_CTLWINDOW *ct = DfFindCommand(db, cmd, DF_RADIOBUTTON);
    if (ct != NULL)
        return (ct->setting == DF_ON);
    return FALSE;
}

/* EOF */
