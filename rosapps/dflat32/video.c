/* --------------------- video.c -------------------- */

#include "dflat.h"

#define clr(fg,bg) ((fg)|((bg)<<4))

BOOL DfClipString;

/* -- read a rectangle of video memory into a save buffer -- */
void DfGetVideo(DFRECT rc, PCHAR_INFO bf)
{
	COORD Size;
	COORD Pos;
	SMALL_RECT Rect;

	Size.X = DfRectRight(rc) - DfRectLeft(rc) + 1;
	Size.Y = DfRectBottom(rc) - DfRectTop(rc) + 1;

	Pos.X = 0;
	Pos.Y = 0;

	Rect.Left   = DfRectLeft(rc);
	Rect.Top    = DfRectTop(rc);
	Rect.Right  = DfRectRight(rc);
	Rect.Bottom = DfRectBottom(rc);

	ReadConsoleOutput (GetStdHandle (STD_OUTPUT_HANDLE),
	                   bf,
	                   Size,
	                   Pos,
	                   &Rect);
}

/* -- write a rectangle of video memory from a save buffer -- */
void DfStoreVideo(DFRECT rc, PCHAR_INFO bf)
{
	COORD Size;
	COORD Pos;
	SMALL_RECT Rect;

	Size.X = DfRectRight(rc) - DfRectLeft(rc) + 1;
	Size.Y = DfRectBottom(rc) - DfRectTop(rc) + 1;

	Pos.X = 0;
	Pos.Y = 0;

	Rect.Left   = DfRectLeft(rc);
	Rect.Top    = DfRectTop(rc);
	Rect.Right  = DfRectRight(rc);
	Rect.Bottom = DfRectBottom(rc);

	WriteConsoleOutput (GetStdHandle (STD_OUTPUT_HANDLE),
	                    bf,
	                    Size,
	                    Pos,
	                    &Rect);
}

/* -------- read a character of video memory ------- */
char DfGetVideoChar(int x, int y)
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
void DfPutVideoChar(int x, int y, int ch)
{
	COORD pos;
	DWORD dwWritten;

	if (x < DfScreenWidth && y < DfScreenHeight)
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

BOOL DfCharInView(DFWINDOW wnd, int x, int y)
{
	DFWINDOW nwnd = DfNextWindow(wnd);
	DFWINDOW pwnd;
	DFRECT rc;
	int x1 = DfGetLeft(wnd)+x;
	int y1 = DfGetTop(wnd)+y;

	if (!DfTestAttribute(wnd, DF_VISIBLE))
		return FALSE;
	if (!DfTestAttribute(wnd, DF_NOCLIP))
	{
		DFWINDOW wnd1 = DfGetParent(wnd);
		while (wnd1 != NULL)
		{
			/* clip character to parent's borders */
			if (!DfTestAttribute(wnd1, DF_VISIBLE))
				return FALSE;
			if (!DfInsideRect(x1, y1, DfClientRect(wnd1)))
				return FALSE;
			wnd1 = DfGetParent(wnd1);
		}
	}
	while (nwnd != NULL)
	{
		if (!isHidden(nwnd) && !DfIsAncestor(wnd, nwnd))
		{
			rc = DfWindowRect(nwnd);
			if (DfTestAttribute(nwnd, DF_SHADOW))
			{
				DfRectBottom(rc)++;
				DfRectRight(rc)++;
			}
			if (!DfTestAttribute(nwnd, DF_NOCLIP))
			{
				pwnd = nwnd;
				while (DfGetParent(pwnd))
				{
					pwnd = DfGetParent(pwnd);
					rc = DfSubRectangle(rc, DfClientRect(pwnd));
				}
			}
			if (DfInsideRect(x1,y1,rc))
				return FALSE;
		}
		nwnd = DfNextWindow(nwnd);
	}
	return (x1 < DfScreenWidth && y1 < DfScreenHeight);
}

/* -------- write a character to a window ------- */
void DfWPutch(DFWINDOW wnd, int c, int x, int y)
{
	if (DfCharInView(wnd, x, y))
	{
		DWORD dwWritten;
		COORD pos;
		WORD Attr;

		pos.X = DfGetLeft(wnd)+x;
		pos.Y = DfGetTop(wnd)+y;

		Attr = clr(DfForeground, DfBackground);

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
void DfWPuts(DFWINDOW wnd, void *s, int x, int y)
{

	int x1 = DfGetLeft(wnd)+x;
	int x2 = x1;
	int y1 = DfGetTop(wnd)+y;

	if (x1 < DfScreenWidth && y1 < DfScreenHeight && DfIsVisible(wnd))
	{
		char ln[200];
		WORD attr[200];
		char *cp = ln;
		WORD *ap = attr;
		unsigned char *str = s;
		int fg = DfForeground;
		int bg = DfBackground;
		int len;
		int off = 0;
		while (*str)
		{
			if (*str == DF_CHANGECOLOR)
			{
				str++;
				DfForeground = (*str++) & 0x7f;
				DfBackground = (*str++) & 0x7f;
				continue;
			}

			if (*str == DF_RESETCOLOR)
			{
				DfForeground = fg & 0x7f;
				DfBackground = bg & 0x7f;
				str++;
				continue;
			}
			*cp = (*str & 255);
			*ap = (WORD)clr(DfForeground, DfBackground);
//			*cp1 = (*str & 255) | (clr(DfForeground, DfBackground) << 8);
//			if (DfClipString)
//				if (!DfCharInView(wnd, x, y))
//					*cp1 = peek(video_address, vad(x2,y1));
			cp++;
			ap++;
			str++;
			x++;
			x2++;
		}
		DfForeground = fg;
		DfBackground = bg;
		len = (int)(cp-ln);
		if (x1+len > DfScreenWidth)
			len = DfScreenWidth-x1;

		if (!DfClipString && !DfTestAttribute(wnd, DF_NOCLIP))
		{
			/* -- clip the line to DfWithin ancestor windows -- */
			DFRECT rc = DfWindowRect(wnd);
			DFWINDOW nwnd = DfGetParent(wnd);
			while (len > 0 && nwnd != NULL)
			{
				if (!DfIsVisible(nwnd))
				{
					len = 0;
					break;
				}
				rc = DfSubRectangle(rc, DfClientRect(nwnd));
				nwnd = DfGetParent(nwnd);
			}
			while (len > 0 && !DfInsideRect(x1+off,y1,rc))
			{
				off++;
				--len;
			}
			if (len > 0)
			{
				x2 = x1+len-1;
				while (len && !DfInsideRect(x2,y1,rc))
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
void DfScrollWindow(DFWINDOW wnd, DFRECT rc, int d)
{
	if (DfRectTop(rc) != DfRectBottom(rc))
	{
		CHAR_INFO ciFill;
		SMALL_RECT rcScroll;
		SMALL_RECT rcClip;
		COORD pos;

		ciFill.Attributes = clr(DfWndForeground(wnd),DfWndBackground(wnd));
		ciFill.Char.AsciiChar = ' ';

		rcScroll.Left = DfRectLeft(rc);
		rcScroll.Right = DfRectRight(rc);
		rcScroll.Top = DfRectTop(rc);
		rcScroll.Bottom = DfRectBottom(rc);

		rcClip = rcScroll;

		pos.X = DfRectLeft(rc);

		if (d == 0)
		{
			/* scroll 1 line down */
			pos.Y = DfRectTop(rc)+1;
		}
		else
		{
			/* scroll 1 line up */
			pos.Y = DfRectTop(rc)-1;
		}

		ScrollConsoleScreenBuffer (GetStdHandle(STD_OUTPUT_HANDLE),
		                           &rcScroll,
		                           &rcClip,
		                           pos,
		                           &ciFill);
	}
}

/* EOF */
