/* -------------- button.c -------------- */

#include "dflat.h"

void PaintMsg(DFWINDOW wnd, CTLWINDOW *ct, DFRECT *rc)
{
    if (isVisible(wnd))
    {
        if (TestAttribute(wnd, SHADOW))
		{
            /* -------- draw the button's shadow ------- */
            int x;
            background = WndBackground(GetParent(wnd));
            foreground = BLACK;
            for (x = 1; x <= WindowWidth(wnd); x++)
                wputch(wnd, (char)223, x, 1);
            wputch(wnd, (char)220, WindowWidth(wnd), 0);
        }
        if (ct->itext != NULL)
		{
            unsigned char *txt;
            txt = DFcalloc(1, strlen(ct->itext)+10);
            if (ct->setting == OFF)    {
                txt[0] = CHANGECOLOR;
                txt[1] = wnd->WindowColors
                            [HILITE_COLOR] [FG] | 0x80;
                txt[2] = wnd->WindowColors
                            [STD_COLOR] [BG] | 0x80;
            }
            CopyCommand(txt+strlen(txt),ct->itext,!ct->setting,
                WndBackground(wnd));
            DfSendMessage(wnd, CLEARTEXT, 0, 0);
            DfSendMessage(wnd, ADDTEXT, (PARAM) txt, 0);
            free(txt);
        }
        /* --------- write the button's text ------- */
        WriteTextLine(wnd, rc, 0, wnd == inFocus);
    }
}

void LeftButtonMsg(DFWINDOW wnd, DFMESSAGE msg, CTLWINDOW *ct)
{
    /* --------- draw a pushed button -------- */
    int x;
    background = WndBackground(GetParent(wnd));
    foreground = WndBackground(wnd);
    wputch(wnd, ' ', 0, 0);
    for (x = 0; x < WindowWidth(wnd); x++)
    {
         wputch(wnd, (char)220, x+1, 0);
         wputch(wnd, (char)223, x+1, 1);
    }
    if (msg == LEFT_BUTTON)
        DfSendMessage(NULL, WAITMOUSE, 0, 0);
    else
        DfSendMessage(NULL, WAITKEYBOARD, 0, 0);
    DfSendMessage(wnd, PAINT, 0, 0);
    if (ct->setting == ON)
        DfPostMessage(GetParent(wnd), DFM_COMMAND, ct->command, 0);
    else
        beep();
}

int ButtonProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    CTLWINDOW *ct = GetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case SETFOCUS:
                BaseWndProc(BUTTON, wnd, msg, p1, p2);
                p1 = 0;
                /* ------- fall through ------- */
            case PAINT:
                PaintMsg(wnd, ct, (DFRECT*)p1);
                return TRUE;
            case KEYBOARD:
                if (p1 != '\r')
                    break;
                /* ---- fall through ---- */
            case LEFT_BUTTON:
                LeftButtonMsg(wnd, msg, ct);
                return TRUE;
            case HORIZSCROLL:
                return TRUE;
            default:
                break;
        }
    }
    return BaseWndProc(BUTTON, wnd, msg, p1, p2);
}

/* EOF */
