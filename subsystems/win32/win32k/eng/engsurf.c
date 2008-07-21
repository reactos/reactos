/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engsurf.c
 * PURPOSE:         Surface and Bitmap Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HBITMAP
APIENTRY
EngCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngCopyBits(OUT SURFOBJ* DestSurf,
            IN SURFOBJ* SourceSurf,
            IN CLIPOBJ* ClipRegion,
            IN XLATEOBJ* ColorTranslation,
            IN PRECTL prclDest,
            IN PPOINTL pptlSrc)
{
    UNIMPLEMENTED;
	return FALSE;
}

HBITMAP
APIENTRY
EngCreateDeviceBitmap(IN DHSURF dhSurf,
                      IN SIZEL Size,
                      IN ULONG Format)
{
    UNIMPLEMENTED;
    return NULL;
}

HSURF
APIENTRY
EngCreateDeviceSurface(IN DHSURF dhSurf,
                       IN SIZEL Size,
                       IN ULONG Format)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
EngDeleteSurface(IN HSURF hSurf)
{
    UNIMPLEMENTED;
    return FALSE;
}

SURFOBJ*
APIENTRY
EngLockSurface(IN HSURF hSurf)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
APIENTRY
EngUnlockSurface(IN SURFOBJ* Surface)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
EngAssociateSurface(IN HSURF hSurf,
                    IN HDEV hDev,
                    IN FLONG flHooks)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngEraseSurface(IN SURFOBJ* Surface,
                IN PRECTL prcl,
                IN ULONG iColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngModifySurface(IN HSURF hSurf,
                 IN HDEV hDev,
                 IN FLONG flHooks,
                 IN FLONG flSurface,
                 IN DHSURF dhSurf,
                 IN PVOID pvScan0,
                 IN LONG lDelta,
                 IN PVOID pvReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}
