#ifndef __WIN32K_BRUSHOBJ_H
#define __WIN32K_BRUSHOBJ_H

/* GDI Brush Attributes */
#define GDIBRUSH_NEED_FG_CLR        0x0001
#define GDIBRUSH_NEED_BK_CLR        0x0002 /* Background color is needed */
#define GDIBRUSH_DITHER_OK          0x0004 /* Allow color dithering */
#define GDIBRUSH_IS_SOLID           0x0010 /* Solid brush */
#define GDIBRUSH_IS_HATCH           0x0020 /* Hatch brush */
#define GDIBRUSH_IS_BITMAP          0x0040 /* DDB pattern brush */
#define GDIBRUSH_IS_DIB             0x0080 /* DIB pattern brush */
#define GDIBRUSH_IS_NULL            0x0100 /* Null/hollow brush */
#define GDIBRUSH_IS_GLOBAL          0x0200 /* Stock objects */
#define GDIBRUSH_IS_PEN             0x0400 /* Pen */
#define GDIBRUSH_IS_OLDSTYLEPEN     0x0800 /* Geometric pen */
#define GDIBRUSH_IS_DIBPALCOLORS    0x1000
#define GDIBRUSH_IS_DIBPALINDICE    0x2000
#define GDIBRUSH_IS_DEFAULTSTYLE    0x4000
#define GDIBRUSH_IS_MASKING         0x8000 /* Pattern bitmap is used as transparent mask (?) */
#define GDIBRUSH_IS_INSIDEFRAME     0x00010000
#define GDIBRUSH_CACHED_ENGINE      0x00040000
#define GDIBRUSH_CACHED_IS_SOLID	0x80000000

/* BRUSHGDI is a handleless GDI object */
typedef struct _BRUSHGDI
{
    BRUSHOBJ BrushObj;

    ULONG ulStyle;
    ULONG flAttrs;
    POINT ptPenWidth;
    ULONG ulPenStyle;
    DWORD *pStyle;
    ULONG dwStyleCount;
    HBITMAP hbmPattern;
    XLATEOBJ *XlateObject;
} BRUSHGDI, *PBRUSHGDI;

PBRUSHGDI NTAPI
GreCreatePen(
   DWORD dwPenStyle,
   DWORD dwWidth,
   IN ULONG ulBrushStyle,
   IN ULONG ulColor,
   IN ULONG_PTR ulClientHatch,
   IN ULONG_PTR ulHatch,
   DWORD dwStyleCount,
   PULONG pStyle,
   IN ULONG cjDIB,
   IN BOOL bOldStylePen);

PBRUSHGDI NTAPI
GreCreateSolidBrush(COLORREF crColor);

PBRUSHGDI NTAPI
GreCreatePatternBrush(HBITMAP hbmPattern);

PBRUSHGDI NTAPI
GreCreateNullBrush();

VOID NTAPI
GreFreeBrush(PBRUSHGDI pBrush);

#endif
