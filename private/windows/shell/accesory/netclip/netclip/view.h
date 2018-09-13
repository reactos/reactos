// NetClipView.h : interface of the CNetClipView class
//
/////////////////////////////////////////////////////////////////////////////

class CNetClipView : public CRichEditView
{
friend class CMainFrame;
protected: // create from serialization only
	CNetClipView();
	DECLARE_DYNCREATE(CNetClipView)

// Attributes
public:

// Operations
public:
    BOOL CanDisplay(UINT cf);

// Overrides
    virtual HRESULT QueryAcceptData(LPDATAOBJECT lpdataobj,
	CLIPFORMAT* lpcfFormat, DWORD /*dwReco*/, BOOL bReally, HGLOBAL hMetaPict);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetClipView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNetClipView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CNetClipView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUpdateNeedSel(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/*
#ifndef _DEBUG  // debug version in NetClipView.cpp
inline CNetClipDoc* CNetClipView::GetDocument()
   { return (CNetClipDoc*)m_pDocument; }
#endif
*/
/////////////////////////////////////////////////////////////////////////////
