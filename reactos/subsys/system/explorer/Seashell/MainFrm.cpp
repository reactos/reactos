// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "SeaShell.h"
#include "MFCExplorerDlg.h"
#include "FilterDlg.h"
#include "ShellTreeDlg.h"
#include "MainFrm.h"
#include "LeftView.h"
#include "SeaShellView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CUIExplorerFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CUIExplorerFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_EXPLORERDIALOG, OnViewExplorerdialog)
	ON_COMMAND(ID_VIEW_FILEFILTER, OnViewFilefilter)
	ON_COMMAND(ID_VIEW_TREEDIALOG, OnViewTreedialog)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnUpdateViewStyles)
	ON_COMMAND_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnViewStyle)
	ON_MESSAGE(WM_SETTINGCHANGE,OnSettingChange)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	m_IDToolbar = IDR_MAINFRAME;
	SetExplorerView(RUNTIME_CLASS(CSeaShellView));
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::CreateCoolBar()
{
	if (m_pwndCoolBar == NULL)
		m_pwndCoolBar = new CWebBrowserCoolBar;
	// hook up the frame window for notification messages
	((CWebBrowserCoolBar*)m_pwndCoolBar)->GetComboBox().SetTreeCtrlWnd(GetSafeHwnd());
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CUIExplorerFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;	
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CUIExplorerFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CUIExplorerFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CUIExplorerFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

CSeaShellView* CMainFrame::GetRightPane()
{
	CWnd* pWnd = GetSplitterWnd().GetPane(0, 1);
	CSeaShellView* pView = DYNAMIC_DOWNCAST(CSeaShellView, pWnd);
	return pView;
}

void CMainFrame::OnUpdateViewStyles(CCmdUI* pCmdUI)
{
	// TODO: customize or extend this code to handle choices on the
	// View menu.

	CSeaShellView* pView = GetRightPane(); 

	// if the right-hand pane hasn't been created or isn't a view,
	// disable commands in our range

	if (pView == NULL)
		pCmdUI->Enable(FALSE);
	else
	{
		DWORD dwStyle = pView->GetListCtrl().GetViewType();

		// if the command is ID_VIEW_LINEUP, only enable command
		// when we're in LVS_ICON or LVS_SMALLICON mode

		if (pCmdUI->m_nID == ID_VIEW_LINEUP)
		{
			if (dwStyle == LVS_ICON || dwStyle == LVS_SMALLICON)
				pCmdUI->Enable();
			else
				pCmdUI->Enable(FALSE);
		}
		else
		{
			// otherwise, use dots to reflect the style of the view
			pCmdUI->Enable();
			BOOL bChecked = FALSE;

			switch (pCmdUI->m_nID)
			{
			case ID_VIEW_DETAILS:
				bChecked = (dwStyle == LVS_REPORT);
				break;

			case ID_VIEW_SMALLICON:
				bChecked = (dwStyle == LVS_SMALLICON);
				break;

			case ID_VIEW_LARGEICON:
				bChecked = (dwStyle == LVS_ICON);
				break;

			case ID_VIEW_LIST:
				bChecked = (dwStyle == LVS_LIST);
				break;

			default:
				bChecked = FALSE;
				break;
			}

			pCmdUI->SetRadio(bChecked ? 1 : 0);
		}
	}
}


void CMainFrame::OnViewStyle(UINT nCommandID)
{
	// TODO: customize or extend this code to handle choices on the
	// View menu.
	CSeaShellView* pView = (CSeaShellView*)theApp.GetView(RUNTIME_CLASS(CSeaShellView));;

	// if the right-hand pane has been created and is a CSeaShellView,
	// process the menu commands...
	if (pView != NULL)
	{
		DWORD dwStyle = -1;

		switch (nCommandID)
		{
		case ID_VIEW_LINEUP:
			{
				// ask the list control to snap to grid
				CListCtrl& refListCtrl = pView->GetListCtrl();
				refListCtrl.Arrange(LVA_SNAPTOGRID);
			}
			break;

		// other commands change the style on the list control
		case ID_VIEW_DETAILS:
			dwStyle = LVS_REPORT;
			break;

		case ID_VIEW_SMALLICON:
			dwStyle = LVS_SMALLICON;
			break;

		case ID_VIEW_LARGEICON:
			dwStyle = LVS_ICON;
			break;

		case ID_VIEW_LIST:
			dwStyle = LVS_LIST;
			break;
		}

		// change the style; window will repaint automatically
		if (dwStyle != -1)
			pView->GetListCtrl().SetViewType(dwStyle);
	}
}

LRESULT CMainFrame::OnSettingChange(WPARAM wParam, LPARAM lParam)
{
	TRACE(_T("OnSettingChange\n"));
	CSeaShellView *pListView = (CSeaShellView*)theApp.GetView(RUNTIME_CLASS(CSeaShellView));
	if (pListView)
		pListView->GetListCtrl().SendMessage(WM_SETTINGCHANGE,wParam,lParam);		
	return 1;
}

void CMainFrame::OnViewExplorerdialog() 
{
	// TODO: Add your command handler code here
	CLeftView *pTreeView = (CLeftView*)theApp.GetView(RUNTIME_CLASS(CLeftView));
	CMFCExplorerDlg	dlg;
	if (pTreeView && pTreeView->GetShellTreeCtrl().GetSelectedItem() != pTreeView->GetShellTreeCtrl().GetRootItem())
	{
		dlg.SetPath(pTreeView->GetShellTreeCtrl().GetSelectedPath());
	}
	dlg.DoModal();		
}

void CMainFrame::OnViewFilefilter() 
{
	// TODO: Add your command handler code here
	CSeaShellView *pListView = (CSeaShellView*)theApp.GetView(RUNTIME_CLASS(CSeaShellView));
	if (pListView)
	{
		CFilterDlg dlg;
		dlg.m_Filter = pListView->GetShellListCtrl().GetFileFilter();
		if (!pListView->GetShellListCtrl().GetExcludedFileTypes().IsEmpty())
			dlg.m_FileType = pListView->GetShellListCtrl().GetExcludedFileTypes().GetHead();
		if (dlg.DoModal() == IDOK)
		{
			if (!dlg.m_FileType.IsEmpty())
				pListView->GetShellListCtrl().ExcludeFileType(dlg.m_FileType);		
			pListView->GetShellListCtrl().SetFileFilter(dlg.m_Filter);		
			pListView->GetShellListCtrl().Refresh();
		}
	}	
}

void CMainFrame::OnViewTreedialog() 
{
	// TODO: Add your command handler code here
	CShellTreeDlg dlg;
	dlg.DoModal();
}
