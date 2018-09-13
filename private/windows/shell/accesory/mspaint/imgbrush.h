      #ifndef __IMGBRUSH_H__
#define __IMGBRUSH_H__

class CImgBrush : public CObject
    {
    public: /******************************************************************/

    CImgBrush();
    virtual ~CImgBrush();


    BOOL CopyTo( CImgBrush& destImgBrush );

    CPalette* SetBrushPalette( CDC* pdc, BOOL bForce = FALSE );
    HPALETTE  SetBrushPalette( HDC  hdc, BOOL bForce = FALSE );

    BOOL SetSize   ( CSize newSize, BOOL bStretchToFit=TRUE );
    void ColorToMonoBitBlt(CDC* pdcMono, int xMono, int yMono, int cx, int cy,
        CDC *pdcColor, int xColor, int yColor, DWORD dwROP, COLORREF transparentColor);
    void RecalcMask( COLORREF transparentColor );

    void BltMatte  ( IMG* pimg, CPoint topLeft );
    void BltReplace( IMG* pimg, CPoint topLeft );
    void BltColor  ( IMG* pimg, CPoint topLeft, COLORREF color );

    void CenterHandle();
    void TopLeftHandle();

    CRgn      m_cRgnPolyFreeHandSelBorder;
    CRgn      m_cRgnPolyFreeHandSel;

    CDC       m_dc;
    CBitmap   m_bitmap;
    CSize     m_size;
    CDC       m_maskDC;
    CBitmap   m_maskBitmap;

    HBITMAP   m_hbmOld;
    HBITMAP   m_hbmMaskOld;

    BOOL      m_bFirstDrag;
    BOOL      m_bLastDragWasASmear;
    BOOL      m_bLastDragWasFirst;
    BOOL      m_bCuttingFromImage;
    BOOL      m_bMakingSelection;
    BOOL      m_bMoveSel;
    BOOL      m_bSmearSel;
    BOOL      m_bOpaque;

    CRect     m_rcDraggedFrom;
    CSize     m_dragOffset;

    IMG*      m_pImg;

    CRect     m_rcSelection;
    CSize     m_handle;
    };


extern CImgBrush NEAR theImgBrush;

void GetMonoBltColors(HDC hDC, HBITMAP hBM, COLORREF& crNewBk, COLORREF& crNewText);
BOOL QuickColorToMono(CDC* pdcMono, int xMono, int yMono, int cx, int cy,
	CDC *pdcColor, int xColor, int yColor, DWORD dwROP, COLORREF crTrans);

// #define DEBUGSHOWBITMAPS
#if defined(DEBUGSHOWBITMAPS)
void DebugShowBitmap(HDC hdcSrc, int x, int y, int wid, int hgt);
#else
#define DebugShowBitmap(hdc,x,y,w,h)
#endif

#endif // __IMGBRUSH_H__

