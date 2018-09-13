#ifndef __IMGSUPRT_H__
#define __IMGSUPRT_H__

#define MAX_PALETTE_COLORS 256

#define WM_CANCEL WM_USER + 0

// Brush Shapes
#define roundBrush          0
#define squareBrush         1
#define slantedLeftBrush    2
#define slantedRightBrush   3


// Combine Modes
#define combineColor        0
#define combineMatte        1
#define combineReplace      2


// Non-standard Raster Ops
#define DSx                 0x00660046L
#define DSa                 0x008800C6L
#define DSna                0x00220326L
#define DSPao               0x00ea02e9L
#define DSo                 0x00ee0086L
#define DSno                0x00bb0226L
#define DSPDxax             0x00e20746L
#define SPxn                0x00c3006aL

// Indices of the screen and inverse colors in color table.
#define IDScreen    -1
#define IDInvScreen -2

// NOTE: These structures mirror the LOGPALETTE structure in WINDOWS.H

struct LOGPALETTE16
    {
    WORD         palVersion;
    WORD         palNumEntries;
    PALETTEENTRY palPalEntry[16];
    };


struct LOGPALETTE256
    {
    WORD         palVersion;
    WORD         palNumEntries;
    PALETTEENTRY palPalEntry[MAX_PALETTE_COLORS];
    };

// Note: this bogus structure is not my fault!  It's stolen from
// the old Windows SDK ImagEdit program...
#pragma pack(1)

struct COLORFILEHEADER
    {
    char  tag; // this is a 'C'
    short colors;
    char  reserved[47]; // fill with 0
    // DWORD rgrgb [colors];
    };

#pragma pack()

/*************************************************************************/

//extern int FileTypeFromExtension( const TCHAR FAR* lpcExt );

void InitCustomData();          // see customiz.cpp
void CustomExit();

class CImgWnd;

// IMGSUPRT.CPP

extern IMG*     CreateImg          (int cxWidth, int cyHeight,
                                    int cPlanes, int cBitCount, BOOL bPalette = TRUE );
extern void     SelectImg          (IMG* pImg);
extern BOOL     ClearImg           (IMG* pImg);
extern void     FreeImg            (IMG* pImg);
extern void     DirtyImg           (IMG* pImg);
extern void     AddImgWnd          (IMG* pImg, CImgWnd* pImgWnd);
extern BOOL     UpdateCurIcoImg    (IMG* pImg);
extern BOOL     ChangeICBackground (IMG* pImg, COLORREF rgbNewScr);
extern BOOL     SetImgSize         (IMG* pImg, CSize newSize, BOOL bStretch);
extern void     GetImgSize         (IMG* pImg, CSize& size);
extern BOOL     ReplaceImgPalette  (IMG* pImg, LPLOGPALETTE lpLogPal);
extern int      AddNewColor        (IMG* pImg,  COLORREF crNew );

// IMGED.CPP
extern void     Draw3dRect         (HDC hDC, RECT* prc);

// DRAW.CPP
extern void     InvalImgRect       (IMG* pImg, CRect* prc);
extern void     CommitImgRect      (IMG* pImg, CRect* prc);
extern void     FixRect            (RECT* prc);
extern void     StandardiseCoords  (CPoint* s, CPoint* e);
extern void     DrawBrush          (IMG* pImg, CPoint pt, BOOL bDraw);
extern void     HideBrush          ();
extern void     SetCombineMode     (int wNewCombineMode);
extern BOOL     SetupRubber        (IMG* pImg);
extern void     PolyTo             (CDC* pDC, CPoint fromPt,
                                              CPoint toPt, CSize size);
extern BOOL     GetTanPt           (CSize size, CPoint delta, CRect& tan);

extern void     SetDrawColor       (COLORREF cr);
extern void     SetEraseColor      (COLORREF cr);
extern void     SetTransColor      (COLORREF cr);
extern void     SetDrawColor       (int iColor);
extern void     SetEraseColor      (int iColor);
extern void     SetTransColor      (int iColor);
extern void     InvalColorWnd      ();
extern BOOL     SetUndo            (IMG* pImg);
extern void     SetLeftColor       (int nColor);
extern void     SetRightColor      (int nColor);
extern void     CommitSelection    (BOOL bSetUndo);
extern void     PickupSelection    ();

extern BOOL     EnsureUndoSize     (IMG* pimg);

extern void     CleanupImages   ();
extern void     CleanupImgRubber();
extern void     CleanupImgUndo  ();

extern IMG*      pImgCur;

#define TRANS_COLOR_NONE 0x87654321 // undefined

extern BOOL      fDraggingBrush;
extern BOOL      g_bCustomBrush;
extern BOOL      g_bDriverCanStretch;
extern BOOL      g_bUseTrans;

extern int       theLeft;
extern int       theRight;
extern int       theTrans;
extern COLORREF  crLeft;
extern COLORREF  crRight;
extern COLORREF  crTrans;
extern int       wCombineMode;

struct DINFO
    {
    TCHAR* m_szDesc;
    UINT  m_nColors;
    SIZE  m_size;
    };

extern COLORREF  std2Colors[];

extern CPalette* GetStd256Palette();
extern CPalette* GetStd16Palette();
extern CPalette* GetStd2Palette();
extern CPalette* PaletteFromDS(HDC hdc);

#define IS_WIN30_DIB(lpbi) ((*(LPDWORD)(lpbi)) >= sizeof (BITMAPINFOHEADER))

extern WORD      DIBNumColors      ( LPSTR lpbi, BOOL bJustUsed=TRUE );
extern DWORD     DIBWidth          ( LPSTR lpDIB );
extern DWORD     DIBHeight         ( LPSTR lpDIB );
extern CPalette* CreateDIBPalette  ( LPSTR lpbi );
extern HBITMAP   DIBToBitmap       ( LPSTR lpDIBHdr, CPalette* pPal, HDC hdc = NULL );
extern HBITMAP   DIBToDS           ( LPSTR lpDIBHdr, DWORD dwOffBits, HDC hdc );
extern LPSTR     DibFromBitmap     ( HBITMAP hBitmap, DWORD dwStyle, WORD wBits,
                                     CPalette* pPal, HBITMAP hMaskBitmap, DWORD& dwSize );
extern LPSTR     FindDIBBits       ( LPSTR lpbi, DWORD dwOffBits = 0 );
extern WORD      PaletteSize       ( LPSTR lpbi );
extern void      FreeDib           ( LPSTR lpDib );
extern CPalette* CreatePalette     ( const COLORREF* colors, int nColors );
extern CPalette* MergePalettes     ( CPalette *pPal1, CPalette *pPal2, int& iAdds );
extern void      AdjustPointForGrid( CPoint *ptPointLocation );

// drawing support functions
extern void      StretchCopy (HDC, int, int, int, int, HDC, int, int, int, int);
extern void      FillImgRect (HDC hDC, CRect * prc, COLORREF cr );
extern void      BrushLine   (CDC* pDC, CPoint fromPt, CPoint toPt,
                             int nWidth, int nShape);
extern void      DrawDCLine  (HDC hDC, CPoint pt1, CPoint pt2,
                              COLORREF color, int nWidth, int nShape,
                              CRect& rc);
extern void      DrawImgLine (IMG* pimg, CPoint pt1, CPoint pt2,
                              COLORREF color, int nWidth, int nShape,
                              BOOL bCommit);
extern void      Mylipse     (HDC hDC, int x1, int y1, int x2, int y2, BOOL bFilled);

// PATSTENCIL:
// This is a ternary raster operation, listed in the SDK ref as "PSDPxax",
// but I prefer to think of it as "DSaPSnao".  More practically, this rop
// is useful to apply the color of the current brush to only those pixels
// where the source bitmap is zero (black).  That is, a binary bitmap as the
// source will be drawn onto the destination in the color of the current
// brush.
//
#define PATSTENCIL 0xB8074AL

// DrawBitmap:
// This draws a bitmap on a display context with a given raster operation.
//   CDC* dc         The target display context.
//   CBitmap* bmSrc  The bitmap to be drawn.
//   CRect* rc       A position rectangle.
//                   If NULL, bitmap drawn with the upper-left at 0, 0.
//                   Otherwise, bitmap drawn centered in this rectangle.
//                   Result is not clipped to the rectangle.
//   DWORD dwROP     Raster operation.  See table 11.3 of the Win30 SDK ref.
//   CDC* memdc      A memory context for the BitBlt process to use.
//                   If NULL, DrawBitmap creates and destroys its own.
//
void DrawBitmap(CDC* dc, CBitmap* bmSrc, CRect* rc,
                DWORD dwROP = SRCCOPY, CDC* memdc = NULL);

extern HDC       hRubberDC;
extern HBITMAP   hRubberBM;
extern HBITMAP   g_hUndoImgBitmap;
extern HPALETTE  g_hUndoPalette;
extern int       cxRubberWidth;
extern int       cyRubberHeight;
extern IMG*      pRubberImg;
extern WORD      gwClipboardFormat;
extern CBrush    g_brSelectHorz;
extern CBrush    g_brSelectVert;

CPalette *PBSelectPalette(CDC *pDC, CPalette *pPalette, BOOL bForceBk);

class CTempBitmap : public CBitmap
{
public:
        ~CTempBitmap() { DeleteObject(); } // DeleteObject checks for NULL
} ;

#endif // __IMGSUPRT_H__
