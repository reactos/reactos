/*  Status Bar

    Part of the FreeDOS Editor

*/

#include "dflat.h"

extern char time_string[];

int StatusBarProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    char *statusbar;

    switch (msg)
        {
        case CREATE_WINDOW:
            SendMessage(wnd, CAPTURE_CLOCK, 0, 0);
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
            strncpy(statusbar+1, "F1=Help Ý", 9);
            if (wnd->text)
                {
                int len = min(strlen(wnd->text), WindowWidth(wnd)-9);

                if (len > 0)
                    {
                    int off=(WindowWidth(wnd)-67);

                    strncpy(statusbar+off, wnd->text, len);
                    }

                }

            strncpy(statusbar+WindowWidth(wnd)-10, "³", 1);
            if (wnd->TimePosted)
                *(statusbar+WindowWidth(wnd)-8) = '\0';
            else
                strncpy(statusbar+WindowWidth(wnd)-8,time_string, 9);

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
            SendMessage(wnd->PrevClock, msg, p1, p2);
            return TRUE;
        case CLOSE_WINDOW:
            SendMessage(wnd, RELEASE_CLOCK, 0, 0);
            break;
        default:
            break;
        }

    return BaseWndProc(STATUSBAR, wnd, msg, p1, p2);

}
