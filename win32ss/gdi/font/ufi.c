/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include <include/font.h>

#define NDEBUG
#include "font.h"

W32KAPI
BOOL
APIENTRY
NtGdiGetUFI(
    IN  HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG *pfl)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetEmbUFI(
    IN HDC hdc,
    OUT PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL DESIGNVECTOR *pdv,
    OUT ULONG *pcjDV,
    OUT ULONG *pulBaseCheckSum,
    OUT FLONG  *pfl,
    OUT KERNEL_PVOID *embFontID)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetUFIPathname(
    IN PUNIVERSAL_FONT_ID pufi,
    OUT OPTIONAL ULONG* pcwc,
    OUT OPTIONAL LPWSTR pwszPathname,
    OUT OPTIONAL ULONG* pcNumFiles,
    IN FLONG fl,
    OUT OPTIONAL BOOL *pbMemFont,
    OUT OPTIONAL ULONG *pcjView,
    OUT OPTIONAL PVOID pvView,
    OUT OPTIONAL BOOL *pbTTC,
    OUT OPTIONAL ULONG *piTTC)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiSetLinkedUFIs(
    IN HDC hdc,
    IN PUNIVERSAL_FONT_ID pufiLinks,
    IN ULONG uNumUFIs)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
INT
APIENTRY
NtGdiGetLinkedUFIs(
    IN HDC hdc,
    OUT OPTIONAL PUNIVERSAL_FONT_ID pufiLinkedUFIs,
    IN INT BufferSize)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiForceUFIMapping(
    IN HDC hdc,
    IN PUNIVERSAL_FONT_ID pufi)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiRemoveMergeFont(
    IN HDC hdc,
    IN UNIVERSAL_FONT_ID *pufi)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
INT
APIENTRY
NtGdiQueryFonts(
    OUT PUNIVERSAL_FONT_ID pufiFontList,
    IN ULONG nBufferSize,
    OUT PLARGE_INTEGER pTimeStamp)
{
    ASSERT(FALSE);
    return FALSE;
}
