/**************************************************/
/*					                              */
/*					                              */
/*	Reference other characters	                  */
/*		(Dialog)		                          */
/*					                              */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include "refrlist.h"
#include "eudclist.h"

class CRefrDlg : public CDialog
{

public:
	CRefrDlg(CWnd* pParent = NULL);

	//{{AFX_DATA(CRefrDlg)
	enum { IDD = IDD_REFERENCE };
	//}}AFX_DATA

private:
	void 	SetViewFont();
	void 	JumpReferCode();
	void 	AdjustFontName();

private:
	int	vHeight;
	TCHAR	FocusCode[5];
	TCHAR	FocusChar[3];
	TCHAR	FontName[LF_FACESIZE];
	CRefListFrame	m_RefListFrame1;
	CRefInfoFrame	m_RefInfoFrame;
	CColumnHeading	m_ColumnHeadingR;
	CEdit       m_EditChar;

public:
	CRefrList	m_CodeList;

protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	//{{AFX_MSG(CRefrDlg)
	afx_msg void OnClickedButtomfont();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnChangeEditcode();
	afx_msg void OnSetfocusEditcode();
	afx_msg void OnKillfocusEditcode();
	afx_msg void OnSetfocusEditchar();
	afx_msg void OnKillfocusEditchar();
	afx_msg void OnChangeEditchar();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
