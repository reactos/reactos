// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
class CNetClipView;

// The main frame supports OLE D&D to/from the clipboard; CDropTarget
// implements our support.
//
class CDropTarget : public COleDropTarget
{
public:
	CDropTarget();
	virtual ~CDropTarget();

    virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
		DROPEFFECT dropEffect, CPoint point);
};

class CMainFrame : public CFrameWnd
{
public:
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)
    BOOL SavePosition(LPCTSTR szName) ;
    BOOL RestorePosition(LPCTSTR szName, int nCmdShow) ;
    LPTSTR GetNameOfClipboardFormat(CLIPFORMAT cf);

// Attributes
protected:
public:
                        // name of machine we're connected to
	CString             m_strMachine;
                        // remote clipboard
	IClipboard*         m_pClipboard;
                        // OLE D&D support
    CDropTarget         m_dropTarget;
                        // Helper pointer to our view class
    CNetClipView*       m_pNetClipView;
                        // Display embeddings as icon
    BOOL                m_fDisplayAsIcon;

                        // Currently selected clipformat or 0 for auto
    UINT                m_cfDisplay;
                        // Array of formats currently on clipboard
    UINT                m_rgFormats[ID_DISPLAY_LAST-ID_DISPLAY_FIRST];
                        // For clipboard chaining
    HWND                m_hwndNextCB;

                        // Connection point connection cookie
    DWORD               m_dwConnectionCookie;
                        // Connection point for IClipboardNotify
    IConnectionPoint*   m_pConnectionPt;

                        // prevent multiple refreshes.
	BOOL                m_fRefreshPosted;

    // Operations
public:
	HRESULT Connect(CString &strMachine);
	HRESULT Disconnect();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnDisplayAuto();
	afx_msg void OnUpdateDisplayAuto(CCmdUI* pCmdUI);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnDrawClipboard();
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnEditClear();
	afx_msg void OnUpdateEditClear(CCmdUI* pCmdUI);
	afx_msg void OnConnectConnect();
	afx_msg void OnUpdateConnectConnect(CCmdUI* pCmdUI);
	afx_msg void OnConnectDisconnect();
	afx_msg void OnUpdateConnectDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
    afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
#ifndef _NTBUILD
#ifdef _DEBUG
	afx_msg void OnViewDataObject();
	afx_msg void OnUpdateViewDataObject(CCmdUI* pCmdUI);
#endif
#endif
    afx_msg void OnOtherFormat( UINT nID );
	afx_msg void OnUpdateOtherFormat(CCmdUI* pCmdUI);
    afx_msg LRESULT OnRefresh(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

	DECLARE_INTERFACE_MAP()

    BEGIN_INTERFACE_PART(ClipboardNotify, IClipboardNotify)
        virtual HRESULT __stdcall OnClipboardChanged(void);
    END_INTERFACE_PART(ClipboardNotify)

friend class CNetClipApp;
};

/////////////////////////////////////////////////////////////////////////////
