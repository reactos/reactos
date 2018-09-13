#include "precomp.hxx"
#pragma hdrstop

#include <shguidp.h>    // CLSID_MyDocuments, CLSID_ShellFSFolder
#include <shlobjp.h>    // SHFlushSFCache()
#include "util.h"
#include "dll.h"
#include "resource.h"

// this is where shell32 looks for per user regitem names
TCHAR const c_szDesktopIni[] = TEXT("desktop.ini");

TCHAR const c_szUserShellFolders[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
TCHAR const c_szPersonal[] = TEXT("Personal");
TCHAR const c_szMyPics[] = TEXT("My Pictures");

// Determine if we're running under Winstone 97 (at least whether we're in
// the state where Winstone messes up CSIDL_PERSONAL)

BOOL IsWinstone97()
{
    BOOL fRet = FALSE;
    HKEY hkey;

    // Winstone97 sets CSIDL_PERSONAL in HKEY_LOCAL_MACHINE to be 
    // C:\wstemp\mydocs. if that is there we are running under winstone

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, c_szUserShellFolders, &hkey))
    {
        TCHAR szPath[MAX_PATH];
        DWORD cbSize = SIZEOF(szPath);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szPersonal, NULL, NULL, (LPBYTE)szPath, &cbSize))
        {
            if (cbSize > (3 * sizeof(TCHAR)))
            {
                // szPath+3 is to skip the drive letter & :\...

                fRet = StrCmpI( &szPath[3], TEXT("wstemp\\mydocs")) == 0;
            }
        }
        RegCloseKey(hkey);
    }
    return fRet;
}

// check the enumerated bit in the name space to see if this is visible to the user

BOOL IsMyDocsHidden(void)
{
    BOOL bHidden = TRUE;
    IShellFolder *psf;
    if (SUCCEEDED(SHGetDesktopFolder(&psf)))
    {
        LPITEMIDLIST pidl;
        DWORD dwAttrib = SFGAO_NONENUMERATED;
        WCHAR wszName[128];

        SHTCharToUnicode(TEXT("::") MYDOCS_CLSID, wszName, ARRAYSIZE(wszName));

        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, wszName, NULL, &pidl, &dwAttrib)))
        {
            bHidden = dwAttrib & SFGAO_NONENUMERATED;
            ILFree(pidl);
        }
        psf->Release();
    }
    return bHidden;
}


// Return the current display name for "My Documents"

HRESULT GetMyDocumentsDisplayName(LPTSTR pszPath, UINT cch)
{
    *pszPath = 0;

    IShellFolder *psf;
    if (SUCCEEDED(SHGetDesktopFolder(&psf)))
    {
        LPITEMIDLIST pidl;
        WCHAR wszName[128];

        SHTCharToUnicode(TEXT("::") MYDOCS_CLSID, wszName, ARRAYSIZE(wszName));

        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, wszName, NULL, &pidl, NULL)))
        {
            STRRET sr;
            if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_NORMAL, &sr)))
            {
                StrRetToBuf(&sr, pidl, pszPath, cch);
            }
            ILFree(pidl);
        }
        psf->Release();
    }
    return *pszPath ? S_OK : E_FAIL;
}

LPITEMIDLIST MyDocsIDList(void)
{
    LPITEMIDLIST pidl = NULL;
    IShellFolder *psf;
    if (SUCCEEDED(SHGetDesktopFolder(&psf)))
    {
        WCHAR wszName[128];
        SHTCharToUnicode(TEXT("::") MYDOCS_CLSID, wszName, ARRAYSIZE(wszName));

        psf->ParseDisplayName(NULL, NULL, wszName, NULL, &pidl, NULL);
        psf->Release();
    }
    return pidl;
}


void SHChangeNotifyMyDocs(UINT nNotify)
{
    LPITEMIDLIST pidlMyDocs = MyDocsIDList();
    if (pidlMyDocs)
    {
        SHChangeNotify(nNotify, SHCNF_IDLIST, pidlMyDocs, NULL);
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlMyDocs, NULL);
        ILFree(pidlMyDocs);
    }
}

DWORD MyDocsGetAttributes()
{
    DWORD dwAttributes = SFGAO_CANLINK |            // 00000004
                         SFGAO_CANRENAME |          // 00000010
                         SFGAO_CANDELETE |          // 00000020
                         SFGAO_HASPROPSHEET |       // 00000040
                         SFGAO_DROPTARGET |         // 00000100
                         SFGAO_FILESYSANCESTOR |    // 10000000
                         SFGAO_FOLDER |             // 20000000
                         SFGAO_FILESYSTEM |         // 40000000
                         SFGAO_HASSUBFOLDER |       // 80000000
                         SFGAO_CANMONIKER;          // 00400000
                         // SFGAO_NONENUMERATED     // 00100000
                         //                         // F0400174
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\") MYDOCS_CLSID TEXT("\\ShellFolder"), &hkey))
    {
        DWORD dwSize = sizeof(dwAttributes);
        RegQueryValueEx(hkey, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwAttributes, &dwSize);
        RegCloseKey(hkey);
    }
    return dwAttributes;
}

HRESULT MyDocsSetAttributes(DWORD dwMask, DWORD dwNewBits)
{
    HKEY hkey;
    HRESULT hr = E_FAIL;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CSLID\\") MYDOCS_CLSID TEXT("\\ShellFolder"), &hkey))
    {
        DWORD dwValue, cbSize = SIZEOF(dwValue);
        if (RegQueryValueEx(hkey, TEXT("Attributes"), NULL, NULL, (BYTE *)&dwValue, &cbSize) == ERROR_SUCCESS)
        {
            dwValue = (dwValue & ~dwMask) | (dwNewBits & dwMask);

            if (RegSetValueEx(hkey, TEXT("Attributes"), 0, REG_DWORD, (BYTE *)&dwValue, SIZEOF(dwValue)) == ERROR_SUCCESS)
            {
                hr = S_OK;
            }
        }
        RegCloseKey(hkey);
    }
    return hr;
}

void HideMyDocs()
{
    MyDocsSetAttributes(SFGAO_NONENUMERATED, 0);
}

void UnHideMyDocs()
{
    MyDocsSetAttributes(SFGAO_NONENUMERATED, SFGAO_NONENUMERATED);
}


// Resets special registry value so that My Docs will be visible...
void RestoreMyDocsFolder(void)
{
    // delete the HideMyDocs value
    UnHideMyDocs();

    // Add item to sendto directory
    UpdateSendToFile(FALSE);

    // Tell the shell we're back...
    SHChangeNotifyMyDocs(SHCNE_CREATE);
}

// Create/Updates file in SendTo directory to have current display name

void UpdateSendToFile(BOOL bDeleteOnly)
{
    TCHAR szSendToDir[MAX_PATH];
    
    if (S_OK == SHGetFolderPath(NULL, CSIDL_SENDTO, NULL, SHGFP_TYPE_CURRENT, szSendToDir))
    {
        // Create c:\winnt\profile\chrisg\sendto\<dislpay name>.mydocs
        TCHAR szNewFile[MAX_PATH];
        if (!bDeleteOnly)
        {
            TCHAR szName[MAX_PATH];
            if (SUCCEEDED(GetMyDocumentsDisplayName(szName, ARRAYSIZE(szName))))
            {
                PathCombine(szNewFile, szSendToDir, szName);
                lstrcat(szNewFile, TEXT(".mydocs"));
            }
            else
            {
                // we can't create a new file, because we don't have a name
                bDeleteOnly = TRUE;
            }
        }
        
        TCHAR szFile[MAX_PATH];
        WIN32_FIND_DATA fd;

        // delete c:\winnt\profile\chrisg\sendto\*.mydocs

        PathCombine(szFile, szSendToDir, TEXT("*.mydocs"));

        HANDLE hFind = FindFirstFile(szFile, &fd);
        if (hFind != INVALID_HANDLE_VALUE )
        {
            do
            {
                PathCombine(szFile, szSendToDir, fd.cFileName);
                if (0 == lstrcmp(szFile, szNewFile))
                {
                    // The file that we needed to create already exists,
                    // just leave it in place instead of deleting it and
                    // then creating it again below (this fixes
                    // app compat problems - see NT bug 246932)
                    bDeleteOnly = TRUE;
                }
                else
                {
                    DeleteFile(szFile);
                }
            } while (FindNextFile(hFind, &fd));
            FindClose(hFind);
        }

        if (!bDeleteOnly)
        {
            hFind = CreateFile(szNewFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFind);
            }
        }
    }
}


// test pszChild against pszParent to see if
// pszChild is equal (PATH_IS_EQUAL) or 
// a DIRECT child (PATH_IS_CHILD)

DWORD ComparePaths(LPCTSTR pszChild, LPCTSTR pszParent)
{
    DWORD dwRet = PATH_IS_DIFFERENT;
    INT cchParent = lstrlen( pszParent );
    INT cchChild = lstrlen( pszChild );

    if (cchParent <= cchChild)
    {
        TCHAR szChild[MAX_PATH];

        lstrcpyn(szChild, pszChild, ARRAYSIZE(szChild));

        LPTSTR pszChildSlice = szChild + cchParent;
        TCHAR cSave = *pszChildSlice;

        *pszChildSlice = 0;

        if (lstrcmpi( szChild, pszParent ) == 0 )
        {
            if (cchChild > cchParent)
            {
                LPTSTR pTmp = pszChildSlice + 1;

                while( *pTmp && *pTmp!=TEXT('\\') )
                {
                    pTmp++;
                }

                if (!(*pTmp))
                {
                    dwRet = PATH_IS_CHILD;
                }

            }
            else
            {
                dwRet = PATH_IS_EQUAL;
            }
        }

        *pszChildSlice = cSave;
    }

    return dwRet;
}

TCHAR const c_szShellInfo[]= TEXT(".ShellClassInfo");

// Checks the path to see if it is marked as system or read only and
// then check desktop.ini for CLSID or CLSID2 entry...

BOOL IsPathAlreadyShellFolder(LPCTSTR pPath, DWORD dwAttrib)
{
    TCHAR szDesktopIni[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];

    if (!PathIsSystemFolder(pPath, dwAttrib))
        return FALSE;

    PathCombine(szDesktopIni, pPath, c_szDesktopIni);

    // Check for CLSID entry...
    GetPrivateProfileString( c_szShellInfo, TEXT("CLSID"), TEXT("foo"), szBuffer, ARRAYSIZE(szBuffer), szDesktopIni );

    if ( (lstrcmpi( szBuffer, TEXT("foo") ) !=0 ) &&
         (lstrcmpi( szBuffer, MYDOCS_CLSID ) !=0 ) )
    {
        return TRUE;
    }

    // Check for CLSID2 entry...
    GetPrivateProfileString( c_szShellInfo, TEXT("CLSID2"), TEXT("foo"), szBuffer, ARRAYSIZE(szBuffer), szDesktopIni );

    if ( (lstrcmpi( szBuffer, TEXT("foo") ) != 0) &&
         (lstrcmpi( szBuffer, MYDOCS_CLSID ) != 0) )
    {
        return TRUE;
    }

    return FALSE;
}

const struct
{
    DWORD dwDir;
    DWORD dwFlags;
    DWORD dwRet;
}
_adirs[] =
{
    { CSIDL_DESKTOP,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_DESKTOP   },
    { CSIDL_PERSONAL,           PATH_IS_EQUAL                , PATH_IS_MYDOCS    },
    { CSIDL_SENDTO,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_SENDTO    },
    { CSIDL_RECENT,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_RECENT    },
    { CSIDL_HISTORY,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_HISTORY   },
    { CSIDL_COOKIES,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_COOKIES   },
    { CSIDL_PRINTHOOD,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_PRINTHOOD },
    { CSIDL_NETHOOD,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_NETHOOD   },
    { CSIDL_STARTMENU,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_STARTMENU },
    { CSIDL_TEMPLATES,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_TEMPLATES },
    { CSIDL_FAVORITES,          PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_FAVORITES },
    { CSIDL_FONTS,              PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_FONTS     },
    { CSIDL_APPDATA,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_APPDATA   },
    { CSIDL_INTERNET_CACHE,     PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_TEMP_INET },
    { CSIDL_COMMON_STARTMENU,   PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_STARTMENU },
    { CSIDL_COMMON_DESKTOPDIRECTORY, PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_DESKTOP },
    { CSIDL_WINDOWS,            PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_WINDOWS },
    { CSIDL_SYSTEM,             PATH_IS_EQUAL | PATH_IS_CHILD, PATH_IS_SYSTEM },
    { CSIDL_PROFILE,            PATH_IS_EQUAL                , PATH_IS_PROFILE },
};

#ifdef DEBUG
BOOL PathHasBackslash(LPCTSTR pszPath)
{
    BOOL fRet = FALSE;

    TCHAR* pszTest = StrDup(pszPath);
    if (pszTest)
    {
        PathRemoveBackslash(pszTest);
        fRet = (lstrcmpi(pszTest, pszPath) != 0);
        LocalFree(pszTest);
    }

    return fRet;
}
#endif

#ifdef UNICODE
#define CharN(psz, n)  ((psz)[n])
#else
TCHAR CharN(LPCTSTR psz, int n)
{
    ASSERT(n < lstrlen(psz));
    while (n--)
    {
        psz = CharNext(psz);
    }

    return *psz;
}
#endif

BOOL PathEndsInDot(LPCTSTR pszPath)
{
    //
    // CreateDirectory("c:\foo.") or CreateDirectory("c:\foo.....")
    // will succeed but create a directory named "c:\foo", which isn't
    // what the user asked for.  So we use this function to guard
    // against those cases.
    //
    // Note that this simple test also picks off "c:\foo\." -- ok for
    // our purposes.
    //

    ASSERT(!PathHasBackslash(pszPath));

    UINT cLen = lstrlen(pszPath);
    if (cLen >= 1)
    {
        if (CharN(pszPath, cLen - 1) == TEXT('.'))
        {
            return TRUE;
        }
    }

    return FALSE;
}

//
// Checks the path to see if it is okay as a MyDocs path
//
DWORD IsPathGoodMyDocsPath( HWND hwnd, LPCTSTR pPath )
{
    if (NULL == pPath)
    {
        return PATH_IS_ERROR;
    }
    
    TCHAR szRootPath[MAX_PATH];
    lstrcpyn(szRootPath, pPath, ARRAYSIZE(szRootPath));
    if (!PathStripToRoot(szRootPath))
    {
        return PATH_IS_ERROR;
    }

    if (PathEndsInDot(pPath))
    {
        return PATH_IS_ERROR;
    }
    
    DWORD dwRes, dwAttr = GetFileAttributes( pPath );
    if (dwAttr == 0xFFFFFFFF)
    {
        if (0xFFFFFFFF == GetFileAttributes(szRootPath))
        {
            // If the root path doesn't exist, then we're not going
            // to be able to create a path:
            return PATH_IS_ERROR;
        }
        else
        {
            return PATH_IS_NONEXISTENT;
        }
    }

    if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        return PATH_IS_NONDIR;
    }

    for (int i = 0; i < ARRAYSIZE(_adirs); i++)
    {
        TCHAR szPathToCheck[MAX_PATH];
        //
        // Check for various special shell folders
        //
        if (S_OK == SHGetFolderPath(hwnd, _adirs[i].dwDir | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPathToCheck))
        {
            dwRes = ComparePaths(pPath, szPathToCheck);

            if (dwRes & _adirs[i].dwFlags)
            {
                //
                // The inevitable exceptions
                //
                switch(_adirs[i].dwDir) 
                {
                case CSIDL_DESKTOP:
                    if (PATH_IS_CHILD == dwRes) 
                    {
                        continue;   // allowing subfolder of CSIDL_DESKTOP
                    }
                    break;

                default:
                    break;
                } // switch

                return _adirs[i].dwRet;
            }
        }
    }
    
    //
    // Make sure path isn't set as a system or some other kind of
    // folder that already has a CLSID or CLSID2 entry...
    //
    if (IsPathAlreadyShellFolder(pPath, dwAttr))
    {
        return PATH_IS_SHELLFOLDER;
    }

    return PATH_IS_GOOD;
}

// Scans a desktop.ini file for sections to see if all of them are empty...

BOOL IsDesktopIniEmpty(LPCTSTR pIniFile)
{
    TCHAR szSections[1024];  // for section names
    if (GetPrivateProfileSectionNames(szSections, ARRAYSIZE(szSections), pIniFile))
    {
        for (LPTSTR pTmp = szSections; *pTmp; pTmp += lstrlen(pTmp) + 1)
        {
            TCHAR szSection[1024];   // for section key names and values
            GetPrivateProfileSection(pTmp, szSection, ARRAYSIZE(szSection), pIniFile);
            if (szSection[0])
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Remove our entries from the desktop.ini file in this directory, and
// then test the desktop.ini to see if it's empty.  If it is, delete it
// and remove the system/readonly bit from the directory...

void MyDocsUnmakeSystemFolder( LPCTSTR pPath )
{
    TCHAR szIniFile[MAX_PATH];

    PathCombine( szIniFile, pPath, c_szDesktopIni );

    // Remove CLSID2
    WritePrivateProfileString(c_szShellInfo, TEXT("CLSID2"), NULL, szIniFile);

    // Remove InfoTip
    WritePrivateProfileString(c_szShellInfo, TEXT("InfoTip"), NULL, szIniFile);

    // Remove Icon
    WritePrivateProfileString( c_szShellInfo, TEXT("IconFile"), NULL, szIniFile);

    DWORD dwAttrb = GetFileAttributes( szIniFile );
    if (dwAttrb != 0xFFFFFFFF)
    {
        if (IsDesktopIniEmpty(szIniFile))
        {
            dwAttrb &= ~( FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN );
            SetFileAttributes( szIniFile, dwAttrb );
            DeleteFile( szIniFile );
        }
        PathUnmakeSystemFolder( pPath );
    }
}

// Delete the desktop.ini (if it exists)
// and remove the system/readonly bit from the directory...

void MyPicsUnmakeSystemFolder(LPCTSTR pPath)
{
    // nuke old mypics' desktop.ini and fix attribs on old mypics dir
    TCHAR szIniFile[MAX_PATH];
    PathCombine(szIniFile, pPath, c_szDesktopIni);

    DWORD dwAttrb = GetFileAttributes(szIniFile);
    if (dwAttrb != 0xFFFFFFFF)
    {
        dwAttrb &= ~(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN);
        SetFileAttributes(szIniFile, dwAttrb);
        DeleteFile(szIniFile);
        PathUnmakeSystemFolder(pPath);
    }
}


void SetFolderPath(LPCTSTR pszFolder, LPCTSTR pszPath)
{
    HKEY hkey;
    if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, c_szUserShellFolders, &hkey))
    {
        TCHAR szPath[MAX_PATH];
        if (pszPath)
        {
#ifdef WINNT
            if (!PathUnExpandEnvStrings(pszPath, szPath, ARRAYSIZE(szPath)))
                lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
            RegSetValueEx(hkey, pszFolder, 0, REG_EXPAND_SZ, (BYTE *)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
#else
            lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
            RegSetValueEx(hkey, pszFolder, 0, REG_SZ, (BYTE *)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
#endif
        }
        else
            RegDeleteValue(hkey, pszFolder);

        RegCloseKey(hkey);
        
        SHFlushSFCache();   // invalidate the shells folder cached to keep them in sync
    }
}

// Change CSIDL_PERSONAL to the specified path.  If pszOld is NULL we
// assume we're creating CSIDL_PERSONAL from setup, and can therefore
// whack the registry directly.

HRESULT ChangePersonalPath(LPCTSTR pszNew, LPCTSTR pszOld, BOOL* pfResetMyPics)
{
    HRESULT hr = S_OK;
    BOOL fResetMyPics = TRUE;
    BOOL fSetReadOnly = FALSE;
    LPITEMIDLIST pidlOld = NULL, pidlNew = NULL;
    TCHAR szNewPath[MAX_PATH], szOldPath[MAX_PATH];

    szNewPath[0] = 0; 
    szOldPath[0] = 0;

    if (pszNew)
        ExpandEnvironmentStrings(pszNew, szNewPath, ARRAYSIZE(szNewPath));

    if (szNewPath[0] == 0)
        return E_INVALIDARG;

    if (pszOld)
        ExpandEnvironmentStrings(pszOld, szOldPath, ARRAYSIZE(szOldPath));

    if (szOldPath[0])
    {
        TCHAR szMyPics[MAX_PATH];
        if (S_OK == SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szMyPics))
        {
            fResetMyPics = (PATH_IS_CHILD == ComparePaths(szMyPics, szOldPath));
        }

        // Check if old target folder is read only.
        DWORD dwAtt = GetFileAttributes(szOldPath);
        if (0xFFFFFFFF != dwAtt)
        {
            fSetReadOnly = (FILE_ATTRIBUTE_READONLY == (dwAtt & FILE_ATTRIBUTE_READONLY));
        }

        // pidlOld may be NULL if the szOldPath does not exist
        pidlOld = ILCreateFromPath(szOldPath);

        // Clean up old attributes and desktop.ini, if possible...
        MyDocsUnmakeSystemFolder(szOldPath);
    }

    pidlNew = ILCreateFromPath(szNewPath);
    if (pidlNew)
    {
        // Restore read only settings from old target. Needed to ensure customized
        // WebView settings are rendered (348749)
        if (fSetReadOnly)
        {
            SetFileAttributes(szNewPath, FILE_ATTRIBUTE_READONLY);
        }
        
        SetFolderPath(c_szPersonal, szNewPath);
        SHChangeNotify( SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlNew, NULL );
        ILFree( pidlNew );
    }
    else
        hr = E_FAIL;

    if (pidlOld)
    {
        SHChangeNotify( SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlOld, NULL );
        ILFree( pidlOld );
    }

    if (pfResetMyPics)
        *pfResetMyPics = fResetMyPics;

    return hr;
}

void ChangeMyPicsPath( LPCTSTR pszNewPath, LPCTSTR pszOldPath )
{
    ASSERT( pszOldPath );
    if( *pszOldPath )
    {
        MyPicsUnmakeSystemFolder( pszOldPath );
    }

    SetFolderPath( c_szMyPics, pszNewPath );
    InstallMyPictures();
}

void ResetMyPics(LPCTSTR pszOldMyPics)
{
    TCHAR   szPath[MAX_PATH];

    ChangeMyPicsPath( NULL, pszOldMyPics );

    // 99/05/24 #341098 vtan: When the call to SetFolderPath() in
    // InstallMyPictures() was removed for #316476 that introduced
    // this bug. Put back the functionality but make it specific
    // to ResetMyPics() which deliberately removes the entry so
    // that the default install case doesn't mess with the USF
    // registry value.

    if (S_OK == SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, szPath))
    {
        SetFolderPath(c_szMyPics, szPath);
    }
}


void InstallMyPictures()
{
    TCHAR szPath[MAX_PATH];
    // Create the My Pictures folder
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, szPath);
    if (hr == S_OK)
    {
        // Set the default custom settings for the folder.
        SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_VIEWID | FCSM_WEBVIEWTEMPLATE | FCSM_ICONFILE | FCSM_INFOTIP, 0};

        SHELLVIEWID vid = CLSID_ThumbnailViewExt;
        fcs.pvid = &vid;

        TCHAR szWebViewTemplate[MAX_PATH];
        GetWindowsDirectory(szWebViewTemplate, ARRAYSIZE(szWebViewTemplate));
        PathAppend(szWebViewTemplate, TEXT("Web\\ImgView.htt"));
        fcs.pszWebViewTemplate = szWebViewTemplate;   // template path
        fcs.pszWebViewTemplateVersion = TEXT("NT5");   // template version
        
        TCHAR szInfoTip[128];
        LoadString(g_hInstance, IDS_MYPICS_INFOTIP, szInfoTip, ARRAYSIZE(szInfoTip) );
        fcs.pszInfoTip = szInfoTip;

        // Get the IconFile and IconIndex for the Recent Files Folder
        // BUGBUG: Ideally, we could just write "mydocs.dll" or
        // "%SystemRoot%\system32\mydocs.dll", but neither work,
        // so we write the full path here:
        TCHAR szIconFile[MAX_PATH];
        GetSystemDirectory(szIconFile, ARRAYSIZE(szIconFile));
        PathAppend(szIconFile, TEXT("mydocs.dll"));
        fcs.pszIconFile = szIconFile;
        fcs.iIconIndex = -IDI_MYPICS;

        // NOTE: we need FCS_FORCEWRITE because we didn't used to specify iIconIndex
        // and so "0" was written to the ini file.  When we upgrade, this API won't
        // fix the ini file unless we pass FCS_FORCEWRITE
        SHGetSetFolderCustomSettings(&fcs, szPath, FCS_FORCEWRITE);
    }
}

#define _ILSkip(pidl, cb)       ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)           _ILSkip(pidl, (pidl)->mkid.cb)

BOOL IsMyDocsIDList(LPCITEMIDLIST pidl)
{
    if (pidl && !ILIsEmpty(pidl) && ILIsEmpty(_ILNext(pidl)))
    {
        BOOL bIsMyDocs = FALSE;
        IShellFolder *psf;
        if (SUCCEEDED(SHGetDesktopFolder(&psf)))
        {
            LPITEMIDLIST pidlMyDocs = MyDocsIDList();
            if (pidlMyDocs)
            {
                bIsMyDocs = psf->CompareIDs(0, pidl, pidlMyDocs) == 0;
                ILFree(pidlMyDocs);
            }
            psf->Release();
        }
        return bIsMyDocs;
    }
    return FALSE;
}

