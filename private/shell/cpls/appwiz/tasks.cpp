//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: tasks.cpp
// App Management tasks running on the secondary thread
// 
// History:
//         2-26-98  by dli implemented CAppUemInfoTask
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "shguidp.h"
#include "uemapp.h"
#include "appsize.h"
#include "findapp.h"
#include "tasks.h"
#include "slowfind.h"
#include "dump.h"
#include "util.h"


// Utility function to get times used or last used time for "exe" files
void ExtractExeInfo(LPCTSTR pszExe, PSLOWAPPINFO psai, BOOL bNoImageChange)
{
    ASSERT(IS_VALID_STRING_PTR(pszExe, -1));
    
    // Got to have a legal psai
    ASSERT(psai);

    // Get the "times used" info
    UEMINFO uei = {0};
    uei.cbSize = SIZEOF(uei);
    uei.dwMask = UEIM_HIT;
    if (SUCCEEDED(UEMQueryEvent(&UEMIID_SHELL, UEME_RUNPATH, (WPARAM)-1, (LPARAM)pszExe, &uei)))
    {
        if (uei.cHit > psai->iTimesUsed)
            psai->iTimesUsed = uei.cHit;
    }

    // Get the most recent access time
    HANDLE hFile = CreateFile(pszExe, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, 0, NULL ); 
    if( INVALID_HANDLE_VALUE != hFile )
    {
        FILETIME ftCreate, ftAccessed, ftWrite;
        if (GetFileTime(hFile, &ftCreate, &ftAccessed, &ftWrite))
        {
            // Is the creation and accessed dates identical, and is the
            // UEM's statistic useless?
            if (0 == CompareFileTime(&ftAccessed, &ftCreate) && 
                0 == psai->ftLastUsed.dwHighDateTime)
            {
                // Yes; then it doesn't look like anyone has used it
                psai->ftLastUsed.dwHighDateTime = NOTUSED_HIGHDATETIME;
                psai->ftLastUsed.dwLowDateTime = NOTUSED_LOWDATETIME;
                if (!bNoImageChange && (psai->pszImage == NULL))
                    SHStrDup(pszExe, &psai->pszImage);
            }
            else if (CompareFileTime(&ftAccessed, &psai->ftLastUsed) > 0)
            {
                // No; someone must have used this program
                psai->ftLastUsed = ftAccessed;

                if (!bNoImageChange)
                {
                    // If there was an exe file for the icon, release that
                    if (psai->pszImage)
                        SHFree(psai->pszImage);

                    // Set the icon image of this app to this exe's icon
                    // because this exe is the most recently used one.

                    SHStrDup(pszExe, &psai->pszImage);
                }
            }
        }

        CloseHandle(hFile);
    }
}

const static struct {
    LPTSTR szAppName;
    LPTSTR szExeName;
} s_rgAppHacks[] = {
    { TEXT("Microsoft Office"), TEXT("msoffice.exe")},
};

//--------------------------------------------------------------------------------
//  CAppInfoFinder class
//--------------------------------------------------------------------------------

static const WCHAR sc_wszStarDotExe[] = L"*.exe";

// Use the TreeWalker to find the application "exe" file
class CAppInfoFinder : public CAppFolderSize
{
public:
    CAppInfoFinder(PSLOWAPPINFO psai, BOOL fSize, LPCTSTR pszHintExe, BOOL fNoImageChange);
    
    // *** IShellTreeWalkerCallBack methods (override) ***
    STDMETHOD(FoundFile)    (LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);

    HRESULT SearchInFolder(LPCTSTR pszFolder);

protected:
    PSLOWAPPINFO _psai;
    BOOL _fComputeSize;         // Compute size or not
    BOOL _fNoImageChange;       // Do not change image from now on.
    TCHAR _szHintExe[MAX_PATH];
}; 


// constructor
CAppInfoFinder::CAppInfoFinder(PSLOWAPPINFO psai, BOOL fSize, LPCTSTR pszHintExe, BOOL fNoImageChange) :
   _fComputeSize(fSize), _fNoImageChange(fNoImageChange), _psai(psai), CAppFolderSize(&psai->ullSize)
{
    if (pszHintExe)
        lstrcpyn(_szHintExe, pszHintExe, ARRAYSIZE(_szHintExe));
}


/*-------------------------------------------------------------------------
Purpose: IShellTreeWalkerCallBack::FoundFile

         Extracts the exe info that we want if the given file matches
         an exe spec.  The info is stored in the _psai member variable.
*/
HRESULT CAppInfoFinder::FoundFile(LPCWSTR pwszFile, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfdw)
{
    HRESULT hres = S_OK;

    ASSERT(IS_VALID_STRING_PTRW(pwszFile, -1));

    if (PathMatchSpecW(pwfdw->cFileName, sc_wszStarDotExe))
    {
        TCHAR szPath[MAX_PATH];
        TraceMsg(TF_SLOWFIND, "Found Exe File: %s", pwszFile);
        
        SHUnicodeToTChar(pwszFile, szPath, ARRAYSIZE(szPath));

        if (!PathIsSetup(szPath, 2))
        {
            ExtractExeInfo(szPath, _psai, _fNoImageChange);

            if (_szHintExe[0] != TEXT('\0'))
            {
                // Does this exe match our app's Hint icon exe?
                if (!lstrcmpi(_szHintExe, PathFindFileName(szPath)))
                {
                    // Yes, Bingo!! Use this icon. 

                    // If there was an exe file for the icon, release that
                    if (_psai->pszImage)
                        SHFree(_psai->pszImage);

                    // Set the icon image of this app to this exe's icon
                    SHStrDup(szPath, &_psai->pszImage);

                    _fNoImageChange = TRUE;
                }
            }
        }
    }

    if (_fComputeSize)
        hres = CAppFolderSize::FoundFile(pwszFile, ptws, pwfdw);
        
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: Method to kick off the tree walk, starting at pszFolder.
*/
HRESULT CAppInfoFinder::SearchInFolder(LPCTSTR pszFolder)
{
    HRESULT hres = E_FAIL;
    WCHAR wszDir[MAX_PATH];

    SHTCharToUnicode(pszFolder, wszDir, SIZECHARS(wszDir));

    if (_pstw)
        hres = _pstw->WalkTree(0, wszDir, NULL, 0, SAFECAST(this, IShellTreeWalkerCallBack *));

    return hres;
}


//--------------------------------------------------------------------------------
//  CAppInfoFinderSM class
//--------------------------------------------------------------------------------


// Use the TreeWalker to find the application "exe" file
class CAppInfoFinderSM : public CStartMenuAppFinder
{
public:
    CAppInfoFinderSM(LPCTSTR pszFullName, LPCTSTR pszShortName, PSLOWAPPINFO psai);
    
    // *** IShellTreeWalkerCallBack methods ***
    virtual STDMETHODIMP FoundFile(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);

protected:
    PSLOWAPPINFO _psai;
    TCHAR _szFakeFolder[MAX_PATH];
}; 


// constructor
CAppInfoFinderSM::CAppInfoFinderSM(LPCTSTR pszFullName, LPCTSTR pszShortName, PSLOWAPPINFO psai) : 
    _psai(psai),CStartMenuAppFinder(pszFullName, pszShortName, _szFakeFolder)
{
}


/*-------------------------------------------------------------------------
Purpose: IShellTreeWalkerCallBack::FoundFile

         Extracts the exe info that we want if the given file matches
         an exe spec.  The info is stored in the _psai member variable.
*/
HRESULT CAppInfoFinderSM::FoundFile(LPCWSTR pwszFile, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    TCHAR szLnkFile[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRW(pwszFile, -1));

    SHUnicodeToTChar(pwszFile, szLnkFile, ARRAYSIZE(szLnkFile));
    TraceMsg(TF_SLOWFIND, "CSMAF:Lnk %s -- %s %s", _pszFullName, szLnkFile);

    if (!_MatchSMLinkWithApp(szLnkFile))
        return S_OK;

    TCHAR szTargetFile[MAX_PATH];
    HRESULT hresT = GetShortcutTarget(szLnkFile, szTargetFile, ARRAYSIZE(szTargetFile));
    if ((S_FALSE == hresT) || ((S_OK == hresT) && !PathIsRoot(szTargetFile) && !PathIsUnderWindows(szTargetFile) && !PathIsSetup(szTargetFile, 1)))
    {
        LPCTSTR pszName = PathFindFileName(szTargetFile);
        
        if (PathMatchSpec(pszName, sc_wszStarDotExe))
            ExtractExeInfo(szTargetFile, _psai, FALSE);
    }

    return S_OK;
}


LPTSTR LookUpHintExes(LPCTSTR pszAppName, LPTSTR pszHintExe, DWORD cbHintExe)
{
    // Open the reg key
    HKEY hkeyIconHints = NULL;
    LPTSTR pszRet = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Management\\Icon Hints")
                                      , 0, KEY_READ, &hkeyIconHints))
    {
        DWORD dwType;
        // Look up in the registry for this cpl name
        if ((ERROR_SUCCESS == SHQueryValueEx(hkeyIconHints, pszAppName, NULL, &dwType, pszHintExe, &cbHintExe))
            && (dwType == REG_SZ))
        {
            pszRet = pszHintExe;
        }

        RegCloseKey(hkeyIconHints);
    }

    return pszRet;
}

// use the tree walker to find the "exe" file for the application
HRESULT FindAppInfo(LPCTSTR pszFolder, LPCTSTR pszFullName, LPCTSTR pszShortName, PSLOWAPPINFO psai, BOOL bChanged)
{
    // If there is no output string, a folder and a name, we can't do anything
    ASSERT(IS_VALID_WRITE_PTR(psai, SLOWAPPINFO));
    if (pszFolder)
    {
        // We only compute sizes for locally installed apps and apps installed
        // on fixed drives. Ex: On board or external hard drives.
        // We purposely not compute size for network apps, apps on the CD ROMs and so on

        BOOL bGetSize = bChanged && PathIsLocalAndFixed(pszFolder);
        
        TCHAR szHintExe[MAX_PATH];
        LPTSTR pszHintExe = LookUpHintExes(pszFullName, szHintExe, SIZEOF(szHintExe));
        CAppInfoFinder * paef = new CAppInfoFinder(psai, bGetSize, pszHintExe, !bChanged);
        if (paef)
        {
            if (SUCCEEDED(paef->Initialize()))
                paef->SearchInFolder(pszFolder);
            
            paef->Release();
        }
    }

    if (bChanged)
    {
        CAppInfoFinderSM * paifsm = new CAppInfoFinderSM(pszFullName, pszShortName, psai);
        if (paifsm)
        {
            if (SUCCEEDED(paifsm->Initialize()))
            {
                TCHAR szStartMenu[MAX_PATH];
                if (SHGetSpecialFolderPath(NULL, szStartMenu, CSIDL_COMMON_PROGRAMS, FALSE))
                    paifsm->SearchInFolder(szStartMenu);

                if (SHGetSpecialFolderPath(NULL, szStartMenu, CSIDL_PROGRAMS, FALSE))
                    paifsm->SearchInFolder(szStartMenu);
            }            
            paifsm->Release();
        }
    }
    
    return S_OK;
}

#endif //DOWNLEVEL_PLATFORM
