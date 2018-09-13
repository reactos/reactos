// printres.h : interface of the Cprintres class
//

#define MARGINS_UNITS 2540 // Store hundredths of MM
#define MARGINS_DEFAULT (MARGINS_UNITS * 3/4) // 3/4 inch default margins

class CImgWnd;

/***************************************************************************/

class CPrintResObj : public CObject
    {
    DECLARE_DYNAMIC( CPrintResObj )

    public:

    CPrintResObj( CPBView* pView, CPrintInfo* pInfo );

    void BeginPrinting( CDC* pDC, CPrintInfo* pInfo );
    void PrepareDC    ( CDC* pDC, CPrintInfo* pInfo );
    BOOL PrintPage    ( CDC* pDC, CPrintInfo* pInfo );
    void EndPrinting  ( CDC* pDC, CPrintInfo* pInfo );

    // Attributes

    CPBView*  m_pView;
    LPVOID    m_pDIB;
    LPVOID    m_pDIBits;
    DWORD     m_dwPicWidth;
    DWORD     m_dwPicHeight;
    DWORD     m_dwPrtWidth;
    DWORD     m_dwPrtHeight;
    int       m_iPageWidthinScreenPels;
    int       m_iPageHeightinScreenPels;
    int       m_iZoom;
    int       m_iPagesWide;
    int       m_iPagesHigh;
    int       m_iWidthinPels;
    int       m_iHeightinPels;
    CPalette* m_pDIBpalette;
    CSize     m_cSizeScroll;
    CRect     m_rtMargins;
    };

/***************************************************************************/
