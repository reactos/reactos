/* --------------------- video.c -------------------- */

#include "dflat.h"

BOOL ClipString;

void movefromscreen(void *bf, int offset, int len);
void movetoscreen(void *bf, int offset, int len);

/* -- read a rectangle of video memory into a save buffer -- */
void getvideo(RECT rc, void far *bf)
{
    int x1 = RectLeft(rc);
    int x2 = RectRight(rc);
    int y1 = RectTop(rc);
    int y2 = RectBottom(rc);
    int ht;
    int bytes_row;
    int pitch_row = (x2-x1+1)*2;
    unsigned long int vadr = vad(x1, y1);

    /* perform clipping! */
    if (x1 >= SCREENWIDTH ||
        y1 >= SCREENHEIGHT)
        return;
    if (x2 >= SCREENWIDTH)
        x2 = SCREENWIDTH-1;
    if (y2 >= SCREENHEIGHT)
        y2 = SCREENHEIGHT-1;

    /* number of rows to transfer */
    ht = y2-y1+1;
    /* number of columns to transfer (byte size) */
    bytes_row = (x2-x1+1) * 2;

    while (ht--)    {
		movefromscreen(bf, vadr, bytes_row);
        vadr += SCREENWIDTH*2;
        bf = (char *)bf + pitch_row;
    }
}

/* -- write a rectangle of video memory from a save buffer -- */
void storevideo(RECT rc, void far *bf)
{
    int x1 = RectLeft(rc);
    int x2 = RectRight(rc);
    int y1 = RectTop(rc);
    int y2 = RectBottom(rc);
    int ht;
    int bytes_row;
    int pitch_row = (x2-x1+1)*2;
    unsigned long int vadr = vad(x1, y1);

    /* perform clipping! */
    if (x1 >= SCREENWIDTH ||
        y1 >= SCREENHEIGHT)
        return;
    if (x2 >= SCREENWIDTH)
        x2 = SCREENWIDTH-1;
    if (y2 >= SCREENHEIGHT)
        y2 = SCREENHEIGHT-1;

    /* number of rows to transfer */
    ht = y2-y1+1;
    /* number of columns to transfer (byte size) */
    bytes_row = (x2-x1+1) * 2;

    while (ht--)    {
		movetoscreen(bf, vadr, bytes_row);
        vadr += SCREENWIDTH*2;
        bf = (char *)bf + pitch_row;
    }
}

/* -------- read a character of video memory ------- */
unsigned int GetVideoChar(int x, int y)
{
    return w32_get_video(x,y);
}

/* -------- write a character of video memory ------- */
void PutVideoChar(int x, int y, int c)
{
    if (x < SCREENWIDTH && y < SCREENHEIGHT) {
        w32_put_video(x, y, c);
    }
}

BOOL CharInView(WINDOW wnd, int x, int y)
{
	WINDOW nwnd = NextWindow(wnd);
	WINDOW pwnd;
	RECT rc;
    int x1 = GetLeft(wnd)+x;
    int y1 = GetTop(wnd)+y;

	if (!TestAttribute(wnd, VISIBLE))
		return FALSE;
    if (!TestAttribute(wnd, NOCLIP))    {
        WINDOW wnd1 = GetParent(wnd);
        while (wnd1 != NULL)    {
            /* --- clip character to parent's borders -- */
			if (!TestAttribute(wnd1, VISIBLE))
				return FALSE;
			if (!InsideRect(x1, y1, ClientRect(wnd1)))
                return FALSE;
            wnd1 = GetParent(wnd1);
        }
    }
	while (nwnd != NULL)	{
		if (!isHidden(nwnd) && !isAncestor(wnd, nwnd))	{
			rc = WindowRect(nwnd);
    		if (TestAttribute(nwnd, SHADOW))    {
        		RectBottom(rc)++;
        		RectRight(rc)++;
    		}
			if (!TestAttribute(nwnd, NOCLIP))	{
				pwnd = nwnd;
				while (GetParent(pwnd))	{
					pwnd = GetParent(pwnd);
					rc = subRectangle(rc, ClientRect(pwnd));
				}
			}
			if (InsideRect(x1,y1,rc))
				return FALSE;
		}
		nwnd = NextWindow(nwnd);
	}
    return (x1 < SCREENWIDTH && y1 < SCREENHEIGHT);
}

/* -------- write a character to a window ------- */
void wputch(WINDOW wnd, int c, int x, int y)
{
	if (CharInView(wnd, x, y))	{
		int ch = (c & 255) | (clr(foreground, background) << 8);
		int xc = GetLeft(wnd)+x;
		int yc = GetTop(wnd)+y;
        w32_put_video(xc, yc, ch);
	}
}

/* ------- write a string to a window ---------- */
void wputs(WINDOW wnd, void *s, int x, int y)
{
	int x1 = GetLeft(wnd)+x;
	int x2 = x1;
	int y1 = GetTop(wnd)+y;
    if (x1 < SCREENWIDTH && y1 < SCREENHEIGHT && isVisible(wnd))	{
		unsigned short int ln[200];
		unsigned short int *cp1 = ln;
	    unsigned char *str = s;
	    int fg = foreground;
    	int bg = background;
	    int len;
		int off = 0;
        while (*str)    {
            if (*str == CHANGECOLOR)    {
                str++;
                foreground = (*str++) & 0x7f;
                background = (*str++) & 0x7f;
                continue;
            }
            if (*str == RESETCOLOR)    {
                foreground = fg & 0x7f;
                background = bg & 0x7f;
                str++;
                continue;
            }
   	        *cp1 = (*str & 255) | (clr(foreground, background) << 8);
			if (ClipString)
				if (!CharInView(wnd, x, y))
					*cp1 = w32_get_video(x2,y1);
			cp1++;
			str++;
			x++;
			x2++;
        }
        foreground = fg;
        background = bg;
   		len = (int)(cp1-ln);
   		if (x1+len > SCREENWIDTH)
       		len = SCREENWIDTH-x1;

		if (!ClipString && !TestAttribute(wnd, NOCLIP))	{
			/* -- clip the line to within ancestor windows -- */
			RECT rc = WindowRect(wnd);
			WINDOW nwnd = GetParent(wnd);
			while (len > 0 && nwnd != NULL)	{
				if (!isVisible(nwnd))	{
					len = 0;
					break;
				}
				rc = subRectangle(rc, ClientRect(nwnd));
				nwnd = GetParent(nwnd);
			}
			while (len > 0 && !InsideRect(x1+off,y1,rc))	{
				off++;
				--len;
			}
			if (len > 0)	{
				x2 = x1+len-1;
				while (len && !InsideRect(x2,y1,rc))	{
					--x2;
					--len;
				}
			}
		}
		if (len > 0)	{
			movetoscreen(ln+off, vad(x1+off,y1), len*2);
		}
    }
}

/* --------- get the current video mode -------- */
void get_videomode(void)
{
}

/* --------- scroll the window. d: 1 = up, 0 = dn ---------- */
void scroll_window(WINDOW wnd, RECT rc, int d)
{
	if (RectTop(rc) != RectBottom(rc))	{
        if (d)
            w32_scroll_up(RectLeft(rc), RectTop(rc),
                          RectRight(rc), RectBottom(rc),
                          clr(WndForeground(wnd),WndBackground(wnd))
            );
        else
            w32_scroll_dw(RectLeft(rc), RectTop(rc),
                          RectRight(rc), RectBottom(rc),
                          clr(WndForeground(wnd),WndBackground(wnd))
            );
	}
}


