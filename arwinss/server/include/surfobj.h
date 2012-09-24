#ifndef __WIN32K_BITMAP_H
#define __WIN32K_BITMAP_H

#define SRF_BITSALLOCD 0x01 /* GRE allocated memory for bits itself */

typedef struct _SURFACE
{
    BASEOBJECT BaseObject;

    SURFOBJ SurfObj;
    FLONG   flHooks;

    HPALETTE hDIBPalette;
    PFAST_MUTEX pBitsLock; /* grab this lock before accessing actual bits in the bitmap */
    ULONG ulFlags;         /* implementation specific flags */
} SURFACE, *PSURFACE;

HBITMAP
GreCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits);

HBITMAP APIENTRY
IntGdiCreateBitmap(
    INT Width,
    INT Height,
    UINT Planes,
    UINT BitsPixel,
    IN OPTIONAL LPBYTE pBits);

LONG FASTCALL
GreGetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits);

LONG FASTCALL
GreSetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits);

INT FASTCALL
BITMAP_GetWidthBytes(INT bmWidth, INT bpp);

HBITMAP FASTCALL
BITMAP_CopyBitmap(HBITMAP hBitmap);

BOOL APIENTRY
SURFACE_Cleanup(PVOID ObjectBody);

#define GDIDEV(SurfObj) ((PDEVOBJ *)((SurfObj)->hdev))
#define GDIDEVFUNCS(SurfObj) ((PDEVOBJ *)((SurfObj)->hdev))->DriverFunctions

#define  SURFACE_LockSurface(hBMObj) \
  ((PSURFACE) GDIOBJ_LockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP))
#define  SURFACE_ShareLockSurface(hBMObj) \
  ((PSURFACE) GDIOBJ_ShareLockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP))
#define  SURFACE_UnlockSurface(pBMObj)  \
  GDIOBJ_UnlockObjByPtr ((PBASEOBJECT)pBMObj)
#define  SURFACE_ShareUnlockSurface(pBMObj)  \
  GDIOBJ_ShareUnlockObjByPtr ((PBASEOBJECT)pBMObj)

#define SURFACE_LockBitmapBits(pBMObj) ExEnterCriticalRegionAndAcquireFastMutexUnsafe((pBMObj)->pBitsLock)
#define SURFACE_UnlockBitmapBits(pBMObj) ExReleaseFastMutexUnsafeAndLeaveCriticalRegion((pBMObj)->pBitsLock)

#endif
