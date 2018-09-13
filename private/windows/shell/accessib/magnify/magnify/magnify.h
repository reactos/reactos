// Magnify.h : main header file for the MAGNIFY application
//

#if !defined(AFX_MAGNIFY_H__C7D0DB66_D691_11D0_AD59_00C04FC2A136__INCLUDED_)
#define AFX_MAGNIFY_H__C7D0DB66_D691_11D0_AD59_00C04FC2A136__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMagnifyApp:
// See Magnify.cpp for the implementation of this class
//

class CMagnifyApp : public CWinApp
{
public:
	CMagnifyApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMagnifyApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMagnifyApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAGNIFY_H__C7D0DB66_D691_11D0_AD59_00C04FC2A136__INCLUDED_)
