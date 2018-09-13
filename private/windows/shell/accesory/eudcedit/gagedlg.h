/**************************************************/
/*					                              */
/*					                              */
/*	Gage when import bitmap		                  */
/*		(Dialog)		                          */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

class CEditGage : public CEdit
{
public:
	CEditGage();
	~CEditGage();

protected:
	//{{AFX_MSG(CEditGage)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CGageDlg : public CDialog
{
public:
	CGageDlg(CWnd* pParent = NULL, 
			 LPTSTR szUserFont=NULL, 
			 PTSTR szBmpFile=NULL, 
			 LPTSTR szTtfFile=NULL,
			 BOOL bIsWin95EUDC = FALSE);   // standard constructor

	//{{AFX_DATA(CGageDlg)
	enum { IDD = IDD_GAGE };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CGageDlg)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL
private:
	CEditGage	m_EditGage;
	TCHAR m_szUserFont[MAX_PATH];
	TCHAR m_szTtfFile[MAX_PATH];
	TCHAR m_szBmpFile[MAX_PATH];
	BOOL m_bIsWin95EUDC;

protected:

	// Generated message map functions
	//{{AFX_MSG(CGageDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
