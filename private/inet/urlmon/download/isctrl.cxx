#include <cdlpch.h>
#pragma hdrstop

#include "verp.h"
#include <mstask.h>
#include <pkgguid.h>

extern LCID g_lcidBrowser;     // default to english
extern char g_szBrowserPrimaryLang[];
// global that encapsulates delay-loaded version.dll
CVersion g_versiondll;
extern BOOL g_bRunOnWin95;

HRESULT NeedForceLanguageCheck(HKEY hkeyCLSID, CLocalComponentInfo *plci);
BOOL    SniffStringFileInfo( LPSTR szFileName, LPCTSTR lpszSubblock, DWORD *pdw = NULL );
HRESULT IsDistUnitLocallyInstalledZI(LPCWSTR , LPCWSTR, DWORD , DWORD , CLocalComponentInfo * );
HRESULT IsDistUnitLocallyInstalledSxS(LPCWSTR wszDistUnit, LPCWSTR wszClsid, DWORD dwFileVersionMS, DWORD dwFileVersionLS, CLocalComponentInfo * plci);
void ExtractVersion(char *pszDistUnit, DWORD *pdwVerMS, DWORD *pdwVerLS);
void GetLatestZIVersion(const WCHAR *pwzDistUnit, DWORD *pdwVerMS, DWORD *pdwVerLS);

// ---------------------------------------------------------------------------
// %%Function: CLocalComponentInfo::CLocalComponentInfo
// ---------------------------------------------------------------------------
CLocalComponentInfo::CLocalComponentInfo()
{
    szExistingFileName[0] = '\0';
    pBaseExistingFileName = NULL;
    lpDestDir = NULL;
    dwLocFVMS = 0;
    dwLocFVLS = 0;
    ftLastModified.dwLowDateTime = 0;
    ftLastModified.dwHighDateTime = 0;
    lcid = g_lcidBrowser;

    bForceLangGetLatest = FALSE;

    pbEtag = NULL;

    dwAvailMS = 0;
    dwAvailLS = 0;

}  // CLocalComponentInfo

// ---------------------------------------------------------------------------
// %%Function: CLocalComponentInfo::~CLocalComponentInfo
// ---------------------------------------------------------------------------
CLocalComponentInfo::~CLocalComponentInfo()
{
    SAFEDELETE(lpDestDir);

    SAFEDELETE(pbEtag);

}  // ~CLocalComponentInfo

// ---------------------------------------------------------------------------
// %%Function: CLocalComponentInfo::MakeDestDir
// ---------------------------------------------------------------------------
HRESULT
CLocalComponentInfo::MakeDestDir()
{

    HRESULT hr = S_OK;

    Assert(pBaseExistingFileName);

    if (szExistingFileName[0]) {

        DWORD cbLen = (DWORD) (pBaseExistingFileName - szExistingFileName);


        lpDestDir = new char[cbLen + 1];

        if (lpDestDir) {
            StrNCpy(lpDestDir, szExistingFileName, cbLen);
        } else {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;

}  // ~MakeDestDir


// ---------------------------------------------------------------------------
// %%Function: CModuleUsage::CModuleUsage
// CModuleUsage is the basic download obj.
// ---------------------------------------------------------------------------
CModuleUsage::CModuleUsage(LPCSTR szFileName, DWORD muflags, HRESULT *phr)
{

    m_szFileName = NULL;

    if (szFileName) {
        CHAR szCanonical[MAX_PATH];

        if ( CDLGetLongPathNameA( szCanonical, szFileName, MAX_PATH ) != 0 )
            m_szFileName = new char [lstrlen(szCanonical)+1];

        if (m_szFileName) {
            lstrcpy(m_szFileName, szCanonical);
        }
        else
        {
            // CDLGetLongPathName failed, so we are on a platform that
            // doesn't support it, and we can't guess what the right long
            // pathname is. Just use the short one
            m_szFileName = new char [lstrlen(szFileName)+1];

            if (m_szFileName) {
                lstrcpy(m_szFileName, szFileName);
            }
            else {
                *phr = E_OUTOFMEMORY;
            }
        }
    }

    m_dwFlags = muflags;

}  // CModuleUsage

// ---------------------------------------------------------------------------
// %%Function: CModuleUsage::~CModuleUsage
// ---------------------------------------------------------------------------
CModuleUsage::~CModuleUsage()
{
    if (m_szFileName)
        SAFEDELETE(m_szFileName);

}  // ~CModuleUsage

// ---------------------------------------------------------------------------
// %%Function: CModuleUsage::Update(LPCSTR lpClientName)
// ---------------------------------------------------------------------------
HRESULT
CModuleUsage::Update(LPCSTR lpClientName)
{

    return ::UpdateModuleUsage(m_szFileName,
                            lpClientName, NULL,
                            m_dwFlags);
}  // ~CModuleUsage

HRESULT
GetVersionHint(HKEY hkeyVersion, DWORD *pdwVersionMS, DWORD *pdwVersionLS)
{
    DWORD Size = MAX_PATH;
    char szVersionBuf[MAX_PATH];
    DWORD dwType;
    HRESULT hr = S_OK;

    *pdwVersionMS = 0;
    *pdwVersionLS = 0;

    DWORD lResult = ::RegQueryValueEx(hkeyVersion, NULL, NULL, &dwType, 
                        (unsigned char *)szVersionBuf, &Size);

    if (lResult != ERROR_SUCCESS) {

        // value not found, consider this is always with bad/low version
        // of InstalledVersion

        hr = S_FALSE;
        goto Exit;
    }

    if ( FAILED(GetVersionFromString(szVersionBuf, pdwVersionMS, pdwVersionLS))){
        hr = S_FALSE;
    }

Exit:

    return hr;
}


/*******************************************************************

    NAME:        CheckInstalledVersionHint
        
    SYNOPSIS:   Checks for key InstalledVersion under {...}
                If no key then we fail
                once key is present we check version numbers
                S_OK: local version is good enough
                S_FALSE: need update:
                ERROR: not applicable, caller proceeds with InProcServer32
                check.

                [HKCR:clsid]
                    [{...}]
                        [InstalledVersion]
                        Deafult     "1,0,0,1"
                        Path        "c:\foo\foo.ocx"

                The Path is optional, if find we will req update id
                file pointed to by path is missing on client. This is a
                way to robustify if user deletes the file on disk but 
                not the regsitry (unclean uninstall)
                In this case update is needed
                    
********************************************************************/
HRESULT  
CheckInstalledVersionHint(
    HKEY hKeyEmbedding,
    CLocalComponentInfo *plci,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS)
{
    HRESULT hr = S_OK;
    DWORD Size = MAX_PATH;
    DWORD dwType =0;
    LONG lResult = ERROR_SUCCESS;

    const static char * szInstalledVersion = "InstalledVersion";
    const static char * szAvailableVersion = "AvailableVersion";
    const static char * szPATH = "Path";
    const static char * szLASTMODIFIED = "LastModified";
    const static char * szETAG = "Etag";

    char szVersionBuf[MAX_PATH];
    char szFileName[MAX_PATH];
    HKEY hKeyVersion = 0;
    HKEY hkeyAvailableVersion = 0;

    DWORD dwLocFVMS = 0;
    DWORD dwLocFVLS = 0;
    char szLastMod[INTERNET_RFC1123_BUFSIZE+1];

    if (RegOpenKeyEx(hKeyEmbedding, szAvailableVersion, 0, 
            KEY_READ, &hkeyAvailableVersion) == ERROR_SUCCESS) {

        DWORD dwAvailMS = 0;
        DWORD dwAvailLS = 0;

        if ( GetVersionHint(hkeyAvailableVersion, &dwAvailMS, &dwAvailLS) == S_OK){

            plci->dwAvailMS = dwAvailMS;
            plci->dwAvailLS = dwAvailLS;
        }

    }


    lResult = ::RegOpenKeyEx(hKeyEmbedding, szInstalledVersion, 0, 
                        KEY_READ, &hKeyVersion);

    if (lResult != ERROR_SUCCESS) {

        // key not found, consider this is regular ActiveX with no hint
        // of InstalledVersion

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    if ( GetVersionHint(hKeyVersion, &dwLocFVMS, &dwLocFVLS) == S_FALSE){
        hr = S_FALSE;
        goto Exit;
    }

    plci->dwLocFVMS = dwLocFVMS;
    plci->dwLocFVLS = dwLocFVLS;

    Size = INTERNET_RFC1123_BUFSIZE + 1;
    lResult = ::RegQueryValueEx(hKeyVersion, szLASTMODIFIED, NULL, &dwType, 
                        (unsigned char *)szLastMod, &Size);

    if (lResult == ERROR_SUCCESS) {

        SYSTEMTIME st;
        FILETIME ft;
        // convert intenet time to file time
        if (InternetTimeToSystemTime(szLastMod, &st, 0) &&
             SystemTimeToFileTime(&st, &ft) ) {

            memcpy(&(plci->ftLastModified), &ft, sizeof(FILETIME));

        }
    }

    Size = 0;
    lResult = ::RegQueryValueEx(hKeyVersion, szETAG, NULL, &dwType, 
                        (unsigned char *)NULL, &Size);
    if (lResult == ERROR_SUCCESS) {
        char *pbEtag = new char [Size];
        if (pbEtag) {
            lResult = ::RegQueryValueEx(hKeyVersion, szETAG, NULL, &dwType, 
                        (unsigned char *)pbEtag, &Size);
            if (lResult == ERROR_SUCCESS)
                plci->pbEtag = pbEtag;
            else
                delete pbEtag;
        }
    }

    // check file versions
    if ((dwFileVersionMS > dwLocFVMS) ||
             ((dwFileVersionMS == dwLocFVMS) &&
                 (dwFileVersionLS > dwLocFVLS)))
                     hr = S_FALSE;


    if (hr == S_OK) {

        // if we seem to have the right version
        // check if the file physically exists on disk
        // the software can specify this by having a PATH="c:\foo\foo.class"

        // if present we will check for physical file existance

        dwType = 0;
        Size = MAX_PATH;
        lResult = ::SHQueryValueEx(hKeyVersion, szPATH, NULL, &dwType, 
                            (unsigned char *)szFileName, &Size);

        if (lResult != ERROR_SUCCESS)
            goto Exit;

        // value present, check if file is present
        if (GetFileAttributes(szFileName) == -1) {

            // if file is not physically present then clear out our
            // local file version. this comes in the way of doing
            // get latest. (ie get latest will assume that if a local file 
            // is present then do If-Modified-Since
            plci->dwLocFVMS = 0;
            plci->dwLocFVLS = 0;

            hr = S_FALSE;
        }
    }

Exit:

    if (hKeyVersion)
        ::RegCloseKey(hKeyVersion);
    if (hkeyAvailableVersion)
        ::RegCloseKey(hkeyAvailableVersion);

    return hr;
}

HRESULT
CreateJavaPackageManager(IJavaPackageManager **ppPackageManager)
{
    HRESULT hr = S_OK;

    Assert(ppPackageManager);

    if (!(*ppPackageManager)) {
       ICreateJavaPackageMgr *picjpm;

        hr=CoCreateInstance(CLSID_JavaPackageManager,NULL,(CLSCTX_INPROC_SERVER),
            IID_ICreateJavaPackageMgr,(LPVOID *) &picjpm);
        if (SUCCEEDED(hr)) {
            hr = picjpm->GetPackageManager(ppPackageManager);
            picjpm->Release();
        }
    }

    return hr;
}

HRESULT
IsPackageLocallyInstalled(IJavaPackageManager **ppPackageManager, LPCWSTR szPackageName, LPCWSTR szNameSpace, DWORD dwVersionMS, DWORD dwVersionLS)
{
    HRESULT hr = S_OK;      // assume Ok!
    IJavaPackage *pJavaPkg = NULL;
    DWORD dwLocMS = 0;
    DWORD dwLocLS = 0;

    Assert(ppPackageManager);

    if (!(*ppPackageManager)) {
        hr = CreateJavaPackageManager(ppPackageManager);

        if (FAILED(hr))
            goto Exit;
    }


    if (SUCCEEDED((*ppPackageManager)->GetPackage(szPackageName, szNameSpace,  &pJavaPkg))) {

        Assert(pJavaPkg);

        pJavaPkg->GetVersion(&dwLocMS, &dwLocLS);

        if ((dwVersionMS > dwLocMS) ||
                 ((dwVersionMS == dwLocMS) &&
                     (dwVersionLS > dwLocLS)))
                         hr = S_FALSE;


        BSTR bstrFileName = NULL;
        if (SUCCEEDED(pJavaPkg->GetFilePath(&bstrFileName))) {

            // check if file really exists
            LPSTR szFileName = NULL;

            if (SUCCEEDED(Unicode2Ansi(bstrFileName, &szFileName))) {
                if (GetFileAttributes(szFileName) == -1)
                    hr = S_FALSE;
            } else {
                hr = S_FALSE;
            }
        } else {
            hr = S_FALSE;
        }

        SAFESYSFREESTRING(bstrFileName);
        SAFERELEASE(pJavaPkg);

    } else {
        hr = S_FALSE;
    }

Exit:

    return hr;
}

HRESULT
AreNameSpacePackagesIntact(HKEY hkeyJava, LPCWSTR lpwszNameSpace, IJavaPackageManager **ppPackageManager)
{
    int iValue = 0;
    DWORD dwType = REG_SZ;
    DWORD dwValueSize = MAX_PATH;
    char szPkgName[MAX_PATH];
    HRESULT hr = S_OK;

    while (
        RegEnumValue(hkeyJava, iValue++, szPkgName, &dwValueSize, 0,
            &dwType, NULL, NULL) == ERROR_SUCCESS) {

        LPWSTR lpwszPkgName = NULL;

        dwValueSize = MAX_PATH; // reset

        if ( (Ansi2Unicode(szPkgName,&lpwszPkgName) == S_OK)
            && ((IsPackageLocallyInstalled
            (ppPackageManager, lpwszPkgName, lpwszNameSpace, 0,0) != S_OK)) ) {

            hr = S_FALSE;
            SAFEDELETE(lpwszPkgName);
            break;
        }
        SAFEDELETE(lpwszPkgName);
    }

    return hr;
}


HRESULT
ArePackagesIntact(HKEY hkeyContains)
{
    HRESULT hr = S_OK;
    HKEY hkeyJava = 0;
    const static char * szJava = "Java";
    IJavaPackageManager *pPackageManager = NULL;
    DWORD iSubKey = 0;
    DWORD dwSize = MAX_PATH;
    DWORD lResult;
    char szNameSpace[MAX_PATH];

    // first validate pkgs in the global namespace
    if (RegOpenKeyEx( hkeyContains, szJava,
            0, KEY_READ, &hkeyJava) == ERROR_SUCCESS) {

        hr = AreNameSpacePackagesIntact(hkeyJava, NULL, &pPackageManager);

        if (hr != S_OK)
            goto Exit;
    } else {
        goto Exit;
    }

    // validate pkgs in each of other namespaces, if any
    while ( (lResult = RegEnumKeyEx(hkeyJava, iSubKey++, szNameSpace, &dwSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS) {

        dwSize = MAX_PATH;
        HKEY hkeyNameSpace = 0;

        if (RegOpenKeyEx( hkeyJava, szNameSpace,
                0, KEY_READ, &hkeyNameSpace) == ERROR_SUCCESS) {

            LPWSTR lpwszNameSpace = NULL;

            if (Ansi2Unicode(szNameSpace,&lpwszNameSpace) == S_OK)
                hr = AreNameSpacePackagesIntact(hkeyNameSpace, lpwszNameSpace,
                    &pPackageManager);

            SAFEDELETE(lpwszNameSpace);
            RegCloseKey(hkeyNameSpace);

            if (hr != S_OK)
                goto Exit;
        }
    }


    if (lResult != ERROR_NO_MORE_ITEMS) {
        hr = HRESULT_FROM_WIN32(lResult);
        //FALLTHRU goto Exit;
    }

Exit:

    SAFERELEASE(pPackageManager);

    if (hkeyJava)
        RegCloseKey(hkeyJava);

    return hr;
}

/*******************************************************************

NAME:        IsDistUnitLocallyInstalled
    
SYNOPSIS:   S_OK - distribution is installed correctly
            S_FALSE - distribution unit entry exists, but not installed
                      or installed incorrectly
            E_FAIL - distribution unit doesn't exit (no entry for it)
  
********************************************************************/

HRESULT
IsDistUnitLocallyInstalled(
    LPCWSTR szDistUnit,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    CLocalComponentInfo *plci,
    LPSTR szDestDirHint,
    LPBOOL pbParanoidCheck,
    DWORD flags)
{
    LPSTR pszDist = NULL;
    HRESULT hr = S_FALSE;
    HKEY hkeyDist, hkeyThisDist = 0;
    HKEY hkeyContains = 0;
    HKEY hkeyFiles = 0;
    HKEY hkeyDepends = 0;
    const static char * szContains = "Contains";
    const static char * szFiles = "Files";
    const static char * szDistUnitStr = "Distribution Units";
    LONG lResult = ERROR_SUCCESS;
    char szFileName[MAX_PATH];
    ULONG cbSize = MAX_PATH;
    DWORD dwType = REG_SZ;

    if (pbParanoidCheck) {
        *pbParanoidCheck = FALSE;
    }

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS,
                          0, KEY_READ, &hkeyDist);

    if (lResult != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(REGDB_E_KEYMISSING);
        goto Exit;
    }

    if (FAILED((hr=::Unicode2Ansi(szDistUnit, &pszDist))))
    {
        goto Exit;
    }

    hr = S_FALSE;   // reset to NOT found

    // Open the key for this embedding:

    lResult = ::RegOpenKeyEx(hkeyDist, pszDist, 0, KEY_READ, 
                    &hkeyThisDist);

    if (lResult == ERROR_SUCCESS) {

        hr = CheckInstalledVersionHint( hkeyThisDist, plci,
            dwFileVersionMS, dwFileVersionLS);

    }
    else
    {
        hr = HRESULT_FROM_WIN32(REGDB_E_KEYMISSING);
        goto Exit;
    }

    if (hr == S_OK || (SUCCEEDED(hr) && dwFileVersionMS == -1 && dwFileVersionLS == -1)) {

        if (RegOpenKeyEx( hkeyThisDist, szContains, 
            0, KEY_READ, &hkeyContains) == ERROR_SUCCESS) {

            if (pbParanoidCheck) {
                *pbParanoidCheck = TRUE;
            }

            // BUGBUG: only do if paranoid flag on?

            // assert dependency state installed correctly on machine
            // this is where we would have to walk the dependecy tree
            // as well as make sure the contained packages and files
            // are indeed availabel on the client machine
            // BUGBUG: maybe we should only do this in paranoid mode
            // instead of all the time.

            if (RegOpenKeyEx( hkeyContains, szDistUnitStr,
                0, KEY_READ, &hkeyDepends) == ERROR_SUCCESS ) {
        
                int iSubKey = 0;
                while (RegEnumValue(hkeyDepends, iSubKey++,
                    szFileName, &cbSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

                    CLocalComponentInfo lci;
                    LPWSTR wszFileName = 0;
                    CLSID clsidTemp;

                    if (FAILED(Ansi2Unicode(szFileName, &wszFileName)))
                        break;
                    
                    clsidTemp = CLSID_NULL;

                    if (wszFileName)
                        CLSIDFromString(wszFileName, &clsidTemp);
                    // if above call fails DistUnit is not clsid

                    if (IsControlLocallyInstalled(NULL, &clsidTemp, wszFileName, 0,0, &lci, NULL) != S_OK) {
                        SAFEDELETE(wszFileName);

                        plci->dwLocFVMS = 0;
                        plci->dwLocFVLS = 0;

                        hr = S_FALSE;
                        goto Exit;
                    }

                    SAFEDELETE(wszFileName);
                    cbSize = MAX_PATH;
               }
            }

            if (RegOpenKeyEx( hkeyContains, szFiles,
                    0, KEY_READ, &hkeyFiles) == ERROR_SUCCESS) {

                int iValue = 0;
                DWORD dwType = REG_SZ;
                DWORD dwValueSize = MAX_PATH;

                while (RegEnumValue(hkeyFiles, iValue++, 
                    szFileName, &dwValueSize, 0, &dwType, NULL, NULL) == ERROR_SUCCESS) {

                    dwValueSize = MAX_PATH; // reset

                    if (GetFileAttributes(szFileName) == -1) {

                        // if file is not physically present then clear out our
                        // local file version. this comes in the way of doing
                        // get latest. (get latest will assume that if a local 
                        // file is present then do If-Modified-Since

                        plci->dwLocFVMS = 0;
                        plci->dwLocFVLS = 0;

                        hr = S_FALSE;
                        goto Exit;
                    }
                }
            }

            if (ArePackagesIntact(hkeyContains) == S_FALSE) {
                plci->dwLocFVMS = 0;
                plci->dwLocFVLS = 0;
                hr = S_FALSE;
                goto Exit;
            }

        }

    } else {

        hr = S_FALSE; // mark not present, don't error out
    }


Exit:
    if (pszDist)
        delete pszDist;

    if (hkeyDepends)
        ::RegCloseKey(hkeyDepends);

    if (hkeyFiles)
        ::RegCloseKey(hkeyFiles);

    if (hkeyContains)
        ::RegCloseKey(hkeyContains);

    if (hkeyThisDist)
        ::RegCloseKey(hkeyThisDist);

    if (hkeyDist)
        ::RegCloseKey(hkeyDist);



    return hr;
}


IsFileLocallyInstalled(
    LPSTR lpCurCode,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    CLocalComponentInfo *plci,
    LPSTR szDestDirHint,
    BOOL bExactVersion
    )
{
    HRESULT hr = S_FALSE;

    // no clsid, but we have a file name

    // first check for a file of the same name in DestDirHint.
    // This is the directory that the main OCX file existed on
    // and so should be checked first.

    // In case this is a new install this will point to the 
    // suggested destination dir for the DLL.

    if (szDestDirHint) {
        
        if (SearchPath( szDestDirHint, 
                    lpCurCode, NULL, MAX_PATH, 
                        plci->szExistingFileName, &(plci->pBaseExistingFileName))) {

            //    check fileversion to see if update reqd.
            hr = LocalVersionOK(0,plci,dwFileVersionMS, dwFileVersionLS, bExactVersion);

            goto Exit;
        }
    }

    // file not found in suggested destination. Look in system searchpath
    // SearchPath for this filename 

    if (!(SearchPath( NULL, lpCurCode, NULL, MAX_PATH, 
                                        plci->szExistingFileName, &(plci->pBaseExistingFileName)))) {
            hr = S_FALSE;
            goto Exit;
    }

    //    check fileversion to see if update reqd.
    hr = LocalVersionOK(0,plci,dwFileVersionMS, dwFileVersionLS, bExactVersion);

Exit:

    return hr;
}

BOOL GetEXEName(LPSTR szCmdLine)
{

    Assert(szCmdLine);

    LPSTR pchStartBaseName = szCmdLine;
    BOOL bFullyQualified = FALSE;
    BOOL bHasSpaces = FALSE;
    char *pch;

    if (*szCmdLine == '"') {
        szCmdLine++;
        char *pszEnd = StrStrA(szCmdLine, "\"");
        
        ASSERT(pszEnd);
        *pszEnd = '\0';

        if (GetFileAttributes(szCmdLine) != -1) {
            //found the EXE name, but got to get rid of the first quote

            szCmdLine--; // step back to the "
            while (*szCmdLine) {
                *szCmdLine = *(szCmdLine + 1);
                szCmdLine++;
            }
            return TRUE;
        }

        szCmdLine--; // step back to the "
        while (*szCmdLine) {
            *szCmdLine = *(szCmdLine + 1);
            szCmdLine++;
        }
        return FALSE;
    }
        

    // skip past the directory if fully qualified.
    for (pch = szCmdLine; *pch; pch++) {

        if (*pch == '\\')
            pchStartBaseName = pch;

        if ( (*pch == ' ') || (*pch == '\t') )
            bHasSpaces = TRUE;
    }

    if (!bHasSpaces) {
        if (GetFileAttributes(szCmdLine) != -1) {
            //found the EXE name, it is already in szCmdLine
            return TRUE;
        }
        return FALSE;
    }

    // pchStartBaseName now points at the last '\\' if any
    if (*pchStartBaseName == '\\') {
        pchStartBaseName++;

        bFullyQualified = TRUE;
    }

    // pchStartBaseName no points at the base name of the EXE.

    // Now look for spaces. When we find a space we will 
    // replace with a '\0' and check if the cmd line is valid. 
    // if valid, this must be the EXE name, return
    // if not valid, march on to do the same. when we finish
    // examining all of the cmd line, or no more spaces, likely the
    // EXE is missing.

    for (pch = pchStartBaseName; *pch != '\0'; pch++) {

        if ( (*pch == ' ') || (*pch == '\t') ) {
            
            char chTemp = *pch; // sacve the white spc char

            *pch = '\0'; // stomp the spc with nul. now we have in szCmdLine
                        // what could be the full EXE name

            if (bFullyQualified) {

                if (GetFileAttributes(szCmdLine) != -1) {
                    //found the EXE name, it is already in szCmdLine
                    return TRUE;
                }

            } else {

                char szBuf[MAX_PATH];
                LPSTR pBaseFileName;

                if (SearchPath( NULL, szCmdLine, NULL, MAX_PATH, 
                        szBuf, &pBaseFileName)) {

                    //found the EXE name, it is already in szCmdLine
                    return TRUE;
                }
            }

            *pch = chTemp; // restore the while spc and move past.
        }

    }

    return FALSE;
}

BOOL
AdviseForceDownload(const LPCLSID lpclsid, DWORD dwClsContext)
{
    HRESULT hr = S_OK;
    BOOL bNullClsid = lpclsid?IsEqualGUID(*lpclsid , CLSID_NULL):TRUE;
    HKEY hKeyClsid = 0;
    HKEY hKeyEmbedding = 0;
    HKEY hkeyDist = 0;
    BOOL bForceDownload = TRUE;
    LPOLESTR pwcsClsid = NULL;
    DWORD dwType;
    LONG lResult = ERROR_SUCCESS;
    static char * szAppID = "AppID";
    LPSTR pszClsid = NULL;
    CLocalComponentInfo lci;
    static char * szInprocServer32 = "InProcServer32";
    static char * szLocalServer32 = "LocalServer32";

    if (bNullClsid)
        goto Exit;

    // return if we can't get a valid string representation of the CLSID
    if (FAILED((hr=StringFromCLSID(*lpclsid, &pwcsClsid))))
        goto Exit;

    Assert(pwcsClsid != NULL);


    if (FAILED((hr=::Unicode2Ansi(pwcsClsid, &pszClsid))))
    {
        goto Exit;
    }

    // Open root HKEY_CLASSES_ROOT\CLSID key
    lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_READ, &hKeyClsid);


    if (lResult == ERROR_SUCCESS)
    {
        
        // Open the key for this embedding:
        lResult = ::RegOpenKeyEx(hKeyClsid, pszClsid, 0, KEY_READ, 
                        &hKeyEmbedding);

        if (lResult == ERROR_SUCCESS) {

            // check for hint of FileVersion before actually getting FileVersion
            // This way non-PE files, like Java, random data etc. can be
            // accomodated with CODEBASE= with proper version checking.
            // If they are not really COM object then they can't be 
            // instantiated by COM or us (if object needed). But if they
            // use AsyncGetClassBits(no object instantiated) or from the
            // browser give height=0 width=0, the browser will not
            // report placeholders with errors, so they can work gracefully
            // overloading the object tag to do version checking and
            // random stuff

            hr = CheckInstalledVersionHint( hKeyEmbedding, &lci,
                0, 0);

            // if the key is found and 
            // if the latest version is not available then 
            // return now with false, if right version is already 
            // present return. If the key is missing then we
            // proceed with checks for InprocServer32/LocalServer32
            if (SUCCEEDED(hr)) {

                // if using installed version hint and the verdict is 
                // local good enough
                if (hr == S_OK)
                    bForceDownload = FALSE;

                goto Exit;
            }


            hr = S_OK;  // reset


            // ckeck if DCOM
            HKEY hKeyAppID;
            lResult = ::RegOpenKeyEx(hKeyEmbedding, szAppID, 0, 
                                KEY_READ, &hKeyAppID);
            if (lResult == ERROR_SUCCESS) {

                // DCOM 
                // just assume that this is the latest version already
                // we never attempt code download for dcom

                bForceDownload = FALSE; // no force download for DCOM

                RegCloseKey(hKeyAppID);
                goto Exit;
            }

            HKEY hKeyInProc;
            lResult = ::RegOpenKeyEx(hKeyEmbedding, szInprocServer32, 0, 
                                KEY_READ, &hKeyInProc);

            if (lResult != ERROR_SUCCESS) {
                if (RegOpenKeyEx(hKeyEmbedding, szLocalServer32, 0, 
                                KEY_READ, &hKeyInProc) == ERROR_SUCCESS) {

                    // specific look for vb doc obj hack where they just use 
                    // the OBJECT tag to do code download, but not hosting

                    // no inproc but we have localserver
                    // we could have failed because we pass the wrong clsctx

                    RegCloseKey(hKeyInProc);
                    if (!(dwClsContext & CLSCTX_LOCAL_SERVER))
                        bForceDownload = FALSE;
                }

            } else {
                RegCloseKey(hKeyInProc);
            }
        }
    }


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS,
                          0, KEY_READ, &hkeyDist);

    if (lResult == ERROR_SUCCESS)
    {

        // Open the key for this embedding:
        HKEY hkeyThisDist = 0;

        if (RegOpenKeyEx(hkeyDist, pszClsid, 0, KEY_READ, 
                        &hkeyThisDist) == ERROR_SUCCESS) {

            HKEY hkeyJava = 0;
            if (RegOpenKeyEx(hkeyThisDist, "Contains\\Java", 0, KEY_READ, 
                        &hkeyJava) == ERROR_SUCCESS) {
                bForceDownload = FALSE;
                RegCloseKey(hkeyJava);

            }

            RegCloseKey(hkeyThisDist);
        }

        RegCloseKey(hkeyDist);
    }



Exit:
    if (pwcsClsid)
        delete pwcsClsid;

    if (pszClsid)
        delete pszClsid;

    if (hKeyClsid)
        ::RegCloseKey(hKeyClsid);
    if (hKeyEmbedding)
        ::RegCloseKey(hKeyEmbedding);


    return bForceDownload;

}

/*******************************************************************

NAME:        IsControlLocallyInstalled
    
SYNOPSIS:   Indicates whether the provided CLSID represents an
            OLE control.
            If no clsid provided then checks to see if lpCurCode
            exists in system and checks file version to verify if
            update is needed
                
********************************************************************/
HRESULT  
IsControlLocallyInstalled(LPSTR lpCurCode, const LPCLSID lpclsid, 
    LPCWSTR szDistUnit,
    DWORD dwFileVersionMS, DWORD dwFileVersionLS,
    CLocalComponentInfo *plci,
    LPSTR szDestDirHint,
    BOOL bExactVersion)
{
    HRESULT hr1, hr2, hrResult, hr = S_FALSE;
    BOOL bNullClsid = lpclsid?IsEqualGUID(*lpclsid , CLSID_NULL):TRUE;
    BOOL bParanoidCheck = FALSE;
#ifdef _ZEROIMPACT
    LPOLESTR wszClsid = NULL;
    StringFromCLSID(*lpclsid, &wszClsid);
#endif

    if ( bNullClsid  && (lpCurCode == NULL) && (szDistUnit == NULL) ) {

        hr =  E_INVALIDARG;
        goto Exit;

    }

    // hr1: HRESULT for whether the dist unit is installed
    // hr2: HRESULT for whether the particular clsid is installed
    // hrResult: HRESULT for whether the particular clsid is present (but not necessarily installed correctly)

#ifdef _ZEROIMPACT
    // check for an exact match in the ZeroImpact installations first
    if (szDistUnit) {
        hr = IsDistUnitLocallyInstalledZI(szDistUnit, wszClsid, dwFileVersionMS, dwFileVersionLS, plci);
    }
    
    if (hr == S_OK) {
        goto Exit;
    }

    plci->m_bIsZI = FALSE;
#endif

    if (szDistUnit) {
        hr1 = IsDistUnitLocallyInstalled( szDistUnit, dwFileVersionMS, dwFileVersionLS, plci, szDestDirHint, &bParanoidCheck, 0);
    } else {

        if (bNullClsid)
        {
            hr1 = IsFileLocallyInstalled( lpCurCode, dwFileVersionMS, dwFileVersionLS, plci, szDestDirHint, bExactVersion);

            // if no dist unit name or clsid and this is
            // clearly just checking for dependent file then
            // no need to fall thru and check the com br as well

            hr = hr1;
            goto Exit;
        }
        else
        {
            hr1 = E_FAIL;
        }

    }

    hr2 = IsCLSIDLocallyInstalled( lpCurCode, lpclsid, szDistUnit, dwFileVersionMS, dwFileVersionLS, plci, szDestDirHint, &hrResult , bExactVersion);

    if (hr2 != S_OK)
    {
        // if HKLM\CLSID\{CLSID} existed, but control wasn't there, we fail with that error
        // otherwise we fail with hr1.
        if (SUCCEEDED(hrResult))
        {
            hr = hr2;            
        }
        else
        {
            // if DU check returned S_FALSE or S_OK we return that, otherwise return from CLSID check.

            if (SUCCEEDED(hr1))
            {
                hr = hr1;
            }
            else
            {
                hr = hr2;
            }
        }
    }
    else
    {
        if (hr1 == S_FALSE)
        {
            // COM branch says we are OK, but Distribution unit says we are lacking.
            // if we did paranoid checking and then failed the DU then
            // really fail. But if we just looked at the InstalledVersion 
            // in the registry and concluded that our DU is no good then we
            // should go by the COM br and succeed the call as the user
            // could have obtained a newer version thru a mechanism other than
            // code download and so we have no business trying to update this

            // BUGBUG: do we at this point try to correct our registry
            // record of the version? need to ship tomorrow!

            if (bParanoidCheck)
                hr = hr1;
            else
                hr = hr2;

        }
        else
        {
            hr = hr2;
        }
    }

Exit:
#ifdef _ZEROIMPACT
    if(wszClsid)
        delete wszClsid;
#endif
    return hr;
}

/*******************************************************************

NAME:        IsCLSIDLocallyInstalled
    
SYNOPSIS:   Indicates whether the provided CLSID represents an
            OLE control.
            If no clsid provided then checks to see if lpCurCode
            exists in system and checks file version to verify if
            update is needed
                
********************************************************************/
HRESULT  
IsCLSIDLocallyInstalled(LPSTR lpCurCode, const LPCLSID lpclsid, 
    LPCWSTR szDistUnit,
    DWORD dwFileVersionMS, DWORD dwFileVersionLS,
    CLocalComponentInfo *plci,
    LPSTR szDestDirHint,
    HRESULT *pHrExtra,
    BOOL bExactVersion
    )


{
    LPSTR pszClsid = NULL;
    LPOLESTR pwcsClsid = NULL;
    HRESULT hr = S_FALSE;
    DWORD dwType;
    LONG lResult = ERROR_SUCCESS;
    static char * szInprocServer32 = "InProcServer32";
    static char * szLocalServer32 = "LocalServer32";
    static char * szAppID = "AppID";
    HKEY hKeyClsid = 0;
    DWORD Size = MAX_PATH;

    if (pHrExtra)
        *pHrExtra = E_FAIL;

    // return if we can't get a valid string representation of the CLSID
    if (FAILED((hr=StringFromCLSID(*lpclsid, &pwcsClsid))))
        goto Exit;

    Assert(pwcsClsid != NULL);

    // Open root HKEY_CLASSES_ROOT\CLSID key
    lResult = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_READ, &hKeyClsid);

    if (lResult == ERROR_SUCCESS)
    {
        if (FAILED((hr=::Unicode2Ansi(pwcsClsid, &pszClsid))))
        {
            goto Exit;
        }
        
        // Open the key for this embedding:
        HKEY hKeyEmbedding;
        HKEY hKeyInProc;

        lResult = ::RegOpenKeyEx(hKeyClsid, pszClsid, 0, KEY_READ, 
                        &hKeyEmbedding);

        if (lResult == ERROR_SUCCESS) {

            // check for hint of FileVersion before actually getting FileVersion
            // This way non-PE files, like Java, random data etc. can be
            // accomodated with CODEBASE= with proper version checking.
            // If they are not really COM object then they can't be 
            // instantiated by COM or us (if object needed). But if they
            // use AsyncGetClassBits(no object instantiated) or from the
            // browser give height=0 width=0, the browser will not
            // report placeholders with errors, so they can work gracefully
            // overloading the object tag to do version checking and
            // random stuff

            if (pHrExtra)
            {
                // indicate that CLSID reg key exists, so any failures after this
                // imply the control is not registered correctly
                *pHrExtra = S_OK;
            }

            hr = CheckInstalledVersionHint( hKeyEmbedding, plci,
                dwFileVersionMS, dwFileVersionLS);

            // if the key is found and 
            // if the latest version is not available then 
            // return now with false, if right version is already 
            // present return. If the key is missing then we
            // proceed with checks for InprocServer32/LocalServer32
            if (SUCCEEDED(hr))
                goto finish_all;

            hr = S_OK;  // reset

            // ckeck if DCOM
            HKEY hKeyAppID;
            lResult = ::RegOpenKeyEx(hKeyEmbedding, szAppID, 0, 
                                KEY_READ, &hKeyAppID);
            if (lResult == ERROR_SUCCESS) {

                // DCOM 
                // just assume that this is the latest version already
                // we never attempt code download for dcom

                ::RegCloseKey(hKeyAppID);
                goto finish_all;
            }


            lResult = ::RegOpenKeyEx(hKeyEmbedding, szInprocServer32, 0, 
                                KEY_READ, &hKeyInProc);

            if (lResult == ERROR_SUCCESS) {

                Size = MAX_PATH;
                lResult = ::SHQueryValueEx(hKeyInProc, NULL, NULL, &dwType, 
                                    (unsigned char *)plci->szExistingFileName, &Size);

                if (lResult == ERROR_SUCCESS) {

                    if (!(SearchPath( NULL, 
                                plci->szExistingFileName, NULL, MAX_PATH, 
                                    plci->szExistingFileName, &(plci->pBaseExistingFileName)))) {
                        hr = S_FALSE;
                        goto finish_verchecks;
                    }

                    //    check fileversion to see if update reqd.
                    hr = LocalVersionOK(hKeyEmbedding, plci,
                                dwFileVersionMS, dwFileVersionLS, bExactVersion);

                    if (plci->bForceLangGetLatest) {
                        hr = NeedForceLanguageCheck(hKeyEmbedding, plci);
                    }

                    goto finish_verchecks;

                } else {
                    hr = S_FALSE; // problem: can't locate file
                    goto finish_verchecks;
                }

            } else {

                lResult = ::RegOpenKeyEx(hKeyEmbedding, szLocalServer32, 0, 
                                KEY_READ, &hKeyInProc);

                if (lResult != ERROR_SUCCESS) {
                    hr = S_FALSE; // problem :have a clsid but, can't locate it
                    goto finish_all;
                 }    

                Size = MAX_PATH;
                lResult = ::SHQueryValueEx(hKeyInProc, NULL, NULL, &dwType, 
                                    (unsigned char *)plci->szExistingFileName, &Size);

                if (lResult == ERROR_SUCCESS) {

                    // strip out args if any for this localserver32
                    // and extract only the EXE name

                    GetEXEName(plci->szExistingFileName);

                    if (!(SearchPath( NULL, 
                                plci->szExistingFileName, NULL, MAX_PATH, 
                                    plci->szExistingFileName, &(plci->pBaseExistingFileName)))) {
                        hr = S_FALSE;
                        goto finish_verchecks;
                    }

                    //    check fileversion to see if update reqd.
                    hr = LocalVersionOK(hKeyEmbedding, plci, dwFileVersionMS, dwFileVersionLS, bExactVersion);

                    if (plci->bForceLangGetLatest)
                        hr = NeedForceLanguageCheck(hKeyEmbedding, plci);

                    goto finish_verchecks;

                } else {
                    hr = S_FALSE; // problem: can't locate file
                    goto finish_verchecks;
                }

            }


            finish_verchecks:
            ::RegCloseKey(hKeyInProc);

            finish_all:

            ::RegCloseKey(hKeyEmbedding);

        } else {
            // here if we could not find the embedding in HKCR\CLSID
            
            hr = S_FALSE;
            
        }

    } else
        hr = S_FALSE;

Exit:

    // release the string allocated by StringFromCLSID
    if (pwcsClsid)
        delete pwcsClsid;

    if (pszClsid)
        delete pszClsid;

    if (hKeyClsid)
        ::RegCloseKey(hKeyClsid);

    return hr;
}

BOOL SupportsSelfRegister(LPSTR szFileName)
{
    return SniffStringFileInfo( szFileName, TEXT("OLESelfRegister") );
}

BOOL WantsAutoExpire(LPSTR szFileName, DWORD *pnExpireDays)
{
    return SniffStringFileInfo( szFileName, TEXT("Expire"), pnExpireDays );
}


HRESULT GetFileVersion(CLocalComponentInfo *plci, LPDWORD pdwFileVersionMS, LPDWORD pdwFileVersionLS)
{
    DWORD  handle;
    UINT  uiInfoSize;
    UINT  uiVerSize ;
    UINT  uiSize ;
    BYTE* pbData = NULL ;
    VS_FIXEDFILEINFO *lpVSInfo;;
    HRESULT hr = S_OK;
    LPVOID lpVerBuffer = NULL;

#ifdef UNIX
    // We don't have version.dll
    DebugBreak();
    return E_INVALIDARG;
#endif

    if (!pdwFileVersionMS || !pdwFileVersionLS) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;

    // Get the size of the version information.
    uiInfoSize = g_versiondll.GetFileVersionInfoSize( (char *)plci->szExistingFileName, &handle);

    if (uiInfoSize == 0) {
        hr = S_FALSE;
        goto Exit;
    }

    // Allocate a buffer for the version information.
    pbData = new BYTE[uiInfoSize] ;
    if (!pbData)
         return E_OUTOFMEMORY;

    // Fill the buffer with the version information.
    if (!g_versiondll.GetFileVersionInfo((char *)plci->szExistingFileName, handle, uiInfoSize, pbData)) {
         hr = HRESULT_FROM_WIN32(GetLastError());
         goto Exit ;
    }

    // Get the translation information.
    if (!g_versiondll.VerQueryValue( pbData, "\\", (void**)&lpVSInfo, &uiVerSize)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit ;
    }    

    if (!uiVerSize) {
        hr = E_FAIL;
        goto Exit ;
    }

    *pdwFileVersionMS = lpVSInfo->dwFileVersionMS;
    *pdwFileVersionLS = lpVSInfo->dwFileVersionLS;

    // Get the translation information.
    if (!g_versiondll.VerQueryValue( pbData, "\\VarFileInfo\\Translation", &lpVerBuffer, &uiVerSize)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit ;
    }    

    if (!uiVerSize) {
        hr = E_FAIL;
        goto Exit ;
    }

     plci->lcid = LOWORD(*((DWORD *) lpVerBuffer));   // Language ID

Exit:

    if (pbData)
        delete [] pbData ;

    return hr;
}

DWORD
GetLanguageCheckInterval(HKEY hkeyCheckPeriod)
{
    DWORD dwMagicDays = 30;

    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    char szLangString[MAX_PATH];

    LCID lcidPriOverride = PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));
    LCID lcidPriBrowser = PRIMARYLANGID(LANGIDFROMLCID(g_lcidBrowser));

    if (SUCCEEDED(GetLangString(lcidPriOverride, szLangString))) {

        if (RegQueryValueEx(hkeyCheckPeriod, szLangString, NULL, &dwType, 
            (unsigned char *)&dwMagicDays, &dwSize) == ERROR_SUCCESS) {

            return dwMagicDays;
        }
    }

    if ( (lcidPriOverride != lcidPriBrowser) &&
        SUCCEEDED(GetLangString(lcidPriBrowser, szLangString))) {

        if (RegQueryValueEx(hkeyCheckPeriod, szLangString, NULL, &dwType, 
            (unsigned char *)&dwMagicDays, &dwSize) == ERROR_SUCCESS) {

            return dwMagicDays;
        }
    }

    return dwMagicDays;
}

// returns:
//          S_OK: local version OK or local version lang check not reqd now
//          S_FALSE: localversion not of right lang force lang check now
//          ERROR: fail

HRESULT NeedForceLanguageCheck(HKEY hkeyCLSID, CLocalComponentInfo *plci)
{
    HRESULT hr = S_OK;
    DWORD lResult;
    const char *szCHECKPERIOD = "LanguageCheckPeriod";
    const char *szLASTCHECKEDHI = "LastCheckedHi";
    DWORD dwMagicPerDay = 201;
    DWORD dwMagicDays;
    DWORD dwType;
    DWORD dwSize;
    FILETIME ftlast, ftnow;
    SYSTEMTIME st;
    HKEY hkeyCheckPeriod = 0;
    char szLangEnable[MAX_PATH];


    if (!plci->bForceLangGetLatest)
        return hr;

    // lang is mismatched for this browser
    // check when was the last time we checked for the right lang

    if ((lResult = RegOpenKeyEx( hkeyCLSID, szCHECKPERIOD,
                        0, KEY_READ, &hkeyCheckPeriod)) != ERROR_SUCCESS) {
        plci->bForceLangGetLatest = FALSE;
        goto Exit;

    }

    szLangEnable[0] = '\0';
    dwType = REG_SZ;
    dwSize = MAX_PATH;
    if ( (RegQueryValueEx(hkeyCheckPeriod, NULL, NULL, &dwType, 
         (unsigned char *)szLangEnable, &dwSize) != ERROR_SUCCESS) ||
         lstrcmpi(szLangEnable, "Enabled") != 0 ) {

        plci->bForceLangGetLatest = FALSE;
        goto Exit;
    }

    // see if lang check interval is specified for this lang
    dwMagicDays = GetLanguageCheckInterval(hkeyCheckPeriod);

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ftnow);
    ftnow.dwLowDateTime = 0;

    memset(&ftlast, 0, sizeof(FILETIME));

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hkeyCheckPeriod, szLASTCHECKEDHI, NULL, &dwType, 
        (unsigned char *)&ftlast.dwHighDateTime, &dwSize) == ERROR_SUCCESS) {

        ftlast.dwHighDateTime += (dwMagicPerDay * dwMagicDays);
    }

    if (CompareFileTime(&ftlast, &ftnow) > 0) {
        plci->bForceLangGetLatest = FALSE;
    }


Exit:

    SAFEREGCLOSEKEY(hkeyCheckPeriod);

    if (FAILED(hr))
        plci->bForceLangGetLatest = FALSE;

    return plci->bForceLangGetLatest?S_FALSE:S_OK;
}

HRESULT IsRightLanguageLocallyInstalled(CLocalComponentInfo *plci)
{
    HRESULT hr = S_OK;
    LCID lcidLocalVersion;
    LCID lcidNeeded;

    if (!plci->lcid)    // lang neutral?
        goto Exit;

    // make sure that the browser locale and lang strings are
    // initialized at this point

    hr = InitBrowserLangStrings();

    if (FAILED(hr))
        goto Exit;

    // BUGBUG: we are using threadlocale here instead of the 
    // bindopts from the bindctx passed in
    if (plci->lcid == GetThreadLocale())    // full match with override?
        goto Exit;
    if (plci->lcid == g_lcidBrowser)    // full match with browser?
        goto Exit;

    // get primary lang of local version
    lcidLocalVersion = PRIMARYLANGID(LANGIDFROMLCID(plci->lcid));

    // check with primary language of override
    lcidNeeded = PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));

    if (lcidLocalVersion == lcidNeeded)     // same primary lang?
        goto Exit;

    // check with primary language of browser
    lcidNeeded = PRIMARYLANGID(LANGIDFROMLCID(g_lcidBrowser));

    if (lcidLocalVersion == lcidNeeded)     // same primary lang?
        goto Exit;


    // BUGBUG: how to detect language neutral or multiligual OCX

    // we have a mismatch
    // check when was the last time we check and if past the
    // interval to check for new language availability
    // force a download now.
    
    hr = S_FALSE;

    plci->bForceLangGetLatest = TRUE;

Exit:

    return hr;
}

// 
HRESULT LocalVersionOK(HKEY hkeyCLSID, CLocalComponentInfo *plci, DWORD dwFileVersionMS, DWORD dwFileVersionLS, BOOL bExactVersion)
{
    DWORD  handle;
    HRESULT hr = S_OK; // assume local version OK.
    DWORD dwLocFVMS = 0;
    DWORD dwLocFVLS = 0;
    HKEY hkeyCheckPeriod = 0;
    const char *szCHECKPERIOD = "LanguageCheckPeriod";

    if (FAILED(hr = plci->MakeDestDir()) ) {
        goto Exit;
    }

#ifdef UNIX
    return S_OK;
#endif 

    if ((dwFileVersionMS == 0) && (dwFileVersionLS == 0)) {

        // for dlls that don't require lang support and have no version req
        // don't check version numbers
        // this is to boost perf on system/IE dlls

        // One can also avoid such checks
        // by adding a [InstalledVersion] key under the clsid

        if (!hkeyCLSID || RegOpenKeyEx( hkeyCLSID, szCHECKPERIOD,
                            0, KEY_READ, &hkeyCheckPeriod) != ERROR_SUCCESS) {
            goto Exit;

        }


    }

    hr = GetFileVersion( plci, &dwLocFVMS, &dwLocFVLS);

    if (hr == S_OK) {

        plci->dwLocFVMS = dwLocFVMS;
        plci->dwLocFVLS = dwLocFVLS;

        if (bExactVersion) {
            if (dwFileVersionMS != dwLocFVMS || dwFileVersionLS != dwLocFVLS) {
                hr = S_FALSE;
            } else {
                // check language
                // sets the plci->bForcelangGetLatest if reqd
                IsRightLanguageLocallyInstalled(plci);
            }
        }
        else {
            if ((dwFileVersionMS > dwLocFVMS) ||
                     ((dwFileVersionMS == dwLocFVMS) &&
                         (dwFileVersionLS > dwLocFVLS))) {
                             hr = S_FALSE;
            } else {
                // check language
                // sets the plci->bForcelangGetLatest if reqd
                IsRightLanguageLocallyInstalled(plci);
    
            }
        }
    }

    if ((dwFileVersionMS == 0) && (dwFileVersionLS == 0)) {
        hr = S_OK;
    }

    if ((dwFileVersionMS == -1) && (dwFileVersionLS == -1)) {
        hr = S_FALSE;
    }
    

Exit:

    if (hkeyCheckPeriod)
        RegCloseKey(hkeyCheckPeriod);

    return hr;
}


/*
 *
 * UpdateSharedDlls
 *
 * the SharedDlls section looks like this
 *
 * [SharedDlls]
 *        C:\Windows\System\foo.ocx = <ref count>
 *
 *    Parameters:
 *
 *    szFileName        full file name of module we want to use
 *
 *    Returns:
 *
 *        S_OK    incremented tge shared dlls ref count.
 *
 *        Error    the error encountered
 */

HRESULT
UpdateSharedDlls( LPCSTR szFileName)
{
    HKEY hKeySD = NULL;
    HRESULT hr = S_OK;
    DWORD dwType;
    DWORD dwRef = 1;
    DWORD dwSize = sizeof(DWORD);
    LONG lResult;

    // get the main SHAREDDLLS key ready; this is never freed!


    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_SHAREDDLLS,
                        0, KEY_ALL_ACCESS, &hKeySD)) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( HKEY_LOCAL_MACHINE,
                   REGSTR_PATH_SHAREDDLLS, &hKeySD)) != ERROR_SUCCESS) {
            hKeySD = NULL;
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }


    // now look for szFileName
    lResult = SHQueryValueEx(hKeySD, szFileName, NULL, &dwType, 
                        (unsigned char *)&dwRef, &dwSize);

    if (lResult == ERROR_SUCCESS)
        dwRef++;

    // does not exist. Create one and initialize to 1

    if ((lResult = RegSetValueEx (hKeySD, szFileName, 0, REG_DWORD,
                        (unsigned char *)&dwRef, 
                        sizeof(DWORD))) != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

Exit:

    if (hKeySD)
        RegCloseKey(hKeySD);

    return hr;
}

/*
 *
 * UpdateModuleUsage
 *
 * the module usage section in the regitry looks like this
 *
 * [ModuleUsage]
 *        [c:/windows/occache/foo.ocx]
 *            Owner = Internet Code Downloader
 *            FileVersion = <optional text version of 64-bit fileversion of ocx
 *            [clients]
 *                Internet Code Downloader = <this client's ref count>
 * To allow for full path names without using the backslash we convert
 * backslahes to forward slashes.
 *
 *
 *    Parameters:
 *
 *    szFileName        full file name of module we want to use
 *
 *    szClientName      name of or stringfromclsid of client.
 *
 *    szClientPath      optional (if present we can detect if client is gone)
 *
 *    muFlags:
 *        MU_CLIENT    mark ourselves client
 *        MU_OWNER     mark ourselves owner
 *
 *    Returns:
 *        S_OK    we updated the module usage section, 
 *                and if that was previously absent then we also upped the 
 *                shared dlls count.
 *
 *        Error    the error encountered
 */
HRESULT 
UpdateModuleUsage(
    LPCSTR szFileName,
    LPCSTR szClientName,
    LPCSTR szClientPath,
    LONG muFlags)
{
    HRESULT hr = S_OK;
    LONG lResult = 0;
    BOOL fUpdateSharedDlls = TRUE;

    DWORD dwType;
    HKEY hKeyMod = NULL;

    DWORD dwSize = MAX_PATH;
    char szBuf[MAX_PATH];

    const char *pchSrc;
    char *pchDest;
    static const LPCSTR szCLIENTPATHDEFAULT = "";
    LPCSTR lpClientPath = (szClientPath)?szClientPath:szCLIENTPATHDEFAULT;

    HKEY hKeyMU = NULL;
    static const char szOWNER[] = ".Owner";
    static const char szUNKNOWN[] = "Unknown Owner";

    Assert(szClientName);
    DWORD cbClientName = lstrlen(szClientName);
    DWORD cbUnknown = sizeof(szUNKNOWN);

    char szShortFileName[MAX_PATH];

#ifdef SHORTEN
    if (!GetShortPathName(szFileName, szShortFileName, MAX_PATH)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
#else
    lstrcpy(szShortFileName, szFileName);
#endif

    if (g_bRunOnWin95) {
        char szCharFileName[MAX_PATH];

        OemToChar(szShortFileName, szCharFileName);
        lstrcpy(szShortFileName, szCharFileName);
    }

    // get the main MODULEUSAGE key ready; this is never freed!

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_MODULE_USAGE,
            0, KEY_ALL_ACCESS, &hKeyMU) != ERROR_SUCCESS)
        if ((lResult = RegCreateKey( HKEY_LOCAL_MACHINE,
                   REGSTR_PATH_MODULE_USAGE, &hKeyMU)) != ERROR_SUCCESS) {
            hKeyMU = NULL;
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }


    // check if Usage section is present for this dll
    // open the file's section we are concerned with
    // if absent create it


    // BUGBUG: win95 registry bug does not allow keys to be > 255
    // MAX_PATH for filename is 260
    pchDest = szBuf;
    for (pchSrc = szShortFileName; *pchSrc != '\0'; pchSrc++, pchDest++) {
        if ((*pchDest = *pchSrc) == '\\')
            *pchDest = '/';
    }
    *pchDest = '\0'; // null terminate
    szBuf[256] = '\0'; // truncate if longer than 255 ude to win95 registry bug

    if (RegOpenKeyEx( hKeyMU, szBuf,
            0, KEY_ALL_ACCESS, &hKeyMod) != ERROR_SUCCESS) {
        if ((lResult = RegCreateKey( hKeyMU,
                   szBuf, &hKeyMod)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
            }
    }


    // now look for '.Owner='
    dwSize = MAX_PATH;
    szBuf[0] = '\0';
    if (RegQueryValueEx(hKeyMod, szOWNER, NULL, &dwType, 
                        (unsigned char *)szBuf, &dwSize) == ERROR_SUCCESS) {
        if ((lstrcmpi(szBuf, szClientName) != 0) && (muFlags & MU_OWNER)) {
            // if we are the not the owner we can't make ourselves the owner
            hr = E_INVALIDARG;
            goto Exit;
        }

    } else {

        // '.Owner =' does not exist. Create one and initialize to us
        // if muFlags & MU_OWNER

        if (((lResult = RegSetValueEx (hKeyMod, szOWNER, 0, REG_SZ,
                (UCHAR *)((muFlags & MU_OWNER)?szClientName:szUNKNOWN),
                ((muFlags & MU_OWNER)?cbClientName:cbUnknown)))) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

    }


    // look for szClientName already marked as a client

    dwSize = MAX_PATH;
    if (SHQueryValueEx(hKeyMod, szClientName, NULL, &dwType, 
                        (unsigned char *)szBuf, &dwSize) == ERROR_SUCCESS) {

        // signal that we have already registered as a 
        // client and so don't up ref count in shareddlls

        fUpdateSharedDlls = FALSE; 


    } else {

        // add ourselves as a client

        if ((lResult =RegSetValueEx(hKeyMod, szClientName, 0, REG_SZ,
                (unsigned char *)lpClientPath, lstrlen(lpClientPath)+1 )) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

    }


Exit:

    if (hKeyMod)
        RegCloseKey(hKeyMod);

    if (hKeyMU)
        RegCloseKey(hKeyMU);

    // Update ref count in SharedDlls only if usage section was not
    // already updated. This will ensure that the code downloader has
    // just one ref count represented in SharedDlls

    if ( fUpdateSharedDlls)
        hr = UpdateSharedDlls(szShortFileName);

    return hr;

}

// Name: SniffStringFileInfo
// Parameters:
//     szFileName   -   full path to file whose StringFileInfo we want to sniff
//     lpszSubblock -   sub block of the StringFileInfo to look for
//     pdw          -   place to put the string, interpreted as a number
// Returns:
//     TRUE - if the subblock is present.
//     FALSE - if the subblock is absent.
// Notes:
//     This function implements stuff common to SupportsSelfRegister and WantsAutoExpire

BOOL SniffStringFileInfo(  LPSTR szFileName, LPCTSTR lpszSubblock, DWORD *pdw )
{
    BOOL bResult = FALSE;
    DWORD  handle;
    UINT  uiInfoSize;
    UINT  uiVerSize ;
    UINT  uiSize ;
    BYTE* pbData = NULL ;
    DWORD* lpBuffer;
    TCHAR szName[512] ;
    LPTSTR szExpire;

    if ( pdw )
        *pdw = 0;

#ifdef UNIX
    // Don't have version.dll
    DebugBreak();
    return FALSE;
#endif 
    // Get the size of the version information.
    uiInfoSize = g_versiondll.GetFileVersionInfoSize( szFileName, &handle);

    if (uiInfoSize == 0) return FALSE ;

    // Allocate a buffer for the version information.
    pbData = new BYTE[uiInfoSize] ;

    if (!pbData)
         return TRUE; // nothing nasty, just quirky

    // Fill the buffer with the version information.
    bResult = g_versiondll.GetFileVersionInfo( szFileName, handle, uiInfoSize, pbData);

    if (!bResult) goto Exit ;

    // Get the translation information.
    bResult = g_versiondll.VerQueryValue( pbData, "\\VarFileInfo\\Translation",
                       (void**)&lpBuffer, &uiVerSize);

    if (!bResult) goto Exit ;

    if (!uiVerSize) goto Exit ;

    // Build the path to the OLESelfRegister key
    // using the translation information.
    wsprintf( szName, "\\StringFileInfo\\%04hX%04hX\\%s",
              LOWORD(*lpBuffer), HIWORD(*lpBuffer), lpszSubblock) ;

    // Search for the key.
    bResult = g_versiondll.VerQueryValue( pbData, szName, (void**)&szExpire, &uiSize);

    // If there's a string there, we need to convert it to a count of days.
    if ( bResult && pdw && uiSize )
    {
        DWORD dwExpire = 0;

        for ( ; *szExpire; szExpire++ )
        {
            if ( (*szExpire >= TEXT('0') && *szExpire <= TEXT('9')) )
                dwExpire = dwExpire * 10 + *szExpire - TEXT('0');
            else
                break;
        }

        if (dwExpire > MAX_EXPIRE_DAYS)
            dwExpire = MAX_EXPIRE_DAYS;

        *pdw = dwExpire;
    }

Exit:
    delete [] pbData ;
    return bResult ;
}


// returns S_OK on dist unit(at version number) installed zeroimpactly, S_FALSE on not installed 
// zeroimpactly, or error codes
HRESULT
IsDistUnitLocallyInstalledZI(LPCWSTR wszDistUnit, LPCWSTR wszClsid, DWORD dwFileVersionMS, DWORD dwFileVersionLS, 
                             CLocalComponentInfo * plci)
{
    HRESULT hrNormalCheck = S_OK;
    HRESULT hrZICheck = S_OK;
    HRESULT hrDll = S_OK;
    int cBufLen = MAX_PATH;
    int iLenDU = 0;
    int iLenVer = 0;
    TCHAR szDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    TCHAR szDllName[MAX_PATH];
    WCHAR wszDistDotVersion[MAX_PATH];
    TCHAR szVersion[MAX_PATH];
    WCHAR wszVersion[MAX_PATH];
    BOOL bParanoid = TRUE;

    ASSERT(wszDistUnit);

    if (!dwFileVersionMS && !dwFileVersionLS)
    {
        GetLatestZIVersion(wszDistUnit, &dwFileVersionMS, &dwFileVersionLS);
    }
    
    if(GetStringFromVersion(szVersion, dwFileVersionMS, dwFileVersionLS, '_'))
    {
        if(MultiByteToWideChar(CP_ACP, 0, szVersion, -1, wszVersion, MAX_PATH) != 0)
        {
            // buffer overflow guard
            iLenDU = lstrlenW(wszDistUnit);
            iLenVer = lstrlenW(wszVersion);
            if(iLenDU + iLenVer /*dot and null*/ + 2 > MAX_PATH)
                return E_UNEXPECTED;
            StrCpyW(wszDistDotVersion, wszDistUnit);
            wszDistDotVersion[iLenDU++] = L'!';
            StrCpyW(wszDistDotVersion + iLenDU, wszVersion);
        }
    }


    // Use the ZI api first; this has the final say on whether the control is installed ZI;
    // It also fills in szDir
    hrZICheck = ZIGetInstallationDir(wszDistUnit, &dwFileVersionMS, &dwFileVersionLS, szDir, &cBufLen);

    // Call the normal IsDistUnitLocallyInstalled function; this will fill in plci
    hrNormalCheck = IsDistUnitLocallyInstalled(wszDistDotVersion, dwFileVersionMS, 
                                               dwFileVersionLS, plci, NULL, &bParanoid, 0);


    if(FAILED(hrZICheck))
    {
        if(hrZICheck == HRESULT_FROM_WIN32(ERROR_MORE_DATA))
            return E_UNEXPECTED;
        else
            return hrZICheck;
    }

    // hrZICheck == S_OK: directory was found; hrZICheck == S_FALSE: directory was not found
    if(hrZICheck == S_OK)
    {
        cBufLen = MAX_PATH;

        // Found the dir, now look for a dll name to report up
        hrDll = ZIGetDllName(szDir, wszDistUnit, wszClsid, dwFileVersionMS, dwFileVersionLS, szFile, &cBufLen);
        if(hrDll == S_OK)
        {
            wsprintf(szDllName, TEXT("%s\\%s"), szDir, szFile);
            if(MultiByteToWideChar(CP_ACP, 0, szDllName, -1, plci->wszDllName, MAX_PATH) == 0)
                plci->wszDllName[0] = L'\0';
        }
        else
        {
            plci->wszDllName[0] = L'\0';
        }

        // if IsDistUnit...lled already filled this in, don't overwrite
        if(hrNormalCheck != S_OK)
        {
            plci->dwLocFVMS = dwFileVersionMS;
            plci->dwLocFVLS = dwFileVersionLS;
        }
        plci->m_bIsZI = TRUE;
    }
    else
    {
        plci->dwLocFVMS = 0;
        plci->dwLocFVLS = 0;
        plci->m_bIsZI = FALSE;
    }

    return hrZICheck;
}

void GetLatestZIVersion(const WCHAR *pwzDistUnit, DWORD *pdwVerMS,
                        DWORD *pdwVerLS)
{
    HKEY                         hkeyZI = 0;
    HANDLE                       hFile = INVALID_HANDLE_VALUE;
    DWORD                        dwDistVerMS = 0;
    DWORD                        dwDistVerLS = 0;
    DWORD                        lResult;
    DWORD                        iSubKey;
    DWORD                        dwSize = MAX_REGSTR_LEN;
    char                         szDistUnitCur[MAX_REGSTR_LEN];
    char                        *szPtr = NULL;
    char                        *pszDistUnit = NULL;
    int                          iLen = 0;

    if (Unicode2Ansi(pwzDistUnit, &pszDistUnit))
    {
        goto Exit;
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_ZEROIMPACT_DIRS, 0, KEY_READ,
                     &hkeyZI) == ERROR_SUCCESS)
    {
        iSubKey = 0;
        while ((lResult = RegEnumKeyEx(hkeyZI, iSubKey++, szDistUnitCur,
                                       &dwSize, NULL, NULL, NULL,
                                       NULL)) == ERROR_SUCCESS)
        {
            szPtr = StrStrI(szDistUnitCur, pszDistUnit);

            if (szPtr)
            {
                ExtractVersion(szDistUnitCur, &dwDistVerMS, &dwDistVerLS);

                if (dwDistVerMS > *pdwVerMS ||
                    ((dwDistVerMS == *pdwVerMS) && dwDistVerLS > *pdwVerLS))
                {
                    *pdwVerMS = dwDistVerMS;
                    *pdwVerLS = dwDistVerLS;
                }
            }
            
    
            dwSize = MAX_REGSTR_LEN;
        }
    }

    if (!*pdwVerMS && !*pdwVerLS)
    {
        LPTSTR                      szDir = (LPTSTR)GetZeroImpactRootDir();
        TCHAR                       szQualifiedPath[MAX_PATH];
        WIN32_FIND_DATA             wfd;

        // Still haven't found a version. Look in the file system

        if (szDir) {
            StrCpyN(szQualifiedPath, szDir, MAX_PATH);

            Assert(lstrlen(szQualifiedPath) + 4 < MAX_PATH);
                
            StrCat(szQualifiedPath, TEXT("\\*.*"));

            hFile = FindFirstFile(szQualifiedPath, &wfd);

            iLen = lstrlen(pszDistUnit);

            if (hFile != INVALID_HANDLE_VALUE) {
                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (!StrCmpNI(wfd.cFileName, pszDistUnit, iLen)) {
                        ExtractVersion(wfd.cFileName, &dwDistVerMS,
                                       &dwDistVerLS);
                        
                        if (dwDistVerMS > *pdwVerMS ||
                            ((dwDistVerMS == *pdwVerMS) &&
                              dwDistVerLS > *pdwVerLS))
                        {
                            *pdwVerMS = dwDistVerMS;
                            *pdwVerLS = dwDistVerLS;
                        }
                    }
                }
                        
                while (FindNextFile(hFile, &wfd)) {
                    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (!StrCmpNI(wfd.cFileName, pszDistUnit, iLen)) {
                            ExtractVersion(wfd.cFileName, &dwDistVerMS,
                                           &dwDistVerLS);
                            
                            if (dwDistVerMS > *pdwVerMS ||
                                ((dwDistVerMS == *pdwVerMS) &&
                                  dwDistVerLS > *pdwVerLS))
                            {
                                *pdwVerMS = dwDistVerMS;
                                *pdwVerLS = dwDistVerLS;
                            }
                        }
                    }
                }
            }
        }
    }

Exit:

    if (hkeyZI)
    {
        RegCloseKey(hkeyZI);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        FindClose(hFile);
    }

    SAFEDELETE(pszDistUnit);
}

void ExtractVersion(char *pszDistUnit, DWORD *pdwVerMS, DWORD *pdwVerLS)
{
    char                    *pszCopy = NULL;
    char                    *pszPtr = NULL;
    int                      iLen = 0;

    if (!pszDistUnit)
    {
        return;
    }

    iLen = lstrlen(pszDistUnit) + 1;
    pszCopy = new char[iLen];

    if (!pszCopy) {
        return;
    }

    StrNCpy(pszCopy, pszDistUnit, iLen);

    pszPtr = pszCopy;

    // Convert _ to , for GetVersionFromString()

    while (*pszPtr)
    {
        if (*pszPtr == '_')
        {
            *pszPtr = ',';
        }

        pszPtr++;
    }

    pszPtr = StrStrA(pszCopy, "!");

    if (pszPtr) {
        pszPtr++;
        GetVersionFromString(pszPtr, pdwVerMS, pdwVerLS);
    }

    SAFEDELETE(pszCopy);
}

