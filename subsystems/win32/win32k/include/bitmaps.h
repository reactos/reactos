
#ifndef __WIN32K_BITMAPS_H
#define __WIN32K_BITMAPS_H

#include "win32.h"
#include "gdiobj.h"

/* GDI logical bitmap object */
typedef struct _BITMAPOBJ
{
  BASEOBJECT  BaseObject;

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
  HDC hDC; // Doc in "Undocumented Windows", page 546, seems to be supported with XP.
} BITMAPOBJ, *PBITMAPOBJ;

#define BITMAPOBJ_IS_APIBITMAP		0x1

/*  Internal interface  */

#define  BITMAPOBJ_AllocBitmap()    ((PBITMAPOBJ) GDIOBJ_AllocObj(GDIObjType_SURF_TYPE))
#define  BITMAPOBJ_AllocBitmapWithHandle()    ((PBITMAPOBJ) GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_BITMAP))
#define  BITMAPOBJ_FreeBitmap(pBMObj) GDIOBJ_FreeObj((POBJ) pBMObj, GDIObjType_SURF_TYPE)
#define  BITMAPOBJ_FreeBitmapByHandle(hBMObj) GDIOBJ_FreeObjByHandle((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP)

/* NOTE: Use shared locks! */
#define  BITMAPOBJ_LockBitmap(hBMObj) \
  ((PBITMAPOBJ) GDIOBJ_LockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP))
#define  BITMAPOBJ_ShareLockBitmap(hBMObj) \
  ((PBITMAPOBJ) GDIOBJ_ShareLockObj ((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP))
#define  BITMAPOBJ_UnlockBitmap(pBMObj)  \
  GDIOBJ_UnlockObjByPtr ((POBJ)pBMObj)
#define  BITMAPOBJ_ShareUnlockBitmap(pBMObj)  \
  GDIOBJ_ShareUnlockObjByPtr ((POBJ)pBMObj)

BOOL INTERNAL_CALL BITMAP_Cleanup(PVOID ObjectBody);

BOOL INTERNAL_CALL BITMAPOBJ_InitBitsLock(BITMAPOBJ *pBMObj);
#define BITMAPOBJ_LockBitmapBits(pBMObj) ExEnterCriticalRegionAndAcquireFastMutexUnsafe((pBMObj)->BitsLock)
#define BITMAPOBJ_UnlockBitmapBits(pBMObj) ExReleaseFastMutexUnsafeAndLeaveCriticalRegion((pBMObj)->BitsLock)
void INTERNAL_CALL BITMAPOBJ_CleanupBitsLock(BITMAPOBJ *pBMObj);

INT     FASTCALL BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp);
UINT    FASTCALL BITMAPOBJ_GetRealBitsPixel(UINT nBitsPixel);
HBITMAP FASTCALL BITMAPOBJ_CopyBitmap (HBITMAP  hBitmap);
INT     FASTCALL DIB_GetDIBWidthBytes (INT  width, INT  depth);
int     NTAPI  DIB_GetDIBImageBytes (INT  width, INT  height, INT  depth);
INT     FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
INT     NTAPI  BITMAP_GetObject(BITMAPOBJ * bmp, INT count, LPVOID buffer);
HBITMAP FASTCALL BitmapToSurf(PBITMAPOBJ BitmapObj, HDEV GDIDevice);

#endif


