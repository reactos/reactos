#ifndef __IMGCOLOR_H__
#define __IMGCOLOR_H__

/******************************************************************************/

class CImgWnd;

#ifdef CUSTOMFLOAT
class CDocking;
#endif

/******************************************************************************/

class CImgColorsWnd : public CControlBar
    {
    public:

   
    CImgColorsWnd();

    enum HitZone
        {
        none       = -2,
        curColor   = -1,
        firstColor =  0
        };

    int         m_nDisplayColorsInitial;
    int         m_nDisplayColors;
    int         m_nOffsetY;
    int         m_nCols;
    int         m_nRows;
#ifdef CUSTOMFLOAT
    CDocking*   m_pDocking;
#endif
    CRect       m_rectColors;

    BOOL        Create( const TCHAR* pWindowName, DWORD dwStyle, CWnd* pParentWnd );

    void        InvalidateCurColors();

    WORD        GetHelpOffset();

    HitZone     HitTest(const CPoint& point);
    BOOL        GetHitRect(HitZone hitZone, CRect& rect);

    void        PaintCurColorBox(CDC* pDC, BOOL bRight);
    void        PaintCurColors(CDC* pDC, const CRect* pPaintRect);
    void        PaintColors(CDC* pDC, const CRect* pPaintRect);

    void        CancelDrag();

    virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
    virtual void  OnUpdateCmdUI( CFrameWnd* pTarget, BOOL bDisableIfNoHndler );

    //{{AFX_MSG(CImgColorsWnd)
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
        afx_msg void OnClose();
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
    };

/******************************************************************************/

#ifdef CUSTOMFLOAT

class CFloatImgColorsWnd : public CMiniFrmWnd
    {
    DECLARE_DYNAMIC(CFloatImgColorsWnd)

    public:

    virtual ~CFloatImgColorsWnd(void);
    virtual BOOL Create(const TCHAR* pWindowName, DWORD dwStyle,
                        const RECT& rect, CWnd* pParentWnd);
    virtual WORD GetHelpOffset() { return ID_WND_GRAPHIC; }
    afx_msg void OnClose();

    DECLARE_MESSAGE_MAP()

    };
#endif

extern void InvalColorCache();

extern CImgColorsWnd* NEAR g_pImgColorsWnd;

/***************************************************************************/

#endif // __IMGCOLOR_H__
