//////////////////////////////////////////////////////////////////////////////
/*  File: sharedlg.cpp

    Description: Displays a property-sheet-like dialog containing
        CSC statistics about a given network share.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    11/25/97    Initial creation.                                    BrianAu
    05/11/98    Changed dialog to a property sheet.                  BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "sharedlg.h"
#include <resource.h>
#include "viewerp.h"
#include "uncpath.h"
#include "cscutils.h"
#include "options.h"
#include "msgbox.h"
#include "uihelp.h"
#include "uuid.h"
#include "security.h"
#include "util.h"     // Utils from "dll" directory.

const DWORD SharePropSheet::m_rgHelpIDs[] = {
    IDC_SHRSUM_TXT_SHARENAME,       HIDC_SHRSUM_TXT_SHARENAME,
    IDC_SHRSUM_TXT_SYNC,            HIDC_SHRSUM_TXT_SYNC,
    IDC_STATIC2,                    IDH_NO_HELP,
    IDC_STATIC3,                    IDH_NO_HELP,
    IDC_STATIC4,                    IDH_NO_HELP,
    0,0
    };

SharePropSheet::SharePropSheet(
    HINSTANCE hInstance,
    LONG *pDllRefCnt,
    HWND hwndParent,
    LPCTSTR pszShare
    ) : m_hInstance(hInstance),
        m_pDllRefCnt(pDllRefCnt),
        m_hwndParent(hwndParent),
        m_strShare(pszShare)
{

}

SharePropSheet::~SharePropSheet(
    void
    )
{

}


BOOL CALLBACK 
SharePropSheet::AddPropSheetPage(
    HPROPSHEETPAGE hpage, 
    LPARAM lParam
    )
{
    DBGTRACE((DM_OPTDLG, DL_MID, TEXT("SharePropSheet::AddPropSheetPage")));
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < SharePropSheet::MAXPAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}


DWORD
SharePropSheet::Run(
    void
    )
{
    DWORD dwError = ERROR_SUCCESS;
    try
    {
        CString strSheetTitle(m_hInstance, IDS_CSCOPT_PROPSHEET_TITLE);

        HPROPSHEETPAGE rghPages[SharePropSheet::MAXPAGES];
        HPROPSHEETPAGE hpage;
        PROPSHEETHEADER psh;
        PROPSHEETPAGE psp;
        ZeroMemory(&psh, sizeof(psh));
        ZeroMemory(&psp, sizeof(psp));
        //
        // Define sheet.
        //
        psh.dwSize          = sizeof(PROPSHEETHEADER);
        psh.dwFlags         = 0;
        psh.hInstance       = m_hInstance;
        psh.hwndParent      = m_hwndParent;
        psh.pszIcon         = MAKEINTRESOURCE(IDI_CSCUI_ICON);
        psh.pszCaption      = strSheetTitle;
        psh.nPages          = 0;
        psh.nStartPage      = 0;
        psh.phpage          = rghPages;

        if (!m_strShare.IsEmpty())
        {
            //
            // Define the "Summary" page.
            // Remaining pages are added dynamically by options.cpp.
            // The separation is because the dynamic pages are also used by the
            // shell's "View Options" property sheet.
            //
            psp.dwSize          = sizeof(psp);
            psp.dwFlags         = PSP_USEREFPARENT;
            psp.hInstance       = m_hInstance;
            psp.pszTemplate     = MAKEINTRESOURCE(IDD_CACHEVIEW_SHARESUMMARY);
            psp.hIcon           = NULL;
            psp.pszTitle        = NULL;
            psp.pfnDlgProc      = (DLGPROC)SharePropSheet::GenPgDlgProc;
            psp.lParam          = (LPARAM)this;
            psp.pcRefParent     = (UINT *)m_pDllRefCnt;
            psp.pfnCallback     = NULL;

            hpage = CreatePropertySheetPage(&psp);
            if (NULL == hpage)
            {
                //
                // Early return if we can't create the "General" page.
                // No reason to continue on.
                //
                return GetLastError();
            }

            psh.phpage[psh.nPages++] = hpage;
        }

        if (!g_pSettings->NoConfigCache())
        {
            //
            // Policy doesn't prevent user from configuring CSC cache.
            // Add the dynamic page(s).
            //
            CCoInit coinit;
            HRESULT hr = coinit.Result();
            if (SUCCEEDED(hr))
            {
                com_autoptr<IShellExtInit> psei;
                hr = CoCreateInstance(CLSID_OfflineFilesOptions,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IShellExtInit,
                                      reinterpret_cast<void **>(psei.getaddr()));

                if (SUCCEEDED(hr))
                {
                    com_autoptr<IShellPropSheetExt> pspse;
                    hr = psei->QueryInterface(IID_IShellPropSheetExt,
                                             reinterpret_cast<void **>(pspse.getaddr()));
                    if (SUCCEEDED(hr))
                    {
                        hr = pspse->AddPages(AddPropSheetPage, (LPARAM)&psh);
                    }
                }
                else
                {
                    DBGERROR((TEXT("CoCreateInstance failed with result 0x%08X"), hr));
                }

            }
            else
            {
                DBGERROR((TEXT("CoInitialize failed with result 0x%08X"), hr));
            }
        }

        switch(PropertySheet(&psh))
        {
            case ID_PSREBOOTSYSTEM:
                //
                // User wants to change enabled state of CSC.  Requires reboot.
                //
                if (IDYES == CscMessageBox(m_hwndParent, 
                                           MB_YESNO | MB_ICONINFORMATION,
                                           m_hInstance,
                                           IDS_REBOOTSYSTEM))
                {                                     
                    dwError = CSCUIRebootSystem();
                    if (ERROR_SUCCESS != dwError)
                    {
                        DBGERROR((TEXT("Reboot failed with error %d"), dwError));
                        //
                        // BUGBUG:  This needs a reboot-specific message.
                        //
                        CscWin32Message(m_hwndParent, dwError, CSCUI::SEV_ERROR);
                    }
                }
                dwError = ERROR_SUCCESS;  // Run() succeeded.
                break;

            case -1:
            {
                dwError = GetLastError();
                DBGERROR((TEXT("PropertySheet failed with error %d"), dwError));
                CscWin32Message(m_hwndParent, dwError, CSCUI::SEV_ERROR);
                break;
            }
            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in SharePropSheet::Run"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return dwError;
}


BOOL CALLBACK
SharePropSheet::GenPgDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    SharePropSheet *pThis = reinterpret_cast<SharePropSheet *>(GetWindowLongPtr(hwnd, DWLP_USER));
    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
            {
                PROPSHEETPAGE *pPage = (PROPSHEETPAGE *)lParam;
                pThis = reinterpret_cast<SharePropSheet *>(pPage->lParam);
                SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)pThis);
                DBGASSERT((NULL != pThis));
                pThis->OnInitDialog(hwnd);
                return TRUE;
            }

            case WM_HELP:
                if (HELPINFO_WINDOW == ((LPHELPINFO)lParam)->iContextType)
                {
                    WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle), 
                             c_szHelpFile,
                             HELP_WM_HELP, 
                             (DWORD)((LPTSTR)m_rgHelpIDs));
                }
                break;

            case WM_CONTEXTMENU:
                    WinHelp((HWND)wParam,
                             c_szHelpFile,
                             HELP_CONTEXTMENU, 
                             (DWORD)((LPTSTR)m_rgHelpIDs));
                break;

            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in SharePropSheet::GenPgDlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    return FALSE;
}



void
SharePropSheet::OnInitDialog(
    HWND hwnd
    )
{
    CscShareInformation si;
    CscGetShareInformation(m_strShare, &si);

    SHFILEINFO sfi;
    if (0 != SHGetFileInfo(m_strShare,
                           0,
                           &sfi,
                           sizeof(sfi),
                           SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_LARGEICON))
    {
        CString strText(sfi.szDisplayName);
        if (!si.Connected())
        {
            //
            // If disconnected, append "(Disconnected)" to the share name.
            //
            CString strOffline(m_hInstance, IDS_SHARE_STATUS_OFFLINE);
            CString strSuffix;
            strSuffix.Format(TEXT(" (%1)"), strOffline.Cstr());
            strText += strSuffix;
        }
        //
        // Set the share display name and icon.
        //
        SetWindowText(GetDlgItem(hwnd, IDC_SHRSUM_TXT_SHARENAME), (LPCTSTR)strText);
        SendMessage(GetDlgItem(hwnd, IDC_SHRSUM_ICON), STM_SETICON, (WPARAM)sfi.hIcon, 0);
    }

    //
    // Set the two file count fields (pinned and temporary).
    //
    CString strNumber;
    strNumber.FormatNumber(si.PinnedFileCount());
    SetWindowText(GetDlgItem(hwnd, IDC_SHRSUM_TXT_PINNEDFILES), strNumber);
    strNumber.FormatNumber(si.FileCount() - si.PinnedFileCount());
    SetWindowText(GetDlgItem(hwnd, IDC_SHRSUM_TXT_TEMPFILES), strNumber);

    if (0 < si.StaleFileCount())
    {
        //
        // One or more files are stale.  Change the icon and text.
        //
        CString strText;
        SendMessage(GetDlgItem(hwnd, IDC_SHRSUM_ICON_SYNC), 
                    STM_SETICON,
                    (WPARAM)LoadIcon(NULL, MAKEINTRESOURCE(IDI_EXCLAMATION)),
                    0);
        strNumber.FormatNumber(si.StaleFileCount());
        strText.Format(m_hInstance, 
                       1 == si.StaleFileCount() ? IDS_FMT_SHARE_ONENOTSYNC : 
                                                  IDS_FMT_SHARE_MULTNOTSYNC,
                       strNumber.Cstr());

        SetWindowText(GetDlgItem(hwnd, IDC_SHRSUM_TXT_SYNC), strText);
    }        
}

