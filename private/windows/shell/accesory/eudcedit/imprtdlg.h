/**************************************************/
/*					                              */
/*	Import Bimmap(Windows 3.1)	                  */ 
/*		(Dialogbox)		                          */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

class CImportDlg : public CDialog
{
public:
	CImportDlg(CWnd* pParent = NULL);   // standard constructor

	//{{AFX_DATA(CImportDlg)
	enum { IDD = IDD_IMPORT };
	//}}AFX_DATA
private:
	char	UserFontTitle[MAX_PATH];
	char	EUDCFontTitle[MAX_PATH];

	//{{AFX_VIRTUAL(CImportDlg)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CImportDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnFileBrowse();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
