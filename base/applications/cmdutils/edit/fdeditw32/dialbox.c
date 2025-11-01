/* ----------------- dialbox.c -------------- */

#include "dflat.h"

static int inFocusCommand(DBOX *);
static BOOL dbShortcutKeys(DBOX *, int);
static int ControlProc(WINDOW, MESSAGE, PARAM, PARAM);
static void FirstFocus(DBOX *db);
static void NextFocus(DBOX *db);
static void PrevFocus(DBOX *db);
static CTLWINDOW *AssociatedControl(DBOX *, enum commands);

static BOOL SysMenuOpen;
static DBOX **dbs = NULL;
static int dbct = 0;

/* --- clear all heap allocations to control text fields --- */
void ClearDialogBoxes(void)
{
    int i;

    for (i=0;i<dbct;i++)
        {
        CTLWINDOW *ct = (*(dbs+i))->ctl;

        while (ct->Class)
            {
            if ((ct->Class == EDITBOX || ct->Class == TEXTBOX || ct->Class == COMBOBOX) && ct->itext != NULL)
                {
                free(ct->itext);
                ct->itext = NULL;
                }

            ct++;
            }

        }

    if (dbs != NULL)
        {
        free(dbs);
        dbs = NULL;
        }

    dbct = 0;

}

/* -------- CREATE_WINDOW Message --------- */
static int CreateWindowMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;
    CTLWINDOW *ct = db->ctl;
    WINDOW cwnd;
    int rtn, i;

    /* ---- build a table of processed dialog boxes ---- */
    for (i=0;i<dbct;i++)
        if (db == dbs[i])
            break;

    if (i == dbct)
        {
        dbs = DFrealloc(dbs, sizeof(DBOX *) * (dbct+1));
        *(dbs + dbct++) = db;
        }

    rtn = BaseWndProc(DIALOG, wnd, CREATE_WINDOW, p1, p2);
    ct = db->ctl;
    while (ct->Class)
        {
        int attrib = 0;

        if (TestAttribute(wnd, NOCLIP))
            attrib |= NOCLIP;

        if (wnd->Modal)
            attrib |= SAVESELF;

        ct->setting = ct->isetting;
        if (ct->Class == EDITBOX && ct->dwnd.h > 1)
            attrib |= (MULTILINE | HASBORDER);
        else if ((ct->Class == LISTBOX || ct->Class == TEXTBOX) && ct->dwnd.h > 2)
            attrib |= HASBORDER;

        cwnd = CreateWindow(ct->Class,
                        ct->dwnd.title,
                        ct->dwnd.x+GetClientLeft(wnd),
                        ct->dwnd.y+GetClientTop(wnd),
                        ct->dwnd.h,
                        ct->dwnd.w,
                        ct,
                        wnd,
                        ControlProc,
                        attrib);
        if ((ct->Class == EDITBOX || ct->Class == TEXTBOX ||
                ct->Class == COMBOBOX) && ct->itext != NULL)
            SendMessage(cwnd, SETTEXT, (PARAM) ct->itext, 0);

        ct++;
        }

    return rtn;

}

/* -------- LEFT_BUTTON Message --------- */
static BOOL LeftButtonMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;
    CTLWINDOW *ct = db->ctl;
    if (WindowSizing || WindowMoving)
        return TRUE;
    if (HitControlBox(wnd, p1-GetLeft(wnd), p2-GetTop(wnd))) {
        PostMessage(wnd, KEYBOARD, ' ', ALTKEY);
        return TRUE;
    }
    while (ct->Class)    {
        WINDOW cwnd = ct->wnd;
        if (ct->Class == COMBOBOX)    {
            if (p2 == GetTop(cwnd))    {
                if (p1 == GetRight(cwnd)+1)    {
                    SendMessage(cwnd, LEFT_BUTTON, p1, p2);
                    return TRUE;
                }
            }
            if (GetClass(inFocus) == LISTBOX)
                SendMessage(wnd, SETFOCUS, TRUE, 0);
        }
        else if (ct->Class == SPINBUTTON)    {
            if (p2 == GetTop(cwnd))    {
                if (p1 == GetRight(cwnd)+1 ||
                        p1 == GetRight(cwnd)+2)    {
                    SendMessage(cwnd, LEFT_BUTTON, p1, p2);
                    return TRUE;
                }
            }
        }
        ct++;
    }
    return FALSE;
}

/* -------- KEYBOARD Message --------- */
static BOOL KeyboardMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;
    CTLWINDOW *ct;

    if (WindowMoving || WindowSizing)
        return FALSE;

    switch ((int)p1)
        {
        case F1:
            ct = GetControl(inFocus);
            if (ct != NULL)
                if (DisplayHelp(wnd, ct->help))
                    return TRUE;

            break;
        case SHIFT_HT:
        case LARROW:
        case UP:
            PrevFocus(db);
            break;
        case ALT_F6:
        case '\t':
        case FWD:
        case DN:
            NextFocus(db);
            break;
        case ' ':
            if (((int)p2 & ALTKEY) && TestAttribute(wnd, CONTROLBOX))
                {
                SysMenuOpen = TRUE;
                BuildSystemMenu(wnd);
                return TRUE;
                }

            break;
        case CTRL_F4:
        case ESC:
            SendMessage(wnd, COMMAND, ID_CANCEL, 0);
            break;
        default:
            /* ------ search all the shortcut keys ----- */
            if (dbShortcutKeys(db, (int) p1))
                return TRUE;

            break;

        }

    return wnd->Modal;

}

/* -------- COMMAND Message --------- */
static BOOL CommandMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;

    switch ((int) p1)
        {
        case ID_OK:
        case ID_CANCEL:
            if ((int)p2 != 0)
                return TRUE;

            wnd->ReturnCode = (int) p1;
            if (wnd->Modal)
                PostMessage(wnd, ENDDIALOG, 0, 0);
            else
                SendMessage(wnd, CLOSE_WINDOW, TRUE, 0);

            return TRUE;
        case ID_THREE:
            if ((int)p2 != 0)
                return FALSE;

            wnd->ReturnCode = (int) p1;
            PostMessage(wnd, ENDDIALOG, 0, 0);
            SendMessage(wnd, PAINT, 0, 0);
            return FALSE;
        case ID_HELP:
            if ((int)p2 != 0)
                return TRUE;

            return DisplayHelp(wnd, db->HelpName);
        default:
            break;

        }

    return FALSE;

}

/* ----- window-processing module, DIALOG window class ----- */
int DialogProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    DBOX *db = wnd->extension;

    switch (msg)    {
        case CREATE_WINDOW:
            return CreateWindowMsg(wnd, p1, p2);
        case SHIFT_CHANGED:
            if (wnd->Modal)
                return TRUE;
            break;
        case LEFT_BUTTON:
            if (LeftButtonMsg(wnd, p1, p2))
                return TRUE;
            break;
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case CLOSE_POPDOWN:
            SysMenuOpen = FALSE;
            break;
        case LB_SELECTION:
        case LB_CHOOSE:
            if (SysMenuOpen)
                return TRUE;
            SendMessage(wnd, COMMAND, inFocusCommand(db), msg);
            break;
        case SETFOCUS:
            if ((int)p1 && wnd->dfocus != NULL && isVisible(wnd))
                return SendMessage(wnd->dfocus, SETFOCUS, TRUE, 0);
            break;
        case COMMAND:
            if (CommandMsg(wnd, p1, p2))
                return TRUE;
            break;
        case PAINT:
            p2 = TRUE;
            break;
        case MOVE:
        case SIZE:
            rtn = BaseWndProc(DIALOG, wnd, msg, p1, p2);
            if (wnd->dfocus != NULL && isVisible(wnd))
                SendMessage(wnd->dfocus, SETFOCUS, TRUE, 0);
            return rtn;
        case CLOSE_WINDOW:
            if (!p1)    {
                SendMessage(wnd, COMMAND, ID_CANCEL, 0);
                return TRUE;
            }
            break;
        default:
            break;
    }
    return BaseWndProc(DIALOG, wnd, msg, p1, p2);
}

/* ------- create and execute a dialog box ---------- */
BOOL DialogBox(WINDOW wnd, DBOX *db, BOOL Modal,
  int (*wndproc)(struct window *, enum messages, PARAM, PARAM))
{
    BOOL rtn = FALSE;
    int x = db->dwnd.x, y = db->dwnd.y;
    WINDOW DialogWnd;

    DialogWnd = CreateWindow(DIALOG,
                        db->dwnd.title,
                        x, y,
                        db->dwnd.h,
                        db->dwnd.w,
                        db,
                        wnd,
                        wndproc,
                        (Modal ? SAVESELF : 0));
    SendMessage(DialogWnd, SETFOCUS, TRUE, 0);
    DialogWnd->Modal = Modal;
    FirstFocus(db);
    PostMessage(DialogWnd, INITIATE_DIALOG, 0, 0);
    if (Modal)    {
        SendMessage(DialogWnd, CAPTURE_MOUSE, 0, 0);
        SendMessage(DialogWnd, CAPTURE_KEYBOARD, 0, 0);
        while (dispatch_message())
            TRAP_TO_SCHEDULER
            ;
        rtn = DialogWnd->ReturnCode == ID_OK;
        SendMessage(DialogWnd, RELEASE_MOUSE, 0, 0);
        SendMessage(DialogWnd, RELEASE_KEYBOARD, 0, 0);
        SendMessage(DialogWnd, CLOSE_WINDOW, TRUE, 0);
    }
    return rtn;
}

/* ----- return command code of in-focus control window ---- */
static int inFocusCommand(DBOX *db)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->Class)    {
        if (ct->wnd == inFocus)
            return ct->command;
        ct++;
    }
    return -1;
}

/* -------- find a specified control structure ------- */
CTLWINDOW *FindCommand(DBOX *db, enum commands cmd, int Class)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->Class)    {
        if (Class == -1 || ct->Class == Class)
            if (cmd == ct->command)
                return ct;
        ct++;
    }
    return NULL;
}

/* ---- return the window handle of a specified command ---- */
WINDOW ControlWindow(const DBOX *db, enum commands cmd)
{
    const CTLWINDOW *ct = db->ctl;
    while (ct->Class)    {
        if (ct->Class != TEXT && cmd == ct->command)
            return ct->wnd;
        ct++;
    }
    return NULL;
}

/* --- return a pointer to the control structure that matches a window --- */
CTLWINDOW *WindowControl(DBOX *db, WINDOW wnd)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->Class)    {
        if (ct->wnd == wnd)
            return ct;
        ct++;
    }
    return NULL;
}

/* ---- set a control ON or OFF ----- */
void ControlSetting(DBOX *db, enum commands cmd,
                                int Class, int setting)
{
    CTLWINDOW *ct = FindCommand(db, cmd, Class);
    if (ct != NULL) {
        ct->isetting = setting;
        if (ct->wnd != NULL)
            ct->setting = setting;
    }
}

/* ----- test if a control is on or off ----- */
BOOL isControlOn(DBOX *db, enum commands cmd, int Class)
{
    const CTLWINDOW *ct = FindCommand(db, cmd, Class);
    return ct ? (ct->wnd ? ct->setting : ct->isetting) : FALSE;
}

/* ---- return pointer to the text of a control window ---- */
char *GetDlgTextString(DBOX *db,enum commands cmd,CLASS Class)
{
    CTLWINDOW *ct = FindCommand(db, cmd, Class);
    if (ct != NULL)
        return ct->itext;
    else
        return NULL;
}

/* ------- set the text of a control specification ------ */
void SetDlgTextString(DBOX *db, enum commands cmd,
                                    char *text, CLASS Class)
{
    CTLWINDOW *ct = FindCommand(db, cmd, Class);
    if (ct != NULL)    {
        if (text != NULL)   {
            if (ct->Class == TEXT)
                ct->itext = text;  /* text may not go out of scope */
            else    {
                ct->itext = DFrealloc(ct->itext, strlen(text)+1);
                strcpy(ct->itext, text);
            }
        }
        else    {
            if (ct->Class == TEXT)
                ct->itext = "";
            else    {
                free(ct->itext);
                ct->itext = NULL;
            }
        }
        if (ct->wnd != NULL)    {
            if (text != NULL)
                SendMessage(ct->wnd, SETTEXT, (PARAM) text, 0);
            else
                SendMessage(ct->wnd, CLEARTEXT, 0, 0);
            SendMessage(ct->wnd, PAINT, 0, 0);
        }
    }
}

/* ------- set the text of a control window ------ */
void PutItemText(WINDOW wnd, enum commands cmd, char *text)
{
    CTLWINDOW *ct = FindCommand(wnd->extension, cmd, EDITBOX);

    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, TEXTBOX);
    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, COMBOBOX);
    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, LISTBOX);
    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, SPINBUTTON);
    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, TEXT);
    if (ct != NULL)        {
        WINDOW cwnd = (WINDOW) (ct->wnd);
        switch (ct->Class)    {
            case COMBOBOX:
            case EDITBOX:
                SendMessage(cwnd, CLEARTEXT, 0, 0);
                SendMessage(cwnd, ADDTEXT, (PARAM) text, 0);
                if (!isMultiLine(cwnd))
                    SendMessage(cwnd, PAINT, 0, 0);
                break;
            case LISTBOX:
            case TEXTBOX:
            case SPINBUTTON:
                SendMessage(cwnd, ADDTEXT, (PARAM) text, 0);
                break;
            case TEXT:    {
                SendMessage(cwnd, CLEARTEXT, 0, 0);
                SendMessage(cwnd, ADDTEXT, (PARAM) text, 0);
                SendMessage(cwnd, PAINT, 0, 0);
                break;
            }
            default:
                break;
        }
    }
}

/* ------- get the text of a control window ------ */
void GetItemText(WINDOW wnd, enum commands cmd,
                                char *text, int len)
{
    CTLWINDOW *ct = FindCommand(wnd->extension, cmd, EDITBOX);
    unsigned char *cp;

    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, COMBOBOX);
    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, TEXTBOX);
    if (ct == NULL)
        ct = FindCommand(wnd->extension, cmd, TEXT);
    if (ct != NULL)    {
        WINDOW cwnd = (WINDOW) (ct->wnd);
        if (cwnd != NULL)    {
            switch (ct->Class)    {
                case TEXT:
                    if (GetText(cwnd) != NULL)    {
                        cp = strchr(GetText(cwnd), '\n');
                        if (cp != NULL)
                            len = (int) (cp - GetText(cwnd));
                        strncpy(text, GetText(cwnd), len);
                        *(text+len) = '\0';
                    }
                    break;
                case TEXTBOX:
                    if (GetText(cwnd) != NULL)
                        strncpy(text, GetText(cwnd), len);
                    break;
                case COMBOBOX:
                case EDITBOX:
                    SendMessage(cwnd,GETTEXT,(PARAM)text,len);
                    break;
                default:
                    break;
            }
        }
    }
}

/* ------- set the text of a listbox control window ------ */
void GetDlgListText(WINDOW wnd, char *text, enum commands cmd)
{
    CTLWINDOW *ct = FindCommand(wnd->extension, cmd, LISTBOX);
    int sel = SendMessage(ct->wnd, LB_CURRENTSELECTION, 0, 0);
    SendMessage(ct->wnd, LB_GETTEXT, (PARAM) text, sel);
}

/* -- find control structure associated with text control -- */
static CTLWINDOW *AssociatedControl(DBOX *db,enum commands Tcmd)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->Class)    {
        if (ct->Class != TEXT)
            if (ct->command == Tcmd)
                break;
        ct++;
    }
    return ct;
}

/* --- process dialog box shortcut keys --- */
static BOOL dbShortcutKeys(DBOX *db, int ky)
{
    CTLWINDOW *ct;
    int ch = AltConvert(ky);

    if (ch != 0)    {
        ct = db->ctl;
        while (ct->Class)    {
            char *cp = ct->itext;
            while (cp && *cp)    {
                if (*cp == SHORTCUTCHAR &&
                            tolower(*(cp+1)) == ch)    {
                    if (ct->Class == TEXT)
                        ct = AssociatedControl(db, ct->command);
                    if (ct->Class == RADIOBUTTON)
                        SetRadioButton(db, ct);
                    else if (ct->Class == CHECKBOX)    {
                        ct->setting ^= ON;
                        SendMessage(ct->wnd, PAINT, 0, 0);
                    }
                    else if (ct->Class)    {
                        SendMessage(ct->wnd, SETFOCUS, TRUE, 0);
                        if (ct->Class == BUTTON)
                           SendMessage(ct->wnd,KEYBOARD,'\r',0);
                    }
                    return TRUE;
                }
                cp++;
            }
            ct++;
        }
    }
    return FALSE;
}

/* --- dynamically add or remove scroll bars
                            from a control window ---- */
void SetScrollBars(WINDOW wnd)
{
    int oldattr = GetAttribute(wnd);
    if (wnd->wlines > ClientHeight(wnd))
        AddAttribute(wnd, VSCROLLBAR);
    else 
        ClearAttribute(wnd, VSCROLLBAR);
    if (wnd->textwidth > ClientWidth(wnd))
        AddAttribute(wnd, HSCROLLBAR);
    else 
        ClearAttribute(wnd, HSCROLLBAR);
    if (GetAttribute(wnd) != oldattr)
        SendMessage(wnd, BORDER, 0, 0);
}

/* ------- CREATE_WINDOW Message (Control) ----- */
static void CtlCreateWindowMsg(WINDOW wnd)
{
    CTLWINDOW *ct;
    ct = wnd->ct = wnd->extension;
    wnd->extension = NULL;
    if (ct != NULL)
        ct->wnd = wnd;
}

/* ------- KEYBOARD Message (Control) ----- */
static BOOL CtlKeyboardMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    CTLWINDOW *ct = GetControl(wnd);
    switch ((int) p1)    {
        case F1:
            if (WindowMoving || WindowSizing)
                break;
            if (!DisplayHelp(wnd, ct->help))
                SendMessage(GetParent(wnd),COMMAND,ID_HELP,0);
            return TRUE;
        case ' ':
            if (!((int)p2 & ALTKEY))
                break;
        case ALT_F6:
        case CTRL_F4:
        case ALT_F4:
            PostMessage(GetParent(wnd), KEYBOARD, p1, p2);
            return TRUE;
        default:
            break;
    }
    if (GetClass(wnd) == EDITBOX)
        if (isMultiLine(wnd))
            return FALSE;
    if (GetClass(wnd) == TEXTBOX)
        if (WindowHeight(wnd) > 1)
            return FALSE;
    switch ((int) p1)    {
        case LARROW:
        case DN:
            if (!isDerivedFrom(wnd, LISTBOX) && !isDerivedFrom(wnd, COMBOBOX))
                p1 = '\t';

            if (isDerivedFrom(wnd, BUTTON))
                break;

            break;
        case FWD:
            if (!isDerivedFrom(wnd, EDITBOX))
                p1 = '\t';
            break;
        case '\r':
            if (isDerivedFrom(wnd, EDITBOX))
                if (isMultiLine(wnd))
                    break;
            if (isDerivedFrom(wnd, BUTTON))
                break;
            if (isDerivedFrom(wnd, LISTBOX))
                break;
            SendMessage(GetParent(wnd), COMMAND, ID_OK, 0);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

/* ------- CLOSE_WINDOW Message (Control) ----- */
static void CtlCloseWindowMsg(WINDOW wnd)
{
    CTLWINDOW *ct = GetControl(wnd);
    if (ct != NULL)    {
        ct->wnd = NULL;
        if (GetParent(wnd)->ReturnCode == ID_OK)    {
            if (ct->Class == EDITBOX || ct->Class == COMBOBOX)  {
                ct->itext=DFrealloc(ct->itext,strlen(wnd->text)+1);
                strcpy(ct->itext, wnd->text);
                if (!isMultiLine(wnd))    {
                    char *cp = ct->itext+strlen(ct->itext)-1;
                    if (*cp == '\n')
                        *cp = '\0';
                }
            }
            else if (ct->Class == RADIOBUTTON || ct->Class == CHECKBOX)
                ct->isetting = ct->setting;
        }
    }
}

static void FixColors(WINDOW wnd)
{
    CTLWINDOW *ct = wnd->ct;
    if (ct->Class != BUTTON)    {
        if (ct->Class != SPINBUTTON && ct->Class != COMBOBOX)   {
            if (ct->Class != EDITBOX && ct->Class != LISTBOX)   {
                wnd->WindowColors[FRAME_COLOR][FG] = 
                    GetParent(wnd)->WindowColors[FRAME_COLOR][FG];
                wnd->WindowColors[FRAME_COLOR][BG] = 
                    GetParent(wnd)->WindowColors[FRAME_COLOR][BG];
                wnd->WindowColors[STD_COLOR][FG] = 
                    GetParent(wnd)->WindowColors[STD_COLOR][FG];
                wnd->WindowColors[STD_COLOR][BG] = 
                    GetParent(wnd)->WindowColors[STD_COLOR][BG];
            }
        }
    }
}

/* -- generic window processor used by dialog box controls -- */
static int ControlProc(WINDOW wnd,MESSAGE msg,PARAM p1,PARAM p2)
{
    DBOX *db;

    if (wnd == NULL)
        return FALSE;
    db = GetParent(wnd) ? GetParent(wnd)->extension : NULL;

    switch (msg)    {
        case CREATE_WINDOW:
            CtlCreateWindowMsg(wnd);
            break;
        case KEYBOARD:
            if (CtlKeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case PAINT:
            FixColors(wnd);
            if (GetClass(wnd) == EDITBOX ||
                    GetClass(wnd) == LISTBOX ||
                        GetClass(wnd) == TEXTBOX)
                SetScrollBars(wnd);
            break;
        case BORDER:
            FixColors(wnd);
            if (GetClass(wnd) == EDITBOX)    {
                WINDOW oldFocus = inFocus;
                inFocus = NULL;
                DefaultWndProc(wnd, msg, p1, p2);
                inFocus = oldFocus;
                return TRUE;
            }
            break;
        case SETFOCUS:  {
            WINDOW pwnd = GetParent(wnd);
            if (p1)    {
                WINDOW oldFocus = inFocus;
                if (pwnd && GetClass(oldFocus) != APPLICATION &&
                        !isAncestor(inFocus, pwnd)) {
                    inFocus = NULL;
                    SendMessage(oldFocus, BORDER, 0, 0);
                    SendMessage(pwnd, SHOW_WINDOW, 0, 0);
                    inFocus = oldFocus;
                    ClearVisible(oldFocus);
                }
                if (GetClass(oldFocus) == APPLICATION &&
                        NextWindow(pwnd) != NULL)
                    pwnd->wasCleared = FALSE;
                DefaultWndProc(wnd, msg, p1, p2);
                SetVisible(oldFocus);
                if (pwnd != NULL)   {
                    pwnd->dfocus = wnd;
                    SendMessage(pwnd, COMMAND,
                        inFocusCommand(db), ENTERFOCUS);
                }
                return TRUE;
            }
            else
                SendMessage(pwnd, COMMAND,
                    inFocusCommand(db), LEAVEFOCUS);
            break;
        }
        case CLOSE_WINDOW:
            CtlCloseWindowMsg(wnd);
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

/* ---- change the focus to the first control --- */
static void FirstFocus(DBOX *db)
{
    CTLWINDOW *ct = db->ctl;
    if (ct != NULL) {
        while (ct->Class == TEXT || ct->Class == BOX)   {
            ct++;
            if (ct->Class == 0)
                return;
        }
        SendMessage(ct->wnd, SETFOCUS, TRUE, 0);
    }
}

/* ---- change the focus to the next control --- */
static void NextFocus(DBOX *db)
{
    CTLWINDOW *ct = WindowControl(db, inFocus);
    int looped = 0;
    if (ct != NULL) {
        do  {
            ct++;
            if (ct->Class == 0) {
                if (looped)
                    return;
                looped++;
                ct = db->ctl;
            }
        } while (ct->Class == TEXT || ct->Class == BOX);
        SendMessage(ct->wnd, SETFOCUS, TRUE, 0);
    }
}

/* ---- change the focus to the previous control --- */
static void PrevFocus(DBOX *db)
{
    CTLWINDOW *ct = WindowControl(db, inFocus);
    int looped = 0;
    if (ct != NULL) {
        do  {
            if (ct == db->ctl)  {
                if (looped)
                    return;
                looped++;
                while (ct->Class)
                    ct++;
            }
            --ct;
        } while (ct->Class == TEXT || ct->Class == BOX);
        SendMessage(ct->wnd, SETFOCUS, TRUE, 0);
    }
}

void SetFocusCursor(WINDOW wnd)
{
    if (wnd == inFocus)    {
        SendMessage(NULL, SHOW_CURSOR, 0, 0);
        SendMessage(wnd, KEYBOARD_CURSOR, 1, 0);
    }
}
