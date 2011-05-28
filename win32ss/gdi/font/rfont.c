/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h">

#define NDEBUG
#include <debug.h>

ULONG
APIENTRY
FONTOBJ_cGetAllGlyphHandles (
	IN FONTOBJ  *FontObj,
	IN HGLYPH   *Glyphs
	)
{
    ASSERT(FALSE);
    return 0;
}

ULONG
APIENTRY
FONTOBJ_cGetGlyphs(
	IN FONTOBJ *FontObj,
	IN ULONG    Mode,
	IN ULONG    NumGlyphs,
	IN HGLYPH  *GlyphHandles,
	IN PVOID   *OutGlyphs)
{
    ASSERT(FALSE);
    return 0;
}

IFIMETRICS*
APIENTRY
FONTOBJ_pifi(IN FONTOBJ *pfo)
{
    ASSERT(FALSE);
    return NULL;
}

PVOID
APIENTRY
FONTOBJ_pvTrueTypeFontFile(
	IN FONTOBJ  *FontObj,
	IN ULONG    *FileSize)
{
    ASSERT(FALSE);
    return NULL;
}

#undef XFORMOBJ
WIN32KAPI
XFORMOBJ*
APIENTRY
FONTOBJ_pxoGetXform(
  IN FONTOBJ *pfo)
{
    ASSERT(FALSE);
    return NULL;
}

/*
 * @unimplemented
 */
VOID
APIENTRY
FONTOBJ_vGetInfo(
	IN  FONTOBJ   *FontObj,
	IN  ULONG      InfoSize,
	OUT PFONTINFO  FontInfo)
{
    ASSERT(FALSE);
}

/*
 * @unimplemented
 */
FD_GLYPHSET * APIENTRY
FONTOBJ_pfdg(
   IN FONTOBJ *FontObj)
{
   ASSERT(FALSE);
   return NULL;
}

/*
 * @unimplemented
 */
PBYTE APIENTRY
FONTOBJ_pjOpenTypeTablePointer(
   IN FONTOBJ *FontObj,
   IN ULONG Tag,
   OUT ULONG *Table)
{
   ASSERT(FALSE);
   return NULL;
}

/*
 * @unimplemented
 */
PFD_GLYPHATTR APIENTRY
FONTOBJ_pQueryGlyphAttrs(
   IN FONTOBJ *FontObj,
   IN ULONG Mode)
{
   ASSERT(FALSE);
   return NULL;
}

/*
 * @unimplemented
 */
LPWSTR APIENTRY
FONTOBJ_pwszFontFilePaths(
   IN FONTOBJ *FontObj,
   OUT ULONG *PathLength)
{
   ASSERT(FALSE);
   return NULL;
}
