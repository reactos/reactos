/* -------------- button.c -------------- */

#include "dflat.h"

void PaintMsg(DFWINDOW wnd, DF_CTLWINDOW *ct, DFRECT *rc)
{
    if (DfIsVisible(wnd))
    {
        if (DfTestAttribute(wnd, DF_SHADOW))
		{
            /* -------- draw the button's shadow ------- */
            int x;
            DfBackground = DfWndBackground(DfGetParent(wnd));
            DfForeground = BLACK;
            for (x = 1; x <= DfWindowWidth(wnd); x++)
                DfWPutch(wnd, (char)223, x, 1);
            DfWPutch(wnd, (char)220, DfWindowWidth(wnd), 0);
        }
        if (ct->itext != NULL)
		{
            unsigned char *txt;
            txt = DfCalloc(1, strlen(ct->itext)+10);
            if (ct->setting == DF_OFF)    {
                txt[0] = DF_CHANGECOLOR;
                txt[1] = wnd->WindowColors
                            [DF_HILITE_COLOR] [DF_FG] | 0x80;
                txt[2] = wnd->WindowColors
                            [DF_STD_COLOR] [DF_BG] | 0x80;
            }
            DfCopyCommand(txt+strlen(txt),ct->itext,!ct->setting,
                DfWndBackground(wnd));
            DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
            DfSendMessage(wnd, DFM_ADDTEXT, (DF_PARAM) txt, 0);
            free(txt);
        }
        /* --------- write the button's text ------- */
        DfWriteTextLine(wnd, rc, 0, wnd == DfInFocus);
    }
}

void LeftButtonMsg(DFWINDOW wnd, DFMESSAGE msg, DF_CTLWINDOW *ct)
{
    /* --------- draw a pushed button -------- */
    int x;
    DfBackground = DfWndBackground(DfGetParent(wnd));
    DfForeground = DfWndBackground(wnd);
    DfWPutch(wnd, ' ', 0, 0);
    for (x = 0; x < DfWindowWidth(wnd); x++)
    {
         DfWPutch(wnd, (char)220, x+1, 0);
         DfWPutch(wnd, (char)223, x+1, 1);
    }
    if (msg == DFM_LEFT_BUTTON)
        DfSendMessage(NULL, DFM_WAITMOUSE, 0, 0);
    else
        DfSendMessage(NULL, DFM_WAITKEYBOARD, 0, 0);
    DfSendMessage(wnd, DFM_PAINT, 0, 0);
    if (ct->setting == DF_ON)
        DfPostMessage(DfGetParent(wnd), DFM_COMMAND, ct->command, 0);
    else
        DfBeep();
}

int DfButtonProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    DF_CTLWINDOW *ct = DfGetControl(wnd);
    if (ct != NULL)    {
        switch (msg)    {
            case DFM_SETFOCUS:
                DfBaseWndProc(DF_BUTTON, wnd, msg, p1, p2);
                p1 = 0;
                /* ------- fall through ------- */
            case DFM_PAINT:
                PaintMsg(wnd, ct, (DFRECT*)p1);
                return TRUE;
            case DFM_KEYBOARD:
                if (p1 != '\r')
                    break;
                /* ---- fall through ---- */
            case DFM_LEFT_BUTTON:
                LeftButtonMsg(wnd, msg, ct);
                return TRUE;
            case DFM_HORIZSCROLL:
                return TRUE;
            default:
                break;
        }
    }
    return DfBaseWndProc(DF_BUTTON, wnd, msg, p1, p2);
}

/* EOF */
