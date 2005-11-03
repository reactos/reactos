/* ------------ log .c ------------ */

#include "dflat.h"

#ifdef INCLUDE_LOGGING

static char *message[] = {
    #undef DFlatMsg
    #define DFlatMsg(m) " " #m,
    #include "dflatmsg.h"
    NULL
};

static FILE *logfile = NULL;
extern DF_DBOX Log;

void DfLogMessages (DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    if (logfile != NULL && message[msg][0] != ' ')
        fprintf(logfile,
            "%-20.20s %-12.12s %-20.20s, %5.5ld, %5.5ld\n",
            wnd ? (DfGetTitle(wnd) ? DfGetTitle(wnd) : "") : "",
            wnd ? DfClassNames[DfGetClass(wnd)] : "",
            message[msg]+1, p1, p2);
}

static int LogProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    DFWINDOW cwnd = DfControlWindow(&Log, DF_ID_LOGLIST);
    char **mn = message;
    switch (msg)    {
        case DFM_INITIATE_DIALOG:
            DfAddAttribute(cwnd, DF_MULTILINE | DF_VSCROLLBAR);
            while (*mn)    {
                DfSendMessage(cwnd, DFM_ADDTEXT, (DF_PARAM) (*mn), 0);
                mn++;
            }
            DfSendMessage(cwnd, DFM_SHOW_WINDOW, 0, 0);
            break;
        case DFM_COMMAND:
            if ((int) p1 == DF_ID_OK)    {
                int item;
                int tl = DfGetTextLines(cwnd);
                for (item = 0; item < tl; item++)
                    if (DfItemSelected(cwnd, item))
                        mn[item][0] = DF_LISTSELECTOR;
            }
            break;
        default:
            break;
    }
    return DfDefaultWndProc(wnd, msg, p1, p2);
}

void DfMessageLog(DFWINDOW wnd)
{
	if (DfDialogBox(wnd, &Log, TRUE, LogProc))
	{
		if (DfCheckBoxSetting(&Log, DF_ID_LOGGING))
		{
			logfile = fopen("DFLAT.LOG", "wt");
			DfSetCommandToggle(&DfMainMenu, DF_ID_LOG);
		}
		else if (logfile != NULL)
		{
			fclose(logfile);
			logfile = NULL;
			DfClearCommandToggle(&DfMainMenu, DF_ID_LOG);
		}
	}
}

#endif

/* EOF */
