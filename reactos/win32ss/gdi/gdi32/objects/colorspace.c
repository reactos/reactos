#include <precomp.h>

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
BOOL
WINAPI
GetLogColorSpaceA(
    HCOLORSPACE		a0,
    LPLOGCOLORSPACEA	a1,
    DWORD			a2
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetLogColorSpaceW(
    HCOLORSPACE		a0,
    LPLOGCOLORSPACEW	a1,
    DWORD			a2
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CheckColorsInGamut(
    HDC	a0,
    LPVOID	a1,
    LPVOID	a2,
    DWORD	a3
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetDeviceGammaRamp( HDC hdc,
                    LPVOID lpGammaRamp)
{
    BOOL retValue = FALSE;
    if (lpGammaRamp == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        retValue = NtGdiGetDeviceGammaRamp(hdc,lpGammaRamp);
    }

    return retValue;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetDeviceGammaRamp(HDC hdc,
                   LPVOID lpGammaRamp)
{
    BOOL retValue = FALSE;

    if (lpGammaRamp)
    {
        retValue = NtGdiSetDeviceGammaRamp(hdc, lpGammaRamp);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return  retValue;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
ColorMatchToTarget(
    HDC	a0,
    HDC	a1,
    DWORD	a2
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetColorAdjustment(
    HDC			hdc,
    CONST COLORADJUSTMENT	*a1
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
