/* -------------- combobox.c -------------- */

#include "dflat.h"

int ListProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);

int ComboProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)    {
        case CREATE_WINDOW:
            wnd->extension = DfCreateWindow(
                        LISTBOX,
                        NULL,
                        wnd->rc.lf,wnd->rc.tp+1,
                        wnd->ht-1, wnd->wd+1,
                        NULL,
                        wnd,
                        ListProc,
                        HASBORDER | NOCLIP | SAVESELF);
            ((DFWINDOW)(wnd->extension))->ct->command =
                                        wnd->ct->command;
            wnd->ht = 1;
            wnd->rc.bt = wnd->rc.tp;
			break;
        case PAINT:
            foreground = WndBackground(wnd);
            background = WndForeground(wnd);
            wputch(wnd, DOWNSCROLLBOX, WindowWidth(wnd), 0);
            break;
        case KEYBOARD:
            if ((int)p1 == DN)    {
                DfSendMessage(wnd->extension, SETFOCUS, TRUE, 0);
                return TRUE;
            }
            break;
        case LEFT_BUTTON:
            if ((int)p1 == GetRight(wnd) + 1)
                DfSendMessage(wnd->extension, SETFOCUS, TRUE, 0);
            break;
        case CLOSE_WINDOW:
            DfSendMessage(wnd->extension, CLOSE_WINDOW, 0, 0);
            break;
        default:
            break;
    }
    return BaseWndProc(COMBOBOX, wnd, msg, p1, p2);
}

int ListProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
	DFWINDOW pwnd = GetParent(GetParent(wnd));
	DBOX *db = pwnd->extension;
	DFWINDOW cwnd;
	char text[130];
	int rtn;
	DFWINDOW currFocus;

	switch (msg)
	{
		case CREATE_WINDOW:
			wnd->ct = DFmalloc(sizeof(CTLWINDOW));
			wnd->ct->setting = OFF;
			wnd->WindowColors[FRAME_COLOR][FG] =
				wnd->WindowColors[STD_COLOR][FG];
			wnd->WindowColors[FRAME_COLOR][BG] =
				wnd->WindowColors[STD_COLOR][BG];
			rtn = DefaultWndProc(wnd, msg, p1, p2);
			return rtn;

		case SETFOCUS:
			if ((int)p1 == FALSE)
			{
				if (!wnd->isHelping)
				{
					DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
					wnd->ct->setting = OFF;
				}
			}
			else
				wnd->ct->setting = ON;
			break;

		case SHOW_WINDOW:
			if (wnd->ct->setting == OFF)
				return TRUE;
			break;

		case BORDER:
			currFocus = inFocus;
			inFocus = NULL;
			rtn = DefaultWndProc(wnd, msg, p1, p2);
			inFocus = currFocus;
			return rtn;

		case LB_SELECTION:
			rtn = DefaultWndProc(wnd, msg, p1, p2);
			DfSendMessage(wnd, DFM_LB_GETTEXT,
			              (PARAM) text, wnd->selection);
			PutItemText(pwnd, wnd->ct->command, text);
			cwnd = ControlWindow(db, wnd->ct->command);
			DfSendMessage(cwnd, PAINT, 0, 0);
			cwnd->TextChanged = TRUE;
			return rtn;

		case KEYBOARD:
			switch ((int) p1)
			{
				case ESC:
				case FWD:
				case BS:
					cwnd = ControlWindow(db, wnd->ct->command);
					DfSendMessage(cwnd, SETFOCUS, TRUE, 0);
					return TRUE;

				default:
					break;
			}
			break;

		case LB_CHOOSE:
			cwnd = ControlWindow(db, wnd->ct->command);
			DfSendMessage(cwnd, SETFOCUS, TRUE, 0);
			return TRUE;

		case CLOSE_WINDOW:
			if (wnd->ct != NULL)
				free(wnd->ct);
			wnd->ct = NULL;
			break;

		default:
			break;
	}

	return DefaultWndProc(wnd, msg, p1, p2);
}

void PutComboListText(DFWINDOW wnd, enum commands cmd, char *text)
{
	CTLWINDOW *ct = FindCommand(wnd->extension, cmd, COMBOBOX);

	if (ct != NULL)
	{
		DFWINDOW lwnd = ((DFWINDOW)(ct->wnd))->extension;
		DfSendMessage(lwnd, ADDTEXT, (PARAM) text, 0);
	}
}

/* EOF */
