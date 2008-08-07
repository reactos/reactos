/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gditext.c
 * PURPOSE:         GDI Text Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiExtTextOutW(IN HDC hdc,
                 IN INT x,
                 IN INT y,
                 IN UINT flOpts,
                 IN OPTIONAL LPRECT prcl,
                 IN LPWSTR pwsz,
                 IN INT cwc,
                 IN OPTIONAL LPINT pdx,
                 IN DWORD dwCodePage)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiPolyTextOutW(IN HDC hdc,
                  IN POLYTEXTW *pptw,
                  IN UINT cStr,
                  IN DWORD dwCodePage)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetETM(IN HDC hdc,
            OUT EXTTEXTMETRIC *petm)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiGetTextCharsetInfo(IN HDC hdc,
                        OUT OPTIONAL LPFONTSIGNATURE lpSig,
                        IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSetTextJustification(IN HDC hdc,
                          IN INT lBreakExtra,
                          IN INT cBreak)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetTextExtent(IN HDC hdc,
                   IN LPWSTR lpwsz,
                   IN INT cwc,
                   OUT LPSIZE psize,
                   IN UINT flOpts)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetTextExtentExW(IN HDC hdc,
                      IN OPTIONAL LPWSTR lpwsz,
                      IN ULONG cwc,
                      IN ULONG dxMax,
                      OUT OPTIONAL ULONG *pcCh,
                      OUT OPTIONAL PULONG pdxOut,
                      OUT LPSIZE psize,
                      IN FLONG fl)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiGetTextFaceW(IN HDC hdc,
                  IN INT cChar,
                  OUT OPTIONAL LPWSTR pszOut,
                  IN BOOL bAliasName)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetTextMetricsW(IN HDC hdc,
                     OUT TMW_INTERNAL * ptm,
                     IN ULONG cj)
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT
APIENTRY
NtGdiGetStringBitmapW(IN HDC hdc,
                      IN LPWSTR pwsz,
                      IN UINT cwc,
                      OUT BYTE *lpSB,
                      IN UINT cj)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetCharABCWidthsW(IN HDC hdc,
                       IN UINT wchFirst,
                       IN ULONG cwch,
                       IN OPTIONAL PWCHAR pwch,
                       IN FLONG fl,
                       OUT PVOID pvBuf)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtGdiGetCharacterPlacementW(IN HDC hdc,
                            IN LPWSTR pwsz,
                            IN INT nCount,
                            IN INT nMaxExtent,
                            IN OUT LPGCP_RESULTSW pgcpw,
                            IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
/* Missing APIENTRY! */
NtGdiGetCharSet(IN HDC hdc)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetCharWidthW(IN HDC hdc,
                   IN UINT wcFirst,
                   IN UINT cwc,
                   IN OPTIONAL PWCHAR pwc,
                   IN FLONG fl,
                   OUT PVOID pvBuf)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetCharWidthInfo(IN HDC hdc,
                      OUT PCHWIDTHINFO pChWidthInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(IN HDC hdc,
                                    IN ULONG cjotm,
                                    OUT OPTIONAL OUTLINETEXTMETRICW *potmw,
                                    OUT TMDIFF *ptmd)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetWidthTable(IN HDC hdc,
                   IN ULONG cSpecial,
                   IN WCHAR *pwc,
                   IN ULONG cwc,
                   OUT USHORT *psWidth,
                   OUT OPTIONAL WIDTHDATA *pwd,
                   OUT FLONG *pflInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}
