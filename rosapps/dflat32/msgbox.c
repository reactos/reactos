/* ------------------ msgbox.c ------------------ */

#include "dflat.h"

extern DF_DBOX MsgBox;
extern DF_DBOX InputBoxDB;
DFWINDOW CancelWnd;

static int ReturnValue;

int DfMessageBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            DfGetClass(wnd) = DF_MESSAGEBOX;
			DfInitWindowColors(wnd);
            DfClearAttribute(wnd, DF_CONTROLBOX);
            break;
        case DFM_KEYBOARD:
            if (p1 == '\r' || p1 == DF_ESC)
                ReturnValue = (int)p1;
            break;
        default:
            break;
    }
    return DfBaseWndProc(DF_MESSAGEBOX, wnd, msg, p1, p2);
}

int DfYesNoBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            DfGetClass(wnd) = DF_MESSAGEBOX;
			DfInitWindowColors(wnd);
            DfClearAttribute(wnd, DF_CONTROLBOX);
            break;
        case DFM_KEYBOARD:    {
            int c = tolower((int)p1);
            if (c == 'y')
                DfSendMessage(wnd, DFM_COMMAND, DF_ID_OK, 0);
            else if (c == 'n')
                DfSendMessage(wnd, DFM_COMMAND, DF_ID_CANCEL, 0);
            break;
        }
        default:
            break;
    }
    return DfBaseWndProc(DF_MESSAGEBOX, wnd, msg, p1, p2);
}

int DfErrorBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            DfGetClass(wnd) = DF_ERRORBOX;
			DfInitWindowColors(wnd);
            break;
        case DFM_KEYBOARD:
            if (p1 == '\r' || p1 == DF_ESC)
                ReturnValue = (int)p1;
            break;
        default:
            break;
    }
    return DfBaseWndProc(DF_ERRORBOX, wnd, msg, p1, p2);
}

int DfCancelBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            CancelWnd = wnd;
            DfSendMessage(wnd, DFM_CAPTURE_MOUSE, 0, 0);
            DfSendMessage(wnd, DFM_CAPTURE_KEYBOARD, 0, 0);
            break;
        case DFM_COMMAND:
            if ((int) p1 == DF_ID_CANCEL && (int) p2 == 0)
                DfSendMessage(DfGetParent(wnd), msg, p1, p2);
            return TRUE;
        case DFM_CLOSE_WINDOW:
            CancelWnd = NULL;
            DfSendMessage(wnd, DFM_RELEASE_MOUSE, 0, 0);
            DfSendMessage(wnd, DFM_RELEASE_KEYBOARD, 0, 0);
            p1 = TRUE;
            break;
        default:
            break;
    }
    return DfBaseWndProc(DF_MESSAGEBOX, wnd, msg, p1, p2);
}

void DfCloseCancelBox(void)
{
    if (CancelWnd != NULL)
        DfSendMessage(CancelWnd, DFM_CLOSE_WINDOW, 0, 0);
}

static char *InputText;
static int TextLength;

int InputBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn;
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            rtn = DfDefaultWndProc(wnd, msg, p1, p2);
            DfSendMessage(DfControlWindow(&InputBoxDB,DF_ID_INPUTTEXT),
                        DFM_SETTEXTLENGTH, TextLength, 0);
            return rtn;
        case DFM_COMMAND:
            if ((int) p1 == DF_ID_OK && (int) p2 == 0)
                DfGetItemText(wnd, DF_ID_INPUTTEXT,
                            InputText, TextLength);
            break;
        default:
            break;
    }
    return DfDefaultWndProc(wnd, msg, p1, p2);
}

BOOL DfInputBox(DFWINDOW wnd,char *ttl,char *msg,char *text,int len)
{
    InputText = text;
    TextLength = len;
    InputBoxDB.dwnd.title = ttl;
    InputBoxDB.dwnd.w = 4 + 
        max(20, max(len, max((int)strlen(ttl), (int)strlen(msg))));
    InputBoxDB.ctl[1].dwnd.x = (InputBoxDB.dwnd.w-2-len)/2;
    InputBoxDB.ctl[0].dwnd.w = strlen(msg);
    InputBoxDB.ctl[0].itext = msg;
    InputBoxDB.ctl[1].dwnd.w = len;
    InputBoxDB.ctl[2].dwnd.x = (InputBoxDB.dwnd.w - 20) / 2;
    InputBoxDB.ctl[3].dwnd.x = InputBoxDB.ctl[2].dwnd.x + 10;
    InputBoxDB.ctl[2].isetting = DF_ON;
    InputBoxDB.ctl[3].isetting = DF_ON;
    return DfDialogBox(wnd, &InputBoxDB, TRUE, InputBoxProc);
}

BOOL DfGenericMessage(DFWINDOW wnd,char *ttl,char *msg,int buttonct,
      int (*wndproc)(struct DfWindow *,enum DfMessages,DF_PARAM,DF_PARAM),
      char *b1, char *b2, int c1, int c2, int isModal)
{
    BOOL rtn;
    MsgBox.dwnd.title = ttl;
    MsgBox.ctl[0].dwnd.h = DfMsgHeight(msg);
	if (ttl)
		MsgBox.ctl[0].dwnd.w = max(max(DfMsgWidth(msg),
			   (int)(buttonct*8 + buttonct + 2)), (int)strlen(ttl)+2);
	else
		MsgBox.ctl[0].dwnd.w = max(DfMsgWidth(msg), (int)(buttonct*8 + buttonct + 2));
    MsgBox.dwnd.h = MsgBox.ctl[0].dwnd.h+6;
    MsgBox.dwnd.w = MsgBox.ctl[0].dwnd.w+4;
    if (buttonct == 1)
        MsgBox.ctl[1].dwnd.x = (MsgBox.dwnd.w - 10) / 2;
    else    {
        MsgBox.ctl[1].dwnd.x = (MsgBox.dwnd.w - 20) / 2;
        MsgBox.ctl[2].dwnd.x = MsgBox.ctl[1].dwnd.x + 10;
        MsgBox.ctl[2].class = DF_BUTTON;
    }
    MsgBox.ctl[1].dwnd.y = MsgBox.dwnd.h - 4;
    MsgBox.ctl[2].dwnd.y = MsgBox.dwnd.h - 4;
    MsgBox.ctl[0].itext = msg;
    MsgBox.ctl[1].itext = b1;
    MsgBox.ctl[2].itext = b2;
    MsgBox.ctl[1].command = c1;
    MsgBox.ctl[2].command = c2;
    MsgBox.ctl[1].isetting = DF_ON;
    MsgBox.ctl[2].isetting = DF_ON;
    rtn = DfDialogBox(wnd, &MsgBox, isModal, wndproc);
    MsgBox.ctl[2].class = 0;
    return rtn;
}

DFWINDOW DfMomentaryMessage(char *msg)
{
    DFWINDOW wnd = DfDfCreateWindow(
                    DF_TEXTBOX,
                    NULL,
                    -1,-1,DfMsgHeight(msg)+2,DfMsgWidth(msg)+2,
                    NULL,NULL,NULL,
                    DF_HASBORDER | DF_SHADOW | DF_SAVESELF);
    DfSendMessage(wnd, DFM_SETTEXT, (DF_PARAM) msg, 0);
    DfWindowClientColor(wnd, WHITE, GREEN);
    DfWindowFrameColor(wnd, WHITE, GREEN);
    DfSendMessage (wnd, DFM_SHOW_WINDOW, 0, 0);
    return wnd;
}

int DfMsgHeight(char *msg)
{
	int h = 1;

	while ((msg = strchr(msg, '\n')) != NULL)
	{
		h++;
		msg++;
	}

	return min(h, DfGetScreenHeight ()-10);
}

int DfMsgWidth(char *msg)
{
	int w = 0;
	char *cp = msg;

	while ((cp = strchr(msg, '\n')) != NULL)
	{
		w = max(w, (int) (cp-msg));
		msg = cp+1;
	}

	return min(max((int)strlen(msg), (int)w), DfGetScreenWidth()-10);
}

/* EOF */
