/* --------------------- video.c -------------------- */

#include "dflat.h"

#define clr(fg,bg) ((fg)|((bg)<<4))

BOOL ClipString;

/* -- read a rectangle of video memory into a save buffer -- */
void GetVideo(DFRECT rc, PCHAR_INFO bf)
{
	COORD Size;
	COORD Pos;
	SMALL_RECT Rect;

	Size.X = RectRight(rc) - RectLeft(rc) + 1;
	Size.Y = RectBottom(rc) - RectTop(rc) + 1;

	Pos.X = 0;
	Pos.Y = 0;

	Rect.Left   = RectLeft(rc);
	Rect.Top    = RectTop(rc);
	Rect.Right  = RectRight(rc);
	Rect.Bottom = RectBottom(rc);

	ReadConsoleOutput (GetStdHandle (STD_OUTPUT_HANDLE),
	                   bf,
	                   Size,
	                   Pos,
	                   &Rect);
}

/* -- write a rectangle of video memory from a save buffer -- */
void StoreVideo(DFRECT rc, PCHAR_INFO bf)
{
	COORD Size;
	COORD Pos;
	SMALL_RECT Rect;

	Size.X = RectRight(rc) - RectLeft(rc) + 1;
	Size.Y = RectBottom(rc) - RectTop(rc) + 1;

	Pos.X = 0;
	Pos.Y = 0;

	Rect.Left   = RectLeft(rc);
	Rect.Top    = RectTop(rc);
	Rect.Right  = RectRight(rc);
	Rect.Bottom = RectBottom(rc);

	WriteConsoleOutput (GetStdHandle (STD_OUTPUT_HANDLE),
	                    bf,
	                    Size,
	                    Pos,
	                    &Rect);
}

/* -------- read a character of video memory ------- */
char GetVideoChar(int x, int y)
{
	COORD pos;
	DWORD dwRead;
	char ch;

	pos.X = x;
	pos.Y = y;

	ReadConsoleOutputCharacter (GetStdHandle(STD_OUTPUT_HANDLE),
	                            &ch,
	                            1,
	                            pos,
	                            &dwRead);

	return ch;
}

/* -------- write a character of video memory ------- */
void PutVideoChar(int x, int y, int ch)
{
	COORD pos;
	DWORD dwWritten;

	if (x < sScreenWidth && y < sScreenHeight)
	{
		pos.X = x;
		pos.Y = y;

		WriteConsoleOutputCharacter (GetStdHandle(STD_OUTPUT_HANDLE),
		                             (char *)&ch,
		                             1,
		                             pos,
		                             &dwWritten);
	}
}

BOOL CharInView(DFWINDOW wnd, int x, int y)
{
	DFWINDOW nwnd = NextWindow(wnd);
	DFWINDOW pwnd;
	DFRECT rc;
	int x1 = GetLeft(wnd)+x;
	int y1 = GetTop(wnd)+y;

	if (!TestAttribute(wnd, VISIBLE))
		return FALSE;
	if (!TestAttribute(wnd, NOCLIP))
	{
		DFWINDOW wnd1 = GetParent(wnd);
		while (wnd1 != NULL)
		{
			/* clip character to parent's borders */
			if (!TestAttribute(wnd1, VISIBLE))
				return FALSE;
			if (!InsideRect(x1, y1, ClientRect(wnd1)))
				return FALSE;
			wnd1 = GetParent(wnd1);
		}
	}
	while (nwnd != NULL)
	{
		if (!isHidden(nwnd) && !isAncestor(wnd, nwnd))
		{
			rc = WindowRect(nwnd);
			if (TestAttribute(nwnd, SHADOW))
			{
				RectBottom(rc)++;
				RectRight(rc)++;
			}
			if (!TestAttribute(nwnd, NOCLIP))
			{
				pwnd = nwnd;
				while (GetParent(pwnd))
				{
					pwnd = GetParent(pwnd);
					rc = subRectangle(rc, ClientRect(pwnd));
				}
			}
			if (InsideRect(x1,y1,rc))
				return FALSE;
		}
		nwnd = NextWindow(nwnd);
	}
	return (x1 < sScreenWidth && y1 < sScreenHeight);
}

/* -------- write a character to a window ------- */
void wputch(DFWINDOW wnd, int c, int x, int y)
{
	if (CharInView(wnd, x, y))
	{
		DWORD dwWritten;
		COORD pos;
		WORD Attr;

		pos.X = GetLeft(wnd)+x;
		pos.Y = GetTop(wnd)+y;

		Attr = clr(foreground, background);

		WriteConsoleOutputAttribute (GetStdHandle(STD_OUTPUT_HANDLE),
		                             &Attr,
		                             1,
		                             pos,
		                             &dwWritten);

		WriteConsoleOutputCharacter (GetStdHandle(STD_OUTPUT_HANDLE),
		                             (char *)&c,
		                             1,
		                             pos,
		                             &dwWritten);
	}
}

/* ------- write a string to a window ---------- */
void wputs(DFWINDOW wnd, void *s, int x, int y)
{

	int x1 = GetLeft(wnd)+x;
	int x2 = x1;
	int y1 = GetTop(wnd)+y;

	if (x1 < sScreenWidth && y1 < sScreenHeight && isVisible(wnd))
	{
		char ln[200];
		WORD attr[200];
		char *cp = ln;
		WORD *ap = attr;
		unsigned char *str = s;
		int fg = foreground;
		int bg = background;
		int len;
		int off = 0;
		while (*str)
		{
			if (*str == CHANGECOLOR)
			{
				str++;
				foreground = (*str++) & 0x7f;
				background = (*str++) & 0x7f;
				continue;
			}

			if (*str == RESETCOLOR)
			{
				foreground = fg & 0x7f;
				background = bg & 0x7f;
				str++;
				continue;
			}
			*cp = (*str & 255);
			*ap = (WORD)clr(foreground, background);
//			*cp1 = (*str & 255) | (clr(foreground, background) << 8);
//			if (ClipString)
//				if (!CharInView(wnd, x, y))
//					*cp1 = peek(video_address, vad(x2,y1));
			cp++;
			ap++;
			str++;
			x++;
			x2++;
		}
		foreground = fg;
		background = bg;
		len = (int)(cp-ln);
		if (x1+len > sScreenWidth)
			len = sScreenWidth-x1;

		if (!ClipString && !TestAttribute(wnd, NOCLIP))
		{
			/* -- clip the line to within ancestor windows -- */
			DFRECT rc = WindowRect(wnd);
			DFWINDOW nwnd = GetParent(wnd);
			while (len > 0 && nwnd != NULL)
			{
				if (!isVisible(nwnd))
				{
					len = 0;
					break;
				}
				rc = subRectangle(rc, ClientRect(nwnd));
				nwnd = GetParent(nwnd);
			}
			while (len > 0 && !InsideRect(x1+off,y1,rc))
			{
				off++;
				--len;
			}
			if (len > 0)
			{
				x2 = x1+len-1;
				while (len && !InsideRect(x2,y1,rc))
				{
					--x2;
					--len;
				}
			}
		}
		if (len > 0)
		{
			COORD pos;
			DWORD dwWritten;

			pos.X = x1;
			pos.Y = y1;

			WriteConsoleOutputAttribute (GetStdHandle(STD_OUTPUT_HANDLE),
			                             attr,
			                             len,
			                             pos,
			                             &dwWritten);

			WriteConsoleOutputCharacter (GetStdHandle(STD_OUTPUT_HANDLE),
			                             ln,
			                             len,
			                             pos,
			                             &dwWritten);
		}
	}
}

/* --------- scroll the window. d: 1 = up, 0 = dn ---------- */
void scroll_window(DFWINDOW wnd, DFRECT rc, int d)
{
	if (RectTop(rc) != RectBottom(rc))
	{
		CHAR_INFO ciFill;
		SMALL_RECT rcScroll;
		SMALL_RECT rcClip;
		COORD pos;

		ciFill.Attributes = clr(WndForeground(wnd),WndBackground(wnd));
		ciFill.Char.AsciiChar = ' ';

		rcScroll.Left = RectLeft(rc);
		rcScroll.Right = RectRight(rc);
		rcScroll.Top = RectTop(rc);
		rcScroll.Bottom = RectBottom(rc);

		rcClip = rcScroll;

		pos.X = RectLeft(rc);

		if (d == 0)
		{
			/* scroll 1 line down */
			pos.Y = RectTop(rc)+1;
		}
		else
		{
			/* scroll 1 line up */
			pos.Y = RectTop(rc)-1;
		}

		ScrollConsoleScreenBuffer (GetStdHandle(STD_OUTPUT_HANDLE),
		                           &rcScroll,
		                           &rcClip,
		                           pos,
		                           &ciFill);
	}
}

/* EOF */
