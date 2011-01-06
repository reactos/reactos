#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

typedef struct _DCLEVEL
{
    HPALETTE hpal;
    struct _PALETTE * ppal;

    PSURFACE pSurface;

    PBRUSH   pbrFill;
    PBRUSH   pbrLine;

    POINTL   ptlBrushOrigin;
} DCLEVEL, *PDCLEVEL;

typedef struct _DC
{
    BASEOBJECT    BaseObject;

    PPDEVOBJ     ppdev;

    DCLEVEL     dclevel;

    COLORREF     crForegroundClr;
    COLORREF     crBackgroundClr;

    EBRUSHOBJ   eboFill;
    EBRUSHOBJ   eboLine;

    /* Origins and extents */
    RECT         rcVport;

    /* Combined clipping region */
    struct region *Clipping;
    CLIPOBJ      *CombinedClip;
    PSWM_WINDOW  pWindow;
} DC, *PDC;

#define  DC_LockDc(hDC)  \
  ((PDC) GDIOBJ_LockObj ((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_UnlockDc(pDC)  \
  GDIOBJ_UnlockObjByPtr ((PBASEOBJECT)pDC)

VOID APIENTRY RosGdiUpdateClipping(PDC pDC);


BOOL INTERNAL_CALL DC_Cleanup(PVOID ObjectBody);

#endif
