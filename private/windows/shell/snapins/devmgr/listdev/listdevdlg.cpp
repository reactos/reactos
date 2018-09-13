// ListDevDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ListDev.h"
#include "devtree.h"
#include "computer.h"
#include "ListDevDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CListDevDlg dialog

CListDevDlg::CListDevDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CListDevDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CListDevDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
}

void CListDevDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CListDevDlg)
	DDX_Control(pDX, IDC_DEVDATA, m_lbDevData);
	DDX_Control(pDX, IDC_DEVTREE, m_DevTree);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CListDevDlg, CDialog)
	//{{AFX_MSG_MAP(CListDevDlg)
	ON_WM_PAINT()
	ON_NOTIFY(TVN_SELCHANGED, IDC_DEVTREE, OnSelchanged)
	ON_BN_CLICKED(IDC_CHANGE_COMPUTERNAME, OnChangeComputername)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListDevDlg message handlers

BOOL CListDevDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	RECT rc;
	m_lbDevData.GetClientRect(&rc);
	m_lbDevData.ClientToScreen(&rc);
	m_lbDevData.SetHorizontalExtent((rc.right - rc.left + 1) * 10);
	// TODO: Add extra initialization here
	InitializeDeviceTree(g_strStartupComputerName.IsEmpty() ? NULL :
			     (LPCTSTR)g_strStartupComputerName);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CListDevDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}


void
CListDevDlg::InitializeDeviceTree(
    LPCTSTR ComputerName
    )
{
    m_lbDevData.ResetContent();
    m_DevTree.DeleteAllItems();

    OutputDebugString(_T("InitializeDeviceTree\n"));
    OutputDebugString(ComputerName);
    if (m_DeviceTree.Create(ComputerName))
    {
	CComputer* pComputer = m_DeviceTree.GetComputer();
	SetDlgItemText(IDC_COMPUTERNAME, pComputer->GetDisplayName());
	//m_ImageList.Detach();
	m_ImageList.Attach(m_DeviceTree.GetClassImageList());
	m_DevTree.SetImageList(&m_ImageList, TVSIL_NORMAL);
	// insert computer node as the root node
	TV_INSERTSTRUCT tis;
	tis.hParent = TVI_ROOT;
	tis.hInsertAfter = TVI_SORT;
	memset(&tis.item, 0, sizeof(tis.item));
	tis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE;
	tis.item.pszText = (LPTSTR)pComputer->GetDisplayName();
	tis.item.lParam = (LPARAM)this;
	tis.item.iImage = pComputer->GetImageIndex();
	HTREEITEM hti = m_DevTree.InsertItem(&tis);
	InitializeSubtree(hti, pComputer->GetChild());
	m_DevTree.Expand(hti, TVE_EXPAND);
    }
}


void
CListDevDlg::InitializeSubtree(
    HTREEITEM htiParent,
    CDevice*  pDevice
    )
{
    CDevice* pChild;
    TV_INSERTSTRUCT tis;

    memset(&tis, 0, sizeof(tis));
    do
    {
	tis.hParent = htiParent;
	tis.hInsertAfter = TVI_SORT;
	tis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE;
	tis.item.pszText = (LPTSTR)pDevice->GetDisplayName();
	tis.item.lParam = (LPARAM)pDevice;
	tis.item.iImage = pDevice->GetImageIndex();
	HTREEITEM hti = m_DevTree.InsertItem(&tis);
	pChild = pDevice->GetChild();
	if (pChild)
	{
	    InitializeSubtree(hti, pChild);
	}
	pDevice = pDevice->GetSibling();
    } while (pDevice);
}


typedef struct tagColumnInfo
{
    TCHAR*	Text;
    int 	Format;
    int 	Width;
}COLUMNINFO, *PCOLUMNINFO;


#define NUMBER_OF_COLUMNS	    2
const COLUMNINFO    g_ColumnInfo[2] =
{
    {_T("Data type"), LVCFMT_LEFT, 100},
    {_T("Value"), LVCFMT_LEFT, 200 }
};

void
CListDevDlg::InitializeDeviceData(
    CDevice* pDevice
    )
{
#if 0
    m_DevData.DeleteAllItems();
    LV_COLUMN lvc;
    int i;
    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    for (i = 0; i < NUMBER_OF_COLUMNS; i++)
    {
	lvc.pszText = (LPTSTR)g_ColumnInfo[i].Text;
	lvc.fmt = g_ColumnInfo[i].Format;
	lvc.cx = g_ColumnInfo[i].Width;
	m_DevData.SetColumn(i, &lvc);
    }
    LV_ITEM lvi;
    memset(&lvi, 0, sizeof(lvi));
    lvi.state = 0;
    lvi.lParam = 0;
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    int Index  = 0;
    CAttribute* pAttr;
    while (pDevice->EnumerateAttribute(Index, &pAttr))
    {
	lvi.iItem = Index;
	lvi.pszText = (LPTSTR)pAttr->GetType();
	lvi.iSubItem = 0;
	m_DevData.InsertItem(&lvi);
	lvi.pszText = (LPTSTR)pAttr->GetValue();
	lvi.iSubItem = 1;
	m_DevData.InsertItem(&lvi);
	Index++;
    }
#else
	m_lbDevData.ResetContent();
	CAttribute* pAttr;
	int Index = 0;
	while (pDevice->EnumerateAttribute(Index, &pAttr))
	{
	    CString strText;
	    strText = pAttr->GetType();
	    strText += _T("   ");
	    strText += pAttr->GetValue();
	    m_lbDevData.AddString(strText);
	    Index++;

	}
#endif
}

void CListDevDlg::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	CDevice* pDevice = (CDevice*) pNMTreeView->itemNew.lParam;
	OutputDebugString(_T("OnSelChanged\n"));
	InitializeDeviceData(pDevice);
	*pResult = 0;
}

void CListDevDlg::OnChangeComputername() 
{
	// TODO: Add your control notification handler code here
	computer NewComputer(this);
	NewComputer.DoModal();
	LPCTSTR NewComputerName = NewComputer.GetNewComputerName();
	if (NewComputerName)
	{
	    CString strComputerName;
	    if (_T('\\') != NewComputerName[0])
		strComputerName = _T("\\\\");
	    strComputerName += NewComputerName;
	    InitializeDeviceTree(strComputerName);
	}
}
