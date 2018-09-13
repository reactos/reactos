#ifndef _tcsource_h
#define _tcsource_h


/////////////////////////////////////////////////////////////////////////////
// CTcSource window

class CXEventSource;

class CTcSource : public CTreeCtrl
{
// Construction
public:
	CTcSource();
	SCODE CreateWindowEpilogue();
	CXEventSource* GetSelectedEventSource();
	void SelChanged() { m_pSource->NotifyTcSelChanged(); }
	BOOL Find(CString& sText, BOOL bWholeWord, BOOL bMatchCase);

// Attributes
public:
	

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTcSource)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTcSource();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTcSource)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	void LoadImageList();
	SCODE LoadTreeFromRegistry();
	CImageList m_ImageList;	

	friend class CSource;
	CSource* m_pSource;
};

#endif //_tcsource_h
