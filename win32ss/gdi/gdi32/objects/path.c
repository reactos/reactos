/*
 * reactos/lib/gdi32/objects/path.c
 *
 * GDI32.DLL Path
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */

#include <precomp.h>

/*
 * @implemented
 */
BOOL
WINAPI
AbortPath(
    HDC hdc)
{
    HANDLE_METADC0P(BOOL, AbortPath, FALSE, hdc);
    return NtGdiAbortPath(hdc);
}


/*
 * @implemented
 */
BOOL
WINAPI
BeginPath(
    HDC hdc)
{
    HANDLE_METADC0P(BOOL, BeginPath, FALSE, hdc);
    return NtGdiBeginPath(hdc);
}

/*
 * @implemented
 */
BOOL
WINAPI
CloseFigure(
    HDC	hdc)
{
    HANDLE_METADC0P(BOOL, CloseFigure, FALSE, hdc);
    return NtGdiCloseFigure(hdc);
}


/*
 * @implemented
 */
BOOL
WINAPI
EndPath(
    HDC hdc)
{
    HANDLE_METADC0P(BOOL, EndPath, FALSE, hdc);
    return NtGdiEndPath( hdc );
}


/*
 * @implemented
 */
BOOL
WINAPI
FillPath(
    HDC	hdc)
{
    HANDLE_METADC0P(BOOL, FillPath, FALSE, hdc);
    return NtGdiFillPath( hdc );
}


/*
 * @implemented
 */
BOOL
WINAPI
FlattenPath(
    HDC	hdc)
{
    HANDLE_METADC0P(BOOL, FlattenPath, FALSE, hdc);
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
    HDC	hdc)
{
    HANDLE_METADC0P(HRGN, AbortPath, NULL, hdc);
    return NtGdiPathToRegion(hdc);
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
    HDC	hdc)
{
    HANDLE_METADC0P(BOOL, StrokeAndFillPath, FALSE, hdc);
    return NtGdiStrokeAndFillPath ( hdc );
}


/*
 * @implemented
 */
BOOL
WINAPI
StrokePath(
    HDC	hdc)
{
    HANDLE_METADC0P(BOOL, StrokePath, FALSE, hdc);
    return NtGdiStrokePath ( hdc );
}


/*
 * @implemented
 */
BOOL
WINAPI
WidenPath(
    HDC	hdc)
{
    HANDLE_METADC0P(BOOL, WidenPath, FALSE, hdc);
    return NtGdiWidenPath ( hdc );
}

/*
 * @implemented
 */
BOOL
WINAPI
SelectClipPath(
    HDC	hdc,
    int	iMode)
{
    HANDLE_EMETAFDC(BOOL, SelectClipPath, FALSE, hdc, iMode);
    return NtGdiSelectClipPath(hdc, iMode);
}
