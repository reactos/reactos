//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#ifndef __TABSPLITTERWND_H__
#define __TABSPLITTERWND_H__

// This splitter bar is persistent and will restore the previous
// position and will also set the focus back to the previous pane
// after the bar has been respositioned
class CTRL_EXT_CLASS CTabSplitterWnd : public CSplitterWnd 
{
	DECLARE_DYNAMIC(CTabSplitterWnd)
public:
	CTabSplitterWnd();

	// for registry
	void SetSection(LPCTSTR szSection);
// for workspace
	void SetPropertyItem(CRuntimeClass *pPropertyItemClass);
	void SetSize(int nCur,int nMin);
	void Apply();
	BOOL IsEmpty();
	virtual void SaveSize();	
	virtual CWnd *GetActiveWnd();
public:
// Overrides	
	virtual void Serialize(CArchive& ar);
	virtual BOOL CreateView( int row, int col, CRuntimeClass* pViewClass, SIZE sizeInit, CCreateContext* pContext );
	virtual void ActivateNext(BOOL bPrev);
protected:
	virtual void StopTracking(BOOL bAccept);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
// Generated message map functions
protected:
	//{{AFX_MSG(CTabSplitterWnd)
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	LRESULT OnAppSplitterMouseWheel(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()
protected:
	CString m_strSection;
	int m_nCurRow;
	int m_nCurCol;
	int m_cxCur;
	int m_cyCur;
	int m_cxMin;
	int m_cyMin;
private:
	static LPCTSTR szSplitterSection;
	static LPCTSTR szPaneWidthCurrent;
	static LPCTSTR szPaneWidthMinimum;
	static LPCTSTR szPaneHeightCurrent;
	static LPCTSTR szPaneHeightMinimum;
};

inline CTabSplitterWnd::IsEmpty()
{
	return m_nCols == 0 && m_nRows == 0;
}

#endif //__TABSPLITTERWND_H__