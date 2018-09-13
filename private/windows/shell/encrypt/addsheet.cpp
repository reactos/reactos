// AddSheet.cpp : implementation file
//

#include "stdafx.h"
#include "EFSADU.h"
#include "AddSheet.h"
#include "userlist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddSheet

IMPLEMENT_DYNAMIC(CAddSheet, CPropertySheet)

CAddSheet::CAddSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	EnableAutomation();
    AddControlPages();
    m_cfDsObjectNames = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
}

CAddSheet::CAddSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage), m_SheetTitle(pszCaption)
{
	EnableAutomation();
    AddControlPages();
    m_cfDsObjectNames = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
}

CAddSheet::~CAddSheet()
{
}

void CAddSheet::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CPropertySheet::OnFinalRelease();
}

//
// This routine adds the tab to the sheet
//

void CAddSheet::AddControlPages()
{

    AddPage(&m_WelcomePage);
    AddPage(&m_LocatePage);
    AddPage(&m_CompletePage);
 
}


BEGIN_MESSAGE_MAP(CAddSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CAddSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CAddSheet, CPropertySheet)
	//{{AFX_DISPATCH_MAP(CAddSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IAddSheet to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {AD17A13F-5492-11D1-BB63-00A0C906345D}
static const IID IID_IAddSheet =
{ 0xad17a13f, 0x5492, 0x11d1, { 0xbb, 0x63, 0x0, 0xa0, 0xc9, 0x6, 0x34, 0x5d } };

BEGIN_INTERFACE_MAP(CAddSheet, CPropertySheet)
	INTERFACE_PART(CAddSheet, IID_IAddSheet, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddSheet message handlers

BOOL CAddSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
    SetTitle(m_SheetTitle);
	
	return bResult;
}

UINT CAddSheet::GetDataFormat()
{
    return m_cfDsObjectNames;
}

DWORD   
CAddSheet::Add(
        LPTSTR UserName,
        LPTSTR DnName, 
        PVOID UserCert, 
        PSID UserSid /*= NULL */, 
        DWORD Flag /*= USERINFILE*/,
        PVOID Context /*= NULL*/
      )
{
    return m_Users.Add(
                    UserName,
                    DnName,
                    UserCert,
                    UserSid,
                    Flag,
                    Context
                    );    
}

DWORD
CAddSheet::Remove(
    LPCTSTR UserName,
    LPCTSTR UserCertName
    )
{
    return m_Users.Remove(
                    UserName,
                    UserCertName
                    );
}

PVOID
CAddSheet::StartEnum()
{
    return m_Users.StartEnum();
}

PVOID
CAddSheet::GetNextUser(
    PVOID Token, 
    CString &UserName,
    CString &CertName
    )
{
    return m_Users.GetNextUser(
                    Token,
                    UserName,
                    CertName
                    );
}

void
CAddSheet::ClearUserList(void)
{
   m_Users.Clear();
}

DWORD
CAddSheet::AddNewUsers(void)
{
    USERLIST    *Parent = (USERLIST *)GetParent();

    return Parent->AddNewUsers(m_Users);
}
