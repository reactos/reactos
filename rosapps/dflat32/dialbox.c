/* ----------------- dialbox.c -------------- */

#include "dflat.h"

static int inFocusCommand(DF_DBOX *);
static void dbShortcutKeys(DF_DBOX *, int);
static int ControlProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
static void FirstFocus(DF_DBOX *db);
static void NextFocus(DF_DBOX *db);
static void PrevFocus(DF_DBOX *db);
static DF_CTLWINDOW *AssociatedControl(DF_DBOX *, enum DfCommands);

static BOOL SysMenuOpen;

static DF_DBOX **dbs = NULL;
static int dbct = 0;

/* --- clear all heap allocations to control text fields --- */
void DfClearDialogBoxes(void)
{
	int i;

	for (i = 0; i < dbct; i++)
	{
		DF_CTLWINDOW *ct = (*(dbs+i))->ctl;

		while (ct->class)
		{
			if ((ct->class == DF_EDITBOX ||
			     ct->class == DF_COMBOBOX) &&
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


/* -------- DFM_CREATE_WINDOW Message --------- */
static int CreateWindowMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    DF_DBOX *db = wnd->extension;
    DF_CTLWINDOW *ct = db->ctl;
    DFWINDOW cwnd;
    int rtn, i;
    /* ---- build a table of processed dialog boxes ---- */
    for (i = 0; i < dbct; i++)
        if (db == dbs[i])
            break;
    if (i == dbct)    {
        dbs = DfRealloc(dbs, sizeof(DF_DBOX *) * (dbct+1));
        *(dbs + dbct++) = db;
    }
    rtn = DfBaseWndProc(DF_DIALOG, wnd, DFM_CREATE_WINDOW, p1, p2);
    ct = db->ctl;
    while (ct->class)    {
        int attrib = 0;
        if (DfTestAttribute(wnd, DF_NOCLIP))
            attrib |= DF_NOCLIP;
        if (wnd->Modal)
            attrib |= DF_SAVESELF;
        ct->setting = ct->isetting;
        if (ct->class == DF_EDITBOX && ct->dwnd.h > 1)
            attrib |= (DF_MULTILINE | DF_HASBORDER);
        else if ((ct->class == DF_LISTBOX || ct->class == DF_TEXTBOX) &&
				ct->dwnd.h > 2)
            attrib |= DF_HASBORDER;
        cwnd = DfDfCreateWindow(ct->class,
                        ct->dwnd.title,
                        ct->dwnd.x+DfGetClientLeft(wnd),
                        ct->dwnd.y+DfGetClientTop(wnd),
                        ct->dwnd.h,
                        ct->dwnd.w,
                        ct,
                        wnd,
                        ControlProc,
                        attrib);
        if ((ct->class == DF_EDITBOX ||
                ct->class == DF_COMBOBOX) &&
                    ct->itext != NULL)
            DfSendMessage(cwnd, DFM_SETTEXT, (DF_PARAM) ct->itext, 0);
        ct++;
    }
    return rtn;
}

/* -------- DFM_LEFT_BUTTON Message --------- */
static BOOL LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    DF_DBOX *db = wnd->extension;
    DF_CTLWINDOW *ct = db->ctl;
    if (DfWindowSizing || DfWindowMoving)
        return TRUE;
    if (DfHitControlBox(wnd, p1-DfGetLeft(wnd), p2-DfGetTop(wnd))) {
        DfPostMessage(wnd, DFM_KEYBOARD, ' ', DF_ALTKEY);
        return TRUE;
    }
    while (ct->class)    {
        DFWINDOW cwnd = ct->wnd;
        if (ct->class == DF_COMBOBOX)    {
            if (p2 == DfGetTop(cwnd))    {
                if (p1 == DfGetRight(cwnd)+1)    {
                    DfSendMessage(cwnd, DFM_LEFT_BUTTON, p1, p2);
                    return TRUE;
                }
            }
            if (DfGetClass(DfInFocus) == DF_LISTBOX)
                DfSendMessage(wnd, DFM_SETFOCUS, TRUE, 0);
        }
        else if (ct->class == DF_SPINBUTTON)    {
            if (p2 == DfGetTop(cwnd))    {
                if (p1 == DfGetRight(cwnd)+1 ||
                        p1 == DfGetRight(cwnd)+2)    {
                    DfSendMessage(cwnd, DFM_LEFT_BUTTON, p1, p2);
                    return TRUE;
                }
            }
        }
        ct++;
    }
    return FALSE;
}

/* -------- DFM_KEYBOARD Message --------- */
static BOOL KeyboardMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    DF_DBOX *db = wnd->extension;
    DF_CTLWINDOW *ct;

    if (DfWindowMoving || DfWindowSizing)
        return FALSE;
    switch ((int)p1)    {
        case DF_F1:
            ct = DfGetControl(DfInFocus);
            if (ct != NULL)
                if (DfDisplayHelp(wnd, ct->help))
                    return TRUE;
            break;
        case DF_SHIFT_HT:
        case DF_BS:
        case DF_UP:
            PrevFocus(db);
            break;
        case DF_ALT_F6:
        case '\t':
        case DF_FWD:
        case DF_DN:
            NextFocus(db);
            break;
        case ' ':
            if (((int)p2 & DF_ALTKEY) &&
                    DfTestAttribute(wnd, DF_CONTROLBOX))    {
                SysMenuOpen = TRUE;
                DfBuildSystemMenu(wnd);
            }
            break;
        case DF_CTRL_F4:
        case DF_ESC:
            DfSendMessage(wnd, DFM_COMMAND, DF_ID_CANCEL, 0);
            break;
        default:
            /* ------ search all the shortcut keys ----- */
            dbShortcutKeys(db, (int) p1);
            break;
    }
    return wnd->Modal;
}

/* -------- COMMAND Message --------- */
static BOOL CommandMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    DF_DBOX *db = wnd->extension;
    switch ((int) p1)    {
        case DF_ID_OK:
        case DF_ID_CANCEL:
            if ((int)p2 != 0)
                return TRUE;
            wnd->ReturnCode = (int) p1;
            if (wnd->Modal)
                DfPostMessage(wnd, DFM_ENDDIALOG, 0, 0);
            else
                DfSendMessage(wnd, DFM_CLOSE_WINDOW, TRUE, 0);
            return TRUE;
        case DF_ID_HELP:
            if ((int)p2 != 0)
                return TRUE;
            return DfDisplayHelp(wnd, db->HelpName);
        default:
            break;
    }
    return FALSE;
}

/* ----- window-processing module, DF_DIALOG window class ----- */
int DfDialogProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
	int rtn;
    DF_DBOX *db = wnd->extension;

    switch (msg)    {
        case DFM_CREATE_WINDOW:
            return CreateWindowMsg(wnd, p1, p2);
        case DFM_SHIFT_CHANGED:
            if (wnd->Modal)
                return TRUE;
            break;
        case DFM_LEFT_BUTTON:
            if (LeftButtonMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_CLOSE_POPDOWN:
            SysMenuOpen = FALSE;
            break;
        case DFM_LB_SELECTION:
        case DFM_LB_CHOOSE:
            if (SysMenuOpen)
                return TRUE;
            DfSendMessage(wnd, DFM_COMMAND, inFocusCommand(db), msg);
            break;
        case DFM_COMMAND:
            if (CommandMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_PAINT:
            p2 = TRUE;
            break;
		case DFM_MOVE:
		case DFM_DFM_SIZE:
			rtn = DfBaseWndProc(DF_DIALOG, wnd, msg, p1, p2);
			if (wnd->dfocus != NULL)
				DfSendMessage(wnd->dfocus, DFM_SETFOCUS, TRUE, 0);
			return rtn;

		case DFM_CLOSE_WINDOW:
			if (!p1)
			{
                DfSendMessage(wnd, DFM_COMMAND, DF_ID_CANCEL, 0);
                return TRUE;
            }
            break;

        default:
            break;
    }
    return DfBaseWndProc(DF_DIALOG, wnd, msg, p1, p2);
}

/* ------- create and execute a dialog box ---------- */
BOOL DfDialogBox(DFWINDOW wnd, DF_DBOX *db, BOOL Modal,
  int (*wndproc)(struct DfWindow *, enum DfMessages, DF_PARAM, DF_PARAM))
{
    BOOL rtn;
    int x = db->dwnd.x, y = db->dwnd.y;
    DFWINDOW DialogWnd;

    if (!Modal && wnd != NULL)
    {
        x += DfGetLeft(wnd);
        y += DfGetTop(wnd);
    }
    DialogWnd = DfDfCreateWindow(DF_DIALOG,
                        db->dwnd.title,
                        x, y,
                        db->dwnd.h,
                        db->dwnd.w,
                        db,
                        wnd,
                        wndproc,
                        Modal ? DF_SAVESELF : 0);
    DialogWnd->Modal = Modal;
    FirstFocus(db);
    DfPostMessage(DialogWnd, DFM_INITIATE_DIALOG, 0, 0);
    if (Modal)
    {
        DfSendMessage(DialogWnd, DFM_CAPTURE_MOUSE, 0, 0);
        DfSendMessage(DialogWnd, DFM_CAPTURE_KEYBOARD, 0, 0);
        while (DfDispatchMessage ())
            ;
        rtn = DialogWnd->ReturnCode == DF_ID_OK;
        DfSendMessage(DialogWnd, DFM_RELEASE_MOUSE, 0, 0);
        DfSendMessage(DialogWnd, DFM_RELEASE_KEYBOARD, 0, 0);
        DfSendMessage(DialogWnd, DFM_CLOSE_WINDOW, TRUE, 0);
        return rtn;
    }
    return FALSE;
}

/* ----- return command code of in-focus control window ---- */
static int inFocusCommand(DF_DBOX *db)
{
    DF_CTLWINDOW *ct = db->ctl;
    while (ct->class)    {
        if (ct->wnd == DfInFocus)
            return ct->command;
        ct++;
    }
    return -1;
}

/* -------- find a specified control structure ------- */
DF_CTLWINDOW *DfFindCommand(DF_DBOX *db, enum DfCommands cmd, int class)
{
    DF_CTLWINDOW *ct = db->ctl;
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
DFWINDOW DfControlWindow(DF_DBOX *db, enum DfCommands cmd)
{
    DF_CTLWINDOW *ct = db->ctl;
    while (ct->class)
    {
        if (ct->class != DF_TEXT && cmd == ct->command)
            return ct->wnd;
        ct++;
    }
    return NULL;
}

/* --- return a pointer to the control structure that matches a window --- */
DF_CTLWINDOW *WindowControl(DF_DBOX *db, DFWINDOW wnd)
{
    DF_CTLWINDOW *ct = db->ctl;
    while (ct->class)
    {
        if (ct->wnd == wnd)
            return ct;
        ct++;
    }
    return NULL;
}

/* ---- set a control DF_ON or DF_OFF ----- */
void DfControlSetting(DF_DBOX *db, enum DfCommands cmd,
                                int class, int setting)
{
    DF_CTLWINDOW *ct = DfFindCommand(db, cmd, class);
    if (ct != NULL)	{
        ct->isetting = setting;
		if (ct->wnd != NULL)
			ct->setting = setting;
	}
}

/* ---- return pointer to the text of a control window ---- */
char *DfGetDlgTextString(DF_DBOX *db,enum DfCommands cmd,DFCLASS class)
{
    DF_CTLWINDOW *ct = DfFindCommand(db, cmd, class);
    if (ct != NULL)
        return ct->itext;
    else
        return NULL;
}

/* ------- set the text of a control specification ------ */
void DfSetDlgTextString(DF_DBOX *db, enum DfCommands cmd,
                                    char *text, DFCLASS class)
{
    DF_CTLWINDOW *ct = DfFindCommand(db, cmd, class);
    if (ct != NULL)    {
        ct->itext = DfRealloc(ct->itext, strlen(text)+1);
        strcpy(ct->itext, text);
    }
}

/* ------- set the text of a control window ------ */
void DfPutItemText(DFWINDOW wnd, enum DfCommands cmd, char *text)
{
    DF_CTLWINDOW *ct = DfFindCommand(wnd->extension, cmd, DF_EDITBOX);

    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_TEXTBOX);
    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_COMBOBOX);
    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_LISTBOX);
    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_SPINBUTTON);
    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_TEXT);
    if (ct != NULL)        {
        DFWINDOW cwnd = (DFWINDOW) (ct->wnd);
        switch (ct->class)    {
            case DF_COMBOBOX:
            case DF_EDITBOX:
                DfSendMessage(cwnd, DFM_CLEARTEXT, 0, 0);
                DfSendMessage(cwnd, DFM_ADDTEXT, (DF_PARAM) text, 0);
                if (!DfIsMultiLine(cwnd))
                    DfSendMessage(cwnd, DFM_PAINT, 0, 0);
                break;
            case DF_LISTBOX:
            case DF_TEXTBOX:
            case DF_SPINBUTTON:
                DfSendMessage(cwnd, DFM_ADDTEXT, (DF_PARAM) text, 0);
                break;
            case DF_TEXT:    {
                DfSendMessage(cwnd, DFM_CLEARTEXT, 0, 0);
                DfSendMessage(cwnd, DFM_ADDTEXT, (DF_PARAM) text, 0);
                DfSendMessage(cwnd, DFM_PAINT, 0, 0);
                break;
            }
            default:
                break;
        }
    }
}

/* ------- get the text of a control window ------ */
void DfGetItemText(DFWINDOW wnd, enum DfCommands cmd,
                                char *text, int len)
{
    DF_CTLWINDOW *ct = DfFindCommand(wnd->extension, cmd, DF_EDITBOX);
    unsigned char *cp;

    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_COMBOBOX);
    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_TEXTBOX);
    if (ct == NULL)
        ct = DfFindCommand(wnd->extension, cmd, DF_TEXT);
    if (ct != NULL)    {
        DFWINDOW cwnd = (DFWINDOW) (ct->wnd);
        if (cwnd != NULL)    {
            switch (ct->class)    {
                case DF_TEXT:
                    if (DfGetText(cwnd) != NULL)    {
                        cp = strchr(DfGetText(cwnd), '\n');
                        if (cp != NULL)
                            len = (int) (cp - DfGetText(cwnd));
                        strncpy(text, DfGetText(cwnd), len);
                        *(text+len) = '\0';
                    }
                    break;
                case DF_TEXTBOX:
                    if (DfGetText(cwnd) != NULL)
                        strncpy(text, DfGetText(cwnd), len);
                    break;
                case DF_COMBOBOX:
                case DF_EDITBOX:
                    DfSendMessage(cwnd,DFM_GETTEXT,(DF_PARAM)text,len);
                    break;
                default:
                    break;
            }
        }
    }
}

/* ------- set the text of a listbox control window ------ */
void DfGetDlgListText(DFWINDOW wnd, char *text, enum DfCommands cmd)
{
    DF_CTLWINDOW *ct = DfFindCommand(wnd->extension, cmd, DF_LISTBOX);
    int sel = DfSendMessage(ct->wnd, DFM_LB_CURRENTSELECTION, 0, 0);
    DfSendMessage(ct->wnd, DFM_LB_GETTEXT, (DF_PARAM) text, sel);
}

/* -- find control structure associated with text control -- */
static DF_CTLWINDOW *AssociatedControl(DF_DBOX *db,enum DfCommands Tcmd)
{
    DF_CTLWINDOW *ct = db->ctl;
    while (ct->class)    {
        if (ct->class != DF_TEXT)
            if (ct->command == Tcmd)
                break;
        ct++;
    }
    return ct;
}

/* --- process dialog box shortcut keys --- */
static void dbShortcutKeys(DF_DBOX *db, int ky)
{
    DF_CTLWINDOW *ct;
    int ch = DfAltConvert(ky);

    if (ch != 0)    {
        ct = db->ctl;
        while (ct->class)    {
            char *cp = ct->itext;
            while (cp && *cp)    {
                if (*cp == DF_SHORTCUTCHAR &&
                            tolower(*(cp+1)) == ch)    {
                    if (ct->class == DF_TEXT)
                        ct = AssociatedControl(db, ct->command);
                    if (ct->class == DF_RADIOBUTTON)
                        DfSetRadioButton(db, ct);
                    else if (ct->class == DF_CHECKBOX)    {
                        ct->setting ^= DF_ON;
                        DfSendMessage(ct->wnd, DFM_PAINT, 0, 0);
                    }
                    else if (ct->class)    {
                        DfSendMessage(ct->wnd, DFM_SETFOCUS, TRUE, 0);
                        if (ct->class == DF_BUTTON)
                           DfSendMessage(ct->wnd,DFM_KEYBOARD,'\r',0);
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
void DfSetScrollBars(DFWINDOW wnd)
{
    int oldattr = DfGetAttribute(wnd);
    if (wnd->wlines > DfClientHeight(wnd))
        DfAddAttribute(wnd, DF_VSCROLLBAR);
    else 
        DfClearAttribute(wnd, DF_VSCROLLBAR);
    if (wnd->textwidth > DfClientWidth(wnd))
        DfAddAttribute(wnd, DF_HSCROLLBAR);
    else 
        DfClearAttribute(wnd, DF_HSCROLLBAR);
    if (DfGetAttribute(wnd) != oldattr)
        DfSendMessage(wnd, DFM_BORDER, 0, 0);
}

/* ------- DFM_CREATE_WINDOW Message (Control) ----- */
static void CtlCreateWindowMsg(DFWINDOW wnd)
{
    DF_CTLWINDOW *ct;
    ct = wnd->ct = wnd->extension;
    wnd->extension = NULL;
    if (ct != NULL)
        ct->wnd = wnd;
}

/* ------- DFM_KEYBOARD Message (Control) ----- */
static BOOL CtlKeyboardMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    DF_CTLWINDOW *ct = DfGetControl(wnd);
    switch ((int) p1)    {
        case DF_F1:
            if (DfWindowMoving || DfWindowSizing)
                break;
            if (!DfDisplayHelp(wnd, ct->help))
                DfSendMessage(DfGetParent(wnd),DFM_COMMAND,DF_ID_HELP,0);
            return TRUE;
        case ' ':
            if (!((int)p2 & DF_ALTKEY))
                break;
        case DF_ALT_F6:
        case DF_CTRL_F4:
        case DF_ALT_F4:
            DfPostMessage(DfGetParent(wnd), DFM_KEYBOARD, p1, p2);
            return TRUE;
        default:
            break;
    }
    if (DfGetClass(wnd) == DF_EDITBOX)
        if (DfIsMultiLine(wnd))
            return FALSE;
    switch ((int) p1)    {
        case DF_UP:
            if (!DfIsDerivedFrom(wnd, DF_LISTBOX))    {
                p1 = DF_CTRL_FIVE;
                p2 = DF_LEFTSHIFT;
            }
            break;
        case DF_BS:
            if (!DfIsDerivedFrom(wnd, DF_EDITBOX))    {
                p1 = DF_CTRL_FIVE;
                p2 = DF_LEFTSHIFT;
            }
            break;
        case DF_DN:
            if (!DfIsDerivedFrom(wnd, DF_LISTBOX) &&
                    !DfIsDerivedFrom(wnd, DF_COMBOBOX))
                p1 = '\t';
            break;
        case DF_FWD:
            if (!DfIsDerivedFrom(wnd, DF_EDITBOX))
                p1 = '\t';
            break;
        case '\r':
            if (DfIsDerivedFrom(wnd, DF_EDITBOX))
                if (DfIsMultiLine(wnd))
                    break;
            if (DfIsDerivedFrom(wnd, DF_BUTTON))
                break;
            DfSendMessage(DfGetParent(wnd), DFM_COMMAND, DF_ID_OK, 0);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

/* ------- DFM_CLOSE_WINDOW Message (Control) ----- */
static void CtlCloseWindowMsg(DFWINDOW wnd)
{
    DF_CTLWINDOW *ct = DfGetControl(wnd);
    if (ct != NULL)    {
        ct->wnd = NULL;
        if (DfGetParent(wnd)->ReturnCode == DF_ID_OK)	{
            if (ct->class == DF_EDITBOX || ct->class == DF_COMBOBOX)	{
            	if (wnd->TextChanged)    {
                	ct->itext=DfRealloc(ct->itext,strlen(wnd->text)+1);
                	strcpy(ct->itext, wnd->text);
                	if (!DfIsMultiLine(wnd))    {
                    	char *cp = ct->itext+strlen(ct->itext)-1;
                    	if (*cp == '\n')
                        	*cp = '\0';
                	}
            	}
			}
            else if (ct->class == DF_RADIOBUTTON || ct->class == DF_CHECKBOX)
                ct->isetting = ct->setting;
        }
    }
}


static void FixColors(DFWINDOW wnd)
{
	DF_CTLWINDOW *ct = wnd->ct;

	if (ct->class != DF_BUTTON)
	{
		if (ct->class != DF_SPINBUTTON && ct->class != DF_COMBOBOX)
		{
			wnd->WindowColors[DF_FRAME_COLOR][DF_FG] = 
				DfGetParent(wnd)->WindowColors[DF_FRAME_COLOR][DF_FG];
			wnd->WindowColors[DF_FRAME_COLOR][DF_BG] = 
				DfGetParent(wnd)->WindowColors[DF_FRAME_COLOR][DF_BG];
			if (ct->class != DF_EDITBOX && ct->class != DF_LISTBOX)
			{
				wnd->WindowColors[DF_STD_COLOR][DF_FG] = 
					DfGetParent(wnd)->WindowColors[DF_STD_COLOR][DF_FG];
				wnd->WindowColors[DF_STD_COLOR][DF_BG] = 
					DfGetParent(wnd)->WindowColors[DF_STD_COLOR][DF_BG];
			}
		}
	}
}


/* -- generic window processor used by dialog box controls -- */
static int ControlProc(DFWINDOW wnd,DFMESSAGE msg,DF_PARAM p1,DF_PARAM p2)
{
    DF_DBOX *db;

    if (wnd == NULL)
        return FALSE;
    db = DfGetParent(wnd) ? DfGetParent(wnd)->extension : NULL;

    switch (msg)    {
        case DFM_CREATE_WINDOW:
            CtlCreateWindowMsg(wnd);
            break;
        case DFM_KEYBOARD:
            if (CtlKeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_PAINT:
			FixColors(wnd);
            if (DfGetClass(wnd) == DF_EDITBOX ||
                    DfGetClass(wnd) == DF_LISTBOX ||
                        DfGetClass(wnd) == DF_TEXTBOX)
                DfSetScrollBars(wnd);
            break;
        case DFM_BORDER:
			FixColors(wnd);
            if (DfGetClass(wnd) == DF_EDITBOX)    {
                DFWINDOW oldFocus = DfInFocus;
                DfInFocus = NULL;
                DfDefaultWndProc(wnd, msg, p1, p2);
                DfInFocus = oldFocus;
                return TRUE;
            }
            break;
        case DFM_SETFOCUS:	{
			DFWINDOW pwnd = DfGetParent(wnd);
			if (p1)
			{
				DfDefaultWndProc(wnd, msg, p1, p2);
				if (pwnd != NULL)
				{
					pwnd->dfocus = wnd;
					DfSendMessage(pwnd, DFM_COMMAND,
						inFocusCommand(db), DFM_ENTERFOCUS);
				}
                return TRUE;
            }
            else
                DfSendMessage(pwnd, DFM_COMMAND,
                    inFocusCommand(db), DFM_LEAVEFOCUS);
            break;
		}
        case DFM_CLOSE_WINDOW:
            CtlCloseWindowMsg(wnd);
            break;
        default:
            break;
    }
    return DfDefaultWndProc(wnd, msg, p1, p2);
}

/* ---- change the focus to the first control --- */
static void FirstFocus(DF_DBOX *db)
{
	DF_CTLWINDOW *ct = db->ctl;
	if (ct != NULL)
	{
		while (ct->class == DF_TEXT || ct->class == DF_BOX)	{
			ct++;
			if (ct->class == 0)
				return;
		}
		DfSendMessage(ct->wnd, DFM_SETFOCUS, TRUE, 0);
	}
}

/* ---- change the focus to the next control --- */
static void NextFocus(DF_DBOX *db)
{
	DF_CTLWINDOW *ct = WindowControl(db, DfInFocus);
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
		} while (ct->class == DF_TEXT || ct->class == DF_BOX);
		DfSendMessage(ct->wnd, DFM_SETFOCUS, TRUE, 0);
	}
}

/* ---- change the focus to the previous control --- */
static void PrevFocus(DF_DBOX *db)
{
	DF_CTLWINDOW *ct = WindowControl(db, DfInFocus);
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
		} while (ct->class == DF_TEXT || ct->class == DF_BOX);
		DfSendMessage(ct->wnd, DFM_SETFOCUS, TRUE, 0);
	}
}

void DfSetFocusCursor(DFWINDOW wnd)
{
	if (wnd == DfInFocus)
	{
		DfSendMessage(NULL, DFM_SHOW_CURSOR, 0, 0);
		DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, 1, 0);
	}
}

/* EOF */
