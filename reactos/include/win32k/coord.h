#ifndef __WIN32K_COORD_H
#define __WIN32K_COORD_H

BOOL
STDCALL
W32kCombineTransform (
	LPXFORM		XformResult,
	CONST LPXFORM	xform1,
	CONST LPXFORM	xform2
	);
BOOL
STDCALL
W32kDPtoLP (
	HDC	hDC,
	LPPOINT	Points,
	int	Count
	);
int
STDCALL
W32kGetGraphicsMode (
	HDC	hDC
	);
BOOL
STDCALL
W32kGetWorldTransform (
	HDC	hDC,
	LPXFORM	Xform
	);
BOOL
STDCALL
W32kLPtoDP (
	HDC	hDC,
	LPPOINT	Points,
	int	Count
	);
BOOL
STDCALL
W32kModifyWorldTransform (
	HDC		hDC,
	CONST LPXFORM	Xform,
	DWORD		Mode
	);
BOOL
STDCALL
W32kOffsetViewportOrgEx (
	HDC	hDC,
	int	XOffset,
	int	YOffset,
	LPPOINT	Point
	);
BOOL
STDCALL
W32kOffsetWindowOrgEx (
	HDC	hDC,
	int	XOffset,
	int	YOffset,
	LPPOINT	Point
	);
BOOL
STDCALL
W32kScaleViewportExtEx (
	HDC	hDC,
	int	Xnum,
	int	Xdenom,
	int	Ynum,
	int	Ydenom,
	LPSIZE	Size
	);
BOOL
STDCALL
W32kScaleWindowExtEx (
	HDC	hDC,
	int	Xnum,
	int	Xdenom,
	int	Ynum,
	int	Ydenom,
	LPSIZE  Size
	);
int
STDCALL
W32kSetGraphicsMode (
	HDC	hDC,
	int	Mode
	);
int
STDCALL
W32kSetMapMode (
	HDC	hDC,
	int	MapMode
	);
BOOL
STDCALL
W32kSetViewportExtEx (
	HDC	hDC,
	int	XExtent,
	int	YExtent,
	LPSIZE	Size
	);
BOOL
STDCALL
W32kSetViewportOrgEx (
	HDC	hDC,
	int	X,
	int	Y,
	LPPOINT	Point
	);
BOOL
STDCALL
W32kSetWindowExtEx (
	HDC	hDC,
	int	XExtent,
	int	YExtent,
	LPSIZE	Size
	);
BOOL
STDCALL
W32kSetWindowOrgEx (
	HDC	hDC,
	int	X,
	int	Y,
	LPPOINT	Point
	);
BOOL
STDCALL
W32kSetWorldTransform (
	HDC		hDC,
	CONST LPXFORM	Xform
	);
#endif
