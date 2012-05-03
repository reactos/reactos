#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

/* Constants ******************************************************************/

/* Get/SetBounds/Rect support. */
#define DCB_WINDOWMGR 0x8000 /* Queries the Windows bounding rectangle instead of the application's */

/* flFontState */
#define DC_DIRTYFONT_XFORM 1
#define DC_DIRTYFONT_LFONT 2
#define DC_UFI_MAPPING 4

/* fl */
#define DC_FL_PAL_BACK 1

#define DC_DISPLAY  1
#define DC_DIRECT 2
#define DC_CANCELED 4
#define DC_PERMANANT 0x08
#define DC_DIRTY_RAO 0x10
#define DC_ACCUM_WMGR 0x20
#define DC_ACCUM_APP 0x40
#define DC_RESET 0x80
#define DC_SYNCHRONIZEACCESS 0x100
#define DC_EPSPRINTINGESCAPE 0x200
#define DC_TEMPINFODC 0x400
#define DC_FULLSCREEN 0x800
#define DC_IN_CLONEPDEV 0x1000
#define DC_REDIRECTION 0x2000
#define DC_SHAREACCESS 0x4000

typedef enum
{
  DCTYPE_DIRECT = 0,
  DCTYPE_MEMORY = 1,
  DCTYPE_INFO = 2,
} DCTYPE;


/* Type definitions ***********************************************************/

typedef struct _ROS_DC_INFO
{
  HRGN     hClipRgn;     /* Clip region (may be 0) */
  HRGN     hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */

  CLIPOBJ     *CombinedClip;
} ROS_DC_INFO;

typedef struct _DCLEVEL
{
  HPALETTE          hpal;
  struct _PALETTE  * ppal;
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
  PREGION           prgnClip;
  PREGION           prgnMeta;
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
  PREGION     prgnAPI;
  PREGION     prgnVis; /* Visible region (must never be 0) */
  PREGION     prgnRao;
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
} DC;

extern PDC defaultDCstate;

/* Internal functions *********************************************************/

/* dcobjs.c */

VOID FASTCALL DC_vUpdateFillBrush(PDC pdc);
VOID FASTCALL DC_vUpdateLineBrush(PDC pdc);
VOID FASTCALL DC_vUpdateTextBrush(PDC pdc);
VOID FASTCALL DC_vUpdateBackgroundBrush(PDC pdc);

HPALETTE
NTAPI
GdiSelectPalette(
    HDC hDC,
    HPALETTE hpal,
    BOOL ForceBackground);



INIT_FUNCTION NTSTATUS NTAPI InitDcImpl(VOID);
PPDEVOBJ FASTCALL IntEnumHDev(VOID);
PDC NTAPI DC_AllocDcWithHandle(VOID);
BOOL NTAPI DC_bAllocDcAttr(PDC pdc);
BOOL NTAPI DC_Cleanup(PVOID ObjectBody);
BOOL FASTCALL IntGdiDeleteDC(HDC, BOOL);

BOOL FASTCALL DC_InvertXform(const XFORM *xformSrc, XFORM *xformDest);
VOID FASTCALL DC_vUpdateViewportExt(PDC pdc);
VOID FASTCALL DC_vCopyState(PDC pdcSrc, PDC pdcDst, BOOL to);
VOID FASTCALL DC_vFinishBlit(PDC pdc1, PDC pdc2);
VOID FASTCALL DC_vPrepareDCsForBlit(PDC pdc1, RECT rc1, PDC pdc2, RECT rc2);

VOID NTAPI DC_vRestoreDC(IN PDC pdc, INT iSaveLevel);

VOID FASTCALL DCU_SetDcUndeletable(HDC);
VOID NTAPI DC_vFreeDcAttr(PDC pdc);
VOID NTAPI DC_vInitDc(PDC pdc, DCTYPE dctype, PPDEVOBJ ppdev);

COLORREF FASTCALL IntGdiSetBkColor (HDC hDC, COLORREF Color);
INT FASTCALL IntGdiSetBkMode(HDC  hDC, INT  backgroundMode);
COLORREF FASTCALL  IntGdiSetTextColor(HDC hDC, COLORREF color);
UINT FASTCALL IntGdiSetTextAlign(HDC  hDC, UINT  Mode);
VOID FASTCALL IntGdiReferencePdev(PPDEVOBJ pPDev);
VOID FASTCALL IntGdiUnreferencePdev(PPDEVOBJ pPDev, DWORD CleanUpType);
HDC FASTCALL IntGdiCreateDisplayDC(HDEV hDev, ULONG DcType, BOOL EmptyDC);
BOOL FASTCALL IntGdiCleanDC(HDC hDC);
VOID FASTCALL IntvGetDeviceCaps(PPDEVOBJ, PDEVCAPS);
BOOL FASTCALL IntSetDefaultRegion(PDC);
BOOL NTAPI GreSetDCOwner(HDC hdc, ULONG ulOwner);

VOID
NTAPI
DC_vSetBrushOrigin(PDC pdc, LONG x, LONG y);

FORCEINLINE
PDC
DC_LockDc(HDC hdc)
{
    //if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_DC_TYPE) return NULL; ???
    return GDIOBJ_LockObject(hdc, GDIObjType_DC_TYPE);
}

FORCEINLINE
VOID
DC_UnlockDc(PDC pdc)
{
    GDIOBJ_vUnlockObject(&pdc->BaseObject);
}

VOID
FORCEINLINE
DC_vSelectSurface(PDC pdc, PSURFACE psurfNew)
{
    PSURFACE psurfOld = pdc->dclevel.pSurface;
    if (psurfOld)
    {
        psurfOld->hdc = NULL;
        SURFACE_ShareUnlockSurface(psurfOld);
    }
    if (psurfNew)
        GDIOBJ_vReferenceObjectByPointer((POBJ)psurfNew);
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
        GDIOBJ_vReferenceObjectByPointer((POBJ)pbrFill);
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
        GDIOBJ_vReferenceObjectByPointer((POBJ)pbrLine);
    pdc->dclevel.pbrLine = pbrLine;
}

VOID
FORCEINLINE
DC_vSelectPalette(PDC pdc, PPALETTE ppal)
{
    PPALETTE ppalOld = pdc->dclevel.ppal;
    if (ppalOld)
        PALETTE_ShareUnlockPalette(ppalOld);
    if (ppal)
        GDIOBJ_vReferenceObjectByPointer((POBJ)ppal);
    pdc->dclevel.ppal = ppal;
}

extern PBRUSH pbrDefaultBrush ;

#endif /* not __WIN32K_DC_H */
