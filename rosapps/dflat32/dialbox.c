/* ----------------- dialbox.c -------------- */

#include "dflat.h"

static int inFocusCommand(DBOX *);
static void dbShortcutKeys(DBOX *, int);
static int ControlProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
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

	for (i = 0; i < dbct; i++)
	{
		CTLWINDOW *ct = (*(dbs+i))->ctl;

		while (ct->class)
		{
			if ((ct->class == EDITBOX ||
			     ct->class == COMBOBOX) &&
			    ct->itext != NULL)
			{
				free(ct->itext);
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
static int CreateWindowMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;
    CTLWINDOW *ct = db->ctl;
    DFWINDOW cwnd;
    int rtn, i;
    /* ---- build a table of processed dialog boxes ---- */
    for (i = 0; i < dbct; i++)
        if (db == dbs[i])
            break;
    if (i == dbct)    {
        dbs = DFrealloc(dbs, sizeof(DBOX *) * (dbct+1));
        *(dbs + dbct++) = db;
    }
    rtn = BaseWndProc(DIALOG, wnd, CREATE_WINDOW, p1, p2);
    ct = db->ctl;
    while (ct->class)    {
        int attrib = 0;
        if (TestAttribute(wnd, NOCLIP))
            attrib |= NOCLIP;
        if (wnd->Modal)
            attrib |= SAVESELF;
        ct->setting = ct->isetting;
        if (ct->class == EDITBOX && ct->dwnd.h > 1)
            attrib |= (MULTILINE | HASBORDER);
        else if ((ct->class == LISTBOX || ct->class == TEXTBOX) &&
				ct->dwnd.h > 2)
            attrib |= HASBORDER;
        cwnd = DfCreateWindow(ct->class,
                        ct->dwnd.title,
                        ct->dwnd.x+GetClientLeft(wnd),
                        ct->dwnd.y+GetClientTop(wnd),
                        ct->dwnd.h,
                        ct->dwnd.w,
                        ct,
                        wnd,
                        ControlProc,
                        attrib);
        if ((ct->class == EDITBOX ||
                ct->class == COMBOBOX) &&
                    ct->itext != NULL)
            DfSendMessage(cwnd, SETTEXT, (PARAM) ct->itext, 0);
        ct++;
    }
    return rtn;
}

/* -------- LEFT_BUTTON Message --------- */
static BOOL LeftButtonMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;
    CTLWINDOW *ct = db->ctl;
    if (WindowSizing || WindowMoving)
        return TRUE;
    if (HitControlBox(wnd, p1-GetLeft(wnd), p2-GetTop(wnd))) {
        DfPostMessage(wnd, KEYBOARD, ' ', ALTKEY);
        return TRUE;
    }
    while (ct->class)    {
        DFWINDOW cwnd = ct->wnd;
        if (ct->class == COMBOBOX)    {
            if (p2 == GetTop(cwnd))    {
                if (p1 == GetRight(cwnd)+1)    {
                    DfSendMessage(cwnd, LEFT_BUTTON, p1, p2);
                    return TRUE;
                }
            }
            if (GetClass(inFocus) == LISTBOX)
                DfSendMessage(wnd, SETFOCUS, TRUE, 0);
        }
        else if (ct->class == SPINBUTTON)    {
            if (p2 == GetTop(cwnd))    {
                if (p1 == GetRight(cwnd)+1 ||
                        p1 == GetRight(cwnd)+2)    {
                    DfSendMessage(cwnd, LEFT_BUTTON, p1, p2);
                    return TRUE;
                }
            }
        }
        ct++;
    }
    return FALSE;
}

/* -------- KEYBOARD Message --------- */
static BOOL KeyboardMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;
    CTLWINDOW *ct;

    if (WindowMoving || WindowSizing)
        return FALSE;
    switch ((int)p1)    {
        case F1:
            ct = GetControl(inFocus);
            if (ct != NULL)
                if (DisplayHelp(wnd, ct->help))
                    return TRUE;
            break;
        case SHIFT_HT:
        case BS:
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
            if (((int)p2 & ALTKEY) &&
                    TestAttribute(wnd, CONTROLBOX))    {
                SysMenuOpen = TRUE;
                BuildSystemMenu(wnd);
            }
            break;
        case CTRL_F4:
        case ESC:
            DfSendMessage(wnd, DFM_COMMAND, ID_CANCEL, 0);
            break;
        default:
            /* ------ search all the shortcut keys ----- */
            dbShortcutKeys(db, (int) p1);
            break;
    }
    return wnd->Modal;
}

/* -------- COMMAND Message --------- */
static BOOL CommandMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    DBOX *db = wnd->extension;
    switch ((int) p1)    {
        case ID_OK:
        case ID_CANCEL:
            if ((int)p2 != 0)
                return TRUE;
            wnd->ReturnCode = (int) p1;
            if (wnd->Modal)
                DfPostMessage(wnd, ENDDIALOG, 0, 0);
            else
                DfSendMessage(wnd, CLOSE_WINDOW, TRUE, 0);
            return TRUE;
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
int DialogProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
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
            DfSendMessage(wnd, DFM_COMMAND, inFocusCommand(db), msg);
            break;
        case DFM_COMMAND:
            if (CommandMsg(wnd, p1, p2))
                return TRUE;
            break;
        case PAINT:
            p2 = TRUE;
            break;
		case MOVE:
		case DFM_SIZE:
			rtn = BaseWndProc(DIALOG, wnd, msg, p1, p2);
			if (wnd->dfocus != NULL)
				DfSendMessage(wnd->dfocus, SETFOCUS, TRUE, 0);
			return rtn;

		case CLOSE_WINDOW:
			if (!p1)
			{
                DfSendMessage(wnd, DFM_COMMAND, ID_CANCEL, 0);
                return TRUE;
            }
            break;

        default:
            break;
    }
    return BaseWndProc(DIALOG, wnd, msg, p1, p2);
}

/* ------- create and execute a dialog box ---------- */
BOOL DfDialogBox(DFWINDOW wnd, DBOX *db, BOOL Modal,
  int (*wndproc)(struct window *, enum messages, PARAM, PARAM))
{
    BOOL rtn;
    int x = db->dwnd.x, y = db->dwnd.y;
    DFWINDOW DialogWnd;

    if (!Modal && wnd != NULL)
    {
        x += GetLeft(wnd);
        y += GetTop(wnd);
    }
    DialogWnd = DfCreateWindow(DIALOG,
                        db->dwnd.title,
                        x, y,
                        db->dwnd.h,
                        db->dwnd.w,
                        db,
                        wnd,
                        wndproc,
                        Modal ? SAVESELF : 0);
    DialogWnd->Modal = Modal;
    FirstFocus(db);
    DfPostMessage(DialogWnd, INITIATE_DIALOG, 0, 0);
    if (Modal)
    {
        DfSendMessage(DialogWnd, CAPTURE_MOUSE, 0, 0);
        DfSendMessage(DialogWnd, CAPTURE_KEYBOARD, 0, 0);
        while (DfDispatchMessage ())
            ;
        rtn = DialogWnd->ReturnCode == ID_OK;
        DfSendMessage(DialogWnd, RELEASE_MOUSE, 0, 0);
        DfSendMessage(DialogWnd, RELEASE_KEYBOARD, 0, 0);
        DfSendMessage(DialogWnd, CLOSE_WINDOW, TRUE, 0);
        return rtn;
    }
    return FALSE;
}

/* ----- return command code of in-focus control window ---- */
static int inFocusCommand(DBOX *db)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->class)    {
        if (ct->wnd == inFocus)
            return ct->command;
        ct++;
    }
    return -1;
}

/* -------- find a specified control structure ------- */
CTLWINDOW *FindCommand(DBOX *db, enum commands cmd, int class)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->class)
    {
        if (ct->class == class)
            if (cmd == ct->command)
                return ct;
        ct++;
    }
    return NULL;
}

/* ---- return the window handle of a specified command ---- */
DFWINDOW ControlWindow(DBOX *db, enum commands cmd)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->class)
    {
        if (ct->class != TEXT && cmd == ct->command)
            return ct->wnd;
        ct++;
    }
    return NULL;
}

/* --- return a pointer to the control structure that matches a window --- */
CTLWINDOW *WindowControl(DBOX *db, DFWINDOW wnd)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->class)
    {
        if (ct->wnd == wnd)
            return ct;
        ct++;
    }
    return NULL;
}

/* ---- set a control ON or OFF ----- */
void ControlSetting(DBOX *db, enum commands cmd,
                                int class, int setting)
{
    CTLWINDOW *ct = FindCommand(db, cmd, class);
    if (ct != NULL)	{
        ct->isetting = setting;
		if (ct->wnd != NULL)
			ct->setting = setting;
	}
}

/* ---- return pointer to the text of a control window ---- */
char *GetDlgTextString(DBOX *db,enum commands cmd,DFCLASS class)
{
    CTLWINDOW *ct = FindCommand(db, cmd, class);
    if (ct != NULL)
        return ct->itext;
    else
        return NULL;
}

/* ------- set the text of a control specification ------ */
void SetDlgTextString(DBOX *db, enum commands cmd,
                                    char *text, DFCLASS class)
{
    CTLWINDOW *ct = FindCommand(db, cmd, class);
    if (ct != NULL)    {
        ct->itext = DFrealloc(ct->itext, strlen(text)+1);
        strcpy(ct->itext, text);
    }
}

/* ------- set the text of a control window ------ */
void PutItemText(DFWINDOW wnd, enum commands cmd, char *text)
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
        DFWINDOW cwnd = (DFWINDOW) (ct->wnd);
        switch (ct->class)    {
            case COMBOBOX:
            case EDITBOX:
                DfSendMessage(cwnd, CLEARTEXT, 0, 0);
                DfSendMessage(cwnd, ADDTEXT, (PARAM) text, 0);
                if (!isMultiLine(cwnd))
                    DfSendMessage(cwnd, PAINT, 0, 0);
                break;
            case LISTBOX:
            case TEXTBOX:
            case SPINBUTTON:
                DfSendMessage(cwnd, ADDTEXT, (PARAM) text, 0);
                break;
            case TEXT:    {
                DfSendMessage(cwnd, CLEARTEXT, 0, 0);
                DfSendMessage(cwnd, ADDTEXT, (PARAM) text, 0);
                DfSendMessage(cwnd, PAINT, 0, 0);
                break;
            }
            default:
                break;
        }
    }
}

/* ------- get the text of a control window ------ */
void GetItemText(DFWINDOW wnd, enum commands cmd,
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
        DFWINDOW cwnd = (DFWINDOW) (ct->wnd);
        if (cwnd != NULL)    {
            switch (ct->class)    {
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
                    DfSendMessage(cwnd,GETTEXT,(PARAM)text,len);
                    break;
                default:
                    break;
            }
        }
    }
}

/* ------- set the text of a listbox control window ------ */
void GetDlgListText(DFWINDOW wnd, char *text, enum commands cmd)
{
    CTLWINDOW *ct = FindCommand(wnd->extension, cmd, LISTBOX);
    int sel = DfSendMessage(ct->wnd, LB_CURRENTSELECTION, 0, 0);
    DfSendMessage(ct->wnd, DFM_LB_GETTEXT, (PARAM) text, sel);
}

/* -- find control structure associated with text control -- */
static CTLWINDOW *AssociatedControl(DBOX *db,enum commands Tcmd)
{
    CTLWINDOW *ct = db->ctl;
    while (ct->class)    {
        if (ct->class != TEXT)
            if (ct->command == Tcmd)
                break;
        ct++;
    }
    return ct;
}

/* --- process dialog box shortcut keys --- */
static void dbShortcutKeys(DBOX *db, int ky)
{
    CTLWINDOW *ct;
    int ch = AltConvert(ky);

    if (ch != 0)    {
        ct = db->ctl;
        while (ct->class)    {
            char *cp = ct->itext;
            while (cp && *cp)    {
                if (*cp == SHORTCUTCHAR &&
                            tolower(*(cp+1)) == ch)    {
                    if (ct->class == TEXT)
                        ct = AssociatedControl(db, ct->command);
                    if (ct->class == RADIOBUTTON)
                        SetRadioButton(db, ct);
                    else if (ct->class == CHECKBOX)    {
                        ct->setting ^= ON;
                        DfSendMessage(ct->wnd, PAINT, 0, 0);
                    }
                    else if (ct->class)    {
                        DfSendMessage(ct->wnd, SETFOCUS, TRUE, 0);
                        if (ct->class == BUTTON)
                           DfSendMessage(ct->wnd,KEYBOARD,'\r',0);
                    }
                    return;
                }
                cp++;
            }
            ct++;
        }
    }
}

/* --- dynamically add or remove scroll bars
                            from a control window ---- */
void SetScrollBars(DFWINDOW wnd)
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
        DfSendMessage(wnd, BORDER, 0, 0);
}

/* ------- CREATE_WINDOW Message (Control) ----- */
static void CtlCreateWindowMsg(DFWINDOW wnd)
{
    CTLWINDOW *ct;
    ct = wnd->ct = wnd->extension;
    wnd->extension = NULL;
    if (ct != NULL)
        ct->wnd = wnd;
}

/* ------- KEYBOARD Message (Control) ----- */
static BOOL CtlKeyboardMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    CTLWINDOW *ct = GetControl(wnd);
    switch ((int) p1)    {
        case F1:
            if (WindowMoving || WindowSizing)
                break;
            if (!DisplayHelp(wnd, ct->help))
                DfSendMessage(GetParent(wnd),DFM_COMMAND,ID_HELP,0);
            return TRUE;
        case ' ':
            if (!((int)p2 & ALTKEY))
                break;
        case ALT_F6:
        case CTRL_F4:
        case ALT_F4:
            DfPostMessage(GetParent(wnd), KEYBOARD, p1, p2);
            return TRUE;
        default:
            break;
    }
    if (GetClass(wnd) == EDITBOX)
        if (isMultiLine(wnd))
            return FALSE;
    switch ((int) p1)    {
        case UP:
            if (!isDerivedFrom(wnd, LISTBOX))    {
                p1 = CTRL_FIVE;
                p2 = LEFTSHIFT;
            }
            break;
        case BS:
            if (!isDerivedFrom(wnd, EDITBOX))    {
                p1 = CTRL_FIVE;
                p2 = LEFTSHIFT;
            }
            break;
        case DN:
            if (!isDerivedFrom(wnd, LISTBOX) &&
                    !isDerivedFrom(wnd, COMBOBOX))
                p1 = '\t';
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
            DfSendMessage(GetParent(wnd), DFM_COMMAND, ID_OK, 0);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

/* ------- CLOSE_WINDOW Message (Control) ----- */
static void CtlCloseWindowMsg(DFWINDOW wnd)
{
    CTLWINDOW *ct = GetControl(wnd);
    if (ct != NULL)    {
        ct->wnd = NULL;
        if (GetParent(wnd)->ReturnCode == ID_OK)	{
            if (ct->class == EDITBOX || ct->class == COMBOBOX)	{
            	if (wnd->TextChanged)    {
                	ct->itext=DFrealloc(ct->itext,strlen(wnd->text)+1);
                	strcpy(ct->itext, wnd->text);
                	if (!isMultiLine(wnd))    {
                    	char *cp = ct->itext+strlen(ct->itext)-1;
                    	if (*cp == '\n')
                        	*cp = '\0';
                	}
            	}
			}
            else if (ct->class == RADIOBUTTON || ct->class == CHECKBOX)
                ct->isetting = ct->setting;
        }
    }
}


static void FixColors(DFWINDOW wnd)
{
	CTLWINDOW *ct = wnd->ct;

	if (ct->class != BUTTON)
	{
		if (ct->class != SPINBUTTON && ct->class != COMBOBOX)
		{
			wnd->WindowColors[FRAME_COLOR][FG] = 
				GetParent(wnd)->WindowColors[FRAME_COLOR][FG];
			wnd->WindowColors[FRAME_COLOR][BG] = 
				GetParent(wnd)->WindowColors[FRAME_COLOR][BG];
			if (ct->class != EDITBOX && ct->class != LISTBOX)
			{
				wnd->WindowColors[STD_COLOR][FG] = 
					GetParent(wnd)->WindowColors[STD_COLOR][FG];
				wnd->WindowColors[STD_COLOR][BG] = 
					GetParent(wnd)->WindowColors[STD_COLOR][BG];
			}
		}
	}
}


/* -- generic window processor used by dialog box controls -- */
static int ControlProc(DFWINDOW wnd,DFMESSAGE msg,PARAM p1,PARAM p2)
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
                DFWINDOW oldFocus = inFocus;
                inFocus = NULL;
                DefaultWndProc(wnd, msg, p1, p2);
                inFocus = oldFocus;
                return TRUE;
            }
            break;
        case SETFOCUS:	{
			DFWINDOW pwnd = GetParent(wnd);
			if (p1)
			{
				DefaultWndProc(wnd, msg, p1, p2);
				if (pwnd != NULL)
				{
					pwnd->dfocus = wnd;
					DfSendMessage(pwnd, DFM_COMMAND,
						inFocusCommand(db), ENTERFOCUS);
				}
                return TRUE;
            }
            else
                DfSendMessage(pwnd, DFM_COMMAND,
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
	if (ct != NULL)
	{
		while (ct->class == TEXT || ct->class == BOX)	{
			ct++;
			if (ct->class == 0)
				return;
		}
		DfSendMessage(ct->wnd, SETFOCUS, TRUE, 0);
	}
}

/* ---- change the focus to the next control --- */
static void NextFocus(DBOX *db)
{
	CTLWINDOW *ct = WindowControl(db, inFocus);
	int looped = 0;
	if (ct != NULL)
	{
		do
		{
			ct++;
			if (ct->class == 0)
			{
				if (looped)
					return;
				looped++;
				ct = db->ctl;
			}
		} while (ct->class == TEXT || ct->class == BOX);
		DfSendMessage(ct->wnd, SETFOCUS, TRUE, 0);
	}
}

/* ---- change the focus to the previous control --- */
static void PrevFocus(DBOX *db)
{
	CTLWINDOW *ct = WindowControl(db, inFocus);
	int looped = 0;
	if (ct != NULL)
	{
		do
		{
			if (ct == db->ctl)
			{
				if (looped)
					return;
				looped++;
				while (ct->class)
					ct++;
			}
			--ct;
		} while (ct->class == TEXT || ct->class == BOX);
		DfSendMessage(ct->wnd, SETFOCUS, TRUE, 0);
	}
}

void SetFocusCursor(DFWINDOW wnd)
{
	if (wnd == inFocus)
	{
		DfSendMessage(NULL, SHOW_CURSOR, 0, 0);
		DfSendMessage(wnd, KEYBOARD_CURSOR, 1, 0);
	}
}

/* EOF */
