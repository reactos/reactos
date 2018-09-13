// docking.h : interface of the CDocking class
//

class CDocking : public CObject
    {
    DECLARE_DYNAMIC( CDocking )

    // Constructors
    public:     /***********************************************************/

    CDocking();

    // Attributes

    BOOL    Create( CPoint ptDrop, CRect& rectCurrent, BOOL bDocked, CPBView::DOCKERS tool );
    void    Move  ( CPoint ptNew );
    BOOL    Move  ( CPoint ptNew, CRect& rectFrame );
    BOOL    Clear ( CRect* prectLast = NULL );

    protected:  /***********************************************************/

    BOOL    DrawFocusRect();

    int     m_iDockingX;
    int     m_iDockingY;
    BOOL    m_bStarted;
    BOOL    m_bDocked;
    BOOL    m_bDocking;
    CRect   m_rectDockingPort;
    CRect   m_rectDocked;
    CRect   m_rectFree;
    CPoint  m_ptLast;
    CPoint  m_ptDocking;

    CPBView::DOCKERS m_Tool;
    };

/***************************************************************************/
