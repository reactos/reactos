/******************************************************************************

  Header File:  Device Property Page.H

  Defines the class that handles the various device profile management pages.
  These derive from CShellExtensionPage.

  Since much of the profile management process is common to all devices, a base
  class (CDeviceProfileManagement) provides these core services- filling the
  device list box, properly enabling and disabling the "Remove" button, and
  adding, associating, and dissociating profiles as needed.  Virtual functions
  provide the means by which the individual device pages customize or modify
  this behavior.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises, Inc. Production

  Change History:

  11-27-96  a-RobKj@microsoft.com coded it

******************************************************************************/

#if !defined(DEVICE_PROFILE_UI)

#define DEVICE_PROFILE_UI

#include    "PropPage.H"
#include    "Profile.H"

/******************************************************************************

  CDeviceProfileManagement class

  This class provides the core services for these pages.

  NOTES:

  The profiles must be in a list box (not a combo box) with the ID ProfileList.
  If this isn't done, you must both override OnInit, and not call this class's
  OnInit function.

  Most implementations of derived classes will call this class's version of
  functions they override.  Whether they do that before or after they do their
  customizatins will probably require understanding what this class does.

******************************************************************************/

/*
 * m_bReadOnly == FALSE (default)
 * In this case the property page behaves normally - user input
 * is accepted.
 * m_bReadOnly == TRUE
 * In this case all the buttons for this page are greyed out and
 * the user can only inspect the data.
 *
 * The flag is used for locking out users without permission to
 * modify the settings - but still allows them to view the settings.
 *
 * m_bCMYK is true if the device is a printer and it supports
 * CMYK profiles
 */

#define DEVLIST_ONINIT   0x0001
#define DEVLIST_CHANGED  0x0002
#define DEVLIST_NOSELECT 0x0004

class CDeviceProfileManagement : public CShellExtensionPage {

    DWORD           m_dwType;       //  Type class of target device

protected:

    CUintArray      m_cuaRemovals;  //  indices of dissociations to be done
    CProfileArray   m_cpaAdds;      //  Profiles to be added
    CProfileArray   m_cpaProfile;   //  Associated Profile Names
    CString         m_csDevice;     //  Target Device Name
    HWND            m_hwndList;     //  Profile list box in dialog
    BOOL            m_bCMYK;        //  Printer support for CMYK
    BOOL            m_bReadOnly;    //  Flag indicating that settings can be
                                    //  modified by the user
    virtual void    InitList();
    virtual void    FillList(DWORD dwFlags = 0);

    void            GetDeviceTypeString(DWORD dwType,CString& csDeviceName);

public:

    CDeviceProfileManagement(LPCTSTR lpstrName, HINSTANCE hiWhere, int idPage,
                             DWORD  dwType);
    ~CDeviceProfileManagement() {}


    virtual BOOL    OnInit();
    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl);
    virtual BOOL    OnNotify(int idCtrl, LPNMHDR pnmh);
};

//  This class encapsulates the required "Add Profile" old-style file open
//  dialog.  Kind of a shame, Explorer's a much nicer interface...

class CAddProfileDialog {

    CStringArray csa_Files;

    static UINT_PTR APIENTRY OpenFileHookProc(HWND hDlg, UINT uMessage, WPARAM wp,
                                          LPARAM lp);

public:

    CAddProfileDialog(HWND hwndOwner, HINSTANCE hi);
    ~CAddProfileDialog() { csa_Files.Empty(); }

    unsigned  ProfileCount()              { return csa_Files.Count(); }
    LPCTSTR   ProfileName(unsigned u)     { return csa_Files[u]; }
    CString   ProfileNameAndExtension(unsigned u) 
                                          { return csa_Files[u].NameAndExtension(); }
    void      AddProfile(LPCTSTR str)     { csa_Files.Add(str); }
};

//  The Printer Profile Management class uses the core class pretty much as is.
//  We override the OnInit member to disable all controls if the user lacks
//  administrative authority for the target printer.

class CPrinterProfileManagement : public CDeviceProfileManagement {

protected:

    unsigned    m_uDefault;    //  Default profile index
    BOOL        m_bManualMode; //  Manual profile selection mode
    BOOL        m_bAdminAccess;
    BOOL        m_bLocalPrinter;

    virtual void    InitList();
    virtual void    FillList(DWORD dwFlags = 0);

public:

    CPrinterProfileManagement(LPCTSTR lpstrName, HINSTANCE hiWhere);
    ~CPrinterProfileManagement() {}

    virtual BOOL    OnInit();
    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl);
    virtual BOOL    OnNotify(int idCtrl, LPNMHDR pnmh);

    virtual BOOL    OnHelp(LPHELPINFO pHelp);
    virtual BOOL    OnContextMenu(HWND hwnd);
};

//  The Scanner Profile Management class uses the core class pretty much as is.
//  We override the OnInit member to disable all controls if the user lacks
//  administrative authority for the target printer.

class CScannerProfileManagement : public CDeviceProfileManagement {

public:

    CScannerProfileManagement(LPCTSTR lpstrName, HINSTANCE hiWhere);
    ~CScannerProfileManagement() {}

    virtual BOOL    OnInit();

    virtual BOOL    OnHelp(LPHELPINFO pHelp);
    virtual BOOL    OnContextMenu(HWND hwnd);
};

//  The monitor profile class is a bit more complex, as it allows the
//  manipulation and setting of device default profiles as well as association
//  and dissociation of profiles.  It also has some extra controls to
//  initialize.

class CMonitorProfileManagement : public CDeviceProfileManagement {

protected:

    unsigned    m_uDefault;              //  Default profile index
    CString     m_csDeviceFriendlyName;  //  Target Device Friendly Name

    virtual void    InitList();
    virtual void    FillList(DWORD dwFlags = 0);

public:

    CMonitorProfileManagement(LPCTSTR lpstrName, LPCTSTR lpstrFriendlyName, HINSTANCE hiWhere);
    ~CMonitorProfileManagement() {}

    virtual BOOL    OnInit();
    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl);
    virtual BOOL    OnNotify(int idCtrl, LPNMHDR pnmh);

    virtual BOOL    OnHelp(LPHELPINFO pHelp);
    virtual BOOL    OnContextMenu(HWND hwnd);
};

#endif
