// computer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// computer dialog

class computer : public CDialog
{
// Construction
public:
	computer(CWnd* pParent = NULL);   // standard constructor

	LPCTSTR GetNewComputerName()
	{
	   return m_strComputerName.IsEmpty() ?	NULL : (LPCTSTR)m_strComputerName;

	}

// Dialog Data
	//{{AFX_DATA(computer)
	enum { IDD = IDD_NEW_COMPUTER };
	CString	m_strComputerName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(computer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(computer)
	afx_msg void OnCancel();
	afx_msg void OnOk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
