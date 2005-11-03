/* ------------ log .c ------------ */

#include "dflat32/dflat.h"

#ifdef INCLUDE_LOGGING

static char *message[] = {
    #undef DFlatMsg
    #define DFlatMsg(m) " " #m,
    #include "dflat32/dflatmsg.h"
    NULL
};

static FILE *logfile = NULL;
extern DBOX Log;

void LogMessages (DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    if (logfile != NULL && message[msg][0] != ' ')
        fprintf(logfile,
            "%-20.20s %-12.12s %-20.20s, %5.5ld, %5.5ld\n",
            wnd ? (GetTitle(wnd) ? GetTitle(wnd) : "") : "",
            wnd ? ClassNames[GetClass(wnd)] : "",
            message[msg]+1, p1, p2);
}

static int LogProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    DFWINDOW cwnd = ControlWindow(&Log, ID_LOGLIST);
    char **mn = message;
    switch (msg)    {
        case INITIATE_DIALOG:
            AddAttribute(cwnd, MULTILINE | VSCROLLBAR);
            while (*mn)    {
                DfSendMessage(cwnd, ADDTEXT, (PARAM) (*mn), 0);
                mn++;
            }
            DfSendMessage(cwnd, SHOW_WINDOW, 0, 0);
            break;
        case DFM_COMMAND:
            if ((int) p1 == ID_OK)    {
                int item;
                int tl = GetTextLines(cwnd);
                for (item = 0; item < tl; item++)
                    if (ItemSelected(cwnd, item))
                        mn[item][0] = LISTSELECTOR;
            }
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

void MessageLog(DFWINDOW wnd)
{
	if (DfDialogBox(wnd, &Log, TRUE, LogProc))
	{
		if (CheckBoxSetting(&Log, ID_LOGGING))
		{
			logfile = fopen("DFLAT.LOG", "wt");
			SetCommandToggle(&MainMenu, ID_LOG);
		}
		else if (logfile != NULL)
		{
			fclose(logfile);
			logfile = NULL;
			ClearCommandToggle(&MainMenu, ID_LOG);
		}
	}
}

#endif

/* EOF */