/**************************************************/
/*						                          */
/*						                          */
/*		Update EUDC Dialog		                  */
/*						                          */
/*						                          */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include "eudclist.h"

class CEudcDlg : public CDialog
{
public:
	CEudcDlg(CWnd* pParent = NULL);

	//{{AFX_DATA(CEudcDlg)
	enum { IDD = IDD_CUSTOMCODE };
	//}}AFX_DATA

protected:
	void SetViewCustomFont();

public:
	CEudcList	m_EditList;
	CViewEdit	m_ViewChar;
#ifdef BUILD_ON_WINNT
    CColumnHeading  m_ColumnHeading;
#endif //BUILD_ON_WINNT
CCustomListFrame	m_ListFrame;
CCustomInfoFrame	m_Frame;
CCustomInfoFrame	m_InfoFrame2;
CCustomInfoFrame	m_InfoFrame3;
	CComboBox	m_ComboRange;
	CStatic		m_StaticRange;
	BOOL		WorRFlag;

private:
	void	AdjustFileName();
	void	AdjustFontName();
	void	RegistRegFont();
	TCHAR	FileName[MAX_PATH];
	TCHAR	FontName[LF_FACESIZE];

protected:
	virtual BOOL OnInitDialog();
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	//{{AFX_MSG(CEudcDlg)
	virtual void OnOK();
	afx_msg void OnSelchangeChinarange();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
