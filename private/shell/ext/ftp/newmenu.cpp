/*****************************************************************************\
    FILE: newmenu.cpp
    
    DESCRIPTION:
        The file supports the "New" menu to create new items on the FTP server.
    This currently only supports Folders but hopefully it will support other
    items later.
\*****************************************************************************/

#include "priv.h"
#include "util.h"
#include "newmenu.h"

// This is used to surf the hwnds to find the one we need to
// hack because IShellView2::SelectAndPositionItem() isn't implemented
// on Browser Only.
#define DEFVIEW_CLASS_BROWSERONLYA       "SHELLDLL_DefView"


/////////////////////////////////////////////////////////////////////////
///////  Private helpers    /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

LPITEMIDLIST DV_GetPIDL(HWND hwndLV, int i)
{
    LV_ITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = i;
    item.iSubItem = 0;
    item.lParam = 0;
    if (i != -1)
    {
        ListView_GetItem(hwndLV, &item);
    }

    return (LPITEMIDLIST) item.lParam;
}


int DefView_FindItemHack(CFtpFolder * pff, HWND hwndListView, LPCITEMIDLIST pidl)
{
    int nIndex;
    int nItemsTotal;

    nItemsTotal = ListView_GetItemCount(hwndListView);
    for (nIndex = 0; nItemsTotal > nIndex; nIndex++)
    {
        HRESULT hres = ResultFromShort(-1);
        LPITEMIDLIST pidlT = DV_GetPIDL(hwndListView, nIndex);
        if (!pidlT)
            return -1;

        // BUGBUG: this passes 0 for the lParam
        hres = pff->CompareIDs(0, pidl, pidlT);

        ASSERT(SUCCEEDED(hres));
        if (FAILED(hres))
            return -1;

        if (ShortFromResult(hres) == 0)
        {
            return nIndex;
        }
    }

    return -1;  // not found
}


typedef struct tagFOLDERNAMECOMP
{
    BOOL *      pfFound;
    LPCWSTR     pszFolderName;
} FOLDERNAMECOMP;


/*****************************************************************************\
    FUNCTION: _ComparePidlAndFolderStr

    DESCRIPTION:
        Compare the pidl and folder name str.
\*****************************************************************************/
int _ComparePidlAndFolderStr(LPVOID pvPidl, LPVOID pvFolderNameComp)
{
    FOLDERNAMECOMP * pFolderNameComp = (FOLDERNAMECOMP *) pvFolderNameComp;
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) pvPidl;
    WCHAR wzDisplayName[MAX_PATH];
    BOOL fContinue = TRUE;

    if (EVAL(SUCCEEDED(FtpPidl_GetDisplayName(pidl, wzDisplayName, ARRAYSIZE(wzDisplayName)))))
    {
        if (!StrCmpW(wzDisplayName, pFolderNameComp->pszFolderName))
        {
            *pFolderNameComp->pfFound = TRUE;
            fContinue = FALSE;
        }
    }

    return fContinue;   // Continue looking?
}


/*****************************************************************************\
    FUNCTION: _DoesFolderExist

    DESCRIPTION:
        Look thru all the items (files and folders) in this folder and see if
    any have the same name as pszFolderName.
\*****************************************************************************/
BOOL _DoesFolderExist(LPCWSTR pszFolderName, CFtpDir * pfd)
{
    BOOL fExist = FALSE;
    if (EVAL(pfd))
    {
        CFtpPidlList * pPidlList = pfd->GetHfpl();

        // This may fail, but the worst that will happen is that the new folder won't appear.
        // This happens when the cache is flushed.
        if (pPidlList)
        {
            FOLDERNAMECOMP folderNameComp = {&fExist, pszFolderName};

            pPidlList->Enum(_ComparePidlAndFolderStr, (LPVOID) &folderNameComp);
            pPidlList->Release();
        }
    }

    return fExist;
}


/*****************************************************************************\
    FUNCTION: _CreateNewFolderName

    DESCRIPTION:
        Create the name of a new folder.
\*****************************************************************************/
HRESULT _CreateNewFolderName(LPWSTR pszNewFolder, DWORD cchSize, CFtpDir * pfd)
{
    HRESULT hr = S_OK;
    int nTry = 1;
    WCHAR wzTemplate[MAX_PATH];

    wzTemplate[0] = 0;

    LoadStringW(HINST_THISDLL, IDS_NEW_FOLDER_FIRST, pszNewFolder, cchSize);
    while (_DoesFolderExist(pszNewFolder, pfd))
    {
        if (0 == wzTemplate[0])
            LoadStringW(HINST_THISDLL, IDS_NEW_FOLDER_TEMPLATE, wzTemplate, ARRAYSIZE(wzTemplate));

        nTry++; // Try the next number.
        wnsprintf(pszNewFolder, cchSize, wzTemplate, nTry);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _CreateNewFolder

    DESCRIPTION:
        Create the actual directory.
\*****************************************************************************/
HRESULT CreateNewFolderCB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pvFCFS, BOOL * pfReleaseHint)
{
    HRESULT hr = S_OK;
    FTPCREATEFOLDERSTRUCT * pfcfs = (FTPCREATEFOLDERSTRUCT *) pvFCFS;
    WIRECHAR wFilePath[MAX_PATH];
    CWireEncoding * pWireEncoding = phpi->pfd->GetFtpSite()->GetCWireEncoding();

    hr = pWireEncoding->UnicodeToWireBytes(NULL, pfcfs->pszNewFolderName, (phpi->pfd->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), wFilePath, ARRAYSIZE(wFilePath));
    if (EVAL(SUCCEEDED(hr)))
    {
        hr = FtpCreateDirectoryWrap(hint, TRUE, wFilePath);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlNew;
            HINTERNET hIntFind;

            // For some reason, FtpFindFirstFile needs an '*' behind the name.
            StrCatA(wFilePath, SZ_ASTRICSA);

            hr = FtpFindFirstFilePidlWrap(hint, TRUE, NULL, pWireEncoding, wFilePath, &pidlNew, (INTERNET_NO_CALLBACK | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RESYNCHRONIZE | INTERNET_FLAG_RELOAD), 0, &hIntFind);
            if (EVAL(SUCCEEDED(hr)))
            {
                // Notify the folder of the new item so the Shell Folder updates.
                // PERF: I worry about doing a FtpFindFirstFile() being too expensive onto to get the date correct
                //       for SHChangeNotify().
                FtpChangeNotify(phpi->hwnd, SHCNE_MKDIR, pfcfs->pff, phpi->pfd, pidlNew, NULL, TRUE);

                ILFree(pidlNew);
                InternetCloseHandle(hIntFind);
            }
        }
    }

    return hr;
}




/////////////////////////////////////////////////////////////////////////
///////  DLL Wide Functions    /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

HRESULT CreateNewFolder(HWND hwnd, CFtpFolder * pff, CFtpDir * pfd, IUnknown * punkSite, BOOL fPosition, POINT point)
{
    HRESULT hr = E_FAIL;
    CFtpDir * pfdTemp = NULL;

    if (!pfd)
        pfd = pfdTemp = pff->GetFtpDir();

    if (EVAL(pfd))
    {
        WCHAR wzNewFolderName[MAX_PATH];

        // 1. Check if "New Folder" exists.
        // 2. Cycle thru names until a unique name is found.
        hr = _CreateNewFolderName(wzNewFolderName, ARRAYSIZE(wzNewFolderName), pfd);
        if (EVAL(SUCCEEDED(hr) && pfd))
        {
            FTPCREATEFOLDERSTRUCT fcfs = {wzNewFolderName, pff};

            // 3. Create a Directory with that name.
            hr = pfd->WithHint(NULL, hwnd, CreateNewFolderCB, (LPVOID) &fcfs, punkSite, pff);
            if (SUCCEEDED(hr))
            {
                WIRECHAR wNewFolderWireName[MAX_PATH];
                LPITEMIDLIST pidlFolder = NULL;
                CWireEncoding * pWireEncoding = pff->GetCWireEncoding();

                // Give me UTF-8 baby.
                EVAL(SUCCEEDED(pWireEncoding->UnicodeToWireBytes(NULL, wzNewFolderName, (pfd->IsUTF8Supported() ? WIREENC_USE_UTF8 : WIREENC_NONE), wNewFolderWireName, ARRAYSIZE(wNewFolderWireName))));
                if (EVAL(SUCCEEDED(FtpItemID_CreateFake(wzNewFolderName, wNewFolderWireName, TRUE, FALSE, FALSE, &pidlFolder))))
                {
                    // Is this browser only?
                    if (SHELL_VERSION_W95NT4 == GetShellVersion())
                    {
                        HWND hwndDefView = NULL;
                        // Yes, so we need to do this the hard way.
                
                        // 1. 
                        ShellFolderView_SetItemPos(hwnd, pidlFolder, point.x, point.y);
                        hwndDefView = FindWindowExA(hwnd, NULL, DEFVIEW_CLASS_BROWSERONLYA, NULL);

                        if (EVAL(hwndDefView))
                        {
                            HWND hwndListView = FindWindowExA(hwndDefView, NULL, WC_LISTVIEWA, NULL);

                            if (EVAL(hwndListView))
                            {
                                 int nIndex = DefView_FindItemHack(pff, hwndListView, pidlFolder);

                                 if (EVAL(-1 != nIndex))
                                    ListView_EditLabel(hwndListView, nIndex);
                            }
                        }
                    }
                    else
                    {
                        // No, so this won't be as hard.
                        IShellView2 * pShellView2 = NULL;

//                      ASSERT(punkSite);   // Can happen when invoked from Captionbar.
                        IUnknown_QueryService(punkSite, SID_DefView, IID_IShellView2, (void **)&pShellView2);
                        if (!pShellView2)
                        {
                            IDefViewFrame * pdvf = NULL;
                            IUnknown_QueryService(punkSite, SID_DefView, IID_IDefViewFrame, (void **)&pdvf);
                            if (pdvf)   // Can fail when invoked from caption bar.
                            {
                                EVAL(SUCCEEDED(pdvf->QueryInterface(IID_IShellView2, (void **) &pShellView2)));
                                pdvf->Release();
                            }
                        }

                        if (pShellView2)    // Can fail when invoked from the caption bar.  Oh well, cry me a river.
                        {
                            if (fPosition)  // BUGBUG: This looks wrong.  (Should be ! (?))
                                pShellView2->SelectAndPositionItem(pidlFolder, (SVSI_SELECT | SVSI_TRANSLATEPT | SVSI_EDIT), &point);
                            else
                                pShellView2->SelectItem(pidlFolder, (SVSI_EDIT | SVSI_SELECT));

                            pShellView2->Release();
                        }
                    }

                    ILFree(pidlFolder);
                }
            }
            else
            {
                // An error occured, so display UI.  Most often because access is denied.
                DisplayWininetError(hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_NEWFOLDER, IDS_FTPERR_WININET, MB_OK, NULL);
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
        }

        if (pfdTemp)
            pfdTemp->Release();
    }


    return hr;
}



