/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"


/** Public Interface **********************************************************/

PFD_GLYPHATTR
APIENTRY
FtfdQueryGlyphAttrs(
    FONTOBJ *pfo,
    ULONG iMode)
{
    return NULL;
}

LONG
APIENTRY
FtfdQueryFontData(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    ULONG iMode,
    HGLYPH hg,
    OUT GLYPHDATA *pgd,
    PVOID pv,
    ULONG cjSize)
{
    return FD_ERROR;
}
