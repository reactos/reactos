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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
FlattenPath(
	HDC	hdc
	)
{
	return NtGdiFlattenPath ( hdc );
}


/*
 * @implemented
 */
INT
WINAPI
GetPath(HDC hdc,
        LPPOINT pptlBuf,
        LPBYTE pjTypes,
        INT cptBuf)
{
    INT retValue = -1;

    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        retValue = NtGdiGetPath(hdc,pptlBuf,pjTypes,cptBuf);
    }

    return retValue;
}


/*
 * @implemented
 */
HRGN
WINAPI
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
WINAPI
SetMiterLimit(
	HDC	hdc,
	FLOAT	a1,
	PFLOAT	a2
	)
{
  BOOL Ret;
  gxf_long worker, worker1;

  worker.f  = a1;
  Ret = NtGdiSetMiterLimit ( hdc, worker.l, a2 ? &worker1.l : NULL  );
  if (a2 && Ret) *a2 = worker1.f;
  return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
SelectClipPath(
	HDC	hdc,
	int	Mode
	)
{
	return NtGdiSelectClipPath ( hdc, Mode );
}
