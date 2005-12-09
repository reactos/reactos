#ifndef _WIN32K_CLIPRGN_H
#define _WIN32K_CLIPRGN_H

HRGN WINAPI SaveVisRgn(HDC hdc);
INT WINAPI SelectVisRgn(HDC hdc, HRGN hrgn);

int
STDCALL
IntGdiExtSelectClipRgn (
        PDC     dc,
        HRGN    hrgn, 
        int     fnMode
        );

int
STDCALL
NtGdiExcludeClipRect (
	HDC	hDC,
	int	LeftRect,
	int	TopRect,
	int	RightRect,
	int	BottomRect
	);
int
STDCALL
NtGdiExtSelectClipRgn (
	HDC	hDC,
	HRGN	hrgn,
	int	fnMode
	);
int
STDCALL
NtGdiGetClipBox (
	HDC	hDC,
	LPRECT	rc
	);
int
STDCALL
NtGdiGetMetaRgn (
	HDC	hDC,
	HRGN	hrgn
	);
int
STDCALL
NtGdiIntersectClipRect (
	HDC	hDC,
	int	LeftRect,
	int	TopRect,
	int	RightRect,
	int	BottomRect
	);
int
STDCALL
NtGdiOffsetClipRgn (
	HDC	hDC,
	int	XOffset,
	int	YOffset
	);
BOOL
STDCALL
NtGdiPtVisible (
	HDC	hDC,
	int	X,
	int	Y
	);
BOOL
STDCALL
NtGdiRectVisible (
	HDC		hDC,
	CONST PRECT	rc
	);
BOOL
STDCALL
NtGdiSelectClipPath (
	HDC	hDC,
	int	Mode
	);
int
STDCALL
NtGdiSelectClipRgn (
	HDC	hDC,
	HRGN	hrgn
	);
int
STDCALL
NtGdiSetMetaRgn (
	HDC	hDC
	);
#endif
