// wordpdoc.h : interface of the CWordPadDoc class
//
// Copyright (C) 1992-1999 Microsoft Corporation
// All rights reserved.

class CFormatBar;
class CWordPadSrvrItem;
class CWordPadView;

class CWordPadDoc : public CRichEdit2Doc
{
protected: // create from serialization only
	CWordPadDoc();
	DECLARE_DYNCREATE(CWordPadDoc)

// Attributes
public:
	int     m_nDocType;
	int     m_nNewDocType;
    LPTSTR  m_short_filename;

	void SetDocType(int nDocType, BOOL bNoOptionChange = FALSE);
	CWordPadView* GetView();
	CLSID GetClassID();
	LPCTSTR GetSection();

// Operations
public:
	void SaveState(int nType);
	void RestoreState(int nType);
	virtual CFile* GetFile(LPCTSTR pszPathName, UINT nOpenFlags, 
		CFileException* pException);
	virtual BOOL DoSave(LPCTSTR pszPathName, BOOL bReplace = TRUE);
	int MapType(int nType);
	void ForceDelayed(CFrameWnd* pFrameWnd);

// Overrides
	virtual CRichEdit2CntrItem* CreateClientItem(REOBJECT* preo) const;
	virtual void OnDeactivateUI(BOOL bUndoable);
	virtual void Serialize(CArchive& ar);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWordPadDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	        BOOL OnOpenDocument2(LPCTSTR lpszPathName, bool defaultToText = true);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void OnCloseDocument();
	virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	protected:
	virtual COleServerItem* OnGetEmbeddedItem();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual void PreCloseFrame(CFrameWnd* pFrameArg);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CWordPadDoc)
	afx_msg void OnViewOptions();
	afx_msg void OnUpdateOleVerbPopup(CCmdUI* pCmdUI);
	afx_msg void OnFileSendMail();
	afx_msg void OnUpdateIfEmbedded(CCmdUI* pCmdUI);
	afx_msg void OnEditLinks();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
