/******************************************************************************

  Header File:  Profile Property Sheet.H

  This defines the classes used to implement the profile management property
  sheet as defined in the ICM 2.0 shell extension functional specification.

  This class supplies one of two basic dialogs, depending upon whether or not
  the profile has already been installed.  We use the C++ profile class to
  hide any details of that knowledge from this code.

  All structures needed by any of the individual pages or resulting dialogs
  are kept here.  This allows us to easily handle the final Install/Don't
  Install/Associate/Don't Associate decisions.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version

******************************************************************************/

#if !defined(PROFILE_PROPSHEET)

#define PROFILE_PROPSHEET

#include    "Profile.H"
#include    "Dialog.H"

//  class   CProfilePropertySheet - cpps

class CProfilePropertySheet : public CDialog {

    CProfile&   m_cpTarget;
    CDialog     *m_pcdPage[2];
    RECT        m_rcTab;        //  Client area of tab Control
    BOOL        m_bDelete;
    CUintArray  m_cuaAdd;       //  Device associatins to be added
    CUintArray  m_cuaDelete;    //  Device associations to zap
    CUintArray  m_cuaAssociate; //  Tentative list of associated devices

    void    ConstructAssociations();

public:

    CProfilePropertySheet(HINSTANCE hiWhere, CProfile& cpTarget);

    ~CProfilePropertySheet();

    HWND        Window() const { return m_hwnd; }
    HINSTANCE   Instance() const { return m_hiWhere; }
    CProfile&   Profile() { return m_cpTarget; }
    BOOL        DeleteIsOK() const { return m_bDelete; }
    unsigned    AssociationCount() const { 
        return  m_cuaAssociate.Count();
    }

    unsigned    Association(unsigned u) { return m_cuaAssociate[u]; }

    LPCTSTR     DisplayName(unsigned u) { 
        return m_cpTarget.DisplayName(m_cuaAssociate[u]);
    }

    void    DeleteOnUninstall(BOOL bOn) {
        m_bDelete = bOn;
    }

    void    Associate(unsigned u);
    void    Dissociate(unsigned u);

    virtual BOOL    OnInit();
    virtual BOOL    OnNotify(int idCtrl, LPNMHDR pnmh);
    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndControl);
};

#endif

