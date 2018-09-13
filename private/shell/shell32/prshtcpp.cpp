//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       prshtprog.cpp
//
//  Contents:   Code for multi and single file property sheet progress dialogs
//
//----------------------------------------------------------------------------


#include "shellprv.h"
#pragma  hdrstop

#include "prshtcpp.h"
#include "propsht.h"
#include "treewkcb.h"   // for CBaseTreeWalkerCB class
#include "ftascstr.h"   // for CFTAssocStore
#include "ftcmmn.h"     // for MAX_APPFRIENDLYNAME
#include "ascstr.h"     // for IAssocInfo class

//
// Folder attribute tree waker class
//
class CFolderAttribTreeWalker : public CBaseTreeWalkerCB
{
    friend BOOL _stdcall ApplyRecursiveFolderAttribs(LPCTSTR pszDir, FILEPROPSHEETPAGE* pfpsp);
public:
    // constructor
    CFolderAttribTreeWalker(FILEPROPSHEETPAGE* pfpsp);

    // *** IShellTreeWalkerCallBack overridden methods ***
    virtual STDMETHODIMP FoundFile(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    virtual STDMETHODIMP EnterFolder(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);

protected:
    FILEPROPSHEETPAGE* _pfpsp;
    LPDWORD _pdwError;
}; 


//
// CFolderAttribTreeWalker consructor
//
CFolderAttribTreeWalker::CFolderAttribTreeWalker(FILEPROPSHEETPAGE* pfpsp): _pfpsp(pfpsp)
{
}


//
// CFolderAttribTreeWalker::FoundFile 
//
HRESULT CFolderAttribTreeWalker::FoundFile(LPCWSTR pwszFile, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW* pwfd)
{
    HWND hwndParent = NULL;
    TCHAR szFileName[MAX_PATH];
    ULARGE_INTEGER ulSizeOnDisk;
    BOOL bSomethingChanged;

    // check to see if the user hit cancel on the progress dlg
    if (HasUserCanceledAttributeProgressDlg(_pfpsp))
    {
        return E_FAIL;
    }

    if (_pfpsp->pProgressDlg)
    {
        // if we have a progress hwnd, try to use it
        // this will fail if the progress dialog hasent been displayed yet.
        IUnknown_GetWindow(_pfpsp->pProgressDlg, &hwndParent);
    }

    if (!hwndParent)
    {
        // if we dont have a progress hwnd, use the property page hwnd
        hwndParent = GetParent(_pfpsp->hDlg);
    }

    // thunk the pwszFile string
    SHUnicodeToTChar(pwszFile, szFileName, ARRAYSIZE(szFileName));

    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        // BUGBUG (reinerf) - in the directory case, we set the size to zero since the
        // FolderSize() function dosen't add in the sizes of directories, and we need
        // the sizes to match when doing progress calcuations.
        _pfpsp->fd.nFileSizeLow = 0;
        _pfpsp->fd.nFileSizeHigh = 0;
    }
    else
    {
#ifdef WINNT
        // if compression is supported, we check to see if the file is sparse or compressed
        if (_pfpsp->fIsCompressionAvailable && (pwfd->dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)))
        {
            ulSizeOnDisk.LowPart = SHGetCompressedFileSize(szFileName, &ulSizeOnDisk.HighPart);
        }
        else
#endif
        {
            // the file isint comrpessed so just round to the cluster size
            ulSizeOnDisk.LowPart = pwfd->nFileSizeLow;
            ulSizeOnDisk.HighPart = pwfd->nFileSizeHigh;
            ulSizeOnDisk.QuadPart = ROUND_TO_CLUSTER(ulSizeOnDisk.QuadPart, ptws->dwClusterSize);
        }

        // we set this so the progress dialog knows how much to update the progress slider
        _pfpsp->fd.nFileSizeLow = ulSizeOnDisk.LowPart;
        _pfpsp->fd.nFileSizeHigh = ulSizeOnDisk.HighPart;
    }

    if (!ApplyFileAttributes(szFileName, _pfpsp, hwndParent, &bSomethingChanged))
    {
        // the user hit cancel, so stop
        return E_FAIL;
    }

    return S_OK;
}
 
//
// CFolderAttribTreeWalker::EnterFolder
//
HRESULT CFolderAttribTreeWalker::EnterFolder(LPCWSTR pwszDir, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW* pwfd)
{
    return FoundFile(pwszDir, ptws, pwfd);
}


STDAPI_(BOOL) ApplyRecursiveFolderAttribs(LPCTSTR pszDir, FILEPROPSHEETPAGE* pfpsp)
{
    HRESULT hres = E_FAIL;
    CFolderAttribTreeWalker* pfatw = new CFolderAttribTreeWalker(pfpsp);
    
    if (pfatw)
    {
        if (SUCCEEDED(pfatw->Initialize()))
        {
            WCHAR wszDir[MAX_PATH];

            ASSERT(IS_VALID_CODE_PTR(pfatw->_pstw, IShellTreeWalker));
#ifdef UNICODE
            lstrcpyn(wszDir, pszDir, ARRAYSIZE(wszDir));
#else
            MultiByteToWideChar(CP_ACP, 0, pszDir, -1, wszDir, ARRAYSIZE(wszDir));
#endif
            hres = pfatw->_pstw->WalkTree(WT_NOTIFYFOLDERENTER, wszDir, NULL, 0, SAFECAST(pfatw, IShellTreeWalkerCallBack*));
        }
        pfatw->Release();
    }

    return SUCCEEDED(hres) ? TRUE : FALSE;
}


//
// Checks the progress dialog to see if the user has hit cancel
//
STDAPI_(BOOL) HasUserCanceledAttributeProgressDlg(FILEPROPSHEETPAGE* pfpsp)
{
    ASSERT(pfpsp->pProgressDlg);
    return pfpsp->pProgressDlg->HasUserCancelled();
}


//
// Creates the CProgressDialog object used by the attribs dlg
//
STDAPI_(BOOL) CreateAttributeProgressDlg(FILEPROPSHEETPAGE* pfpsp)
{
    WCHAR wzBuffer[MAX_PATH];

    ASSERT(pfpsp->fMultipleFiles || pfpsp->fRecursive);
    
    // create the progress dialog as a modal window
    pfpsp->pProgressDlg = CProgressDialog_CreateInstance(IDS_APPLYINGATTRIBS, IDA_APPLYATTRIBS, HINST_THISDLL);

    if (!pfpsp->pProgressDlg)
    {
        // couldn't create a progress dialog, so bail
        return FALSE;
    }

    // set the static string "Applying Attrbiutes to:"
    EVAL(LoadStringW(HINST_THISDLL, IDS_APPLYINGATTRIBSTO, wzBuffer, ARRAYSIZE(wzBuffer)));
    EVAL(SUCCEEDED(pfpsp->pProgressDlg->SetLine(1, wzBuffer, FALSE, NULL)));

    EVAL(SUCCEEDED(pfpsp->pProgressDlg->StartProgressDialog(GetParent(pfpsp->hDlg), NULL, (PROGDLG_MODAL | PROGDLG_AUTOTIME), NULL)));
    return TRUE;
}


//
// Delets the CProgressDialog object used by the attribs dlg
//
STDAPI_(BOOL) DestroyAttributeProgressDlg(FILEPROPSHEETPAGE* pfpsp)
{
    if (!pfpsp->pProgressDlg)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    if (!(pfpsp->fMultipleFiles || pfpsp->fRecursive))
        return FALSE;

    EVAL(SUCCEEDED(pfpsp->pProgressDlg->StopProgressDialog()));
    pfpsp->pProgressDlg->Release();
    pfpsp->pProgressDlg = NULL;

    // reset NumberOfBytesDone so we are back to zero if the user tries something else,
    // we will start over at zero
    pfpsp->ulNumberOfBytesDone.QuadPart = 0;
    pfpsp->cItemsDone = 0;
    
    return TRUE;
}


//
// Sets the current file we are applying attribs for in the progress dlg
//
STDAPI SetProgressDlgPath(FILEPROPSHEETPAGE* pfpsp, LPCTSTR pszPath, BOOL fCompactPath)
{
    WCHAR wzPath[MAX_PATH];
    ASSERT(pfpsp->pProgressDlg);

    SHTCharToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));
    return pfpsp->pProgressDlg->SetLine(2, wzPath, fCompactPath, NULL);
}


//
// Updates the progress bar in the dlg
//
STDAPI UpdateProgressBar(FILEPROPSHEETPAGE* pfpsp)
{
    ASSERT(pfpsp->pProgressDlg);

    pfpsp->cItemsDone++;
    
    // if we are not changing compression or encryption, then
    // do progress based on the number of items we are applying to.
    if (pfpsp->asCurrent.fCompress == pfpsp->asInitial.fCompress &&
        pfpsp->asCurrent.fEncrypt == pfpsp->asInitial.fEncrypt)
    {
        if (pfpsp->fRecursive)
        {
            //progress is based on number of items done out of total items in all folders
            return pfpsp->pProgressDlg->SetProgress(pfpsp->cItemsDone, pfpsp->fci.cFiles + pfpsp->fci.cFolders);
        }
        else
        {
            //progress is based on number of items done out of total selected items
            return pfpsp->pProgressDlg->SetProgress(pfpsp->cItemsDone, HIDA_GetCount(pfpsp->hida));
        }
    }
    else
    {
        // since we are either encrypting or compressing, we do progress based on the sizes of the files
        return pfpsp->pProgressDlg->SetProgress64(pfpsp->ulNumberOfBytesDone.QuadPart, pfpsp->ulTotalNumberOfBytes.QuadPart);
    }
}


void EnableAndShowWindow(HWND hWnd, BOOL bShow)
{
    ShowWindow(hWnd, bShow ? SW_SHOW : SW_HIDE);
    EnableWindow(hWnd, bShow);
}

// we dynamically size the text depeneding on whether or not the small icon is visible and
// if the "Change..." button is visible
void SizeOpensWithTextBox(FILEPROPSHEETPAGE* pfpsp, BOOL bIsSmallIconVisible, BOOL bIsOpensWithEnabled)
{
    RECT rcArray[3]; // array of three rects: the IDD_TYPEICON rect, the IDC_CHANGEFILETYPE rect, and the IDD_OPENSWITH rect
    RECT* prcSmallIcon = &rcArray[0];
    RECT* prcChangeButton = &rcArray[1];
    RECT* prcText = &rcArray[2];
    BOOL  bFailed = FALSE;

    GetWindowRect(GetDlgItem(pfpsp->hDlg, IDD_TYPEICON), &rcArray[0]);
    GetWindowRect(GetDlgItem(pfpsp->hDlg, IDC_CHANGEFILETYPE), &rcArray[1]);
    GetWindowRect(GetDlgItem(pfpsp->hDlg, IDD_OPENSWITH), &rcArray[2]);

    // map the rects into dlg coordiates
    // MapWindowPoints is mirroring aware only when you pass one rect. let's loop.
    for (int i =0; i < ARRAYSIZE(rcArray); i++ )
    {
        if(!(MapWindowPoints(NULL, pfpsp->hDlg, (LPPOINT)(&rcArray[i]), 2)))
        {
            bFailed = TRUE;
            break;
        }
    }
    if (!bFailed)
    {
        RECT rcTemp = {0,0,4,0}; // we need to find out how many pixels are in 4 DLU's worth of witdth

        MapDialogRect(pfpsp->hDlg, &rcTemp);

        if (bIsSmallIconVisible)
        {
            prcText->left = prcSmallIcon->right + rcTemp.right; // spacing between controls is 4 DLU's
        }
        else
        {
            prcText->left = prcSmallIcon->left;
        }

        if (bIsOpensWithEnabled)
        {
            prcText->right = prcChangeButton->left - rcTemp.right; // spacing between controls is 4 DLU's
        }
        else
        {
            prcText->right = prcChangeButton->right;
        }

        SetWindowPos(GetDlgItem(pfpsp->hDlg, IDD_OPENSWITH),
                     HWND_BOTTOM,
                     prcText->left,
                     prcText->top,
                     (prcText->right - prcText->left),
                     (prcText->bottom - prcText->top),
                     SWP_NOZORDER);
    }
}

// this function sets the "Opens With:" / "Description:" text based on whether or not 
// we are allowed to change the assocation, and enables / disables the "Opens With..." button
void SetDescriptionAndOpensWithBtn(FILEPROPSHEETPAGE* pfpsp, BOOL fAllowModifyOpenWith)
{
    TCHAR szOpensWithText[MAX_PATH];

    LoadString(HINST_THISDLL, fAllowModifyOpenWith ? IDS_OPENSWITH : IDS_DESCRIPTION, szOpensWithText, ARRAYSIZE(szOpensWithText));
    SetDlgItemText(pfpsp->hDlg, IDD_OPENSWITH_TXT, szOpensWithText);
    
    // enable/disable the "Change..." button accordingly
    EnableAndShowWindow(GetDlgItem(pfpsp->hDlg, IDC_CHANGEFILETYPE), fAllowModifyOpenWith);
}

// set the Friendly Name text (eg the text to the right of the "Opens With:" / "Description:" field
void SetFriendlyNameText(LPTSTR pszPath, FILEPROPSHEETPAGE* pfpsp, IAssocInfo* pai, BOOL bIsSmallIconVisible)
{
    TCHAR szAppFriendlyName[MAX_PATH];
    DWORD cchFriendlyName = ARRAYSIZE(szAppFriendlyName);
    szAppFriendlyName[0] = TEXT('\0');

    if (pai)
    {
        if (FAILED(pai->GetString(AISTR_APPFRIENDLY, szAppFriendlyName, &cchFriendlyName)))
        {
            // if we failed, it could mean that this app is not associated yet. in this
            // case we just use "Unknown Applicaion"
            LoadString(HINST_THISDLL, IDS_UNKNOWNAPPLICATION, szAppFriendlyName, ARRAYSIZE(szAppFriendlyName));
        }
    }
    else
    {
        UINT cchBuff = (UINT)cchFriendlyName;

        // get the friendly name from the file itself
        if (!pszPath || !pszPath[0] || !GetFileDescription(pszPath, szAppFriendlyName, &cchBuff))
        {
            // use the short name as it appears in the "rename" edit box if something above didnt work
            lstrcpyn(szAppFriendlyName, pfpsp->szInitialName, ARRAYSIZE(szAppFriendlyName));
        }
    }

    ASSERT(szAppFriendlyName[0]);

    SetDlgItemText(pfpsp->hDlg, IDD_OPENSWITH, szAppFriendlyName);

    // size and position the text properly depending on wether the small icon is visible and
    // the state of the "Change..." button
    SizeOpensWithTextBox(pfpsp, bIsSmallIconVisible, IsWindowEnabled(GetDlgItem(pfpsp->hDlg, IDC_CHANGEFILETYPE)));
}

// sets the small icon in the description field
//
// return value:  TRUE  - a small icon is visible
//                FALSE - small icon was not set
//
BOOL SetSmallIcon(FILEPROPSHEETPAGE* pfpsp, IAssocInfo* pai, BOOL fAllowModifyOpenWith)
{
    HICON hIcon = NULL;
    HICON hIconOld = NULL;
    int iIcon;
    BOOL bShowSmallIcon;
    
    // only setup the small icon of the associated app if we have the "Change..." button 
    // and we were able to get the friendly name.
    if (fAllowModifyOpenWith && pai && SUCCEEDED(pai->GetDWORD(AIDWORD_APPSMALLICON, (DWORD*)&iIcon)))
    {
        HIMAGELIST hIL = NULL;

        Shell_GetImageLists(NULL, &hIL);
        if (hIL)
        {
            hIcon = ImageList_ExtractIcon(g_hinst, hIL, iIcon);
        }
    }

    // we will show the small icon if we got one and if we are allowed to modify the opens with
    bShowSmallIcon = (hIcon != NULL);

    hIconOld = (HICON)SendDlgItemMessage(pfpsp->hDlg, IDD_TYPEICON, STM_SETICON, (WPARAM)hIcon, 0);

    if (hIconOld)
        DestroyIcon(hIconOld);

    // enable/disable the IDD_TYPEICON icon accordingly
    EnableAndShowWindow(GetDlgItem(pfpsp->hDlg, IDD_TYPEICON), bShowSmallIcon);

    return bShowSmallIcon;
}


//
// We use this to set the text for the associated application and other goodies
//
STDAPI UpdateOpensWithInfo(FILEPROPSHEETPAGE* pfpsp)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];
    IAssocStore* pas = NULL;
    IAssocInfo* pai = NULL;
    BOOL fAllowChangeAssoc = TRUE;
    BOOL fAllowModifyOpenWith = TRUE;
    BOOL fIsLink = FALSE;
    BOOL bShowSmallIcon;
    LPTSTR pszExt;
    
    szPath[0] = TEXT('\0');

    // We need to check to see if this is a link. If so, then we need to get the information for
    // the link target
    if (pfpsp->fIsLink)
    {
        if (S_OK != GetPathFromLinkFile(pfpsp->szPath, szPath, ARRAYSIZE(szPath)))
        {
            // we failed for some strange reason, perhaps its a darwin link,
            // we just treat the file as if it were not a link. And we do not let
            // the user change the association
            fAllowModifyOpenWith = FALSE;
            pfpsp->fIsLink = FALSE;
        }
        else
        {
            // if the link target didn't change, we dont need to update anything
            if (pfpsp->szLinkTarget[0] && lstrcmpi(pfpsp->szLinkTarget, szPath) == 0)
            {
                return S_FALSE;
            }
        }
    }
    else
    {
        // just use the path of the file since it is not a link
        lstrcpyn(szPath, pfpsp->szPath, ARRAYSIZE(szPath));
    }

    // if we haven't initialized the AssocStore, do so now
    pas = (IAssocStore*)pfpsp->pAssocStore;
    if (!pas)
    {
        // BUGBUG (reinerf) - we should be using COM but the AssocStore isint COM enable yet.
        pas = new CFTAssocStore();
        pfpsp->pAssocStore = (LPVOID)pas;
    }

    if (!pfpsp->pAssocStore)
    {
        // if we couldn't make an AssocStore, so bail
        return E_OUTOFMEMORY;
    }

    pszExt = PathFindExtension(szPath);
    if (PathIsExe(szPath) || !szPath[0] || *pszExt == TEXT('\0'))
    {
        // this file is an .exe (or .com, .bat, etc) or GetPathFromLinkFile returned a 
        // null path (eg link to a special folder) or the file has no extension (eg 'c:\foo.',
        // or 'c:\'), then we dont want the user to be able to change the association since 
        // there isint one.
        fAllowModifyOpenWith = FALSE;
    }

    if (fAllowModifyOpenWith)
    {
        // get the AssocInfo for this file, based on its extension
        hr = pas->GetAssocInfo(pszExt, AIINIT_EXT, &pai);

#ifdef DEBUG
        if (FAILED(hr))
        {
            ASSERT(pai == NULL);
        }
#endif
    }

    BOOL fNoFileAssociate=FALSE;
#ifdef WINNT
    fNoFileAssociate = SHRestricted(REST_NOFILEASSOCIATE);
#endif

    SetDescriptionAndOpensWithBtn(pfpsp, fAllowModifyOpenWith & ~fNoFileAssociate );
    bShowSmallIcon = SetSmallIcon(pfpsp, pai, fAllowModifyOpenWith);
    SetFriendlyNameText(szPath, pfpsp, pai, bShowSmallIcon);


    if (pai)
    {
        pai->Release();
    }

    // save off the link target so we only update the stuff above when the target changes.
    if (pfpsp->fIsLink)
    {
        lstrcpyn(pfpsp->szLinkTarget, szPath, ARRAYSIZE(pfpsp->szLinkTarget));
    }
    else
    {
        // its not a link so reset the link target to the empty string
        pfpsp->szLinkTarget[0] = TEXT('\0');
    }

    // BUGBUG (reinerf) - need to update the big icon in the upper left hand corner of the prop sheet as well.
    return S_OK;
}


STDAPI CleanupOpensWithInfo(FILEPROPSHEETPAGE* pfpsp)
{
    HICON hIcon = (HICON)SendDlgItemMessage(pfpsp->hDlg, IDD_TYPEICON, STM_GETICON, 0, 0);
    IAssocStore* pas = (IAssocStore*)pfpsp->pAssocStore;

    // we release the AssocStore we are holding
    if (pas)
    {
        pfpsp->pAssocStore = NULL;
        delete pas;
    }

    if (hIcon)
    {
        DestroyIcon(hIcon);
    }

    return S_OK;
}
