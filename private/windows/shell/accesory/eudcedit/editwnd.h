//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
#include	<afxtempl.h>

class CEditWnd : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CEditWnd)
public:
	CEditWnd();
	int  	SaveEUDCCode(UINT msgBoxType);
	void 	CallCharTextOut();
	void 	SetDuplicateRect( RECT	*rect, POINT *point);
	void 	FlipRotate( int RadioItem);
	void	WriteSelRectBitmap();
	BOOL 	SelectCodes();
	BOOL	UpdateBitmap();
	BOOL 	GetBitmapDirty();
	BOOL	SetBitmapDirty( BOOL Flg);
	BOOL 	Create( LPCTSTR szTitle, LONG Style, 
			RECT EudcWndRect, CMDIFrameWnd* Parent);
	virtual ~CEditWnd();
	void 	CaptionDraw();

private:
	BOOL	MoveClipRect();
	void	RotateFigure90( LPBYTE pBuf1, LPBYTE pBuf2, int bWid, int bHgt);
	void	RotateFigure270( LPBYTE pBuf1, LPBYTE pBuf2,int bWid, int bHgt);
	void	DrawGridLine( CDC *dc);
	void 	DrawMoveRect( CDC *dc);
	void 	DrawStretchRect( CDC *dc);
	BOOL	CurveFittingDraw( CDC *dc);
	void 	CorrectMouseDownPoint( CPoint point);
	void 	CorrectMouseUpPoint( CPoint point);
	void 	DrawClipBmp();
	void 	UndoImageDraw();
	void 	EraseRectangle();
	void 	EraseFreeForm();
	BOOL 	SetFreeForm();
	void 	ZoomPoint( CPoint *DrawPt, int x, int y);
	void 	IllegalRect( PPOINT ptTL, PPOINT ptBR);
	void 	DrawFreeForm( BOOL MouseSts);
	void 	SelectFreeForm( BOOL MouseSts);
	void 	ToolInit( int LRButton);
	void 	ToolTerm();
	void 	DrawRubberBand( BOOL StretchFlag);
	void 	SetMoveRect();
	void 	SetValidRect();
	void 	SetPickRect();
	void	SetClickRect();
	void 	StretchMoveRect();
	void 	DrawRectBmp();
	void 	DrawPoint( CPoint Pt, BOOL bErase);
	void 	DrawToPoint(BOOL bErase);
	void 	InitFlipRotate( CDC *RotateDC, CBitmap *RotateBMP);
	BOOL 	DrawStretchClipToDisp();
	BOOL 	CreateNewBitmap();
	BOOL 	CreateUndoBitmap();
	BOOL 	ClipPickValueInit();
	BOOL	InitEditLogfont();
	BOOL 	ClipImageCopy();
	BOOL 	ClipImageCut();
	int	CheckClipRect( POINT ClickPoint);

private:
	CBitmap	UndoImage;
	CBitmap	CRTDrawBmp;
	CBitmap	ImageBmp;
	CDC	CRTDrawDC;
	CDC	ImageDC;
	BOOL	BitmapDirty;
	BOOL	RectClipFlag;
	BOOL 	UndoBitmapFlag;
	BOOL 	IsCapture;
	BOOL	ButtonFlag;
	UINT	ClipboardFormat;
	int	Ratio;
	int	CheckNum;
	int	BrushWidth;
	CArray<CPoint,CPoint>	m_pointArray;
	CArray<CPoint,CPoint>	m_SelectArray;
	CRgn	FreeRgn;
	CRgn	PickRgn;
	CPoint	ptStart;
	CPoint	ptPrev;
	CPoint	ptEnd;
	CRect	PickRect[8];
	CRect	ClipRect[5];
	CRect	EudcWndRect;

public:
	TCHAR 	SelectFont[40];
	int	SelectItem;
	int	ZoomRate;
	WORD	UpdateCode;
	BOOL	GridShow;
	WORD	CallCode;
	BOOL	FlagTmp;
	BOOL    bFocus;

protected:
	static CMenu NEAR menu;

	//{{AFX_MSG(CEditWnd)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnGaijiCopy();
	afx_msg void OnGaijiCut();
	afx_msg void OnGaijiPaste();
	afx_msg void OnGaijiUndo();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnUpdateGaijiPaste(CCmdUI* pCmdUI);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	afx_msg void OnUpdateGaijiCopy(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGaijiCut(CCmdUI* pCmdUI);
	afx_msg void OnDeleteEdit();
	afx_msg void OnUpdateDeleteEdit(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGaijiUndo(CCmdUI* pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
