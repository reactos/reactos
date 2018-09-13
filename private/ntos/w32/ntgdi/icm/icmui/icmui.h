/******************************************************************************

  Header File:  Profile Management UI.H

  Defines the class(es) used in implementing the ICM 2.0 UI.  Most of these are
  defined in other headers, but are assembled here by reference.  

  Copyright (c) 1996, 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:

  10-24-96  a-robkj@microsoft.com (Pretty Penny Enterprises) began coding this
  01-08-97  KjelgaardR@acm.org  Added color printer determination function to
            the CGlobals class.

******************************************************************************/

#undef  WIN32_LEAN_AND_MEAN
#if !defined(STRICT)
#define STRICT
#endif

#include    <Windows.H>
#include    <commctrl.h>
#include    <crtdbg.h>
#include    <dlgs.h>
#include    <icmpriv.h>

// #include    "PropDlg.H"
#include    "ProfInfo.H"
#include    "ProfAssoc.H"
#include    "Resource.H"
#include    "DevProp.H"
#include    "IcmUIHlp.H"

//  To handle various globals, etc., we implement the following class (with no
//  non-static members).  I'll admit to being a real bigot about global data.

class CGlobals {
    static int      m_icDLLReferences;
    static HMODULE  m_hmThisDll;
    //  List of profiles kept here to speed GetIconLocation up
    static CStringArray m_csaProfiles;
    static BOOL         m_bIsValid;

public:
    
    static void Attach() { m_icDLLReferences++; }
    static void Detach() { m_icDLLReferences--; }
    static int& ReferenceCounter() { return m_icDLLReferences; }
    static void SetHandle(HMODULE hmNew) { 
        if  (!m_hmThisDll) 
            m_hmThisDll = hmNew; 
    }

    static HMODULE  Instance() { 
        return m_hmThisDll;
    }
    
    static HRESULT  CanUnload() { 
        return (!m_icDLLReferences && CShellExtensionPage::OKToClose()) ?
            S_OK : S_FALSE;
    }

    //  Error routine to report problems via a Message box.  Pass the String ID
    //  of the error...

    static void Report(int idError, HWND hwndParent = NULL);
    static int  ReportEx(int idError, HWND hwndParent, BOOL bSystemMessage, UINT uType, DWORD dwNumMsg, ...);

    //  Routines for maintenance of a cached set of installed profiles to speed
    //  up GetIconLocation

    static BOOL IsInstalled(CString& csProfile);
    static void InvalidateList() { m_bIsValid = FALSE; }


    // Routine for determining a printer's hdc
    // Caller is responsible for calling DeleteDC() on 
    // the returned value.
    // Note that this routine uses CreateIC() to get an
    // information context rather than CreateDC() to get the
    // device context.
    static HDC GetPrinterHDC(LPCTSTR lpctstrName);

    //  Routine for determining if a printer is monochrome or color
    static BOOL ThisIsAColorPrinter(LPCTSTR lpctstrName);
};

