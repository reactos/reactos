// SeaShell.h : main header file for the SEASHELL application
//

#if !defined(AFX_SEASHELL_H__6922416D_59C7_4311_9767_0AF1B6CD7C95__INCLUDED_)
#define AFX_SEASHELL_H__6922416D_59C7_4311_9767_0AF1B6CD7C95__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSeaShellApp:
// See SeaShell.cpp for the implementation of this class
//
#include "UIApp.h"

class CSeaShellApp : public CUIApp
{
public:
	CSeaShellApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSeaShellApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CSeaShellApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CSeaShellApp theApp;
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEASHELL_H__6922416D_59C7_4311_9767_0AF1B6CD7C95__INCLUDED_)
