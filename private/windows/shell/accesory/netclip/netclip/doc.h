// NetClipDoc.h : interface of the CNetClipDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CNetClipDoc : public CRichEditDoc
{
protected: // create from serialization only
	CNetClipDoc();
	DECLARE_DYNCREATE(CNetClipDoc)

// Attributes
public:

// Operations
public:

// Overrides
	virtual CRichEditCntrItem* CreateClientItem(REOBJECT* preo) const;
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetClipDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void DeleteContents();
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNetClipDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CNetClipDoc)
	afx_msg void OnFileSaveAs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

friend class CNetClipApp;
};

/////////////////////////////////////////////////////////////////////////////
