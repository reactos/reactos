// Complete.cpp : implementation file
//

#include "stdafx.h"
#include "efsadu.h"
#include "Complete.h"
#include "AddSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComplete property page

IMPLEMENT_DYNCREATE(CComplete, CPropertyPage)

CComplete::CComplete() : CPropertyPage(CComplete::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CComplete)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CComplete::~CComplete()
{
}

void CComplete::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CPropertyPage::OnFinalRelease();
}

void CComplete::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComplete)
	DDX_Control(pDX, IDC_ADDLIST, m_UserAddList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CComplete, CPropertyPage)
	//{{AFX_MSG_MAP(CComplete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CComplete, CPropertyPage)
	//{{AFX_DISPATCH_MAP(CComplete)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IComplete to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {F3A2938E-54B9-11D1-BB63-00A0C906345D}
static const IID IID_IComplete =
{ 0xf3a2938e, 0x54b9, 0x11d1, { 0xbb, 0x63, 0x0, 0xa0, 0xc9, 0x6, 0x34, 0x5d } };

BEGIN_INTERFACE_MAP(CComplete, CPropertyPage)
	INTERFACE_PART(CComplete, IID_IComplete, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComplete message handlers

BOOL CComplete::OnSetActive() 
{
    CAddSheet *AddSheet = (CAddSheet *)GetParent();
    AddSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );

    SetUserList();
	
	return CPropertyPage::OnSetActive();
}

void CComplete::SetUserList()
{
    CAddSheet *AddSheet = (CAddSheet *)GetParent();
    PVOID   Token = NULL;
    CString   UserName;
    CString   DnName;

    try {
        CString UnKnownUser;
        CString NoCertName;

        UnKnownUser.LoadString(IDS_UNKNOWNUSER);
        NoCertName.LoadString(IDS_NOCERTNAME);
        Token = AddSheet->StartEnum();
        while (Token){
            Token = AddSheet->GetNextUser(Token, UserName, DnName);
            if ( (!UserName.IsEmpty()) || (!DnName.IsEmpty())){
                LV_ITEM fillItem;

                fillItem.mask = LVIF_TEXT;
                fillItem.iItem = 0;
                fillItem.iSubItem = 0;

                if (UserName.IsEmpty()){
                    fillItem.pszText = UnKnownUser.GetBuffer(UnKnownUser.GetLength() + 1);
                } else {
                    fillItem.pszText = UserName.GetBuffer(UserName.GetLength() + 1);
                }
                fillItem.iItem = m_UserAddList.InsertItem(&fillItem);
                if (UserName.IsEmpty()){
                    UnKnownUser.ReleaseBuffer();
                } else {
                    UserName.ReleaseBuffer();
                }
                if (fillItem.iItem != -1 ){
                    fillItem.iSubItem = 1;
                    if (DnName.IsEmpty()){
                        fillItem.pszText = NoCertName.GetBuffer(NoCertName.GetLength() + 1);
                    } else {
                        fillItem.pszText = DnName.GetBuffer(DnName.GetLength() + 1);
                    }
                    m_UserAddList.SetItem(&fillItem);
                    if (DnName.IsEmpty()){
                        NoCertName.ReleaseBuffer();
                    } else {
                        DnName.ReleaseBuffer();
                    }
                }

            }
        }
        m_UserAddList.EnableWindow(FALSE);
	}
    catch(...){
                m_UserAddList.DeleteAllItems( );
    }

}

LRESULT CComplete::OnWizardBack() 
{
    m_UserAddList.DeleteAllItems( );	
	return CPropertyPage::OnWizardBack();
}

BOOL CComplete::OnWizardFinish() 
{
    CAddSheet *AddSheet = (CAddSheet *)GetParent();

    AddSheet->AddNewUsers();	
	return CPropertyPage::OnWizardFinish();
}

BOOL CComplete::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    CString UserNameTitle;
    CString UserDnTitle;
    RECT ListRect;
    DWORD ColWidth;

    try {	
	    m_UserAddList.GetClientRect(&ListRect);
        ColWidth = (ListRect.right - ListRect.left)/2;
        UserNameTitle.LoadString(IDS_USERCOLTITLE);
        UserDnTitle.LoadString(IDS_DNCOLTITLE);
        m_UserAddList.InsertColumn(0, UserNameTitle, LVCFMT_LEFT, ColWidth );
        m_UserAddList.InsertColumn(1, UserDnTitle, LVCFMT_LEFT, ColWidth );
    }
    catch (...){
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
