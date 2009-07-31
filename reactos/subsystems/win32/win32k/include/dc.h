#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

typedef struct _DC
{
    BASEOBJECT    BaseObject;

    PPDEVOBJ     pPDevice;

    /* Device palette */
    HPALETTE     hPalette;

    PSURFACE     pBitmap;
    PBRUSHGDI    pFillBrush;
    PBRUSHGDI    pLineBrush;
    COLORREF     crForegroundClr;
    COLORREF     crBackgroundClr;

    /* Origins and extents */
    RECT rcDcRect; /* Relative to Vport */
    RECT rcVport;

    /* Combined clipping region */
    CLIPOBJ *CombinedClip;

    /* Transformations */
    MATRIX       mxWorldToDevice;
    MATRIX       mxDeviceToWorld;
    MATRIX       mxWorldToPage;
} DC, *PDC;

#define  DC_Lock(hDC)  \
  ((PDC) GDIOBJ_LockObj ((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_Unlock(pDC)  \
  GDIOBJ_UnlockObjByPtr ((PBASEOBJECT)pDC)

INT APIENTRY GreGetDeviceCaps(PDC pDC, INT cap);

#endif
