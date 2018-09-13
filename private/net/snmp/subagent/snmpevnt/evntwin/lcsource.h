#ifndef _lcsource_h
#define _lcsource_h

#define CX_DEFAULT_DESCRIPTION_WIDTH 100
#define CX_DESCRIPTION_SLOP 25


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLcSource window

class CXMessageArray;
class CXEventSource;
class CXMessage;

class CLcSource : public CListCtrl
{
// Construction
public:
	CLcSource();
	SCODE CreateWindowEpilogue();

// Attributes
public:


// Operations
public:
	BOOL Find(CString sText, BOOL bWholeWord, BOOL bMatchCase);
	SCODE SetEventSource(CXEventSource* pEventSource);
	void SortItems(DWORD dwColumn);
	LONG FindItem(DWORD dwMessageId);
	void RefreshItem(LONG iItem);
	CXMessage* operator[](LONG iItem) {return GetAt(iItem); }
	CXMessage* GetAt(LONG iItem);
	LONG GetSize() {return GetItemCount(); }
    void GetSelectedMessages(CXMessageArray& amsg);
    void NotifyTrappingChange(DWORD dwMessageId, BOOL bIsTrapping);
    LONG SetDescriptionWidth();

// Overrides

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLcSource)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLcSource();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLcSource)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	friend class CSource;
	CSource* m_pSource;

	void SetColumnHeadings();
	SCODE GetMessageLibPath(CString& sLog, CString& sEventSource, CString& sLibPath);
	void AddMessage(CXMessage* pMessage);
    void SetDescriptionWidth(CXMessageArray& aMessages);
	
};

enum {ICOL_LcSource_EVENTID = 0, 
	  ICOL_LcSource_SEVERITY,
	  ICOL_LcSource_TRAPPING,
	  ICOL_LcSource_DESCRIPTION,
	  ICOL_LcSource_MAX	  
	  };


#endif //_lcsource_h
