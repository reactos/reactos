/* ------------- editbox.c ------------ */
#include "dflat.h"

#define EditBufLen(wnd) (DfIsMultiLine(wnd) ? DF_EDITLEN : DF_ENTRYLEN)
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

/* ----------- DFM_CREATE_WINDOW Message ---------- */
static int CreateWindowMsg(DFWINDOW wnd)
{
    int rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_CREATE_WINDOW, 0, 0);
    wnd->MaxTextLength = DF_MAXTEXTLEN+1;
    wnd->textlen = EditBufLen(wnd);
    wnd->InsertMode = TRUE;
	DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
    return rtn;
}
/* ----------- DFM_SETTEXT Message ---------- */
static int SetTextMsg(DFWINDOW wnd, DF_PARAM p1)
{
    int rtn = FALSE;
    if (strlen((char *)p1) <= wnd->MaxTextLength)
        rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_SETTEXT, p1, 0);
    return rtn;
}
/* ----------- DFM_CLEARTEXT Message ------------ */
static int ClearTextMsg(DFWINDOW wnd)
{
    int rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_CLEARTEXT, 0, 0);
    unsigned blen = EditBufLen(wnd)+2;
    wnd->text = DfRealloc(wnd->text, blen);
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
/* ----------- DFM_ADDTEXT Message ---------- */
static int AddTextMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int rtn = FALSE;
    if (strlen((char *)p1)+wnd->textlen <= wnd->MaxTextLength) {
        rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_ADDTEXT, p1, p2);
        if (rtn != FALSE)    {
            if (!DfIsMultiLine(wnd))    {
                wnd->CurrLine = 0;
                wnd->CurrCol = strlen((char *)p1);
                if (wnd->CurrCol >= DfClientWidth(wnd))    {
                    wnd->wleft = wnd->CurrCol-DfClientWidth(wnd);
                    wnd->CurrCol -= wnd->wleft;
                }
                wnd->BlkEndCol = wnd->CurrCol;
                DfSendMessage(wnd, DFM_KEYBOARD_CURSOR,
                                     DfWndCol, wnd->WndRow);
            }
        }
    }
    return rtn;
}
/* ----------- DFM_GETTEXT Message ---------- */
static int GetTextMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
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
/* ----------- DFM_SETTEXTLENGTH Message ---------- */
static int SetTextLengthMsg(DFWINDOW wnd, unsigned int len)
{
    if (++len < DF_MAXTEXTLEN)    {
        wnd->MaxTextLength = len;
        if (len < wnd->textlen)    {
            wnd->text=DfRealloc(wnd->text, len+2);
            wnd->textlen = len;
            *((wnd->text)+len) = '\0';
            *((wnd->text)+len+1) = '\0';
            DfBuildTextPointers(wnd);
        }
        return TRUE;
    }
    return FALSE;
}
/* ----------- DFM_KEYBOARD_CURSOR Message ---------- */
static void KeyboardCursorMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    wnd->CurrCol = (int)p1 + wnd->wleft;
    wnd->WndRow = (int)p2;
    wnd->CurrLine = (int)p2 + wnd->wtop;
    if (wnd == DfInFocus)	{
		if (DfCharInView(wnd, (int)p1, (int)p2))
	        DfSendMessage(NULL, DFM_SHOW_CURSOR,
				(wnd->InsertMode && !TextMarking), 0);
    	else
			DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
	}
}
/* ----------- SIZE Message ---------- */
int SizeMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_DFM_SIZE, p1, p2);
    if (DfWndCol > DfClientWidth(wnd)-1)
        wnd->CurrCol = DfClientWidth(wnd)-1 + wnd->wleft;
    if (wnd->WndRow > DfClientHeight(wnd)-1)    {
        wnd->WndRow = DfClientHeight(wnd)-1;
        SetLinePointer(wnd, wnd->WndRow+wnd->wtop);
    }
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
    return rtn;
}
/* ----------- DFM_SCROLL Message ---------- */
static int ScrollMsg(DFWINDOW wnd, DF_PARAM p1)
{
    int rtn = FALSE;
    if (DfIsMultiLine(wnd))    {
        rtn = DfBaseWndProc(DF_EDITBOX,wnd,DFM_SCROLL,p1,0);
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
                if (wnd->WndRow == DfClientHeight(wnd)-1)    {
                    if (wnd->CurrLine > 0)
                        --wnd->CurrLine;
                    StickEnd(wnd);
                }
                else
                    wnd->WndRow++;
            }
            DfSendMessage(wnd,DFM_KEYBOARD_CURSOR,DfWndCol,wnd->WndRow);
        }
    }
    return rtn;
}
/* ----------- DFM_HORIZSCROLL Message ---------- */
static int HorizScrollMsg(DFWINDOW wnd, DF_PARAM p1)
{
    int rtn = FALSE;
    char *currchar = DfCurrChar;
    if (!(p1 &&
            wnd->CurrCol == wnd->wleft && *currchar == '\n'))  {
        rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_HORIZSCROLL, p1, 0);
        if (rtn != FALSE)    {
            if (wnd->CurrCol < wnd->wleft)
                wnd->CurrCol++;
            else if (DfWndCol == DfClientWidth(wnd))
                --wnd->CurrCol;
            DfSendMessage(wnd,DFM_KEYBOARD_CURSOR,DfWndCol,wnd->WndRow);
        }
    }
    return rtn;
}
/* ----------- DFM_SCROLLPAGE Message ---------- */
static int ScrollPageMsg(DFWINDOW wnd, DF_PARAM p1)
{
    int rtn = FALSE;
    if (DfIsMultiLine(wnd))    {
        rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_SCROLLPAGE, p1, 0);
        SetLinePointer(wnd, wnd->wtop+wnd->WndRow);
        StickEnd(wnd);
        DfSendMessage(wnd, DFM_KEYBOARD_CURSOR,DfWndCol, wnd->WndRow);
    }
    return rtn;
}
/* ----------- HORIZSCROLLPAGE Message ---------- */
static int HorizPageMsg(DFWINDOW wnd, DF_PARAM p1)
{
    int rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_HORIZPAGE, p1, 0);
    if ((int) p1 == FALSE)    {
        if (wnd->CurrCol > wnd->wleft+DfClientWidth(wnd)-1)
            wnd->CurrCol = wnd->wleft+DfClientWidth(wnd)-1;
    }
    else if (wnd->CurrCol < wnd->wleft)
        wnd->CurrCol = wnd->wleft;
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
    return rtn;
}
/* ----- Extend the marked block to the new x,y position ---- */
static void ExtendBlock(DFWINDOW wnd, int x, int y)
{
    int bbl, bel;
    int ptop = min(wnd->BlkBegLine, wnd->BlkEndLine);
    int pbot = max(wnd->BlkBegLine, wnd->BlkEndLine);
    char *lp = DfTextLine(wnd, wnd->wtop+y);
    int len = (int) (strchr(lp, '\n') - lp);
    x = max(0, min(x, len));
    y = max(0, y);
    wnd->BlkEndCol = min(len, x+wnd->wleft);
    wnd->BlkEndLine = y+wnd->wtop;
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, wnd->BlkEndCol, wnd->BlkEndLine);
    bbl = min(wnd->BlkBegLine, wnd->BlkEndLine);
    bel = max(wnd->BlkBegLine, wnd->BlkEndLine);
    while (ptop < bbl)    {
        DfWriteTextLine(wnd, NULL, ptop, FALSE);
        ptop++;
    }
    for (y = bbl; y <= bel; y++)
        DfWriteTextLine(wnd, NULL, y, FALSE);
    while (pbot > bel)    {
        DfWriteTextLine(wnd, NULL, pbot, FALSE);
        --pbot;
    }
}
/* ----------- DFM_LEFT_BUTTON Message ---------- */
static int LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int MouseX = (int) p1 - DfGetClientLeft(wnd);
    int MouseY = (int) p2 - DfGetClientTop(wnd);
    DFRECT rc = DfClientRect(wnd);
    char *lp;
    int len;
    if (KeyBoardMarking)
        return TRUE;
    if (DfWindowMoving || DfWindowSizing)
        return FALSE;
    if (DfIsMultiLine(wnd))    {
        if (TextMarking)    {
            if (!DfInsideRect(p1, p2, rc))    {
				int x = MouseX, y = MouseY;
				int dir;
				DFMESSAGE msg = 0;
                if ((int)p2 == DfGetTop(wnd))
					y++, dir = FALSE, msg = DFM_SCROLL;
                else if ((int)p2 == DfGetBottom(wnd))
					--y, dir = TRUE, msg = DFM_SCROLL;
                else if ((int)p1 == DfGetLeft(wnd))
					--x, dir = FALSE, msg = DFM_HORIZSCROLL;
                else if ((int)p1 == DfGetRight(wnd))
					x++, dir = TRUE, msg = DFM_HORIZSCROLL;
				if (msg != 0)	{
                    if (DfSendMessage(wnd, msg, dir, 0))
                        ExtendBlock(wnd, x, y);
	                DfSendMessage(wnd, DFM_PAINT, 0, 0);
				}
            }
            return TRUE;
        }
        if (!DfInsideRect(p1, p2, rc))
            return FALSE;
        if (DfTextBlockMarked(wnd))    {
            DfClearTextBlock(wnd);
            DfSendMessage(wnd, DFM_PAINT, 0, 0);
        }
        if (wnd->wlines)    {
            if (MouseY > wnd->wlines-1)
                return TRUE;
            lp = DfTextLine(wnd, MouseY+wnd->wtop);
            len = (int) (strchr(lp, '\n') - lp);
            MouseX = min(MouseX, len);
            if (MouseX < wnd->wleft)    {
                MouseX = 0;
                DfSendMessage(wnd, DFM_KEYBOARD, DF_HOME, 0);
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
    if (DfIsMultiLine(wnd) ||
        (!DfTextBlockMarked(wnd)
            && (int)(MouseX+wnd->wleft) < (int)strlen(wnd->text)))
        wnd->CurrCol = MouseX+wnd->wleft;
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
    return TRUE;
}
/* ----------- MOUSE_MOVED Message ---------- */
static int MouseMovedMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int MouseX = (int) p1 - DfGetClientLeft(wnd);
    int MouseY = (int) p2 - DfGetClientTop(wnd);
    DFRECT rc = DfClientRect(wnd);
    if (!DfInsideRect(p1, p2, rc))
        return FALSE;
    if (MouseY > wnd->wlines-1)
        return FALSE;
    if (ButtonDown)    {
        SetAnchor(wnd, ButtonX+wnd->wleft, ButtonY+wnd->wtop);
        TextMarking = TRUE;
		rc = DfWindowRect(wnd);
        DfSendMessage(NULL,DFM_MOUSE_TRAVEL,(DF_PARAM) &rc, 0);
        ButtonDown = FALSE;
    }
    if (TextMarking && !(DfWindowMoving || DfWindowSizing))    {
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
    if (DfIsMultiLine(wnd))    {
        ButtonDown = FALSE;
        if (TextMarking && !(DfWindowMoving || DfWindowSizing))  {
            /* release the mouse ouside the edit box */
            DfSendMessage(NULL, DFM_MOUSE_TRAVEL, 0, 0);
            StopMarking(wnd);
            return TRUE;
        }
        else
            PrevY = -1;
    }
    return FALSE;
}
/* ---- Process text block keys for multiline text box ---- */
static void DoMultiLines(DFWINDOW wnd, int c, DF_PARAM p2)
{
    if (DfIsMultiLine(wnd) && !KeyBoardMarking)    {
        if ((int)p2 & (DF_LEFTSHIFT | DF_RIGHTSHIFT))    {
            switch (c)    {
                case DF_HOME:
                case DF_CTRL_HOME:
                case DF_CTRL_BS:
                case DF_PGUP:
                case DF_CTRL_PGUP:
                case DF_UP:
                case DF_BS:
                case DF_END:
                case DF_CTRL_END:
                case DF_PGDN:
                case DF_CTRL_PGDN:
                case DF_DN:
                case DF_FWD:
                case DF_CTRL_FWD:
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
static int DoScrolling(DFWINDOW wnd, int c, DF_PARAM p2)
{
    switch (c)    {
        case DF_PGUP:
        case DF_PGDN:
            if (DfIsMultiLine(wnd))
                DfBaseWndProc(DF_EDITBOX, wnd, DFM_KEYBOARD, c, p2);
            break;
        case DF_CTRL_PGUP:
        case DF_CTRL_PGDN:
            DfBaseWndProc(DF_EDITBOX, wnd, DFM_KEYBOARD, c, p2);
            break;
        case DF_HOME:
            Home(wnd);
            break;
        case DF_END:
            End(wnd);
            break;
        case DF_CTRL_FWD:
            NextWord(wnd);
            break;
        case DF_CTRL_BS:
            PrevWord(wnd);
            break;
        case DF_CTRL_HOME:
            if (DfIsMultiLine(wnd))    {
                DfSendMessage(wnd, DFM_SCROLLDOC, TRUE, 0);
                wnd->CurrLine = 0;
                wnd->WndRow = 0;
            }
            Home(wnd);
            break;
        case DF_CTRL_END:
			if (DfIsMultiLine(wnd) &&
					wnd->WndRow+wnd->wtop+1 < wnd->wlines
						&& wnd->wlines > 0) {
                DfSendMessage(wnd, DFM_SCROLLDOC, FALSE, 0);
                SetLinePointer(wnd, wnd->wlines-1);
                wnd->WndRow =
                    min(DfClientHeight(wnd)-1, wnd->wlines-1);
                Home(wnd);
            }
            End(wnd);
            break;
        case DF_UP:
            if (DfIsMultiLine(wnd))
                Upward(wnd);
            break;
        case DF_DN:
            if (DfIsMultiLine(wnd))
                Downward(wnd);
            break;
        case DF_FWD:
            Forward(wnd);
            break;
        case DF_BS:
            Backward(wnd);
            break;
        default:
            return FALSE;
    }
    if (!KeyBoardMarking && DfTextBlockMarked(wnd))    {
        DfClearTextBlock(wnd);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
    }
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
    return TRUE;
}
/* -------------- Del key ---------------- */
static void DelKey(DFWINDOW wnd)
{
    char *currchar = DfCurrChar;
    int repaint = *currchar == '\n';
    if (DfTextBlockMarked(wnd))    {
        DfSendMessage(wnd, DFM_COMMAND, DF_ID_DELETETEXT, 0);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
        return;
    }
    if (DfIsMultiLine(wnd) && *currchar == '\n' && *(currchar+1) == '\0')
        return;
    strcpy(currchar, currchar+1);
    if (repaint)    {
        DfBuildTextPointers(wnd);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
    }
    else    {
        ModTextPointers(wnd, wnd->CurrLine+1, -1);
        DfWriteTextLine(wnd, NULL, wnd->WndRow+wnd->wtop, FALSE);
    }
    wnd->TextChanged = TRUE;
}
/* ------------ Tab key ------------ */
static void TabKey(DFWINDOW wnd, DF_PARAM p2)
{
    if (DfIsMultiLine(wnd))    {
        int insmd = wnd->InsertMode;
        do  {
            char *cc = DfCurrChar+1;
            if (!insmd && *cc == '\0')
                break;
            if (wnd->textlen == wnd->MaxTextLength)
                break;
            DfSendMessage(wnd,DFM_KEYBOARD,insmd ? ' ' : DF_FWD,0);
        } while (wnd->CurrCol % DfCfg.Tabs);
    }
	else
	    DfPostMessage(DfGetParent(wnd), DFM_KEYBOARD, '\t', p2);
}
/* ------------ Shift+Tab key ------------ */
static void ShiftTabKey(DFWINDOW wnd, DF_PARAM p2)
{
    if (DfIsMultiLine(wnd))    {
        do  {
            if (DfCurrChar == DfGetText(wnd))
                break;
            DfSendMessage(wnd,DFM_KEYBOARD,DF_BS,0);
        } while (wnd->CurrCol % DfCfg.Tabs);
    }
	else
	    DfPostMessage(DfGetParent(wnd), DFM_KEYBOARD, DF_SHIFT_HT, p2);
}
/* --------- All displayable typed keys ------------- */
static void KeyTyped(DFWINDOW wnd, int c)
{
    char *currchar = DfCurrChar;
    if ((c != '\n' && c < ' ') || (c & 0x1000))
        /* ---- not recognized by editor --- */
        return;
    if (!DfIsMultiLine(wnd) && DfTextBlockMarked(wnd))    {
		DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
        currchar = DfCurrChar;
    }
    /* ---- test typing at end of text ---- */
    if (currchar == wnd->text+wnd->MaxTextLength)    {
        /* ---- typing at the end of maximum buffer ---- */
        DfBeep();
        return;
    }
    if (*currchar == '\0')    {
        /* --- insert a newline at end of text --- */
        *currchar = '\n';
        *(currchar+1) = '\0';
        DfBuildTextPointers(wnd);
    }
    /* --- displayable char or newline --- */
    if (c == '\n' || wnd->InsertMode || *currchar == '\n') {
        /* ------ inserting the keyed character ------ */
        if (wnd->text[wnd->textlen-1] != '\0')    {
            /* --- the current text buffer is full --- */
            if (wnd->textlen == wnd->MaxTextLength)    {
                /* --- text buffer is at maximum size --- */
                DfBeep();
                return;
            }
            /* ---- increase the text buffer size ---- */
            wnd->textlen += DF_GROWLENGTH;
            /* --- but not above maximum size --- */
            if (wnd->textlen > wnd->MaxTextLength)
                wnd->textlen = wnd->MaxTextLength;
            wnd->text = DfRealloc(wnd->text, wnd->textlen+2);
            wnd->text[wnd->textlen-1] = '\0';
            currchar = DfCurrChar;
        }
        memmove(currchar+1, currchar, strlen(currchar)+1);
        ModTextPointers(wnd, wnd->CurrLine+1, 1);
        if (DfIsMultiLine(wnd) && wnd->wlines > 1)
            wnd->textwidth = max(wnd->textwidth,
                (int) (DfTextLine(wnd, wnd->CurrLine+1)-
                DfTextLine(wnd, wnd->CurrLine)));
        else
            wnd->textwidth = max((int)wnd->textwidth,
                (int)strlen(wnd->text));
        DfWriteTextLine(wnd, NULL,
            wnd->wtop+wnd->WndRow, FALSE);
    }
    /* ----- put the char in the buffer ----- */
    *currchar = c;
    wnd->TextChanged = TRUE;
    if (c == '\n')    {
        wnd->wleft = 0;
        DfBuildTextPointers(wnd);
        End(wnd);
        Forward(wnd);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
        return;
    }
    /* ---------- test end of window --------- */
    if (DfWndCol == DfClientWidth(wnd)-1)    {
        if (!DfIsMultiLine(wnd))	{
			if (!(currchar == wnd->text+wnd->MaxTextLength-2))
            DfSendMessage(wnd, DFM_HORIZSCROLL, TRUE, 0);
		}
		else	{
			char *cp = currchar;
	        while (*cp != ' ' && cp != DfTextLine(wnd, wnd->CurrLine))
	            --cp;
	        if (cp == DfTextLine(wnd, wnd->CurrLine) ||
	                !wnd->WordWrapMode)
	            DfSendMessage(wnd, DFM_HORIZSCROLL, TRUE, 0);
	        else    {
	            int dif = 0;
	            if (c != ' ')    {
	                dif = (int) (currchar - cp);
	                wnd->CurrCol -= dif;
	                DfSendMessage(wnd, DFM_KEYBOARD, DF_DEL, 0);
	                --dif;
	            }
	            DfSendMessage(wnd, DFM_KEYBOARD, '\n', 0);
	            currchar = DfCurrChar;
	            wnd->CurrCol = dif;
	            if (c == ' ')
	                return;
	        }
	    }
	}
    /* ------ display the character ------ */
    DfSetStandardColor(wnd);
    DfPutWindowChar(wnd, c, DfWndCol, wnd->WndRow);
    /* ----- advance the pointers ------ */
    wnd->CurrCol++;
}
/* ------------ screen changing key strokes ------------- */
static void DoKeyStroke(DFWINDOW wnd, int c, DF_PARAM p2)
{
    switch (c)    {
        case DF_RUBOUT:
			if (wnd->CurrCol == 0 && wnd->CurrLine == 0)
				break;
            Backward(wnd);
        case DF_DEL:
            DelKey(wnd);
            break;
        case DF_SHIFT_HT:
            ShiftTabKey(wnd, p2);
            break;
        case '\t':
            TabKey(wnd, p2);
            break;
        case '\r':
            if (!DfIsMultiLine(wnd))    {
                DfPostMessage(DfGetParent(wnd), DFM_KEYBOARD, c, p2);
                break;
            }
            c = '\n';
        default:
            if (DfTextBlockMarked(wnd))    {
                DfSendMessage(wnd, DFM_COMMAND, DF_ID_DELETETEXT, 0);
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
            }
            KeyTyped(wnd, c);
            break;
    }
}
/* ----------- DFM_KEYBOARD Message ---------- */
static int KeyboardMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
	int c = (int) p1;

	if (DfWindowMoving || DfWindowSizing || ((int)p2 & DF_ALTKEY))
		return FALSE;

	switch (c)
	{
		/* these keys get processed by lower classes */
		case DF_ESC:
		case DF_F1:
		case DF_F2:
		case DF_F3:
		case DF_F4:
		case DF_F5:
		case DF_F6:
		case DF_F7:
		case DF_F8:
		case DF_F9:
		case DF_F10:
		case DF_INS:
		case DF_SHIFT_INS:
		case DF_SHIFT_DEL:
			return FALSE;

		/* these keys get processed here */
		case DF_CTRL_FWD:
		case DF_CTRL_BS:
		case DF_CTRL_HOME:
		case DF_CTRL_END:
		case DF_CTRL_PGUP:
		case DF_CTRL_PGDN:
			break;

		default:
			/* other ctrl keys get processed by lower classes */
			if ((int)p2 & DF_CTRLKEY)
				return FALSE;
			/* all other keys get processed here */
			break;
	}

	DoMultiLines(wnd, c, p2);
	if (DoScrolling(wnd, c, p2))
	{
		if (KeyBoardMarking)
			ExtendBlock(wnd, DfWndCol, wnd->WndRow);
	}
	else if (!DfTestAttribute(wnd, DF_READONLY))
	{
		DoKeyStroke(wnd, c, p2);
		DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
	}
	else
		DfBeep();

	return TRUE;
}

/* ----------- DFM_SHIFT_CHANGED Message ---------- */
static void ShiftChangedMsg(DFWINDOW wnd, DF_PARAM p1)
{
    if (!((int)p1 & (DF_LEFTSHIFT | DF_RIGHTSHIFT)) &&
                                   KeyBoardMarking)    {
        StopMarking(wnd);
        KeyBoardMarking = FALSE;
    }
}
/* ----------- DF_ID_DELETETEXT Command ---------- */
static void DeleteTextCmd(DFWINDOW wnd)
{
    if (DfTextBlockMarked(wnd))    {
        char *bbl=DfTextLine(wnd,wnd->BlkBegLine)+wnd->BlkBegCol;
        char *bel=DfTextLine(wnd,wnd->BlkEndLine)+wnd->BlkEndCol;
        int len = (int) (bel - bbl);
        SaveDeletedText(wnd, bbl, len);
        wnd->TextChanged = TRUE;
        strcpy(bbl, bel);
        wnd->CurrLine = DfTextLineNumber(wnd, bbl-wnd->BlkBegCol);
        wnd->CurrCol = wnd->BlkBegCol;
        wnd->WndRow = wnd->BlkBegLine - wnd->wtop;
        if (wnd->WndRow < 0)    {
            wnd->wtop = wnd->BlkBegLine;
            wnd->WndRow = 0;
        }
        DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
        DfClearTextBlock(wnd);
        DfBuildTextPointers(wnd);
    }
}
/* ----------- DF_ID_CLEAR Command ---------- */
static void ClearCmd(DFWINDOW wnd)
{
    if (DfTextBlockMarked(wnd))    {
        char *bbl=DfTextLine(wnd,wnd->BlkBegLine)+wnd->BlkBegCol;
        char *bel=DfTextLine(wnd,wnd->BlkEndLine)+wnd->BlkEndCol;
        int len = (int) (bel - bbl);
        SaveDeletedText(wnd, bbl, len);
        wnd->CurrLine = DfTextLineNumber(wnd, bbl);
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
        DfClearTextBlock(wnd);
        DfBuildTextPointers(wnd);
        DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
        wnd->TextChanged = TRUE;
    }
}
/* ----------- DF_ID_UNDO Command ---------- */
static void UndoCmd(DFWINDOW wnd)
{
    if (wnd->DeletedText != NULL)    {
        DfPasteText(wnd, wnd->DeletedText, wnd->DeletedLength);
        free(wnd->DeletedText);
        wnd->DeletedText = NULL;
        wnd->DeletedLength = 0;
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
    }
}
/* ----------- DF_ID_PARAGRAPH Command ---------- */
static void ParagraphCmd(DFWINDOW wnd)
{
    int bc, fl;
    char *bl, *bbl, *bel, *bb;

    DfClearTextBlock(wnd);
    /* ---- forming paragraph from DfCursor position --- */
    fl = wnd->wtop + wnd->WndRow;
    bbl = bel = bl = DfTextLine(wnd, wnd->CurrLine);
    if ((bc = wnd->CurrCol) >= DfClientWidth(wnd))
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
        DfSendMessage(wnd, DFM_KEYBOARD, DF_DN, 0);
        return;
    }
    if (*bel == '\0')
        --bel;
    if (*bel == '\n')
        --bel;
    /* --- change all newlines in block to spaces --- */
    while (DfCurrChar < bel)    {
        if (*DfCurrChar == '\n')    {
            *DfCurrChar = ' ';
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
        if ((int)(bbl - bb) == DfClientWidth(wnd)-1)    {
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
    DfBuildTextPointers(wnd);
    /* --- put DfCursor back at beginning --- */
    wnd->CurrLine = DfTextLineNumber(wnd, bl);
    wnd->CurrCol = bc;
    if (fl < wnd->wtop)
        wnd->wtop = fl;
    wnd->WndRow = fl - wnd->wtop;
    DfSendMessage(wnd, DFM_PAINT, 0, 0);
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
    wnd->TextChanged = TRUE;
    DfBuildTextPointers(wnd);
}
/* ----------- COMMAND Message ---------- */
static int CommandMsg(DFWINDOW wnd, DF_PARAM p1)
{
    switch ((int)p1)    {
        case DF_ID_DELETETEXT:
            DeleteTextCmd(wnd);
            return TRUE;
        case DF_ID_CLEAR:
            ClearCmd(wnd);
            return TRUE;
        case DF_ID_UNDO:
            UndoCmd(wnd);
            return TRUE;
        case DF_ID_PARAGRAPH:
            ParagraphCmd(wnd);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}
/* ---------- DFM_CLOSE_WINDOW Message ----------- */
static int CloseWindowMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
	int rtn;
    DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
    if (wnd->DeletedText != NULL)
        free(wnd->DeletedText);
	if (wnd->text != NULL)
	{
		free(wnd->text);
		wnd->text = NULL;
	}
    rtn = DfBaseWndProc(DF_EDITBOX, wnd, DFM_CLOSE_WINDOW, p1, p2);
    return rtn;
}

/* ------- Window processing module for DF_EDITBOX class ------ */
int DfEditBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int rtn;
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case DFM_ADDTEXT:
            return AddTextMsg(wnd, p1, p2);
        case DFM_SETTEXT:
            return SetTextMsg(wnd, p1);
        case DFM_CLEARTEXT:
			return ClearTextMsg(wnd);
        case DFM_GETTEXT:
            return GetTextMsg(wnd, p1, p2);
        case DFM_SETTEXTLENGTH:
            return SetTextLengthMsg(wnd, (unsigned) p1);
        case DFM_KEYBOARD_CURSOR:
            KeyboardCursorMsg(wnd, p1, p2);
			return TRUE;
        case DFM_SETFOCUS:
			if (!(int)p1)
				DfSendMessage(NULL, DFM_HIDE_CURSOR, 0, 0);
        case DFM_PAINT:
        case DFM_MOVE:
            rtn = DfBaseWndProc(DF_EDITBOX, wnd, msg, p1, p2);
            DfSendMessage(wnd,DFM_KEYBOARD_CURSOR,DfWndCol,wnd->WndRow);
            return rtn;
        case DFM_DFM_SIZE:
            return SizeMsg(wnd, p1, p2);
        case DFM_SCROLL:
            return ScrollMsg(wnd, p1);
        case DFM_HORIZSCROLL:
            return HorizScrollMsg(wnd, p1);
        case DFM_SCROLLPAGE:
            return ScrollPageMsg(wnd, p1);
        case DFM_HORIZPAGE:
            return HorizPageMsg(wnd, p1);
        case DFM_LEFT_BUTTON:
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
        case DFM_KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_SHIFT_CHANGED:
            ShiftChangedMsg(wnd, p1);
            break;
        case DFM_COMMAND:
            if (CommandMsg(wnd, p1))
                return TRUE;
            break;
        case DFM_CLOSE_WINDOW:
            return CloseWindowMsg(wnd, p1, p2);
        default:
            break;
    }
    return DfBaseWndProc(DF_EDITBOX, wnd, msg, p1, p2);
}
/* ------ save deleted text for the Undo command ------ */
static void SaveDeletedText(DFWINDOW wnd, char *bbl, int len)
{
    wnd->DeletedLength = len;
    wnd->DeletedText=DfRealloc(wnd->DeletedText,len);
    memmove(wnd->DeletedText, bbl, len);
}
/* ---- DfCursor right key: right one character position ---- */
static void Forward(DFWINDOW wnd)
{
    char *cc = DfCurrChar+1;
    if (*cc == '\0')
        return;
    if (*DfCurrChar == '\n')    {
        Home(wnd);
        Downward(wnd);
    }
    else    {
        wnd->CurrCol++;
        if (DfWndCol == DfClientWidth(wnd))
            DfSendMessage(wnd, DFM_HORIZSCROLL, TRUE, 0);
    }
}
/* ----- stick the moving DfCursor to the end of the line ---- */
static void StickEnd(DFWINDOW wnd)
{
    char *cp = DfTextLine(wnd, wnd->CurrLine);
    char *cp1 = strchr(cp, '\n');
    int len = cp1 ? (int) (cp1 - cp) : 0;
    wnd->CurrCol = min(len, wnd->CurrCol);
    if (wnd->wleft > wnd->CurrCol)    {
        wnd->wleft = max(0, wnd->CurrCol - 4);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
    }
    else if (wnd->CurrCol-wnd->wleft >= DfClientWidth(wnd))    {
        wnd->wleft = wnd->CurrCol - (DfClientWidth(wnd)-1);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
    }
}
/* --------- DfCursor down key: down one line --------- */
static void Downward(DFWINDOW wnd)
{
    if (DfIsMultiLine(wnd) &&
            wnd->WndRow+wnd->wtop+1 < wnd->wlines)  {
        wnd->CurrLine++;
        if (wnd->WndRow == DfClientHeight(wnd)-1)
            DfBaseWndProc(DF_EDITBOX, wnd, DFM_SCROLL, TRUE, 0);
        else
            wnd->WndRow++;
        StickEnd(wnd);
    }
}
/* -------- DfCursor up key: up one line ------------ */
static void Upward(DFWINDOW wnd)
{
    if (DfIsMultiLine(wnd) && wnd->CurrLine != 0)    {
        --wnd->CurrLine;
        if (wnd->WndRow == 0)
            DfBaseWndProc(DF_EDITBOX, wnd, DFM_SCROLL, FALSE, 0);
        else
            --wnd->WndRow;
        StickEnd(wnd);
    }
}
/* ---- DfCursor left key: left one character position ---- */
static void Backward(DFWINDOW wnd)
{
    if (wnd->CurrCol)    {
        --wnd->CurrCol;
        if (wnd->CurrCol < wnd->wleft)
            DfSendMessage(wnd, DFM_HORIZSCROLL, FALSE, 0);
    }
    else if (DfIsMultiLine(wnd) && wnd->CurrLine != 0)    {
        Upward(wnd);
        End(wnd);
    }
}
/* -------- End key: to end of line ------- */
static void End(DFWINDOW wnd)
{
    while (*DfCurrChar && *DfCurrChar != '\n')
        ++wnd->CurrCol;
    if (DfWndCol >= DfClientWidth(wnd))    {
        wnd->wleft = wnd->CurrCol - (DfClientWidth(wnd)-1);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
    }
}
/* -------- Home key: to beginning of line ------- */
static void Home(DFWINDOW wnd)
{
    wnd->CurrCol = 0;
    if (wnd->wleft != 0)    {
        wnd->wleft = 0;
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
    }
}
/* -- Ctrl+DfCursor right key: to beginning of next word -- */
static void NextWord(DFWINDOW wnd)
{
    int savetop = wnd->wtop;
    int saveleft = wnd->wleft;
    DfClearVisible(wnd);
    while (!isWhite(*DfCurrChar))    {
        char *cc = DfCurrChar+1;
        if (*cc == '\0')
            break;
        Forward(wnd);
    }
    while (isWhite(*DfCurrChar))    {
        char *cc = DfCurrChar+1;
        if (*cc == '\0')
            break;
        Forward(wnd);
    }
    DfSetVisible(wnd);
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
    if (wnd->wtop != savetop || wnd->wleft != saveleft)
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
}
/* -- Ctrl+DfCursor left key: to beginning of previous word -- */
static void PrevWord(DFWINDOW wnd)
{
    int savetop = wnd->wtop;
    int saveleft = wnd->wleft;
    DfClearVisible(wnd);
    Backward(wnd);
    while (isWhite(*DfCurrChar))    {
        if (wnd->CurrLine == 0 && wnd->CurrCol == 0)
            break;
        Backward(wnd);
    }
    while (wnd->CurrCol != 0 && !isWhite(*DfCurrChar))
        Backward(wnd);
    if (isWhite(*DfCurrChar))
        Forward(wnd);
    DfSetVisible(wnd);
    if (wnd->wleft != saveleft)
        if (wnd->CurrCol >= saveleft)
            if (wnd->CurrCol - saveleft < DfClientWidth(wnd))
                wnd->wleft = saveleft;
    DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, DfWndCol, wnd->WndRow);
    if (wnd->wtop != savetop || wnd->wleft != saveleft)
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
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
    DfClearTextBlock(wnd);
    /* ------ set the anchor ------ */
    wnd->BlkBegLine = wnd->BlkEndLine = my;
    wnd->BlkBegCol = wnd->BlkEndCol = mx;
    DfSendMessage(wnd, DFM_PAINT, 0, 0);
}

/* EOF */
