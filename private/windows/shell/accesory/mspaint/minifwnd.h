// minifwnd.h : Declares the interface to the CMiniFrmWnd class.
//

#ifndef __MINIFWND_H__
#define __MINIFWND_H__

/////////////////////////////////////////////////////////////////////////////

class CDocking;

class CMiniFrmWnd : public CFrameWnd
    {
    DECLARE_DYNAMIC( CMiniFrmWnd )

    public:

    CMiniFrmWnd();

    BOOL Create( const TCHAR FAR* lpWindowName, DWORD dwStyle,
                 const RECT& rect, CWnd* pParentWnd );

    afx_msg void    OnNcLButtonDown( UINT nHitTest, CPoint pt );
    afx_msg void    OnLButtonUp    ( UINT nFlags, CPoint pt );
    afx_msg int     OnCreate       ( LPCREATESTRUCT lpCreateStruct);
    afx_msg void    OnSysCommand   ( UINT nID, LONG lParam);
    afx_msg LRESULT OnHelpHitTest  ( WPARAM wParam, LPARAM lParam);
    afx_msg void    OnRButtonDown  ( UINT nFlags, CPoint point);
    afx_msg void    OnKeyDown      ( UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg LRESULT OnMoving       ( WPARAM fwSide, LPARAM lprc );
    afx_msg void    OnMove         ( int x, int y );
    afx_msg BOOL    OnNcActivate   ( BOOL bActive );

    virtual BOOL OnCommand( UINT wParam, LONG lParam );

    virtual WORD GetHelpOffset() = 0;       // All of our minifwnds need help.

    DECLARE_MESSAGE_MAP()

    protected:

    CDocking*   m_pDocking;
    CPBView::DOCKERS m_Dockable;

    void CancelDrag();
    };

/////////////////////////////////////////////////////////////////////////////

#endif // __MINIFWND_H__

