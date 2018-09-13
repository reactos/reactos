// ListDevDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CListDevDlg dialog

class CListDevDlg : public CDialog
{
// Construction
public:
	CListDevDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CListDevDlg)
	enum { IDD = IDD_LISTDEV_DIALOG };
	CListBox	m_lbDevData;
	CTreeCtrl	m_DevTree;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListDevDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CListDevDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeComputername();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
    void InitializeSubtree(HTREEITEM htiParent, CDevice* pDevice);
    void InitializeDeviceTree(LPCTSTR ComputerName);
    void InitializeDeviceData(CDevice* pDevice);
    CDeviceTree     m_DeviceTree;
    CImageList	    m_ImageList;
};
