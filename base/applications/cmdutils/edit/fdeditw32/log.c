/* ------------ log .c ------------ */

#include "dflat.h"

#ifdef INCLUDE_LOGGING

static char *message[] = {
    #undef DFlatMsg
    #define DFlatMsg(m) " " #m,
    #include "dflatmsg.h"
    NULL
};

static FILE *log = NULL;
extern DBOX Log;

void LogMessages (WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    if (log != NULL && message[msg][0] != ' ')
        fprintf(log,
            "%-20.20s %-12.12s %-20.20s, %5.5ld, %5.5ld\n",
            wnd ? (GetTitle(wnd) ? GetTitle(wnd) : "") : "",
            wnd ? ClassNames[GetClass(wnd)] : "",
            message[msg]+1, p1, p2);
}

static int LogProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    WINDOW cwnd = ControlWindow(&Log, ID_LOGLIST);
    char **mn = message;
    switch (msg)    {
        case INITIATE_DIALOG:
            AddAttribute(cwnd, MULTILINE | VSCROLLBAR);
            while (*mn)    {
                SendMessage(cwnd, ADDTEXT, (PARAM) (*mn), 0);
                mn++;
            }
            SendMessage(cwnd, SHOW_WINDOW, 0, 0);
            break;
        case COMMAND:
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

void MessageLog(WINDOW wnd)
{
    if (DialogBox(wnd, &Log, TRUE, LogProc))    {
        if (CheckBoxSetting(&Log, ID_LOGGING))    {
            log = fopen("DFLAT.LOG", "wt");
            SetCommandToggle(&MainMenu, ID_LOG);
        }
        else if (log != NULL)    {
            fclose(log);
            log = NULL;
            ClearCommandToggle(&MainMenu, ID_LOG);
        }
    }
}

#endif
