/******************************************************************************

  Header File:  Profile Association Page.H

  Defines the class used to display the profile association sheet.

  Copyright (c) 1996 by Microsoft Corporation

  Change History:

  05-09-97 hideyukn - Created

******************************************************************************/

#include    "PropPage.H"
#include    "Profile.H"

//  CProfileInformationPage class- this handles the Profile Information page(s)

class CProfileAssociationPage: public CShellExtensionPage {

    CString     m_csProfile;

    CProfile   *m_pcpTarget;

    CUintArray  m_cuaAdd;       //  Device associatins to be added
    CUintArray  m_cuaDelete;    //  Device associations to zap

    CUintArray  m_cuaAssociate; //  Tentative list of associated devices

    BOOL        m_bAssociationChanged;

public:

    CProfileAssociationPage(HINSTANCE hiWhere, LPCTSTR lpstrTarget);
    ~CProfileAssociationPage();

    VOID ConstructAssociations();
    VOID UpdateDeviceListBox();

    virtual BOOL    OnInit();
    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndControl);
    virtual BOOL    OnNotify(int idCtrl, LPNMHDR pnmh);
    virtual BOOL    OnDestroy();

    virtual BOOL    OnHelp(LPHELPINFO pHelp);
    virtual BOOL    OnContextMenu(HWND hwnd);

    HINSTANCE   Instance() { return m_psp.hInstance; }

    CProfile *  Profile() { return m_pcpTarget; }

    void        Associate(unsigned uAdd);
    void        Dissociate(unsigned uRemove);
    unsigned    Association(unsigned u) {
        return  m_cuaAssociate[u]; 
    }
    unsigned    AssociationCount() const { 
        return  m_cuaAssociate.Count();
    }
    BOOL        AssociationChanged()    {
        return m_bAssociationChanged;
    }

    VOID        DeviceListChanged()     {
        ConstructAssociations();
    }

    LPCTSTR     DisplayName(unsigned u) { 
        return m_pcpTarget->DisplayName(m_cuaAssociate[u]);
    }
};


//  The CAddDeviceDialog class handles the Add Device Dialog, which can be called
//  from the Association page.

class CAddDeviceDialog: public CDialog {
    CProfileAssociationPage  *m_pcpasBoss;
    HWND                      m_hwndList, m_hwndButton;
    BOOL                      m_bCanceled;

public:

    CAddDeviceDialog(CProfileAssociationPage *pcpas, HWND hwndParent);

    virtual BOOL    OnInit();
    virtual BOOL    OnCommand(WORD wNotification, WORD wid, HWND hwndControl);

    virtual BOOL    OnHelp(LPHELPINFO pHelp);
    virtual BOOL    OnContextMenu(HWND hwnd);

    BOOL            bCanceled()    {return m_bCanceled;}
};
