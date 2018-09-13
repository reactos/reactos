//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
#ifndef _EUDCLIST_H_
#define _EUDCLIST_H_

/************************************************/
/*						*/
/*	Character List (Update EUDC)		*/
/*						*/
/************************************************/

class CViewEdit :public CEdit
{
//	member function
public:
	CViewEdit();
	~CViewEdit();
  CFont EUDCFont;

private:
	void DrawConcave( CDC *dc, CRect rect);

protected:

	//{{AFX_MSG(CViewEdit)
	afx_msg void OnPaint();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CCustomListFrame :public CStatic
{
//	member function
public:
	CCustomListFrame();
	~CCustomListFrame();

private:
	void DrawConcave( CDC *dc, CRect rect);

protected:

	//{{AFX_MSG(CCustomListFrame)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CCustomInfoFrame :public CStatic
{
//	member function
public:
	CCustomInfoFrame();
	~CCustomInfoFrame();

private:
	void DrawConcave( CDC *dc, CRect rect);

protected:

	//{{AFX_MSG(CCustomInfoFrame)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifdef BUILD_ON_WINNT
class CColumnHeading :public CWnd
{
//	member function
public:
	CColumnHeading();
	~CColumnHeading();

protected:

	//{{AFX_MSG(CColumnHeading)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

//	member parameter
public:
	CFont	SysFFont;
};
#endif //BUILD_ON_WINNT

class CEudcList : public CEdit
{
	DECLARE_DYNCREATE( CEudcList)

//	Member function
public:
	CEudcList();
	~CEudcList();
	BOOL	CodeButtonClicked();
	void	SetInitEUDCCodeRange( int nIndex);
	void 	SetEUDCCodeRange( int nIndex);
	void 	CalcCharSize();

private:
	WORD 	GetNextCode( WORD Code);
	WORD 	CorrectEUDCCode( WORD Code, BOOL UporDown);
	WORD 	CorrectEUDCCodeKey( WORD Code, int MovePt, BOOL UporDown);
	WORD 	GetCodeScrPos( int Pos);
	int	GetBarPosition( WORD Code);
	void 	DrawConcave( CDC *dc, CRect rect, BOOL PtIn);
	void 	SearchSelectPosition();
	BOOL	IsCorrectChar( UINT i, UINT j);

//	Member parameter
public:
	CFont	EUDCFont;
	CFont	SysFFont;
	WORD 	EUDC_SView;
	WORD	EUDC_EView;
	WORD	EUDC_SCode;
	WORD	EUDC_ECode;
	WORD	SelectCode;
	int	cRange;
	int	BarRange;
	CPoint  LButtonPt;
	CSize 	CharSize;
	short	ScrlBarPos;

private:
	CRect	EditListRect;
	CSize 	FixSize;
	CPoint	WritePos;
	int	ySpace;
	int	m_Index;
	BYTE	CSpace[5];
	BOOL	FocusFlag;

protected:

	//{{AFX_MSG(CEudcList)
	afx_msg void OnPaint();
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
