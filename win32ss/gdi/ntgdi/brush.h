#pragma once

/* Internal interface */

#define NB_HATCH_STYLES  6

/*
 * The layout of this structure is taken from "Windows Graphics Programming"
 * book written by Feng Yuan.
 *
 * DON'T MODIFY THIS STRUCTURE UNLESS REALLY NEEDED AND EVEN THEN ASK ON
 * A MAILING LIST FIRST.
 */
typedef struct _BRUSH
{
    /* Header for all gdi objects in the handle table.
       Do not (re)move this. */
    BASEOBJECT BaseObject;

    ULONG iHatch;           // This is not the brush style, but the hatch style!
    HBITMAP hbmPattern;
    HBITMAP hbmClient;
    ULONG flAttrs;

    ULONG ulBrushUnique;
    BRUSH_ATTR *pBrushAttr; // Pointer to the currently active brush attribute
    BRUSH_ATTR BrushAttr;   // Internal brush attribute for global brushes
    POINT ptOrigin;
    ULONG bCacheGrabbed;
    COLORREF crBack;
    COLORREF crFore;
    ULONG ulPalTime;
    ULONG ulSurfTime;
    PVOID pvRBrush;
    HDEV hdev;
    //DWORD unk054;
    LONG lWidth;
    FLOAT eWidth;
    ULONG ulPenStyle;
    DWORD *pStyle;
    ULONG dwStyleCount;
    BYTE jJoin;             // 0x06c Join styles for geometric wide lines
    BYTE jEndCap;           //       end cap style for a geometric wide line
    //WORD unk06e;          // 0x06e
    INT iBrushStyle;        // 0x070
    //PREGION prgn;           // 0x074
    //DWORD unk078;         // 0x078
    DWORD unk07c;           // 0x07c
    LIST_ENTRY ListHead;    // 0x080
} BRUSH, *PBRUSH;

typedef struct _EBRUSHOBJ
{
    BRUSHOBJ    BrushObject;

    COLORREF    crRealize;
    ULONG       ulRGBColor;
    PVOID       pengbrush;
    ULONG       ulSurfPalTime;
    ULONG       ulDCPalTime;
    COLORREF    crCurrentText;
    COLORREF    crCurrentBack;
    COLORADJUSTMENT *pca;
//    DWORD       dwUnknown2c;
//    DWORD       dwUnknown30;
    SURFACE *   psurfTrg;
    struct _PALETTE *   ppalSurf;
    struct _PALETTE *   ppalDC;
    struct _PALETTE *   ppalDIB;
//    DWORD       dwUnknown44;
    BRUSH *     pbrush;
    FLONG       flattrs;
    DWORD       ulUnique;
//    DWORD       dwUnknown54;
//    DWORD       dwUnknown58;

    SURFOBJ *psoMask;
} EBRUSHOBJ, *PEBRUSHOBJ;

/* GDI Brush Attributes */
#define BR_NEED_FG_CLR      0x00000001
#define BR_NEED_BK_CLR      0x00000002 /* Background color is needed */
#define BR_DITHER_OK        0x00000004 /* Allow color dithering */
#define BR_IS_SOLID         0x00000010 /* Solid brush */
#define BR_IS_HATCH         0x00000020 /* Hatch brush */
#define BR_IS_BITMAP        0x00000040 /* DDB pattern brush */
#define BR_IS_DIB           0x00000080 /* DIB pattern brush */
#define BR_IS_NULL          0x00000100 /* Null/hollow brush */
#define BR_IS_GLOBAL        0x00000200 /* Stock objects */
#define BR_IS_PEN           0x00000400 /* Pen */
#define BR_IS_OLDSTYLEPEN   0x00000800 /* Geometric pen */
#define BR_IS_DIBPALCOLORS  0x00001000
#define BR_IS_DIBPALINDICE  0x00002000
#define BR_IS_DEFAULTSTYLE  0x00004000
#define BR_IS_MASKING       0x00008000 /* Pattern bitmap is used as transparent mask (?) */
#define BR_IS_INSIDEFRAME   0x00010000
#define BR_CACHED_ENGINE    0x00040000
#define BR_CACHED_IS_SOLID  0x80000000

#define  BRUSH_FreeBrush(pBrush) GDIOBJ_FreeObj((POBJ)pBrush, GDIObjType_BRUSH_TYPE)
#define  BRUSH_FreeBrushByHandle(hBrush) GDIOBJ_FreeObjByHandle((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH)
#define  BRUSH_ShareLockBrush(hBrush) ((PBRUSH)GDIOBJ_ShareLockObj((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH))
#define  BRUSH_ShareUnlockBrush(pBrush) GDIOBJ_vDereferenceObject((POBJ)pBrush)

INT
FASTCALL
BRUSH_GetObject(
    PBRUSH GdiObject,
    INT Count,
    LPLOGBRUSH Buffer);

VOID
NTAPI
BRUSH_vCleanup(
    PVOID ObjectBody);

extern HSURF gahsurfHatch[HS_DDI_MAX];

struct _SURFACE;
struct _PALETTE;
struct _DC;

INIT_FUNCTION
NTSTATUS
NTAPI
InitBrushImpl(VOID);

VOID
NTAPI
EBRUSHOBJ_vInit(EBRUSHOBJ *pebo, PBRUSH pbrush, struct _SURFACE *, COLORREF, COLORREF, struct _PALETTE *);

VOID
NTAPI
EBRUSHOBJ_vInitFromDC(EBRUSHOBJ *pebo, PBRUSH pbrush, struct _DC *);

VOID
FASTCALL
EBRUSHOBJ_vSetSolidRGBColor(EBRUSHOBJ *pebo, COLORREF crColor);

VOID
NTAPI
EBRUSHOBJ_vUpdateFromDC(EBRUSHOBJ *pebo, PBRUSH pbrush, struct _DC *);

BOOL
NTAPI
EBRUSHOBJ_bRealizeBrush(EBRUSHOBJ *pebo, BOOL bCallDriver);

VOID
NTAPI
EBRUSHOBJ_vCleanup(EBRUSHOBJ *pebo);

PVOID
NTAPI
EBRUSHOBJ_pvGetEngBrush(EBRUSHOBJ *pebo);

SURFOBJ*
NTAPI
EBRUSHOBJ_psoPattern(EBRUSHOBJ *pebo);

#define BRUSHOBJ_psoPattern(pbo) \
    EBRUSHOBJ_psoPattern(CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject))

SURFOBJ*
NTAPI
EBRUSHOBJ_psoMask(EBRUSHOBJ *pebo);

#define BRUSHOBJ_psoMask(pbo) \
    EBRUSHOBJ_psoMask(CONTAINING_RECORD(pbo, EBRUSHOBJ, BrushObject))

FORCEINLINE
ULONG
EBRUSHOBJ_iSetSolidColor(EBRUSHOBJ *pebo, ULONG iSolidColor)
{
    ULONG iOldColor = pebo->BrushObject.iSolidColor;
    pebo->BrushObject.iSolidColor = iSolidColor;
    return iOldColor;
}

BOOL FASTCALL IntGdiSetBrushOwner(PBRUSH,DWORD);
BOOL FASTCALL GreSetBrushOwner(HBRUSH,DWORD);

HBRUSH APIENTRY
IntGdiCreatePatternBrush(
   HBITMAP hBitmap);

HBRUSH APIENTRY
IntGdiCreateSolidBrush(
   COLORREF Color);

HBRUSH APIENTRY
IntGdiCreateNullBrush(VOID);

VOID FASTCALL
IntGdiSetSolidBrushColor(HBRUSH hBrush, COLORREF Color);
