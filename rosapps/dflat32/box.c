/* ----------- box.c ------------ */

#include "dflat.h"

int BoxProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
	int rtn;
	DFWINDOW oldFocus;
	CTLWINDOW *ct = GetControl(wnd);
	static BOOL SettingFocus = FALSE;
	if (ct != NULL)
	{
		switch (msg)
		{
			case SETFOCUS:
				SettingFocus = isVisible(wnd);
				rtn = BaseWndProc(BOX, wnd, msg, p1, p2);
				SettingFocus = FALSE;
				return rtn;

            case PAINT:
                return FALSE;
            case LEFT_BUTTON:
            case DFM_BUTTON_RELEASED:
                return DfSendMessage(GetParent(wnd), msg, p1, p2);
            case BORDER:
				if (SettingFocus)
					return TRUE;
				oldFocus = inFocus;
				inFocus = NULL;
				rtn = BaseWndProc(BOX, wnd, msg, p1, p2);
				inFocus = oldFocus;
                if (ct != NULL)
                    if (ct->itext != NULL)
                        writeline(wnd, ct->itext, 1, 0, FALSE);
				return rtn;
            default:
                break;
        }
    }
    return BaseWndProc(BOX, wnd, msg, p1, p2);
}

/* EOF */
