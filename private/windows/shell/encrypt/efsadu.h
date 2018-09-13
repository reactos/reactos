// EFSADU.h : main header file for the EFSADU DLL
//

#if !defined(AFX_EFSADU_H__15788838_4F37_11D1_BB5D_00A0C906345D__INCLUDED_)
#define AFX_EFSADU_H__15788838_4F37_11D1_BB5D_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CEFSADUApp
// See EFSADU.cpp for the implementation of this class
//

class CEFSADUApp : public CWinApp
{
public:
	CEFSADUApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEFSADUApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CEFSADUApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EFSADU_H__15788838_4F37_11D1_BB5D_00A0C906345D__INCLUDED_)
