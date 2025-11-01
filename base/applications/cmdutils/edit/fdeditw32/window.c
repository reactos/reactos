/* ---------- window.c ------------- */

#include "dflat.h"

WINDOW inFocus = NULL;

int foreground, background;   /* current video colors */

static void TopLine(WINDOW, int, RECT);

/* --------- create a window ------------ */
WINDOW CreateWindow(
    CLASS Class,              /* class of this window       */
    const char *ttl,          /* title or NULL              */
    int left, int top,        /* upper left coordinates     */
    int height, int width,    /* dimensions                 */
    void *extension,          /* pointer to additional data */
    WINDOW parent,            /* parent of this window      */
    int (*wndproc)(struct window *,enum messages,PARAM,PARAM),
    int attrib)               /* window attribute           */
{
    WINDOW wnd = DFcalloc(1, sizeof(struct window));
    get_videomode();
    if (wnd != NULL)    {
        int base;
        /* ----- height, width = -1: fill the screen ------- */
        if (height == -1)
            height = SCREENHEIGHT;
        if (width == -1)
            width = SCREENWIDTH;
        /* ----- coordinates -1, -1 = center the window ---- */
        if (left == -1)
            wnd->rc.lf = (SCREENWIDTH-width)/2;
        else
            wnd->rc.lf = left;
        if (top == -1)
            wnd->rc.tp = (SCREENHEIGHT-height)/2;
        else
            wnd->rc.tp = top;
        wnd->attrib = attrib;
        if (ttl != NULL)
			if (*ttl != '\0')
	            AddAttribute(wnd, HASTITLEBAR);
        if (wndproc == NULL)
            wnd->wndproc = classdefs[Class].wndproc;
        else
            wnd->wndproc = wndproc;
        /* ---- derive attributes of base classes ---- */
        base = Class;
        while (base != -1)    {
            AddAttribute(wnd, classdefs[base].attrib);
            base = classdefs[base].base;
        }
        if (parent)	{
			if (!TestAttribute(wnd, NOCLIP))    {
            	/* -- keep upper left within borders of parent - */
            	wnd->rc.lf = max(wnd->rc.lf,GetClientLeft(parent));
            	wnd->rc.tp = max(wnd->rc.tp,GetClientTop(parent));
        	}
		}
		else
			parent = ApplicationWindow;
        wnd->Class = Class;
        wnd->extension = extension;
        wnd->rc.rt = GetLeft(wnd)+width-1;
        wnd->rc.bt = GetTop(wnd)+height-1;
        wnd->ht = height;
        wnd->wd = width;
        if (ttl != NULL)
            InsertTitle(wnd, ttl);
        wnd->parent = parent;
        wnd->oldcondition = wnd->condition = ISRESTORED;
        wnd->RestoredRC = wnd->rc;
		InitWindowColors(wnd);
        SendMessage(wnd, CREATE_WINDOW, 0, 0);
        if (isVisible(wnd))
            SendMessage(wnd, SHOW_WINDOW, 0, 0);
    }
    return wnd;
}

/* -------- add a title to a window --------- */
void AddTitle(WINDOW wnd, const char *ttl)
{
    InsertTitle(wnd, ttl);
    SendMessage(wnd, BORDER, 0, 0);
}

/* ----- insert a title into a window ---------- */
void InsertTitle(WINDOW wnd, const char *ttl)
{
    wnd->title=DFrealloc(wnd->title,strlen(ttl)+1);
    strcpy(wnd->title, ttl);
}

static unsigned char line[300];

/* ------ write a line to video window client area ------ */
void writeline(WINDOW wnd, char *str, int x, int y, BOOL pad)
{
    char *cp;
    int len;
    int dif;
	char wline[200];

	memset(wline, 0, 200);
    len = LineLength(str);
    dif = strlen(str) - len;
    strncpy(wline, str, ClientWidth(wnd) + dif);
    if (pad)    {
        cp = wline+strlen(wline);
        while (len++ < ClientWidth(wnd)-x)
            *cp++ = ' ';
    }
    wputs(wnd, wline, x, y);
}

RECT AdjustRectangle(WINDOW wnd, RECT rc)
{
    /* -------- adjust the rectangle ------- */
    if (TestAttribute(wnd, HASBORDER))    {
        if (RectLeft(rc) == 0)
            --rc.rt;
        else if (RectLeft(rc) < RectRight(rc) &&
                RectLeft(rc) < WindowWidth(wnd)+1)
            --rc.lf;
    }
    if (TestAttribute(wnd, HASBORDER | HASTITLEBAR))    {
        if (RectTop(rc) == 0)
            --rc.bt;
        else if (RectTop(rc) < RectBottom(rc) &&
                RectTop(rc) < WindowHeight(wnd)+1)
            --rc.tp;
    }
    RectRight(rc) = max(RectLeft(rc),
                        min(RectRight(rc),WindowWidth(wnd)));
    RectBottom(rc) = max(RectTop(rc),
                        min(RectBottom(rc),WindowHeight(wnd)));
    return rc;
}

/* -------- display a window's title --------- */
void DisplayTitle(WINDOW wnd, RECT *rcc)
{
	if (GetTitle(wnd) != NULL)	{
    	int tlen = min(strlen(GetTitle(wnd)), WindowWidth(wnd)-2);
    	int tend = WindowWidth(wnd)-3-BorderAdj(wnd);
    	RECT rc;

    	if (rcc == NULL)
        	rc = RelativeWindowRect(wnd, WindowRect(wnd));
    	else
        	rc = *rcc;
    	rc = AdjustRectangle(wnd, rc);

    	if (SendMessage(wnd, TITLE, (PARAM) rcc, 0))    {
        	if (wnd == inFocus)    {
            	foreground = cfg.clr[TITLEBAR] [HILITE_COLOR] [FG];
            	background = cfg.clr[TITLEBAR] [HILITE_COLOR] [BG];
        	}
        	else    {
            	foreground = cfg.clr[TITLEBAR] [STD_COLOR] [FG];
            	background = cfg.clr[TITLEBAR] [STD_COLOR] [BG];
        	}
        	memset(line,' ',WindowWidth(wnd));
#ifdef INCLUDE_MINIMIZE
        	if (wnd->condition != ISMINIMIZED)
#endif
            	strncpy(line + ((WindowWidth(wnd)-2 - tlen) / 2),
                	wnd->title, tlen);
        	if (TestAttribute(wnd, CONTROLBOX))
            	line[2-BorderAdj(wnd)] = CONTROLBOXCHAR;
        	if (TestAttribute(wnd, MINMAXBOX))    {
            	switch (wnd->condition)    {
                	case ISRESTORED:
#ifdef INCLUDE_MAXIMIZE
                    	line[tend+1] = MAXPOINTER;
#endif
#ifdef INCLUDE_MINIMIZE
                    	line[tend]   = MINPOINTER;
#endif
                    	break;
#ifdef INCLUDE_MINIMIZE
                	case ISMINIMIZED:
                    	line[tend+1] = MAXPOINTER;
                    	break;
#endif
#ifdef INCLUDE_MAXIMIZE
                	case ISMAXIMIZED:
#ifdef INCLUDE_MINIMIZE
                    	line[tend]   = MINPOINTER;
#endif
#ifdef INCLUDE_RESTORE
                    	line[tend+1] = RESTOREPOINTER;
#endif
                    	break;
#endif
                	default:
                    	break;
            	}
        	}
        	line[RectRight(rc)+1] = line[tend+3] = '\0';
			if (wnd != inFocus)
				ClipString++;
        	writeline(wnd, line+RectLeft(rc),
                       	RectLeft(rc)+BorderAdj(wnd),
                       	0,
                       	FALSE);
			ClipString = 0;
    	}
	}
}

#ifdef INCLUDE_MINIMIZE
#define MinTest() (wnd->condition == ISMINIMIZED) ||
#else
#define MinTest() /**/
#endif

#ifdef INCLUDE_MAXIMIZE
#define MaxTest() (wnd->condition == ISMAXIMIZED) ||
#else
#define MaxTest() /**/
#endif

#define NoShadow(wnd)                    \
     (TestAttribute(wnd, SHADOW) == 0 || \
      MinTest()                          \
      MaxTest()                          \
	  cfg.mono)

/* --- display right border shadow character of a window --- */
static void near shadow_char(WINDOW wnd, int y)
{
    int fg = foreground;
    int bg = background;
    int x = WindowWidth(wnd);
    int c = videochar(GetLeft(wnd)+x, GetTop(wnd)+y);

	if (NoShadow(wnd))
        return;
    foreground = DARKGRAY;
    background = BLACK;
    wputch(wnd, c, x, y);
    foreground = fg;
    background = bg;
}

/* --- display the bottom border shadow line for a window -- */
static void near shadowline(WINDOW wnd, RECT rc)
{
    int i;
    int y = GetBottom(wnd)+1;
    int fg = foreground;
    int bg = background;

	if (NoShadow(wnd))
        return;
    for (i = 0; i < WindowWidth(wnd)+1; i++)
        line[i] = videochar(GetLeft(wnd)+i, y);
    line[i] = '\0';
    foreground = DARKGRAY;
    background = BLACK;
    line[RectRight(rc)+1] = '\0';
    if (RectLeft(rc) == 0)
        rc.lf++;
	ClipString++;
    wputs(wnd, line+RectLeft(rc), RectLeft(rc),
        WindowHeight(wnd));
	--ClipString;
    foreground = fg;
    background = bg;
}

static RECT ParamRect(WINDOW wnd, RECT *rcc)
{
	RECT rc;
    if (rcc == NULL)    {
        rc = RelativeWindowRect(wnd, WindowRect(wnd));
	    if (TestAttribute(wnd, SHADOW))    {
    	    rc.rt++;
        	rc.bt++;
	    }
    }
    else
        rc = *rcc;
	return rc;
}

void PaintShadow(WINDOW wnd)
{
	int y;
	RECT rc = ParamRect(wnd, NULL);
	for (y = 1; y < WindowHeight(wnd); y++)
		shadow_char(wnd, y);
    shadowline(wnd, rc);
}

static unsigned int SeCorner(WINDOW wnd, unsigned int stdse)
{
	if (TestAttribute(wnd, SIZEABLE) && wnd->condition == ISRESTORED)
		return SIZETOKEN;
	return stdse;
}

/* ------- display a window's border ----- */
void RepaintBorder(WINDOW wnd, RECT *rcc)
{
    int y;
    unsigned int lin, side, ne, nw, se, sw;
    RECT rc, clrc;

    if (!TestAttribute(wnd, HASBORDER))
        return;
	rc = ParamRect(wnd, rcc);
    clrc = AdjustRectangle(wnd, rc);

    if (wnd == inFocus)    {
        lin  = FOCUS_LINE;
        side = FOCUS_SIDE;
        ne   = FOCUS_NE;
        nw   = FOCUS_NW;
        se   = SeCorner(wnd, FOCUS_SE);
        sw   = FOCUS_SW;
    }
    else    {
        lin  = LINE;
        side = SIDE;
        ne   = NE;
        nw   = NW;
        se   = SeCorner(wnd, SE);
        sw   = SW;
    }
    line[WindowWidth(wnd)] = '\0';
    /* ---------- window title ------------ */
    if (TestAttribute(wnd, HASTITLEBAR))
        if (RectTop(rc) == 0)
            if (RectLeft(rc) < WindowWidth(wnd)-BorderAdj(wnd))
                DisplayTitle(wnd, &rc);
    foreground = FrameForeground(wnd);
    background = FrameBackground(wnd);
    /* -------- top frame corners --------- */
    if (RectTop(rc) == 0)    {
        if (RectLeft(rc) == 0)
            wputch(wnd, nw, 0, 0);
        if (RectLeft(rc) < WindowWidth(wnd))    {
            if (RectRight(rc) >= WindowWidth(wnd)-1)
                wputch(wnd, ne, WindowWidth(wnd)-1, 0);
            TopLine(wnd, lin, clrc);
        }
    }

    /* ----------- window body ------------ */
    for (y = RectTop(rc); y <= RectBottom(rc); y++)    {
        int ch;
        if (y == 0 || y >= WindowHeight(wnd)-1)
            continue;
        if (RectLeft(rc) == 0)
            wputch(wnd, side, 0, y);
        if (RectLeft(rc) < WindowWidth(wnd) &&
                RectRight(rc) >= WindowWidth(wnd)-1)    {
            if (TestAttribute(wnd, VSCROLLBAR))
                ch = (    y == 1 ? UPSCROLLBOX      :
                          y == WindowHeight(wnd)-2  ?
                                DOWNSCROLLBOX       :
                          y-1 == wnd->VScrollBox    ?
                                SCROLLBOXCHAR       :
                          SCROLLBARCHAR );
            else
                ch = side;
            wputch(wnd, ch, WindowWidth(wnd)-1, y);
        }
        if (RectRight(rc) == WindowWidth(wnd))
            shadow_char(wnd, y);
    }

    if (RectTop(rc) <= WindowHeight(wnd)-1 &&
            RectBottom(rc) >= WindowHeight(wnd)-1)    {
        /* -------- bottom frame corners ---------- */
        if (RectLeft(rc) == 0)
            wputch(wnd, sw, 0, WindowHeight(wnd)-1);
        if (RectLeft(rc) < WindowWidth(wnd) &&
                RectRight(rc) >= WindowWidth(wnd)-1)
            wputch(wnd, se, WindowWidth(wnd)-1,
                WindowHeight(wnd)-1);


		if (wnd->StatusBar == NULL)	{
        	/* ----------- bottom line ------------- */
        	memset(line,lin,WindowWidth(wnd)-1);
        	if (TestAttribute(wnd, HSCROLLBAR))    {
            	line[0] = LEFTSCROLLBOX;
            	line[WindowWidth(wnd)-3] = RIGHTSCROLLBOX;
            	memset(line+1, SCROLLBARCHAR, WindowWidth(wnd)-4);
            	line[wnd->HScrollBox] = SCROLLBOXCHAR;
        	}
        	line[WindowWidth(wnd)-2] = line[RectRight(rc)] = '\0';
        	if (RectLeft(rc) != RectRight(rc) ||
	        	(RectLeft(rc) && RectLeft(rc) < WindowWidth(wnd)-1))	{
				if (wnd != inFocus)
					ClipString++;
            	writeline(wnd,
                			line+(RectLeft(clrc)),
                			RectLeft(clrc)+1,
                			WindowHeight(wnd)-1,
                			FALSE);
				ClipString = 0;
			}
		}
        if (RectRight(rc) == WindowWidth(wnd))
            shadow_char(wnd, WindowHeight(wnd)-1);
    }
    if (RectBottom(rc) == WindowHeight(wnd))
        /* ---------- bottom shadow ------------- */
        shadowline(wnd, rc);
}

static void TopLine(WINDOW wnd, int lin, RECT rc)
{
    if (TestAttribute(wnd, HASMENUBAR))
        return;
    if (TestAttribute(wnd, HASTITLEBAR) && GetTitle(wnd))
        return;
	if (RectLeft(rc) == 0)	{
		RectLeft(rc) += BorderAdj(wnd);
		RectRight(rc) += BorderAdj(wnd);
	}
	if (RectRight(rc) < WindowWidth(wnd)-1)
		RectRight(rc)++;

    if (RectLeft(rc) < RectRight(rc))    {
        /* ----------- top line ------------- */
        memset(line,lin,WindowWidth(wnd)-1);
		if (TestAttribute(wnd, CONTROLBOX))	{
			strncpy(line+1, "   ", 3);
			*(line+2) = CONTROLBOXCHAR;
		}
        line[RectRight(rc)] = '\0';
        writeline(wnd, line+RectLeft(rc),
            RectLeft(rc), 0, FALSE);
    }
}

/* ------ clear the data space of a window -------- */
void ClearWindow(WINDOW wnd, RECT *rcc, int clrchar)
{
    if (isVisible(wnd))    {
        int y;
        RECT rc = rcc ? *rcc : RelativeWindowRect(wnd, WindowRect(wnd));

		int top = TopBorderAdj(wnd);
		int bot = WindowHeight(wnd)-1-BottomBorderAdj(wnd);

        if (RectLeft(rc) == 0)
            RectLeft(rc) = BorderAdj(wnd);
        if (RectRight(rc) > WindowWidth(wnd)-1)
            RectRight(rc) = WindowWidth(wnd)-1;
        SetStandardColor(wnd);
        memset(line, clrchar, sizeof line);
        line[RectRight(rc)+1] = '\0';
        for (y = RectTop(rc); y <= RectBottom(rc); y++)    {
            if (y < top || y > bot)
                continue;
            writeline(wnd,
                line+(RectLeft(rc)),
                RectLeft(rc),
                y,
                FALSE);
        }
    }
}

/* ------ compute the logical line length of a window ------ */
int LineLength(char *ln)
{
    int len = strlen(ln);
    char *cp = ln;
    while ((cp = strchr(cp, CHANGECOLOR)) != NULL)    {
        cp++;
        len -= 3;
    }
    cp = ln;
    while ((cp = strchr(cp, RESETCOLOR)) != NULL)    {
        cp++;
        --len;
    }
    return len;
}

void InitWindowColors(WINDOW wnd)
{
	int fbg,col;
	int cls = GetClass(wnd);
	/* window classes without assigned colors inherit parent's colors */
	if (cfg.clr[cls][0][0] == 0xff && GetParent(wnd) != NULL)
		cls = GetClass(GetParent(wnd));
	/* ---------- set the colors ---------- */
	for (fbg = 0; fbg < 2; fbg++)
		for (col = 0; col < 4; col++)
			wnd->WindowColors[col][fbg] = cfg.clr[cls][col][fbg];
}

void PutWindowChar(WINDOW wnd, int c, int x, int y)
{
	if (x < ClientWidth(wnd) && y < ClientHeight(wnd))
		wputch(wnd, c, x+BorderAdj(wnd), y+TopBorderAdj(wnd));
}

void PutWindowLine(WINDOW wnd, void *s, int x, int y)
{
	int saved = FALSE, sv;
	if (x < ClientWidth(wnd) && y < ClientHeight(wnd))	{
		char *en = (char *)s+ClientWidth(wnd)-x;
		if (strlen(s)+x > ClientWidth(wnd))	{
			sv = *en;
			*en = '\0';
			saved = TRUE;
		}
		ClipString++;
		wputs(wnd, s, x+BorderAdj(wnd), y+TopBorderAdj(wnd));
		--ClipString;
		if (saved)
			*en = sv;
	}
}

