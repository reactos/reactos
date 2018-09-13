/**************************************************/
/*						                          */
/*						                          */
/*	MDI mainframe window class		              */
/*						                          */
/*						                          */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include	"guidebar.h"

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();
	BOOL 	Create( LPCTSTR WndTitle, DWORD WndStyle, RECT MainWndRect, LPCTSTR nID);
	BOOL 	OpenReferWindow();
	BOOL	OutputSaveMessage();
	BOOL	CustomActivate();
	CGuideBar	m_wndGuideBar;

private:
	WORD	CorrectCode( WORD Code, BOOL UporDown);	
	UINT	SetReffCode( BOOL EditorRefer);

private:
	BOOL 	CustomWndOpen;
	BOOL 	RefferWndVisible;
	BOOL	ToolBarVisible;
	BOOL 	GuideBarVisible;
	CRect	EudcWndRect;
	CRect	ReffWndRect;

protected:
	CToolBar	m_wndToolBar;

public:
	virtual ~CMainFrame();

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext *pContext);

#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif

private:
	BOOL CreateToolBar();
	BOOL CreateGuideBar();
	BOOL CreateReferWnd();
	void SetEudcWndPos();
	void SetReffWndPos();
	void CalcEudcWndRect();
	void CalcReffWndRect();

protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnStylesBar(UINT nID);
	afx_msg void OnUpdateStylesBar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolbar(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnRefferfileNew();
	afx_msg void OnReadChar();
	afx_msg void OnUpdateRefferfileNew(CCmdUI* pCmdUI);
	afx_msg void OnToolbar();
	afx_msg void OnRefferClose();
	afx_msg void OnUpdateRefferClose(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnShowGrid();
	afx_msg void OnUpdateShowGrid(CCmdUI* pCmdUI);
	afx_msg void OnSelectfamily();
	afx_msg void OnSavechar();
	afx_msg void OnUpdateSavechar(CCmdUI* pCmdUI);
	afx_msg void OnSaveCharas();
	afx_msg void OnUpdateSaveCharas(CCmdUI* pCmdUI);
	afx_msg void OnLinkime();
	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnUpdateReadChar(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLinkime(CCmdUI* pCmdUI);
	afx_msg void OnNextcode();
	afx_msg void OnUpdateNextcode(CCmdUI* pCmdUI);
	afx_msg void OnPrevcode();
	afx_msg void OnUpdatePrevcode(CCmdUI* pCmdUI);
	afx_msg void OnCallChar();
	afx_msg void OnUpdateCallChar(CCmdUI* pCmdUI);
	afx_msg void OnImportFile();
	afx_msg void OnUpdateImportFile(CCmdUI* pCmdUI);
	afx_msg void OnLinkbatchmode();
	afx_msg void OnUpdateLinkbatchmode(CCmdUI* pCmdUI);
	afx_msg void OnHelp();
	afx_msg void OnFitCurve();
	afx_msg void OnUpdateFitCurve(CCmdUI* pCmdUI);
	afx_msg void OnRotate();
	afx_msg void OnUpdateRotate(CCmdUI* pCmdUI);
	afx_msg void OnPalettebrush();
	afx_msg void OnUpdatePalettebrush(CCmdUI* pCmdUI);
	afx_msg void OnPalettecircle();
	afx_msg void OnUpdatePalettecircle(CCmdUI* pCmdUI);
	afx_msg void OnPalettecirclefill();
	afx_msg void OnUpdatePalettecirclefill(CCmdUI* pCmdUI);
	afx_msg void OnPalettefreeform();
	afx_msg void OnUpdatePalettefreeform(CCmdUI* pCmdUI);
	afx_msg void OnPalettepen();
	afx_msg void OnUpdatePalettepen(CCmdUI* pCmdUI);
	afx_msg void OnPaletterect();
	afx_msg void OnUpdatePaletterect(CCmdUI* pCmdUI);
	afx_msg void OnPaletterectband();
	afx_msg void OnUpdatePaletterectband(CCmdUI* pCmdUI);
	afx_msg void OnPaletterectfill();
	afx_msg void OnUpdatePaletterectfill(CCmdUI* pCmdUI);
	afx_msg void OnPaletteeraser();
	afx_msg void OnUpdatePaletteeraser(CCmdUI* pCmdUI);
	afx_msg void OnPaletteslope();
	afx_msg void OnUpdatePaletteslope(CCmdUI* pCmdUI);
	afx_msg void OnSelectCharSet();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
