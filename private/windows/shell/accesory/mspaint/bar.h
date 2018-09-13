/******************************************************************************/
/* Bar.H:   Defines the Interface to the CStatBar (Status Bar) CLASS          */
/*                                                                            */
/******************************************************************************/

#ifndef __BAR_H__
#define __BAR_H__

//below size does not include bitmap width
#define SIZE_POS_PANE_WIDTH 12    // Bitmap Width + 1 char separator + 5 digits + 1 char separator + 5 digits

// The 2 below defines were needed, since we had to duplicate the DrawStatusText
// method from the barcore.cpp file in the msvc\mfc\src directory
#define CX_BORDER 1   // from auxdata.h in the msvc\mfc\src directory
#define CY_BORDER 1   // from auxdata.h in the msvc\mfc\src directory

/******************************************************************************/

class CStatBar : public CStatusBar
    {
    DECLARE_DYNAMIC( CStatBar )

private:

    CBitmap m_posBitmap;
    CBitmap m_sizeBitmap;
    CString m_cstringSizeSeparator;
    CString m_cstringPosSeparator;
    int     m_iBitmapWidth;
    int     m_iBitmapHeight;
    int     m_iSizeY;

protected:

    virtual void DoPaint(CDC* pDC);
    virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);

    static  void PASCAL DrawStatusText( HDC hDC, CRect const& rect,
                                            LPCTSTR lpszText, UINT nStyle,
                                            int iIndentText = 0);

    afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSizeParent(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnNcDestroy( void );
        afx_msg void    OnSysColorChange( void );

public:

    CStatBar();
    ~CStatBar();

    BOOL Create(CWnd* pParentWnd);

    BOOL SetText(LPCTSTR sz);

    BOOL SetPosition(const CPoint& pos);
    BOOL SetSize(const CSize& size);

    BOOL ClearPosition();
    BOOL ClearSize();

    BOOL Reset();

    DECLARE_MESSAGE_MAP()
    };

extern CStatBar *g_pStatBarWnd;

/******************************************************************************/
// NON-OBJECT Status bar API, Uses global object pointer to the StatBar object

void ShowStatusBar                ( BOOL bShow = TRUE );
BOOL IsStatusBarVisible           ();
void InvalidateStatusBar          ( BOOL bErase = FALSE );
void ClearStatusBarSize           ();
void ClearStatusBarPosition       ();
void ClearStatusBarPositionAndSize();
void ResetStatusBar               ();
void SetStatusBarPosition         ( const CPoint& pos );
void SetStatusBarSize             ( const CSize& size );
void SetStatusBarPositionAndSize  ( const CRect& rect );
void SetPrompt                    ( LPCTSTR, BOOL bRedrawNow = FALSE );
void SetPrompt                    ( UINT, BOOL bRedrawNow = FALSE );

/******************************************************************************/

#endif // __BAR_H__
