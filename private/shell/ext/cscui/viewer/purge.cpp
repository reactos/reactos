//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       purge.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <cscuiext.h>   // CSCUIRemoveFolderFromCache
#include "purge.h"
#include "pathstr.h"
#include "cscutils.h"
#include "msgbox.h"
#include "resource.h"
#include "security.h"
#include "util.h"     // Utils from "dll" directory.

//
// This is also defined in cscui\dll\pch.h
// If you change it there you must change it here and vice versa.
//
#define FLAG_CSC_HINT_PIN_ADMIN  FLAG_CSC_HINT_PIN_SYSTEM

//
// Purge confirmation dialog.
// The user can set which files are purged from the cache.
//
class CConfirmPurgeDialog
{
    public:
        CConfirmPurgeDialog(void) throw()
            : m_hInstance(NULL),
              m_hwnd(NULL),
              m_hwndLV(NULL),
              m_pSel(NULL)
              { }

        ~CConfirmPurgeDialog(void) throw()
            { if (NULL != m_hwnd) DestroyWindow(m_hwnd); }

        int Run(HINSTANCE hInstance, HWND hwndParent, CCachePurgerSel *pSel);

    private:
        HINSTANCE        m_hInstance;
        HWND             m_hwnd;
        HWND             m_hwndLV;
        CCachePurgerSel *m_pSel;                // Ptr to destination for selection info.

        static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        void OnInitDialog(HWND hwnd);
        void OnDestroy(void);
        void OnOk(void);
        void OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
        LPTSTR MyStrDup(LPCTSTR psz);
        BOOL LVSetItemData(HWND hwndLV, int i, LPARAM lParam);
        LPARAM LVGetItemData(HWND hwndLV, int i);
        int LVAddItem(HWND hwndLV, LPCTSTR pszItem);
};


inline ULONGLONG
MakeULongLong(DWORD dwLow, DWORD dwHigh)
{
    return ((ULONGLONG)(((DWORD)(dwLow)) | ((LONGLONG)((DWORD)(dwHigh))) << 32));
}

inline bool
IsDirty(const CscFindData& cfd)
{
    return 0 != (FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY & cfd.dwStatus);
}


inline bool
IsSuperHidden(const CscFindData& cfd)
{
    const DWORD dwSuperHidden = (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
    return (cfd.fd.dwFileAttributes & dwSuperHidden) == dwSuperHidden;
}

inline bool
OthersHaveAccess(const CscFindData& cfd)
{
    return CscAccessOther(cfd.dwStatus);
}


CCachePurger::CCachePurger(
    const CCachePurgerSel& sel,
    LPFNPURGECALLBACK pfnCbk, 
    LPVOID pvCbkData
    ) : m_ullBytesToScan(0),
        m_ullBytesScanned(0),
        m_ullBytesToDelete(0),
        m_ullBytesDeleted(0),
        m_ullFileBytes(0),
        m_dwPhase(0),
        m_cFilesToScan(0),
        m_cFilesScanned(0),
        m_cFilesToDelete(0),
        m_cFilesDeleted(0),
        m_iFile(0),
        m_dwFileAttributes(0),
        m_dwResult(0),
        m_hmutexPurgeInProg(NULL),
        m_pszFile(NULL),
        m_pvCbkData(pvCbkData),
        m_bWillDelete(FALSE),
        m_pfnCbk(pfnCbk),
        m_bIsValid(true),
        m_sel(sel),
        m_bDelPinned(0 != (PURGE_FLAG_PINNED & sel.Flags())),
        m_bDelUnpinned(0 != (PURGE_FLAG_UNPINNED & sel.Flags())),
        m_bIgnoreAccess(0 != (PURGE_IGNORE_ACCESS & sel.Flags())),
        m_bUserIsAnAdmin(boolify(IsCurrentUserAnAdminMember()))
{
    //
    // Let the world know we're purging files from the cache.
    // In particular, our overlay handler in cscui\dll\shellex.cpp needs
    // to disable it's auto-pin code whenever we're purging.  If we didn't
    // do this AND the source folder is open during the purge, we get a 
    // nasty race condition between our shell notifications for purging
    // and the overlay handler's auto-pin code.  We'll delete a file
    // and send a notification.  The shell updates the icon overlay.
    // Our handler sees that the parent folder is pinned so it re-pins
    // the purged file which restores the file in the folder. This ends
    // up resulting in a very nasty infinite loop. Those interested
    // should call IsPurgeInProgress() to determine if the purge is
    // in progress (see cscui\dll\util.cpp).  [brianau - 11/01/99]
    //
    m_hmutexPurgeInProg = CreateMutex(NULL, TRUE, c_szPurgeInProgMutex);
}

CCachePurger::~CCachePurger(
    void
    )
{
    if (NULL != m_hmutexPurgeInProg)
    {
        ReleaseMutex(m_hmutexPurgeInProg);
        CloseHandle(m_hmutexPurgeInProg);
    }
}

   
//
// Deletes a directory and all it's contents according to the PURGE_FLAG_XXXXX flag 
// bits set by the caller of PurgeCache().
// This function recursively traverses the cache file hierarchy in a post-order fashion.
// Directory nodes are deleted after all children have been deleted.
// If the caller of PurgeCache provided a callback function, it is called
// after each deletion.   If the callback function returns FALSE, the traversal
// operation is terminated.
//
// pstrPath - Address of CPath object containing the path of the directory to be
//            deleted.  This object is used to contain the working path throughout
//            the tree traversal.
//
// dwPhase -  PURGE_PHASE_SCAN    - Scanning for file totals.
//            PURGE_PHASE_DELETE  - Deleting files.
//
// bShareIsOpen - There's an open connection to the share.  It's OK
//                to issue a shell change notify.
//
// Returns:   true  = Continue traversal.
//            false = User cancelled via callback.  Terminate traversal.
//            
//
bool
CCachePurger::ProcessDirectory(
    CPath *pstrPath,
    DWORD dwPhase,
    bool bShareIsOpen
    )
{
    bool bContinue = true;
    bool bShellNotified = false;
    CscFindData cfd;

    CscFindHandle hFind(CacheFindFirst(*pstrPath, m_sel.UserSid(), &cfd));
    if (hFind.IsValidHandle())
    {
        do
        {
            bool bIsDirectory = (0 != (FILE_ATTRIBUTE_DIRECTORY & cfd.fd.dwFileAttributes));
            //
            // Create full path to this file/folder.
            //
            pstrPath->Append(cfd.fd.cFileName);

            if (bIsDirectory)
            {
                //
                // It's a directory.  Recursively delete it's contents.
                //
                bContinue = ProcessDirectory(pstrPath, dwPhase, bShareIsOpen);
            }
            if (bContinue)
            {
                bool bPinned = (0 != ((FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_ADMIN) & cfd.dwHintFlags));
                //
                // The decision to delete a file has several criteria.  I've tried to break this
                // up using inlines and member variables to make it more understandable and minimize
                // maintenance bugs.
                // The logic for deletion is this:
                //
                //  bDelete = false;
                //  If (pinned AND deleting pinned) OR (not pinned and deleting unpinned) then
                //      If super_hidden then
                //          bDelete = true;
                //      else
                //          If (not locally dirty) then
                //              If (ignore access) then
                //                  bDelete = true;
                //              else
                //                  If (user is an admin) then
                //                      bDelete = true;
                //                  else
                //                      if (others have NO access) then
                //                          bDelete = true;
                //                      endif
                //                  endif
                //              endif
                //          endif
                //      endif
                //  endif
                bool bDelete = ((bPinned && m_bDelPinned) || (!bPinned && m_bDelUnpinned)) &&
                               (IsSuperHidden(cfd) || 
                                    (!IsDirty(cfd) && 
                                        (m_bIgnoreAccess ||
                                            (m_bUserIsAnAdmin || !OthersHaveAccess(cfd)))));

                m_pszFile          = pstrPath->Cstr();
                m_dwFileAttributes = cfd.fd.dwFileAttributes;
                m_ullFileBytes     = MakeULongLong(cfd.fd.nFileSizeLow, cfd.fd.nFileSizeHigh);
                m_bWillDelete      = bDelete;

                if (PURGE_PHASE_SCAN == dwPhase)
                {
                    if (!bIsDirectory && m_pfnCbk)
                    {
                        //
                        // Exclude directories from the file and byte counts.
                        // 
                        if (bDelete)
                        {
                            m_cFilesToDelete++;
                            m_ullBytesToDelete += m_ullFileBytes;
                        }
                        m_cFilesScanned++;
                        m_ullBytesScanned += m_ullFileBytes;

                        m_dwResult = ERROR_SUCCESS;
                        bContinue = boolify((*m_pfnCbk)(this));
                        m_iFile++;
                    }
                }
                else if (PURGE_PHASE_DELETE == dwPhase && bDelete)
                {
                    m_dwResult = CscDelete(*pstrPath);
                    if (ERROR_SUCCESS == m_dwResult)
                    {
                        if (!bIsDirectory)
                        {
                            m_cFilesDeleted++;
                            m_ullBytesDeleted += m_ullFileBytes;
                        }                            
                        if (bShareIsOpen)
                        {
                            //
                            // Only notify the shell if there's an open 
                            // connection to the share.  If there's not an 
                            // open connection and the share is not available 
                            // via the net, this change notify will generate a 
                            // "bad net name" error in the redir and cause the 
                            // CSC agent to transition the share to offline mode.  
                            // This results in a possible state change to the UI 
                            // which (when deleting files from the cache) is a 
                            // weird experience for the user.
                            //
                            ShellChangeNotify(*pstrPath);
                            bShellNotified = true;
                        }
                    }
                    else if (ERROR_ACCESS_DENIED == m_dwResult)
                    {
                        //
                        // This is a little weird.  CscDelete
                        // returns ERROR_ACCESS_DENIED if there's
                        // a handle open on the file. Set the
                        // code to ERROR_BUSY so we know to handle 
                        // this as a special case.
                        //
                        m_dwResult = ERROR_BUSY;
                    }
                    if (!bIsDirectory && m_pfnCbk)
                    {
                        bContinue = boolify((*m_pfnCbk)(this));
                        m_iFile++;
                    }
                }
            }
            pstrPath->RemoveFileSpec();
        }
        while(bContinue && CacheFindNext(hFind, &cfd));
    }
    pstrPath->RemoveBackslash();

    if (bShellNotified)
    {
        //
        // Flush notifications to ensure all updates for this 
        // directory are reflected in the shell.
        //
        ShellChangeNotify(*pstrPath, true, SHCNE_UPDATEDIR);
    }

    return bContinue;
}



//
// Public function for purging cache contents.
//
HRESULT
CCachePurger::Process(
    DWORD dwPhase
    )
{
    HRESULT hr = NOERROR;

    if (!m_bIsValid)
        return E_OUTOFMEMORY;  // Failed ctor.

    m_dwPhase = dwPhase;

    if (PURGE_PHASE_SCAN == dwPhase)
    {
        //
        // At start of scanning phase, get the max bytes and file count 
        // from the CSC database.  This will let us provide meaningful 
        // progress data during the scanning phase.
        //
        ULARGE_INTEGER ulTotalBytes = {0, 0};
        ULARGE_INTEGER ulUsedBytes  = {0, 0};
        DWORD dwTotalFiles          = 0;
        DWORD dwTotalDirs           = 0;
        TCHAR szVolume[MAX_PATH];
        CSCGetSpaceUsage(szVolume,
                         ARRAYSIZE(szVolume),
                         &ulTotalBytes.HighPart,
                         &ulTotalBytes.LowPart,
                         &ulUsedBytes.HighPart,
                         &ulUsedBytes.LowPart,
                         &dwTotalFiles,
                         &dwTotalDirs);

        m_cFilesToScan     = dwTotalFiles + dwTotalDirs;
        m_ullBytesToScan   = ulTotalBytes.QuadPart;
        m_ullBytesToDelete = 0;
        m_ullBytesDeleted  = 0;
        m_ullBytesScanned  = 0;
        m_ullFileBytes     = 0;
        m_cFilesToDelete   = 0;
        m_cFilesDeleted    = 0;
        m_cFilesScanned    = 0;
        m_dwFileAttributes = 0;
        m_dwResult         = 0;
        m_pszFile          = NULL;
        m_bWillDelete      = false;
    }
    m_iFile = 0; // Reset this for each phase.

    bool bContinue = true;
    try
    {
        CscFindData cfd;
        CPath strPath;
        if (0 < m_sel.ShareCount())
        {
            //
            // Delete 1+ (but not all) shares.
            //
            for (int i = 0; i < m_sel.ShareCount(); i++)
            {
                strPath = m_sel.ShareName(i);
                bContinue = ProcessDirectory(&strPath, 
                                             dwPhase, 
                                             S_OK == ::IsOpenConnectionPathUNC(m_sel.ShareName(i)));
                                             
                if (PURGE_PHASE_DELETE == dwPhase)
                {
                    CscDelete(strPath);
                }
            }
        }
        else
        {
            //
            // Delete all shares.
            //
            CscFindHandle hFind(CacheFindFirst(NULL, m_sel.UserSid(), &cfd));
            if (hFind.IsValidHandle())
            {
                do
                {
                    strPath = cfd.fd.cFileName;
                    bContinue = ProcessDirectory(&strPath, 
                                                 dwPhase,
                                                 S_OK == ::IsOpenConnectionShare(cfd.fd.cFileName));
                }
                while(bContinue && CacheFindNext(hFind, &cfd));
            }
        }

        if (PURGE_PHASE_DELETE == dwPhase)
        {
            //
            // On the DELETE phase always try to remove any empty
            // share entries from the database.  CSCDelete will
            // harmlessly fail if the share entry cannot be deleted.
            //
            CscFindHandle hFind(CacheFindFirst(NULL, m_sel.UserSid(), &cfd));
            if (hFind.IsValidHandle())
            {
                do
                {
                    CscDelete(cfd.fd.cFileName);
                }
                while(CacheFindNext(hFind, &cfd));
            }
        }
    }
    catch(CException &e)
    {
        hr = e.dwError;
    }
   
    return hr;
}


//
// Displays a modal dialog to get cache purging confirmation from the 
// user.  Let's user indicate if they want to purge only temp files
// from the cache or both temp and pinned.
//
// Returns PURGE_FLAG_XXXX flags and a list of share names
// in the CCachePurgerSel object.
//
void
CCachePurger::AskUserWhatToPurge(
    HWND hwndParent,
    CCachePurgerSel *pSel
    )
{
    CConfirmPurgeDialog dlg;
    dlg.Run(GetModuleHandle(TEXT("cscui.dll")), hwndParent, pSel);
}


//
// Returns:
//      0 = User cancelled.
//      1 = User pressed OK.
//
// Returns PURGE_FLAG_XXXX flags and a list of share names
// in the CCachePurgerSel object.
//
int
CConfirmPurgeDialog::Run(
    HINSTANCE hInstance,
    HWND hwndParent,
    CCachePurgerSel *pSel        // We don't "own" this.  Merely a WRITE reference.
    )
{
    DBGASSERT((NULL != hInstance));
    DBGASSERT((NULL != hwndParent));
    DBGASSERT((NULL != pSel));
   
    m_hInstance = hInstance;

    m_pSel = pSel;

    int iResult = (int)DialogBoxParam(hInstance, 
                                      MAKEINTRESOURCE(IDD_CONFIRM_PURGE),
                                      hwndParent,
                                      DlgProc,
                                      (LPARAM)this);
    if (-1 == iResult)
    {
        DBGERROR((TEXT("Error %d creating delete confirmation dialog"), GetLastError()));
    }
    return iResult;
}




INT_PTR CALLBACK
CConfirmPurgeDialog::DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CConfirmPurgeDialog *pThis = reinterpret_cast<CConfirmPurgeDialog *>(GetWindowLongPtr(hwnd, DWLP_USER));

    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
                SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)lParam);
                pThis = reinterpret_cast<CConfirmPurgeDialog *>(lParam);
                DBGASSERT((NULL != pThis));
                pThis->OnInitDialog(hwnd);
                return TRUE;

            case WM_ENDSESSION:
                EndDialog(hwnd, IDNO);
                return FALSE;

            case WM_COMMAND:
            {
                int iResult = 0; // Assume [Cancel]
                switch(LOWORD(wParam))
                {
                    case IDOK:
                        iResult = 1;
                        pThis->OnOk();
                        //
                        // Fall through...
                        //
                    case IDCANCEL:
                        EndDialog(hwnd, iResult);
                        return FALSE;
                }
            }
            break;

            case WM_SETTINGCHANGE:
            case WM_SYSCOLORCHANGE:
                pThis->OnSettingChange(message, wParam, lParam);
                break;
                
            case WM_DESTROY:
                pThis->OnDestroy();
                pThis->m_hwnd = NULL;
                return FALSE;

            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CConfirmPurgeDialog::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }

    return FALSE;
}


void
CConfirmPurgeDialog::OnInitDialog(
    HWND hwnd
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CConfirmPurgeDialog::OnInitDialog")));
    DBGASSERT((NULL != hwnd));

    RECT rc;
    m_hwnd   = hwnd;
    m_hwndLV = GetDlgItem(hwnd, IDC_LIST_PURGE);
    CheckDlgButton(hwnd, IDC_RBN_CONFIRMPURGE_UNPINNED, BST_CHECKED);
    CheckDlgButton(hwnd, IDC_RBN_CONFIRMPURGE_ALL,      BST_UNCHECKED);

    //
    // Turn on checkboxes in the listview.
    //
    ListView_SetExtendedListViewStyleEx(m_hwndLV, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    //
    // Add the single column to the listview.
    //
    GetClientRect(m_hwndLV, &rc);

    LV_COLUMN col = { LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM, 
                      LVCFMT_LEFT, 
                      rc.right - rc.left, 
                      TEXT(""), 
                      0, 
                      0 };

    ListView_InsertColumn(m_hwndLV, 0, &col);

    //
    // Create the image list for the listview.
    //
    HIMAGELIST hSmallImages = ImageList_Create(16, 16, ILC_MASK, 1, 0);
    if (NULL != hSmallImages)
    {
        HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SHARE));
        if (NULL != hIcon)
        {
            ImageList_AddIcon(hSmallImages, hIcon);
        }
        ListView_SetImageList(m_hwndLV, hSmallImages, LVSIL_SMALL);        
    }

    //
    // Fill "shares" list with share names.
    //
    CscFindData cfd;
    CscFindHandle hFind(CacheFindFirst(NULL, &cfd));
    if (hFind.IsValidHandle())
    {
        LPITEMIDLIST pidl = NULL;
        SHFILEINFO sfi;
        CSCSHARESTATS ss;
        CSCGETSTATSINFO si = { SSEF_NONE, SSUF_TOTAL, true, false };
         
        do
        {
            _GetShareStatisticsForUser(cfd.fd.cFileName, &si, &ss);
            if (0 < ss.cTotal)
            {
                ZeroMemory(&sfi, sizeof(sfi));
                if (SUCCEEDED(SHSimpleIDListFromFindData(cfd.fd.cFileName, &cfd.fd, &pidl)))
                {
                    SHGetFileInfo((LPCTSTR)pidl,
                                  0,
                                  &sfi,
                                  sizeof(sfi),
                                  SHGFI_PIDL | SHGFI_DISPLAYNAME);
                    SHFree(pidl);
                }
                int iItem = LVAddItem(m_hwndLV, sfi.szDisplayName);
                if (0 <= iItem)
                {
                    //
                    // All items are initially checked.
                    //
                    ListView_SetCheckState(m_hwndLV, iItem, TRUE);
                    //
                    // Each item's lParam contains a pointer to the
                    // UNC path allocated on the heap.  Must be deleted
                    // in OnDestroy().
                    //
                    LVSetItemData(m_hwndLV, iItem, (LPARAM)MyStrDup(cfd.fd.cFileName));
                }
            }
        }
        while(CacheFindNext(hFind, &cfd));
    }
    if (0 == ListView_GetItemCount(m_hwndLV))
    {
        //
        // No items are in the listview.
        // Disable all of the controls, hide the "OK" button and 
        // change the "Cancel" button to "Close".
        //
        const UINT rgidCtls[] = { IDC_TXT_CONFIRMPURGE3,
                                  IDC_RBN_CONFIRMPURGE_UNPINNED,
                                  IDC_RBN_CONFIRMPURGE_ALL,
                                  IDC_LIST_PURGE,
                                  IDOK};
                                  
        ShowWindow(GetDlgItem(m_hwnd, IDOK), SW_HIDE);

        for (int i = 0; i < ARRAYSIZE(rgidCtls); i++)                                  
        {
            EnableWindow(GetDlgItem(m_hwnd, rgidCtls[i]), FALSE);
        }

        TCHAR szText[MAX_PATH];
        LoadString(m_hInstance, IDS_BTN_TITLE_CLOSE, szText, ARRAYSIZE(szText));
        SetWindowText(GetDlgItem(m_hwnd, IDCANCEL), szText);
        //
        // Replace the listview's caption with something like "There are
        // no offline files to delete".
        //
        LoadString(m_hInstance, IDS_TXT_NO_FILES_TO_DELETE, szText, ARRAYSIZE(szText));
        SetWindowText(GetDlgItem(m_hwnd, IDC_TXT_CONFIRMPURGE2), szText);
        //
        // Uncheck both radio buttons.
        //
        CheckDlgButton(m_hwnd, IDC_RBN_CONFIRMPURGE_UNPINNED, BST_UNCHECKED);
        CheckDlgButton(m_hwnd, IDC_RBN_CONFIRMPURGE_ALL, BST_UNCHECKED);
    }
}


LPARAM
CConfirmPurgeDialog::LVGetItemData(
    HWND hwndLV,
    int i
    )
{
    LVITEM item;
    item.mask     = LVIF_PARAM;
    item.iItem    = i;
    item.iSubItem = 0;

    if (ListView_GetItem(hwndLV, &item))
    {
        return item.lParam;
    }
    return 0;
}


BOOL
CConfirmPurgeDialog::LVSetItemData(
    HWND hwndLV,
    int i,
    LPARAM lParam
    )
{
    LVITEM item;
    item.mask     = LVIF_PARAM;
    item.iItem    = i;
    item.iSubItem = 0;
    item.lParam   = lParam;

    return ListView_SetItem(hwndLV, &item);
}


int
CConfirmPurgeDialog::LVAddItem(
    HWND hwndLV,
    LPCTSTR pszItem
    )
{
    LVITEM item;
    item.mask     = LVIF_TEXT;
    item.pszText  = (LPTSTR)pszItem;
    item.iSubItem = 0;
    item.iItem    = ListView_GetItemCount(hwndLV);

    return ListView_InsertItem(hwndLV, &item);
}


void 
CConfirmPurgeDialog::OnOk(
    void
    )
{
    const int cShares = ListView_GetItemCount(m_hwndLV);
    for (int i = 0; i < cShares; i++)
    {
        if (0 != ListView_GetCheckState(m_hwndLV, i))
        {
            m_pSel->AddShareName((LPCTSTR)LVGetItemData(m_hwndLV, i));
        }
    }
    
    if (0 < m_pSel->ShareCount())
    {
        m_pSel->SetFlags((BST_CHECKED == IsDlgButtonChecked(m_hwnd, IDC_RBN_CONFIRMPURGE_UNPINNED)) ? 
                          PURGE_FLAG_UNPINNED : PURGE_FLAG_ALL);
    }
    else
    {
        m_pSel->SetFlags(PURGE_FLAG_NONE);
    }
}


void
CConfirmPurgeDialog::OnDestroy(
    void
    )
{
    if (NULL != m_hwndLV)
    {
        const int cShares = ListView_GetItemCount(m_hwndLV);
        for (int i = 0; i < cShares; i++)
        {
            delete[] (LPTSTR)LVGetItemData(m_hwndLV, i);        
        }
    }
}

void
CConfirmPurgeDialog::OnSettingChange(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (NULL != m_hwndLV)
        SendMessage(m_hwndLV, uMsg, wParam, lParam);
}


LPTSTR 
CConfirmPurgeDialog::MyStrDup(
    LPCTSTR psz
    )
{
    LPTSTR pszCopy = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszCopy)
        lstrcpy(pszCopy, psz);

    return pszCopy;
}



CCachePurgerSel::~CCachePurgerSel(
    void
    )
{
    if (NULL != m_hdpaShares)
    {
        const int cShares = DPA_GetPtrCount(m_hdpaShares);
        for (int i = 0; i < cShares; i++)
        {
            delete[] DPA_GetPtr(m_hdpaShares, i);
        }
        DPA_Destroy(m_hdpaShares);
    }
    if (NULL != m_psidUser && IsValidSid(m_psidUser))
    {
        FreeSid(m_psidUser);
    }
}


LPTSTR 
CCachePurgerSel::MyStrDup(
    LPCTSTR psz
    )
{
    LPTSTR pszCopy = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszCopy)
        lstrcpy(pszCopy, psz);

    return pszCopy;
}


BOOL
CCachePurgerSel::SetUserSid(
    PSID psid
    )
{
    if (NULL != m_psidUser && IsValidSid(m_psidUser))
    {
        FreeSid(m_psidUser);
        m_psidUser = NULL;
    }
    if (NULL != psid && IsValidSid(psid))
    {
        DWORD cbSid = GetLengthSid(psid);
        PSID psidNew = (PSID)new BYTE[cbSid];
        if (NULL != psidNew)
        {
            if (!CopySid(cbSid, psidNew, psid))
            {
                delete[] psidNew;
                psidNew = NULL;
            }
            m_psidUser = psidNew;
        }
    }
    return NULL != m_psidUser;
}


BOOL 
CCachePurgerSel::AddShareName(
    LPCTSTR pszShare
    )
{
    //
    // Be tolerant of a NULL pszShare pointer.
    //
    if (NULL != m_hdpaShares && NULL != pszShare)
    {
        LPTSTR pszCopy = MyStrDup(pszShare);
        if (NULL != pszCopy)
        {
            if (-1 != DPA_AppendPtr(m_hdpaShares, pszCopy))
            {
                return true;
            }
            delete[] pszCopy;
        }
    }
    return false;
}


typedef struct _RemoveFolderCBData
{
    PFN_CSCUIRemoveFolderCallback pfnCB;
    LPARAM lParam;
} RemoveFolderCBData, *PRemoveFolderCBData;

BOOL CALLBACK
_RemoveFolderCallback(CCachePurger *pPurger)
{
    PRemoveFolderCBData pcbdata = (PRemoveFolderCBData)pPurger->CallbackData();
    if (pcbdata->pfnCB)
        return pcbdata->pfnCB(pPurger->FileName(), pcbdata->lParam);
    return TRUE;
}

STDAPI
CSCUIRemoveFolderFromCache(LPCWSTR pszFolder,
                           DWORD /*dwReserved*/,    // can use for flags
                           PFN_CSCUIRemoveFolderCallback pfnCB,
                           LPARAM lParam)
{
    RemoveFolderCBData cbdata = { pfnCB, lParam };

    CCachePurgerSel sel;
    sel.SetFlags(PURGE_FLAG_ALL | PURGE_IGNORE_ACCESS);
    sel.AddShareName(pszFolder);

    CCachePurger purger(sel, _RemoveFolderCallback, &cbdata);

    return purger.Delete();
}
