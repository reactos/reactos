#if !defined(AFX_USERLIST_H__3A1CD0AB_4FC3_11D1_BB5D_00A0C906345D__INCLUDED_)
#define AFX_USERLIST_H__3A1CD0AB_4FC3_11D1_BB5D_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// USERLIST.h : header file
//

#include "users.h"
#include "lmcons.h"
#include "dns.h"
#include "ntdsapi.h"

/////////////////////////////////////////////////////////////////////////////
// USERLIST dialog

class USERLIST : public CDialog
{
// Construction
public:
	USERLIST(CWnd* pParent = NULL);   // standard constructor
	USERLIST(LPCTSTR FileName, CWnd* pParent = NULL); 
	DWORD ApplyChanges(LPCTSTR FileName);

    DWORD    AddNewUsers(CUsers &NewUsers);

// Dialog Data
	//{{AFX_DATA(USERLIST)
	enum { IDD = IDD_ENCRYPT_DETAILS };
	CButton	m_AddButton;
	CButton	m_RemoveButton;
	CListCtrl	m_UserList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(USERLIST)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(USERLIST)
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedUserlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusUserlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusUserlist(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void SetUpListBox(BOOL *Enable);
	void ShowRemove(void);
    CString m_FileName;
    CUsers m_Users;    
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERLIST_H__3A1CD0AB_4FC3_11D1_BB5D_00A0C906345D__INCLUDED_)
