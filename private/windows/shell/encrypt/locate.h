#if !defined(AFX_LOCATE_H__8C048CD9_54B2_11D1_BB63_00A0C906345D__INCLUDED_)
#define AFX_LOCATE_H__8C048CD9_54B2_11D1_BB63_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Locate.h : header file
//

#include <wincrypt.h>

/////////////////////////////////////////////////////////////////////////////
// CLocate dialog

class CLocate : public CPropertyPage
{
	DECLARE_DYNCREATE(CLocate)

// Construction
public:
	CLocate();
	~CLocate();

// Dialog Data
	//{{AFX_DATA(CLocate)
	enum { IDD = IDD_LOCATING };
	CListCtrl	m_UserAddList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLocate)
	public:
	virtual void OnFinalRelease();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLocate)
	afx_msg void OnBrowseDir();
	afx_msg void OnBrowseFile();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CLocate)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
private:
	DWORD GetCertNameFromCertContext(PCCERT_CONTEXT CertContext, LPTSTR *UserCertName);
	HRESULT FindUserFromDir();
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOCATE_H__8C048CD9_54B2_11D1_BB63_00A0C906345D__INCLUDED_)
