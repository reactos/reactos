/* ----------- box.c ------------ */

#include "dflat.h"

int DfBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
	int rtn;
	DFWINDOW oldFocus;
	DF_CTLWINDOW *ct = DfGetControl(wnd);
	static BOOL SettingFocus = FALSE;
	if (ct != NULL)
	{
		switch (msg)
		{
			case DFM_SETFOCUS:
				SettingFocus = DfIsVisible(wnd);
				rtn = DfBaseWndProc(DF_BOX, wnd, msg, p1, p2);
				SettingFocus = FALSE;
				return rtn;

            case DFM_PAINT:
                return FALSE;
            case DFM_LEFT_BUTTON:
            case DFM_BUTTON_RELEASED:
                return DfSendMessage(DfGetParent(wnd), msg, p1, p2);
            case DFM_BORDER:
				if (SettingFocus)
					return TRUE;
				oldFocus = DfInFocus;
				DfInFocus = NULL;
				rtn = DfBaseWndProc(DF_BOX, wnd, msg, p1, p2);
				DfInFocus = oldFocus;
                if (ct != NULL)
                    if (ct->itext != NULL)
                        DfWriteLine(wnd, ct->itext, 1, 0, FALSE);
				return rtn;
            default:
                break;
        }
    }
    return DfBaseWndProc(DF_BOX, wnd, msg, p1, p2);
}

/* EOF */
