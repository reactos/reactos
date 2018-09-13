/**************************************************/
/*                                           */
/*                                           */
/* MDI Child Window (reference)                */
/*                                           */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include <afxtempl.h>
class CRefrWnd : public CMDIChildWnd
{
   DECLARE_DYNCREATE(CRefrWnd)
public:
   CRefrWnd();       
   BOOL Create( LPCTSTR szTitle, LONG Style, RECT ReffWndRect, CMDIFrameWnd* Parent);
   BOOL UpdateBitmap();
   void CaptionDraw();

private:
   void DrawFreeForm( BOOL MouseSts);
   void SelectFreeForm( BOOL MouseSts);
   BOOL CreateNewBitmap();
   BOOL ClipPickValueInit();
   BOOL InitSelectLogfont();
   BOOL ClipImageCopy();
   BOOL LoadCloseBitmap();
   void DrawGridLine( CDC *dc);
// void CaptionDraw( CDC *dc);
   void RubberBand( BOOL TestFlag);
   void RubberBandPaint( CDC *dc);
   void IllegalRect( PPOINT ptTL, PPOINT ptBR);
   void CorrectMouseDownPt( CPoint point);
   void CorrectMouseUpPt( CPoint point);
   void MoveRectangle( CPoint point);

public:
   BOOL  GridShow;
   WORD  ReferCode;
   BOOL  RectVisible;
   BOOL  bFocus;
   int   SelectItems;

private:
   CBitmapButton  CloseBtm;
   CBitmap  ImageBmp;
   CDC   ImageDC;
   BOOL  IsCapture;
   BOOL  ValidateFlag;
   CPoint   ptStart;
   CPoint   ptEnd;
   CPoint   ptPrev;
   CPoint   m_ptMouse;
   CPoint   m_ptLast;
   int   ZoomRate;
   UINT  ClipboardFormat;
   CRect PickRect;
   CRect MoveRect;
   CRect ReffWndRect;
   CRgn  FreeRgn, PickRgn;
   CArray<CPoint,CPoint>   m_pointArray;
   CArray<CPoint,CPoint>   m_selectArray;
   BOOL  m_bCloseOnLeft;

protected:
   virtual ~CRefrWnd();
   static CMenu NEAR menu;

   //{{AFX_MSG(CRefrWnd)
   afx_msg void OnClickClose();
   afx_msg void OnPaint();
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg void OnGaijiCopy();   
   afx_msg void OnUpdateGaijiCopy( CCmdUI* pCmdUI);
   afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
   afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);                          
   afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    //afx_msg void OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
    //afx_msg int OnMouseActivate( CWnd* pDesktopWnd, UINT nHitTest, UINT message );

   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};

