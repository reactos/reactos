// ListDev.h : main header file for the LISTDEV application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CListDevApp:
// See ListDev.cpp for the implementation of this class
//

extern CString g_strStartupComputerName;

class CListDevApp : public CWinApp
{
public:
	CListDevApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListDevApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CListDevApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



class CMyCommandLineInfo : public CCommandLineInfo
{
public:
    CMyCommandLineInfo() : m_WaitForName(FALSE), m_DisplayUsage(FALSE)
	{}
    virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);
    BOOL    m_DisplayUsage;
private:
    BOOL    m_WaitForName;
};


/////////////////////////////////////////////////////////////////////////////
