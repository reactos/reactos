/* ---------------- statbar.c -------------- */

#include "dflat.h"

int StatusBarProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
	char *statusbar;
	switch (msg)	{
		case CREATE_WINDOW:
		case MOVE:
			DfSendMessage(wnd, CAPTURE_CLOCK, 0, 0);
			break;
		case KEYBOARD:
			if ((int)p1 == CTRL_F4)
				return TRUE;
			break;
		case PAINT:	
			if (!isVisible(wnd))
				break;
			statusbar = DFcalloc(1, WindowWidth(wnd)+1);
			memset(statusbar, ' ', WindowWidth(wnd));
			*(statusbar+WindowWidth(wnd)) = '\0';
			strncpy(statusbar+1, "F1=Help Ý FreeDos Edit", 22);
			if (wnd->text)	{
				int len = min((int)strlen(wnd->text), (int)(WindowWidth(wnd)-17));
				if (len > 0)	{
					int off=(WindowWidth(wnd)-len)/2;
					strncpy(statusbar+off, wnd->text, len);
				}
			}
			if (wnd->TimePosted)
				*(statusbar+WindowWidth(wnd)-8) = '\0';
			SetStandardColor(wnd);
    	    PutWindowLine(wnd, statusbar, 0, 0);
			free(statusbar);
			return TRUE;
		case BORDER:
			return TRUE;
		case CLOCKTICK:
			SetStandardColor(wnd);
			PutWindowLine(wnd, (char *)p1, WindowWidth(wnd)-8, 0);
			wnd->TimePosted = TRUE;
			return TRUE;
		case CLOSE_WINDOW:
			DfSendMessage(NULL, RELEASE_CLOCK, 0, 0);
			break;
		default:
			break;
	}
	return BaseWndProc(STATUSBAR, wnd, msg, p1, p2);
}

/* EOF */
