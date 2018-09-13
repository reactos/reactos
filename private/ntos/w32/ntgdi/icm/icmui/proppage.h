/******************************************************************************

  Header File:  Property Page.H

  This defines the C++ class used to encapsulate property pages.  This class
  has a static method for the dialog procedure, which automatically caches the
  "this" pointer for the class in the DWL_USER field of the windows internal
  structure for the dialog used for the property page.  This hand-off is
  accomplished by setting the lParam field of the PROPSHEETPAGE structure to
  the "this" pointer.  It also saves the dialog handle in a protected member
  for easy access from derived classes.

  To create a C++ class for any specific property sheet, derive the class
  from this class, providing the dialog ID and instance handle needed to get
  the resource in the m_psp member.

  The dialog procedure then provides virtual functions for Windows messages
  of interest.  I've added these as needed.  If I were going to a truly 
  universal class of this sort, I'd just as well go to MFC, and save the 
  debugging time, so this approach seems reasonable to me.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version

******************************************************************************/

#if !defined(PROPERTY_PAGE)

#define PROPERTY_PAGE

//  CPropertyPage class- abstracts a property page for us

class CPropertyPage {

    //  Basic dialog procedure for all derived classes

    static BOOL CALLBACK    DialogProc(HWND hwndPage, UINT uMsg, WPARAM wp,
                                       LPARAM lp);

    //  These elements should be protected (only available to derived classes)

protected:
    PROPSHEETPAGE   m_psp;
    HWND            m_hwnd, m_hwndSheet;
    HPROPSHEETPAGE  m_hpsp;

    BOOL            m_bChanged;

public:

    CPropertyPage();    //  Default Constructor
    virtual ~CPropertyPage() {}

    HPROPSHEETPAGE  Handle();   //  Calls CreatePropertySheetPage, if needed

    VOID            EnableApplyButton() {
        SendMessage(m_hwndSheet, PSM_CHANGED, (WPARAM) m_hwnd, 0);
    }

    VOID            DisableApplyButton() {
        SendMessage(m_hwndSheet, PSM_UNCHANGED, (WPARAM) m_hwnd, 0);
    }

    BOOL            SettingChanged()    {
        return m_bChanged;
    }

    VOID            SettingChanged(BOOL b) {
        m_bChanged = b;
    }

    //  virtual functions- these get redefined on an as needed basis for
    //  any specialized handling desired by any derived classes.

    //  The default handling allows one to initially display the sheet with
    //  no coding beyond the constructor for the derived class

    virtual BOOL    OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl) {
        return FALSE;
    }

    virtual BOOL    OnNotify(int idCtrl, LPNMHDR pnmh) {
        return  FALSE;
    }

    virtual BOOL    OnInit() { return TRUE; }

    virtual BOOL    OnDestroy() { return FALSE; }

    virtual BOOL    OnHelp(LPHELPINFO pHelp) { return TRUE; }
    virtual BOOL    OnContextMenu(HWND hwnd) { return TRUE; }
};

/******************************************************************************

  Shell Extension property page class

  Noteworthy details:

  These pages are displayed by the shell.  The thread of execution is such that
  we create the page, then return to the shell.  The shell will then attempt to
  unload the extension.  It will query CanDllUnloadNow to do this.  Since
  freeing the library frees the page template and dialog procedure, we can't
  allow this to happen while any instances of this class exist.

  However, the shell doesn't know this is a class, so it won't destroy it.

  What I've done is build a circular chain of all of the instances of this
  class, anchored in a private static class member.  A public static method
  (OKToClose) then walks the chain.  If an instance's window handle is no
  longer valid, then the shell has finished with it, and we delete it.  The
  criterion for closing then becomes not finding a valid handle (so we don't
  delay unloading by any lazy evaluation, such as requiring an empty chain
  on entry).

  All Property pages displayed by a property sheet extension should be derived
  from this class.

  While a mechanism is provided by property sheets for a reference count
  maintenance mechanism, this mechanism will not call any class destructor-
  this could lead to memory leaks, which is why I've chosen this method.

******************************************************************************/

class CShellExtensionPage: public CPropertyPage {

    static  CShellExtensionPage *m_pcsepAnchor; //  Anchor the chain of these

    CShellExtensionPage *m_pcsepPrevious, *m_pcsepNext;

public:

    CShellExtensionPage();
    ~CShellExtensionPage();

    static BOOL OKToClose();
};


#endif  //  Keep us from getting multiply defined
