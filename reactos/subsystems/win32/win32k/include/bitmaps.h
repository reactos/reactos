
#ifndef __WIN32K_BITMAPS_H
#define __WIN32K_BITMAPS_H

/* GDI logical bitmap object */
typedef struct _BITMAPOBJ
{
  HGDIOBJ     hHmgr;
  PVOID       pvEntry;
  ULONG       lucExcLock;
  ULONG       Tid;

  SURFOBJ     SurfObj;
  FLONG	      flHooks;
  FLONG       flFlags;
  SIZE        dimension;    /* For SetBitmapDimension(), do NOT use
                               to get width/height of bitmap, use
                               bitmap.bmWidth/bitmap.bmHeight for
                               that */
  PFAST_MUTEX BitsLock;     /* You need to hold this lock before you touch
                               the actual bits in the bitmap */

  /* For device-independent bitmaps: */
  DIBSECTION *dib;
  HPALETTE hDIBPalette;
} BITMAPOBJ, *PBITMAPOBJ;

#define BITMAPOBJ_IS_APIBITMAP		0x1

/*  Internal interface  */

#define  BITMAPOBJ_AllocBitmap()  \
  ((HBITMAP) GDIOBJ_AllocObj (GdiHandleTable, GDI_OBJECT_TYPE_BITMAP))
#define  BITMAPOBJ_FreeBitmap(hBMObj)  \
  GDIOBJ_FreeObj(GdiHandleTable, (HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP)
/* NOTE: Use shared locks! */
#define  BITMAPOBJ_LockBitmap(hBMObj) (PBITMAPOBJ)EngLockSurface((HSURF)hBMObj)
#define  BITMAPOBJ_UnlockBitmap(pBMObj) EngUnlockSurface(&pBMObj->SurfObj)
BOOL INTERNAL_CALL BITMAP_Cleanup(PVOID ObjectBody);

BOOL INTERNAL_CALL BITMAPOBJ_InitBitsLock(BITMAPOBJ *pBMObj);
#define BITMAPOBJ_LockBitmapBits(pBMObj) ExEnterCriticalRegionAndAcquireFastMutexUnsafe((pBMObj)->BitsLock)
#define BITMAPOBJ_UnlockBitmapBits(pBMObj) ExReleaseFastMutexUnsafeAndLeaveCriticalRegion((pBMObj)->BitsLock)
void INTERNAL_CALL BITMAPOBJ_CleanupBitsLock(BITMAPOBJ *pBMObj);

INT     FASTCALL BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp);
INT     FASTCALL BITMAPOBJ_GetRealBitsPixel(INT nBitsPixel);
HBITMAP FASTCALL BITMAPOBJ_CopyBitmap (HBITMAP  hBitmap);
INT     FASTCALL DIB_GetDIBWidthBytes (INT  width, INT  depth);
int     NTAPI  DIB_GetDIBImageBytes (INT  width, INT  height, INT  depth);
INT     FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
INT     NTAPI  BITMAP_GetObject(BITMAPOBJ * bmp, INT count, LPVOID buffer);
HBITMAP FASTCALL BitmapToSurf(PBITMAPOBJ BitmapObj, HDEV GDIDevice);

#endif

