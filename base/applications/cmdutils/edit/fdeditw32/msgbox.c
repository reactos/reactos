/* ------------------ msgbox.c ------------------ */

#include "dflat.h"

extern DBOX MsgBox;
extern DBOX InputBoxDB;
WINDOW CancelWnd;
static int ReturnValue;
static int TextLength;
static char *InputText;

int MessageBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            GetClass(wnd) = MESSAGEBOX;
            InitWindowColors(wnd);
            ClearAttribute(wnd, CONTROLBOX);
            break;
        case KEYBOARD:
            if (p1 == '\r' || p1 == ESC)
                ReturnValue = (int)p1;

            break;
        default:
            break;

        }

    return BaseWndProc(MESSAGEBOX, wnd, msg, p1, p2);

}

int YesNoBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            GetClass(wnd) = MESSAGEBOX;
            InitWindowColors(wnd);
            ClearAttribute(wnd, CONTROLBOX);
            break;
        case KEYBOARD:
            {
            int c = tolower((int)p1);

            if (c == 'y')
                SendMessage(wnd, COMMAND, ID_OK, 0);
            else if (c == 'n')
                SendMessage(wnd, COMMAND, ID_CANCEL, 0);

            break;
            }
        default:
            break;

        }

    return BaseWndProc(MESSAGEBOX, wnd, msg, p1, p2);

}

int ErrorBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            GetClass(wnd) = ERRORBOX;
            InitWindowColors(wnd);
            break;
        case KEYBOARD:
            if (p1 == '\r' || p1 == ESC)
                ReturnValue = (int)p1;

            break;
        default:
            break;

        }

    return BaseWndProc(ERRORBOX, wnd, msg, p1, p2);

}

int CancelBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            CancelWnd = wnd;
            SendMessage(wnd, CAPTURE_MOUSE, 0, 0);
            SendMessage(wnd, CAPTURE_KEYBOARD, 0, 0);
            break;
        case COMMAND:
            if ((int) p1 == ID_CANCEL && (int) p2 == 0)
                SendMessage(GetParent(wnd), msg, p1, p2);

            return TRUE;
        case CLOSE_WINDOW:
            CancelWnd = NULL;
            SendMessage(wnd, RELEASE_MOUSE, 0, 0);
            SendMessage(wnd, RELEASE_KEYBOARD, 0, 0);
            p1 = TRUE;
            break;
        default:
            break;

        }

    return BaseWndProc(MESSAGEBOX, wnd, msg, p1, p2);

}

void CloseCancelBox(void)
{
    if (CancelWnd != NULL)
        SendMessage(CancelWnd, CLOSE_WINDOW, 0, 0);

}

int InputBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;

    switch (msg)
        {
        case CREATE_WINDOW:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            SendMessage(ControlWindow(&InputBoxDB,ID_INPUTTEXT),
                        SETTEXTLENGTH, TextLength, 0);
            SendMessage(ControlWindow(&InputBoxDB,ID_INPUTTEXT),
                        ADDTEXT, (PARAM) InputText, 0);
            return rtn;
        case COMMAND:
            if ((int) p1 == ID_OK && (int) p2 == 0)
                GetItemText(wnd, ID_INPUTTEXT, InputText, TextLength);

            break;
        default:
            break;

        }

    return DefaultWndProc(wnd, msg, p1, p2);

}

BOOL InputBox(WINDOW wnd,char *ttl,char *msg,char *text,int len,int wd)
{
    int ln = wd ? wd : len;

    ln=min(SCREENWIDTH-8, ln);
    InputText=text;
    TextLength=len;
    InputBoxDB.dwnd.title=ttl;
    InputBoxDB.dwnd.w=4+max(20, max(ln, max(strlen(ttl), strlen(msg))));
    InputBoxDB.ctl[1].dwnd.x = (InputBoxDB.dwnd.w-2-ln)/2;
    InputBoxDB.ctl[0].dwnd.w = strlen(msg);
    InputBoxDB.ctl[0].itext = msg;
    InputBoxDB.ctl[1].itext = NULL;
    InputBoxDB.ctl[1].dwnd.w = ln;
    InputBoxDB.ctl[2].dwnd.x = (InputBoxDB.dwnd.w - 20) / 2;
    InputBoxDB.ctl[3].dwnd.x = InputBoxDB.ctl[2].dwnd.x + 10;
    InputBoxDB.ctl[2].isetting = ON;
    InputBoxDB.ctl[3].isetting = ON;
    return DialogBox(wnd, &InputBoxDB, TRUE, InputBoxProc);

}

BOOL GenericMessage(WINDOW wnd,char *ttl,char *msg,int buttonct,
      int (*wndproc)(struct window *,enum messages,PARAM,PARAM),
      char *b1, char *b2, int c1, int c2, int isModal)
{
    BOOL rtn;
    MsgBox.dwnd.title = ttl;
    MsgBox.ctl[0].dwnd.h = MsgHeight(msg);
    MsgBox.ctl[0].dwnd.w = max(max(MsgWidth(msg), buttonct*8 + buttonct + 2), strlen(ttl)+2);
    MsgBox.dwnd.h = MsgBox.ctl[0].dwnd.h+6;
    MsgBox.dwnd.w = MsgBox.ctl[0].dwnd.w+4;
    if (buttonct == 1)
        MsgBox.ctl[1].dwnd.x = (MsgBox.dwnd.w - 10) / 2;
    else
        {
        MsgBox.ctl[1].dwnd.x = (MsgBox.dwnd.w - 20) / 2;
        MsgBox.ctl[2].dwnd.x = MsgBox.ctl[1].dwnd.x + 10;
        MsgBox.ctl[2].Class = BUTTON;
        }

    MsgBox.ctl[1].dwnd.y = MsgBox.dwnd.h - 4;
    MsgBox.ctl[2].dwnd.y = MsgBox.dwnd.h - 4;
    MsgBox.ctl[0].itext = msg;
    MsgBox.ctl[1].itext = b1;
    MsgBox.ctl[2].itext = b2;
    MsgBox.ctl[1].command = c1;
    MsgBox.ctl[2].command = c2;
    MsgBox.ctl[1].isetting = ON;
    MsgBox.ctl[2].isetting = ON;
    rtn = DialogBox(wnd, &MsgBox, isModal, wndproc);
    MsgBox.ctl[2].Class = 0;
    return rtn;

}

WINDOW MomentaryMessage(char *msg)
{
    WINDOW wnd = CreateWindow(TEXTBOX,NULL,-1,-1,MsgHeight(msg)+2,MsgWidth(msg)+2,NULL,NULL,NULL,HASBORDER | SHADOW | SAVESELF);

    SendMessage(wnd, SETTEXT, (PARAM) msg, 0);
    if (cfg.mono == 0)
        {
        WindowClientColor(wnd, WHITE, GREEN);
        WindowFrameColor(wnd, WHITE, GREEN);
        }

    SendMessage(wnd, SHOW_WINDOW, 0, 0);
    return wnd;

}

int MsgHeight(char *msg)
{
    int h=1;

    while ((msg = strchr(msg, '\n')) != NULL)
        {
        h++;
        msg++;
        }

    return min(h, SCREENHEIGHT-10);

}

int MsgWidth(char *msg)
{
    int w=0;
    char *cp=msg;

    while ((cp=strchr(msg, '\n')) != NULL)
        {
        w=max(w, (int) (cp-msg));
        msg=cp+1;
        }

    return min(max(strlen(msg),w), SCREENWIDTH-10);

}
