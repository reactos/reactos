/* ------------- textbox.c ------------ */

#include "dflat.h"

static void ComputeWindowTop(DFWINDOW);
static void ComputeWindowLeft(DFWINDOW);
static int ComputeVScrollBox(DFWINDOW);
static int ComputeHScrollBox(DFWINDOW);
static void MoveScrollBox(DFWINDOW, int);
static char *GetTextLine(DFWINDOW, int);

BOOL VSliding;
BOOL HSliding;

/* ------------ ADDTEXT Message -------------- */
static BOOL AddTextMsg(DFWINDOW wnd, char *txt)
{
    /* --- append text to the textbox's buffer --- */
    unsigned adln = strlen(txt);
    if (adln > (unsigned)0xfff0)
        return FALSE;
    if (wnd->text != NULL)    {
        /* ---- appending to existing text ---- */
        unsigned txln = strlen(wnd->text);
        if ((long)txln+adln > (unsigned) 0xfff0)
            return FALSE;
        if (txln+adln > wnd->textlen)    {
            wnd->text = DFrealloc(wnd->text, txln+adln+3);
            wnd->textlen = txln+adln+1;
        }
    }
    else    {
        /* ------ 1st text appended ------ */
        wnd->text = DFcalloc(1, adln+3);
        wnd->textlen = adln+1;
    }
    if (wnd->text != NULL)    {
        /* ---- append the text ---- */
        strcat(wnd->text, txt);
        strcat(wnd->text, "\n");
        BuildTextPointers(wnd);
		return TRUE;
    }
	return FALSE;
}

/* ------------ DELETETEXT Message -------------- */
static void DeleteTextMsg(DFWINDOW wnd, int lno)
{
	char *cp1 = TextLine(wnd, lno);
	--wnd->wlines;
	if (lno == wnd->wlines)
		*cp1 = '\0';
	else 	{
		char *cp2 = TextLine(wnd, lno+1);
		memmove(cp1, cp2, strlen(cp2)+1);
	}
    BuildTextPointers(wnd);
}

/* ------------ INSERTTEXT Message -------------- */
static void InsertTextMsg(DFWINDOW wnd, char *txt, int lno)
{
	if (AddTextMsg(wnd, txt))	{
		int len = strlen(txt);
		char *cp2 = TextLine(wnd, lno);
		char *cp1 = cp2+len+1;
		memmove(cp1, cp2, strlen(cp2)-len);
		strcpy(cp2, txt);
		*(cp2+len) = '\n';
	    BuildTextPointers(wnd);
	}
}

/* ------------ SETTEXT Message -------------- */
static void SetTextMsg(DFWINDOW wnd, char *txt)
{
    /* -- assign new text value to textbox buffer -- */
    unsigned int len = strlen(txt)+1;
	DfSendMessage(wnd, CLEARTEXT, 0, 0);
    wnd->textlen = len;
    wnd->text=DFrealloc(wnd->text, len+1);
    wnd->text[len] = '\0';
    strcpy(wnd->text, txt);
    BuildTextPointers(wnd);
}

/* ------------ CLEARTEXT Message -------------- */
static void ClearTextMsg(DFWINDOW wnd)
{
    /* ----- clear text from textbox ----- */
    if (wnd->text != NULL)
        free(wnd->text);
    wnd->text = NULL;
    wnd->textlen = 0;
    wnd->wlines = 0;
    wnd->textwidth = 0;
    wnd->wtop = wnd->wleft = 0;
    ClearTextBlock(wnd);
    ClearTextPointers(wnd);
}

/* ------------ KEYBOARD Message -------------- */
static int KeyboardMsg(DFWINDOW wnd, PARAM p1)
{
    switch ((int) p1)    {
        case UP:
            return DfSendMessage(wnd,SCROLL,FALSE,0);
        case DN:
            return DfSendMessage(wnd,SCROLL,TRUE,0);
        case FWD:
            return DfSendMessage(wnd,HORIZSCROLL,TRUE,0);
        case BS:
            return DfSendMessage(wnd,HORIZSCROLL,FALSE,0);
        case PGUP:
            return DfSendMessage(wnd,SCROLLPAGE,FALSE,0);
        case PGDN:
            return DfSendMessage(wnd,SCROLLPAGE,TRUE,0);
        case CTRL_PGUP:
            return DfSendMessage(wnd,HORIZPAGE,FALSE,0);
        case CTRL_PGDN:
            return DfSendMessage(wnd,HORIZPAGE,TRUE,0);
        case HOME:
            return DfSendMessage(wnd,SCROLLDOC,TRUE,0);
        case END:
            return DfSendMessage(wnd,SCROLLDOC,FALSE,0);
        default:
            break;
    }
    return FALSE;
}

/* ------------ LEFT_BUTTON Message -------------- */
static int LeftButtonMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int mx = (int) p1 - GetLeft(wnd);
    int my = (int) p2 - GetTop(wnd);
    if (TestAttribute(wnd, VSCROLLBAR) &&
                        mx == WindowWidth(wnd)-1)    {
        /* -------- in the right border ------- */
        if (my == 0 || my == ClientHeight(wnd)+1)
            /* --- above or below the scroll bar --- */
            return FALSE;
        if (my == 1)
            /* -------- top scroll button --------- */
            return DfSendMessage(wnd, SCROLL, FALSE, 0);
        if (my == ClientHeight(wnd))
            /* -------- bottom scroll button --------- */
            return DfSendMessage(wnd, SCROLL, TRUE, 0);
        /* ---------- in the scroll bar ----------- */
        if (!VSliding && my-1 == wnd->VScrollBox)    {
            DFRECT rc;
            VSliding = TRUE;
            rc.lf = rc.rt = GetRight(wnd);
            rc.tp = GetTop(wnd)+2;
            rc.bt = GetBottom(wnd)-2;
            return DfSendMessage(NULL, MOUSE_TRAVEL,
                (PARAM) &rc, 0);
        }
        if (my-1 < wnd->VScrollBox)
            return DfSendMessage(wnd,SCROLLPAGE,FALSE,0);
        if (my-1 > wnd->VScrollBox)
            return DfSendMessage(wnd,SCROLLPAGE,TRUE,0);
    }
    if (TestAttribute(wnd, HSCROLLBAR) &&
                        my == WindowHeight(wnd)-1) {
        /* -------- in the bottom border ------- */
        if (mx == 0 || my == ClientWidth(wnd)+1)
            /* ------  outside the scroll bar ---- */
            return FALSE;
        if (mx == 1)
            return DfSendMessage(wnd, HORIZSCROLL,FALSE,0);
        if (mx == WindowWidth(wnd)-2)
            return DfSendMessage(wnd, HORIZSCROLL,TRUE,0);
        if (!HSliding && mx-1 == wnd->HScrollBox)    {
            /* --- hit the scroll box --- */
            DFRECT rc;
            rc.lf = GetLeft(wnd)+2;
            rc.rt = GetRight(wnd)-2;
            rc.tp = rc.bt = GetBottom(wnd);
            /* - keep the mouse in the scroll bar - */
            DfSendMessage(NULL,MOUSE_TRAVEL,(PARAM)&rc,0);
            HSliding = TRUE;
            return TRUE;
        }
        if (mx-1 < wnd->HScrollBox)
            return DfSendMessage(wnd,HORIZPAGE,FALSE,0);
        if (mx-1 > wnd->HScrollBox)
            return DfSendMessage(wnd,HORIZPAGE,TRUE,0);
    }
    return FALSE;
}

/* ------------ MOUSE_MOVED Message -------------- */
static BOOL MouseMovedMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    int mx = (int) p1 - GetLeft(wnd);
    int my = (int) p2 - GetTop(wnd);
    if (VSliding)    {
        /* ---- dragging the vertical scroll box --- */
        if (my-1 != wnd->VScrollBox)    {
            foreground = FrameForeground(wnd);
            background = FrameBackground(wnd);
            wputch(wnd, SCROLLBARCHAR, WindowWidth(wnd)-1,
                    wnd->VScrollBox+1);
            wnd->VScrollBox = my-1;
            wputch(wnd, SCROLLBOXCHAR, WindowWidth(wnd)-1,
                    my);
        }
        return TRUE;
    }
    if (HSliding)    {
        /* --- dragging the horizontal scroll box --- */
        if (mx-1 != wnd->HScrollBox)    {
            foreground = FrameForeground(wnd);
            background = FrameBackground(wnd);
            wputch(wnd, SCROLLBARCHAR, wnd->HScrollBox+1,
                    WindowHeight(wnd)-1);
            wnd->HScrollBox = mx-1;
            wputch(wnd, SCROLLBOXCHAR, mx, WindowHeight(wnd)-1);
        }
        return TRUE;
    }
    return FALSE;
}

/* ------------ BUTTON_RELEASED Message -------------- */
static void ButtonReleasedMsg(DFWINDOW wnd)
{
    if (HSliding || VSliding)    {
        /* release the mouse ouside the scroll bar */
        DfSendMessage(NULL, MOUSE_TRAVEL, 0, 0);
        VSliding ? ComputeWindowTop(wnd):ComputeWindowLeft(wnd);
        DfSendMessage(wnd, PAINT, 0, 0);
        DfSendMessage(wnd, KEYBOARD_CURSOR, 0, 0);
        VSliding = HSliding = FALSE;
    }
}

/* ------------ SCROLL Message -------------- */
static BOOL ScrollMsg(DFWINDOW wnd, PARAM p1)
{
    /* ---- vertical scroll one line ---- */
    if (p1)    {
        /* ----- scroll one line up ----- */
        if (wnd->wtop+ClientHeight(wnd) >= wnd->wlines)
            return FALSE;
        wnd->wtop++;
    }
    else    {
        /* ----- scroll one line down ----- */
        if (wnd->wtop == 0)
            return FALSE;
        --wnd->wtop;
    }
    if (isVisible(wnd))    {
        DFRECT rc;
        rc = ClipRectangle(wnd, ClientRect(wnd));
        if (ValidRect(rc))    {
            /* ---- scroll the window ----- */
            if (wnd != inFocus)
                DfSendMessage(wnd, PAINT, 0, 0);
            else    {
                scroll_window(wnd, rc, (int)p1);
                if (!(int)p1)
                    /* -- write top line (down) -- */
                    WriteTextLine(wnd,NULL,wnd->wtop,FALSE);
                else    {
                    /* -- write bottom line (up) -- */
                    int y=RectBottom(rc)-GetClientTop(wnd);
                    WriteTextLine(wnd, NULL,
                        wnd->wtop+y, FALSE);
                }
            }
        }
        /* ---- reset the scroll box ---- */
        if (TestAttribute(wnd, VSCROLLBAR))    {
            int vscrollbox = ComputeVScrollBox(wnd);
            if (vscrollbox != wnd->VScrollBox)
                MoveScrollBox(wnd, vscrollbox);
        }
    }
    return TRUE;
}

/* ------------ HORIZSCROLL Message -------------- */
static BOOL HorizScrollMsg(DFWINDOW wnd, PARAM p1)
{
    /* --- horizontal scroll one column --- */
    if (p1)    {
        /* --- scroll left --- */
        if (wnd->wleft + ClientWidth(wnd)-1 >= wnd->textwidth)
			return FALSE;
        wnd->wleft++;
    }
    else	{
        /* --- scroll right --- */
        if (wnd->wleft == 0)
			return FALSE;
        --wnd->wleft;
	}
    DfSendMessage(wnd, PAINT, 0, 0);
	return TRUE;
}

/* ------------  SCROLLPAGE Message -------------- */
static void ScrollPageMsg(DFWINDOW wnd, PARAM p1)
{
    /* --- vertical scroll one page --- */
    if ((int) p1 == FALSE)    {
        /* ---- page up ---- */
        if (wnd->wtop)
            wnd->wtop -= ClientHeight(wnd);
    }
    else     {
        /* ---- page down ---- */
        if (wnd->wtop+ClientHeight(wnd) < wnd->wlines) {
            wnd->wtop += ClientHeight(wnd);
            if (wnd->wtop>wnd->wlines-ClientHeight(wnd))
                wnd->wtop=wnd->wlines-ClientHeight(wnd);
        }
    }
    if (wnd->wtop < 0)
        wnd->wtop = 0;
    DfSendMessage(wnd, PAINT, 0, 0);
}

/* ------------ HORIZSCROLLPAGE Message -------------- */
static void HorizScrollPageMsg(DFWINDOW wnd, PARAM p1)
{
    /* --- horizontal scroll one page --- */
    if ((int) p1 == FALSE)
        /* ---- page left ----- */
        wnd->wleft -= ClientWidth(wnd);
    else    {
        /* ---- page right ----- */
        wnd->wleft += ClientWidth(wnd);
        if (wnd->wleft > wnd->textwidth-ClientWidth(wnd))
            wnd->wleft = wnd->textwidth-ClientWidth(wnd);
    }
    if (wnd->wleft < 0)
        wnd->wleft = 0;
    DfSendMessage(wnd, PAINT, 0, 0);
}

/* ------------ SCROLLDOC Message -------------- */
static void ScrollDocMsg(DFWINDOW wnd, PARAM p1)
{
    /* --- scroll to beginning or end of document --- */
    if ((int) p1)
        wnd->wtop = wnd->wleft = 0;
    else if (wnd->wtop+ClientHeight(wnd) < wnd->wlines){
        wnd->wtop = wnd->wlines-ClientHeight(wnd);
        wnd->wleft = 0;
    }
    DfSendMessage(wnd, PAINT, 0, 0);
}

/* ------------ PAINT Message -------------- */
static void PaintMsg(DFWINDOW wnd, PARAM p1, PARAM p2)
{
    /* ------ paint the client area ----- */
    DFRECT rc, rcc;
    int y;
    char blankline[201];

    /* ----- build the rectangle to paint ----- */
    if ((DFRECT *)p1 == NULL)
        rc=RelativeWindowRect(wnd, WindowRect(wnd));
    else
        rc= *(DFRECT *)p1;
    if (TestAttribute(wnd, HASBORDER) &&
            RectRight(rc) >= WindowWidth(wnd)-1) {
        if (RectLeft(rc) >= WindowWidth(wnd)-1)
            return;
        RectRight(rc) = WindowWidth(wnd)-2;
    }
    rcc = AdjustRectangle(wnd, rc);

	if (!p2 && wnd != inFocus)
		ClipString++;

    /* ----- blank line for padding ----- */
    memset(blankline, ' ', SCREENWIDTH);
    blankline[RectRight(rcc)+1] = '\0';

    /* ------- each line within rectangle ------ */
    for (y = RectTop(rc); y <= RectBottom(rc); y++){
        int yy;
        /* ---- test outside of Client area ---- */
        if (TestAttribute(wnd,
                    HASBORDER | HASTITLEBAR))    {
            if (y < TopBorderAdj(wnd))
                continue;
            if (y > WindowHeight(wnd)-2)
                continue;
        }
        yy = y-TopBorderAdj(wnd);
        if (yy < wnd->wlines-wnd->wtop)
            /* ---- paint a text line ---- */
            WriteTextLine(wnd, &rc,
                        yy+wnd->wtop, FALSE);
        else    {
            /* ---- paint a blank line ---- */
            SetStandardColor(wnd);
            writeline(wnd, blankline+RectLeft(rcc),
                    RectLeft(rcc)+BorderAdj(wnd), y, FALSE);
        }
    }
    /* ------- position the scroll box ------- */
    if (TestAttribute(wnd, VSCROLLBAR|HSCROLLBAR)) {
        int hscrollbox = ComputeHScrollBox(wnd);
        int vscrollbox = ComputeVScrollBox(wnd);
        if (hscrollbox != wnd->HScrollBox ||
                vscrollbox != wnd->VScrollBox)    {
            wnd->HScrollBox = hscrollbox;
            wnd->VScrollBox = vscrollbox;
            DfSendMessage(wnd, BORDER, p1, 0);
        }
    }
	if (!p2 && wnd != inFocus)
		--ClipString;
}

/* ------------ CLOSE_WINDOW Message -------------- */
static void CloseWindowMsg(DFWINDOW wnd)
{
    DfSendMessage(wnd, CLEARTEXT, 0, 0);
    if (wnd->TextPointers != NULL)    {
        free(wnd->TextPointers);
        wnd->TextPointers = NULL;
    }
}

/* ----------- TEXTBOX Message-processing Module ----------- */
int TextBoxProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)    {
        case CREATE_WINDOW:
            wnd->HScrollBox = wnd->VScrollBox = 1;
            ClearTextPointers(wnd);
            break;
        case ADDTEXT:
            return AddTextMsg(wnd, (char *) p1);
		case DELETETEXT:
            DeleteTextMsg(wnd, (int) p1);
            return TRUE;
		case INSERTTEXT:
            InsertTextMsg(wnd, (char *) p1, (int) p2);
            return TRUE;
        case SETTEXT:
            SetTextMsg(wnd, (char *) p1);
			return TRUE;
        case CLEARTEXT:
            ClearTextMsg(wnd);
            break;
        case KEYBOARD:
            if (WindowMoving || WindowSizing)
                break;
            if (KeyboardMsg(wnd, p1))
                return TRUE;
            break;
        case LEFT_BUTTON:
            if (WindowSizing || WindowMoving)
                return FALSE;
            if (LeftButtonMsg(wnd, p1, p2))
                return TRUE;
            break;
        case MOUSE_MOVED:
            if (MouseMovedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case DFM_BUTTON_RELEASED:
            ButtonReleasedMsg(wnd);
            break;
        case SCROLL:
            return ScrollMsg(wnd, p1);
        case HORIZSCROLL:
            return HorizScrollMsg(wnd, p1);
        case SCROLLPAGE:
            ScrollPageMsg(wnd, p1);
            return TRUE;
        case HORIZPAGE:
            HorizScrollPageMsg(wnd, p1);
            return TRUE;
        case SCROLLDOC:
            ScrollDocMsg(wnd, p1);
            return TRUE;
        case PAINT:
            if (isVisible(wnd))
            {
                PaintMsg(wnd, p1, p2);
                return FALSE;
            }
            break;
        case CLOSE_WINDOW:
            CloseWindowMsg(wnd);
            break;
        default:
            break;
    }
    return BaseWndProc(TEXTBOX, wnd, msg, p1, p2);
}

/* ------ compute the vertical scroll box position from
                   the text pointers --------- */
static int ComputeVScrollBox(DFWINDOW wnd)
{
    int pagelen = wnd->wlines - ClientHeight(wnd);
    int barlen = ClientHeight(wnd)-2;
    int lines_tick;
    int vscrollbox;

    if (pagelen < 1 || barlen < 1)
        vscrollbox = 1;
    else    {
        if (pagelen > barlen)
            lines_tick = pagelen / barlen;
        else
            lines_tick = barlen / pagelen;
        vscrollbox = 1 + (wnd->wtop / lines_tick);
        if (vscrollbox > ClientHeight(wnd)-2 ||
                wnd->wtop + ClientHeight(wnd) >= wnd->wlines)
            vscrollbox = ClientHeight(wnd)-2;
    }
    return vscrollbox;
}

/* ---- compute top text line from scroll box position ---- */
static void ComputeWindowTop(DFWINDOW wnd)
{
    int pagelen = wnd->wlines - ClientHeight(wnd);
    if (wnd->VScrollBox == 0)
        wnd->wtop = 0;
    else if (wnd->VScrollBox == ClientHeight(wnd)-2)
        wnd->wtop = pagelen;
    else    {
        int barlen = ClientHeight(wnd)-2;
        int lines_tick;

        if (pagelen > barlen)
            lines_tick = barlen ? (pagelen / barlen) : 0;
        else
            lines_tick = pagelen ? (barlen / pagelen) : 0;
        wnd->wtop = (wnd->VScrollBox-1) * lines_tick;
        if (wnd->wtop + ClientHeight(wnd) > wnd->wlines)
            wnd->wtop = pagelen;
    }
    if (wnd->wtop < 0)
        wnd->wtop = 0;
}

/* ------ compute the horizontal scroll box position from
                   the text pointers --------- */
static int ComputeHScrollBox(DFWINDOW wnd)
{
    int pagewidth = wnd->textwidth - ClientWidth(wnd);
    int barlen = ClientWidth(wnd)-2;
    int chars_tick;
    int hscrollbox;

    if (pagewidth < 1 || barlen < 1)
        hscrollbox = 1;
    else     {
        if (pagewidth > barlen)
            chars_tick = barlen ? (pagewidth / barlen) : 0;
        else
            chars_tick = pagewidth ? (barlen / pagewidth) : 0;
        hscrollbox = 1 + (chars_tick ? (wnd->wleft / chars_tick) : 0);
        if (hscrollbox > ClientWidth(wnd)-2 ||
                wnd->wleft + ClientWidth(wnd) >= wnd->textwidth)
            hscrollbox = ClientWidth(wnd)-2;
    }
    return hscrollbox;
}

/* ---- compute left column from scroll box position ---- */
static void ComputeWindowLeft(DFWINDOW wnd)
{
    int pagewidth = wnd->textwidth - ClientWidth(wnd);

    if (wnd->HScrollBox == 0)
        wnd->wleft = 0;
    else if (wnd->HScrollBox == ClientWidth(wnd)-2)
        wnd->wleft = pagewidth;
    else    {
        int barlen = ClientWidth(wnd)-2;
        int chars_tick;

        if (pagewidth > barlen)
            chars_tick = pagewidth / barlen;
        else
            chars_tick = barlen / pagewidth;
        wnd->wleft = (wnd->HScrollBox-1) * chars_tick;
        if (wnd->wleft + ClientWidth(wnd) > wnd->textwidth)
            wnd->wleft = pagewidth;
    }
    if (wnd->wleft < 0)
        wnd->wleft = 0;
}

/* ----- get the text to a specified line ----- */
static char *GetTextLine(DFWINDOW wnd, int selection)
{
    char *line;
    int len = 0;
    char *cp, *cp1;
    cp = cp1 = TextLine(wnd, selection);
    while (*cp && *cp != '\n')    {
        len++;
        cp++;
    }
    line = DFmalloc(len+7);
    memmove(line, cp1, len);
    line[len] = '\0';
    return line;
}

/* ------- write a line of text to a textbox window ------- */
void WriteTextLine(DFWINDOW wnd, DFRECT *rcc, int y, BOOL reverse)
{
    int len = 0;
    int dif = 0;
    unsigned char line[200];
    DFRECT rc;
    unsigned char *lp, *svlp;
    int lnlen;
    int i;
    BOOL trunc = FALSE;

    /* ------ make sure y is inside the window ----- */
    if (y < wnd->wtop || y >= wnd->wtop+ClientHeight(wnd))
        return;

    /* ---- build the retangle within which can write ---- */
    if (rcc == NULL)    {
        rc = RelativeWindowRect(wnd, WindowRect(wnd));
        if (TestAttribute(wnd, HASBORDER) &&
                RectRight(rc) >= WindowWidth(wnd)-1)
            RectRight(rc) = WindowWidth(wnd)-2;
    }
    else
        rc = *rcc;

    /* ----- make sure rectangle is within window ------ */
    if (RectLeft(rc) >= WindowWidth(wnd)-1)
        return;
    if (RectRight(rc) == 0)
        return;
    rc = AdjustRectangle(wnd, rc);
    if (y-wnd->wtop<RectTop(rc) || y-wnd->wtop>RectBottom(rc))
        return;

    /* --- get the text and length of the text line --- */
    lp = svlp = GetTextLine(wnd, y);
    if (svlp == NULL)
        return;
    lnlen = LineLength(lp);

    /* -------- insert block color change controls ------- */
    if (TextBlockMarked(wnd))    {
        int bbl = wnd->BlkBegLine;
        int bel = wnd->BlkEndLine;
        int bbc = wnd->BlkBegCol;
        int bec = wnd->BlkEndCol;
        int by = y;

        /* ----- put lowest marker first ----- */
        if (bbl > bel)    {
            swap(bbl, bel);
            swap(bbc, bec);
        }
        if (bbl == bel && bbc > bec)
            swap(bbc, bec);

        if (by >= bbl && by <= bel)    {
            /* ------ the block includes this line ----- */
            int blkbeg = 0;
            int blkend = lnlen;
            if (!(by > bbl && by < bel))    {
                /* --- the entire line is not in the block -- */
                if (by == bbl)
                    /* ---- the block begins on this line --- */
                    blkbeg = bbc;
                if (by == bel)
                    /* ---- the block ends on this line ---- */
                    blkend = bec;
            }
			if (blkend == 0 && lnlen == 0)	{
				strcpy(lp, " ");
				blkend++;
			}
            /* ----- insert the reset color token ----- */
            memmove(lp+blkend+1,lp+blkend,strlen(lp+blkend)+1);
            lp[blkend] = RESETCOLOR;
            /* ----- insert the change color token ----- */
            memmove(lp+blkbeg+3,lp+blkbeg,strlen(lp+blkbeg)+1);
            lp[blkbeg] = CHANGECOLOR;
            /* ----- insert the color tokens ----- */
            SetReverseColor(wnd);
            lp[blkbeg+1] = foreground | 0x80;
            lp[blkbeg+2] = background | 0x80;
            lnlen += 4;
        }
    }
    /* - make sure left margin doesn't overlap color change - */
    for (i = 0; i < wnd->wleft+3; i++)    {
        if (*(lp+i) == '\0')
            break;
        if (*(unsigned char *)(lp + i) == RESETCOLOR)
            break;
    }
    if (*(lp+i) && i < wnd->wleft+3)    {
        if (wnd->wleft+4 > lnlen)
            trunc = TRUE;
        else 
            lp += 4;
    }
    else     {
        /* --- it does, shift the color change over --- */
        for (i = 0; i < wnd->wleft; i++)    {
            if (*(lp+i) == '\0')
                break;
            if (*(unsigned char *)(lp + i) == CHANGECOLOR)    {
                *(lp+wnd->wleft+2) = *(lp+i+2);
                *(lp+wnd->wleft+1) = *(lp+i+1);
                *(lp+wnd->wleft) = *(lp+i);
                break;
            }
        }
    }
    /* ------ build the line to display -------- */
    if (!trunc)    {
        if (lnlen < wnd->wleft)
            lnlen = 0;
        else
            lp += wnd->wleft;
        if (lnlen > RectLeft(rc))    {
            /* ---- the line exceeds the rectangle ---- */
            int ct = RectLeft(rc);
            char *initlp = lp;
            /* --- point to end of clipped line --- */
            while (ct)    {
                if (*(unsigned char *)lp == CHANGECOLOR)
                    lp += 3;
                else if (*(unsigned char *)lp == RESETCOLOR)
                    lp++;
                else
                    lp++, --ct;
            }
            if (RectLeft(rc))    {
                char *lpp = lp;
                while (*lpp)    {
                    if (*(unsigned char*)lpp==CHANGECOLOR)
                        break;
                    if (*(unsigned char*)lpp==RESETCOLOR) {
                        lpp = lp;
                        while (lpp >= initlp)    {
                            if (*(unsigned char *)lpp ==
                                            CHANGECOLOR) {
                                lp -= 3;
                                memmove(lp,lpp,3);
                                break;
                            }
                            --lpp;
                        }
                        break;
                    }
                    lpp++;
                }
            }
            lnlen = LineLength(lp);
            len = min(lnlen, RectWidth(rc));
            dif = strlen(lp) - lnlen;
            len += dif;
            if (len > 0)
                strncpy(line, lp, len);
        }
    }
    /* -------- pad the line --------- */
    while (len < RectWidth(rc)+dif)
        line[len++] = ' ';
    line[len] = '\0';
    dif = 0;
    /* ------ establish the line's main color ----- */
    if (reverse)    {
        char *cp = line;
        SetReverseColor(wnd);
        while ((cp = strchr(cp, CHANGECOLOR)) != NULL)    {
            cp += 2;
            *cp++ = background | 0x80;
        }
        if (*(unsigned char *)line == CHANGECOLOR)
            dif = 3;
    }
    else
        SetStandardColor(wnd);
    /* ------- display the line -------- */
    writeline(wnd, line+dif,
                RectLeft(rc)+BorderAdj(wnd),
                    y-wnd->wtop+TopBorderAdj(wnd), FALSE);
    free(svlp);
}

void MarkTextBlock(DFWINDOW wnd, int BegLine, int BegCol,
                               int EndLine, int EndCol)
{
    wnd->BlkBegLine = BegLine;
    wnd->BlkEndLine = EndLine;
    wnd->BlkBegCol = BegCol;
    wnd->BlkEndCol = EndCol;
}

/* ----- clear and initialize text line pointer array ----- */
void ClearTextPointers(DFWINDOW wnd)
{
    wnd->TextPointers = DFrealloc(wnd->TextPointers, sizeof(int));
    *(wnd->TextPointers) = 0;
}

#define INITLINES 100

/* ---- build array of pointers to text lines ---- */
void BuildTextPointers(DFWINDOW wnd)
{
    char *cp = wnd->text, *cp1;
    int incrs = INITLINES;
    unsigned int off;
    wnd->textwidth = wnd->wlines = 0;
    while (*cp)    {
        if (incrs == INITLINES)    {
            incrs = 0;
            wnd->TextPointers = DFrealloc(wnd->TextPointers,
                    (wnd->wlines + INITLINES) * sizeof(int));
        }
        off = (unsigned int) ((unsigned int)cp - (unsigned int)wnd->text);
        *((wnd->TextPointers) + wnd->wlines) = off;
        wnd->wlines++;
        incrs++;
        cp1 = cp;
        while (*cp && *cp != '\n')
            cp++;
        wnd->textwidth = max(wnd->textwidth, (int)(cp - cp1));
        if (*cp)
            cp++;
    }
}

static void MoveScrollBox(DFWINDOW wnd, int vscrollbox)
{
    foreground = FrameForeground(wnd);
    background = FrameBackground(wnd);
    wputch(wnd, SCROLLBARCHAR, WindowWidth(wnd)-1,
            wnd->VScrollBox+1);
    wputch(wnd, SCROLLBOXCHAR, WindowWidth(wnd)-1,
            vscrollbox+1);
    wnd->VScrollBox = vscrollbox;
}

int TextLineNumber(DFWINDOW wnd, char *lp)
{
    int lineno;
    char *cp;
    for (lineno = 0; lineno < wnd->wlines; lineno++)    {
        cp = wnd->text + *((wnd->TextPointers) + lineno);
        if (cp == lp)
            return lineno;
        if (cp > lp)
            break;
    }
    return lineno-1;
}

/* EOF */
