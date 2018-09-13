//----------------------------------------------------------------------------
//
// prsht.h  - PropSheet definitions
//
// Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved
//
//----------------------------------------------------------------------------

#ifndef _PRSHT_H_
#define _PRSHT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAXPROPPAGES 24

struct _PSP;
typedef struct _PSP FAR* HPROPSHEETPAGE;

typedef struct _PROPSHEETPAGE FAR *LPPROPSHEETPAGE;     // forward declaration

//
// Property sheet page helper function
//
typedef void (CALLBACK FAR * LPFNRELEASEPROPSHEETPAGE)(LPPROPSHEETPAGE ppsp);

#define PSP_DEFAULT             0x0000
#define PSP_DLGINDIRECT         0x0001 // use pResource instead of pszTemplate
#define PSP_USEHICON            0x0002
#define PSP_USEICONID           0x0004
#define PSP_USETITLE            0x0008
#define PSP_USERELEASEFUNC      0x0020
#define PSP_USEREFPARENT        0x0040

// this structure is passed to CreatePropertySheetPage() and is in the LPARAM on
// of the WM_INITDIALOG message when a property sheet page is created
typedef struct _PROPSHEETPAGE {
        DWORD           dwSize;             // size of this structure (including extra data)
        DWORD           dwFlags;            // PSP_ bits define the use and meaning of fields
        HINSTANCE       hInstance;	    // instance to load the template from
        union {
            LPCSTR          pszTemplate;    // template to use
#ifdef WIN32
            LPCDLGTEMPLATE  pResource;      // PSP_DLGINDIRECT: pointer to resource in memory
#else
            const VOID FAR *pResource;	    // PSP_DLGINDIRECT: pointer to resource in memory
#endif
        };
        union {
            HICON       hIcon;              // PSP_USEICON: hIcon to use
            LPCSTR      pszIcon;            // PSP_USEICONID: or icon name string or icon id
        };
        LPCSTR          pszTitle;	    // name to override the template title or string id
        DLGPROC         pfnDlgProc;	    // dlg proc
        LPARAM          lParam;		    // user data
        LPFNRELEASEPROPSHEETPAGE pfnRelease;// PSP_USERELEASEFUNC: function will be called before HPROPSHEETPAGE is destroyed
        UINT FAR * pcRefParent;		    // PSP_USERREFPARENT: pointer to ref count variable
} PROPSHEETPAGE, FAR *LPPROPSHEETPAGE;
typedef const PROPSHEETPAGE FAR *LPCPROPSHEETPAGE;

#define PSH_DEFAULT             0x0000
#define PSH_PROPTITLE           0x0001 // use "Properties for <lpszCaption>" as the title
#define PSH_USEHICON            0x0002 // use specified hIcon for the caption
#define PSH_USEICONID           0x0004 // use lpszIcon to load the icon
#define PSH_PROPSHEETPAGE       0x0008 // use ppsp instead of phpage (points to array of PROPSHEETPAGE structures)
#define PSH_MULTILINETABS	0x0010 // do multiline tabs

typedef struct _PROPSHEETHEADER {
        DWORD           dwSize;         // size of this structure
        DWORD           dwFlags;        // PSH_
        HWND            hwndParent;
        HINSTANCE       hInstance;      // to load icon or caption string
        union {
            HICON       hIcon;          // PSH_USEHICON: hIcon to use
            LPCSTR      pszIcon;        // PSH_USEICONID: or icon name string or icon id
        };
        LPCSTR          pszCaption;	// PSH_PROPTITLE: dlg caption or "Properties for <lpszCaption>"
					// may be MAKEINTRESOURCE()

        UINT            nPages;	        // # of HPROPSHEETPAGE (or PROPSHEETPAGE) elements in phpage
        UINT            nStartPage;	// initial page to be shown (zero based)
        union {
            LPCPROPSHEETPAGE ppsp;
            HPROPSHEETPAGE FAR *phpage;
        };
} PROPSHEETHEADER, FAR *LPPROPSHEETHEADER;
typedef const PROPSHEETHEADER FAR *LPCPROPSHEETHEADER;


//
// property sheet APIs
//

HPROPSHEETPAGE WINAPI CreatePropertySheetPage(LPCPROPSHEETPAGE);
BOOL           WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE);
int            WINAPI PropertySheet(LPCPROPSHEETHEADER);
#ifdef WIN32
#endif
//
// callback for property sheet extensions to call to add pages
//
typedef BOOL (CALLBACK FAR * LPFNADDPROPSHEETPAGE)(HPROPSHEETPAGE, LPARAM);

//
// generic routine for prop sheet extensions to export.  this is called
// to have the extension add pages.  specific versions of this will be
// implemented when necessary.
//

typedef BOOL (CALLBACK FAR * LPFNADDPROPSHEETPAGES)(LPVOID, LPFNADDPROPSHEETPAGE, LPARAM);





#define PSN_FIRST       (0U-200U)
#define PSN_LAST        (0U-299U)


// PropertySheet notification codes sent to the page.  NOTE: RESULTS
// MUST BE RETURNED BY USING SetWindowLong(hdlg, DWL_MSGRESULT, result)

// page is being activated. initialize the data on the page here if other pages can
// effect this page, otherwise init the page at WM_INITDIALOG time. return value is
// ignored.
#define PSN_SETACTIVE           (PSN_FIRST-0)

// indicates the current page is being switched away from.  validate input
// at this time and return TRUE to keep the page switch from happening.
// to commit changes on page switches commit data after validating on this message.
#define PSN_KILLACTIVE          (PSN_FIRST-1)
// #define PSN_VALIDATE            (PSN_FIRST-1)

// indicates that the OK or Apply Now buttons have been pressed (OK will
// destroy the dialog when done)
// return FALSE to force the page to be destroyed and recreated.
#define PSN_APPLY               (PSN_FIRST-2)

// indicates that the cancel button has been pressed, the page may want use this
// as an oportunity to confirm canceling the dialog.
#define PSN_RESET               (PSN_FIRST-3)
// #define PSN_CANCEL              (PSN_FIRST-3)

// sent to page to see if the help button should be enabled, the page
// should return TRUE or FALSE
#define PSN_HASHELP             (PSN_FIRST-4)

// sent to page indicating that the help button has been pressed
#define PSN_HELP                (PSN_FIRST-5)




//// MESSAGES sent to the main property sheet dialog

// used to set the current selection
// supply either the hpage or the index to the tab
#define PSM_SETCURSEL           (WM_USER + 101)
#define PropSheet_SetCurSel(hDlg, hpage, index) \
        SendMessage(hDlg, PSM_SETCURSEL, (WPARAM)index, (LPARAM)hpage)

// NOT IMPLEMENTED
// remove a page
// wParam = index of page to remove
// lParam = hwnd of page to remove
// NOT IMPLEMENTED
#define PSM_REMOVEPAGE          (WM_USER + 102)
#define PropSheet_RemovePage(hDlg, index, hpage) \
        SendMessage(hDlg, PSM_REMOVEPAGE, index, (LPARAM)hpage)

// NOT IMPLEMENTED
// add a page
// lParam = hPage of page to remove
// NOT IMPLEMENTED
#define PSM_ADDPAGE             (WM_USER + 103)
#define PropSheet_AddPage(hDlg, hpage) \
        SendMessage(hDlg, PSM_ADDPAGE, 0, (LPARAM)hpage)

// tell the PS manager that that the page has changed and "Apply Now" should be enabled
// (we may mark the visually tab so the user knows that a change has been made)
#define PSM_CHANGED             (WM_USER + 104)
#define PropSheet_Changed(hDlg, hwnd) \
        SendMessage(hDlg, PSM_CHANGED, (WPARAM)hwnd, 0L)


// tell the PS manager that we need to restart windows due to a change made so
// the restart windows dialog will be presented when dismissing the dialog
#define PSM_RESTARTWINDOWS            (WM_USER + 105)
#define PropSheet_RestartWindows(hDlg) \
        SendMessage(hDlg, PSM_RESTARTWINDOWS, 0, 0L)

// tell the PS manager that we need to reboot due to a change made so
// the reboot windows dialog will be presented when dismissing the dialog
#define PSM_REBOOTSYSTEM              (WM_USER + 106)
#define PropSheet_RebootSystem(hDlg) \
        SendMessage(hDlg, PSM_REBOOTSYSTEM, 0, 0L)

// change the OK button to Close and disable cancel.  this indicates a non cancelable
// change has been made
#define PSM_CANCELTOCLOSE       (WM_USER + 107)
#define PropSheet_CancelToClose(hDlg) \
        SendMessage(hDlg, PSM_CANCELTOCLOSE, 0, 0L)

// have the PS manager forward this query to each initialized tab's hwnd
// until a non-zero value is returned.  This value is returned to the caller.
#define PSM_QUERYSIBLINGS       (WM_USER + 108)
#define PropSheet_QuerySiblings(hDlg, wParam, lParam) \
        SendMessage(hDlg, PSM_QUERYSIBLINGS, wParam, lParam)

// tell the PS manager the opposite of PSM_CHANGED -- that the page has reverted
// to its previously saved state.  If no pages remain changed, "Apply Now"
// will be disabled.  (we may remove the visually marked tab so that the user
// knows no change has been made)
#define PSM_UNCHANGED           (WM_USER + 109)
#define PropSheet_UnChanged(hDlg, hwnd) \
        SendMessage(hDlg, PSM_UNCHANGED, (WPARAM)hwnd, 0L)

// tell the PS manager to do an "Apply Now"
#define PSM_APPLY               (WM_USER + 110)
#define PropSheet_Apply(hDlg) \
        SendMessage(hDlg, PSM_APPLY, 0, 0L)

// iStyle can be PSH_PROPTITLE or PSH_DEFAULT
// lpszText can be a string or an rcid
#define PSM_SETTITLE            (WM_USER + 111)
#define PropSheet_SetTitle(hDlg, wStyle, lpszText)\
        SendMessage(hDlg, PSM_SETTITLE, wStyle, (LPARAM)(LPCSTR)lpszText)


#define ID_PSRESTARTWINDOWS 0x2
#define ID_PSREBOOTSYSTEM   (ID_PSRESTARTWINDOWS | 0x1)

#ifdef __cplusplus
} /* end of 'extern "C" {' */
#endif

#endif // _PRSHT_H_
