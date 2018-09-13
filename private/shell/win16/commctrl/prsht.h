//----------------------------------------------------------------------------
//
// prsht.h  - PropSheet definitions
//
// Copyright (c) 1993-1995, Microsoft Corp.	All rights reserved
//
//----------------------------------------------------------------------------

#ifndef _PRSHT_H_
#define _PRSHT_H_

//
// Define API decoration for direct importing of DLL references.
//  BUGBUG: Exact same block is in commctrl.h	/* ;Internal */
//
#ifndef WINCOMMCTRLAPI
#if !defined(_COMCTL32_) && defined(_WIN32)
#define WINCOMMCTRLAPI DECLSPEC_IMPORT
#else
#define WINCOMMCTRLAPI
#endif
#endif // WINCOMMCTRLAPI

//
// For compilers that don't support nameless unions
//  BUGBUG: Exact same block is in commctrl.h	/* ;Internal */
//
#ifndef DUMMYUNIONNAME
#ifdef NONAMELESSUNION
#define DUMMYUNIONNAME	 u
#define DUMMYUNIONNAME2  u2
#define DUMMYUNIONNAME3  u3
#else
#define DUMMYUNIONNAME	
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#endif
#endif // DUMMYUNIONNAME

#ifdef __cplusplus
extern "C" {
#endif

#define MAXPROPPAGES 100

struct _PSP;
typedef struct _PSP FAR* HPROPSHEETPAGE;

typedef struct _PROPSHEETPAGE FAR *LPPROPSHEETPAGE;     // forward declaration

//
// Property sheet page helper function
//

typedef UINT (CALLBACK FAR * LPFNPSPCALLBACK)(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

#define PSP_DEFAULT             0x0000
#define PSP_DLGINDIRECT         0x0001 // use pResource instead of pszTemplate
#define PSP_USEHICON            0x0002
#define PSP_USEICONID           0x0004
#define PSP_USETITLE		0x0008
#define PSP_RTLREADING          0x0010 // MidEast versions only

#define PSP_HASHELP		0x0020
#define PSP_USEREFPARENT	0x0040
#define PSP_USECALLBACK         0x0080
#define PSP_ALL 		0x00FF // ;Internal
#define PSP_IS16                0x8000 // ;Internal

#define PSPCB_RELEASE           1
#define PSPCB_CREATE            2


// this structure is passed to CreatePropertySheetPage() and is in the LPARAM on
// of the WM_INITDIALOG message when a property sheet page is created
typedef struct _PROPSHEETPAGE {
        DWORD           dwSize;             // size of this structure (including extra data)
        DWORD           dwFlags;            // PSP_ bits define the use and meaning of fields
        HINSTANCE       hInstance;	    // instance to load the template from
        union {
            LPCSTR          pszTemplate;    // template to use
#ifdef _WIN32
            LPCDLGTEMPLATE  pResource;      // PSP_DLGINDIRECT: pointer to resource in memory
#else
            const VOID FAR *pResource;	    // PSP_DLGINDIRECT: pointer to resource in memory
#endif
        } DUMMYUNIONNAME;
        union {
            HICON       hIcon;              // PSP_USEICON: hIcon to use
            LPCSTR      pszIcon;            // PSP_USEICONID: or icon name string or icon id
        } DUMMYUNIONNAME2;
        LPCSTR          pszTitle;	    // name to override the template title or string id
        DLGPROC         pfnDlgProc;	    // dlg proc
        LPARAM          lParam;		    // user data
        LPFNPSPCALLBACK pfnCallback;        // if PSP_USECALLBACK this is called with PSPCB_* msgs
        UINT FAR * pcRefParent;		    // PSP_USERREFPARENT: pointer to ref count variable
} PROPSHEETPAGE;
typedef const PROPSHEETPAGE FAR *LPCPROPSHEETPAGE;

#define PSH_DEFAULT             0x0000
#define PSH_PROPTITLE           0x0001 // use "Properties for <lpszCaption>" as the title
#define PSH_USEHICON            0x0002 // use specified hIcon for the caption
#define PSH_USEICONID           0x0004 // use lpszIcon to load the icon
#define PSH_PROPSHEETPAGE       0x0008 // use ppsp instead of phpage (points to array of PROPSHEETPAGE structures)
#define PSH_MULTILINETABS	0x0010 // do multiline tabs   // ;Internal
#define PSH_WIZARD		0x0020 // Wizard
#define PSH_USEPSTARTPAGE	0x0040 // use pStartPage for starting page
#define PSH_NOAPPLYNOW          0x0080 // Remove Apply Now button
#define PSH_USECALLBACK 	0x0100 // pfnCallback is valid
#define PSH_HASHELP		0x0200 // Display help button
#define PSH_MODELESS		0x0400 // modless property sheet, PropertySheet returns HWND
#define PSH_RTLREADING  0x0800 // MidEast versions only
#define PSH_ALL 		0x0FFF // ;Internal

typedef int (CALLBACK *PFNPROPSHEETCALLBACK)(HWND, UINT, LPARAM);

typedef struct _PROPSHEETHEADER {
        DWORD           dwSize;         // size of this structure
        DWORD           dwFlags;        // PSH_
        HWND            hwndParent;
        HINSTANCE       hInstance;      // to load icon, caption or page string
        union {
            HICON       hIcon;          // PSH_USEHICON: hIcon to use
            LPCSTR      pszIcon;        // PSH_USEICONID: or icon name string or icon id
        } DUMMYUNIONNAME;
        LPCSTR          pszCaption;	// PSH_PROPTITLE: dlg caption or "Properties for <lpszCaption>"
					// may be MAKEINTRESOURCE()

        UINT            nPages;	        // # of HPROPSHEETPAGE (or PROPSHEETPAGE) elements in phpage
	union {
	    UINT        nStartPage;	// !PSH_USEPSTARTPAGE: page number (0-based)
	    LPCSTR      pStartPage;	// PSH_USEPSTARTPAGE: name of page or string id
	} DUMMYUNIONNAME2;
        union {
            LPCPROPSHEETPAGE ppsp;
            HPROPSHEETPAGE FAR *phpage;
        } DUMMYUNIONNAME3;
        PFNPROPSHEETCALLBACK pfnCallback;
} PROPSHEETHEADER, FAR *LPPROPSHEETHEADER;
typedef const PROPSHEETHEADER FAR *LPCPROPSHEETHEADER;

//
// pfnCallback message values
//

#define PSCB_INITIALIZED  1
#define PSCB_PRECREATE    2

//
// property sheet APIs
//

WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreatePropertySheetPage(LPCPROPSHEETPAGE);
WINCOMMCTRLAPI BOOL           WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE);
WINCOMMCTRLAPI int            WINAPI PropertySheet(LPCPROPSHEETHEADER);
#ifdef _WIN32										/* ;Internal */
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreateProxyPage32Ex(HPROPSHEETPAGE hpage16, HINSTANCE hinst16);	/* ;Internal */
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreateProxyPage(HPROPSHEETPAGE hpage16, HINSTANCE hinst16);	/* ;Internal */
#endif											/* ;Internal */
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


typedef struct _PSHNOTIFY
{
    NMHDR hdr;
    LPARAM lParam;
} PSHNOTIFY, FAR *LPPSHNOTIFY;


// ;Internal these need to match shell.h's ranges
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
// pshnotify's lparam is true if this was from an IDOK, false if it's from applynow
// return TRUE or PSNRET_INVALID to abort the save
#define PSN_APPLY               (PSN_FIRST-2)

// indicates that the cancel button has been pressed, the page may want use this
// as an oportunity to confirm canceling the dialog.
// pshnotify's lparam is true if it was done by system close button, false if it's from idcancel button
#define PSN_RESET               (PSN_FIRST-3)
// #define PSN_CANCEL              (PSN_FIRST-3)

// sent to page to see if the help button should be enabled, the page   // ;Internal
// should return TRUE or FALSE                                           // ;Internal
#define PSN_HASHELP             (PSN_FIRST-4)                             // ;Internal

// sent to page indicating that the help button has been pressed
#define PSN_HELP                (PSN_FIRST-5)

// sent to wizard sheets only
#define PSN_WIZBACK		(PSN_FIRST-6)
#define PSN_WIZNEXT		(PSN_FIRST-7)
#define PSN_WIZFINISH		(PSN_FIRST-8)

// called sheet can reject a cancel by returning non-zero
#define PSN_QUERYCANCEL 	(PSN_FIRST-9)


// results that may be returned:
#define PSNRET_NOERROR              0
#define PSNRET_INVALID              1
#define PSNRET_INVALID_NOCHANGEPAGE 2


//// MESSAGES sent to the main property sheet dialog

// used to set the current selection
// supply either the hpage or the index to the tab
#define PSM_SETCURSEL           (WM_USER + 101)
#define PropSheet_SetCurSel(hDlg, hpage, index) \
        SendMessage(hDlg, PSM_SETCURSEL, (WPARAM)index, (LPARAM)hpage)

// remove a page
// wParam = index of page to remove
// lParam = hwnd of page to remove
#define PSM_REMOVEPAGE          (WM_USER + 102)
#define PropSheet_RemovePage(hDlg, index, hpage) \
        SendMessage(hDlg, PSM_REMOVEPAGE, index, (LPARAM)hpage)

// add a page
// lParam = hPage of page to remove
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

// tell the PS manager which wizard buttons to enable.
#define PSM_SETWIZBUTTONS	(WM_USER + 112)
#define PropSheet_SetWizButtons(hDlg, dwFlags) \
	PostMessage(hDlg, PSM_SETWIZBUTTONS, 0, (LPARAM)dwFlags)
#define PropSheet_SetWizButtonsNow(hDlg, dwFlags) PropSheet_SetWizButtons(hDlg, dwFlags) /* ;Internal */

#define PSWIZB_BACK		0x00000001
#define PSWIZB_NEXT		0x00000002
#define PSWIZB_FINISH		0x00000004
#define PSWIZB_DISABLEDFINISH	0x00000008	// Show disabled finish button

// press a button automagically
#define PSM_PRESSBUTTON 	(WM_USER + 113)
#define PropSheet_PressButton(hDlg, iButton) \
	SendMessage(hDlg, PSM_PRESSBUTTON, (WPARAM)iButton, 0)

#define PSBTN_BACK	0
#define PSBTN_NEXT	1
#define PSBTN_FINISH	2
#define PSBTN_OK	3
#define PSBTN_APPLYNOW	4
#define PSBTN_CANCEL	5
#define PSBTN_HELP	6
#define PSBTN_MAX	6	//;Internal


// used to set the current selection by supplying the resource ID
// supply either the hpage or the index to the tab
#define PSM_SETCURSELID 	(WM_USER + 114)
#define PropSheet_SetCurSelByID(hDlg, id) \
	SendMessage(hDlg, PSM_SETCURSELID, 0, (LPARAM)id)


//
//  Force the "Finish" button to be enabled and change
//  the text to the specified string.  The Back button will be hidden.
//
#define PSM_SETFINISHTEXT	(WM_USER + 115)
#define PropSheet_SetFinishText(hDlg, lpszText) \
	SendMessage(hDlg, PSM_SETFINISHTEXT, 0, (LPARAM)lpszText)


// returns the tab control
#define PSM_GETTABCONTROL       (WM_USER + 116)
#define PropSheet_GetTabControl(hDlg) \
        (HWND)SendMessage(hDlg, PSM_GETTABCONTROL, 0, 0)

#define PSM_ISDIALOGMESSAGE	(WM_USER + 117)
#define PropSheet_IsDialogMessage(hDlg, pMsg) \
        (BOOL)SendMessage(hDlg, PSM_ISDIALOGMESSAGE, 0, (LPARAM)pMsg)

#define PSM_GETCURRENTPAGEHWND  (WM_USER + 118)
#define PropSheet_GetCurrentPageHwnd(hDlg) \
        (HWND)SendMessage(hDlg, PSM_GETCURRENTPAGEHWND, 0, 0L)

#define ID_PSRESTARTWINDOWS 0x2
#define ID_PSREBOOTSYSTEM   (ID_PSRESTARTWINDOWS | 0x1)

//
//  Standard sizes for wizard sheet dialog templates.  Use these sizes to create
//  wizards that conform to the Windows standard.
//
#define WIZ_CXDLG 276
#define WIZ_CYDLG 140

#define WIZ_CXBMP 80	    // Org at 0,0 -- Use WIZ_CYDLG for height

#define WIZ_BODYX 92	    // Y org is 0
#define WIZ_BODYCX 184

//
//  Standard sizes for property sheet dialog templates.  Use these
//  property sheets that conform to the Windows standard.
//
#define PROP_SM_CXDLG	212	// small
#define PROP_SM_CYDLG	188

#define PROP_MED_CXDLG	227	// medium
#define PROP_MED_CYDLG	215	// some are 200

#define PROP_LG_CXDLG	252	// large
#define PROP_LG_CYDLG	218

#ifdef __cplusplus
} /* end of 'extern "C" {' */
#endif

#endif // _PRSHT_H_
