/* -------------- combobox.c -------------- */

#include "dflat.h"

int ListProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);

int DfComboProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            wnd->extension = DfDfCreateWindow(
                        DF_LISTBOX,
                        NULL,
                        wnd->rc.lf,wnd->rc.tp+1,
                        wnd->ht-1, wnd->wd+1,
                        NULL,
                        wnd,
                        ListProc,
                        DF_HASBORDER | DF_NOCLIP | DF_SAVESELF);
            ((DFWINDOW)(wnd->extension))->ct->command =
                                        wnd->ct->command;
            wnd->ht = 1;
            wnd->rc.bt = wnd->rc.tp;
			break;
        case DFM_PAINT:
            DfForeground = DfWndBackground(wnd);
            DfBackground = DfWndForeground(wnd);
            DfWPutch(wnd, DF_DOWNSCROLLBOX, DfWindowWidth(wnd), 0);
            break;
        case DFM_KEYBOARD:
            if ((int)p1 == DF_DN)    {
                DfSendMessage(wnd->extension, DFM_SETFOCUS, TRUE, 0);
                return TRUE;
            }
            break;
        case DFM_LEFT_BUTTON:
            if ((int)p1 == DfGetRight(wnd) + 1)
                DfSendMessage(wnd->extension, DFM_SETFOCUS, TRUE, 0);
            break;
        case DFM_CLOSE_WINDOW:
            DfSendMessage(wnd->extension, DFM_CLOSE_WINDOW, 0, 0);
            break;
        default:
            break;
    }
    return DfBaseWndProc(DF_COMBOBOX, wnd, msg, p1, p2);
}

int ListProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
	DFWINDOW pwnd = DfGetParent(DfGetParent(wnd));
	DF_DBOX *db = pwnd->extension;
	DFWINDOW cwnd;
	char text[130];
	int rtn;
	DFWINDOW currFocus;

	switch (msg)
	{
		case DFM_CREATE_WINDOW:
			wnd->ct = DfMalloc(sizeof(DF_CTLWINDOW));
			wnd->ct->setting = DF_OFF;
			wnd->WindowColors[DF_FRAME_COLOR][DF_FG] =
				wnd->WindowColors[DF_STD_COLOR][DF_FG];
			wnd->WindowColors[DF_FRAME_COLOR][DF_BG] =
				wnd->WindowColors[DF_STD_COLOR][DF_BG];
			rtn = DfDefaultWndProc(wnd, msg, p1, p2);
			return rtn;

		case DFM_SETFOCUS:
			if ((int)p1 == FALSE)
			{
				if (!wnd->isHelping)
				{
					DfSendMessage(wnd, DFM_HIDE_WINDOW, 0, 0);
					wnd->ct->setting = DF_OFF;
				}
			}
			else
				wnd->ct->setting = DF_ON;
			break;

		case DFM_SHOW_WINDOW:
			if (wnd->ct->setting == DF_OFF)
				return TRUE;
			break;

		case DFM_BORDER:
			currFocus = DfInFocus;
			DfInFocus = NULL;
			rtn = DfDefaultWndProc(wnd, msg, p1, p2);
			DfInFocus = currFocus;
			return rtn;

		case DFM_LB_SELECTION:
			rtn = DfDefaultWndProc(wnd, msg, p1, p2);
			DfSendMessage(wnd, DFM_LB_GETTEXT,
			              (DF_PARAM) text, wnd->selection);
			DfPutItemText(pwnd, wnd->ct->command, text);
			cwnd = DfControlWindow(db, wnd->ct->command);
			DfSendMessage(cwnd, DFM_PAINT, 0, 0);
			cwnd->TextChanged = TRUE;
			return rtn;

		case DFM_KEYBOARD:
			switch ((int) p1)
			{
				case DF_ESC:
				case DF_FWD:
				case DF_BS:
					cwnd = DfControlWindow(db, wnd->ct->command);
					DfSendMessage(cwnd, DFM_SETFOCUS, TRUE, 0);
					return TRUE;

				default:
					break;
			}
			break;

		case DFM_LB_CHOOSE:
			cwnd = DfControlWindow(db, wnd->ct->command);
			DfSendMessage(cwnd, DFM_SETFOCUS, TRUE, 0);
			return TRUE;

		case DFM_CLOSE_WINDOW:
			if (wnd->ct != NULL)
				free(wnd->ct);
			wnd->ct = NULL;
			break;

		default:
			break;
	}

	return DfDefaultWndProc(wnd, msg, p1, p2);
}

void DfPutComboListText(DFWINDOW wnd, enum DfCommands cmd, char *text)
{
	DF_CTLWINDOW *ct = DfFindCommand(wnd->extension, cmd, DF_COMBOBOX);

	if (ct != NULL)
	{
		DFWINDOW lwnd = ((DFWINDOW)(ct->wnd))->extension;
		DfSendMessage(lwnd, DFM_ADDTEXT, (DF_PARAM) text, 0);
	}
}

/* EOF */
