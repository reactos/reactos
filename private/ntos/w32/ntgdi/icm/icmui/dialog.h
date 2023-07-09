/******************************************************************************

  Header File:  Dialog.H

  This defines the C++ class used to encapsulate dialogs.  It supports both
  modal and modeless styles.  This class uses a static method for the dialog
  procedure, which automatically caches the "this" pointer for the class in the
  DWL_USER field of the window's internal structure for the dialog.  This
  hand-off is accomplished by setting the lParam parameter on a DialogBoxParam
  or CreateDialogParam call to the "this" pointer.  It also saves the dialog
  handle in a protected member for easy access from derived classes.

  To create a C++ class for any specific dialog, derive the class from this
  class, providing the dialog ID and instance handle needed to get the dialog
  resource in the derived class constructor.  Also provide the parent window
  handle, if there is one.

  The dialog procedure then provides virtual functions for Windows messages
  of interest.  I've added these as needed.  If I were going to a truly
  universal class of this sort, I'd just as well go to MFC, and save the
  debugging time, so this approach seems reasonable to me.

  12-11-96- to support the hook procedure used in the application UI, I've
  added two protected members to allow this support to be in the base class.
  If there is a hook procedure, it gets first chance at all messages except
  WM_INITDIALOG.  If it returns TRUE, we do no further processing, otherwise
  we will call the various overrides, if applicable.

  For WM_INITDIALOG, we call any overrides first.  The derived class' OnInit
  procedure can then supply an LPARAM for the hook procedure (e.g., a pointer
  to some relevant class member), if desired.  We handle the returns from the
  calls so that if either an override or the hook procedure states it has
  altered the focus, we return the appropriate value.  This is pretty standard
  handling for dialog hooks, so it should serve well, and it's almost 0 code.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version
  12-11-96  a-robkj@microsoft.com   Added hook procedure support

******************************************************************************/

#if !defined(DIALOG_STUFF)

#define DIALOG_STUFF

class CDialog {

    static INT_PTR CALLBACK    DialogProc(HWND hwndPage, UINT uMsg, WPARAM wp,
                                       LPARAM lp);
    int         m_idMain;
    BOOL        m_bIsModal;

protected:

    //  These should be protected (accessible only from derived classes)
    //  The instance is available for string table or other resource access

    HWND        m_hwndParent;
    HWND        m_hwnd;
    HINSTANCE   m_hiWhere;
    DLGPROC     m_dpHook;       //  Dialog Hook Procedure
    LPARAM      m_lpHook;       //  LPARAM for Hook WM_INITDIALOG call

public:

    //  Our constructor requires the resource ID and instance.  Not
    //  unreasonable for this project.

    CDialog(HINSTANCE hiWhere, int id, HWND hwndParent = NULL);
    CDialog(CDialog& cdOwner, int id);
    ~CDialog();

    //  Modal dialog box operation

    LONG    DoModal();

    //  Modeless dialog boxes- create and destroy.  We only allow one
    //  modeless DB per instance of this class.

    void    Create();   //  For modeless boxes
    void    Destroy();

    //  This allows us to adjust the position of a dialog in its parent window.
    //  We use only the left and top members of the given rectangle.

    void    Adjust(RECT& rc);

    //  Windows message overrides

    //  WM_COMMAND- control notifications.  We assure you can always get out
    //  of a modal dialog by pressing any button to make prototyping easy.

    virtual BOOL OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl) {
        //  Call EndDialog for all BN_CLICKED messages on Modal boxes
        if  (m_bIsModal && wNotifyCode == BN_CLICKED)
            EndDialog(m_hwnd, wid);
        return  FALSE;
    }

    //  WM_NOTIFY- common control notifications

    virtual BOOL    OnNotify(int idCtrl, LPNMHDR pnmh) {
        return  FALSE;
    }

    //  WM_INITDIALOG- before being called, this and m_hwnd will be valid.

    virtual BOOL    OnInit() { return TRUE; }

    //  WM_HELP and WM_CONTEXTMENU- for context-sensitive help.

    virtual BOOL    OnHelp(LPHELPINFO pHelp) { return TRUE; }
    virtual BOOL    OnContextMenu(HWND hwnd) { return TRUE; }
};

#endif
