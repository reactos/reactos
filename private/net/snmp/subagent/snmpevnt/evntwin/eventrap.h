// eventrap.h : main header file for the EVENTRAP application
//

#ifndef __AFXWIN_H__
        #error include 'stdafx.h' before including this file for PCH
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventrapApp:
// See eventrap.cpp for the implementation of this class
//

class CEventrapApp : public CWinApp
{
public:
        CEventrapApp();

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CEventrapApp)
	public:
        virtual BOOL InitInstance();
        virtual int ExitInstance();
        virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg);
	//}}AFX_VIRTUAL

// Implementation

        //{{AFX_MSG(CEventrapApp)
                // NOTE - the ClassWizard will add and remove member functions here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
