/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Text output
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
BOOL
APIENTRY
EngTextOut (
	SURFOBJ  *pso,
	STROBJ   *pstro,
	FONTOBJ  *pfo,
	CLIPOBJ  *pco,
	RECTL    *prclExtra,
	RECTL    *prclOpaque,
	BRUSHOBJ *pboFore,
	BRUSHOBJ *pboOpaque,
	POINTL   *pptlOrg,
	MIX       mix
	)
{
    ASSERT(FALSE);
    return FALSE;
}

BOOL
APIENTRY
GreExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL PRECTL lprc,
    IN LPWSTR String,
    IN INT Count,
    IN OPTIONAL LPINT Dx,
    IN DWORD dwCodePage)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiExtTextOutW(
    IN HDC hdc,
    IN INT x,
    IN INT y,
    IN UINT flOpts,
    IN OPTIONAL LPRECT prcl,
    IN LPWSTR pwsz,
    IN INT cwc,
    IN OPTIONAL LPINT pdx,
    IN DWORD dwCodePage)
{
    ASSERT(FALSE);
    return 0;
}


BOOL
APIENTRY
NtGdiPolyTextOutW(
    IN HDC hdc,
    IN POLYTEXTW *pptw,
    IN UINT cStr,
    IN DWORD dwCodePage)
{
    ASSERT(FALSE);
    return FALSE;
}


W32KAPI
UINT
APIENTRY
NtGdiGetStringBitmapW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN UINT cwc,
    OUT BYTE *lpSB,
    IN UINT cj)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiConsoleTextOut(
    IN HDC hdc,
    IN POLYTEXTW *lpto,
    IN UINT nStrings,
    IN RECTL *prclBounds)
{
    ASSERT(FALSE);
    return FALSE;
}

