/* ------------- textbox.c ------------ */

#include "dflat.h"

static void ComputeWindowTop(DFWINDOW);
static void ComputeWindowLeft(DFWINDOW);
static int ComputeVScrollBox(DFWINDOW);
static int ComputeHScrollBox(DFWINDOW);
static void MoveScrollBox(DFWINDOW, int);
static char *GetTextLine(DFWINDOW, int);

BOOL DfVSliding;
BOOL DfHSliding;

/* ------------ DFM_ADDTEXT Message -------------- */
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
            wnd->text = DfRealloc(wnd->text, txln+adln+3);
            wnd->textlen = txln+adln+1;
        }
    }
    else    {
        /* ------ 1st text appended ------ */
        wnd->text = DfCalloc(1, adln+3);
        wnd->textlen = adln+1;
    }
    if (wnd->text != NULL)    {
        /* ---- append the text ---- */
        strcat(wnd->text, txt);
        strcat(wnd->text, "\n");
        DfBuildTextPointers(wnd);
		return TRUE;
    }
	return FALSE;
}

/* ------------ DFM_DELETETEXT Message -------------- */
static void DeleteTextMsg(DFWINDOW wnd, int lno)
{
	char *cp1 = DfTextLine(wnd, lno);
	--wnd->wlines;
	if (lno == wnd->wlines)
		*cp1 = '\0';
	else 	{
		char *cp2 = DfTextLine(wnd, lno+1);
		memmove(cp1, cp2, strlen(cp2)+1);
	}
    DfBuildTextPointers(wnd);
}

/* ------------ DFM_INSERTTEXT Message -------------- */
static void InsertTextMsg(DFWINDOW wnd, char *txt, int lno)
{
	if (AddTextMsg(wnd, txt))	{
		int len = strlen(txt);
		char *cp2 = DfTextLine(wnd, lno);
		char *cp1 = cp2+len+1;
		memmove(cp1, cp2, strlen(cp2)-len);
		strcpy(cp2, txt);
		*(cp2+len) = '\n';
	    DfBuildTextPointers(wnd);
	}
}

/* ------------ DFM_SETTEXT Message -------------- */
static void SetTextMsg(DFWINDOW wnd, char *txt)
{
    /* -- assign new text value to textbox buffer -- */
    unsigned int len = strlen(txt)+1;
	DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
    wnd->textlen = len;
    wnd->text=DfRealloc(wnd->text, len+1);
    wnd->text[len] = '\0';
    strcpy(wnd->text, txt);
    DfBuildTextPointers(wnd);
}

/* ------------ DFM_CLEARTEXT Message -------------- */
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
    DfClearTextBlock(wnd);
    DfClearTextPointers(wnd);
}

/* ------------ DFM_KEYBOARD Message -------------- */
static int KeyboardMsg(DFWINDOW wnd, DF_PARAM p1)
{
    switch ((int) p1)    {
        case DF_UP:
            return DfSendMessage(wnd,DFM_SCROLL,FALSE,0);
        case DF_DN:
            return DfSendMessage(wnd,DFM_SCROLL,TRUE,0);
        case DF_FWD:
            return DfSendMessage(wnd,DFM_HORIZSCROLL,TRUE,0);
        case DF_BS:
            return DfSendMessage(wnd,DFM_HORIZSCROLL,FALSE,0);
        case DF_PGUP:
            return DfSendMessage(wnd,DFM_SCROLLPAGE,FALSE,0);
        case DF_PGDN:
            return DfSendMessage(wnd,DFM_SCROLLPAGE,TRUE,0);
        case DF_CTRL_PGUP:
            return DfSendMessage(wnd,DFM_HORIZPAGE,FALSE,0);
        case DF_CTRL_PGDN:
            return DfSendMessage(wnd,DFM_HORIZPAGE,TRUE,0);
        case DF_HOME:
            return DfSendMessage(wnd,DFM_SCROLLDOC,TRUE,0);
        case DF_END:
            return DfSendMessage(wnd,DFM_SCROLLDOC,FALSE,0);
        default:
            break;
    }
    return FALSE;
}

/* ------------ DFM_LEFT_BUTTON Message -------------- */
static int LeftButtonMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int mx = (int) p1 - DfGetLeft(wnd);
    int my = (int) p2 - DfGetTop(wnd);
    if (DfTestAttribute(wnd, DF_VSCROLLBAR) &&
                        mx == DfWindowWidth(wnd)-1)    {
        /* -------- in the right border ------- */
        if (my == 0 || my == DfClientHeight(wnd)+1)
            /* --- above or below the scroll bar --- */
            return FALSE;
        if (my == 1)
            /* -------- top scroll button --------- */
            return DfSendMessage(wnd, DFM_SCROLL, FALSE, 0);
        if (my == DfClientHeight(wnd))
            /* -------- bottom scroll button --------- */
            return DfSendMessage(wnd, DFM_SCROLL, TRUE, 0);
        /* ---------- in the scroll bar ----------- */
        if (!DfVSliding && my-1 == wnd->VScrollBox)    {
            DFRECT rc;
            DfVSliding = TRUE;
            rc.lf = rc.rt = DfGetRight(wnd);
            rc.tp = DfGetTop(wnd)+2;
            rc.bt = DfGetBottom(wnd)-2;
            return DfSendMessage(NULL, DFM_MOUSE_TRAVEL,
                (DF_PARAM) &rc, 0);
        }
        if (my-1 < wnd->VScrollBox)
            return DfSendMessage(wnd,DFM_SCROLLPAGE,FALSE,0);
        if (my-1 > wnd->VScrollBox)
            return DfSendMessage(wnd,DFM_SCROLLPAGE,TRUE,0);
    }
    if (DfTestAttribute(wnd, DF_HSCROLLBAR) &&
                        my == DfWindowHeight(wnd)-1) {
        /* -------- in the bottom border ------- */
        if (mx == 0 || my == DfClientWidth(wnd)+1)
            /* ------  outside the scroll bar ---- */
            return FALSE;
        if (mx == 1)
            return DfSendMessage(wnd, DFM_HORIZSCROLL,FALSE,0);
        if (mx == DfWindowWidth(wnd)-2)
            return DfSendMessage(wnd, DFM_HORIZSCROLL,TRUE,0);
        if (!DfHSliding && mx-1 == wnd->HScrollBox)    {
            /* --- hit the scroll box --- */
            DFRECT rc;
            rc.lf = DfGetLeft(wnd)+2;
            rc.rt = DfGetRight(wnd)-2;
            rc.tp = rc.bt = DfGetBottom(wnd);
            /* - keep the mouse in the scroll bar - */
            DfSendMessage(NULL,DFM_MOUSE_TRAVEL,(DF_PARAM)&rc,0);
            DfHSliding = TRUE;
            return TRUE;
        }
        if (mx-1 < wnd->HScrollBox)
            return DfSendMessage(wnd,DFM_HORIZPAGE,FALSE,0);
        if (mx-1 > wnd->HScrollBox)
            return DfSendMessage(wnd,DFM_HORIZPAGE,TRUE,0);
    }
    return FALSE;
}

/* ------------ MOUSE_MOVED Message -------------- */
static BOOL MouseMovedMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    int mx = (int) p1 - DfGetLeft(wnd);
    int my = (int) p2 - DfGetTop(wnd);
    if (DfVSliding)    {
        /* ---- dragging the vertical scroll box --- */
        if (my-1 != wnd->VScrollBox)    {
            DfForeground = DfFrameForeground(wnd);
            DfBackground = DfFrameBackground(wnd);
            DfWPutch(wnd, DF_SCROLLBARCHAR, DfWindowWidth(wnd)-1,
                    wnd->VScrollBox+1);
            wnd->VScrollBox = my-1;
            DfWPutch(wnd, DF_SCROLLBOXCHAR, DfWindowWidth(wnd)-1,
                    my);
        }
        return TRUE;
    }
    if (DfHSliding)    {
        /* --- dragging the horizontal scroll box --- */
        if (mx-1 != wnd->HScrollBox)    {
            DfForeground = DfFrameForeground(wnd);
            DfBackground = DfFrameBackground(wnd);
            DfWPutch(wnd, DF_SCROLLBARCHAR, wnd->HScrollBox+1,
                    DfWindowHeight(wnd)-1);
            wnd->HScrollBox = mx-1;
            DfWPutch(wnd, DF_SCROLLBOXCHAR, mx, DfWindowHeight(wnd)-1);
        }
        return TRUE;
    }
    return FALSE;
}

/* ------------ BUTTON_RELEASED Message -------------- */
static void ButtonReleasedMsg(DFWINDOW wnd)
{
    if (DfHSliding || DfVSliding)    {
        /* release the mouse ouside the scroll bar */
        DfSendMessage(NULL, DFM_MOUSE_TRAVEL, 0, 0);
        DfVSliding ? ComputeWindowTop(wnd):ComputeWindowLeft(wnd);
        DfSendMessage(wnd, DFM_PAINT, 0, 0);
        DfSendMessage(wnd, DFM_KEYBOARD_CURSOR, 0, 0);
        DfVSliding = DfHSliding = FALSE;
    }
}

/* ------------ DFM_SCROLL Message -------------- */
static BOOL ScrollMsg(DFWINDOW wnd, DF_PARAM p1)
{
    /* ---- vertical scroll one line ---- */
    if (p1)    {
        /* ----- scroll one line up ----- */
        if (wnd->wtop+DfClientHeight(wnd) >= wnd->wlines)
            return FALSE;
        wnd->wtop++;
    }
    else    {
        /* ----- scroll one line down ----- */
        if (wnd->wtop == 0)
            return FALSE;
        --wnd->wtop;
    }
    if (DfIsVisible(wnd))    {
        DFRECT rc;
        rc = DfClipRectangle(wnd, DfClientRect(wnd));
        if (DfValidRect(rc))    {
            /* ---- scroll the window ----- */
            if (wnd != DfInFocus)
                DfSendMessage(wnd, DFM_PAINT, 0, 0);
            else    {
                DfScrollWindow(wnd, rc, (int)p1);
                if (!(int)p1)
                    /* -- write top line (down) -- */
                    DfWriteTextLine(wnd,NULL,wnd->wtop,FALSE);
                else    {
                    /* -- write bottom line (up) -- */
                    int y=DfRectBottom(rc)-DfGetClientTop(wnd);
                    DfWriteTextLine(wnd, NULL,
                        wnd->wtop+y, FALSE);
                }
            }
        }
        /* ---- reset the scroll box ---- */
        if (DfTestAttribute(wnd, DF_VSCROLLBAR))    {
            int vscrollbox = ComputeVScrollBox(wnd);
            if (vscrollbox != wnd->VScrollBox)
                MoveScrollBox(wnd, vscrollbox);
        }
    }
    return TRUE;
}

/* ------------ DFM_HORIZSCROLL Message -------------- */
static BOOL HorizScrollMsg(DFWINDOW wnd, DF_PARAM p1)
{
    /* --- horizontal scroll one column --- */
    if (p1)    {
        /* --- scroll left --- */
        if (wnd->wleft + DfClientWidth(wnd)-1 >= wnd->textwidth)
			return FALSE;
        wnd->wleft++;
    }
    else	{
        /* --- scroll right --- */
        if (wnd->wleft == 0)
			return FALSE;
        --wnd->wleft;
	}
    DfSendMessage(wnd, DFM_PAINT, 0, 0);
	return TRUE;
}

/* ------------  DFM_SCROLLPAGE Message -------------- */
static void ScrollPageMsg(DFWINDOW wnd, DF_PARAM p1)
{
    /* --- vertical scroll one page --- */
    if ((int) p1 == FALSE)    {
        /* ---- page up ---- */
        if (wnd->wtop)
            wnd->wtop -= DfClientHeight(wnd);
    }
    else     {
        /* ---- page down ---- */
        if (wnd->wtop+DfClientHeight(wnd) < wnd->wlines) {
            wnd->wtop += DfClientHeight(wnd);
            if (wnd->wtop>wnd->wlines-DfClientHeight(wnd))
                wnd->wtop=wnd->wlines-DfClientHeight(wnd);
        }
    }
    if (wnd->wtop < 0)
        wnd->wtop = 0;
    DfSendMessage(wnd, DFM_PAINT, 0, 0);
}

/* ------------ HORIZSCROLLPAGE Message -------------- */
static void HorizScrollPageMsg(DFWINDOW wnd, DF_PARAM p1)
{
    /* --- horizontal scroll one page --- */
    if ((int) p1 == FALSE)
        /* ---- page left ----- */
        wnd->wleft -= DfClientWidth(wnd);
    else    {
        /* ---- page right ----- */
        wnd->wleft += DfClientWidth(wnd);
        if (wnd->wleft > wnd->textwidth-DfClientWidth(wnd))
            wnd->wleft = wnd->textwidth-DfClientWidth(wnd);
    }
    if (wnd->wleft < 0)
        wnd->wleft = 0;
    DfSendMessage(wnd, DFM_PAINT, 0, 0);
}

/* ------------ DFM_SCROLLDOC Message -------------- */
static void ScrollDocMsg(DFWINDOW wnd, DF_PARAM p1)
{
    /* --- scroll to beginning or end of document --- */
    if ((int) p1)
        wnd->wtop = wnd->wleft = 0;
    else if (wnd->wtop+DfClientHeight(wnd) < wnd->wlines){
        wnd->wtop = wnd->wlines-DfClientHeight(wnd);
        wnd->wleft = 0;
    }
    DfSendMessage(wnd, DFM_PAINT, 0, 0);
}

/* ------------ DFM_PAINT Message -------------- */
static void PaintMsg(DFWINDOW wnd, DF_PARAM p1, DF_PARAM p2)
{
    /* ------ paint the client area ----- */
    DFRECT rc, rcc;
    int y;
    char blankline[201];

    /* ----- build the rectangle to paint ----- */
    if ((DFRECT *)p1 == NULL)
        rc=DfRelativeWindowRect(wnd, DfWindowRect(wnd));
    else
        rc= *(DFRECT *)p1;
    if (DfTestAttribute(wnd, DF_HASBORDER) &&
            DfRectRight(rc) >= DfWindowWidth(wnd)-1) {
        if (DfRectLeft(rc) >= DfWindowWidth(wnd)-1)
            return;
        DfRectRight(rc) = DfWindowWidth(wnd)-2;
    }
    rcc = DfAdjustRectangle(wnd, rc);

	if (!p2 && wnd != DfInFocus)
		DfClipString++;

    /* ----- blank line for padding ----- */
    memset(blankline, ' ', DfGetScreenWidth());
    blankline[DfRectRight(rcc)+1] = '\0';

    /* ------- each line DfWithin rectangle ------ */
    for (y = DfRectTop(rc); y <= DfRectBottom(rc); y++){
        int yy;
        /* ---- test outside of Client area ---- */
        if (DfTestAttribute(wnd,
                    DF_HASBORDER | DF_HASTITLEBAR))    {
            if (y < DfTopBorderAdj(wnd))
                continue;
            if (y > DfWindowHeight(wnd)-2)
                continue;
        }
        yy = y-DfTopBorderAdj(wnd);
        if (yy < wnd->wlines-wnd->wtop)
            /* ---- paint a text line ---- */
            DfWriteTextLine(wnd, &rc,
                        yy+wnd->wtop, FALSE);
        else    {
            /* ---- paint a blank line ---- */
            DfSetStandardColor(wnd);
            DfWriteLine(wnd, blankline+DfRectLeft(rcc),
                    DfRectLeft(rcc)+DfBorderAdj(wnd), y, FALSE);
        }
    }
    /* ------- position the scroll box ------- */
    if (DfTestAttribute(wnd, DF_VSCROLLBAR|DF_HSCROLLBAR)) {
        int hscrollbox = ComputeHScrollBox(wnd);
        int vscrollbox = ComputeVScrollBox(wnd);
        if (hscrollbox != wnd->HScrollBox ||
                vscrollbox != wnd->VScrollBox)    {
            wnd->HScrollBox = hscrollbox;
            wnd->VScrollBox = vscrollbox;
            DfSendMessage(wnd, DFM_BORDER, p1, 0);
        }
    }
	if (!p2 && wnd != DfInFocus)
		--DfClipString;
}

/* ------------ DFM_CLOSE_WINDOW Message -------------- */
static void CloseWindowMsg(DFWINDOW wnd)
{
    DfSendMessage(wnd, DFM_CLEARTEXT, 0, 0);
    if (wnd->TextPointers != NULL)    {
        free(wnd->TextPointers);
        wnd->TextPointers = NULL;
    }
}

/* ----------- DF_TEXTBOX Message-processing Module ----------- */
int DfTextBoxProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            wnd->HScrollBox = wnd->VScrollBox = 1;
            DfClearTextPointers(wnd);
            break;
        case DFM_ADDTEXT:
            return AddTextMsg(wnd, (char *) p1);
		case DFM_DELETETEXT:
            DeleteTextMsg(wnd, (int) p1);
            return TRUE;
		case DFM_INSERTTEXT:
            InsertTextMsg(wnd, (char *) p1, (int) p2);
            return TRUE;
        case DFM_SETTEXT:
            SetTextMsg(wnd, (char *) p1);
			return TRUE;
        case DFM_CLEARTEXT:
            ClearTextMsg(wnd);
            break;
        case DFM_KEYBOARD:
            if (DfWindowMoving || DfWindowSizing)
                break;
            if (KeyboardMsg(wnd, p1))
                return TRUE;
            break;
        case DFM_LEFT_BUTTON:
            if (DfWindowSizing || DfWindowMoving)
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
        case DFM_SCROLL:
            return ScrollMsg(wnd, p1);
        case DFM_HORIZSCROLL:
            return HorizScrollMsg(wnd, p1);
        case DFM_SCROLLPAGE:
            ScrollPageMsg(wnd, p1);
            return TRUE;
        case DFM_HORIZPAGE:
            HorizScrollPageMsg(wnd, p1);
            return TRUE;
        case DFM_SCROLLDOC:
            ScrollDocMsg(wnd, p1);
            return TRUE;
        case DFM_PAINT:
            if (DfIsVisible(wnd))
            {
                PaintMsg(wnd, p1, p2);
                return FALSE;
            }
            break;
        case DFM_CLOSE_WINDOW:
            CloseWindowMsg(wnd);
            break;
        default:
            break;
    }
    return DfBaseWndProc(DF_TEXTBOX, wnd, msg, p1, p2);
}

/* ------ compute the vertical scroll box position from
                   the text pointers --------- */
static int ComputeVScrollBox(DFWINDOW wnd)
{
    int pagelen = wnd->wlines - DfClientHeight(wnd);
    int barlen = DfClientHeight(wnd)-2;
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
        if (vscrollbox > DfClientHeight(wnd)-2 ||
                wnd->wtop + DfClientHeight(wnd) >= wnd->wlines)
            vscrollbox = DfClientHeight(wnd)-2;
    }
    return vscrollbox;
}

/* ---- compute top text line from scroll box position ---- */
static void ComputeWindowTop(DFWINDOW wnd)
{
    int pagelen = wnd->wlines - DfClientHeight(wnd);
    if (wnd->VScrollBox == 0)
        wnd->wtop = 0;
    else if (wnd->VScrollBox == DfClientHeight(wnd)-2)
        wnd->wtop = pagelen;
    else    {
        int barlen = DfClientHeight(wnd)-2;
        int lines_tick;

        if (pagelen > barlen)
            lines_tick = barlen ? (pagelen / barlen) : 0;
        else
            lines_tick = pagelen ? (barlen / pagelen) : 0;
        wnd->wtop = (wnd->VScrollBox-1) * lines_tick;
        if (wnd->wtop + DfClientHeight(wnd) > wnd->wlines)
            wnd->wtop = pagelen;
    }
    if (wnd->wtop < 0)
        wnd->wtop = 0;
}

/* ------ compute the horizontal scroll box position from
                   the text pointers --------- */
static int ComputeHScrollBox(DFWINDOW wnd)
{
    int pagewidth = wnd->textwidth - DfClientWidth(wnd);
    int barlen = DfClientWidth(wnd)-2;
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
        if (hscrollbox > DfClientWidth(wnd)-2 ||
                wnd->wleft + DfClientWidth(wnd) >= wnd->textwidth)
            hscrollbox = DfClientWidth(wnd)-2;
    }
    return hscrollbox;
}

/* ---- compute left column from scroll box position ---- */
static void ComputeWindowLeft(DFWINDOW wnd)
{
    int pagewidth = wnd->textwidth - DfClientWidth(wnd);

    if (wnd->HScrollBox == 0)
        wnd->wleft = 0;
    else if (wnd->HScrollBox == DfClientWidth(wnd)-2)
        wnd->wleft = pagewidth;
    else    {
        int barlen = DfClientWidth(wnd)-2;
        int chars_tick;

        if (pagewidth > barlen)
            chars_tick = pagewidth / barlen;
        else
            chars_tick = barlen / pagewidth;
        wnd->wleft = (wnd->HScrollBox-1) * chars_tick;
        if (wnd->wleft + DfClientWidth(wnd) > wnd->textwidth)
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
    cp = cp1 = DfTextLine(wnd, selection);
    while (*cp && *cp != '\n')    {
        len++;
        cp++;
    }
    line = DfMalloc(len+7);
    memmove(line, cp1, len);
    line[len] = '\0';
    return line;
}

/* ------- write a line of text to a textbox window ------- */
void DfWriteTextLine(DFWINDOW wnd, DFRECT *rcc, int y, BOOL reverse)
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
    if (y < wnd->wtop || y >= wnd->wtop+DfClientHeight(wnd))
        return;

    /* ---- build the retangle DfWithin which can write ---- */
    if (rcc == NULL)    {
        rc = DfRelativeWindowRect(wnd, DfWindowRect(wnd));
        if (DfTestAttribute(wnd, DF_HASBORDER) &&
                DfRectRight(rc) >= DfWindowWidth(wnd)-1)
            DfRectRight(rc) = DfWindowWidth(wnd)-2;
    }
    else
        rc = *rcc;

    /* ----- make sure rectangle is DfWithin window ------ */
    if (DfRectLeft(rc) >= DfWindowWidth(wnd)-1)
        return;
    if (DfRectRight(rc) == 0)
        return;
    rc = DfAdjustRectangle(wnd, rc);
    if (y-wnd->wtop<DfRectTop(rc) || y-wnd->wtop>DfRectBottom(rc))
        return;

    /* --- get the text and length of the text line --- */
    lp = svlp = GetTextLine(wnd, y);
    if (svlp == NULL)
        return;
    lnlen = DfLineLength(lp);

    /* -------- insert block DfColor change controls ------- */
    if (DfTextBlockMarked(wnd))    {
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
            /* ----- insert the reset DfColor token ----- */
            memmove(lp+blkend+1,lp+blkend,strlen(lp+blkend)+1);
            lp[blkend] = DF_RESETCOLOR;
            /* ----- insert the change DfColor token ----- */
            memmove(lp+blkbeg+3,lp+blkbeg,strlen(lp+blkbeg)+1);
            lp[blkbeg] = DF_CHANGECOLOR;
            /* ----- insert the DfColor tokens ----- */
            DfSetReverseColor(wnd);
            lp[blkbeg+1] = DfForeground | 0x80;
            lp[blkbeg+2] = DfBackground | 0x80;
            lnlen += 4;
        }
    }
    /* - make sure left margin doesn't overlap DfColor change - */
    for (i = 0; i < wnd->wleft+3; i++)    {
        if (*(lp+i) == '\0')
            break;
        if (*(unsigned char *)(lp + i) == DF_RESETCOLOR)
            break;
    }
    if (*(lp+i) && i < wnd->wleft+3)    {
        if (wnd->wleft+4 > lnlen)
            trunc = TRUE;
        else 
            lp += 4;
    }
    else     {
        /* --- it does, shift the DfColor change over --- */
        for (i = 0; i < wnd->wleft; i++)    {
            if (*(lp+i) == '\0')
                break;
            if (*(unsigned char *)(lp + i) == DF_CHANGECOLOR)    {
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
        if (lnlen > DfRectLeft(rc))    {
            /* ---- the line exceeds the rectangle ---- */
            int ct = DfRectLeft(rc);
            char *initlp = lp;
            /* --- point to end of clipped line --- */
            while (ct)    {
                if (*(unsigned char *)lp == DF_CHANGECOLOR)
                    lp += 3;
                else if (*(unsigned char *)lp == DF_RESETCOLOR)
                    lp++;
                else
                    lp++, --ct;
            }
            if (DfRectLeft(rc))    {
                char *lpp = lp;
                while (*lpp)    {
                    if (*(unsigned char*)lpp==DF_CHANGECOLOR)
                        break;
                    if (*(unsigned char*)lpp==DF_RESETCOLOR) {
                        lpp = lp;
                        while (lpp >= initlp)    {
                            if (*(unsigned char *)lpp ==
                                            DF_CHANGECOLOR) {
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
            lnlen = DfLineLength(lp);
            len = min(lnlen, DfRectWidth(rc));
            dif = strlen(lp) - lnlen;
            len += dif;
            if (len > 0)
                strncpy(line, lp, len);
        }
    }
    /* -------- pad the line --------- */
    while (len < DfRectWidth(rc)+dif)
        line[len++] = ' ';
    line[len] = '\0';
    dif = 0;
    /* ------ establish the line's main DfColor ----- */
    if (reverse)    {
        char *cp = line;
        DfSetReverseColor(wnd);
        while ((cp = strchr(cp, DF_CHANGECOLOR)) != NULL)    {
            cp += 2;
            *cp++ = DfBackground | 0x80;
        }
        if (*(unsigned char *)line == DF_CHANGECOLOR)
            dif = 3;
    }
    else
        DfSetStandardColor(wnd);
    /* ------- display the line -------- */
    DfWriteLine(wnd, line+dif,
                DfRectLeft(rc)+DfBorderAdj(wnd),
                    y-wnd->wtop+DfTopBorderAdj(wnd), FALSE);
    free(svlp);
}

void DfMarkTextBlock(DFWINDOW wnd, int BegLine, int BegCol,
                               int EndLine, int EndCol)
{
    wnd->BlkBegLine = BegLine;
    wnd->BlkEndLine = EndLine;
    wnd->BlkBegCol = BegCol;
    wnd->BlkEndCol = EndCol;
}

/* ----- clear and initialize text line pointer array ----- */
void DfClearTextPointers(DFWINDOW wnd)
{
    wnd->TextPointers = DfRealloc(wnd->TextPointers, sizeof(int));
    *(wnd->TextPointers) = 0;
}

#define INITLINES 100

/* ---- build array of pointers to text lines ---- */
void DfBuildTextPointers(DFWINDOW wnd)
{
    char *cp = wnd->text, *cp1;
    int incrs = INITLINES;
    unsigned int off;
    wnd->textwidth = wnd->wlines = 0;
    while (*cp)    {
        if (incrs == INITLINES)    {
            incrs = 0;
            wnd->TextPointers = DfRealloc(wnd->TextPointers,
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
    DfForeground = DfFrameForeground(wnd);
    DfBackground = DfFrameBackground(wnd);
    DfWPutch(wnd, DF_SCROLLBARCHAR, DfWindowWidth(wnd)-1,
            wnd->VScrollBox+1);
    DfWPutch(wnd, DF_SCROLLBOXCHAR, DfWindowWidth(wnd)-1,
            vscrollbox+1);
    wnd->VScrollBox = vscrollbox;
}

int DfTextLineNumber(DFWINDOW wnd, char *lp)
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
