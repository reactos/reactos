/******************************************************************************

  Header File:  Property Dialogs.H

  This header defines several classes used for property pages in the UI.  Each
  class is a derived class from CDialog and is described in more detail at the
  point it is defined.

  Each of these classes maintain a reference to the CProfilePropertySheet class
  and use it for the bulk of their information retrieval and interaction.  The
  underlying profile information is available via the Profile() method of
  CProfilePropertySheet.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version

******************************************************************************/

#if !defined(PROPERTY_DIALOGS)

#define PROPERTY_DIALOGS

#include    "ProfProp.H"

/******************************************************************************

  The CInstallPage class presents the dialog on either the install or uninstall
  tab, based upon whether or not the supplied profile is installed.  The two
  dialogs are similar enough that one piece of code will initialize both.

******************************************************************************/

//  CInstallPage class- this encapsulates the Install/Uninstall sheet

class   CInstallPage : public CDialog {
    CProfilePropertySheet&  m_cppsBoss;

public:

    CInstallPage(CProfilePropertySheet *pcppsBoss);
    ~CInstallPage();

    virtual BOOL    OnInit();
    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl);
};

/******************************************************************************

  The CAdvancedPage handles the dialog on the Advanced tab of the UI.  This
  dialog and its handling are the same whether or not the profile is installed.

******************************************************************************/

//  CAdvancedPage class- this handles the Advanced property page

class CAdvancedPage: public CDialog {
    CProfilePropertySheet&  m_cppsBoss;

    void    Update();   //  Update the list and controls

public:

    CAdvancedPage(CProfilePropertySheet *pcppsBoss);
    ~CAdvancedPage();

    virtual BOOL    OnInit();
    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl);
};

/******************************************************************************

  The CAddDeviceDialog class handles the Add Device Dialog, which can be called
  from the Advanced tab.

******************************************************************************/

class CAddDeviceDialog: public CDialog {
    CProfilePropertySheet&  m_cppsBoss;
    HWND                    m_hwndList, m_hwndButton;

public:

    CAddDeviceDialog(CProfilePropertySheet& cpps, HWND hwndParent);

    virtual BOOL    OnInit();

    virtual BOOL    OnCommand(WORD wNotification, WORD wid, HWND hwndControl);

};

#endif
