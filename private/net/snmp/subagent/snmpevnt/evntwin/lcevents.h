#ifndef _lcevents_h
#define _lcevents_h

class CXEventLogArray;
class CXMessageArray;
class CXEventArray;
class CXEvent;
class CSource;
class CLcEvents;



/////////////////////////////////////////////////////////////////////////////
// CLcEvents window
class CEvent;
class CLcEvents : public CListCtrl
{
// Construction
public:
	CLcEvents();
	SCODE CreateWindowEpilogue();


// Attributes
public:

// Operations
public:
    LONG GetSize() {return GetItemCount(); }
    CXEvent* GetAt(LONG iEvent);
    void RemoveAll();
    void RemoveAt(int nIndex, int nCount = 1);
    void AddEvents(CSource& source, CXEventArray& aEvents);
    void AddEvents(CSource& source, CXEventLogArray& aEventLogs);
    void DeleteSelectedEvents(CSource& source);
    void DeleteAt(LONG iEvent);
    BOOL HasSelection() {return GetNextItem(-1, LVNI_SELECTED) != -1; }
    void GetSelectedEvents(CXEventArray& aEvents);
    void RefreshEvents(CXEventArray& aEvents);
    void SetItem(LONG nItem, CXEvent* pEvent);
    LONG FindEvent(CXEvent* pEvent);
    void SortItems(DWORD dwColumn);
    void SelectEvents(CXEventArray& aEvents);

//    BOOL GetItem(LV_ITEM* pItem) const;


// Overrides

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLcEvents)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLcEvents();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLcEvents)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
    void UpdateDescriptionWidth();
    LONG AddEvent(CXEvent* pEvent);
    void CreateEventsNotTrapped(CXMessageArray& amsg, CXEventArray& aevents);
	void SetColumnHeadings();
    DWORD m_dwSortColumn;
    LONG m_cxWidestMessage;

};

enum {ICOL_LcEvents_LOG = 0, 
	  ICOL_LcEvents_SOURCE,
	  ICOL_LcEvents_ID,
	  ICOL_LcEvents_SEVERITY,
      ICOL_LcEvents_COUNT,
      ICOL_LcEvents_TIME,
      ICOL_LcEvents_DESCRIPTION,
	  ICOL_LcEvents_MAX	  
	  };





#endif //_lcevents_h

/////////////////////////////////////////////////////////////////////////////
