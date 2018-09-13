/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    AppMgr.c

Abstract:

    Abstract-for-module.

Author:

    Dave Hastings (daveh) creation-date-18-Apr-1997

Revision History:


--*/

#include <windows.h>
#include <cpl.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

VOID ShowPages(HWND);

BOOL CALLBACK 
AppMgrDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

PAGEINFO PageInfo[NUM_PAGES] = {
    {PSP_HIDEHEADER,                                // Welcome Page
        MAKEINTRESOURCE(IDD_WELCOME), 
        NULL, 
        NULL, 
        AppMgrDlg, 
        WelcomeInitProc, 
        WelcomeCommandProc,
        WelcomeNotifyProc,
        NULL,
        0,
        NULL
    }, 
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    // First modify page
        MAKEINTRESOURCE(IDD_MODIFY_PROGRAM),
        MAKEINTRESOURCE(IDS_MODIFYPROGRAM1),
        MAKEINTRESOURCE(IDS_MODIFYPROGRAM2),
        AppMgrDlg,
        ModifyProgramInitProc, 
        ModifyProgramCommandProc,
        ModifyProgramNotifyProc,
        ModifyProgramUpdateListViewProc,
        IDD_MODIFY_CANCEL,
        NULL
    },
    {PSP_HIDEHEADER,                                // modify Finish page
        MAKEINTRESOURCE(IDD_MODIFY_FINISH), 
        NULL, 
        NULL, 
        AppMgrDlg, 
        CancelInitProc, 
        NULL,
        ModifyFinishNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    // Add source page
        MAKEINTRESOURCE(IDD_ADD_SOURCE),
        MAKEINTRESOURCE(IDS_ADDSOURCE1),
        MAKEINTRESOURCE(IDS_ADDSOURCE2),
        AppMgrDlg,
        AddSourceInitProc,
        AddSourceCommandProc,
        AddSourceNotifyProc,
        NULL,
        IDD_ADD_CANCEL,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    // Media select page
        MAKEINTRESOURCE(IDD_ADD_MEDIASELECT),
        MAKEINTRESOURCE(IDS_MEDIASELECT1),
        MAKEINTRESOURCE(IDS_MEDIASELECT2),
        AppMgrDlg,
        NULL,
        AddMediaSelectCommandProc,
        AddMediaSelectNotifyProc,
        NULL,
        IDD_ADD_CANCEL,
        NULL
    },
    {PSP_HIDEHEADER,                                // Add finish page
        MAKEINTRESOURCE(IDD_FINISH_MEDIA),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        FinishMediaNotifyProc,
        NULL,
        IDD_ADD_CANCEL,
        NULL
    },
    {PSP_HIDEHEADER,                                // Add cancel page
        MAKEINTRESOURCE(IDD_ADD_CANCEL),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    // Media error page bugbug
        MAKEINTRESOURCE(IDD_ADD_NOMEDIA),
        MAKEINTRESOURCE(IDS_ADDNOMEDIA1),
        MAKEINTRESOURCE(IDS_ADDNOMEDIA2),
        AppMgrDlg,
        NULL,
        AddNoMediaCommandProc,
        AddNoMediaNotifyProc,
        NULL,
        IDD_FINISH_MEDIA_ERROR,
        NULL
    },
    {PSP_HIDEHEADER,                                // Add cancel after media error page
        MAKEINTRESOURCE(IDD_FINISH_MEDIA_ERROR),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_HIDEHEADER,                                // Add cancel after media error page
        MAKEINTRESOURCE(IDD_MODIFY_CANCEL),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    // Media error page bugbug
        MAKEINTRESOURCE(IDD_ADD_BROWSE),
        MAKEINTRESOURCE(IDS_ADDBROWSE1),
        MAKEINTRESOURCE(IDS_ADDBROWSE2),
        AppMgrDlg,
        NULL,
        AddBrowseCommandProc,
        AddBrowseNotifyProc,
        NULL,
        IDD_ADD_CANCEL_CORPNET,
        NULL
    },
    {PSP_HIDEHEADER,                                // Add cancel after media error page
        MAKEINTRESOURCE(IDD_ADD_CANCEL_CORPNET),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_HIDEHEADER,                                // Add Browse Finish page
        MAKEINTRESOURCE(IDD_ADD_FINISH_BROWSE),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        AddFinishBrowseNotifyProc,
        NULL,
        IDD_ADD_CANCEL_CORPNET,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    // Media error page bugbug
        MAKEINTRESOURCE(IDD_ADD_PROGRAM),
        MAKEINTRESOURCE(IDS_ADDPROGRAM1),
        MAKEINTRESOURCE(IDS_ADDPROGRAM2),
        AppMgrDlg,
        AddProgramInitProc,
        AddProgramCommandProc,
        AddProgramNotifyProc,
        NULL,
        IDD_ADD_CANCEL_CORPNET,
        NULL
    },
    {PSP_HIDEHEADER,                                // Add Corp Finish page
        MAKEINTRESOURCE(IDD_ADD_FINISH_CORP),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        AddFinishBrowseNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    // First modify page
        MAKEINTRESOURCE(IDD_REPAIR_PROGRAM),
        MAKEINTRESOURCE(IDS_REPAIRPROGRAM1),
        MAKEINTRESOURCE(IDS_REPAIRPROGRAM2),
        AppMgrDlg,
        RepairSelectInitProc, 
        RepairSelectCommandProc,
        RepairSelectNotifyProc,
        NULL,
        IDD_REPAIR_FINISH_CANCEL,
        NULL
    },
    {PSP_HIDEHEADER,                                // Repair Finish page
        MAKEINTRESOURCE(IDD_REPAIR_FINISH),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        AddFinishBrowseNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_HIDEHEADER,
        MAKEINTRESOURCE(IDD_REPAIR_FINISH_CANCEL),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,    
        MAKEINTRESOURCE(IDD_INTERNETSITE),
        MAKEINTRESOURCE(IDS_INTERNETSITE1),
        MAKEINTRESOURCE(IDS_INTERNETSITE2),
        AppMgrDlg,
        InternetSiteInitProc, 
        InternetSiteCommandProc,
        InternetSiteNotifyProc,
        NULL,
        IDD_ADD_CANCEL_INTERNET,
        NULL
    },
    {PSP_HIDEHEADER,
        MAKEINTRESOURCE(IDD_ADD_CANCEL_INTERNET),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_HIDEHEADER,
        MAKEINTRESOURCE(IDD_ADD_FINISH_INTERNET),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        AddFinishInternetNotifyProc,
        NULL,
        IDD_ADD_CANCEL_INTERNET,
        NULL
    },
    {PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE,
        MAKEINTRESOURCE(IDD_UPGRADE_PROGRAM),
        MAKEINTRESOURCE(IDS_PLACEHOLDER),
        MAKEINTRESOURCE(IDS_PLACEHOLDER),
        AppMgrDlg,
        UpgradeProgramInitProc, 
        UpgradeProgramCommandProc,
        UpgradeProgramNotifyProc,
        NULL,
        IDD_UPGRADE_CANCEL,
        NULL
    },
    {PSP_HIDEHEADER,
        MAKEINTRESOURCE(IDD_UPGRADE_CANCEL),
        NULL,
        NULL,
        AppMgrDlg,
        CancelInitProc,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    },
    {PSP_HIDEHEADER,
        MAKEINTRESOURCE(IDD_UPGRADE_FINISH),
        NULL,
        NULL,
        AppMgrDlg,
        NULL,
        NULL,
        CancelNotifyProc,
        NULL,
        0,
        NULL
    }
};

HINSTANCE Instance;
HFONT LargeTitleFont, SmallTitleFont;

BOOL APIENTRY 
LibMain(
    HANDLE hDll, 
    DWORD dwReason, 
    LPVOID lpReserved
    )
/*++

Routine Description:

    This is the dll initialization entry for the Application Manager.

Arguments:

    hDll -- Supplies the handle for our dll.
    dwReason -- Supplies the reason that the entrypoint was called.
    
Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        Instance = hDll;
    }

    return TRUE;
}

LONG APIENTRY
CPlApplet(
    HWND hwndCpl,
    UINT uMsg,
    LONG lParam1,
    LONG lParam2
    )
/*++

Routine Description:

    This routine is the entry called by the control panel.  For more complete
    documentation, see MSDN

Arguments:

    hwndCpl - Supplies the parent window (control panel).
    uMsg - Supplies the message (see MSDN).
    lParam1 - Supplies the first message parameter.
    lParam2 - Supplies the second message parameter.

Return Value:

    Depends on the message.

--*/
{
    LPCPLINFO CplInfo;

    switch (uMsg) {
        case CPL_INIT:
            //
            // Perform global initialization here
            //

            return SetUpFonts(
                Instance,
                hwndCpl,
                &LargeTitleFont,
                &SmallTitleFont
                );

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            //
            // Fill in CPLINFO
            //
            if (lParam1 > 0) {
                //
                // If someone is requesting info for a non-existent applet,
                // don't process the message
                //
#if DBG
                OutputDebugString(L"appmgr:  CPL_INQUIRE for non-existent applet\n");
#endif
                return FALSE;
            }

            CplInfo = (LPCPLINFO)lParam2;
            CplInfo->idIcon = IDI_ICON;
            CplInfo->idName = IDS_NAME;
            CplInfo->idInfo = IDS_INFO;
            CplInfo->lData = 0;

            return TRUE;

        case CPL_DBLCLK:
            //
            // Start the applet
            //
            InitializePolicy();
            ShowPages(hwndCpl);

            return TRUE;

        default:
            return FALSE;
    }
}

VOID
ShowPages(
    HWND Parent
    )
/*++

Routine Description:

    This routine initializes the wizard pages and displays the initial page.

Arguments:

    Parent - Supplies the handle of the parent window for the property page.

Return Value:

    None.

--*/

{
    PROPSHEETPAGE Pages[NUM_PAGES];
    PROPSHEETHEADER Header;
    UINT i;
    UINT RetVal;

    //
    // Initialize the pages
    //
    for (i = 0; i < NUM_PAGES; i++) {
        Pages[i].dwSize = sizeof(PROPSHEETPAGE);
        Pages[i].dwFlags = PageInfo[i].Flags;
        Pages[i].hInstance = Instance;
        Pages[i].pszTemplate = PageInfo[i].Template;
        Pages[i].hIcon = NULL;
        Pages[i].pszTitle = MAKEINTRESOURCE(IDS_NAME);
        Pages[i].pfnDlgProc = PageInfo[i].DialogProc;
        Pages[i].lParam = i;
        Pages[i].pfnCallback = NULL;
        Pages[i].pcRefParent = NULL;
        Pages[i].pszHeaderTitle = PageInfo[i].Title;
        Pages[i].pszHeaderSubTitle = PageInfo[i].Subtitle;
    }

    //
    // Initialize the header
    //
    Header.dwSize = sizeof(PROPSHEETHEADER);
    Header.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER | PSH_STRETCHWATERMARK;
    Header.hwndParent = Parent;
    Header.hInstance = Instance;
    Header.hIcon = NULL;
    Header.pszCaption = MAKEINTRESOURCE(IDS_NAME);
    Header.nPages = NUM_PAGES;
    Header.nStartPage = 0;
    Header.ppsp = &Pages[0];
    Header.pfnCallback = NULL;
    Header.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK_EXTERIOR);
    Header.pszbmHeader = MAKEINTRESOURCE(IDB_WATERMARK_INTERIOR);

    //
    // Create the pages
    //
    RetVal = PropertySheet(&Header);

    if (RetVal == -1) {
        DisplayError(GetLastError());
    }
}

BOOL CALLBACK 
AppMgrDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function is the generic dialog function for the wizard pages.
    It performs the common functionality, and dispatches to page specific
    routines through the data structure for the page

Arguments:

    hwndDialog - Supplies the window handle for the dialog.
    uMsg -- Supplies the message number.
    wParam -- Supplies the wParam for the message.
    lParam -- Supplies the lParam for the message

Return Value:

    TRUE if the message was handled, FALSE if not.

--*/

{
    LPNMHDR Notification;
    LPPROPSHEETPAGE PropertyPage;
    UINT PageIndex;
    BOOL ReturnValue;

    PageIndex = GetWindowLong(hwndDlg, DWL_USER);

    switch (uMsg) {

        case WM_INITDIALOG:

            //
            // Store the index to the information for this page in the window long
            //
            PropertyPage = (LPPROPSHEETPAGE)lParam;
            PageIndex = PropertyPage->lParam;
            CheckSetWindowLong(hwndDlg, DWL_USER, PageIndex);

			//
			// N.B.  This MUST fall through to WM_DESTROY
			//
		case WM_DESTROY:
            //
            // Perform any page specific initialization
            //
            ReturnValue = TRUE;
            if (PageInfo[PageIndex].InitProc != NULL) {
                ReturnValue = PageInfo[PageIndex].InitProc(
                    hwndDlg,
					uMsg,
                    wParam,
                    lParam
                    );
            }


            return ReturnValue;

        case WM_NOTIFY:
            
            Notification = (LPNMHDR)lParam;

            switch (Notification->code) {
                case PSN_QUERYCANCEL:

                    //
                    // If there's a page id, it implies that 
                    // we only need default handling for 
                    // cancel for this page
                    //
                    if (PageInfo[PageIndex].CancelPageID != 0) {
                        return HandleQueryCancel(
                            hwndDlg,
                            PageInfo[PageIndex].CancelPageID
                            );
                    } else {
                        goto GenericCommandHandling;
                    }

                default:

GenericCommandHandling:
                    ReturnValue = PageInfo[PageIndex].NotifyProc(
                        hwndDlg,
                        wParam,
                        Notification,
                        PageIndex
                        );

                    return ReturnValue;
            }

        case WM_COMMAND:
            //
            // bugbug check the notify code
            //
      

            if (PageInfo[PageIndex].CommandProc == NULL) {
                return FALSE;
            }

            ReturnValue = PageInfo[PageIndex].CommandProc(
                hwndDlg,
                wParam,
                lParam,
                PageIndex
                );

            return ReturnValue;

        case WM_UPDATELISTVIEW:

            if (PageInfo[PageIndex].UpdateListViewProc == NULL) {
                // assert here.  We should never get here, because
                // we filter before we do the post message
                
                return FALSE;
            }

            ReturnValue = PageInfo[PageIndex].UpdateListViewProc(
                hwndDlg,
                wParam,
                lParam,
                PageIndex
                );

            return ReturnValue;

        default:
            return FALSE;
    }
}
