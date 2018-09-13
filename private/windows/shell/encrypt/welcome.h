#if !defined(AFX_WELCOME_H__8C048CD7_54B2_11D1_BB63_00A0C906345D__INCLUDED_)
#define AFX_WELCOME_H__8C048CD7_54B2_11D1_BB63_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Welcome.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWelcome dialog

class CWelcome : public CPropertyPage
{
	DECLARE_DYNCREATE(CWelcome)

// Construction
public:
	CWelcome();
	~CWelcome();

// Dialog Data
	//{{AFX_DATA(CWelcome)
	enum { IDD = IDD_WELCOME };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWelcome)
	public:
	virtual void OnFinalRelease();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWelcome)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CWelcome)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WELCOME_H__8C048CD7_54B2_11D1_BB63_00A0C906345D__INCLUDED_)
