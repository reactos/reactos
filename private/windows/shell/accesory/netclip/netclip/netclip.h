// NetClip.h : main header file for the NETCLIP application
//
#ifndef _NETCLIP_H_
#define _NETCLIP_H_

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// CNetClipApp:
// See NetClip.cpp for the implementation of this class
//
class CNetClipServer;

class CNetClipApp : public CWinApp
{
public:
	CNetClipApp();

    BOOL m_fServing ;
    BOOL m_fNoUpdate;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetClipApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNetClipApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CNetClipApp      theApp ;
extern OSVERSIONINFO    g_osvi ;
extern BOOL             g_fDCOM ;

/////////////////////////////////////////////////////////////////////////////
#endif
