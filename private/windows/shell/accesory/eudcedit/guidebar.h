/**************************************************/
/*					                              */
/*					                              */
/*		GuideBar		                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#ifndef __AFXEXT_H_
#include <afxext.h>
#endif

class CGuideBar : public CStatusBar
{
	DECLARE_DYNAMIC(CGuideBar)
public:
	CGuideBar();
	BOOL 	Create( CWnd* pOwnerWnd, UINT pID);
//	void 	RecalcLayout();
	virtual ~CGuideBar();
	CComboBox m_comboCharset;
	void	PositionStatusPane();

protected:


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
//	void 	DrawConcaveRect( CDC *pDC, int x, int y, CSize size);


protected:
	UINT    m_nColumns;
	CWnd*   m_pOwnerWnd;

private:
	BOOL    m_comboBoxAdded;

protected:
	//{{AFX_MSG(CGuideBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
