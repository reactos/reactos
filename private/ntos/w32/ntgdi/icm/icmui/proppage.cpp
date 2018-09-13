/******************************************************************************

  Source File:  Property Page.CPP

  Implements the CPropertyPage class.  See the associated header file for
  details.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version
  12-04-96  a-robkj@microsoft.com   retrieve handle to sheet, Create a derived
                                    class for shell extension pages

******************************************************************************/

#include    "ICMUI.H"

//  CPropertyPage member functions

//  Class constructor- basic initializations.  Any remaining PROPSHEETPAGE
//  initializations are expected to be done by the derived class.

CPropertyPage::CPropertyPage() {
    m_psp.dwSize = sizeof m_psp;
    m_psp.pfnDlgProc = (DLGPROC) DialogProc;
    m_psp.lParam = (LPARAM) this;
    m_psp.dwFlags = PSP_DEFAULT;    //  Can be overriden later
	m_hpsp = NULL;
    m_bChanged = FALSE;
}

//  Handle retrieval- I'll admit an addiction to one-liners

HPROPSHEETPAGE  CPropertyPage::Handle() {
    return m_hpsp = (m_hpsp ? m_hpsp : CreatePropertySheetPage(&m_psp));
}

//  Dialog Procedure

BOOL CALLBACK   CPropertyPage::DialogProc(HWND hwndPage, UINT uMsg, WPARAM wp,
                                          LPARAM lp) {

    CPropertyPage   *pcppMe =
        (CPropertyPage *) GetWindowLongPtr(hwndPage, DWLP_USER);

    switch  (uMsg) {

        case    WM_INITDIALOG:

            //  In this case, lp points to the PROPSHEETHEADER that created
            //  us.  We look into its lParam member for our this pointer,
            //  and store this in the dialog's private data.  This lets us
            //  use the pointer to get at all of our overridable functions.

            pcppMe = (CPropertyPage *) ((LPPROPSHEETPAGE) lp) -> lParam;

            SetWindowLongPtr(hwndPage, DWLP_USER, (LONG_PTR) pcppMe);
            pcppMe -> m_hwnd = hwndPage;

            //  Now, see if the derived class has any initialization needs

            return  pcppMe -> OnInit();

        //  Overridable processing for standard control notifications

        case    WM_COMMAND:

            return  pcppMe -> OnCommand(HIWORD(wp), LOWORD(wp), (HWND) lp);

        case    WM_DESTROY:

            return  pcppMe -> OnDestroy();

        case    WM_HELP:

            return  pcppMe -> OnHelp((LPHELPINFO) lp);

        case    WM_CONTEXTMENU:

            return  pcppMe -> OnContextMenu((HWND) wp);

        //  Overridable processing for common control notifications

        case    WM_NOTIFY:  {

            //  If the message is PSM_SETACTIVE, note the property sheet hwnd

            LPNMHDR pnmh = (LPNMHDR) lp;

            if  (pnmh -> code == PSN_SETACTIVE)
                pcppMe -> m_hwndSheet = pnmh -> hwndFrom;
            return  pcppMe -> OnNotify((int) wp, pnmh);
        }

    }

    return  FALSE;  //  We didn't handle the message.
}

//  CShellExtensionPage class member methods

CShellExtensionPage *CShellExtensionPage::m_pcsepAnchor = NULL;

//  We enable reference counting, partially because on NT, the Window handle
//  sometimes appears invalid even while the dialog is up.  However, we also
//  keep the chaining mechanism, as this is the only sane way we have of
//  freeing up the object instances we've created.

CShellExtensionPage::CShellExtensionPage() {

    if  (m_pcsepAnchor) {

        // If there is a cell other than anchor, update its list.
        if (m_pcsepAnchor -> m_pcsepNext)
            m_pcsepAnchor -> m_pcsepNext -> m_pcsepPrevious = this;
        // Insert this cell right after the anchor.
        m_pcsepPrevious = m_pcsepAnchor;
        m_pcsepNext = m_pcsepAnchor -> m_pcsepNext;
        m_pcsepAnchor -> m_pcsepNext = this;
    }
    else {

        m_pcsepAnchor = this;
        m_pcsepNext = m_pcsepPrevious = NULL;
    }

    m_psp.pcRefParent = (UINT *) &CGlobals::ReferenceCounter();
    m_psp.dwFlags |= PSP_USEREFPARENT;
}

CShellExtensionPage::~CShellExtensionPage() {

    if  (this == m_pcsepAnchor) {
        m_pcsepAnchor = m_pcsepNext; 
        if (m_pcsepAnchor) {
            // Anchor never has previous.
            m_pcsepAnchor -> m_pcsepPrevious = NULL;
        }
    }
    else {
        m_pcsepPrevious -> m_pcsepNext = m_pcsepNext;
        // If there is other cell following this, update it.
        if (m_pcsepNext)
            m_pcsepNext -> m_pcsepPrevious = m_pcsepPrevious;
    }
}

//  This little ditty lets us decide when it's safe to unload the DLL- it also
//  guarantees all class destructors are called as sheets get closed by the
//  various potential callers.

BOOL    CShellExtensionPage::OKToClose() {

    while   (m_pcsepAnchor) {
        if  (IsWindow(m_pcsepAnchor -> m_hwnd))
            return  FALSE;  //  This page is still alive!

        delete  m_pcsepAnchor;  //  Page isn't alive, delete it...
    }

    return  TRUE;   //  No more pages allocated
}
