#ifndef __WIN32K_COORD_H
#define __WIN32K_COORD_H

BOOL
STDCALL
NtGdiCombineTransform (
	LPXFORM		XformResult,
	CONST LPXFORM	xform1,
	CONST LPXFORM	xform2
	);

VOID
FASTCALL
IntDPtoLP ( PDC dc, LPPOINT Points, INT Count );

VOID
FASTCALL
CoordDPtoLP ( PDC Dc, LPPOINT Point );

BOOL
STDCALL
NtGdiDPtoLP (
	HDC	hDC,
	LPPOINT	Points,
	int	Count
	);

int
FASTCALL
IntGetGraphicsMode ( PDC dc );

int
STDCALL
NtGdiGetGraphicsMode ( HDC hDC );

BOOL
STDCALL
NtGdiGetWorldTransform (
	HDC	hDC,
	LPXFORM	Xform
	);

VOID
FASTCALL
CoordLPtoDP ( PDC Dc, LPPOINT Point );

VOID
FASTCALL
IntLPtoDP ( PDC dc, LPPOINT Points, INT Count );

BOOL
STDCALL
NtGdiLPtoDP (
	HDC	hDC,
	LPPOINT	Points,
	int	Count
	);
BOOL
STDCALL
NtGdiModifyWorldTransform (
	HDC		hDC,
	CONST LPXFORM	Xform,
	DWORD		Mode
	);
BOOL
STDCALL
NtGdiOffsetViewportOrgEx (
	HDC	hDC,
	int	XOffset,
	int	YOffset,
	LPPOINT	Point
	);
BOOL
STDCALL
NtGdiOffsetWindowOrgEx (
	HDC	hDC,
	int	XOffset,
	int	YOffset,
	LPPOINT	Point
	);
BOOL
STDCALL
NtGdiScaleViewportExtEx (
	HDC	hDC,
	int	Xnum,
	int	Xdenom,
	int	Ynum,
	int	Ydenom,
	LPSIZE	Size
	);
BOOL
STDCALL
NtGdiScaleWindowExtEx (
	HDC	hDC,
	int	Xnum,
	int	Xdenom,
	int	Ynum,
	int	Ydenom,
	LPSIZE  Size
	);
int
STDCALL
NtGdiSetGraphicsMode (
	HDC	hDC,
	int	Mode
	);
int
STDCALL
NtGdiSetMapMode (
	HDC	hDC,
	int	MapMode
	);
BOOL
STDCALL
NtGdiSetViewportExtEx (
	HDC	hDC,
	int	XExtent,
	int	YExtent,
	LPSIZE	Size
	);
BOOL
STDCALL
NtGdiSetViewportOrgEx (
	HDC	hDC,
	int	X,
	int	Y,
	LPPOINT	Point
	);
BOOL
STDCALL
NtGdiSetWindowExtEx (
	HDC	hDC,
	int	XExtent,
	int	YExtent,
	LPSIZE	Size
	);
BOOL
STDCALL
NtGdiSetWindowOrgEx (
	HDC	hDC,
	int	X,
	int	Y,
	LPPOINT	Point
	);
BOOL
STDCALL
NtGdiSetWorldTransform (
	HDC		hDC,
	CONST LPXFORM	Xform
	);
#endif
