// ServerInfoDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServerInfoDlg dialog

class CServerInfoDlg : public CDialog
{
// Construction
public:
	CServerInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServerInfoDlg)
	enum { IDD = IDD_SERVERINFO };
	CString	m_strMachine;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServerInfoDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
