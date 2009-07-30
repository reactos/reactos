#ifndef __WIN32K_BITMAP_H
#define __WIN32K_BITMAP_H


typedef struct _SURFACE
{
    GDIOBJHDR BaseObject;

    SURFOBJ SurfObj;
    FLONG   flHooks;

    HPALETTE hDIBPalette;
} SURFACE, *PSURFACE;

HBITMAP
GreCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits);

VOID FASTCALL
GreDeleteBitmap(HGDIOBJ hBitmap);

LONG FASTCALL
GreGetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits);

LONG FASTCALL
GreSetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits);

INT FASTCALL
BITMAP_GetWidthBytes(INT bmWidth, INT bpp);

#define GDIDEVFUNCS(SurfObj) ((PDEVOBJ *)((SurfObj)->hdev))->DriverFunctions


#define SURFACE_LockBitmapBits(x)
#define SURFACE_UnlockBitmapBits(x)

#define MouseSafetyOnDrawStart(srf, a, b, c, d)
#define MouseSafetyOnDrawEnd(srf)

#endif
