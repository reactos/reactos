/* ---------------- statbar.c -------------- */

#include "dflat.h"

int DfStatusBarProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
	char *statusbar;
	switch (msg)	{
		case DFM_CREATE_WINDOW:
		case DFM_MOVE:
			DfSendMessage(wnd, DFM_CAPTURE_CLOCK, 0, 0);
			break;
		case DFM_KEYBOARD:
			if ((int)p1 == DF_CTRL_F4)
				return TRUE;
			break;
		case DFM_PAINT:	
			if (!DfIsVisible(wnd))
				break;
			statusbar = DfCalloc(1, DfWindowWidth(wnd)+1);
			memset(statusbar, ' ', DfWindowWidth(wnd));
			*(statusbar+DfWindowWidth(wnd)) = '\0';
			strncpy(statusbar+1, "DF_F1=Help Ý FreeDos Edit", 22);
			if (wnd->text)	{
				int len = min((int)strlen(wnd->text), (int)(DfWindowWidth(wnd)-17));
				if (len > 0)	{
					int off=(DfWindowWidth(wnd)-len)/2;
					strncpy(statusbar+off, wnd->text, len);
				}
			}
			if (wnd->TimePosted)
				*(statusbar+DfWindowWidth(wnd)-8) = '\0';
			DfSetStandardColor(wnd);
    	    DfPutWindowLine(wnd, statusbar, 0, 0);
			free(statusbar);
			return TRUE;
		case DFM_BORDER:
			return TRUE;
		case DFM_CLOCKTICK:
			DfSetStandardColor(wnd);
			DfPutWindowLine(wnd, (char *)p1, DfWindowWidth(wnd)-8, 0);
			wnd->TimePosted = TRUE;
			return TRUE;
		case DFM_CLOSE_WINDOW:
			DfSendMessage(NULL, DFM_RELEASE_CLOCK, 0, 0);
			break;
		default:
			break;
	}
	return DfBaseWndProc(DF_STATUSBAR, wnd, msg, p1, p2);
}

/* EOF */
