/* ------------- editbox.c ------------ */
#include "dflat32/dflat.h"

#define EditBufLen(wnd) (isMultiLine(wnd) ? EDITLEN : ENTRYLEN)
#define SetLinePointer(wnd, ln) (wnd->CurrLine = ln)
#define isWhite(c)     ((c)==' '||(c)=='\n')
/* ---------- local prototypes ----------- */
static void SaveDeletedText(DFWINDOW, char *, int);
static void Forward(DFWINDOW);
static void Backward(DFWINDOW);
static void End(DFWINDOW);
static void Home(DFWINDOW);
static void Downward(DFWINDOW);
static void Upward(DFWINDOW);
static void StickEnd(DFWINDOW);
static void NextWord(DFWINDOW);
static void PrevWord(DFWINDOW);
static void ModTextPointers(DFWINDOW, int, int);
static void SetAnchor(DFWINDOW, int, int);
/* -------- local variables -------- */
static BOOL KeyBoardMarking, ButtonDown;
static BOOL TextMarking;
static int ButtonX, ButtonY;
static int PrevY = -1;

/* ----------- CREATE_WINDOW Message ---------- */
static int CreateWindowMsg(DFWINDOW wnd)
{
    int rtn = BaseWndProc(EDITBOX, wnd, CREATE_WINDOW, 0, 0);
    wnd->MaxTextLength = MAXTEXTLEN+1;
    wnd->textlen = EditBufLen(wnd);
    wnd->InsertMode = TRUE;
	DfSendMessage(wnd, CLEARTEXT, 0, 0);
    return rtn;
}
/* ----------- SETTEXT Message ---------- */
static int SetTextMsg(DFWINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    if (strlen((char *)p1) <= wnd->MaxTextLength)
        rtn = BaseWndProc(EDITBOX, wnd, SETTEXT, p1, 0);
    return rtn;
}
/* ----------- CLEARTEXT Message ------------ */
static int ClearTextMsg(DFWINDOW wnd)
{
    int rtn = BaseWndProc(EDITBOX, wnd, CLEARTEXT, 0, 0);
    unsigned blen = EditBufLen(wnd)+2;
    wnd->text = DFrealloc(wnd->text, blen);
    memset(wnd->text, 0, blen);
    wnd->wlines = 0;
    wnd->CurrLine = 0;
    wnd->CurrCol = 0;
    wnd->WndRow = 0;
    wnd->wleft = 0;
    wnd->wtop = 0;
    wnd->textwidth = 0;
    wnd->TextChanged = FALSE;
    return rtn;
}
/* ----------- ADDTEXT Message ---------- */
static int AddTextMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn = FALSE;
    if (strlen((char *)p1)+wnd->textlen <= wnd->MaxTextLength) {
        rtn = BaseWndProc(EDITBOX, wnd, ADDTEXT, p1, p2);
        if (rtn != FALSE)    {
            if (!isMultiLine(wnd))    {
                wnd->CurrLine = 0;
                wnd->CurrCol = strlen((char *)p1);
                if (wnd->CurrCol >= ClientWidth(wnd))    {
                    wnd->wleft = wnd->CurrCol-ClientWidth(wnd);
                    wnd->CurrCol -= wnd->wleft;
                }
                wnd->BlkEndCol = wnd->CurrCol;
                DfSendMessage(wnd, KEYBOARD_CURSOR,
                                     WndCol, wnd->WndRow);
            }
        }
    }
    return rtn;
}
/* ----------- GETTEXT Message ---------- */
static int GetTextMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    char *cp1 = (char *)p1;
    char *cp2 = wnd->text;
    if (cp2 != NULL)    {
        while (p2-- && *cp2 && *cp2 != '\n')
            *cp1++ = *cp2++;
        *cp1 = '\0';
        return TRUE;
    }
    return FALSE;
}
/* ----------- SETTEXTLENGTH Message ---------- */
static int SetTextLengthMsg(DFWINDOW wnd, unsigned int len)
{
    if (++len < MAXTEXTLEN)    {
        wnd->MaxTextLength = len;
        if (len < wnd->textlen)    {
            wnd->text=DFrealloc(wnd->text, len+2);
            wnd->textlen = len;
            *((wnd->text)+len) = '\0';
            *((wnd->text)+len+1) = '\0';
            BuildTextPointers(wnd);
        }
        return TRUE;
    }
    return FALSE;
}
/* ----------- KEYBOARD_CURSOR Message ---------- */
static void KeyboardCursorMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    wnd->CurrCol = (int)p1 + wnd->wleft;
    wnd->WndRow = (int)p2;
    wnd->CurrLine = (int)p2 + wnd->wtop;
    if (wnd == inFocus)	{
		if (CharInView(wnd, (int)p1, (int)p2))
	        DfSendMessage(NULL, SHOW_CURSOR,
				(wnd->InsertMode && !TextMarking), 0);
    	else
			DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
	}
}
/* ----------- SIZE Message ---------- */
int SizeMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn = BaseWndProc(EDITBOX, wnd, DFM_SIZE, p1, p2);
    if (WndCol > ClientWidth(wnd)-1)
        wnd->CurrCol = ClientWidth(wnd)-1 + wnd->wleft;
    if (wnd->WndRow > ClientHeight(wnd)-1)    {
        wnd->WndRow = ClientHeight(wnd)-1;
        SetLinePointer(wnd, wnd->WndRow+wnd->wtop);
    }
    DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return rtn;
}
/* ----------- SCROLL Message ---------- */
static int ScrollMsg(DFWINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    if (isMultiLine(wnd))    {
        rtn = BaseWndProc(EDITBOX,wnd,SCROLL,p1,0);
        if (rtn != FALSE)    {
            if (p1)    {
                /* -------- scrolling up --------- */
                if (wnd->WndRow == 0)    {
                    wnd->CurrLine++;
                    StickEnd(wnd);
                }
                else
                    --wnd->WndRow;
            }
            else    {
                /* -------- scrolling down --------- */
                if (wnd->WndRow == ClientHeight(wnd)-1)    {
                    if (wnd->CurrLine > 0)
                        --wnd->CurrLine;
                    StickEnd(wnd);
                }
                else
                    wnd->WndRow++;
            }
            DfSendMessage(wnd,KEYBOARD_CURSOR,WndCol,wnd->WndRow);
        }
    }
    return rtn;
}
/* ----------- HORIZSCROLL Message ---------- */
static int HorizScrollMsg(DFWINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    char *currchar = CurrChar;
    if (!(p1 &&
            wnd->CurrCol == wnd->wleft && *currchar == '\n'))  {
        rtn = BaseWndProc(EDITBOX, wnd, HORIZSCROLL, p1, 0);
        if (rtn != FALSE)    {
            if (wnd->CurrCol < wnd->wleft)
                wnd->CurrCol++;
            else if (WndCol == ClientWidth(wnd))
                --wnd->CurrCol;
            DfSendMessage(wnd,KEYBOARD_CURSOR,WndCol,wnd->WndRow);
        }
    }
    return rtn;
}
/* ----------- SCROLLPAGE Message ---------- */
static int ScrollPageMsg(DFWINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    if (isMultiLine(wnd))    {
        rtn = BaseWndProc(EDITBOX, wnd, SCROLLPAGE, p1, 0);
        SetLinePointer(wnd, wnd->wtop+wnd->WndRow);
        StickEnd(wnd);
        DfSendMessage(wnd, KEYBOARD_CURSOR,WndCol, wnd->WndRow);
    }
    return rtn;
}
/* ----------- HORIZSCROLLPAGE Message ---------- */
static int HorizPageMsg(DFWINDOW wnd, PARAM p1)
{
    int rtn = BaseWndProc(EDITBOX, wnd, HORIZPAGE, p1, 0);
    if ((int) p1 == FALSE)    {
        if (wnd->CurrCol > wnd->wleft+ClientWidth(wnd)-1)
            wnd->CurrCol = wnd->wleft+ClientWidth(wnd)-1;
    }
    else if (wnd->CurrCol < wnd->wleft)
        wnd->CurrCol = wnd->wleft;
    DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return rtn;
}
/* ----- Extend the marked block to the new x,y position ---- */
static void ExtendBlock(DFWINDOW wnd, int x, int y)
{
    int bbl, bel;
    int ptop = min(wnd->BlkBegLine, wnd->BlkEndLine);
    int pbot = max(wnd->BlkBegLine, wnd->BlkEndLine);
    char *lp = TextLine(wnd, wnd->wtop+y);
    int len = (int) (strchr(lp, '\n') - lp);
    x = max(0, min(x, len));
    y = max(0, y);
    wnd->BlkEndCol = min(len, x+wnd->wleft);
    wnd->BlkEndLine = y+wnd->wtop;
    DfSendMessage(wnd, KEYBOARD_CURSOR, wnd->BlkEndCol, wnd->BlkEndLine);
    bbl = min(wnd->BlkBegLine, wnd->BlkEndLine);
    bel = max(wnd->BlkBegLine, wnd->BlkEndLine);
    while (ptop < bbl)    {
        WriteTextLine(wnd, NULL, ptop, FALSE);
        ptop++;
    }
    for (y = bbl; y <= bel; y++)
        WriteTextLine(wnd, NULL, y, FALSE);
    while (pbot > bel)    {
        WriteTextLine(wnd, NULL, pbot, FALSE);
        --pbot;
    }
}
/* ----------- LEFT_BUTTON Message ---------- */
static int LeftButtonMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int MouseX = (int) p1 - GetClientLeft(wnd);
    int MouseY = (int) p2 - GetClientTop(wnd);
    DFRECT rc = ClientRect(wnd);
    char *lp;
    int len;
    if (KeyBoardMarking)
        return TRUE;
    if (WindowMoving || WindowSizing)
        return FALSE;
    if (isMultiLine(wnd))    {
        if (TextMarking)    {
            if (!InsideRect(p1, p2, rc))    {
				int x = MouseX, y = MouseY;
				int dir;
				DFMESSAGE msg = 0;
                if ((int)p2 == GetTop(wnd))
					y++, dir = FALSE, msg = SCROLL;
                else if ((int)p2 == GetBottom(wnd))
					--y, dir = TRUE, msg = SCROLL;
                else if ((int)p1 == GetLeft(wnd))
					--x, dir = FALSE, msg = HORIZSCROLL;
                else if ((int)p1 == GetRight(wnd))
					x++, dir = TRUE, msg = HORIZSCROLL;
				if (msg != 0)	{
                    if (DfSendMessage(wnd, msg, dir, 0))
                        ExtendBlock(wnd, x, y);
	                DfSendMessage(wnd, PAINT, 0, 0);
				}
            }
            return TRUE;
        }
        if (!InsideRect(p1, p2, rc))
            return FALSE;
        if (TextBlockMarked(wnd))    {
            ClearTextBlock(wnd);
            DfSendMessage(wnd, PAINT, 0, 0);
        }
        if (wnd->wlines)    {
            if (MouseY > wnd->wlines-1)
                return TRUE;
            lp = TextLine(wnd, MouseY+wnd->wtop);
            len = (int) (strchr(lp, '\n') - lp);
            MouseX = min(MouseX, len);
            if (MouseX < wnd->wleft)    {
                MouseX = 0;
                DfSendMessage(wnd, KEYBOARD, HOME, 0);
            }
            ButtonDown = TRUE;
            ButtonX = MouseX;
            ButtonY = MouseY;
        }
        else
            MouseX = MouseY = 0;
        wnd->WndRow = MouseY;
        SetLinePointer(wnd, MouseY+wnd->wtop);
    }
    if (isMultiLine(wnd) ||
        (!TextBlockMarked(wnd)
            && (int)(MouseX+wnd->wleft) < (int)strlen(wnd->text)))
        wnd->CurrCol = MouseX+wnd->wleft;
    DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return TRUE;
}
/* ----------- MOUSE_MOVED Message ---------- */
static int MouseMovedMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int MouseX = (int) p1 - GetClientLeft(wnd);
    int MouseY = (int) p2 - GetClientTop(wnd);
    DFRECT rc = ClientRect(wnd);
    if (!InsideRect(p1, p2, rc))
        return FALSE;
    if (MouseY > wnd->wlines-1)
        return FALSE;
    if (ButtonDown)    {
        SetAnchor(wnd, ButtonX+wnd->wleft, ButtonY+wnd->wtop);
        TextMarking = TRUE;
		rc = WindowRect(wnd);
        DfSendMessage(NULL,MOUSE_TRAVEL,(PARAM) &rc, 0);
        ButtonDown = FALSE;
    }
    if (TextMarking && !(WindowMoving || WindowSizing))    {
        ExtendBlock(wnd, MouseX, MouseY);
        return TRUE;
    }
    return FALSE;
}
static void StopMarking(DFWINDOW wnd)
{
    TextMarking = FALSE;
    if (wnd->BlkBegLine > wnd->BlkEndLine)    {
        swap(wnd->BlkBegLine, wnd->BlkEndLine);
        swap(wnd->BlkBegCol, wnd->BlkEndCol);
    }
    if (wnd->BlkBegLine == wnd->BlkEndLine &&
            wnd->BlkBegCol > wnd->BlkEndCol)
        swap(wnd->BlkBegCol, wnd->BlkEndCol);
}
/* ----------- BUTTON_RELEASED Message ---------- */
static int ButtonReleasedMsg(DFWINDOW wnd)
{
    if (isMultiLine(wnd))    {
        ButtonDown = FALSE;
        if (TextMarking && !(WindowMoving || WindowSizing))  {
            /* release the mouse ouside the edit box */
            DfSendMessage(NULL, MOUSE_TRAVEL, 0, 0);
            StopMarking(wnd);
            return TRUE;
        }
        else
            PrevY = -1;
    }
    return FALSE;
}
/* ---- Process text block keys for multiline text box ---- */
static void DoMultiLines(DFWINDOW wnd, int c, PARAM p2)
{
    if (isMultiLine(wnd) && !KeyBoardMarking)    {
        if ((int)p2 & (LEFTSHIFT | RIGHTSHIFT))    {
            switch (c)    {
                case HOME:
                case CTRL_HOME:
                case CTRL_BS:
                case PGUP:
                case CTRL_PGUP:
                case UP:
                case BS:
                case END:
                case CTRL_END:
                case PGDN:
                case CTRL_PGDN:
                case DN:
                case FWD:
                case CTRL_FWD:
                    KeyBoardMarking = TextMarking = TRUE;
                    SetAnchor(wnd, wnd->CurrCol, wnd->CurrLine);
                    break;
                default:
                    break;
            }
        }
    }
}
/* ---------- page/scroll keys ----------- */
static int DoScrolling(DFWINDOW wnd, int c, PARAM p2)
{
    switch (c)    {
        case PGUP:
        case PGDN:
            if (isMultiLine(wnd))
                BaseWndProc(EDITBOX, wnd, KEYBOARD, c, p2);
            break;
        case CTRL_PGUP:
        case CTRL_PGDN:
            BaseWndProc(EDITBOX, wnd, KEYBOARD, c, p2);
            break;
        case HOME:
            Home(wnd);
            break;
        case END:
            End(wnd);
            break;
        case CTRL_FWD:
            NextWord(wnd);
            break;
        case CTRL_BS:
            PrevWord(wnd);
            break;
        case CTRL_HOME:
            if (isMultiLine(wnd))    {
                DfSendMessage(wnd, SCROLLDOC, TRUE, 0);
                wnd->CurrLine = 0;
                wnd->WndRow = 0;
            }
            Home(wnd);
            break;
        case CTRL_END:
			if (isMultiLine(wnd) &&
					wnd->WndRow+wnd->wtop+1 < wnd->wlines
						&& wnd->wlines > 0) {
                DfSendMessage(wnd, SCROLLDOC, FALSE, 0);
                SetLinePointer(wnd, wnd->wlines-1);
                wnd->WndRow =
                    min(ClientHeight(wnd)-1, wnd->wlines-1);
                Home(wnd);
            }
            End(wnd);
            break;
        case UP:
            if (isMultiLine(wnd))
                Upward(wnd);
            break;
        case DN:
            if (isMultiLine(wnd))
                Downward(wnd);
            break;
        case FWD:
            Forward(wnd);
            break;
        case BS:
            Backward(wnd);
            break;
        default:
            return FALSE;
    }
    if (!KeyBoardMarking && TextBlockMarked(wnd))    {
        ClearTextBlock(wnd);
        DfSendMessage(wnd, PAINT, 0, 0);
    }
    DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return TRUE;
}
/* -------------- Del key ---------------- */
static void DelKey(DFWINDOW wnd)
{
    char *currchar = CurrChar;
    int repaint = *currchar == '\n';
    if (TextBlockMarked(wnd))    {
        DfSendMessage(wnd, DFM_COMMAND, ID_DELETETEXT, 0);
        DfSendMessage(wnd, PAINT, 0, 0);
        return;
    }
    if (isMultiLine(wnd) && *currchar == '\n' && *(currchar+1) == '\0')
        return;
    strcpy(currchar, currchar+1);
    if (repaint)    {
        BuildTextPointers(wnd);
        DfSendMessage(wnd, PAINT, 0, 0);
    }
    else    {
        ModTextPointers(wnd, wnd->CurrLine+1, -1);
        WriteTextLine(wnd, NULL, wnd->WndRow+wnd->wtop, FALSE);
    }
    wnd->TextChanged = TRUE;
}
/* ------------ Tab key ------------ */
static void TabKey(DFWINDOW wnd, PARAM p2)
{
    if (isMultiLine(wnd))    {
        int insmd = wnd->InsertMode;
        do  {
            char *cc = CurrChar+1;
            if (!insmd && *cc == '\0')
                break;
            if (wnd->textlen == wnd->MaxTextLength)
                break;
            DfSendMessage(wnd,KEYBOARD,insmd ? ' ' : FWD,0);
        } while (wnd->CurrCol % cfg.Tabs);
    }
	else
	    DfPostMessage(GetParent(wnd), KEYBOARD, '\t', p2);
}
/* ------------ Shift+Tab key ------------ */
static void ShiftTabKey(DFWINDOW wnd, PARAM p2)
{
    if (isMultiLine(wnd))    {
        do  {
            if (CurrChar == GetText(wnd))
                break;
            DfSendMessage(wnd,KEYBOARD,BS,0);
        } while (wnd->CurrCol % cfg.Tabs);
    }
	else
	    DfPostMessage(GetParent(wnd), KEYBOARD, SHIFT_HT, p2);
}
/* --------- All displayable typed keys ------------- */
static void KeyTyped(DFWINDOW wnd, int c)
{
    char *currchar = CurrChar;
    if ((c != '\n' && c < ' ') || (c & 0x1000))
        /* ---- not recognized by editor --- */
        return;
    if (!isMultiLine(wnd) && TextBlockMarked(wnd))    {
		DfSendMessage(wnd, CLEARTEXT, 0, 0);
        currchar = CurrChar;
    }
    /* ---- test typing at end of text ---- */
    if (currchar == wnd->text+wnd->MaxTextLength)    {
        /* ---- typing at the end of maximum buffer ---- */
        beep();
        return;
    }
    if (*currchar == '\0')    {
        /* --- insert a newline at end of text --- */
        *currchar = '\n';
        *(currchar+1) = '\0';
        BuildTextPointers(wnd);
    }
    /* --- displayable char or newline --- */
    if (c == '\n' || wnd->InsertMode || *currchar == '\n') {
        /* ------ inserting the keyed character ------ */
        if (wnd->text[wnd->textlen-1] != '\0')    {
            /* --- the current text buffer is full --- */
            if (wnd->textlen == wnd->MaxTextLength)    {
                /* --- text buffer is at maximum size --- */
                beep();
                return;
            }
            /* ---- increase the text buffer size ---- */
            wnd->textlen += GROWLENGTH;
            /* --- but not above maximum size --- */
            if (wnd->textlen > wnd->MaxTextLength)
                wnd->textlen = wnd->MaxTextLength;
            wnd->text = DFrealloc(wnd->text, wnd->textlen+2);
            wnd->text[wnd->textlen-1] = '\0';
            currchar = CurrChar;
        }
        memmove(currchar+1, currchar, strlen(currchar)+1);
        ModTextPointers(wnd, wnd->CurrLine+1, 1);
        if (isMultiLine(wnd) && wnd->wlines > 1)
            wnd->textwidth = max(wnd->textwidth,
                (int) (TextLine(wnd, wnd->CurrLine+1)-
                TextLine(wnd, wnd->CurrLine)));
        else
            wnd->textwidth = max((int)wnd->textwidth,
                (int)strlen(wnd->text));
        WriteTextLine(wnd, NULL,
            wnd->wtop+wnd->WndRow, FALSE);
    }
    /* ----- put the char in the buffer ----- */
    *currchar = c;
    wnd->TextChanged = TRUE;
    if (c == '\n')    {
        wnd->wleft = 0;
        BuildTextPointers(wnd);
        End(wnd);
        Forward(wnd);
        DfSendMessage(wnd, PAINT, 0, 0);
        return;
    }
    /* ---------- test end of window --------- */
    if (WndCol == ClientWidth(wnd)-1)    {
        if (!isMultiLine(wnd))	{
			if (!(currchar == wnd->text+wnd->MaxTextLength-2))
            DfSendMessage(wnd, HORIZSCROLL, TRUE, 0);
		}
		else	{
			char *cp = currchar;
	        while (*cp != ' ' && cp != TextLine(wnd, wnd->CurrLine))
	            --cp;
	        if (cp == TextLine(wnd, wnd->CurrLine) ||
	                !wnd->WordWrapMode)
	            DfSendMessage(wnd, HORIZSCROLL, TRUE, 0);
	        else    {
	            int dif = 0;
	            if (c != ' ')    {
	                dif = (int) (currchar - cp);
	                wnd->CurrCol -= dif;
	                DfSendMessage(wnd, KEYBOARD, DEL, 0);
	                --dif;
	            }
	            DfSendMessage(wnd, KEYBOARD, '\n', 0);
	            currchar = CurrChar;
	            wnd->CurrCol = dif;
	            if (c == ' ')
	                return;
	        }
	    }
	}
    /* ------ display the character ------ */
    SetStandardColor(wnd);
    PutWindowChar(wnd, c, WndCol, wnd->WndRow);
    /* ----- advance the pointers ------ */
    wnd->CurrCol++;
}
/* ------------ screen changing key strokes ------------- */
static void DoKeyStroke(DFWINDOW wnd, int c, PARAM p2)
{
    switch (c)    {
        case RUBOUT:
			if (wnd->CurrCol == 0 && wnd->CurrLine == 0)
				break;
            Backward(wnd);
        case DEL:
            DelKey(wnd);
            break;
        case SHIFT_HT:
            ShiftTabKey(wnd, p2);
            break;
        case '\t':
            TabKey(wnd, p2);
            break;
        case '\r':
            if (!isMultiLine(wnd))    {
                DfPostMessage(GetParent(wnd), KEYBOARD, c, p2);
                break;
            }
            c = '\n';
        default:
            if (TextBlockMarked(wnd))    {
                DfSendMessage(wnd, DFM_COMMAND, ID_DELETETEXT, 0);
                DfSendMessage(wnd, PAINT, 0, 0);
            }
            KeyTyped(wnd, c);
            break;
    }
}
/* ----------- KEYBOARD Message ---------- */
static int KeyboardMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
	int c = (int) p1;

	if (WindowMoving || WindowSizing || ((int)p2 & ALTKEY))
		return FALSE;

	switch (c)
	{
		/* these keys get processed by lower classes */
		case ESC:
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case INS:
		case SHIFT_INS:
		case SHIFT_DEL:
			return FALSE;

		/* these keys get processed here */
		case CTRL_FWD:
		case CTRL_BS:
		case CTRL_HOME:
		case CTRL_END:
		case CTRL_PGUP:
		case CTRL_PGDN:
			break;

		default:
			/* other ctrl keys get processed by lower classes */
			if ((int)p2 & CTRLKEY)
				return FALSE;
			/* all other keys get processed here */
			break;
	}

	DoMultiLines(wnd, c, p2);
	if (DoScrolling(wnd, c, p2))
	{
		if (KeyBoardMarking)
			ExtendBlock(wnd, WndCol, wnd->WndRow);
	}
	else if (!TestAttribute(wnd, READONLY))
	{
		DoKeyStroke(wnd, c, p2);
		DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
	}
	else
		beep();

	return TRUE;
}

/* ----------- SHIFT_CHANGED Message ---------- */
static void ShiftChangedMsg(DFWINDOW wnd, PARAM p1)
{
    if (!((int)p1 & (LEFTSHIFT | RIGHTSHIFT)) &&
                                   KeyBoardMarking)    {
        StopMarking(wnd);
        KeyBoardMarking = FALSE;
    }
}
/* ----------- ID_DELETETEXT Command ---------- */
static void DeleteTextCmd(DFWINDOW wnd)
{
    if (TextBlockMarked(wnd))    {
        char *bbl=TextLine(wnd,wnd->BlkBegLine)+wnd->BlkBegCol;
        char *bel=TextLine(wnd,wnd->BlkEndLine)+wnd->BlkEndCol;
        int len = (int) (bel - bbl);
        SaveDeletedText(wnd, bbl, len);
        wnd->TextChanged = TRUE;
        strcpy(bbl, bel);
        wnd->CurrLine = TextLineNumber(wnd, bbl-wnd->BlkBegCol);
        wnd->CurrCol = wnd->BlkBegCol;
        wnd->WndRow = wnd->BlkBegLine - wnd->wtop;
        if (wnd->WndRow < 0)    {
            wnd->wtop = wnd->BlkBegLine;
            wnd->WndRow = 0;
        }
        DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
        ClearTextBlock(wnd);
        BuildTextPointers(wnd);
    }
}
/* ----------- ID_CLEAR Command ---------- */
static void ClearCmd(DFWINDOW wnd)
{
    if (TextBlockMarked(wnd))    {
        char *bbl=TextLine(wnd,wnd->BlkBegLine)+wnd->BlkBegCol;
        char *bel=TextLine(wnd,wnd->BlkEndLine)+wnd->BlkEndCol;
        int len = (int) (bel - bbl);
        SaveDeletedText(wnd, bbl, len);
        wnd->CurrLine = TextLineNumber(wnd, bbl);
        wnd->CurrCol = wnd->BlkBegCol;
        wnd->WndRow = wnd->BlkBegLine - wnd->wtop;
        if (wnd->WndRow < 0)    {
            wnd->WndRow = 0;
            wnd->wtop = wnd->BlkBegLine;
        }
        /* ------ change all text lines in block to \n ----- */
        while (bbl < bel)    {
            char *cp = strchr(bbl, '\n');
            if (cp > bel)
                cp = bel;
            strcpy(bbl, cp);
            bel -= (int) (cp - bbl);
            bbl++;
        }
        ClearTextBlock(wnd);
        BuildTextPointers(wnd);
        DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
        wnd->TextChanged = TRUE;
    }
}
/* ----------- ID_UNDO Command ---------- */
static void UndoCmd(DFWINDOW wnd)
{
    if (wnd->DeletedText != NULL)    {
        PasteText(wnd, wnd->DeletedText, wnd->DeletedLength);
        free(wnd->DeletedText);
        wnd->DeletedText = NULL;
        wnd->DeletedLength = 0;
        DfSendMessage(wnd, PAINT, 0, 0);
    }
}
/* ----------- ID_PARAGRAPH Command ---------- */
static void ParagraphCmd(DFWINDOW wnd)
{
    int bc, fl;
    char *bl, *bbl, *bel, *bb;

    ClearTextBlock(wnd);
    /* ---- forming paragraph from cursor position --- */
    fl = wnd->wtop + wnd->WndRow;
    bbl = bel = bl = TextLine(wnd, wnd->CurrLine);
    if ((bc = wnd->CurrCol) >= ClientWidth(wnd))
        bc = 0;
    Home(wnd);
    /* ---- locate the end of the paragraph ---- */
    while (*bel)    {
        int blank = TRUE;
        char *bll = bel;
        /* --- blank line marks end of paragraph --- */
        while (*bel && *bel != '\n')    {
            if (*bel != ' ')
                blank = FALSE;
            bel++;
        }
        if (blank)    {
            bel = bll;
            break;
        }
        if (*bel)
            bel++;
    }
    if (bel == bbl)    {
        DfSendMessage(wnd, KEYBOARD, DN, 0);
        return;
    }
    if (*bel == '\0')
        --bel;
    if (*bel == '\n')
        --bel;
    /* --- change all newlines in block to spaces --- */
    while (CurrChar < bel)    {
        if (*CurrChar == '\n')    {
            *CurrChar = ' ';
            wnd->CurrLine++;
            wnd->CurrCol = 0;
        }
        else
            wnd->CurrCol++;
    }
    /* ---- insert newlines at new margin boundaries ---- */
    bb = bbl;
    while (bbl < bel)    {
        bbl++;
        if ((int)(bbl - bb) == ClientWidth(wnd)-1)    {
            while (*bbl != ' ' && bbl > bb)
                --bbl;
            if (*bbl != ' ')    {
                bbl = strchr(bbl, ' ');
                if (bbl == NULL || bbl >= bel)
                    break;
            }
            *bbl = '\n';
            bb = bbl+1;
        }
    }
    BuildTextPointers(wnd);
    /* --- put cursor back at beginning --- */
    wnd->CurrLine = TextLineNumber(wnd, bl);
    wnd->CurrCol = bc;
    if (fl < wnd->wtop)
        wnd->wtop = fl;
    wnd->WndRow = fl - wnd->wtop;
    DfSendMessage(wnd, PAINT, 0, 0);
    DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    wnd->TextChanged = TRUE;
    BuildTextPointers(wnd);
}
/* ----------- COMMAND Message ---------- */
static int CommandMsg(DFWINDOW wnd, PARAM p1)
{
    switch ((int)p1)    {
        case ID_DELETETEXT:
            DeleteTextCmd(wnd);
            return TRUE;
        case ID_CLEAR:
            ClearCmd(wnd);
            return TRUE;
        case ID_UNDO:
            UndoCmd(wnd);
            return TRUE;
        case ID_PARAGRAPH:
            ParagraphCmd(wnd);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}
/* ---------- CLOSE_WINDOW Message ----------- */
static int CloseWindowMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
	int rtn;
    DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
    if (wnd->DeletedText != NULL)
        free(wnd->DeletedText);
	if (wnd->text != NULL)
	{
		free(wnd->text);
		wnd->text = NULL;
	}
    rtn = BaseWndProc(EDITBOX, wnd, CLOSE_WINDOW, p1, p2);
    return rtn;
}

/* ------- Window processing module for EDITBOX class ------ */
int EditBoxProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    switch (msg)    {
        case CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case ADDTEXT:
            return AddTextMsg(wnd, p1, p2);
        case SETTEXT:
            return SetTextMsg(wnd, p1);
        case CLEARTEXT:
			return ClearTextMsg(wnd);
        case GETTEXT:
            return GetTextMsg(wnd, p1, p2);
        case SETTEXTLENGTH:
            return SetTextLengthMsg(wnd, (unsigned) p1);
        case KEYBOARD_CURSOR:
            KeyboardCursorMsg(wnd, p1, p2);
			return TRUE;
        case SETFOCUS:
			if (!(int)p1)
				DfSendMessage(NULL, HIDE_CURSOR, 0, 0);
        case PAINT:
        case MOVE:
            rtn = BaseWndProc(EDITBOX, wnd, msg, p1, p2);
            DfSendMessage(wnd,KEYBOARD_CURSOR,WndCol,wnd->WndRow);
            return rtn;
        case DFM_SIZE:
            return SizeMsg(wnd, p1, p2);
        case SCROLL:
            return ScrollMsg(wnd, p1);
        case HORIZSCROLL:
            return HorizScrollMsg(wnd, p1);
        case SCROLLPAGE:
            return ScrollPageMsg(wnd, p1);
        case HORIZPAGE:
            return HorizPageMsg(wnd, p1);
        case LEFT_BUTTON:
            if (LeftButtonMsg(wnd, p1, p2))
                return TRUE;
            break;
        case MOUSE_MOVED:
            if (MouseMovedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_BUTTON_RELEASED:
            if (ButtonReleasedMsg(wnd))
                return TRUE;
            break;
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case SHIFT_CHANGED:
            ShiftChangedMsg(wnd, p1);
            break;
        case DFM_COMMAND:
            if (CommandMsg(wnd, p1))
                return TRUE;
            break;
        case CLOSE_WINDOW:
            return CloseWindowMsg(wnd, p1, p2);
        default:
            break;
    }
    return BaseWndProc(EDITBOX, wnd, msg, p1, p2);
}
/* ------ save deleted text for the Undo command ------ */
static void SaveDeletedText(DFWINDOW wnd, char *bbl, int len)
{
    wnd->DeletedLength = len;
    wnd->DeletedText=DFrealloc(wnd->DeletedText,len);
    memmove(wnd->DeletedText, bbl, len);
}
/* ---- cursor right key: right one character position ---- */
static void Forward(DFWINDOW wnd)
{
    char *cc = CurrChar+1;
    if (*cc == '\0')
        return;
    if (*CurrChar == '\n')    {
        Home(wnd);
        Downward(wnd);
    }
    else    {
        wnd->CurrCol++;
        if (WndCol == ClientWidth(wnd))
            DfSendMessage(wnd, HORIZSCROLL, TRUE, 0);
    }
}
/* ----- stick the moving cursor to the end of the line ---- */
static void StickEnd(DFWINDOW wnd)
{
    char *cp = TextLine(wnd, wnd->CurrLine);
    char *cp1 = strchr(cp, '\n');
    int len = cp1 ? (int) (cp1 - cp) : 0;
    wnd->CurrCol = min(len, wnd->CurrCol);
    if (wnd->wleft > wnd->CurrCol)    {
        wnd->wleft = max(0, wnd->CurrCol - 4);
        DfSendMessage(wnd, PAINT, 0, 0);
    }
    else if (wnd->CurrCol-wnd->wleft >= ClientWidth(wnd))    {
        wnd->wleft = wnd->CurrCol - (ClientWidth(wnd)-1);
        DfSendMessage(wnd, PAINT, 0, 0);
    }
}
/* --------- cursor down key: down one line --------- */
static void Downward(DFWINDOW wnd)
{
    if (isMultiLine(wnd) &&
            wnd->WndRow+wnd->wtop+1 < wnd->wlines)  {
        wnd->CurrLine++;
        if (wnd->WndRow == ClientHeight(wnd)-1)
            BaseWndProc(EDITBOX, wnd, SCROLL, TRUE, 0);
        else
            wnd->WndRow++;
        StickEnd(wnd);
    }
}
/* -------- cursor up key: up one line ------------ */
static void Upward(DFWINDOW wnd)
{
    if (isMultiLine(wnd) && wnd->CurrLine != 0)    {
        --wnd->CurrLine;
        if (wnd->WndRow == 0)
            BaseWndProc(EDITBOX, wnd, SCROLL, FALSE, 0);
        else
            --wnd->WndRow;
        StickEnd(wnd);
    }
}
/* ---- cursor left key: left one character position ---- */
static void Backward(DFWINDOW wnd)
{
    if (wnd->CurrCol)    {
        --wnd->CurrCol;
        if (wnd->CurrCol < wnd->wleft)
            DfSendMessage(wnd, HORIZSCROLL, FALSE, 0);
    }
    else if (isMultiLine(wnd) && wnd->CurrLine != 0)    {
        Upward(wnd);
        End(wnd);
    }
}
/* -------- End key: to end of line ------- */
static void End(DFWINDOW wnd)
{
    while (*CurrChar && *CurrChar != '\n')
        ++wnd->CurrCol;
    if (WndCol >= ClientWidth(wnd))    {
        wnd->wleft = wnd->CurrCol - (ClientWidth(wnd)-1);
        DfSendMessage(wnd, PAINT, 0, 0);
    }
}
/* -------- Home key: to beginning of line ------- */
static void Home(DFWINDOW wnd)
{
    wnd->CurrCol = 0;
    if (wnd->wleft != 0)    {
        wnd->wleft = 0;
        DfSendMessage(wnd, PAINT, 0, 0);
    }
}
/* -- Ctrl+cursor right key: to beginning of next word -- */
static void NextWord(DFWINDOW wnd)
{
    int savetop = wnd->wtop;
    int saveleft = wnd->wleft;
    ClearVisible(wnd);
    while (!isWhite(*CurrChar))    {
        char *cc = CurrChar+1;
        if (*cc == '\0')
            break;
        Forward(wnd);
    }
    while (isWhite(*CurrChar))    {
        char *cc = CurrChar+1;
        if (*cc == '\0')
            break;
        Forward(wnd);
    }
    SetVisible(wnd);
    DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    if (wnd->wtop != savetop || wnd->wleft != saveleft)
        DfSendMessage(wnd, PAINT, 0, 0);
}
/* -- Ctrl+cursor left key: to beginning of previous word -- */
static void PrevWord(DFWINDOW wnd)
{
    int savetop = wnd->wtop;
    int saveleft = wnd->wleft;
    ClearVisible(wnd);
    Backward(wnd);
    while (isWhite(*CurrChar))    {
        if (wnd->CurrLine == 0 && wnd->CurrCol == 0)
            break;
        Backward(wnd);
    }
    while (wnd->CurrCol != 0 && !isWhite(*CurrChar))
        Backward(wnd);
    if (isWhite(*CurrChar))
        Forward(wnd);
    SetVisible(wnd);
    if (wnd->wleft != saveleft)
        if (wnd->CurrCol >= saveleft)
            if (wnd->CurrCol - saveleft < ClientWidth(wnd))
                wnd->wleft = saveleft;
    DfSendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    if (wnd->wtop != savetop || wnd->wleft != saveleft)
        DfSendMessage(wnd, PAINT, 0, 0);
}
/* ----- modify text pointers from a specified position
                by a specified plus or minus amount ----- */
static void ModTextPointers(DFWINDOW wnd, int lineno, int var)
{
    while (lineno < wnd->wlines)
        *((wnd->TextPointers) + lineno++) += var;
}
/* ----- set anchor point for marking text block ----- */
static void SetAnchor(DFWINDOW wnd, int mx, int my)
{
    ClearTextBlock(wnd);
    /* ------ set the anchor ------ */
    wnd->BlkBegLine = wnd->BlkEndLine = my;
    wnd->BlkBegCol = wnd->BlkEndCol = mx;
    DfSendMessage(wnd, PAINT, 0, 0);
}

/* EOF */
