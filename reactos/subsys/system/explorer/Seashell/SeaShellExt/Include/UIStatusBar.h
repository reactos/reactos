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

#ifndef __STATUSBAROPTIONS_H__
#define __STATUSBAROPTIONS_H__

// CStatusBarPane holds the information required for each pane
// if m_bActive is false the pane will not be shown
typedef CList<int,int> CListImages;

class CStatusBarPane : public CObject
{
	DECLARE_SERIAL(CStatusBarPane)
public:

	CStatusBarPane();
	CStatusBarPane(UINT nID,BOOL bActive);
	CStatusBarPane(const CStatusBarPane &rOther);
	virtual ~CStatusBarPane();

	BOOL IsPaneActive() const;
	UINT GetCommandID() const;
	UINT GetStyle() const;
	void SetPaneActive(BOOL bActive);
	void AddImage(int nIndex);
	void SetStyle(UINT nStyle);
	void RemoveImage(int nIndex);
	bool FindImage(int nIndex);
	void RemoveAllImages();
	CListImages &GetImageIndex();

	CStatusBarPane &operator=(const CStatusBarPane &rOther);
// Overrides
	virtual void Serialize(CArchive &ar);
private:
	void DoCopy(const CStatusBarPane &rOther);

private:
	UINT m_nID;
	UINT m_nStyle;
	BOOL m_bActive;
// Icons
	CListImages m_listImageIndex;
};

inline CStatusBarPane::CStatusBarPane()
{
}

inline CStatusBarPane::CStatusBarPane(UINT nID,BOOL bActive) 
	:	m_nID(nID), 
		m_bActive(bActive)
{
	m_nStyle = SBPS_NORMAL;
}

inline CStatusBarPane::~CStatusBarPane()
{
}

inline void CStatusBarPane::SetStyle(UINT nStyle)
{
	m_nStyle = nStyle;
}

inline UINT CStatusBarPane::GetStyle() const
{
	return m_nStyle;
}

inline void CStatusBarPane::SetPaneActive(BOOL bActive)
{
	m_bActive = bActive;
}

inline bool CStatusBarPane::FindImage(int nIndex)
{
	return m_listImageIndex.Find(nIndex) != NULL;
}

// index into an ImageList
inline void CStatusBarPane::AddImage(int nIndex)
{
	m_listImageIndex.AddHead(nIndex);
}

inline void CStatusBarPane::RemoveAllImages()
{
	m_listImageIndex.RemoveAll();
}

inline void CStatusBarPane::RemoveImage(int nIndex)
{
	POSITION pos = m_listImageIndex.Find(nIndex);
	if (pos != NULL)
	{
		m_listImageIndex.RemoveAt(pos);
	}
}

inline CListImages &CStatusBarPane::GetImageIndex()
{
	return m_listImageIndex;
}

inline	BOOL CStatusBarPane::IsPaneActive() const
{
	return m_bActive;
}

inline	UINT CStatusBarPane::GetCommandID() const
{
	return m_nID;
}

// A configurable status bar that will show its own popup menu to configure it.
// You initially add all the panes and then by using the popup menu you can show or
// hide individual panes. The state can be saved to the registry. 
class CTRL_EXT_CLASS CUIStatusBar : public CStatusBar
{
	DECLARE_SERIAL(CUIStatusBar)
public:
	// Construction
	CUIStatusBar();
	~CUIStatusBar();

	void Init();
	void Reset();
	void Load();
	void Save();
	void AddPane(UINT nID,BOOL bActive);
	BOOL IsValidPaneID(UINT nID) const;
	BOOL IsPaneActive(UINT nID) const;
	CStatusBarPane *GetPane(UINT nPaneID) const;
	BOOL SetPanes(BOOL bSave);
	int GetPaneIndex(UINT nID) const;
	UINT GetPaneID(int nPaneIndex);
	UINT GetStyle(UINT nID) const;
	void TogglePane(UINT nID);
	void SetImageList(UINT nBitmapID);
	void SetImageList(CImageList *pImageList);
	void CreateImageList();
	void AddIcon(UINT nPaneID,UINT nImageID,bool bUpdate=true);
	void RemoveIcon(UINT nPaneID,UINT nImageID,bool bUpdate=true);
	void SetStyle(UINT nID,UINT nStyle);
	void Clear();
	void CopyBar(const CUIStatusBar &rOtherBar);
	BOOL IsPanes();
	void UpdatePane(int nIndex);
	void SetText(UINT nPaneID,LPCTSTR szText,bool bUpdate);
	void RemoveAllIcons(UINT nPaneID);

// Overrides
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual void Serialize(CArchive& ar);
protected:
	void RemoveIcon(UINT nImageID,CStatusBarPane *pPane,bool bUpdate);
	void AddImageIndex(UINT nPaneID,int nIndex,bool bUpdate);
	int AddIcon(UINT nID);
	int AddIcon(HICON hIcon);
	void SetTextPane(const CStatusBarPane *pPane,LPCTSTR szText,bool bUpdate=true);
public:
	// Main Pane
	UINT m_nStatusPane1ID;
	UINT m_nStatusPane1Style;
	INT  m_nStatusPane1Width;
	BOOL m_bMenuSelect;
private:
	static LPCTSTR szSection;
	static LPCTSTR szPaneEntry;
	CList<CStatusBarPane*,CStatusBarPane*> m_PaneList;
	CImageList *m_pImageList;

	//{{AFX_MSG(CUIStatusBar)
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CUIStatusBar &operator=(const CUIStatusBar &rOther);
	CMap<UINT,UINT,int,int> m_mapImageIndex;
};

inline BOOL CUIStatusBar::IsPanes()
{
	return !m_PaneList.IsEmpty();
}

inline void CUIStatusBar::CopyBar(const CUIStatusBar &rOtherBar)
{
	*this = rOtherBar;
}

inline void CUIStatusBar::SetStyle(UINT nID,UINT nStyle)
{
	CStatusBarPane *pPane = GetPane(nID);
	ASSERT(pPane);
	int nIndex = GetPaneIndex(nID);
	UINT nOldStyle;
	int nWidth;
	GetPaneInfo(nIndex,nID,nOldStyle,nWidth);
	SetPaneInfo(nIndex,nID,nStyle,nWidth);
	pPane->SetStyle(nStyle);
}

inline UINT CUIStatusBar::GetStyle(UINT nID) const
{
	const CStatusBarPane *pPane = GetPane(nID);
	ASSERT(pPane);
	return pPane->GetStyle();
}

inline int CUIStatusBar::GetPaneIndex(UINT nID) const
{
	int nIndex = CommandToIndex(nID);
	return nIndex;
}

inline	BOOL CUIStatusBar::IsPaneActive(UINT nID) const
{
	const CStatusBarPane *pPane = GetPane(nID);
	ASSERT(pPane);
	return pPane ? pPane->IsPaneActive() : FALSE;
}

inline UINT CUIStatusBar::GetPaneID(int nPaneIndex) 
{
	UINT nID,nStyle;
	int nWidth;
	GetPaneInfo(nPaneIndex,nID,nStyle,nWidth);
	return nID;
}

inline BOOL CUIStatusBar::IsValidPaneID(UINT nID) const
{
	if (nID == ID_SEPARATOR)
		return FALSE;
	const CStatusBarPane *pPane = GetPane(nID);
	return pPane != NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CProgressBar -  status bar progress control
	
class CTRL_EXT_CLASS CProgressBar : public CProgressCtrl
// Creates a ProgressBar in the status bar
{
public:
	CProgressBar();
	CProgressBar(int nPaneID, CUIStatusBar *pStatusBar, int MaxValue = 100);
	~CProgressBar();
	BOOL Create(int nPaneID, CUIStatusBar *pStatusBar, int MaxValue=100);

	DECLARE_DYNCREATE(CProgressBar)

// operations
public:

	void SetRange(int nLower, int nUpper, int nStep = 1);
	void SetSize(int nSize);
	int  SetPos(int nPos);
	int  OffsetPos(int nPos);
	int  SetStep(int nStep);
	int  StepIt();
	void Clear();

// Overrides
	//{{AFX_VIRTUAL(CProgressBar)
	//}}AFX_VIRTUAL

// implementation
protected:

	void Resize();

// Generated message map functions
protected:
	//{{AFX_MSG(CProgressBar)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CUIStatusBar *m_pStatusBar;
	int m_nPaneIndex;
};

inline void	CProgressBar::SetSize(int nSize)
{ 
	Resize(); 
}

#endif
