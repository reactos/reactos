/* -------- radio.c -------- */

#include "dflat32/dflat.h"

static CTLWINDOW *rct[MAXRADIOS];

int RadioButtonProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    DBOX *db = GetParent(wnd)->extension;
    CTLWINDOW *ct = GetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case SETFOCUS:
                if (!(int)p1)
                    DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
            case MOVE:
                rtn = BaseWndProc(RADIOBUTTON,wnd,msg,p1,p2);
                SetFocusCursor(wnd);
                return rtn;
            case PAINT:    {
                char rb[] = "( )";
                if (ct->setting)
                    rb[1] = 7;
                DfSendMessage(wnd, CLEARTEXT, 0, 0);
                DfSendMessage(wnd, ADDTEXT, (PARAM) rb, 0);
                SetFocusCursor(wnd);
                break;
            }
            case KEYBOARD:
                if ((int)p1 != ' ')
                    break;
            case LEFT_BUTTON:
                SetRadioButton(db, ct);
                break;
            default:
                break;
        }
    }
    return BaseWndProc(RADIOBUTTON, wnd, msg, p1, p2);
}

static BOOL Setting = TRUE;

void SetRadioButton(DBOX *db, CTLWINDOW *ct)
{
	Setting = FALSE;
	PushRadioButton(db, ct->command);
	Setting = TRUE;
}

void PushRadioButton(DBOX *db, enum commands cmd)
{
    CTLWINDOW *ctt = db->ctl;
    CTLWINDOW *ct = FindCommand(db, cmd, RADIOBUTTON);
    int i;

	if (ct == NULL)
		return;

    /* --- clear all the radio buttons
                in this group on the dialog box --- */

    /* -------- build a table of all radio buttons at the
            same x vector ---------- */
    for (i = 0; i < MAXRADIOS; i++)
        rct[i] = NULL;
    while (ctt->class)    {
        if (ctt->class == RADIOBUTTON)
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
    while (i < MAXRADIOS && rct[i] != NULL)
        i++;
    /* ---- ignore everthing past the group ------ */
    while (i < MAXRADIOS)
        rct[i++] = NULL;

    for (i = 0; i < MAXRADIOS; i++)    {
        if (rct[i] != NULL)    {
            int wason = rct[i]->setting;
            rct[i]->setting = OFF;
			if (Setting)
	            rct[i]->isetting = OFF;
            if (wason)
                DfSendMessage(rct[i]->wnd, PAINT, 0, 0);
        }
    }
	/* ----- set the specified radio button on ----- */
    ct->setting = ON;
	if (Setting)
	    ct->isetting = ON;
    DfSendMessage(ct->wnd, PAINT, 0, 0);
}

BOOL RadioButtonSetting(DBOX *db, enum commands cmd)
{
    CTLWINDOW *ct = FindCommand(db, cmd, RADIOBUTTON);
    if (ct != NULL)
        return (ct->setting == ON);
    return FALSE;
}

/* EOF */
