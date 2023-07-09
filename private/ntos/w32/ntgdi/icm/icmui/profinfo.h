/******************************************************************************

  Header File:  Profile Information Page.H

  Defines the class used to display the profile information sheet.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  10-24-96  a-robkj@microsoft.com (Pretty Penny Enterprises) began coding this

******************************************************************************/

#include    "PropPage.H"
#include    "Profile.H"

//  CProfileInformationPage class- this handles the Profile Information page(s)

class CProfileInformationPage: public CShellExtensionPage {

    CString    m_csProfile;
    CProfile * m_pcpTarget;

public:

    CProfileInformationPage(HINSTANCE hiWhere, LPCTSTR lpstrTarget);
    ~CProfileInformationPage();

    virtual BOOL    OnInit();
    virtual BOOL    OnDestroy();
};
