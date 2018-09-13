/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    dmgrdlg.cpp
**
** Purpose: Implements the Disk Space Cleanup Drive dialog
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "dmgrdlg.h"
#include "dmgrinfo.h"
#include "diskutil.h"
#include "msprintf.h"

#include <help.h>


// To work around a "feature" of listview we need the ability to temporarily ignore certain LVN_ITEMCHANGED messages:
BOOL    g_bIgnoreCheckStateChanges = TRUE;

/*
**------------------------------------------------------------------------------
** Local defines
**------------------------------------------------------------------------------
*/
#define crSliceUsed     RGB( 0, 0, 255 )
#define crSliceFree     RGB( 255, 0, 255)
#define crSliceCleanup  RGB( 255, 255, 0 )

const DWORD aHelpIDs[]=
{
    IDC_INTRO_TEXT,                 IDH_CLEANMGR_INTRO_TEXT,
    IDC_FILES_TO_REMOVE_TEXT,       IDH_CLEANMGR_CLIENT_LIST,
    IDC_CLIENT_LIST,                IDH_CLEANMGR_CLIENT_LIST,
    IDC_TOTAL_SPACE_DESCRIPTION,    IDH_CLEANMGR_TOTAL_SPACE,
    IDC_TOTAL_SPACE_TEXT,           IDH_CLEANMGR_TOTAL_SPACE,
    IDC_DESCRIPTION_GROUP,          IDH_CLEANMGR_DESCRIPTION_GROUP,
    IDC_DESCRIPTION_TEXT,           IDH_CLEANMGR_DESCRIPTION_GROUP,
    IDC_DETAILS_BUTTON,             IDH_CLEANMGR_DETAILS_BUTTON,
    IDC_WINDOWS_SETUP_ICON,         IDH_CLEANMGR_SETUP_GROUP,
    IDC_WINDOWS_SETUP_GROUP,        IDH_CLEANMGR_SETUP_GROUP,
    IDC_WINDOWS_SETUP_TEXT,         IDH_CLEANMGR_SETUP_GROUP,
    IDC_WINDOWS_SETUP_BUTTON,       IDH_CLEANMGR_SETUP_BUTTON,
    IDC_INSTALLED_PROGRAMS_ICON,    IDH_CLEANMGR_PROGRAMS_GROUP,
    IDC_INSTALLED_PROGRAMS_GROUP,   IDH_CLEANMGR_PROGRAMS_GROUP,
    IDC_INSTALLED_PROGRAMS_TEXT,    IDH_CLEANMGR_PROGRAMS_GROUP,
    IDC_INSTALLED_PROGRAMS_BUTTON,  IDH_CLEANMGR_PROGRAMS_BUTTON,
    IDC_FAT32_CONVERSION_ICON,      IDH_CLEANMGR_FAT32_GROUP,
    IDC_FAT32_CONVERSION_GROUP,     IDH_CLEANMGR_FAT32_GROUP,
    IDC_FAT32_CONVERSION_TEXT,      IDH_CLEANMGR_FAT32_GROUP,
    IDC_FAT32_CONVERSION_BUTTON,    IDH_CLEANMGR_FAT32_BUTTON,
    IDC_AUTO_LAUNCH,                IDH_CLEANMGR_AUTO_LAUNCH,
    IDC_DRIVE_ICON_LOCATION,        ((DWORD)-1),
    IDC_SETTINGS_DRIVE_TEXT,        ((DWORD)-1),
    0, 0
};


/*
**------------------------------------------------------------------------------
** Local function prototypes
**------------------------------------------------------------------------------
*/
BOOL CleanupMgrDlgInit     (HWND hDlg, LPARAM lParam);
void CleanupMgrDlgCleanup  (HWND hDlg);
BOOL CleanupMgrDlgCommand  (HWND hDlg, WPARAM wParam, LPARAM lParam);
BOOL CleanupMgrDlgNotify   (HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam);

BOOL CleanupMgrDlgInitText (HWND hDlg);
BOOL CleanupMgrDlgInitList (HWND hDlg);
VOID UpdateTotalSpaceToBeFreed(HWND hDlg);

INT_PTR CALLBACK MoreOptionsDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);


LPARAM ListView_GetItemData(HWND hwndList, int i)
{
    LVITEM lvi = {0};
    lvi.iItem = i;
    lvi.mask = LVIF_PARAM;
    if ( ListView_GetItem(hwndList, &lvi) )
    {
        return lvi.lParam;
    }
    return NULL;
}

/*
**------------------------------------------------------------------------------
** DisplayCleanMgrProperties
**
** Purpose:    Creates the Cleanup Manager property sheet
** Parameters:
**    hDlg     - Handle to dialog window
**    lParam   - DWORD to pass onto the property pages
** Return:     1 if user pressed "OK"
**             0 if user pressed "Cancel"
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
DWORD 
DisplayCleanMgrProperties(
    HWND    hWnd,
    LPARAM  lParam
    )
{
    DWORD           dwRet;
    TCHAR           *psz;
    PROPSHEETPAGE   psp;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE  hpsp[2];
    CleanupMgrInfo * pcmi = (CleanupMgrInfo *)lParam;
    if (pcmi == NULL)
    {
        //
        //Error - passed in invalid CleanupMgrInfo info
        //
        return 0;
    }

    memset(&psh, 0, sizeof(PROPSHEETHEADER));

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE;
    psp.hInstance = g_hInstance;
    psp.lParam = lParam;

    psp.pszTitle = MAKEINTRESOURCE(IDS_DISKCLEANUP);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DISK_CLEANER);
    psp.pfnDlgProc = DiskCleanupManagerProc;
    hpsp[0] = CreatePropertySheetPage(&psp);

    if (!(pcmi->dwUIFlags & FLAG_SAGESET))
    {
        psp.pszTitle = MAKEINTRESOURCE(IDS_MOREOPTIONS);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_MORE_OPTIONS);
        psp.pfnDlgProc = MoreOptionsDlgProc;
        hpsp[1] = CreatePropertySheetPage(&psp);

// commented out until after BEta 2
//      psp.pszTitle = MAKEINTRESOURCE(IDS_SETTINGS);
//      psp.pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS);
//      psp.pfnDlgProc = SettingsDlgProc;
//      hpsp[2] = CreatePropertySheetPage(&psp);

//      psh.nPages = 3;
        psh.nPages = 2;

        //
        //Create the dialog title
        //
        psz = SHFormatMessage( MSG_APP_TITLE, pcmi->szVolName, pcmi->szRoot[0]); 
    }

    else
    {
        psh.nPages = 1;

        //
        //Create the dialog title
        //
        psz = SHFormatMessage( MSG_APP_SETTINGS_TITLE );
    }

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOAPPLYNOW | PSH_USEICONID;
    psh.hInstance = g_hInstance;
    psh.hwndParent = hWnd;
    psh.pszIcon = MAKEINTRESOURCE(ICON_CLEANMGR);
    psh.phpage = hpsp;
    psh.pszCaption = psz;

    dwRet = (DWORD)PropertySheet(&psh);

    LocalFree(psz);
    return dwRet;
}

/*
**------------------------------------------------------------------------------
** DiskCleanupManagerProc
**
** Purpose:    Dialog routine for Disk Cleanup Manager Property Sheet
** Parameters:
**    hDlg     - Handle to dialog window
**    uMessage - behavior type
**    wParam   - depends on message
**    lParam   - depends on message
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
INT_PTR CALLBACK
DiskCleanupManagerProc(
    HWND   hDlg, 
    UINT   uiMessage, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    switch (uiMessage)
    {
        case WM_INITDIALOG:
            return CleanupMgrDlgInit(hDlg, lParam);

        case WM_DESTROY:
            CleanupMgrDlgCleanup(hDlg);
            break;

        case WM_COMMAND:
            return CleanupMgrDlgCommand(hDlg, wParam, lParam);

        case WM_NOTIFY:
            return CleanupMgrDlgNotify(hDlg, uiMessage, wParam, lParam);

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                    HELP_WM_HELP, (DWORD_PTR)(LPTSTR) aHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID) aHelpIDs);
            return TRUE;

        case WM_SYSCOLORCHANGE:
            SendMessage( GetDlgItem(hDlg, IDC_CLIENT_LIST), uiMessage, wParam, lParam);
            break;
    }

    //Non-handled message
    return FALSE;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrDlgInit
**
** Purpose:    Handles dialog initialization
** Parameters:
**    hDlg     - Handle to dialog window
**    lParam   - Property Sheet pointer
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL 
CleanupMgrDlgInit(
    HWND hDlg, 
    LPARAM lParam
    )
{
    LPPROPSHEETPAGE     lppsp;
    
    g_hDlg = hDlg;

    //
    //Make sure we have an invalid pointer to start out with
    //
    SetWindowLongPtr (hDlg, DWLP_USER, 0L);

    //   
    //Get the CleanupMgrInfo
    //
    lppsp = (LPPROPSHEETPAGE)lParam;
    CleanupMgrInfo * pcmi = (CleanupMgrInfo *)lppsp->lParam;
    if (pcmi == NULL)
    {
        //Error - passed in invalid CleanupMgrInfo info
        return FALSE;
    }

    // now as we are becoming visible, we can dismiss the progress dialog
    if ( pcmi->hAbortScanWnd )
    {
        pcmi->bAbortScan = TRUE;

        //
        //Wait for scan thread to finish
        //  
        WaitForSingleObject(pcmi->hAbortScanThread, INFINITE);

        pcmi->bAbortScan = FALSE;
    }

    //
    //Save pointer to CleanupMgrInfo object
    //
    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pcmi);

    //
    //Initialize all text
    //
    if (!CleanupMgrDlgInitText(hDlg))
        goto HAS_ERROR;

    //
    //Initialize the icon
    //
    SendDlgItemMessage(hDlg,IDC_DRIVE_ICON_LOCATION,STM_SETICON,(WPARAM)pcmi->hDriveIcon,0);

    //
    //If we are in SAGE settings mode then hide the total amount of space text
    //
    if (pcmi->dwUIFlags & FLAG_SAGESET)
    {
        ShowWindow(GetDlgItem(hDlg, IDC_TOTAL_SPACE_DESCRIPTION), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_TOTAL_SPACE_TEXT), SW_HIDE);
    }

    //
    //Initialize the list box (all of the cleanup clients)
    //
    if (!CleanupMgrDlgInitList(hDlg))
        goto HAS_ERROR;    

    return TRUE;

HAS_ERROR:
    //  
    //Delete any memory structures still hanging around
    //
    CleanupMgrDlgCleanup (hDlg);
    return FALSE;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrDlgCleanup
**
** Purpose:    
** Parameters:
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
void 
CleanupMgrDlgCleanup(
    HWND hDlg
    )
{
    //
    //Make sure we have a valid parameter
    //
    if (!hDlg)
        return;

    //
    //Hide the window right away since we might block waiting for a 
    //COM client to finish.
    //
    ShowWindow(hDlg, SW_HIDE);

    g_hDlg = NULL;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrWarningPrompt
**
** Purpose:    Asks the user if they are sure they want to delete the files
** Parameters:
**    hDlg     - Handle to dialog window
** Return:     TRUE if user says YES
**             FALSE if user says NO
** Notes;
** Mod Log:    Created by Jason Cobb (6/97)
**------------------------------------------------------------------------------
*/
BOOL
CleanupMgrWarningPrompt(
    HWND hDlg
    )
{
    TCHAR   szWarning[256];
    TCHAR   *pszWarningTitle;
    int     i;
    BOOL    bItemSelected = FALSE;

    //
    //First verify that at least one item is selected.  If no items are selected then
    //nothing will be deleted so we don't need to bother prompting the user.
    //
    CleanupMgrInfo * pcmi = GetCleanupMgrInfoPointer(hDlg);
    if (pcmi == NULL)
        return TRUE;

    for (i=0; i<pcmi->iNumVolumeCacheClients; i++)
    {
        if (pcmi->pClientInfo[i].bSelected == TRUE)
        {
            bItemSelected = TRUE;
            break;
        }
    }        

    if (bItemSelected)
    {
        LoadString(g_hInstance, IDS_DELETEWARNING, szWarning, sizeof(szWarning));
        pszWarningTitle = SHFormatMessage( MSG_APP_TITLE, pcmi->szVolName, pcmi->szRoot[0]);

        if (MessageBox(hDlg, szWarning, pszWarningTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            LocalFree(pszWarningTitle);
            return TRUE;
        }
        else
        {
            LocalFree(pszWarningTitle);
            return FALSE;
        }
    }

    //
    //No items are selected so just return TRUE since nothing will be deleted.
    //
    return TRUE;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrDlgCommand
**
** Purpose:    Handles command messages
** Parameters:
**    hDlg     - Handle to dialog window
**    wParam   - depends on command
**    lParam   - depends on command
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL CleanupMgrDlgCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    WORD wID = LOWORD(wParam);

    if ( IDC_DETAILS_BUTTON == wID )
    {
        HWND    hWndList = GetDlgItem(hDlg, IDC_CLIENT_LIST);
        int     wIndex;

        wIndex = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
        if (-1 != wIndex)
        {
            PCLIENTINFO pClientInfo = (PCLIENTINFO)ListView_GetItemData(hWndList,wIndex);
            if ( pClientInfo )
            {
                pClientInfo->pVolumeCache->ShowProperties(hDlg);
            }
        }
    }
    return 0;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrDlgNotify
**
** Purpose:    Handles notify messages
** Parameters:
**    hDlg     - Handle to dialog window
**    wParam   - depends on command
**    lParam   - depends on command
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
BOOL 
CleanupMgrDlgNotify(
    HWND hDlg, 
    UINT uiMessage,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    CleanupMgrInfo  *pcmi;
    LPNMHDR pnmhdr = (LPNMHDR)lParam;

    if (IDC_CLIENT_LIST == pnmhdr->idFrom)
    {
        // a list view notification
        #define pnmlv ((LPNMLISTVIEW)pnmhdr)

        switch (pnmhdr->code)
        {
            case LVN_ITEMCHANGED:
                if ( pnmlv->uChanged & LVIF_STATE )
                {
                    LVITEM lvi;
                    lvi.iItem = pnmlv->iItem;
                    lvi.iSubItem = pnmlv->iSubItem;
                    lvi.mask = LVIF_PARAM;
                    ListView_GetItem( pnmhdr->hwndFrom, &lvi );
                    PCLIENTINFO pClientInfo = (PCLIENTINFO)lvi.lParam;

                    // check if an item was selected
                    if ( pnmlv->uNewState & LVIS_SELECTED )
                    {
                        if (pClientInfo->wcsDescription)
                        {
                            TCHAR szDescription[DESCRIPTION_LENGTH];
                            SHUnicodeToTChar(pClientInfo->wcsDescription, szDescription, ARRAYSIZE( szDescription ));
                            SetDlgItemText(hDlg, IDC_DESCRIPTION_TEXT, szDescription);
                        }
                        else
                        {
                            SetDlgItemText(hDlg, IDC_DESCRIPTION_TEXT, TEXT(""));
                        }

                        //
                        //Show or Hide the Settings button
                        //
                        if (pClientInfo->dwInitializeFlags & EVCF_HASSETTINGS)
                        {
                            TCHAR szButton[BUTTONTEXT_LENGTH];
                            SHUnicodeToTChar(pClientInfo->wcsAdvancedButtonText, szButton, ARRAYSIZE( szButton ));
                            SetDlgItemText(hDlg, IDC_DETAILS_BUTTON, szButton);
                            ShowWindow(GetDlgItem(hDlg, IDC_DETAILS_BUTTON), SW_SHOW);
                        }
                        else
                        {
                            ShowWindow(GetDlgItem(hDlg, IDC_DETAILS_BUTTON), SW_HIDE);
                        }
                    }

                    // Check if the state image changed.  This results from checking or unchecking
                    // one of the list view checkboxes.
                    if ((pnmlv->uNewState ^ pnmlv->uOldState) & LVIS_STATEIMAGEMASK)
                    {
                        if ( !g_bIgnoreCheckStateChanges )
                        {
                            pClientInfo->bSelected = ListView_GetCheckState( pnmhdr->hwndFrom, pnmlv->iItem );
                            UpdateTotalSpaceToBeFreed(hDlg);
                        }
                    }
                }
                break;
        }
    }
    else
    {
        // must be a property sheet notification
        switch(pnmhdr->code)
        {
            case PSN_RESET:
                pcmi = GetCleanupMgrInfoPointer(hDlg);
                pcmi->bPurgeFiles = FALSE;
                break;

            case PSN_APPLY:
                pcmi = GetCleanupMgrInfoPointer(hDlg);
                if (!(pcmi->dwUIFlags & FLAG_SAGESET) && !CleanupMgrWarningPrompt(hDlg))
                    pcmi->bPurgeFiles = FALSE;
                else
                    pcmi->bPurgeFiles = TRUE;
                break;
        }
    }

    return FALSE;
}


/*
**------------------------------------------------------------------------------
** CleanupMgrDlgInitText
**
** Purpose:    
** Parameters:
**    hDlg     - Handle to dialog window
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL 
CleanupMgrDlgInitText(
    HWND hDlg
    )
{
    if (hDlg == NULL)
        return FALSE;

    //
    // Step 1. Get pointers to Info structures
    //

    CleanupMgrInfo * pcmi = GetCleanupMgrInfoPointer(hDlg);
    if (pcmi == NULL)
        return FALSE;
    if (pcmi->dre == Drive_INV)
        return FALSE;

    // 
    // Step 2. Extract useful info
    //

    //
    //get vol name   
    //
    TCHAR * pszVolName = pcmi->szVolName;
    if (pszVolName == NULL)
        pszVolName = TEXT("");

    //  
    //get drive letter
    //
    TCHAR chDrive = pcmi->dre + 'A';

    //
    // Step 3. Initialize text
    //
      
    //
    //Set header
    //
    if (pcmi->dwUIFlags & FLAG_SAGESET)
    {
        TCHAR * psz;
        psz = SHFormatMessage( MSG_INTRO_SETTINGS_TEXT );
        SetDlgItemText (hDlg, IDC_INTRO_TEXT, psz);
        LocalFree(psz);
    }
    else
    {
        TCHAR * psz;
        TCHAR * pszDrive;
        TCHAR szBuffer[50];
        
        pszDrive = SHFormatMessage( MSG_VOL_NAME_DRIVE_LETTER, pszVolName, chDrive);

        StrFormatKBSize(pcmi->cbEstCleanupSpace.QuadPart, szBuffer, ARRAYSIZE( szBuffer ));
        psz = SHFormatMessage( MSG_INTRO_TEXT, pszDrive, szBuffer);
        SetDlgItemText (hDlg, IDC_INTRO_TEXT, psz);
        LocalFree(pszDrive);
        LocalFree(psz);
    }

    return TRUE;
}

/*
**------------------------------------------------------------------------------
** UpdateTotalSpaceToBeFreed
**
** Purpose:    
** Parameters:
**    hDlg     - Handle to dialog window
** Return:     NONE
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
VOID UpdateTotalSpaceToBeFreed(HWND hDlg)
{
    int             i;
    ULARGE_INTEGER  TotalSpaceToFree;

    TotalSpaceToFree.QuadPart = 0;
    
    if (hDlg == NULL)
        return;

    CleanupMgrInfo * pcmi = GetCleanupMgrInfoPointer(hDlg);
    if (pcmi == NULL)
        return;
    if (pcmi->dre == Drive_INV)
        return;

    //
    //Calculate the total space to be freed by adding up the dwUsedSpace value
    //on all of the selected clients
    //
    for (i=0; i<pcmi->iNumVolumeCacheClients; i++)
    {
        if (pcmi->pClientInfo[i].bSelected)
        {
            TotalSpaceToFree.QuadPart += pcmi->pClientInfo[i].dwUsedSpace.QuadPart;
        }
    }        

    //
    //Display the total space to be freed
    //
    TCHAR szBuffer[10];
    StrFormatKBSize(TotalSpaceToFree.QuadPart, szBuffer, ARRAYSIZE( szBuffer ));
    SetDlgItemText(hDlg, IDC_TOTAL_SPACE_TEXT, szBuffer);
}


/*
**------------------------------------------------------------------------------
** CleanupMgrDlgInitList
**
** Purpose:    
** Parameters:
**    hDlg     - Handle to dialog window
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/

#define NAME_COL_PERCENT    80
#define SIZE_COL_PERCENT    20

BOOL CleanupMgrDlgInitList(HWND hDlg)
{
    int i;
    
    if (hDlg == NULL)
        return FALSE;

    CleanupMgrInfo * pcmi = GetCleanupMgrInfoPointer(hDlg);
    if (pcmi == NULL)
        return FALSE;

    if (pcmi->dre == Drive_INV)
        return FALSE;
        
    HWND hwndList = GetDlgItem(hDlg, IDC_CLIENT_LIST);
    RECT rc;
    GetClientRect(hwndList, &rc);
    int cxList = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    // I have no idea what all this TUNEUP and SAGESET crap means, but the old code
    // only drew the sizes if the following condition was true.  As such, I'm only
    // showing the size column if the same condition is true:
    BOOL bShowTwoCols = (!(pcmi->dwUIFlags & FLAG_TUNEUP) && !(pcmi->dwUIFlags & FLAG_SAGESET));

    LVCOLUMN lvc;
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
    lvc.iSubItem = 0;
    lvc.cx = bShowTwoCols ? MulDiv(cxList, NAME_COL_PERCENT, 100) : cxList;
    ListView_InsertColumn( hwndList, 0, &lvc );

    if ( bShowTwoCols )
    {
        lvc.mask = LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
        lvc.iSubItem = 1;
        lvc.cx = MulDiv(cxList, SIZE_COL_PERCENT, 100);
        lvc.fmt = LVCFMT_RIGHT;
        ListView_InsertColumn( hwndList, 1, &lvc );
    }

    HIMAGELIST himg = ImageList_Create(16, 16, ILC_COLOR|ILC_MASK, 4, 4);
    ListView_SetImageList(hwndList, himg, LVSIL_SMALL );
    ListView_SetExtendedListViewStyleEx(hwndList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    // When we add an item to the listview the listview code always initializes the item to the unchecked
    // state.  It then fires a WM_NOTIFY telling us the state changed to "off" which causes us to nuke our
    // bSelected value.  As such we need to ignore state image changes during the addition of list view
    // items so that we can preserve our bSelected state.
    g_bIgnoreCheckStateChanges = TRUE;

    for (i=0; i<pcmi->iNumVolumeCacheClients; i++)
    {
        if ((pcmi->pClientInfo[i].pVolumeCache != NULL) &&
            (pcmi->pClientInfo[i].wcsDisplayName != NULL) &&
            (pcmi->pClientInfo[i].bShow == TRUE))
        {
            LPTSTR      lpszDisplayName;
            ULONG cb;

            cb = WideCharToMultiByte(CP_ACP, 0, pcmi->pClientInfo[i].wcsDisplayName, -1, NULL, 0, NULL, NULL);
            if ((lpszDisplayName = (LPTSTR)LocalAlloc(LPTR, (cb + 1) * sizeof( TCHAR ))) != NULL)
            {
#ifdef UNICODE
                StrCpyN( lpszDisplayName, pcmi->pClientInfo[i].wcsDisplayName, cb );
#else
                //
                //Convert UNICODE display name to ANSI and then add it to the list
                //
                WideCharToMultiByte(CP_ACP, 0, pcmi->pClientInfo[i].wcsDisplayName, -1, lpszDisplayName, cb, NULL, NULL);
#endif

                //
                //Determine where in the list this item should go.
                //
                int iSortedPossition;
                int totalSoFar = ListView_GetItemCount(hwndList);

                for (iSortedPossition=0; iSortedPossition<totalSoFar; iSortedPossition++)
                {
                    PCLIENTINFO pClientInfo = (PCLIENTINFO)ListView_GetItemData(hwndList, iSortedPossition);
                    if (!pClientInfo | (pcmi->pClientInfo[i].dwPriority < pClientInfo->dwPriority))
                        break;
                }

                //
                //Insert this item at index j in the list
                //
                LVITEM lvi = {0};
                lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
                lvi.iItem = iSortedPossition;
                lvi.iSubItem = 0;
                lvi.pszText = lpszDisplayName;
                lvi.lParam = (LPARAM)&(pcmi->pClientInfo[i]);
                lvi.iImage = ImageList_AddIcon(himg, pcmi->pClientInfo[i].hIcon);
                
                iSortedPossition = ListView_InsertItem(hwndList, &lvi);

                if (bShowTwoCols)
                {
                    TCHAR szBuffer[10];

                    StrFormatKBSize(pcmi->pClientInfo[i].dwUsedSpace.QuadPart, szBuffer, ARRAYSIZE( szBuffer ));
                    ListView_SetItemText( hwndList, iSortedPossition, 1, szBuffer );
                }

                // Set the initial check state.  We can't do this when we add the item because the
                // list view code specifically ingores your State Image Flags if you have the
                // LVS_EX_CHECKBOX style set, which we do.
                ListView_SetCheckState( hwndList, iSortedPossition, pcmi->pClientInfo[i].bSelected );

                LocalFree( lpszDisplayName );
            }
        }
    }        

    g_bIgnoreCheckStateChanges = FALSE;

    UpdateTotalSpaceToBeFreed(hDlg);
    ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);

    return TRUE;
}


INT_PTR CALLBACK
MoreOptionsDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPPROPSHEETPAGE     lppsp;
    CleanupMgrInfo      *pcmi;
    TCHAR               szCommandLine[MAX_PATH];
    STARTUPINFO         startupinfo;
    PROCESS_INFORMATION processinformation;
    
    switch(Message)
    {
        case WM_INITDIALOG:
            //
            //Disable FAT32 conversion if this drive is already FAT32
            //
            lppsp = (LPPROPSHEETPAGE)lParam;
            pcmi = (CleanupMgrInfo *)lppsp->lParam;
            if (pcmi == NULL)
            {
                //Error - passed in invalid CleanupMgrInfo info
                return FALSE;
            }

#ifndef WINNT
            if (lstrcmpi(pcmi->szFileSystem, TEXT("FAT32")) == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_FAT32_CONVERSION_GROUP), FALSE);
                ShowWindow(GetDlgItem(hDlg, IDC_FAT32_CONVERSION_ICON), SW_HIDE);
                EnableWindow(GetDlgItem(hDlg, IDC_FAT32_CONVERSION_TEXT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_FAT32_CONVERSION_BUTTON), FALSE);
            }
#endif
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_WINDOWS_SETUP_BUTTON:
                    {
                        TCHAR szSysDir[MAX_PATH];
                        if ( GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir)) )
                        {
                            TCHAR szParam[MAX_PATH];
                            wsprintf(szParam, SZ_WINDOWS_SETUP, szSysDir );
                            ShellExecute(NULL, NULL, SZ_SYSOCMGR, szParam, NULL, SW_SHOWNORMAL);
                        }
                    }
                    break;

                case IDC_INSTALLED_PROGRAMS_BUTTON:
                    ShellExecute(NULL, NULL, SZ_RUNDLL32, SZ_INSTALLED_PROGRAMS, NULL, SW_SHOWNORMAL);
                    break;

#ifndef WINNT
                case IDC_FAT32_CONVERSION_BUTTON:
                    GetWindowsDirectory(szCommandLine, sizeof(szCommandLine));
                    lstrcat(szCommandLine, SZ_CVT1);

                    memset(&startupinfo, 0, sizeof(STARTUPINFO));
                    startupinfo.cb = sizeof(STARTUPINFO);

                    memset(&processinformation, 0, sizeof(PROCESS_INFORMATION));
                    
                    if ( CreateProcess(szCommandLine, NULL, NULL, NULL, FALSE, 0, NULL, NULL,
                        &startupinfo, &processinformation))
                    {
                        CloseHandle( processinformation.hProcess );
                        CloseHandle( processinformation.hThread );
                    }
                    break;
#endif
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                    HELP_WM_HELP, (DWORD_PTR)(LPTSTR) aHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID) aHelpIDs);
            return TRUE;

        default:
            return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
SettingsDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPPROPSHEETPAGE lppsp;
    CleanupMgrInfo *pcmi;
    DWORD dwType, cbBytes;
    DWORD dwLDSDisable;
    HKEY hk;
    
    switch(Message)
    {
        case WM_INITDIALOG:
        {
            TCHAR * psz;
            hardware hwType;

            lppsp = (LPPROPSHEETPAGE)lParam;
            pcmi = (CleanupMgrInfo *)lppsp->lParam;
            if (pcmi == NULL)
            {
                //Error - passed in invalid CleanupMgrInfo info
                return FALSE;
            }

            //
            //Save pointer to CleanupMgrInfo object
            //
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pcmi);

            TCHAR * pszVolName = pcmi->szVolName;
            if (pszVolName == NULL)
                pszVolName = TEXT("");

            TCHAR chDrive = pcmi->dre + TCHAR('A');

            psz = SHFormatMessage( MSG_INTRO_SETTINGS_TAB, pszVolName, chDrive );
            SetDlgItemText (hDlg, IDC_SETTINGS_DRIVE_TEXT, psz);
            LocalFree(psz);

            //
            //Initialize the icon
            //
            SendDlgItemMessage(hDlg,IDC_DRIVE_ICON_LOCATION,STM_SETICON,(WPARAM)pcmi->hDriveIcon,0);

            //
            //Initialize the auto launch check box
            //
            if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_FILESYSTEM, &hk) == ERROR_SUCCESS)
            {
                dwLDSDisable = 0;
                dwType = REG_DWORD;
                cbBytes = sizeof(dwLDSDisable);
                RegQueryValueEx(hk, REGSTR_VAL_DRIVE_LDS_BDCAST_DISABLE, NULL, 
                    &dwType, (LPBYTE)&dwLDSDisable, &cbBytes);

                if (dwLDSDisable & (0x01 << pcmi->dre))
                    CheckDlgButton(hDlg, IDC_AUTO_LAUNCH, 0);
                else
                    CheckDlgButton(hDlg, IDC_AUTO_LAUNCH, 1);

                RegCloseKey(hk);
            }

            //
            //Gray out the auto launch option if this is not a fixed disk
            //
            if (!GetHardwareType(pcmi->dre, hwType) ||
                (hwType != hwFixed))
            {
                CheckDlgButton(hDlg, IDC_AUTO_LAUNCH, 0);
                EnableWindow(GetDlgItem(hDlg, IDC_AUTO_LAUNCH), FALSE);
            }
        }
            break;

        case WM_NOTIFY:
            switch(((NMHDR *)lParam)->code)
            {
                case PSN_APPLY:
                    pcmi = (CleanupMgrInfo *)GetWindowLongPtr (hDlg, DWLP_USER);

                    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_FILESYSTEM, &hk) == ERROR_SUCCESS)
                    {
                        dwLDSDisable = 0;
                        dwType = REG_DWORD;
                        cbBytes = sizeof(dwLDSDisable);
                        RegQueryValueEx(hk, REGSTR_VAL_DRIVE_LDS_BDCAST_DISABLE, NULL, 
                                &dwType, (LPBYTE)&dwLDSDisable, &cbBytes);

                        if (IsDlgButtonChecked(hDlg, IDC_AUTO_LAUNCH))
                        {
                            dwLDSDisable &= ~(0x01 << pcmi->dre);
                        }

                        else
                        {
                            dwLDSDisable |= (0x01 << pcmi->dre);
                        }
                        
                        RegSetValueEx(hk, REGSTR_VAL_DRIVE_LDS_BDCAST_DISABLE, 0, REG_DWORD,
                            (LPBYTE)&dwLDSDisable, sizeof(dwLDSDisable));
                            
                        RegCloseKey(hk);
                    }
                    break;

                case PSN_RESET:
                    break;
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                    HELP_WM_HELP, (DWORD_PTR)(LPTSTR) aHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID) aHelpIDs);
            return TRUE;

        default:
            return FALSE;
    }

    return TRUE;
}
