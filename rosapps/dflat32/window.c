/* ---------- window.c ------------- */

#include "dflat.h"

DFWINDOW DfInFocus = NULL;

int DfForeground, DfBackground;   /* current video colors */

static void TopLine(DFWINDOW, int, DFRECT);

/* --------- create a window ------------ */
DFWINDOW DfDfCreateWindow(
    DFCLASS class,              /* class of this window       */
    char *ttl,                /* title or NULL              */
    int left, int top,        /* upper left coordinates     */
    int height, int width,    /* dimensions                 */
    void *extension,          /* pointer to additional data */
    DFWINDOW parent,            /* parent of this window      */
    int (*wndproc)(struct DfWindow *,enum DfMessages,DF_PARAM,DF_PARAM),
    int attrib)               /* window attribute           */
{
    DFWINDOW wnd = DfCalloc(1, sizeof(struct DfWindow));
    if (wnd != NULL)    {
        int base;
        /* ----- height, width = -1: fill the screen ------- */
        if (height == -1)
            height = DfScreenHeight;
        if (width == -1)
            width = DfScreenWidth;
        /* ----- coordinates -1, -1 = center the window ---- */
        if (left == -1)
            wnd->rc.lf = (DfScreenWidth-width)/2;
        else
            wnd->rc.lf = left;
        if (top == -1)
            wnd->rc.tp = (DfScreenHeight-height)/2;
        else
            wnd->rc.tp = top;
        wnd->attrib = attrib;
        if (ttl != NULL)
            DfAddAttribute(wnd, DF_HASTITLEBAR);
        if (wndproc == NULL)
            wnd->wndproc = DfClassDefs[class].wndproc;
        else
            wnd->wndproc = wndproc;
        /* ---- derive attributes of base classes ---- */
        base = class;
        while (base != -1)    {
            DfAddAttribute(wnd, DfClassDefs[base].attrib);
            base = DfClassDefs[base].base;
        }
        if (parent)
		{
			if (!DfTestAttribute(wnd, DF_NOCLIP))
			{
            	/* -- keep upper left DfWithin borders of parent - */
            	wnd->rc.lf = max(wnd->rc.lf,DfGetClientLeft(parent));
            	wnd->rc.tp = max(wnd->rc.tp,DfGetClientTop(parent));
        	}
		}
		else
			parent = DfApplicationWindow;

        wnd->class = class;
        wnd->extension = extension;
        wnd->rc.rt = DfGetLeft(wnd)+width-1;
        wnd->rc.bt = DfGetTop(wnd)+height-1;
        wnd->ht = height;
        wnd->wd = width;
        if (ttl != NULL)
            DfInsertTitle(wnd, ttl);
        wnd->parent = parent;
        wnd->oldcondition = wnd->condition = DF_SRESTORED;
        wnd->RestoredRC = wnd->rc;
		DfInitWindowColors(wnd);
        DfSendMessage(wnd, DFM_CREATE_WINDOW, 0, 0);
        if (DfIsVisible(wnd))
            DfSendMessage(wnd, DFM_SHOW_WINDOW, 0, 0);
    }
    return wnd;
}

/* -------- add a title to a window --------- */
void DfAddTitle(DFWINDOW wnd, char *ttl)
{
	DfInsertTitle(wnd, ttl);
	DfSendMessage(wnd, DFM_BORDER, 0, 0);
}

/* ----- insert a title into a window ---------- */
void DfInsertTitle(DFWINDOW wnd, char *ttl)
{
	wnd->title=DfRealloc(wnd->title,strlen(ttl)+1);
	strcpy(wnd->title, ttl);
}

static unsigned char line[300];

/* ------ write a line to video window client area ------ */
void DfWriteLine(DFWINDOW wnd, char *str, int x, int y, BOOL pad)
{
    char *cp;
    int len;
    int dif;
	char wline[200];

	memset(wline, 0, 200);
    len = DfLineLength(str);
    dif = strlen(str) - len;
    strncpy(wline, str, DfClientWidth(wnd) + dif);
    if (pad)    {
        cp = wline+strlen(wline);
        while (len++ < DfClientWidth(wnd)-x)
            *cp++ = ' ';
    }
    DfWPuts(wnd, wline, x, y);
}

DFRECT DfAdjustRectangle(DFWINDOW wnd, DFRECT rc)
{
    /* -------- adjust the rectangle ------- */
    if (DfTestAttribute(wnd, DF_HASBORDER))    {
        if (DfRectLeft(rc) == 0)
            --rc.rt;
        else if (DfRectLeft(rc) < DfRectRight(rc) &&
                DfRectLeft(rc) < DfWindowWidth(wnd)+1)
            --rc.lf;
    }
    if (DfTestAttribute(wnd, DF_HASBORDER | DF_HASTITLEBAR))    {
        if (DfRectTop(rc) == 0)
            --rc.bt;
        else if (DfRectTop(rc) < DfRectBottom(rc) &&
                DfRectTop(rc) < DfWindowHeight(wnd)+1)
            --rc.tp;
    }
    DfRectRight(rc) = max(DfRectLeft(rc),
                        min(DfRectRight(rc),DfWindowWidth(wnd)));
    DfRectBottom(rc) = max(DfRectTop(rc),
                        min(DfRectBottom(rc),DfWindowHeight(wnd)));
    return rc;
}

/* -------- display a window's title --------- */
void DfDisplayTitle(DFWINDOW wnd, DFRECT *rcc)
{
	if (DfGetTitle(wnd) != NULL)
	{
		int tlen = min((int)strlen(DfGetTitle(wnd)), (int)DfWindowWidth(wnd)-2);
		int tend = DfWindowWidth(wnd)-3-DfBorderAdj(wnd);
		DFRECT rc;

		if (rcc == NULL)
			rc = DfRelativeWindowRect(wnd, DfWindowRect(wnd));
		else
			rc = *rcc;
		rc = DfAdjustRectangle(wnd, rc);

		if (DfSendMessage(wnd, DFM_TITLE, (DF_PARAM) rcc, 0))
		{
			if (wnd == DfInFocus)
			{
				DfForeground = DfCfg.clr[DF_TITLEBAR] [DF_HILITE_COLOR] [DF_FG];
				DfBackground = DfCfg.clr[DF_TITLEBAR] [DF_HILITE_COLOR] [DF_BG];
			}
			else
			{
				DfForeground = DfCfg.clr[DF_TITLEBAR] [DF_STD_COLOR] [DF_FG];
				DfBackground = DfCfg.clr[DF_TITLEBAR] [DF_STD_COLOR] [DF_BG];
			}
			memset(line,' ',DfWindowWidth(wnd));
#ifdef INCLUDE_MINIMIZE
			if (wnd->condition != DF_ISMINIMIZED)
#endif
			strncpy (line + ((DfWindowWidth(wnd)-2 - tlen) / 2),
			         wnd->title, tlen);
			if (DfTestAttribute(wnd, DF_CONTROLBOX))
				line[2-DfBorderAdj(wnd)] = DF_CONTROLBOXCHAR;
			if (DfTestAttribute(wnd, DF_MINMAXBOX))
			{
				switch (wnd->condition)
				{
					case DF_SRESTORED:
#ifdef INCLUDE_MAXIMIZE
						line[tend+1] = DF_MAXPOINTER;
#endif
#ifdef INCLUDE_MINIMIZE
						line[tend]   = DF_MINPOINTER;
#endif
						break;
#ifdef INCLUDE_MINIMIZE
					case DF_ISMINIMIZED:
						line[tend+1] = DF_MAXPOINTER;
						break;
#endif
#ifdef INCLUDE_MAXIMIZE
					case DF_ISMAXIMIZED:
#ifdef INCLUDE_MINIMIZE
						line[tend]   = DF_MINPOINTER;
#endif
#ifdef INCLUDE_RESTORE
						line[tend+1] = DF_RESTOREPOINTER;
#endif
						break;
#endif
					default:
						break;
				}
			}
			line[DfRectRight(rc)+1] = line[tend+3] = '\0';
			if (wnd != DfInFocus)
				DfClipString++;
			DfWriteLine(wnd, line+DfRectLeft(rc),
                       	DfRectLeft(rc)+DfBorderAdj(wnd),
                       	0,
                       	FALSE);
			DfClipString = 0;
		}
	}
}

/* --- display right border shadow character of a window --- */
static void shadow_char(DFWINDOW wnd, int y)
{
    int fg = DfForeground;
    int bg = DfBackground;
    int x = DfWindowWidth(wnd);
    char c = DfVideoChar(DfGetLeft(wnd)+x, DfGetTop(wnd)+y);

    if (DfTestAttribute(wnd, DF_SHADOW) == 0)
        return;
    DfForeground = DARKGRAY;
    DfBackground = BLACK;
    DfWPutch(wnd, c, x, y);
    DfForeground = fg;
    DfBackground = bg;
}

/* --- display the bottom border shadow line for a window -- */
static void shadowline(DFWINDOW wnd, DFRECT rc)
{
    int i;
    int y = DfGetBottom(wnd)+1;
    int fg = DfForeground;
    int bg = DfBackground;

    if ((DfTestAttribute(wnd, DF_SHADOW)) == 0)
        return;
    for (i = 0; i < DfWindowWidth(wnd)+1; i++)
        line[i] = DfVideoChar(DfGetLeft(wnd)+i, y);
    line[i] = '\0';
    DfForeground = DARKGRAY;
    DfBackground = BLACK;
    line[DfRectRight(rc)+1] = '\0';
    if (DfRectLeft(rc) == 0)
        rc.lf++;
	DfClipString++;
    DfWPuts(wnd, line+DfRectLeft(rc), DfRectLeft(rc),
        DfWindowHeight(wnd));
	--DfClipString;
    DfForeground = fg;
    DfBackground = bg;
}

static DFRECT ParamRect(DFWINDOW wnd, DFRECT *rcc)
{
	DFRECT rc;
    if (rcc == NULL)    {
        rc = DfRelativeWindowRect(wnd, DfWindowRect(wnd));
	    if (DfTestAttribute(wnd, DF_SHADOW))    {
    	    rc.rt++;
        	rc.bt++;
	    }
    }
    else
        rc = *rcc;
	return rc;
}

void DfPaintShadow(DFWINDOW wnd)
{
	int y;
	DFRECT rc = ParamRect(wnd, NULL);
	for (y = 1; y < DfWindowHeight(wnd); y++)
		shadow_char(wnd, y);
	shadowline(wnd, rc);
}

/* ------- display a window's border ----- */
void DfRepaintBorder(DFWINDOW wnd, DFRECT *rcc)
{
    int y;
    char lin, side, ne, nw, se, sw;
    DFRECT rc, clrc;

    if (!DfTestAttribute(wnd, DF_HASBORDER))
        return;
	rc = ParamRect(wnd, rcc);
    clrc = DfAdjustRectangle(wnd, rc);

    if (wnd == DfInFocus)    {
        lin  = DF_FOCUS_LINE;
        side = DF_FOCUS_SIDE;
        ne   = DF_FOCUS_NE;
        nw   = DF_FOCUS_NW;
        se   = DF_FOCUS_SE;
        sw   = DF_FOCUS_SW;
    }
    else    {
        lin  = DF_LINE;
        side = DF_SIDE;
        ne   = DF_NE;
        nw   = DF_NW;
        se   = DF_SE;
        sw   = DF_SW;
    }
    line[DfWindowWidth(wnd)] = '\0';
    /* ---------- window title ------------ */
    if (DfTestAttribute(wnd, DF_HASTITLEBAR))
        if (DfRectTop(rc) == 0)
            if (DfRectLeft(rc) < DfWindowWidth(wnd)-DfBorderAdj(wnd))
                DfDisplayTitle(wnd, &rc);
    DfForeground = DfFrameForeground(wnd);
    DfBackground = DfFrameBackground(wnd);
    /* -------- top frame corners --------- */
    if (DfRectTop(rc) == 0)    {
        if (DfRectLeft(rc) == 0)
            DfWPutch(wnd, nw, 0, 0);
        if (DfRectLeft(rc) < DfWindowWidth(wnd))    {
            if (DfRectRight(rc) >= DfWindowWidth(wnd)-1)
                DfWPutch(wnd, ne, DfWindowWidth(wnd)-1, 0);
            TopLine(wnd, lin, clrc);
        }
    }

    /* ----------- window body ------------ */
    for (y = DfRectTop(rc); y <= DfRectBottom(rc); y++)    {
        char ch;
        if (y == 0 || y >= DfWindowHeight(wnd)-1)
            continue;
        if (DfRectLeft(rc) == 0)
            DfWPutch(wnd, side, 0, y);
        if (DfRectLeft(rc) < DfWindowWidth(wnd) &&
                DfRectRight(rc) >= DfWindowWidth(wnd)-1)    {
            if (DfTestAttribute(wnd, DF_VSCROLLBAR))
                ch = (    y == 1 ? DF_UPSCROLLBOX      :
                          y == DfWindowHeight(wnd)-2  ?
                                DF_DOWNSCROLLBOX       :
                          y-1 == wnd->VScrollBox    ?
                                DF_SCROLLBOXCHAR       :
                          DF_SCROLLBARCHAR );
            else
                ch = side;
            DfWPutch(wnd, ch, DfWindowWidth(wnd)-1, y);
        }
        if (DfRectRight(rc) == DfWindowWidth(wnd))
            shadow_char(wnd, y);
    }

    if (DfRectTop(rc) <= DfWindowHeight(wnd)-1 &&
            DfRectBottom(rc) >= DfWindowHeight(wnd)-1)    {
        /* -------- bottom frame corners ---------- */
        if (DfRectLeft(rc) == 0)
            DfWPutch(wnd, sw, 0, DfWindowHeight(wnd)-1);
        if (DfRectLeft(rc) < DfWindowWidth(wnd) &&
                DfRectRight(rc) >= DfWindowWidth(wnd)-1)
            DfWPutch(wnd, se, DfWindowWidth(wnd)-1,
                DfWindowHeight(wnd)-1);


		if (wnd->StatusBar == NULL)	{
        	/* ----------- bottom line ------------- */
        	memset(line,lin,DfWindowWidth(wnd)-1);
        	if (DfTestAttribute(wnd, DF_HSCROLLBAR))    {
            	line[0] = DF_LEFTSCROLLBOX;
            	line[DfWindowWidth(wnd)-3] = DF_RIGHTSCROLLBOX;
            	memset(line+1, DF_SCROLLBARCHAR, DfWindowWidth(wnd)-4);
            	line[wnd->HScrollBox] = DF_SCROLLBOXCHAR;
        	}
        	line[DfWindowWidth(wnd)-2] = line[DfRectRight(rc)] = '\0';
        	if (DfRectLeft(rc) != DfRectRight(rc) ||
		    (DfRectLeft(rc) && DfRectLeft(rc) < DfWindowWidth(wnd)-1))
		{
				if (wnd != DfInFocus)
					DfClipString++;
            	DfWriteLine(wnd,
                			line+(DfRectLeft(clrc)),
                			DfRectLeft(clrc)+1,
                			DfWindowHeight(wnd)-1,
                			FALSE);
				DfClipString = 0;
			}
		}
        if (DfRectRight(rc) == DfWindowWidth(wnd))
            shadow_char(wnd, DfWindowHeight(wnd)-1);
    }
    if (DfRectBottom(rc) == DfWindowHeight(wnd))
        /* ---------- bottom shadow ------------- */
        shadowline(wnd, rc);
}

static void TopLine(DFWINDOW wnd, int lin, DFRECT rc)
{
    if (DfTestAttribute(wnd, DF_HASMENUBAR))
        return;
    if (DfTestAttribute(wnd, DF_HASTITLEBAR) && DfGetTitle(wnd))
        return;
	if (DfRectLeft(rc) == 0)	{
		DfRectLeft(rc) += DfBorderAdj(wnd);
		DfRectRight(rc) += DfBorderAdj(wnd);
	}
	if (DfRectRight(rc) < DfWindowWidth(wnd)-1)
		DfRectRight(rc)++;

    if (DfRectLeft(rc) < DfRectRight(rc))    {
        /* ----------- top line ------------- */
        memset(line,lin,DfWindowWidth(wnd)-1);
		if (DfTestAttribute(wnd, DF_CONTROLBOX))	{
			strncpy(line+1, "   ", 3);
			*(line+2) = DF_CONTROLBOXCHAR;
		}
        line[DfRectRight(rc)] = '\0';
        DfWriteLine(wnd, line+DfRectLeft(rc),
            DfRectLeft(rc), 0, FALSE);
    }
}

/* ------ clear the data space of a window -------- */
void DfClearWindow(DFWINDOW wnd, DFRECT *rcc, int clrchar)
{
    if (DfIsVisible(wnd))    {
        int y;
        DFRECT rc;

        if (rcc == NULL)
            rc = DfRelativeWindowRect(wnd, DfWindowRect(wnd));
        else
            rc = *rcc;

        if (DfRectLeft(rc) == 0)
            DfRectLeft(rc) = DfBorderAdj(wnd);
        if (DfRectRight(rc) > DfWindowWidth(wnd)-1)
            DfRectRight(rc) = DfWindowWidth(wnd)-1;
        DfSetStandardColor(wnd);
        memset(line, clrchar, sizeof line);
        line[DfRectRight(rc)+1] = '\0';
        for (y = DfRectTop(rc); y <= DfRectBottom(rc); y++)
		{
            if (y < DfTopBorderAdj(wnd) ||
                    y > DfClientHeight(wnd)+
						(DfTestAttribute(wnd, DF_HASMENUBAR) ? 1 : 0))
                continue;
            DfWriteLine(wnd,
                line+(DfRectLeft(rc)),
                DfRectLeft(rc),
                y,
                FALSE);
        }
    }
}

/* ------ compute the logical line length of a window ------ */
int DfLineLength(char *ln)
{
    int len = strlen(ln);
    char *cp = ln;
    while ((cp = strchr(cp, DF_CHANGECOLOR)) != NULL)
    {
        cp++;
        len -= 3;
    }
    cp = ln;
    while ((cp = strchr(cp, DF_RESETCOLOR)) != NULL)
    {
        cp++;
        --len;
    }
    return len;
}

void DfInitWindowColors(DFWINDOW wnd)
{
	int fbg,col;
	int cls = DfGetClass(wnd);
	/* window classes without assigned colors inherit parent's colors */
	if (DfCfg.clr[cls][0][0] == 0xff && DfGetParent(wnd) != NULL)
		cls = DfGetClass(DfGetParent(wnd));
	/* ---------- set the colors ---------- */
	for (fbg = 0; fbg < 2; fbg++)
		for (col = 0; col < 4; col++)
			wnd->WindowColors[col][fbg] = DfCfg.clr[cls][col][fbg];
}

void DfPutWindowChar(DFWINDOW wnd, char c, int x, int y)
{
	if (x < DfClientWidth(wnd) && y < DfClientHeight(wnd))
		DfWPutch(wnd, c, x+DfBorderAdj(wnd), y+DfTopBorderAdj(wnd));
}

void DfPutWindowLine(DFWINDOW wnd, void *s, int x, int y)
{
	int saved = FALSE;
	int sv = 0;

	if (x < DfClientWidth(wnd) && y < DfClientHeight(wnd))
	{
		char *en = (char *)s+DfClientWidth(wnd)-x;
		if ((int)(strlen(s)+x) > (int)DfClientWidth(wnd))
		{
			sv = *en;
			*en = '\0';
			saved = TRUE;
		}
		DfClipString++;
		DfWPuts(wnd, s, x+DfBorderAdj(wnd), y+DfTopBorderAdj(wnd));
		--DfClipString;
		if (saved)
			*en = sv;
	}
}

/* EOF */
