#if !defined(AFX_ADDSHEET_H__AD17A140_5492_11D1_BB63_00A0C906345D__INCLUDED_)
#define AFX_ADDSHEET_H__AD17A140_5492_11D1_BB63_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddSheet.h : header file
//

#include "welcome.h"
#include "locate.h"
#include "complete.h"
#include <shlobj.h>
#include <dsshell.h>
#include "Users.h"	// Added by ClassView

/////////////////////////////////////////////////////////////////////////////
// CAddSheet

class CAddSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CAddSheet)

// Construction
public:
	CAddSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CAddSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

    DWORD   Add(
                LPTSTR UserName,
                LPTSTR DnName, 
                PVOID UserCert, 
                PSID UserSid = NULL, 
                DWORD Flag = USERINFILE,
                PVOID Context = NULL
              );

    DWORD   Remove(
                LPCTSTR UserName,
                LPCTSTR CertName
              );

    PVOID    StartEnum(void);

    PVOID    GetNextUser(
                        PVOID Token, 
                        CString &UserName,
                        CString &CertName
                        );

    void ClearUserList(void);

    DWORD AddNewUsers(void);

protected:
	void AddControlPages(void);

// Attributes
private:
    CUsers                 m_Users;
    CWelcome           m_WelcomePage;	// Welcome PropPage
    CLocate               m_LocatePage;	// Locate User PropPage
    CComplete          m_CompletePage; // Complete PropPage
    CString                m_SheetTitle;
    UINT                   m_cfDsObjectNames; // ClipBoardFormat

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddSheet)
	public:
	virtual void OnFinalRelease();
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	UINT GetDataFormat(void);
	virtual ~CAddSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAddSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CAddSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDSHEET_H__AD17A140_5492_11D1_BB63_00A0C906345D__INCLUDED_)
