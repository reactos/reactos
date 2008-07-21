/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engtext.c
 * PURPOSE:         Text Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngTextOut(IN SURFOBJ* Surface,
           IN STROBJ* StringObj,
           IN FONTOBJ* FontObj,
           IN CLIPOBJ* ClipRegion,
           IN PRECTL prclExtra,
           IN PRECTL prclOpaque,
           IN BRUSHOBJ* ForegroundBrush,
           IN BRUSHOBJ* OpaqueBrush,
           IN PPOINTL BrushOrigin,
           IN MIX Mix)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
STROBJ_bEnum(IN STROBJ* StringObj,
             OUT PULONG cGlyphpos,
             OUT PGLYPHPOS* Glyphpos)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
STROBJ_bEnumPositionsOnly(IN STROBJ* StringObj,
                          OUT PULONG cGlyphpos,
                          OUT PGLYPHPOS* Glyphpos)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
STROBJ_bGetAdvanceWidths(IN STROBJ* StringObj,
                         IN ULONG iFirst,
                         IN ULONG cCharacters,
                         OUT POINTQF* pptqD)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
APIENTRY
STROBJ_dwGetCodePage(IN STROBJ* StringObj)
{
    UNIMPLEMENTED;
	return 0;
}

FIX
APIENTRY
STROBJ_fxBreakExtra(IN STROBJ* StringObj)
{
    UNIMPLEMENTED;
	return 0;
}

FIX
APIENTRY
STROBJ_fxCharacterExtra(IN STROBJ* StringObj)
{
    UNIMPLEMENTED;
	return 0;
}

VOID
APIENTRY
STROBJ_vEnumStart(IN STROBJ* StringObj)
{
    UNIMPLEMENTED;
}
