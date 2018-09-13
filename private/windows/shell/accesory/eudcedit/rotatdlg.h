/**************************************************/
/*					                              */
/*					                              */
/*	Rotate bimap in Edit window	                  */
/*					                              */
/*					                              */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#define		NOTSEL		0
#define		FLIP_HOR	1
#define		FLIP_VER	2
#define		ROTATE_9	3
#define		ROTATE_18	4
#define		ROTATE_27	5

class CRotateDlg : public CDialog
{
public:
	CRotateDlg(CWnd* pParent = NULL);   // standard constructor

	//{{AFX_DATA(CRotateDlg)
	enum { IDD = IDD_ROTATECHAR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CRotateDlg)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

public:
	int	RadioItem;

protected:

	//{{AFX_MSG(CRotateDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnFliphor();
	afx_msg void OnFlipver();
	afx_msg void OnRotate180();
	afx_msg void OnRotate270();
	afx_msg void OnRotate90();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
