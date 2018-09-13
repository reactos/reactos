//
// Functions in shell32 that did not exist in Win95, or which exist
// on NT differently from Win95.
//

#include "priv.h"
#include "unicpp\dutil.h"

const TCHAR c_szWallpaper[] = TEXT("Wallpaper");

#ifdef UNICODE
#error This file assumes that shell32 is ANSI.
#endif

// Note that the OLESTR gets freed, so don't try to use it later
BOOL WINAPI StrRetToStrN(LPSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl)
{
    switch (pStrRet->uType)
    {
    case STRRET_WSTR:
        SHUnicodeToAnsi(pStrRet->pOleStr, szOut, uszOut);
        SHFree(pStrRet->pOleStr);
        break;

    case STRRET_CSTR:
        lstrcpyn(szOut, pStrRet->cStr, uszOut);
        break;

    case STRRET_OFFSET:
        if (pidl)
        {
            ualstrcpyn(szOut, STRRET_OFFPTR(pidl,pStrRet), uszOut);
            break;
        }
        goto punt;

    default:
        ASSERT( FALSE && "Bad STRRET uType");
punt:
        if (uszOut)
        {
            *szOut = TEXT('\0');
        }
        return(FALSE);
    }

    return(TRUE);
}

//
// Win95 did not support the CSIDL_FLAG_CREATE flag, so we have to use
// the secret SHCloneSpecialIDList function.
//
STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwnd, LPTSTR pszPath, int nFolder, BOOL fCreate)
{
    LPITEMIDLIST pidl;

    *pszPath = 0;

    pidl = SHCloneSpecialIDList(hwnd, nFolder, fCreate);

    if (pidl) {
        BOOL fRet = SHGetPathFromIDList(pidl, pszPath);
        ILFree(pidl);
        return fRet;
    }
    return FALSE;
}

STDAPI_(BOOL) ILGetDisplayNameEx(IShellFolder *psfRoot, LPCITEMIDLIST pidl, LPTSTR pszName, int fType)
{
    STRRET srName;
    DWORD dwGDNFlags;

    if (!pszName)
        return FALSE;

    *pszName = 0;

    if (!pidl)
        return FALSE;

    // no root specified, get the desktop folder as the default
    if (!psfRoot)
    {
        // We never release it, but that's okay, because the shell never
        // destroys it.
        SHGetDesktopFolder(&psfRoot);
        ASSERT(psfRoot);
        if (psfRoot == NULL)
            return FALSE;
    }

    switch (fType)
    {
    case ILGDN_FULLNAME:
        dwGDNFlags = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;

SingleLevelPidl:
        if (SUCCEEDED(psfRoot->GetDisplayNameOf(pidl, dwGDNFlags, &srName)))
        {
            StrRetToStrN(pszName, MAX_PATH, &srName, pidl);
            return TRUE;
        }
        break;

    case ILGDN_INFOLDER:
    case ILGDN_ITEMONLY:
        dwGDNFlags = fType == ILGDN_INFOLDER ? SHGDN_INFOLDER : SHGDN_NORMAL;

        if (!ILIsEmpty(pidl))
        {
            LPCITEMIDLIST pidlLast = ILFindLastID(pidl);
            if (pidlLast != pidl)
            {
                LPITEMIDLIST pidlParent = ILClone(pidl);
                if (pidlParent)
                {
                    BOOL bRet = FALSE;
                    IShellFolder *psfParent;

                    ILRemoveLastID(pidlParent);   // strip to parent of item

                    if (SUCCEEDED(psfRoot->BindToObject(pidlParent, NULL, IID_IShellFolder, (LPVOID *)&psfParent)))
                    {
                        if (SUCCEEDED(psfParent->GetDisplayNameOf(pidlLast, dwGDNFlags, &srName)))
                        {
                            StrRetToStrN(pszName, MAX_PATH, &srName, pidlLast);
                            bRet = TRUE;
                        }
                        psfParent->Release();
                    }
                    ILFree(pidlParent);
                    return bRet;
                }
                return FALSE;       // out of memory
            }
        }
        goto SingleLevelPidl;
        break;
    }

    return FALSE;
}

STDAPI_(LPITEMIDLIST) ILCreateFromPathW(IN LPCWSTR pszPath)
{
    WCHAR wszPath[MAX_PATH];
    LPITEMIDLIST pidl = NULL;
    IShellFolder *psfRoot;
    HRESULT hres;
    ULONG cchEaten;

    // Sigh.  ParseDisplayName requires a writable input buffer.
    StrCpyNW(wszPath, pszPath, ARRAYSIZE(wszPath));

    // We never release it, but that's okay, because the shell never
    // destroys it.
    hres = SHGetDesktopFolder(&psfRoot);
    if (SUCCEEDED(hres)) {
        hres = psfRoot->ParseDisplayName(NULL, NULL, wszPath, &cchEaten, &pidl, NULL);
    }

    ASSERT(SUCCEEDED(hres) ? pidl != NULL : pidl == NULL);

    return pidl;

}
HRESULT Invoke_OnConnectionPointerContainer(IUnknown * punk, REFIID riidCP, DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT hr = S_OK;     // Assume no errors.
    IConnectionPointContainer * pcpc; 

    hr = punk->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pcpc);
    if (EVAL(SUCCEEDED(hr)))
    {
        IConnectionPoint * pcp;

        hr = pcpc->FindConnectionPoint(riidCP, &pcp);
        if (EVAL(SUCCEEDED(hr)))
        {
            IEnumConnections * pec;

            hr = pcp->EnumConnections(&pec);
            if (EVAL(SUCCEEDED(hr)))
            {
                CONNECTDATA cd;
                ULONG cFetched;

                while (S_OK == (hr = pec->Next(1, &cd, &cFetched)))
                {
                    LPDISPATCH pdisp;

                    ASSERT(1 == cFetched);
                    hr = cd.pUnk->QueryInterface(IID_IDispatch, (LPVOID *) &pdisp);
                    if (EVAL(SUCCEEDED(hr)))
                    {
                        DISPPARAMS dispparams = {0};
                        
                        if (!pdispparams)
                            pdispparams = &dispparams;

                        hr = pdisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
                        pdisp->Release();
                    }
                }
                pec->Release();
            }
            pcp->Release();
        }
        pcpc->Release();
    }

    return hr;
}

//
// helper function for pulling ITypeInfo out of our typelib
//
HRESULT Shell32GetTypeInfo(LCID lcid, UUID uuid, ITypeInfo **ppITypeInfo)
{
    HRESULT    hr;
    ITypeLib  *pITypeLib;

    // Just in case we can't find the type library anywhere
    *ppITypeInfo = NULL;

    /*
     * The type libraries are registered under 0 (neutral),
     * 7 (German), and 9 (English) with no specific sub-
     * language, which would make them 407 or 409 and such.
     * If you are sensitive to sub-languages, then use the
     * full LCID instead of just the LANGID as done here.
     */
    hr = LoadRegTypeLib(LIBID_Shell32, 1, 0, PRIMARYLANGID(lcid), &pITypeLib);

    /*
     * If LoadRegTypeLib fails, try loading directly with
     * LoadTypeLib, which will register the library for us.
     * Note that there's no default case here because the
     * prior switch will have filtered lcid already.
     *
     * NOTE:  You should prepend your DIR registry key to the
     * .TLB name so you don't depend on it being it the PATH.
     * This sample will be updated later to reflect this.
     */
    if (FAILED(hr))
    {
        OLECHAR wszPath[MAX_PATH];
#ifdef UNICODE
        GetModuleFileName(HINST_THISDLL, wszPath, ARRAYSIZE(wszPath));
#else
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath));
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));
#endif

        switch (PRIMARYLANGID(lcid))
        {
        case LANG_NEUTRAL:
        case LANG_ENGLISH:
            hr=LoadTypeLib(wszPath, &pITypeLib);
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        //Got the type lib, get type info for the interface we want
        hr=pITypeLib->GetTypeInfoOfGuid(uuid, ppITypeInfo);
        pITypeLib->Release();
    }

    return(hr);
}

STDAPI_(DWORD)
SHWNetGetConnection(LPCTSTR lpLocalName, LPTSTR lpRemoteName, LPDWORD lpnLength)
{
    TCHAR szLocalName[3];

    if (lpLocalName && lstrlen(lpLocalName) > 2)
    {
        // Kludge allert, don't pass c:\ to API, instead only pass C:
        szLocalName[0] = lpLocalName[0];
        szLocalName[1] = TEXT(':');
        szLocalName[2] = 0;
        lpLocalName = szLocalName;
    }

    return WNetGetConnection(lpLocalName, lpRemoteName, lpnLength);
}

void *  __cdecl operator new( unsigned int nSize )
{
    LPVOID pv; 

    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

    return pv;
}

void  __cdecl operator delete(void *pv)
{
    if (pv) {
#ifdef DEBUG
        memset(pv, 0xfe, LocalSize((HLOCAL)pv));
#endif
        LocalFree((HLOCAL)pv);
    }
}

// Comdlg32 stuff
HINSTANCE s_hmodComdlg32 = NULL;
PFNGETOPENFILENAME g_pfnGetOpenFileName = NULL;

void Comdlg32DLL_Term()
{
    if (s_hmodComdlg32) {
        FreeLibrary(s_hmodComdlg32);
        s_hmodComdlg32 = NULL;
        g_pfnGetOpenFileName = NULL;
    }
}

BOOL Comdlg32DLL_Init(void)
{
    if (NULL != s_hmodComdlg32)
        return(TRUE);       // already loaded.

    s_hmodComdlg32 = LoadLibrary(TEXT("comdlg32.dll"));
    if (!s_hmodComdlg32)
    {
        s_hmodComdlg32 = NULL;       // make sure it is NULL
        return(FALSE);
    }
#ifdef UNICODE
    g_pfnGetOpenFileName = (PFNGETOPENFILENAME)GetProcAddress(s_hmodComdlg32,
            "GetOpenFileNameW");
#else
    g_pfnGetOpenFileName = (PFNGETOPENFILENAME)GetProcAddress(s_hmodComdlg32,
            "GetOpenFileNameA");
#endif

    if (!g_pfnGetOpenFileName)
    {
        // BUGBUG: The next call to Comdlg32_Init will incorrectly return TRUE
        ASSERT(FALSE);
        Comdlg32DLL_Term();    // Free our usage of the DLL for now...
        return(FALSE);
    }
    return TRUE;
}

BOOL  APIENTRY GetOpenFileName(LPOPENFILENAME pofn)
{
    if (Comdlg32DLL_Init()) {
        return g_pfnGetOpenFileName(pofn);
    } else {
        return FALSE;
    }
}

#ifdef DEBUG
LRESULT WINAPI SendMessageD( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ASSERTNONCRITICAL;
#ifdef UNICODE
    return SendMessageW(hWnd, Msg, wParam, lParam);
#else
    return SendMessageA(hWnd, Msg, wParam, lParam);
#endif
}
#endif // DEBUG

#ifdef DEBUG
BOOL  g_bInDllEntry;
#endif


// Normally we'd just GetProcAddress from KERNEL32.  However,
// kernel prohibits anyone from getting a no-named export from
// itself.  So we've added a no-named export to SHELL32 which
// will call directly into GetProcessDword on the Win95 platform.

STDAPI_(DWORD)
GetProcessDword(DWORD idProcess, LONG iIndex)
{
    DWORD dwRet;

    if (g_fRunningOnNT) {
        dwRet = 0;
    } else {
        dwRet = SHGetProcessDword(idProcess, iIndex);
    }

    return dwRet;
}

// Copied from shell32, which does not export this.
// The fsmenu code needs this function.
STDAPI_(LPITEMIDLIST) _ILCreate(UINT cbSize)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbSize);
    if (pidl)
        memset(pidl, 0, cbSize);      // needed for external task allicator

    return pidl;
}

//===========================
// BUGBUG -- This is dup'd in shell32 and shdocvw, each one differently!
// Put a common version into shlwapi and call it a day.

#define     SZAPPNAME   TEXT("Explorer")
#define     SZDEFAULT   TEXT(".Default")
void IEPlaySound(LPCTSTR pszSound, BOOL fSysSound)
{
    TCHAR szKey[256];

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so that if they register
    // something we will play it
    wsprintf(szKey, TEXT("AppEvents\\Schemes\\Apps\\%s\\%s\\.current"), 
        (fSysSound ? SZDEFAULT : SZAPPNAME), pszSound);

    TCHAR szFileName[MAX_PATH];
    szFileName[0] = 0;
    LONG cbSize = SIZEOF(szFileName);

    // note the test for an empty string, PlaySound will play the Default Sound if we
    // give it a sound it cannot find...

    if ((RegQueryValue(HKEY_CURRENT_USER, szKey, szFileName, &cbSize) == ERROR_SUCCESS) 
        && cbSize && szFileName[0] != 0) 
    {
        //
        // Unlike SHPlaySound in shell32.dll, we get the registry value
        // above and pass it to PlaySound with SND_FILENAME instead of
        // SDN_APPLICATION, so that we play sound even if the application
        // is not Explroer.exe (such as IExplore.exe or WebBrowserOC).
        //
        PlaySound(szFileName, NULL, SND_FILENAME | SND_ASYNC);
    }
}

//===========================================================================
//
//  Functions that accept TCHAR in shell32, which means that we have to
//  convert from our TCHAR into whatever character set the shell is using.
//
//  WARNING!  We assume that all input string buffers are mo larger than
//  MAX_PATH!  And that all output buffers are exactly MAX_PATH in length!
//

//
//  "SCHAR" is the opposite of "TCHAR".  If we are ANSI, then SCHAR is
//  UNICODE, and vice versa.
//
//  "SHCHAR" is the character set the shell is using.
//
#ifdef UNICODE
typedef  CHAR SCHAR;
#define  SHTCharToSChar     SHTCharToAnsi
#define  SCharToTChar       SHAnsiToTChar
#define  ShellIsTChar()     g_fRunningOnNT
#else
typedef WCHAR SCHAR;
#define  SHTCharToSChar     SHTCharToUnicode
#define  SCharToTChar       SHUnicodeToTChar
#define  ShellIsTChar()     !g_fRunningOnNT
#endif

typedef union {
    SCHAR ssz[MAX_PATH];
    TCHAR tsz[1];
} STRINGCONVERT;

//
//  Convert a TCHAR to an SHCHAR.  Some APIs allow NULL, so we let those
//  go clean through.
//
//  We must cast away const-ness because we don't know whether our incoming
//  parameter was const or non-const.
//
LPTSTR TCharToShChar(LPCTSTR ptszSrc, STRINGCONVERT *psc)
{
    if (ShellIsTChar() || ptszSrc == NULL) {
        return (LPTSTR)ptszSrc;
    } else {
        SHTCharToSChar(ptszSrc, psc->ssz, ARRAYSIZE(psc->ssz));
        return psc->tsz;
    }
}

void ShCharToTChar(LPTSTR ptszSrc, STRINGCONVERT *psc)
{
    if (ShellIsTChar()) {
    } else {
        SCharToTChar(psc->ssz, ptszSrc, ARRAYSIZE(psc->ssz));
    }
}

// Quickie macro which means "Convert to/from the shell character set".
// C = Convert, DC = DeConvert
// Only one parameter can use this macro, since we have only one
// conversion buffer.
#define C(s)        TCharToShChar(s, &sc)
#define DC(s)       ShCharToTChar(s, &sc)

//
//  C2 and DC2 do the same thing to our second conversion buffer.
//  Similary for 3 and 4 (for APIs that have that many strings).

#define C2(s)       TCharToShChar(s, &sc2)
#define DC2(s)      ShCharToTChar(s, &sc2)
#define C3(s)       TCharToShChar(s, &sc3)
#define DC3(s)      ShCharToTChar(s, &sc3)
#define C4(s)       TCharToShChar(s, &sc4)
#define DC4(s)      ShCharToTChar(s, &sc4)

// Delay-load-like macro that does all the grunky work.
// CSETTHUNK assumes that the Shell32 name is already declared elsewhere.
// CSETTHUNKDECL will put out our own private declaration because nobody
// declared it yet.

#define CSETTHUNK(retval, fn, parg, arg) \
    STDAPI_(retval) _##fn parg           \
    {                                    \
        STRINGCONVERT sc;                \
        return fn arg;                   \
    }                                    \

#define CSETTHUNKDECL(retval, fn, parg, arg) \
    STDAPI_(retval)    fn parg;          \
    STDAPI_(retval) _##fn parg           \
    {                                    \
        STRINGCONVERT sc;                \
        return fn arg;                   \
    }                                    \

#undef Shell_GetCachedImageIndex
CSETTHUNK(
int, Shell_GetCachedImageIndex,
        (LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags),
        (      C(pszIconPath),    iIconIndex,      uIconFlags));

#undef Win32DeleteFile
CSETTHUNK(
BOOL, Win32DeleteFile,
        (LPCTSTR pszFile),
        (      C(pszFile)));


#undef IsLFNDrive
CSETTHUNKDECL(
BOOL, IsLFNDrive,
        (LPCTSTR pszPath),
        (      C(pszPath)));

#undef ILCreateFromPath
CSETTHUNKDECL(
LPITEMIDLIST, ILCreateFromPath,
        (LPCTSTR pszPath),
        (      C(pszPath)));

#undef SHSimpleIDListFromPath
CSETTHUNK(
LPITEMIDLIST, SHSimpleIDListFromPath,
        (LPCTSTR pszPath),
        (      C(pszPath)));

#undef SHRunControlPanel
CSETTHUNK(
int, SHRunControlPanel,
        (LPCTSTR pszOrig_cmdline, HWND errwnd),
        (      C(pszOrig_cmdline),     errwnd));

//
//  PathResolve is annoying because of the layers of strings it munges.
//
//  lpszPath is an INOUT parameter (hence gets a C and DC).
//  rgpszDirs[0] is an IN parameter (so it gets a C2 and no DC2).
//
#undef PathResolve
STDAPI_(BOOL) _PathResolve(LPTSTR lpszPath, LPCTSTR rgpszDirs[], UINT fFlags)
{
    STRINGCONVERT sc, sc2;
    BOOL fRc;

    // HACKHACK!
    // We assume dirs has only one element since the only case
    // this is called is in dde.cpp.  Right?

    if (rgpszDirs && rgpszDirs[0]) {
        rgpszDirs[0] = C2(rgpszDirs[0]); // overload the pointer to pass through...

        // None of our callers ask for more than one dir, right?
        if (EVAL(rgpszDirs[1] == 0)) {
            rgpszDirs[1] = NULL;
        }
    }

    fRc = PathResolve(C(lpszPath), rgpszDirs, fFlags);
    DC(lpszPath);                       // Convert it back on the way out

    return fRc;
}

//
// PathYetAnotherMakeUniqueName is annoying because, well look at all
// those strings!
//
#undef PathYetAnotherMakeUniqueName
STDAPI_(BOOL)
_PathYetAnotherMakeUniqueName(IN OUT LPTSTR  pszUniqueName,
                              IN     LPCTSTR pszPath,
                              IN     LPCTSTR pszShort,
                              IN     LPCTSTR pszFileSpec)
{
    STRINGCONVERT sc, sc2, sc3, sc4;
    BOOL fRc;

    fRc = PathYetAnotherMakeUniqueName(C(pszUniqueName),
                                      C2(pszPath),
                                      C3(pszShort),
                                      C4(pszFileSpec));

    DC(pszUniqueName);              // Unconvert the OUT parameter

    return fRc;
}

#undef PathQualify
STDAPI_(void) _PathQualify(IN OUT LPTSTR pszDir)
{
    STRINGCONVERT sc;
    PathQualify(C(pszDir));
    DC(pszDir);                     // Unconvert the OUT parameter
}
