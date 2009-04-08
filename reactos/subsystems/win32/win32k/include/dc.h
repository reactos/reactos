#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

#include "brush.h"
#include "bitmaps.h"
#include "pdevobj.h"

/* Constants ******************************************************************/

/* Get/SetBounds/Rect support. */
#define DCB_WINDOWMGR 0x8000 /* Queries the Windows bounding rectangle instead of the application's */

/* Type definitions ***********************************************************/

typedef struct _ROS_DC_INFO
{
  HRGN     hClipRgn;     /* Clip region (may be 0) */
  HRGN     hVisRgn;      /* Should me to DC. Visible region (must never be 0) */
  HRGN     hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
  HBITMAP  hBitmap;

  BYTE   bitsPerPixel;

  CLIPOBJ     *CombinedClip;
  XLATEOBJ    *XlateBrush;
  XLATEOBJ    *XlatePen;

  UNICODE_STRING    DriverName;

} ROS_DC_INFO;

/* EXtended CLip and Window Region Object */
typedef struct _XCLIPOBJ
{
  WNDOBJ  eClipWnd;
  PVOID   pClipRgn;    /* prgnRao_ or (prgnVis_ if (prgnRao_ == z)) */
  DWORD   Unknown1[16];
  DWORD   nComplexity; /* count/mode based on # of rect in regions scan. */
  PVOID   pUnknown;    /* UnK pointer to a large drawing structure. */
                       /* We will use it for CombinedClip ptr. */
} XCLIPOBJ, *PXCLIPOBJ;

typedef struct _DCLEVEL
{
  HPALETTE          hpal;
  struct _PALGDI  * ppal;
  PVOID             pColorSpace; /* COLORSPACE* */
  LONG              lIcmMode;
  LONG              lSaveDepth;
  DWORD             unk1_00000000;
  HGDIOBJ           hdcSave;
  POINTL            ptlBrushOrigin;
  PBRUSH            pbrFill;
  PBRUSH            pbrLine;
  PVOID             plfnt; /* LFONTOBJ* (TEXTOBJ*) */
  HGDIOBJ           hPath; /* HPATH */
  FLONG             flPath;
  LINEATTRS         laPath; /* 0x20 bytes */
  PVOID             prgnClip; /* PROSRGNDATA */
  PVOID             prgnMeta;
  COLORADJUSTMENT   ca;
  FLONG             flFontState;
  UNIVERSAL_FONT_ID ufi;
  UNIVERSAL_FONT_ID ufiLoc[4]; /* Local List. */
  UNIVERSAL_FONT_ID *pUFI;
  ULONG             uNumUFIs;
  BOOL              ufiSet;
  FLONG             fl;
  FLONG             flBrush;
  MATRIX            mxWorldToDevice;
  MATRIX            mxDeviceToWorld;
  MATRIX            mxWorldToPage;
  FLOATOBJ          efM11PtoD;
  FLOATOBJ          efM22PtoD;
  FLOATOBJ          efDxPtoD;
  FLOATOBJ          efDyPtoD;
  FLOATOBJ          efM11_TWIPS;
  FLOATOBJ          efM22_TWIPS;
  FLOATOBJ          efPr11;
  FLOATOBJ          efPr22;
  PSURFACE          pSurface;
  SIZE              sizl;
} DCLEVEL, *PDCLEVEL;

/* The DC object structure */
typedef struct _DC
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
  BASEOBJECT  BaseObject;

  DHPDEV      dhpdev;   /* <- PDEVOBJ.hPDev DHPDEV for device. */
  INT         dctype;
  INT         fs;
  PPDEVOBJ    ppdev;
  PVOID       hsem;   /* PERESOURCE aka HSEMAPHORE */
  FLONG       flGraphicsCaps;
  FLONG       flGraphicsCaps2;
  PDC_ATTR    pdcattr;
  DCLEVEL     dclevel;
  DC_ATTR     dcattr;
  HDC         hdcNext;
  HDC         hdcPrev;
  RECTL       erclClip;
  POINTL      ptlDCOrig;
  RECTL       erclWindow;
  RECTL       erclBounds;
  RECTL       erclBoundsApp;
  PVOID       prgnAPI; /* PROSRGNDATA */
  PVOID       prgnVis;
  PVOID       prgnRao;
  POINTL      ptlFillOrigin;
  EBRUSHOBJ   eboFill;
  EBRUSHOBJ   eboLine;
  EBRUSHOBJ   eboText;
  EBRUSHOBJ   eboBackground;
  HFONT       hlfntCur;
  FLONG       flSimulationFlags;
  LONG        lEscapement;
  PVOID       prfnt;    /* RFONT* */
  XCLIPOBJ    co;       /* CLIPOBJ */
  PVOID       pPFFList; /* PPFF* */
  PVOID       pClrxFormLnk;
  INT         ipfdDevMax;
  ULONG       ulCopyCount;
  PVOID       pSurfInfo;
  POINTL      ptlDoBanding;

  /* Reactos specific members */
  ROS_DC_INFO rosdc;
} DC, *PDC;

/* Internal functions *********************************************************/

#define  DC_LockDc(hDC)  \
  ((PDC) GDIOBJ_LockObj ((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_UnlockDc(pDC)  \
  GDIOBJ_UnlockObjByPtr ((POBJ)pDC)

extern PDC defaultDCstate;

NTSTATUS FASTCALL InitDcImpl(VOID);
PPDEVOBJ FASTCALL IntEnumHDev(VOID);
HDC  FASTCALL DC_AllocDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_InitDC(HDC  DCToInit);
HDC  FASTCALL DC_FindOpenDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_FreeDC(HDC);
VOID FASTCALL DC_AllocateDcAttr(HDC);
VOID FASTCALL DC_FreeDcAttr(HDC);
BOOL INTERNAL_CALL DC_Cleanup(PVOID ObjectBody);
BOOL FASTCALL DC_SetOwnership(HDC DC, PEPROCESS Owner);
VOID FASTCALL DC_LockDisplay(HDC);
VOID FASTCALL DC_UnlockDisplay(HDC);
BOOL FASTCALL IntGdiDeleteDC(HDC, BOOL);

VOID FASTCALL DC_UpdateXforms(PDC  dc);
BOOL FASTCALL DC_InvertXform(const XFORM *xformSrc, XFORM *xformDest);
VOID FASTCALL DC_vUpdateViewportExt(PDC pdc);
VOID FASTCALL DC_vCopyState(PDC pdcSrc, PDC pdcDst);
VOID FASTCALL DC_vUpdateFillBrush(PDC pdc);
VOID FASTCALL DC_vUpdateLineBrush(PDC pdc);
VOID FASTCALL DC_vUpdateTextBrush(PDC pdc);
VOID FASTCALL DC_vUpdateBackgroundBrush(PDC pdc);

BOOL FASTCALL DCU_SyncDcAttrtoUser(PDC);
BOOL FASTCALL DCU_SynchDcAttrtoUser(HDC);
VOID FASTCALL DCU_SetDcUndeletable(HDC);

COLORREF FASTCALL IntGdiSetBkColor (HDC hDC, COLORREF Color);
INT FASTCALL IntGdiSetBkMode(HDC  hDC, INT  backgroundMode);
COLORREF APIENTRY  IntGdiGetBkColor(HDC  hDC);
INT APIENTRY  IntGdiGetBkMode(HDC  hDC);
COLORREF FASTCALL  IntGdiSetTextColor(HDC hDC, COLORREF color);
UINT FASTCALL IntGdiSetTextAlign(HDC  hDC, UINT  Mode);
UINT APIENTRY  IntGdiGetTextAlign(HDC  hDC);
COLORREF APIENTRY  IntGdiGetTextColor(HDC  hDC);
INT APIENTRY  IntGdiSetStretchBltMode(HDC  hDC, INT  stretchBltMode);
VOID FASTCALL IntGdiReferencePdev(PPDEVOBJ pPDev);
VOID FASTCALL IntGdiUnreferencePdev(PPDEVOBJ pPDev, DWORD CleanUpType);
HDC FASTCALL IntGdiCreateDisplayDC(HDEV hDev, ULONG DcType, BOOL EmptyDC);
BOOL FASTCALL IntGdiCleanDC(HDC hDC);
VOID FASTCALL IntvGetDeviceCaps(PPDEVOBJ, PDEVCAPS);
INT FASTCALL IntGdiGetDeviceCaps(PDC,INT);

extern PPDEVOBJ pPrimarySurface;

VOID
FORCEINLINE
DC_vSelectSurface(PDC pdc, PSURFACE psurfNew)
{
    PSURFACE psurfOld = pdc->dclevel.pSurface;
    if (psurfOld)
        SURFACE_ShareUnlockSurface(psurfOld);
    if (psurfNew)
        GDIOBJ_IncrementShareCount((POBJ)psurfNew);
    pdc->dclevel.pSurface = psurfNew;
}

VOID
FORCEINLINE
DC_vSelectFillBrush(PDC pdc, PBRUSH pbrFill)
{
    PBRUSH pbrFillOld = pdc->dclevel.pbrFill;
    if (pbrFillOld)
        BRUSH_ShareUnlockBrush(pbrFillOld);
    if (pbrFill)
        GDIOBJ_IncrementShareCount((POBJ)pbrFill);
    pdc->dclevel.pbrFill = pbrFill;
}

VOID
FORCEINLINE
DC_vSelectLineBrush(PDC pdc, PBRUSH pbrLine)
{
    PBRUSH pbrLineOld = pdc->dclevel.pbrLine;
    if (pbrLineOld)
        BRUSH_ShareUnlockBrush(pbrLineOld);
    if (pbrLine)
        GDIOBJ_IncrementShareCount((POBJ)pbrLine);
    pdc->dclevel.pbrLine = pbrLine;
}

BOOL FASTCALL
IntPrepareDriverIfNeeded();
extern PDEVOBJ PrimarySurface;

#endif /* not __WIN32K_DC_H */
