#ifndef __IMGWND_H__
#define __IMGWND_H__

#include "tracker.h"

// #define GRIDOPTIONS

class CBitmapObj;

// helper fns
BOOL IsUserEditingText();
BOOL TextToolProcessed( UINT nMessage );

// Mouse Tracking Information

typedef struct _mti
    {
    CPoint ptDown;
    CPoint ptPrev;
    CPoint pt;
    BOOL   fLeft;
    BOOL   fRight;
    BOOL   fCtrlDown;
    } MTI;

class CImgWnd;
class CThumbNailView;

// Image
struct IMG
    {
    class CImgWnd* m_pFirstImgWnd;

    CBitmapObj* m_pBitmapObj;

    BOOL bDirty;

    HDC hDC;
    HDC hMaskDC; // May be NULL (for normal bitmaps)

    // These are usually selected into hDC and hMaskDC respecively
    HBITMAP hBitmap;
    HBITMAP hBitmapOld;
    HBITMAP hMaskBitmap;
    HBITMAP hMaskBitmapOld;

    CPalette* m_pPalette;
    HPALETTE  m_hPalOld;

    int cxWidth;
    int cyHeight;
    int cPlanes;
    int cBitCount;
    int nResType;
    int m_nLastChanged;

    BOOL m_bTileGrid;
    int  m_cxTile;
    int  m_cyTile;
    };

// Image Editor Window

class CImgWnd : public CWnd
    {
    protected:

    static CImgWnd*         c_pImgWndCur;
    static CDragger*        c_pResizeDragger;
    static CTracker::STATE  c_dragState;

    DECLARE_DYNAMIC( CImgWnd )

    public:

     CImgWnd( IMG* pImg );
     CImgWnd( CImgWnd *pImgWnd );
    ~CImgWnd();

    BOOL Create( DWORD dwStyle, const RECT& rect,
                 CWnd* pParentWnd, UINT nID = 0 );

    BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );

    void ClientToImage(CPoint& point );
    void ClientToImage(CRect&  rect  );
    void ImageToClient(CPoint& point );
    void ImageToClient(CRect&  rect  );

    IMG* m_pImg;

    inline IMG*  GetImg() { return m_pImg; }
           void  SetImg( IMG* pImg );
    inline CSize GetScrollPos() { return CSize( m_xScroll, m_yScroll ); }
           void  SetScroll( int xScroll, int yScroll );
           void  ShowBrush( CPoint ptHandle );
           void  PrepareForBrushChange( BOOL bPickup = TRUE, BOOL bErase = TRUE );
    inline int   GetZoom()           { return m_nZoom; }
    inline int   GetPrevZoom() const { return m_nZoomPrev; }
           void  SetZoom( int nZoom );
           BOOL  MakeBrush( HDC hSourceDC, CRect rcSource );
           void  UpdPos(const CPoint& pt) { m_ptDispPos = pt; }
    inline BOOL  IsGridVisible() { return theApp.m_bShowGrid && m_nZoom > 3; }
           void  FinishUndo( const CRect& rect );
           void  RubberBandRect( HDC hDC, MTI* pmti, BOOL bErase );
           void  EraseTracker();
           void  CheckScrollBars();
           void  GetImageRect( CRect& rect );
       CPalette* SetImgPalette( CDC* pdc, BOOL bForce = FALSE );
       HPALETTE  SetImgPalette( HDC hdc, BOOL bForce = FALSE );
       CPalette* FixupDibPalette( LPSTR lpDib, CPalette* ppalDib );
           BOOL  IsSelectionAvailable( void );
           BOOL  IsPasteAvailable( void );
           CRect GetDrawingRect( void );

    static        void     SetToolCursor();
    static inline CImgWnd* GetCurrent() { return c_pImgWndCur; }
    BOOL                   PtInTracker(CPoint cptLocation);

    protected:

    void RubberMouse         (unsigned code, MTI* pmti);
    void OnRButtonDownInSel  (CPoint *pcPointDown);
    void ZoomedInDP          ( unsigned code, unsigned mouseKeys, CPoint newPt );
    void StartSelectionDrag  ( unsigned code, CPoint newPt );
    void CancelSelectionDrag ();
    void SelectionDragHandler( unsigned code, CPoint newPt );
    void ResizeMouseHandler  ( unsigned code, CPoint newPt );
    void EndResizeOperation  ();
    void MoveBrush           ( const CRect& newSelRect );
    void OnScroll            ( BOOL bVert, UINT nSBCode, UINT nPos );
    BOOL OnMouseDown         ( UINT nFlags );
    BOOL OnMouseMessage      ( UINT nFlags );
    void CancelPainting      ();

    afx_msg int  OnCreate       ( LPCREATESTRUCT lpCreateStruct );
#if 0
    afx_msg void OnDestroy      ();
#endif
    afx_msg void OnSetFocus     ( CWnd* pOldWnd );
    afx_msg void OnKillFocus    ( CWnd* pNewWnd );
    afx_msg void OnSize         ( UINT nType, int cx, int cy );
    afx_msg void OnLButtonDown  ( UINT nFlags, CPoint point );
    afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
    afx_msg void OnLButtonUp    ( UINT nFlags, CPoint point );
    afx_msg void OnRButtonDown  ( UINT nFlags, CPoint point );
    afx_msg void OnRButtonDblClk( UINT nFlags, CPoint point );
    afx_msg void OnRButtonUp    ( UINT nFlags, CPoint point );
    afx_msg void OnKeyDown      ( UINT nChar, UINT nRepCnt, UINT nFlags );
    afx_msg void OnKeyUp        ( UINT nChar, UINT nRepCnt, UINT nFlags );
    afx_msg void OnMouseMove    ( UINT nFlags, CPoint point );
    afx_msg void OnTimer        ( UINT nIDEvent );
    afx_msg void OnVScroll      ( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
    afx_msg void OnHScroll      ( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
    afx_msg void OnPaint        ();
    afx_msg void OnCancelMode   ();
    afx_msg void OnWindowPosChanging( WINDOWPOS FAR* lpwndpos );
    afx_msg void OnDestroyClipboard();
    afx_msg void OnPaletteChanged(CWnd *);
    afx_msg BOOL OnSetCursor        (CWnd *pWnd, UINT nHitTest, UINT message);
    afx_msg BOOL OnMouseWheel   (UINT nFlags, short zDelta, CPoint pt);
    void CmdSmallBrush();
    void CmdSmallerBrush();
    void CmdLargerBrush();
    void CmdClearImage();
    void CmdShowGrid();
#ifdef  GRIDOPTIONS
    void CmdGridOptions();
    void CmdShowTileGrid();
#endif  // GRIDOPTIONS
    void CmdInvMode();
    void CmdTransMode();

    void CmdCopy();
    void CmdCut();
    void CmdPaste();
    void CmdClear();

    void CmdInvertColors();
    void CmdTglOpaque();
    void CmdFlipBshH();
    void CmdFlipBshV();
    void CmdRot90();
    void CmdSkewBrush( int wAngle, BOOL bHorz );
    void CmdDoubleBsh();
    void CmdHalfBsh();
    void CmdSel2Bsh();

    void CmdExport();

    void CmdCancel();
    void CmdOK();

    void GetDrawRects(const CRect* pPaintRect, const CRect* pReqDestRect,
        CRect& srcRect, CRect& destRect);

    void DrawGrid(CDC* pDC, const CRect& srcRect, CRect& destRect);
    void DrawBackground(CDC* pDC, const CRect* pPaintRect = NULL);
    void DrawTracker(CDC* pDC = NULL, const CRect* pPaintRect = NULL);
    void DrawImage(CDC* pDC, const CRect* pPaintRect,
                                   CRect* pDestRect = NULL, BOOL bDoGrid = TRUE);
    void SetThumbnailView( CThumbNailView* pwndNewThumbnailView )
                   { m_pwndThumbNailView = pwndNewThumbnailView; }

    BOOL PasteImageClip();
    BOOL PasteImageFile( LPSTR lpDib );
    HBITMAP CopyDC( CDC* pImgDC, CRect* prcClip );
    void CopyBMAndPal(HBITMAP *pBM, CPalette ** ppPal);

    CImgWnd* m_pNextImgWnd; // next viewer link

    int      m_nZoom;
    int      m_nZoomPrev;

    int      m_xScroll;
    int      m_yScroll;
    int      m_LineX;     // this is 1/32 of the bitmap height
    int      m_LineY;     // this is 1/32 of the bitmap width

    CPoint   m_ptDispPos;

    WORD     m_wClipboardFormat;

    HGLOBAL  m_hPoints;

    CThumbNailView* m_pwndThumbNailView;

    DECLARE_MESSAGE_MAP()

    friend class CPBFrame;
    friend class CPBView;
    friend class CBitmapObj;
    friend class CSelectTool;
    friend class CTextTool;
    friend class CCurveTool;
    friend class CTedit;
    friend class CAttrEdit;
    friend class CImgToolWnd; // for key message forwarding
    friend class CImgColorsWnd;
    friend class CCursorIconToolWnd;
    friend BOOL  SetImgSize(IMG*, CSize, BOOL);
    friend void  SetDrawColor(COLORREF);
    friend void  SetEraseColor(COLORREF);
    friend void  SetTransColor(COLORREF);
    friend void  FreeImg(IMG* pImg);
    friend void  AddImgWnd(IMG*, CImgWnd*);
    friend void  InvalImgRect(IMG* pImg, CRect* prc);
    friend void  CommitSelection(BOOL);

    private:
    short m_WheelDelta;
    };

BOOL FillBitmapObj(CImgWnd* pImgWnd, CBitmapObj* pResObject, IMG* pImgStruct,
        int iColor = -1);

extern CImgWnd*  g_pMouseImgWnd;
extern CImgWnd*  g_pDragBrushWnd;

extern CRect   rcDragBrush;

#endif // __IMGWND_H__
