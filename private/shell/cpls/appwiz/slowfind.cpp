//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: slowfind.cpp
//
// Implements CProgFilesAppFinder
//            CStartMenuAppFinder
// History:
//         3-01-98  by dli implemented CProgFilesAppFinder
//         4-15-98  by dli implemented CStartMenuAppFinder
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "appsize.h"
#include "findapp.h"
#include "slowfind.h"


// Todo: Remember the find result somewhere in the registry or cache it in code
// so that we don't waste time repeatedly computing it. 

//
//  App Folder Finder tree walker callback class
//
class CProgFilesAppFinder : public CAppFolderSize
{
    friend BOOL SlowFindAppFolder(LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszFolder);
public:
    CProgFilesAppFinder(LPCTSTR pszFullName, LPCTSTR pszShortName, BOOL * pfFound, LPTSTR pszFolder);

    // *** IShellTreeWalkerCallBack methods ***
    virtual STDMETHODIMP EnterFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);

    HRESULT SearchInFolder(LPCTSTR pszStart);
    void    SetRootSearch(BOOL bRootSearch);
protected:

    LPCTSTR _pszFullName;
    LPCTSTR _pszShortName;

    // The Result
    LPTSTR  _pszFolder;

    // Best match found
    int _iBest;
    int _iCurDepth;

    // found it or not?
    BOOL * _pfFound;

    // are we searching from root dirs like c:?
    BOOL _fRootSearch;

    // system directory used by the root search
    TCHAR _szSystemDir[MAX_PATH];
}; 

CProgFilesAppFinder::CProgFilesAppFinder(LPCTSTR pszFullName, LPCTSTR pszShortName, BOOL * pfFound, LPTSTR pszFolder) :
   CAppFolderSize(NULL), _pszFullName(pszFullName), _pszShortName(pszShortName), _pfFound(pfFound), _pszFolder(pszFolder)
{
    ASSERT(IS_VALID_STRING_PTR(pszFullName, -1) || IS_VALID_STRING_PTR(pszShortName, -1));
    ASSERT(IS_VALID_STRING_PTR(pszFolder, -1));
    
    ASSERT(pfFound);
    ASSERT(*pfFound == FALSE);
    ASSERT(_fRootSearch == FALSE);
}


void CProgFilesAppFinder::SetRootSearch(BOOL bRootSearch)
{
    _fRootSearch = bRootSearch;
    GetSystemDirectory(_szSystemDir, ARRAYSIZE(_szSystemDir));
}

//
// IShellTreeWalkerCallBack::EnterFolder
//
HRESULT CProgFilesAppFinder::EnterFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    HRESULT hres = S_OK;
    TCHAR szFolder[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRW(pwszFolder, -1));

#ifdef UNICODE  
    lstrcpy(szFolder, pwszFolder);
#else
    WideCharToMultiByte(CP_ACP, 0,
                        pwszFolder, -1,
                        szFolder, ARRAYSIZE(szFolder), NULL, NULL);
#endif

    TraceMsg(TF_SLOWFIND, "Enter Folder: %s -- %s  Depth %d", _pszFullName, szFolder, ptws->nDepth);

    LPTSTR pszName = PathFindFileName(szFolder);

    // Don't go into common files or where we already have seen
    // BUGBUG: These should be in the registry.
    if (_fRootSearch)
    {
        if (!lstrcmpi(pszName, TEXT("Program Files")) || !lstrcmpi(pszName, TEXT("Windows")) ||
            !lstrcmpi(pszName, TEXT("Temp")) || !lstrcmpi(pszName, TEXT("Users")) || StrStrI(pszName, TEXT("WINNT")) ||
            !lstrcmpi(_szSystemDir, szFolder))
            return S_FALSE;
    }
    else if (!lstrcmpi(pszName, TEXT("Common Files")) || !lstrcmpi(pszName, TEXT("Windows NT"))
             || !lstrcmpi(pszName, TEXT("Plus!")) || !lstrcmpi(pszName, TEXT("Uninstall Information")))
        return S_FALSE;

    if (pszName)
    {
        int iMatch = MatchAppName(pszName, _pszFullName, _pszShortName, TRUE);

        // The deeper the match folder is down the tree, the better a match
        // it is.
        if ((iMatch > _iBest) || ((iMatch > 0) && (ptws->nDepth > _iCurDepth)))
        {
            _iBest = iMatch;
            _iCurDepth = ptws->nDepth;
            
            TraceMsg(TF_SLOWFIND, "Slow Match Found: %s -- %s Depth %d", _pszFullName, szFolder, _iCurDepth); 
            ASSERT(IS_VALID_STRING_PTR(_pszFolder, -1));
            lstrcpy(_pszFolder, szFolder);

            if (iMatch == MATCH_LEVEL_HIGH)
            {
                *_pfFound = TRUE;
                hres = E_FAIL;
            }
        }
    }

    if (SUCCEEDED(hres))
        hres = CAppFolderSize::EnterFolder(pwszFolder, ptws, pwfd);

    return hres;
}

//
// Wrapper around WalkTree
//
HRESULT CProgFilesAppFinder::SearchInFolder(LPCTSTR pszStart)
{
    HRESULT hres = E_FAIL;
    WCHAR wszDir[MAX_PATH];
    DWORD dwSearchFlags = WT_MAXDEPTH | WT_NOTIFYFOLDERENTER | WT_FOLDERONLY;

    SHTCharToUnicode(pszStart, wszDir, SIZECHARS(wszDir));

    if (_pstw)
        hres = _pstw->WalkTree(dwSearchFlags, wszDir, NULL, MAX_PROGFILES_SEARCH_DEPTH, SAFECAST(this, IShellTreeWalkerCallBack *));

    return hres;
}

CStartMenuAppFinder::CStartMenuAppFinder(LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszFolder) :
   CAppFolderSize(NULL), _pszFullName(pszFullName), _pszShortName(pszShortName), _pszFolder(pszFolder)
{
    ASSERT(IS_VALID_STRING_PTR(pszFullName, -1) || IS_VALID_STRING_PTR(pszShortName, -1));
    ASSERT(IS_VALID_STRING_PTR(pszFolder, -1));
    
}

STDAPI LoadFromFile(const CLSID *pclsid, LPCWSTR pszFile, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IPersistFile *ppf;
    HRESULT hr = SHCoCreateInstance(NULL, pclsid, NULL, IID_IPersistFile, (void **)&ppf);
    if (SUCCEEDED(hr))
    {
        hr = ppf->Load(pszFile, STGM_READ);
        if (SUCCEEDED(hr))
            hr = ppf->QueryInterface(riid, ppv);
        ppf->Release();
    }
    return hr;
}

//
// get the target of a shortcut. this uses IShellLink which 
// Internet Shortcuts (.URL) and Shell Shortcuts (.LNK) support so
// it should work generally
//
// NOTE: pszPath is WCHAR string
//
HRESULT GetShortcutTarget(LPCWSTR pszLinkPath, LPTSTR pszTargetPath, UINT cchTargetPath)
{
    IShellLink* psl;
    HRESULT hres = E_FAIL;
    HRESULT hresT = LoadFromFile(&CLSID_ShellLink, pszLinkPath, IID_PPV_ARG(IShellLink, &psl));
    if (SUCCEEDED(hresT)) 
    { 
        IShellLinkDataList* psldl;
        hresT = psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psldl));
        if (SUCCEEDED(hresT)) 
        {
            EXP_DARWIN_LINK* pexpDarwin;
            BOOL bDarwin = FALSE;
            hresT = psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);
            if (SUCCEEDED(hresT))
            {
                // This is a darwin link, so we return S_FALSE here. 
                LocalFree(pexpDarwin);
                bDarwin = TRUE;
            }
            
            hresT = psl->GetPath(pszTargetPath, cchTargetPath, NULL, NULL);
            if (hresT == S_OK)
            {
                // Return S_FALSE for the darwin apps. 
                hres = bDarwin ? S_FALSE : hresT;
            }
            
            psldl->Release();
        }
        psl->Release();
    }

    return hres;
}

//
// IShellTreeWalkerCallBack::EnterFolder
//
HRESULT CStartMenuAppFinder::EnterFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    TCHAR szFolder[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRW(pwszFolder, -1));

    SHUnicodeToTChar(pwszFolder, szFolder, SIZECHARS(szFolder));

    LPTSTR pszName = PathFindFileName(szFolder);

    // Skip menus like the "Administrative Tools" and the "Accessories"
    // BUGBUG (scotth): these strings should be resourced-based
    if (FindSubWord(pszName, TEXT("Administrative")) || 
        FindSubWord(pszName, TEXT("Accessories")))
    {
        return  S_FALSE;
    }
    return CAppFolderSize::EnterFolder(pwszFolder, ptws, pwfd);
}


/*-------------------------------------------------------------------------
Purpose: Checks if the given shortcut filename closely matches this
         app's fullname or shortname.  Returns TRUE if it does.
*/
BOOL CStartMenuAppFinder::_MatchSMLinkWithApp(LPCTSTR pszLnkFile)
{
    TCHAR szLnkFile[MAX_PATH];
    
    ASSERT(IS_VALID_STRING_PTR(pszLnkFile, -1));
    
    lstrcpyn(szLnkFile, pszLnkFile, SIZECHARS(szLnkFile));
    LPTSTR pszFileName = PathFindFileName(szLnkFile);
    PathRemoveExtension(pszFileName);
    
    if (MATCH_LEVEL_NORMAL <= MatchAppName(pszFileName, _pszFullName, _pszShortName, FALSE))
        return TRUE;
    
    PathRemoveFileSpec(szLnkFile);
    LPTSTR pszDirName = PathFindFileName(szLnkFile);
    if (MatchAppName(pszFileName, _pszFullName, _pszShortName, FALSE) >= MATCH_LEVEL_NORMAL)
        return TRUE;
    
    return FALSE;
}

//
// IShellTreeWalkerCallBack::FoundFile
//
HRESULT CStartMenuAppFinder::FoundFile(LPCWSTR pwszFile, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    HRESULT hres = S_OK;
    TCHAR szLnkFile[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRW(pwszFile, -1));

    SHUnicodeToTChar(pwszFile, szLnkFile, ARRAYSIZE(szLnkFile));
    TraceMsg(TF_SLOWFIND, "CSMAF:Lnk %s -- %s %s", _pszFullName, szLnkFile);

    if (!_MatchSMLinkWithApp(szLnkFile))
        return S_FALSE;

    
    TCHAR szTargetFile[MAX_PATH];
    HRESULT hresT = GetShortcutTarget(pwszFile, szTargetFile, ARRAYSIZE(szTargetFile));
    if (hresT == S_OK) 
    {
        if(!PathIsRoot(szTargetFile) && !PathIsUnderWindows(szTargetFile) && !PathIsSetup(szTargetFile, 3)
           && !PathIsCommonFiles(szTargetFile))
        {
            TraceMsg(TF_SLOWFIND, "CSMAF:Target %s -- %s %s", _pszFullName, szTargetFile);
            PathRemoveFileSpec(szTargetFile);
            if (!PathIsRoot(szTargetFile))
            {
                int iMatch = FindBestMatch(szTargetFile, _pszFullName, _pszShortName, FALSE, _pszFolder);
                // The deeper the match folder is down the tree, the better a match
                // it is.
                if (iMatch > _iBest)
                {
                    _iBest = iMatch;

                    ASSERT(IS_VALID_STRING_PTR(_pszFolder, -1));
                    ASSERT(PathIsPrefix(_pszFolder, szTargetFile));
                    TraceMsg(TF_SLOWFIND, "CSMAF: Slow Match Found: %s -- %s", _pszFullName, szLnkFile); 

                    if (iMatch == MATCH_LEVEL_HIGH)
                        hres = E_FAIL;
                }
            }
        }
    }
    
    if (SUCCEEDED(hres))
        hres = CAppFolderSize::FoundFile(pwszFile, ptws, pwfd);

    return hres;
}

//
// Wrapper around WalkTree
//
HRESULT CStartMenuAppFinder::SearchInFolder(LPCTSTR pszStart)
{
    HRESULT hres = E_FAIL;
    DWORD dwSearchFlags = WT_MAXDEPTH | WT_NOTIFYFOLDERENTER | WT_FOLDERFIRST;

    if (_pstw)
        hres = _pstw->WalkTree(dwSearchFlags, pszStart, L"*.lnk", MAX_STARTMENU_SEARCH_DEPTH, SAFECAST(this, IShellTreeWalkerCallBack *));

    return hres;
}

//
// NOTE: assuming pszFolder was allocated MAX_PATH long
// pszFolder will contain the result as return
// 
BOOL SlowFindAppFolder(LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszFolder)
{
    ASSERT(IS_VALID_STRING_PTR(pszFolder, -1));
    ASSERT(IS_VALID_STRING_PTR(pszFullName, -1) || IS_VALID_STRING_PTR(pszShortName, -1));

    int iMatch = MATCH_LEVEL_NOMATCH;
    
    // Search from the start menu
    CStartMenuAppFinder * psmaf = new CStartMenuAppFinder(pszFullName, pszShortName, pszFolder);
    if (psmaf)
    {
        if (SUCCEEDED(psmaf->Initialize()))
        {
            TCHAR szStartMenu[MAX_PATH];
            if (SHGetSpecialFolderPath(NULL, szStartMenu, CSIDL_COMMON_STARTMENU, FALSE))
                psmaf->SearchInFolder(szStartMenu);

            if ((psmaf->_iBest == MATCH_LEVEL_NOMATCH) && (SHGetSpecialFolderPath(NULL, szStartMenu, CSIDL_STARTMENU, FALSE)))
                psmaf->SearchInFolder(szStartMenu);

            iMatch = psmaf->_iBest;
        }
        psmaf->Release();
    }

    if (iMatch == MATCH_LEVEL_NOMATCH)
    {
        BOOL fFound = FALSE;

        // Start searching from stratch, no hints on where to start what so ever
        CProgFilesAppFinder * psaff = new CProgFilesAppFinder(pszFullName, pszShortName, &fFound, pszFolder);
        if (psaff)
        {
            if (SUCCEEDED(psaff->Initialize()))
            {
                // search down from "Program Files" directory under root of all fixed drives
                TCHAR szDrive[4];
                TCHAR szProgFiles[30];
                lstrcpy(szDrive, TEXT("C:\\"));
                lstrcpy(szProgFiles, TEXT("C:\\Program Files"));
                for (; !fFound && szDrive[0] <= TEXT('Z'); szProgFiles[0]++, szDrive[0]++)
                {
                    ASSERT(szDrive[0] == szProgFiles[0]);
                    if (GetDriveType(szDrive) == DRIVE_FIXED)
                        psaff->SearchInFolder(szProgFiles);
                }

            }

            if (!fFound)
            {
                psaff->SetRootSearch(TRUE);
                
                TCHAR szDrive[4];
                lstrcpy(szDrive, TEXT("C:\\"));
                for (; !fFound && szDrive[0] <= TEXT('Z'); szDrive[0]++)
                {
                    if (GetDriveType(szDrive) == DRIVE_FIXED)
                        psaff->SearchInFolder(szDrive);
                }
            }

            iMatch = psaff->_iBest;
            psaff->Release();
        }
        
        if (iMatch > MATCH_LEVEL_NOMATCH)
            TraceMsg(TF_ALWAYS, "CPFAF: Found %s at %s", pszFullName, pszFolder);
    }
    else
        TraceMsg(TF_ALWAYS, "CSMAF: Found %s at %s", pszFullName, pszFolder);

    return iMatch;
}

#endif //DOWNLEVEL_PLATFORM
