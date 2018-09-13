// evntfind.cpp : implementation file
//

#include "stdafx.h"
#include "eventrap.h"
#include "evntfind.h"
#include "source.h"
#include "globals.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventFindDlg dialog


CEventFindDlg::CEventFindDlg(CWnd* pParent)
	: CDialog(CEventFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEventFindDlg)
	m_sFindWhat = _T("");
	m_bMatchWholeWord = FALSE;
	m_bMatchCase = FALSE;
	//}}AFX_DATA_INIT

    m_pSource = NULL;
    m_bSearchInTree = TRUE;
    m_bMatchCase = FALSE;
    m_bMatchWholeWord = FALSE;
    m_iFoundWhere = I_FOUND_NOTHING;
}

CEventFindDlg::~CEventFindDlg()
{
    m_pSource->m_pdlgFind = NULL;
}


BOOL CEventFindDlg::Create(CSource* pSource, UINT nIDTemplate, CWnd* pParentWnd)
{
    m_pSource = pSource;
    return CDialog::Create(nIDTemplate, pParentWnd);
}

void CEventFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEventFindDlg)
	DDX_Text(pDX, IDC_EDIT_FIND_WHAT, m_sFindWhat);
	DDX_Check(pDX, IDC_CHECK_MATCH_WHOLEWORD, m_bMatchWholeWord);
	DDX_Check(pDX, IDC_CHECK_MATCH_CASE, m_bMatchCase);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEventFindDlg, CDialog)
	//{{AFX_MSG_MAP(CEventFindDlg)
	ON_BN_CLICKED(IDC_CHECK_MATCH_WHOLEWORD, OnCheckMatchWholeword)
	ON_BN_CLICKED(IDC_CHECK_MATCH_CASE, OnCheckMatchCase)
	ON_EN_CHANGE(IDC_EDIT_FIND_WHAT, OnChangeEditFindWhat)
	ON_BN_CLICKED(IDC_RADIO_SEARCH_DESCRIPTIONS, OnRadioSearchDescriptions)
	ON_BN_CLICKED(IDC_RADIO_SEARCH_SOURCES, OnRadioSearchSources)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_FIND_OK, OnOK)
	ON_BN_CLICKED(IDC_FIND_CANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEventFindDlg message handlers



void CEventFindDlg::OnCheckMatchWholeword() 
{
	CButton *pButton = (CButton*)GetDlgItem(IDC_CHECK_MATCH_WHOLEWORD);
	if (pButton != NULL)
        m_bMatchWholeWord = pButton->GetCheck() != 0;
}

void CEventFindDlg::OnCheckMatchCase() 
{
	CButton *pButton = (CButton*)GetDlgItem(IDC_CHECK_MATCH_CASE);
	if (pButton != NULL)
        m_bMatchCase = pButton->GetCheck() != 0;
}

void CEventFindDlg::OnCancel() 
{
	CDialog::OnCancel();
    delete this;
}

void CEventFindDlg::OnChangeEditFindWhat() 
{
    CWnd* pwndEdit = GetDlgItem(IDC_EDIT_FIND_WHAT);
	CButton* pbtnWholeWord = (CButton*) GetDlgItem(IDC_CHECK_MATCH_WHOLEWORD);
	CString sText;

    // Get the search string and check to see if it contains any spaces.	
	pwndEdit->GetWindowText(sText);
	if (sText.Find(_T(' ')) ==-1) {
        // It does not contain a space.  Enable the window.
		if (!pbtnWholeWord->IsWindowEnabled()) {
			pbtnWholeWord->EnableWindow();
		}
	}
	else {
        // The search string contained a space, disable the whole-word button
        // and uncheck it if necessary.
		if (pbtnWholeWord->IsWindowEnabled()) {
			if (pbtnWholeWord->GetCheck() == 1) {
				// The "whole word" button was checked, so uncheck it and
				// disable the button.
			
				pbtnWholeWord->SetCheck(0);
			}
			pbtnWholeWord->EnableWindow(FALSE);
		}
	}	
	
}

BOOL CEventFindDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
    int idButton = m_bSearchInTree ? IDC_RADIO_SEARCH_SOURCES : IDC_RADIO_SEARCH_DESCRIPTIONS;
	CButton *pButton = (CButton*)GetDlgItem(idButton);
	if (pButton != NULL)
		pButton->SetCheck(1);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEventFindDlg::OnRadioSearchDescriptions() 
{
    m_bSearchInTree = FALSE;
}

void CEventFindDlg::OnRadioSearchSources() 
{
    m_bSearchInTree = TRUE;
}



BOOL CEventFindDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::OnCommand(wParam, lParam);
}

void CEventFindDlg::OnOK() 
{
	// Get the Find What text.
	CEdit* pEdit = (CEdit*) GetDlgItem(IDC_EDIT_FIND_WHAT);
	if (pEdit == NULL)
		return; // Can't do anything.

	// Empty Find What string; nothing to do.
	pEdit->GetWindowText(m_sFindWhat);
	if (m_sFindWhat.IsEmpty())
		return;
    pEdit->SetSel(0, -1);

	BOOL bFound = m_pSource->Find(m_bSearchInTree, m_sFindWhat, m_bMatchWholeWord, m_bMatchCase);
    SetFocus();

	// Put the focus on the parent window.
    if (bFound) {
        if (m_bSearchInTree) {
            m_iFoundWhere = I_FOUND_IN_TREE;
        }
        else {
            m_iFoundWhere = I_FOUND_IN_LIST;
        }
    }
	else {
		CString sMsg;
		sMsg.LoadString(IDS_MSG_TEXTNOTFOUND);				
		MessageBox(sMsg, NULL, MB_OK | MB_ICONINFORMATION);
	}
	
}



FOUND_WHERE CEventFindDlg::Find(CSource* pSource)
{
    m_pSource = pSource;
    DoModal();
    return m_iFoundWhere;
}

BOOL CEventFindDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
                   AfxGetApp()->m_pszHelpFilePath,
                   HELP_WM_HELP,
                   (ULONG_PTR)g_aHelpIDs_IDD_EVENTFINDDLG);
	}
	
	return TRUE;
}

void CEventFindDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (this == pWnd)
		return;

    ::WinHelp (pWnd->m_hWnd,
		       AfxGetApp()->m_pszHelpFilePath,
		       HELP_CONTEXTMENU,
		       (ULONG_PTR)g_aHelpIDs_IDD_EVENTFINDDLG);
}

