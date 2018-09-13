#if !defined(AFX_COMPLETE_H__F3A2938F_54B9_11D1_BB63_00A0C906345D__INCLUDED_)
#define AFX_COMPLETE_H__F3A2938F_54B9_11D1_BB63_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Complete.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CComplete dialog

class CComplete : public CPropertyPage
{
	DECLARE_DYNCREATE(CComplete)

// Construction
public:
	CComplete();
	~CComplete();

// Dialog Data
	//{{AFX_DATA(CComplete)
	enum { IDD = IDD_COMPLETION };
	CListCtrl	m_UserAddList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CComplete)
	public:
	virtual void OnFinalRelease();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CComplete)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CComplete)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
private:
	void SetUserList(void);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPLETE_H__F3A2938F_54B9_11D1_BB63_00A0C906345D__INCLUDED_)
