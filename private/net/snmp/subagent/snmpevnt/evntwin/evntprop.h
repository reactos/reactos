// evntprop.h : header file
//

#ifndef EVNTPROP_H
#define EVNTPROP_H 

class CXEventArray;
class CXEvent;



/////////////////////////////////////////////////////////////////////////////
// CEditField window

class CEditField : public CEdit
{
// Construction
public:
	CEditField();
    SCODE CEditField::GetValue(int& iValue);

// Attributes
public:

// Operations
public:
    BOOL IsDirty() {return m_bIsDirty; }
    void ClearDirty() {m_bIsDirty = FALSE; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditField)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditField();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditField)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
    BOOL m_bIsDirty;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CEditSpin window

class CEditSpin : public CSpinButtonCtrl
{
// Construction
public:
	CEditSpin();

// Attributes
public:
    int SetPos(int iPos);
    void SetRange(int nLower, int nUpper);
    BOOL IsDirty();
    void ClearDirty() {m_bIsDirty = FALSE; }

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditSpin)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditSpin();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEditSpin)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private: 
    BOOL m_bIsDirty;
    int m_iSetPos;
};




/////////////////////////////////////////////////////////////////////////////
// CEventPropertiesDlg dialog

class CEventPropertiesDlg : public CDialog
{
// Construction
public:
	CEventPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
    BOOL EditEventProperties(CXEventArray& aEvents);

// Dialog Data
	//{{AFX_DATA(CEventPropertiesDlg)
	enum { IDD = IDD_PROPERTIESDLG };
	CButton	m_btnWithinTime;
	CEditSpin	m_spinEventCount;
	CEditSpin	m_spinTimeInterval;
	CEditField	m_edtTimeInterval;
	CEditField	m_edtEventCount;
	CButton	m_btnOK;
	CString	m_sDescription;
	CString	m_sSource;
	CString	m_sEventId;
	CString	m_sLog;
	CString	m_sSourceOID;
	CString	m_sFullEventID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventPropertiesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEventPropertiesDlg)
	virtual void OnOK();
	afx_msg void OnWithintime();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO*);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void MakeLabelsBold();
    CXEvent* m_pEvent;
    BOOL m_bDidEditEventCount;
    BOOL m_bDidFlipEventCount;
    
    int m_iEventCount;
    int m_iTimeInterval;
};

#endif // EVNTPROP_H
/////////////////////////////////////////////////////////////////////////////
