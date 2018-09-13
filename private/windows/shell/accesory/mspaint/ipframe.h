// ipframe.h : interface of the CInPlaceFrame class
//

class CInPlaceFrame : public COleIPFrameWnd
    {
    DECLARE_DYNCREATE(CInPlaceFrame)

    public:

    CInPlaceFrame();

    // Attributes
    public:

    // Operations
    public:

    // Implementation
    public:

    virtual ~CInPlaceFrame();

	virtual CWnd* GetMessageBar();
    virtual BOOL OnCreateControlBars(CFrameWnd* pWndFrame, CFrameWnd* pWndDoc);
    virtual void RepositionFrame( LPCRECT lpPosRect, LPCRECT lpClipRect );

    #ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
    #endif

    protected:

    CStatBar    	m_statBar;
    CImgToolWnd 	m_toolBar;
	CImgColorsWnd 	m_colorBar;

    COleResizeBar	m_wndResizeBar;
    COleDropTarget	m_dropTarget;

    // Generated message map functions
    protected:

    //{{AFX_MSG(CInPlaceFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSysColorChange();
	afx_msg void OnClose();
	afx_msg void OnHelpIndex();
	//}}AFX_MSG

	afx_msg LRESULT OnContextMenu(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
    };

/***************************************************************************/
