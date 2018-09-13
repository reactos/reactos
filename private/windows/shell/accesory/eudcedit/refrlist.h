/**************************************************/
/*						                          */
/*	Character List (Referrence Dialog)	          */		
/*						                          */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

class CRefListFrame :public CStatic
{
//	member function
public:
	CRefListFrame();
	~CRefListFrame();

private:
	void DrawConcave( CDC *dc, CRect rect);

protected:

	//{{AFX_MSG(CRefListFrame)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CRefInfoFrame :public CStatic
{
//	member function
public:
	CRefInfoFrame();
	~CRefInfoFrame();

private:
	void DrawConcave( CDC *dc, CRect rect);

protected:

	//{{AFX_MSG(CRefInfoFrame)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CRefrList : public CEdit
{
	DECLARE_DYNCREATE( CRefrList)

//	Member function
public:
	CRefrList();
	~CRefrList();

public:
	BOOL	CodeButtonClicked();
	void	SetCodeRange();
	void	ResetParam();
	void 	CalcCharSize();

private:
	int	CheckCharType( WORD Code);
	int	GetBarPosition( WORD Code);
	WORD 	CalculateCode( WORD Start, WORD End);
	WORD 	GetPlusCode( WORD Code, int ScrollNum);
	WORD 	GetMinusCode( WORD Code, int ScrollNum);
	WORD 	GetPlusCodeKey( WORD Code, int ScrollNum);
	WORD 	GetMinusCodeKey( WORD Code, int ScrollNum);
	WORD 	GetCodeScrPos( int Pos);
	BOOL 	IsCheckedCode( WORD CodeStock);
	BOOL	IsCorrectChar( UINT i, UINT j);
	void 	SearchKeyPosition( BOOL Flg);
	void 	DrawConcave( CDC *dc, CRect rect, BOOL PtIn);

//	Member parameter
public:
	CPoint	LButtonPt;
	WORD 	ViewStart;
	WORD	ViewEnd;
	WORD	SelectCode;
	short	ScrlBarPos;
	CFont	SysFFont;
	CFont	CharFont;
	CFont	ViewFont;
	CSize 	CharSize;
	int	PointSize;
	LOGFONT	rLogFont;
	LOGFONT	cLogFont;
	DWORD   dwCodePage;

private:
	CRect	CodeListRect;
	CPoint	WritePos;
	CSize 	FixSize;
	WORD	StartCode;
	WORD	EndCode;
	WORD	BottomCode;
	int	xSpace;
	int	ySpace;
	int	CHN;
	BOOL	FocusFlag;

protected:
	//{{AFX_MSG(CRefrList)
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
