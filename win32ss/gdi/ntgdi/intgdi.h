#pragma once

/* Convert WIN32 ROP into an ENG ROP */
#define WIN32_ROP3_TO_ENG_ROP4(dwRop4) ((((dwRop4) & 0x00FF0000) >> 16) | (((dwRop4) & 0x00FF0000) >> 8))
#define WIN32_ROP4_TO_ENG_ROP4(dwRop4) ((dwRop4) >> 16)

#define WIN32_ROP4_USES_SOURCE(Rop)  ((((Rop) & 0xCCCC0000) >> 2) != ((Rop) & 0x33330000))

/* The range of valid ROP2 values is 1 .. 16 */
#define FIXUP_ROP2(rop2) ((((rop2) - 1) & 0xF) + 1)

/* Brush functions */

extern HDC hSystemBM;
extern HSEMAPHORE hsemDriverMgmt;

/* Line functions */

BOOL FASTCALL
IntGdiLineTo(DC  *dc,
             int XEnd,
             int YEnd);

BOOL FASTCALL
IntGdiMoveToEx(DC      *dc,
               int     X,
               int     Y,
               LPPOINT Point);

BOOL FASTCALL
IntGdiPolyBezier(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL FASTCALL
IntGdiPolyline(DC      *dc,
               LPPOINT pt,
               int     Count);

BOOL FASTCALL
IntGdiPolyBezierTo(DC      *dc,
                   LPPOINT pt,
                   DWORD   Count);

BOOL FASTCALL
IntGdiPolyPolyline(DC      *dc,
                   LPPOINT pt,
                   PULONG PolyPoints,
                   DWORD   Count);

BOOL FASTCALL
IntGdiPolylineTo(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL FASTCALL
GreMoveTo( HDC hdc,
           INT x,
           INT y,
           LPPOINT pptOut);

/* Shape functions */

BOOL
NTAPI
GreGradientFill(
    HDC hdc,
    PTRIVERTEX pVertex,
    ULONG nVertex,
    PVOID pMesh,
    ULONG nMesh,
    ULONG ulMode);

/* DC functions */

HDC FASTCALL
IntGdiCreateDC(PUNICODE_STRING Driver,
               PUNICODE_STRING Device,
               PVOID pUMdhpdev,
               CONST PDEVMODEW InitData,
               BOOL CreateAsIC);

/* Stock objects */

VOID FASTCALL
IntSetSysColors(UINT nColors, CONST INT *Elements, CONST COLORREF *Colors);

HGDIOBJ FASTCALL
IntGetSysColorBrush(INT Object);

DWORD FASTCALL
IntGetSysColor(INT nIndex);

/* Other Stuff */

NTSTATUS
APIENTRY
NtGdiFlushUserBatch(
    VOID);

DWORD
APIENTRY
NtDxEngGetRedirectionBitmap(
    DWORD Unknown0);

HBITMAP
FASTCALL
IntCreateCompatibleBitmap(
    _In_ PDC Dc,
    _In_ INT Width,
    _In_ INT Height,
    _In_ UINT Bpp,
    _In_ UINT Planes);

WORD APIENTRY IntGdiSetHookFlags(HDC hDC, WORD Flags);

UINT APIENTRY IntSetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, CONST RGBQUAD *Colors);

UINT APIENTRY IntGetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, RGBQUAD *Colors);

UINT APIENTRY
IntGetPaletteEntries(HPALETTE  hpal,
                     UINT  StartIndex,
                     UINT  Entries,
                     LPPALETTEENTRY  pe);

UINT APIENTRY
IntGetSystemPaletteEntries(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           LPPALETTEENTRY  pe);

VOID  FASTCALL CreateStockObjects (VOID);
VOID  FASTCALL CreateSysColorObjects (VOID);

PPOINT GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);

BOOL FASTCALL IntFillArc( PDC dc, INT XLeft, INT YLeft, INT Width, INT Height, double StartArc, double EndArc, ARCTYPE arctype);
BOOL FASTCALL IntDrawArc( PDC dc, INT XLeft, INT YLeft, INT Width, INT Height, double StartArc, double EndArc, ARCTYPE arctype, PBRUSH pbrush);

BOOL FASTCALL IntFillEllipse( PDC dc, INT XLeft, INT YLeft, INT Width, INT Height, PBRUSH pbrush);
BOOL FASTCALL IntDrawEllipse( PDC dc, INT XLeft, INT YLeft, INT Width, INT Height, PBRUSH pbrush);
BOOL FASTCALL IntFillRoundRect( PDC dc, INT Left, INT Top, INT Right, INT Bottom, INT Wellipse, INT Hellipse, PBRUSH pbrush);
BOOL FASTCALL IntDrawRoundRect( PDC dc, INT Left, INT Top, INT Right, INT Bottom, INT Wellipse, INT Hellipse, PBRUSH pbrush);
