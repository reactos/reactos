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

#include "stdafx.h"
#include "UIStatusBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////
//
// IMStatusBar
//
///////////////////////////////////
IMPLEMENT_SERIAL(CUIStatusBar, CStatusBar, 0)
IMPLEMENT_SERIAL(CStatusBarPane, CObject, 0)

BEGIN_MESSAGE_MAP(CUIStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CUIStatusBar)
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

LPCTSTR CUIStatusBar::szSection = _T("Settings\\StatusBar");
LPCTSTR CUIStatusBar::szPaneEntry = _T("Pane");

CStatusBarPane::CStatusBarPane(const CStatusBarPane &rOther)
{
	DoCopy(rOther);
}

CStatusBarPane &CStatusBarPane::operator=(const CStatusBarPane &rOther)
{
	if (this == &rOther)
		return *this;
	DoCopy(rOther);
	return *this;
}

void CStatusBarPane::DoCopy(const CStatusBarPane &rOther)
{
	m_nID = rOther.m_nID;
	m_nStyle = rOther.m_nStyle;
	m_bActive = rOther.m_bActive;
//	m_listImageIndex = rOther.m_listImageIndex;
}

void CStatusBarPane::Serialize(CArchive &ar)
{
	if (ar.IsStoring())
	{
		ar << m_nID;
		ar << m_nStyle;
		ar << m_bActive;
	}
	else
	{
		ar >> m_nID;
		ar >> m_nStyle;
		ar >> m_bActive;
	}
}

void CUIStatusBar::OnContextMenu(CWnd *pWnd, CPoint point)
{
/* 	CMenu Menu;

	// Build the popup menu
	Menu.CreatePopupMenu();
	CStatusBarPane *pPane = NULL;
	CString strText;
	for(POSITION pos = m_PaneList.GetTailPosition();pos != NULL;) 
	{
		pPane = m_PaneList.GetPrev(pos);
		if (pPane->GetCommandID() != ID_SEPARATOR && pPane->GetCommandID() != ID_INDICATOR_MAIN) 
		{
			strText.LoadString(pPane->GetCommandID());
			Menu.AppendMenu(MF_STRING,pPane->GetCommandID(),strText);
		}
	}
	// and display using main frame as message window
	Menu.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,point.x,point.y,theApp.m_pMainWnd);
	theApp.m_pMainWnd->SetFocus();
*/
}
////////////////////////////////////////////////////////////

CUIStatusBar::CUIStatusBar()
{
	Init();
}

CUIStatusBar::~CUIStatusBar()
{
	Clear();
}

void CUIStatusBar::Reset()
{
	Clear();
	Init();
}

void CUIStatusBar::Init()
{
	m_nStatusPane1Width = -1;
	m_nStatusPane1Style = SBPS_STRETCH;
	m_bMenuSelect = FALSE;
	m_nMinHeight = GetSystemMetrics(SM_CYSMICON)+4;
	m_pImageList = new CImageList;
}

void CUIStatusBar::Clear()
{
	CStatusBarPane *pPane;	
	while (!m_PaneList.IsEmpty()) 
	{
			pPane = m_PaneList.RemoveTail();
			delete pPane;
	}
	delete m_pImageList;
	m_pImageList = NULL;
}

CUIStatusBar &CUIStatusBar::operator=(const CUIStatusBar &rOther)
{
	if (this == &rOther)
		return *this;
	for(POSITION pos=rOther.m_PaneList.GetHeadPosition();pos != NULL;) 
	{
		CStatusBarPane *pPane = new CStatusBarPane;
		*pPane = *rOther.m_PaneList.GetNext(pos);
		m_PaneList.AddTail(pPane);
	}
	m_nStatusPane1ID = rOther.m_nStatusPane1ID;
	m_nStatusPane1Style = rOther.m_nStatusPane1Style;
	m_nStatusPane1Width = rOther.m_nStatusPane1Width;
	m_bMenuSelect =	rOther.m_bMenuSelect;
	return *this;
}

void CUIStatusBar::UpdatePane(int nIndex)
{
	CRect rect;
	GetStatusBarCtrl().GetRect(nIndex,&rect);
	InvalidateRect(rect);
	UpdateWindow();
}

CStatusBarPane *CUIStatusBar::GetPane(UINT nPaneID) const
{
	if (nPaneID == 0)
	{
		nPaneID = ((CUIStatusBar*)this)->GetPaneID(0);
	}
	CStatusBarPane *pPane = NULL;
	for (POSITION pos=m_PaneList.GetHeadPosition();pos != NULL;) 
	{
		pPane = m_PaneList.GetNext(pos);
		if (pPane->GetCommandID() == nPaneID)
			break;
	}
	return((pPane && nPaneID == pPane->GetCommandID()) ? pPane : NULL);
}

void CUIStatusBar::RemoveAllIcons(UINT nPaneID)
{
	CStatusBarPane *pPane = GetPane(nPaneID);
	ASSERT(pPane);
	pPane->RemoveAllImages();
	UpdatePane(GetPaneIndex(nPaneID));
}

void CUIStatusBar::RemoveIcon(UINT nPaneID,UINT nImageID,bool bUpdate)
{	
	if (nPaneID == 0)
	{
		nPaneID = GetPaneID(0);
	}
	CStatusBarPane *pPane = GetPane(nPaneID);
	ASSERT(pPane);
	RemoveIcon(nImageID,pPane,bUpdate);
}

void CUIStatusBar::RemoveIcon(UINT nImageID,CStatusBarPane *pPane,bool bUpdate)
{	
	int nImageIndex=-1;
	if (m_mapImageIndex.Lookup(nImageID,nImageIndex))
		pPane->RemoveImage(nImageIndex);
	if (bUpdate)
		UpdatePane(nImageIndex);
}

void CUIStatusBar::AddIcon(UINT nPaneID,UINT nImageID,bool bUpdate)
{
	int nIndex = -1;
	if (!m_mapImageIndex.Lookup(nImageID,nIndex))
		nIndex = AddIcon(nImageID);
	// Possibly wrong id using pane index 0
	if (nPaneID == 0)
	{
		nPaneID = GetPaneID(0);
	}
	AddImageIndex(nPaneID,nIndex,bUpdate);
}

// add an image to list of images that will be displayed
// nIndex into the image list
void CUIStatusBar::AddImageIndex(UINT nPaneID,int nImageIndex,bool bUpdate)
{
	CStatusBarPane *pPane = GetPane(nPaneID);
	ASSERT(pPane);
	int nPaneIndex = GetPaneIndex(nPaneID);
	// Pane 0 can have only one icon
	if (nPaneIndex == 0)
	{
		pPane->RemoveAllImages();
	}
	if (!pPane->FindImage(nImageIndex))
	{
		pPane->AddImage(nImageIndex);
	}
	UINT nStyle;
	int nWidth;
	GetPaneInfo(nPaneIndex,nPaneID,nStyle,nWidth);
	// Make sure it's owner draw
	if (!(nStyle & SBT_OWNERDRAW))
	{
		nStyle |= SBT_OWNERDRAW;
		SetPaneInfo(nPaneIndex,nPaneID,nStyle,nWidth);
		pPane->SetStyle(nStyle);
	}
	if (bUpdate)
		UpdatePane(GetPaneIndex(nPaneID));
}

int CUIStatusBar::AddIcon(UINT nID)
{
	if (m_pImageList->m_hImageList == NULL)
		CreateImageList();
	ASSERT(m_pImageList->m_hImageList);
	HICON hIcon = (HICON)::LoadImage(AfxGetInstanceHandle(),MAKEINTRESOURCE(nID),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
	if (hIcon)
	{
		int nIndex = m_pImageList->Add(hIcon);
		m_mapImageIndex[nID] = nIndex;
		return nIndex;
	}
	return -1;
}

// set image list from a bitmap id
void CUIStatusBar::SetImageList(UINT nBitmapID)
{
	delete m_pImageList;
	m_pImageList = NULL;
	m_pImageList = new CImageList;
	m_pImageList->Create(nBitmapID,::GetSystemMetrics(SM_CXSMICON),0,::GetSysColor(COLOR_BTNFACE));
}

void CUIStatusBar::CreateImageList()
{
	delete m_pImageList;
	m_pImageList = new CImageList;
	m_pImageList->Create(::GetSystemMetrics(SM_CXSMICON),::GetSystemMetrics(SM_CYSMICON),ILC_MASK | ILC_COLOR16,0,1);
	m_pImageList->SetBkColor(CLR_NONE);
}

void CUIStatusBar::SetImageList(CImageList *pImageList)
{
	CreateImageList();
	// if one passed in copy it
	if (pImageList)
	{
		int count = pImageList->GetImageCount();
		for(int i=0;i < count;i++)
		{
			HICON hIcon = pImageList->ExtractIcon(i);
			m_pImageList->Add(hIcon);
		}
	}
}

void CUIStatusBar::AddPane(UINT nID,BOOL bActive)
{
	CStatusBarPane *pPane = new CStatusBarPane(nID,bActive);
	pPane->SetStyle(SBT_OWNERDRAW);
	m_PaneList.AddTail(pPane);
}

BOOL CUIStatusBar::SetPanes(BOOL bSave)
{
	int nSize = 0;
	CStatusBarPane *pPane = NULL;
	for(POSITION pos = m_PaneList.GetTailPosition();pos != NULL;) 
	{
		pPane = m_PaneList.GetPrev(pos);
		nSize += pPane->IsPaneActive();
	}
	UINT *pIndicators = new UINT[nSize];

	int nIndex = 0;
	for(pos = m_PaneList.GetHeadPosition();pos != NULL;) 
	{
		pPane = m_PaneList.GetNext(pos);
		if (pPane->IsPaneActive()) 
		{
			pIndicators[nIndex++] = pPane->GetCommandID();
		}
	}
	BOOL ret = SetIndicators(pIndicators, nSize);
	delete[] pIndicators;

	UINT nID, nStyle;
	int nWidth;
	for(pos = m_PaneList.GetTailPosition();pos != NULL;) 
	{
		pPane = m_PaneList.GetPrev(pos);
		if (pPane->IsPaneActive() && pPane->GetCommandID() != ID_SEPARATOR) 
		{
			int nIndex = GetPaneIndex(pPane->GetCommandID());
			GetPaneInfo(nIndex,nID,nStyle,nWidth);
			SetPaneInfo(nIndex,pPane->GetCommandID(),nStyle | pPane->GetStyle(),nWidth);
			SetPaneText(nIndex,_T(""));
		}
	}
	GetPaneInfo(0,nID,nStyle,nWidth);
	// First pane has its own style
	SetPaneInfo(0,nID,m_nStatusPane1Style,nWidth);
	if (bSave) 
	{
		// save to registry
		Save();
	}
	return ret;
}

void CUIStatusBar::Load()
{
	CWinApp *pApp = AfxGetApp();
	CString strPane;
	CStatusBarPane *pPane = NULL;
	int i=0;
	for(POSITION pos = m_PaneList.GetTailPosition();pos != NULL;i++) 
	{
		pPane = m_PaneList.GetPrev(pos);
		strPane.Format(_T("%s%d"),szPaneEntry,i);
		pPane->SetPaneActive(pApp->GetProfileInt(szSection,strPane,pPane->IsPaneActive()));      
	}
}

void CUIStatusBar::Save()
{
	CWinApp *pApp = AfxGetApp();
	CString strPane;
	CStatusBarPane *pPane = NULL;
	int i=0;
	for(POSITION pos = m_PaneList.GetTailPosition();pos != NULL;i++) 
	{
		pPane = m_PaneList.GetPrev(pos);
		strPane.Format(_T("%s%d"),szPaneEntry,i);
		pApp->WriteProfileInt(szSection,strPane,pPane->IsPaneActive());
	}
}

void CUIStatusBar::TogglePane(UINT nID)
{
	CStatusBarPane *pPane = GetPane(nID);
	if (pPane) 
	{
		pPane->SetPaneActive(!pPane->IsPaneActive());
		SetPanes(TRUE);
	}
}

void CUIStatusBar::SetText(UINT nPaneID,LPCTSTR szText,bool bUpdate)
{
	const CStatusBarPane *pPane = GetPane(nPaneID);
	ASSERT(pPane);
	SetTextPane(pPane,szText,bUpdate);
}

void CUIStatusBar::SetTextPane(const CStatusBarPane *pPane,LPCTSTR szText,bool bUpdate)
{
	if (!pPane->IsPaneActive())
		return;

	int nIndex = GetPaneIndex(pPane->GetCommandID());
	UINT nStyle;
	UINT nPaneID=pPane->GetCommandID();
	int nWidth;
	GetPaneInfo(nIndex, nPaneID, nStyle, nWidth);
	nStyle |= SBT_OWNERDRAW;
	if (nIndex != 0)
	{
		HGDIOBJ hOldFont = NULL;
		HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
		CClientDC dc(NULL);
		if (hFont != NULL) 
			hOldFont = dc.SelectObject(hFont);
		CSize size = dc.GetTextExtent(szText);
		if (hOldFont != NULL) 
			dc.SelectObject(hOldFont);
		nWidth = size.cx;
	}
	SetPaneInfo(nIndex, nPaneID, nStyle, nWidth);
	SetPaneText(nIndex, szText, bUpdate);
	if (bUpdate)
		UpdateWindow();
}

void CUIStatusBar::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CStatusBarPane *pPane = GetPane(GetPaneID(lpDrawItemStruct->itemID));
	if (pPane) 
	{
		// Get the pane rectangle and calculate text coordinates
		CRect rect(&lpDrawItemStruct->rcItem);
		CDC *pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
		COLORREF oldBkColor = pDC->SetBkColor(::GetSysColor(COLOR_BTNFACE));
		// draw the image
		CListImages &listImages = pPane->GetImageIndex();
		// make sure there is an image
		if (!listImages.IsEmpty()) 
		{
			for(POSITION pos=listImages.GetTailPosition();pos != NULL;)
			{
				m_pImageList->Draw(pDC,listImages.GetPrev(pos),CPoint(rect.left,rect.top),ILD_NORMAL);
				rect.left += ::GetSystemMetrics(SM_CXSMICON);
				rect.left += 5;
			}
		}
		// draw the text
		LPCTSTR pszText = (LPCTSTR)lpDrawItemStruct->itemData;
		// make sure there is text
		if (pszText)
		{
			UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS | DT_NOPREFIX | DT_VCENTER;
//			if (pPane->GetStyle() & SBPS_POPOUT)
//				nFormat |= DT_VCENTER;
//			else
//				nFormat |= DT_BOTTOM;
			pDC->DrawText(pszText,lstrlen(pszText),rect,nFormat);
		}
		pDC->SetBkColor(oldBkColor);
	}
}

void CUIStatusBar::Serialize(CArchive &ar)
{
	CStatusBar::Serialize(ar);
	m_PaneList.Serialize(ar);
}

template <> void AFXAPI SerializeElements <CStatusBarPane*> (CArchive& ar, CStatusBarPane **ppPane, int nCount)
{
    for (int i=0;i < nCount;i++,ppPane++)
    {
		if (ar.IsLoading())
		{
			ar >> *ppPane;
		}
		else
		{
			ar << *ppPane;
		}
    }
}


void CUIStatusBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CStatusBar::OnLButtonDown(nFlags, point);
}

BOOL CUIStatusBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// TODO: Add your message handler code here and/or call default
/*	if (theApp.IsLoading())
	{
		CPoint point;
		::GetCursorPos(&point);
		ScreenToClient(&point);
		CRect rect;
		GetItemRect(4,&rect);
		if (rect.PtInRect(point))
		{
			SetCursor(::LoadCursor(NULL,IDC_CROSS));
			return TRUE;
		}
	}
*/	
	return CStatusBar::OnSetCursor(pWnd, nHitTest, message);
}

////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CProgressBar, CProgressCtrl)
	
BEGIN_MESSAGE_MAP(CProgressBar, CProgressCtrl)
	//{{AFX_MSG_MAP(CProgressBar)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CProgressBar::CProgressBar() : m_nPaneIndex(-1), m_pStatusBar(NULL)
{
}

CProgressBar::CProgressBar(int nPaneID, CUIStatusBar *pStatusBar, int MaxValue /* = 100 */)
{
	Create(nPaneID, pStatusBar, MaxValue);
}

CProgressBar::~CProgressBar()
{
	Clear();
}	

// Create the CProgressCtrl as a child of the status bar 
BOOL CProgressBar::Create(int nPaneID, CUIStatusBar *pStatusBar, int MaxValue)
{	
	m_pStatusBar = pStatusBar;
	m_nPaneIndex = pStatusBar->GetPaneIndex(nPaneID);
	if (m_nPaneIndex == -1)
		return FALSE;

	// Create the progress bar
	if (!CProgressCtrl::Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH, CRect(0,0,0,0), pStatusBar, 1))
		return FALSE;

	// Set range and step
	SetRange(0, MaxValue);
	SetStep(1);

	// Resize the control to its desired width
	Resize();

	return TRUE;
}

void CProgressBar::Clear()
{
}

void CProgressBar::SetRange(int nLower, int nUpper, int nStep /* = 1 */)	
{
	CProgressCtrl::SetRange(nLower, nUpper);
	CProgressCtrl::SetStep(nStep);
}

int CProgressBar::SetPos(int nPos)	 
{
	if (m_hWnd == NULL)
		return -1;
	ModifyStyle(0,WS_VISIBLE);
	return CProgressCtrl::SetPos(nPos);
}

int  CProgressBar::OffsetPos(int nPos) 
{ 
	if (m_hWnd == NULL)
		return -1;
	ModifyStyle(0,WS_VISIBLE);
	return CProgressCtrl::OffsetPos(nPos);
}

int  CProgressBar::SetStep(int nStep)
{ 
	if (m_hWnd == NULL)
		return -1;
	ModifyStyle(0,WS_VISIBLE);
	return CProgressCtrl::SetStep(nStep);	
}

int  CProgressBar::StepIt()			 
{ 
	if (m_hWnd == NULL)
		return -1;
	ModifyStyle(0,WS_VISIBLE);
	return CProgressCtrl::StepIt();	
}

void CProgressBar::Resize() 
{	
	ASSERT(m_pStatusBar);
	ASSERT(m_nPaneIndex != -1);

	if (!m_pStatusBar) 
		return;

	// Now get the rectangle in which we will draw the progress bar
	CRect rc;
	m_pStatusBar->GetItemRect (m_nPaneIndex, rc);	
	// Resize the window
	if (::IsWindow(m_hWnd))
	    MoveWindow(&rc);
}

BOOL CProgressBar::OnEraseBkgnd(CDC* pDC) 
{
	Resize();
	return CProgressCtrl::OnEraseBkgnd(pDC);
}
