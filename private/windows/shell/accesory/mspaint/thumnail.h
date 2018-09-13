#ifndef __THUMNAIL_H__
#define __THUMNAIL_H__

/******************************************************************************/

class CThumbNailView : public CWnd
    {
    DECLARE_DYNAMIC(CThumbNailView)

    protected:

    class CImgWnd *m_pcImgWnd;

    // Generated message map functions
    //{{AFX_MSG(CThumbNailView)
    afx_msg void OnPaint();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnThumbnailThumbnail();
    afx_msg void OnUpdateThumbnailThumbnail(CCmdUI* pCmdUI);
    afx_msg void OnClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DrawTracker(CDC *pDC);

    public:

    CThumbNailView();
    CThumbNailView(CImgWnd *pcImgWnd);
    ~CThumbNailView();
    Create(DWORD dwStyle, CRect cRectWindow, CWnd *pcParentWnd);
    void DrawImage(CDC* pDC);
    void RefreshImage(void);
    CImgWnd* GetImgWnd(void);
    void UpdateThumbNailView();
    };

/******************************************************************************/

class CFloatThumbNailView : public CMiniFrmWnd
    {
    DECLARE_DYNAMIC(CFloatThumbNailView)

    protected:

    CThumbNailView *m_pcThumbNailView;

    // Generated message map functions
    //{{AFX_MSG(CFloatThumbNailView)
    afx_msg void OnClose();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
        //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    public:

    CPoint GetPosition() { return m_ptPosition; }
    CSize  GetSize()     { return m_szSize; }

    CFloatThumbNailView();
    CFloatThumbNailView(CImgWnd *pcImgWnd);
    ~CFloatThumbNailView();
    CThumbNailView* GetThumbNailView() { return m_pcThumbNailView; }

    virtual BOOL Create(CWnd* pParentWnd);
    virtual void PostNcDestroy();
    virtual WORD GetHelpOffset() { return ID_WND_GRAPHIC; }

    private:

    CPoint  m_ptPosition;
    CSize   m_szSize;
    };

/******************************************************************************/

class CFullScreenThumbNailView : public CFrameWnd
    {
    DECLARE_DYNAMIC(CFullScreenThumbNailView)

    private:
    LONG_PTR m_hOldIcon;
    protected:

    BOOL   m_bSaveShowFlag;
//  CBrush m_brBackground;

    CThumbNailView *m_pcThumbNailView;

    // Generated message map functions
    //{{AFX_MSG(CFullScreenThumbNailView)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnClose ();

        //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    public:

    CFullScreenThumbNailView();
    CFullScreenThumbNailView(CImgWnd *pcImgWnd);
    ~CFullScreenThumbNailView();
    virtual BOOL Create(LPCTSTR szCaption);
    };


#endif // __THUMNAIL_H__
