/* $Id: stubs.c 18897 2005-12-08 23:10:33Z cwittich $
 *
 * reactos/lib/gdi32/objects/path.c
 *
 * GDI32.DLL Path
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */

#include "precomp.h"

/*
 * @implemented
 */
BOOL
STDCALL
AbortPath(
	HDC	hdc
	)
{
	return NtGdiAbortPath( hdc );
}


/*
 * @implemented
 */
BOOL
STDCALL
BeginPath(
	HDC	hdc
	)
{
	return NtGdiBeginPath( hdc );
}

/*
 * @implemented
 */
BOOL
STDCALL
CloseFigure(
	HDC	hdc
	)
{
	return NtGdiCloseFigure ( hdc );
}


/*
 * @implemented
 */
BOOL
STDCALL
EndPath(
	HDC	hdc
	)
{
	return NtGdiEndPath( hdc );
}


/*
 * @implemented
 */
BOOL
STDCALL
FillPath(
	HDC	hdc
	)
{
	return NtGdiFillPath( hdc );
}


/*
 * @implemented
 */
BOOL
STDCALL
FlattenPath(
	HDC	hdc
	)
{
	return NtGdiFlattenPath ( hdc );
}


/*
 * @implemented
 */
int
STDCALL
GetPath(
	HDC		hdc,
	LPPOINT		a1,
	LPBYTE		a2,
	int		a3
	)
{
	return NtGdiGetPath ( hdc, a1, a2, a3 );
}


/*
 * @implemented
 */
HRGN
STDCALL
PathToRegion(
	HDC	hdc
	)
{
	return NtGdiPathToRegion ( hdc );
}

/*
 * @implemented
 */
BOOL
STDCALL
SetMiterLimit(
	HDC	hdc,
	FLOAT	a1,
	PFLOAT	a2
	)
{
	return NtGdiSetMiterLimit ( hdc, a1, (PDWORD)a2 );
}


/*
 * @implemented
 */
BOOL
STDCALL
StrokeAndFillPath(
	HDC	hdc
	)
{
	return NtGdiStrokeAndFillPath ( hdc );
}


/*
 * @implemented
 */
BOOL
STDCALL
StrokePath(
	HDC	hdc
	)
{
	return NtGdiStrokePath ( hdc );
}


/*
 * @implemented
 */
BOOL
STDCALL
WidenPath(
	HDC	hdc
	)
{
	return NtGdiWidenPath ( hdc );
}


/*
 * @implemented
 */
BOOL
STDCALL
GetMiterLimit(
	HDC	hdc,
	PFLOAT	a1
	)
{
	return NtGdiGetMiterLimit ( hdc, (PDWORD)a1 );
}

/*
 * @implemented
 */
BOOL
STDCALL
SelectClipPath(
	HDC	hdc,
	int	Mode
	)
{
	return NtGdiSelectClipPath ( hdc, Mode );
}
