#ifndef _evntfind_h
#define _evntfind_h

// evntfind.h : header file
//
class CSource;

enum FOUND_WHERE {
    I_FOUND_NOTHING,
    I_FOUND_IN_TREE,
    I_FOUND_IN_LIST
};

/////////////////////////////////////////////////////////////////////////////
// CEventFindDlg dialog
class CEventFindDlg : public CDialog
{
// Construction
public:
	CEventFindDlg(CWnd* pParent = NULL);   // standard constructor
    BOOL Create(CSource* pSource, UINT nIDTemplate, CWnd* pParentWnd=NULL);

    ~CEventFindDlg();
    FOUND_WHERE Find(CSource* pSource);

// Dialog Data
    FOUND_WHERE m_iFoundWhere;

	//{{AFX_DATA(CEventFindDlg)
	enum { IDD = IDD_EVENTFINDDLG };
	CString	m_sFindWhat;
	BOOL	m_bMatchWholeWord;
	BOOL	m_bMatchCase;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventFindDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEventFindDlg)
	afx_msg void OnCheckMatchWholeword();
	afx_msg void OnCheckMatchCase();
	afx_msg void OnChangeEditFindWhat();
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioSearchDescriptions();
	afx_msg void OnRadioSearchSources();
	afx_msg BOOL OnHelpInfo(HELPINFO*);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnOK();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    CSource* m_pSource;
    BOOL m_bSearchInTree;
//    BOOL m_bMatchCase;
//    BOOL m_bWholeWord;
};


#endif //_evntfind_h
