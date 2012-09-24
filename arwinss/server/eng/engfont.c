/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engfont.c
 * PURPOSE:         Font Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

FD_GLYPHSET*
APIENTRY
EngComputeGlyphSet(IN INT nCodePage,
                   IN INT nFirstChar,
                   IN INT cChars)
{
    UNIMPLEMENTED;
	return NULL;
}

PVOID
APIENTRY
EngFntCacheAlloc(IN ULONG FastCheckSum,
                 IN ULONG ulSize)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
EngFntCacheFault(IN ULONG ulFastCheckSum,
                 IN ULONG iFaultMode)
{
    UNIMPLEMENTED;
}

PVOID
APIENTRY
EngFntCacheLookUp(IN ULONG FastCheckSum,
                  OUT PULONG pulSize)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngGetType1FontList(
  IN HDEV  hdev,
  OUT TYPE1_FONT  *pType1Buffer,
  IN ULONG  cjType1Buffer,
  OUT PULONG  pulLocalFonts,
  OUT PULONG  pulRemoteFonts,
  OUT LARGE_INTEGER  *pLastModified)
{
    UNIMPLEMENTED;
	return FALSE;
}

ULONG
APIENTRY
FONTOBJ_cGetAllGlyphHandles(
  IN FONTOBJ  *pfo,
  OUT HGLYPH  *phg)
{
    UNIMPLEMENTED;
	return 0;
}

ULONG
APIENTRY
FONTOBJ_cGetGlyphs(IN FONTOBJ  *pfo,
                   IN ULONG  iMode,
                   IN ULONG  cGlyph,
                   IN HGLYPH  *phg,
                   OUT PVOID  *ppvGlyph)
{
    UNIMPLEMENTED;
	return 0;
}

FD_GLYPHSET*
APIENTRY
FONTOBJ_pfdg(IN FONTOBJ  *pfo)
{
    UNIMPLEMENTED;
	return NULL;
}

IFIMETRICS*
APIENTRY
FONTOBJ_pifi(IN FONTOBJ  *pfo)
{
    UNIMPLEMENTED;
	return NULL;
}

PBYTE
APIENTRY
FONTOBJ_pjOpenTypeTablePointer(IN FONTOBJ  *pfo,
                               IN ULONG  ulTag,
                               OUT ULONG  *pcjTable)
{
    UNIMPLEMENTED;
	return NULL;
}

PFD_GLYPHATTR
APIENTRY
FONTOBJ_pQueryGlyphAttrs(IN FONTOBJ  *pfo,
                         IN ULONG  iMode)
{
    UNIMPLEMENTED;
	return NULL;
}

PVOID
APIENTRY
FONTOBJ_pvTrueTypeFontFile(IN FONTOBJ  *pfo,
                           OUT ULONG  *pcjFile)
{
    UNIMPLEMENTED;
	return NULL;
}

LPWSTR
APIENTRY
FONTOBJ_pwszFontFilePaths(IN FONTOBJ  *pfo,
                          OUT ULONG  *pcwc)
{
    UNIMPLEMENTED;
	return NULL;
}

XFORMOBJ*
APIENTRY
FONTOBJ_pxoGetXform(IN FONTOBJ  *pfo)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
FONTOBJ_vGetInfo(IN FONTOBJ  *pfo,
                 IN ULONG  cjSize,
                 OUT FONTINFO  *pfi)
{
    UNIMPLEMENTED;
}
