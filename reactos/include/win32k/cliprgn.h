#ifndef _WIN32K_CLIPRGN_H
#define _WIN32K_CLIPRGN_H

HRGN WINAPI SaveVisRgn(HDC hdc);
INT WINAPI SelectVisRgn(HDC hdc, HRGN hrgn);

int
STDCALL
W32kExcludeClipRect (
	HDC	hDC,
	int	LeftRect,
	int	TopRect,
	int	RightRect,
	int	BottomRect
	);
int
STDCALL
W32kExtSelectClipRgn (
	HDC	hDC,
	HRGN	hrgn,
	int	fnMode
	);
int
STDCALL
W32kGetClipBox (
	HDC	hDC,
	LPRECT	rc
	);
int
STDCALL
W32kGetMetaRgn (
	HDC	hDC,
	HRGN	hrgn
	);
int
STDCALL
W32kIntersectClipRect (
	HDC	hDC,
	int	LeftRect,
	int	TopRect,
	int	RightRect,
	int	BottomRect
	);
int
STDCALL
W32kOffsetClipRgn (
	HDC	hDC,
	int	XOffset,
	int	YOffset
	);
BOOL
STDCALL
W32kPtVisible (
	HDC	hDC,
	int	X,
	int	Y
	);
BOOL
STDCALL
W32kRectVisible (
	HDC		hDC,
	CONST PRECT	rc
	);
BOOL
STDCALL
W32kSelectClipPath (
	HDC	hDC,
	int	Mode
	);
int
STDCALL
W32kSelectClipRgn (
	HDC	hDC,
	HRGN	hrgn
	);
int
STDCALL
W32kSetMetaRgn (
	HDC	hDC
	);
#endif
