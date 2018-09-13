/*****************************************************************************\
    FILE: ftpfoldr.h

    DESCRIPTION:
        This class inherits from CBaseFolder for a base ShellFolder implementation
    of IShellFolder and overrides methods to give Ftp Specific features.

     _UNDOCUMENTED_:  The shell violates Apartment model threading
     when doing background enumeration, so even though this DLL is
     marked as Apartment model, IShellFolder and IEnumIDList must
     be written with the free threading model with respect to anything
     that IEnumIDList can do in the background.
 
     This means that you'll see lots of ENTER_CRITICAL() and
     LEAVE_CRITICAL() calls when your brain would say, "I don't
     need to do that because I'm Apartment-model."  I'll try to
     point them out as they occur; look for the marker _MT_.
 
     CAUTION!  Internally, our property sheet handler also invokes
     methods on CFtpFolder on the wrong thread, so it's not just the
     shell that is weird.
\*****************************************************************************/

#include "priv.h"
#include "ftpfoldr.h"
#include "ftpurl.h"
#include "ftppidl.h"
#include "ftpicon.h"
#include "view.h"
#include "proxycache.h"
#include <idhidden.h>

#define FEATURE_SOFTLINK_SHORTCUT_ICONOVERLAY

// {A11501B3-6EA4-11d2-B679-006097DF5BD4}   Private to msieftp.dll
const GUID IID_CFtpFolder = { 0xa11501b3, 0x6ea4, 0x11d2, { 0xb6, 0x79, 0x0, 0x60, 0x97, 0xdf, 0x5b, 0xd4 } };


/*****************************************************************************
 *
 *    More const statics.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

WORD c_wZero = 0;        /* As promised in ftpview.h */

/*
 *  String separator used when building relative names.
 */
char c_szSlash[] = "/";


#pragma END_CONST_DATA


HRESULT CFtpFolder::_AddToUrlHistory(LPCWSTR pwzUrl)
{
    HRESULT hr = S_OK;

    if (!m_puhs)
        hr = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, IID_IUrlHistoryStg, (void **)&m_puhs);

    if (EVAL(m_puhs))
        EVAL(SUCCEEDED(hr = m_puhs->AddUrl(pwzUrl, pwzUrl, 0)));

    return hr;
}

/* Not yet needed
HRESULT CFtpFolder::AddToUrlHistory(LPCTSTR pszUrl)
{
    return _AddToUrlHistory(wzUrl);
}
*/

HRESULT CFtpFolder::AddToUrlHistory(LPCITEMIDLIST pidl)
{
    WCHAR wzUrl[MAX_URL_STRING];
    HRESULT hr = UrlCreateFromPidlW(pidl, SHGDN_FORPARSING, wzUrl, ARRAYSIZE(wzUrl), (ICU_ESCAPE | ICU_USERNAME), TRUE);

    // BUGBUG This is used across threads, so make it thread safe.    
    if (EVAL(SUCCEEDED(hr)))
        EVAL(SUCCEEDED(hr = _AddToUrlHistory(wzUrl)));

    return hr;
}


CWireEncoding * CFtpFolder::GetCWireEncoding(void)
{
    // GetFtpDir() may return NULL when we aren't rooted in an FTP server.
    CFtpDir * pfd = GetFtpDir();
    CWireEncoding * pwe = NULL;

    if (pfd)
    {
        pwe = pfd->GetFtpSite()->GetCWireEncoding();
        pfd->Release();
    }

    return pwe;
}

HRESULT CFtpFolder::_FixQuestionablePidl(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;

    // BUGBUG: Nuke comment if we can.
    // TODO: In the future, we may want to hit the server to
    //   disambiguate this.
/*
    BOOL fIsDir = TRUE;
    LPCSTR pszName = FtpPidl_GetLastItemName(pidl);

    // Can we get the name?
    if (EVAL(pszName))
    {
        // Is the file extension non-NULL? (Meaning it exists)
        if ('\0' != *PathFindExtensionA(pszName))
            fIsDir = FALSE;     // Yes, so asume it's a file.
    }

    hr = FtpPidl_SetFileItemType((LPITEMIDLIST) pidl, fIsDir);
*/
    return hr;
}


BOOL CFtpFolder::_IsServerVMS(LPCITEMIDLIST pidl)
{
    BOOL fIsServerVMS = FALSE;    // Assume TRUE
    CFtpSite * pfs;

    // Some caller's don't pass the Server ID so let's assume
    // that they already made it past that point.
    if (FtpID_IsServerItemID(pidl) &&
        EVAL(SUCCEEDED(SiteCache_PidlLookup(pidl, FALSE, m_pm, &pfs))))
    {
        fIsServerVMS = pfs->IsServerVMS();
        pfs->Release();
    }

    return fIsServerVMS;
}


/****************************************************\
    FUNCTION: _IsProxyBlockingSite

    DESCRIPTION:
        We need to detect if we cannot connect to the
    site because the proxy is a CERN or CERN type proxy
    that blocks ALL ftp access.  If this is true, we
    need to inform the user can fall all
    IShellFolder::BindToObject() calls.

        We will detect this case by doing the normal
    WININET FTP InternetConnect().  If that returns
    hr=0x80002EE7 (ERROR_INTERNET_NAME_NOT_RESOLVED)
    then it could either be that the name doesn't exist,
    or there is a CERN proxy blocking the call.  We will
    then try connect the CERN method which will tell us
    if it's the proxy that is blocking us.
\****************************************************/
BOOL CFtpFolder::_IsProxyBlockingSite(LPCITEMIDLIST pidl)
{
    BOOL fCacheResult;
    BOOL fResult = TRUE;    // Assume TRUE
    CFtpDir * pfd;

    if (ProxyCache_IsProxyBlocking(pidl, &fCacheResult))
        return fCacheResult;

    if (EVAL(pfd = GetFtpDirFromPidl(pidl)))
    {
        HRESULT hr = pfd->WithHint(NULL, NULL, NULL, NULL, NULL, this);

        // WithHint() often fails if a CERN style proxy blocks REAL wininet
        // access to the server.  If the server name is a DNS name, the error
        // returned will be ERROR_INTERNET_NAME_NOT_RESOLVED because that is
        // what is returned by the CERN proxy.  If the server name is an IP
        // Address, wininet will skip the CERN proxy and try to find it on the
        // intranet.  If not found (because it's past the firewall), then,
        // the attempt will timeout with ERROR_INTERNET_TIMEOUT.  We need
        // to treat this as a proxy block if and ONLY if the server name is an
        // IP address because DNS names can timeout for other reasons.  Us
        // treating IP server name timeouts as proxy blocks is going to have
        // to be tolerated because wininet won't handle this case.  It happens
        // very infrequently so I don't care that much.
        //
        // Some authentication proxies fail with: ERROR_INTERNET_CANNOT_CONNECT
        // We would like to fall back in that case, however, that may include
        // other cases like the server refusing to allow us in.
        // (password or too many logged in users?)
        // It would be great if ERROR_INTERNET_INVALID_PROXY_REQUEST or
        // ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP could be used.
        //
        if ((HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED) == hr) ||
            (HRESULT_FROM_WIN32(ERROR_INTERNET_CANNOT_CONNECT) == hr) ||
            ((HRESULT_FROM_WIN32(ERROR_INTERNET_TIMEOUT) == hr) && !FtpPidl_IsDNSServerName(pidl)))
        {
            TCHAR szUrl[MAX_URL_STRING];

            if (EVAL(SUCCEEDED(UrlCreateFromPidl(pidl, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), ICU_ESCAPE | ICU_USERNAME, FALSE))))
            {
                HINTERNET hintTemp;
                ASSERT(GetWininetSessionHandle());

                // For Web Proxies, InternetOpenUrl should work.  The problem is that
                // some (Netscape's) don't work.
                if (SUCCEEDED(InternetOpenUrlWrap(GetWininetSessionHandle(), TRUE, szUrl, NULL, 0, INTERNET_FLAG_NO_UI, NULL, &hintTemp)))
                {
                    InternetCloseHandle(hintTemp);  // This did work, so we must have a CERN proxy.
                }
                else
                    fResult = FALSE;    // We aren't blocked by the proxy. (Wrong IP Addr or Name?)
            }
        }
        else
            fResult = FALSE;    // We aren't blocked by the proxy.

        // Cache the result since finding out is so expensive.
        ProxyCache_SetProxyBlocking(pidl, fResult);
        pfd->Release();
    }

    return fResult;
}


/*****************************************************************************
 *
 *    InvalidateCache
 *
 *    Invalidate the pflHfpl cache in the corresponding FtpDir.
 *
 *    _MT_:  Note that the background enumerator calls this, so it must be
 *    multithread-safe.
 *
 *****************************************************************************/

void CFtpFolder::InvalidateCache(void)
{
    CFtpDir * pfd = GetFtpDir();

    if (EVAL(pfd))
    {
        // Should have created one on the GetHint()
        pfd->SetCache(0);
        pfd->Release();
    }
}


HRESULT CFtpFolder::_InitFtpSite(void)
{
    HRESULT hr = S_OK;

    if (!m_pfs)         // If we don't already got one...
    {
        ENTERCRITICAL;
        if (!m_pfs)            // Did it get created while we were waiting
        {
            if (EVAL(GetPrivatePidlReference()))
                hr = SiteCache_PidlLookup(GetPrivatePidlReference(), TRUE, m_pm, &m_pfs);
            else
            {
                // Not initialized
                TraceMsg(TF_FTPISF, "CFtpFolder_GetFtpDir(%08x) NOT INITED", this);
                hr = E_FAIL;
            }
        }

        LEAVECRITICAL;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetFtpDir

    DESCRIPTION:
        Say where our dir info is.

    We allocate the pfd only if somebody actually needs it, because
    Explorer does a lot of ILCompare's when you open a new folder,
    each of which creates a new IShellFolder for the sole purpose
    of calling CompareIDs.  We don't want to go through all the
    hubbub of creating an FtpDir and FtpSite when we don't need one.

    _MT_:  Note that the background enumerator calls this, so it must be
    multithread-safe.  In such case, however, the IShellFolder is
    marked cBusy, so we don't have to worry about the this->pfd
    getting wiped out behind our back by a change of identity.
\*****************************************************************************/
CFtpDir * CFtpFolder::GetFtpDir(void)
{
    HRESULT hres = S_OK;
    CFtpDir * pfd = NULL;

    _InitFtpSite(); // Okay if it fails.
    if (m_pfs)
        hres = m_pfs->GetFtpDir(GetPrivatePidlReference(), &pfd);

    return pfd;
}



CFtpDir * CFtpFolder::GetFtpDirFromPidl(LPCITEMIDLIST pidl)
{
    HRESULT hres = S_OK;
    CFtpDir * pfd = NULL;
    CFtpSite * pfs = NULL;

    hres = SiteCache_PidlLookup(pidl, FALSE, m_pm, &pfs);
    if (pfs)
    {
        hres = pfs->GetFtpDir(pidl, &pfd);
        pfs->Release();
    }

    return pfd;
}


CFtpDir * CFtpFolder::GetFtpDirFromUrl(LPCTSTR pszUrl)
{
    LPITEMIDLIST pidl;
    CFtpDir * pfd = NULL;

    if (EVAL(SUCCEEDED(CreateFtpPidlFromUrl(pszUrl, GetCWireEncoding(), NULL, &pidl, m_pm, FALSE))))
    {
        _InitFtpSite(); // Okay if it fails.
        m_pfs->GetFtpDir(pidl, &pfd);
        ILFree(pidl);
    }

    return pfd;
}


/*****************************************************************************\
 *    GetItemAllocator
 *
 *    Return today's pidl allocator.
\*****************************************************************************/
HRESULT CFtpFolder::GetItemAllocator(IMalloc **ppm)
{
    HRESULT hr = E_FAIL;

    *ppm = NULL;
    if (EVAL(m_pm))
    {
        IUnknown_Set(ppm, m_pm);
        hr = S_OK;
    }
    else
        TraceMsg(TF_FTPISF, "CFtpFolder_GetItemAllocator(%08x) NOT INITED", this);

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetUIObjectOfHfpl

    DESCRIPTION:
        _UNDOCUMENTED_:  Nowhere is there a list of interfaces
    that "should be" supported.  You just have to add lots of
    squirties and see what interfaces are asked for.

      _UNDOCUMENTED_:  Nowhere is it mentioned that passing
    cidl = 0 (or the various other weird variants) means to
    get a UI object on the folder itself.

    _UNDOCUMENTED_:  It is not mentioned whether the folder should
    be expected to handle cidl != 1 when asked for an IExtractIcon.
    I code defensively and handle the situation properly.

    IExtractIcon(0) extracts the icon for the folder itself.
    IExtractIcon(1) extracts the icon for the indicated pidl.
    IExtractIcon(n) extracts a generic "multi-document" icon.

    IContextMenu(0) produces a context menu for the folder itself.
        (Not used by the shell, but used by ourselves internally.)
    IContextMenu(n) produces a context menu for the multi-selection.

    IDataObject(0) ?? doesn't do anything
    IDataObject(n) produces a data object for the multi-selection.

    IDropTarget(0) produces a droptarget for the folder itself.
        (Not used by the shell, but used by ourselves internally.)
    IDropTarget(1) produces a droptarget for the single item.

    IShellView(0) ?? doesn't do anything
    IShellView(1) produces a shellview for the single item.
        (Nobody tries this yet, but I'm ready for it.)
\*****************************************************************************/
HRESULT CFtpFolder::GetUIObjectOfHfpl(HWND hwndOwner, CFtpPidlList * pflHfpl, REFIID riid, LPVOID * ppvObj, BOOL fFromCreateViewObject)
{
    HRESULT hr = E_INVALIDARG;

    if (IsEqualIID(riid, IID_IExtractIconA) ||
        IsEqualIID(riid, IID_IExtractIconW) ||
        IsEqualIID(riid, IID_IQueryInfo))
    {
        hr = CFtpIcon_Create(this, pflHfpl, riid, ppvObj);
        //TraceMsg(TF_FTPISF, "CFtpFolder::GetUIObjectOfHfpl() CFtpIcon_Create() hr=%#08lx", hr);
        ASSERT(SUCCEEDED(hr));
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        hr = CFtpMenu_Create(this, pflHfpl, hwndOwner, riid, ppvObj, fFromCreateViewObject);
        TraceMsg(TF_FTPISF, "CFtpFolder::GetUIObjectOfHfpl() CFtpMenu_Create() hr=%#08lx", hr);
        ASSERT(SUCCEEDED(hr));
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
        hr = CFtpObj_Create(this, pflHfpl, riid, ppvObj);
        TraceMsg(TF_FTPISF, "CFtpFolder::GetUIObjectOfHfpl() CFtpObj_Create() hr=%#08lx", hr);
        ASSERT(SUCCEEDED(hr));
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        // This will fail when someone gets a property sheet on an FTP PIDL Shortcut
        // that has a file as the destination.
        hr = CreateSubViewObject(hwndOwner, pflHfpl, riid, ppvObj);
        TraceMsg(TF_FTPISF, "CFtpFolder::GetUIObjectOfHfpl() CreateSubViewObject() hr=%#08lx", hr);
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
        ASSERT(0);  // Shouldn't happen
    }
    else if (IsEqualIID(riid, IID_IQueryAssociations))
    {
        IQueryAssociations * pqa;
        
        hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (void **)&pqa);
        if (SUCCEEDED(hr))
        {
            hr = pqa->Init(0, L"Folder", NULL, NULL);

            if (SUCCEEDED(hr))
                *ppvObj = (void *)pqa;
            else
                pqa->Release();
        }
    }
    else
    {
        //TraceMsg(TF_FTPISF, "CFtpFolder::GetUIObjectOfHfpl() E_NOINTERFACE");
        hr = E_NOINTERFACE;
    }

    if (FAILED(hr))
        *ppvObj = NULL;

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}


static const LPCTSTR pszBadAppArray[] = {TEXT("aol.exe"), TEXT("waol.exe"), TEXT("msnviewr.exe"), TEXT("cs3.exe"), TEXT("msdev.exe")};

/*****************************************************************************\
    FUNCTION: IsAppFTPCompatible

    DESCRIPTION:
        Some apps (WebOC hosts) fail to navigate to FTP directories.
    We check the app here and see if it's one of those incompatible apps.

    I don't worry about perf because we can do the work only once and cache
    the result because our globals will be re-inited for each process.

    GOOD:
    ========================================================================
    iexplore.exe:   Good of course.
    explorer.exe:   Good of course.
    msdev.exe (v6): The HTML help works but folder navigations happen in
                    a new window.  I don't care because the same happens in
                    the shell (File System case).
    <Default Case>: These are apps built with VB's WebOC that work fine, but
                    they also have the open in new folder behavior.

    BAD and UGLY:
    ========================================================================
    msdev.exe (v5): You can navigate their Moniker help to FTP which will
                    cause a hang.
    [MSN] (msnviewr.exe): For some reason MSN calls IPersistFolder::Initialize with an invalid value.
           Navigating to the folder works but launching other folders cause them
           to appear in their own window and they immediately close.  This was
           on browser only so it may be because internet delegate folders aren't
           supported.

    [aol]: (waol.exe) This doesn't work either.
    cs3.exe (CompuServ): ????
    [ATT WorldNet]: ????
    [Protigy]: ????
    [SNAP]: ????
\*****************************************************************************/
BOOL IsAppFTPCompatible(void)
{
    static BOOL s_fIsAppCompatible;
    static BOOL s_fIsResultCached = FALSE;
//
    if (!s_fIsResultCached)
    {
        TCHAR szAppPath[MAX_PATH];

        s_fIsAppCompatible = TRUE;  // Assume all Web OC Hosts are fine...

        if (EVAL(GetModuleFileName(NULL, szAppPath, ARRAYSIZE(szAppPath))))
        {
            int nIndex;
            LPTSTR pszAppFileName = PathFindFileName(szAppPath);

            for (nIndex = 0; nIndex < ARRAYSIZE(pszBadAppArray); nIndex++)
            {
                if (!StrCmpI(pszAppFileName, pszBadAppArray[nIndex]))
                {
                    // This one is bad/
                    s_fIsAppCompatible = FALSE;
                    break;
                }
            }
        }

        s_fIsResultCached = TRUE;
    }

    return s_fIsAppCompatible;
}


/*****************************************************************************\
    FUNCTION: CreateSubViewObject

    DESCRIPTION:
        Somebody is asking for a UI object of a subobject, which is
    better handled by the subobject than by the parent.

    Bind to the subobject and get the requested UI object thence.

    If the pidl list is empty, then we are talking about ourselves again.
\*****************************************************************************/
HRESULT CFtpFolder::CreateSubViewObject(HWND hwndOwner, CFtpPidlList * pflHfpl, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;
    DWORD dwItemsSelected = pflHfpl->GetCount();
    IShellFolder * psf = NULL;

    if (EVAL(ppvObj))             // I wouldn't be surprised if
        *ppvObj = NULL;            // somebody relied on this

    if (1 == dwItemsSelected)
    {
        LPITEMIDLIST pidl = pflHfpl->GetPidl(0);    // This doesn't clone the pidl so we don't need to free it.
        if (EVAL(pidl))
            hr = BindToObject(pidl, 0, IID_IShellFolder, (LPVOID *)&psf);
    }
    else if (EVAL(0 == dwItemsSelected))
        hr = this->QueryInterface(IID_IShellFolder, (void **) &psf);

    ASSERT_POINTER_MATCHES_HRESULT(psf, hr);
    if (EVAL(SUCCEEDED(hr)))
    {
        // CreateViewObject will AddRef the psfT if it wants it
        hr = psf->CreateViewObject(hwndOwner, riid, ppvObj);
    }
    
    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    ATOMICRELEASE(psf);
    return  hr;
}



/*****************************************************************************\
      GetSiteMotd
\*****************************************************************************/

CFtpGlob * CFtpFolder::GetSiteMotd(void)
{
    CFtpGlob * pGlob = NULL;

    _InitFtpSite(); // Okay if it fails.
    if (m_pfs)
        pGlob = m_pfs->GetMotd();

    return pGlob;
}


HRESULT CFtpFolder::_Initialize(LPCITEMIDLIST pidlTarget, LPCITEMIDLIST pidlRoot, int nBytesToPrivate)
{
    IUnknown_Set(&m_pfs, NULL);
    return CBaseFolder::_Initialize(pidlTarget, pidlRoot, nBytesToPrivate);
}


// Sometimes the user will enter incorrect information without knowing.
// We would catch this if we verified everything that was entered, but
// we don't, we just take it on faith until we do the IEnumIDList.
// This is great for perf but sucks for catching these kinds of things.
// An example of this is the user using the File.Open dialog and going to
// "ftp://myserver/dir/".  They then enter "ftp://myserver/dir/file.txt"
// which will try to parse relative but it's an absolute path.
HRESULT CFtpFolder::_FilterBadInput(LPCTSTR pszUrl, LPITEMIDLIST * ppidl)
{
    HRESULT hr = S_OK;

    // If pidlPrivate isn't empty, then we aren't at the
    // root, so reject any urls that are absolute (i.e. have
    // ftp: scheme).
    if (!IsRoot() && (URL_SCHEME_FTP == GetUrlScheme(pszUrl)))
        hr = E_FAIL;
    // More may come here...

    if (FAILED(hr) && *ppidl)
        Pidl_Set(ppidl, NULL);

    return hr;
}


/*****************************************************************************\
    FUNCTION: _ForPopulateAndEnum

    DESCRIPTION:
        This function exists to detect the following case and if it's true,
    populate the cache (pfd) and return the pidl from that cache in ppidl.

     There is one last thing we need to try, we need to detect if:
    1) the URL has an URL path, and 
    2) the last item in the path doesn't have an extension and doesn't
       end in a slash ('/') to indicate it's a directory.
    If this case is true, we then need to find out if it is a directory
    or file by hitting the server.  This is needed because by the time
    we bind, it's too late to fall back to the other thing (IEnumIDList).
    The one thing we might need to be careful about is AutoComplete because
    they may call :: ParseDisplayName() for every character a user types.
    This won't be so bad because it's on a background thread, asynch, and
    the first enum within a segment will cause the cache to be populated
    within a that segment so subsequent enums will be fast.  The problem
    it that it's not uncommon for users to enter between 2 and 5 segments,
    and there would be 1 enum per segment.
\*****************************************************************************/
HRESULT CFtpFolder::_ForPopulateAndEnum(CFtpDir * pfd, LPCITEMIDLIST pidlBaseDir, LPCTSTR pszUrl, LPCWIRESTR pwLastDir, LPITEMIDLIST * ppidl)
{
    HRESULT hr = E_FAIL;

    *ppidl = NULL;
    // We only care if the URL Path isn't empty AND it doesn't end in a '/' AND
    // it doesn't have an extension.
    if (!ILIsEmpty(pfd->GetPathPidlReference()) && (0 == *PathFindExtensionA(pwLastDir)))
    {
        IEnumIDList * penumIDList;

        // NULL hwnd needs to suppress all UI.
        hr = CFtpEidl_Create(pfd, this, NULL, (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN), &penumIDList);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = penumIDList->Reset();
            ASSERT(SUCCEEDED(hr));
            // We are working off of the assumption that calling Reset will force it to hit the server and pull down all of the contents.

            LPITEMIDLIST pidlFromCache = (LPITEMIDLIST) pfd->GetPidlFromWireName(pwLastDir);
            if (pidlFromCache)
            {
                // It was found, this means that it exists now in the cache after we
                // forced it to be populated.
                *ppidl = ILCombine(pidlBaseDir, pidlFromCache);
                ILFree(pidlFromCache);
            }
            else
                hr = E_FAIL;

            penumIDList->Release();
        }
    }

    return hr;
}


HRESULT CFtpFolder::_GetCachedPidlFromDisplayName(LPCTSTR pszDisplayName, LPITEMIDLIST * ppidl)
{
    HRESULT hr = E_FAIL;
    if (ppidl)
    {
        CFtpDir * pfd = GetFtpDir();

        if (pfd)
        {
            // We may have a pointer but the cache may still be empty, as in case NT #353324
            CFtpPidlList * pfl = pfd->GetHfpl();
            if (pfl)
            {
                // Yes, so we will continue to use the cache.  Now let's get rid of that
                // temp pointer.
                pfl->Release();
            }
            else
            {
                // No we don't have it cashed, so pretend the pfd was returned NULL.
                pfd->Release();
                pfd = NULL;
            }
        }

        *ppidl = NULL;
        if (!pfd)
        {
            LPITEMIDLIST pidlBaseDir;

            hr = CreateFtpPidlFromUrl(pszDisplayName, GetCWireEncoding(), NULL, &pidlBaseDir, m_pm, FALSE);
            if (SUCCEEDED(hr))  // May fail because of AutoComplete.
            {
                // If it's not pointing to just a server, then we can enum the contents and
                // find out if it's is a file or directory.
                if (!ILIsEmpty(pidlBaseDir) && !FtpID_IsServerItemID(ILFindLastID(pidlBaseDir)))
                {
                    CFtpSite * pfs;
            
                    hr = SiteCache_PidlLookup(pidlBaseDir, TRUE, m_pm, &pfs);
                    if (EVAL(SUCCEEDED(hr)))
                    {
                        LPCWIRESTR pwLastDirName;

                        // If we are using a hidden password, then ::GetDisplayNameOf() hands out
                        // these "ftp://user@server/dir/" URLs and the password is hidden.  If
                        // :: ParseDisplayName() is given one of these URLs and we are currently in
                        // that server w/that user name, then :: ParseDisplayNameOf() needs to hand
                        // out a pidl with the correct hidden password cookie.
                        //
                        // Is pidlNav the same as GetPublicRootPidlReference() except pidlNav doesn't
                        // have a password.  The same means that the servers match, and the user names
                        // match.
                        EVAL(SUCCEEDED(pfs->UpdateHiddenPassword(pidlBaseDir)));

                        // This is sneaky because pwLastDirName will point into them itemID
                        // that will be removed.  The memory won't really be removed, it will
                        // just have the size set to zero.
                        pwLastDirName = FtpPidl_GetLastItemWireName(pidlBaseDir);

                        ILRemoveLastID(pidlBaseDir);
                        pfs->GetFtpDir(pidlBaseDir, &pfd);

                        if (pfd)
                        {
                            LPITEMIDLIST pidlFromCache = (LPITEMIDLIST) pfd->GetPidlFromWireName(pwLastDirName);
                            if (pidlFromCache)
                            {
                                // It was found, this means we were probably in ftp://serverX/Dir1/
                                // and the user entered something from that directory or another directory
                                // taht we have alread displayed to the user and it's in our cache.
                                *ppidl = ILCombine(pidlBaseDir, pidlFromCache);
                                ILFree(pidlFromCache);
                                hr = S_OK;
                            }
                            else
                            {
                                // There is one last thing we need to try, we need to detect if:
                                // 1) the URL has an URL path, and 
                                // 2) the last item in the path doesn't have an extension and doesn't
                                //    end in a slash ('/') to indicate it's a directory.
                                // If this case is true, we then need to find out if it is a directory
                                // or file by hitting the server.  This is needed because by the time
                                // we bind, it's too late to fall back to the other thing (IEnumIDList).
                                // The one thing we might need to be careful about is AutoComplete because
                                // they may call :: ParseDisplayName() for every character a user types.
                                // This won't be so bad because it's on a background thread, asynch, and
                                // the first enum within a segment will cause the cache to be populated
                                // within a that segment so subsequent enums will be fast.  The problem
                                // it that it's not uncommon for users to enter between 2 and 5 segments,
                                // and there would be 1 enum per segment.
                                hr = _ForPopulateAndEnum(pfd, pidlBaseDir, pszDisplayName, pwLastDirName, ppidl);
                            }


                            pfd->Release();
                        }
                        else
                            hr = E_FAIL;

                        pfs->Release();
                    }
                    else
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;

                ILFree(pidlBaseDir);
            }
        }
        else
        {
            //    Create a new enumeration object for the caller.
            // PERF: log 2 (sizeof(m_pflHfpl))
            *ppidl = (LPITEMIDLIST) pfd->GetPidlFromDisplayName(pszDisplayName);
            if (*ppidl)
            {
                hr = S_OK;
            }
            else
            {
                // If we got here, the cache for this directory is populated.
                // So if the name doesn't match, then either:
                // 1) it doesn't exist,
                // 2) the cache is out of date, or
                // 3) it's multilevel, (like "dir1\dir2\dir3") or
                // 4) It's a weird parsing token that our parent parse should have remoted, like "..", ".", "\", etc.
                // We will assome our parent parse takes care of #4, and #2 isn't true.

                // Is this multilevel? (Case #3)
                if (!StrChr(pszDisplayName, TEXT('/')))
                {
                    // No, so reject it and don't let our caller blindly accept it.
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                }
            }

            pfd->Release();
        }
    }

    return hr;
}


HRESULT CFtpFolder::_GetBindCtx(IBindCtx ** ppbc)
{
    HRESULT hr = CreateBindCtx(NULL, ppbc);

    if (SUCCEEDED(hr))	// Can fail with out of memory
    {
        hr = (*ppbc)->RegisterObjectParam(STR_SKIP_BINDING_CLSID, SAFECAST(this, IShellIcon *));    // We want IUnknown, not IShellIcon, but this is to disambigiuate.
    }

    return hr;
}


HRESULT CFtpFolder::_GetLegacyURL(LPCITEMIDLIST pidl, IBindCtx * pbc, LPTSTR pszUrl, DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlWithVRoot;

    // We now need to insert the virtual root path into the path section
    // of the URL because the old FTP support doesn't follow the correct
    // FTP URL spec that says that the virtual root needs to be left out
    // of the URL.
    hr = _ConvertPidlForRootedFix(pidl, &pidlWithVRoot);
    if (SUCCEEDED(hr))
    {
        WCHAR wzFrag[MAX_PATH];

        // SECURITY ISSUE: We need to get the URL w/password or it won't work, but
        //                 this will expose the password publicly.  We need a way for
        //                 the real FTP URL Pidl to hide the password.
        hr = UrlCreateFromPidlW(pidlWithVRoot, SHGDN_FORPARSING, pszUrl, cchSize, (ICU_ESCAPE | ICU_USERNAME), FALSE);
        if (ILGetHiddenStringW(pidl, IDLHID_URLFRAGMENT, wzFrag, ARRAYSIZE(wzFrag)))  // Add fragment if it exists.
            UrlCombineW(pszUrl, wzFrag, pszUrl, &cchSize, 0);

        ILFree(pidlWithVRoot);
    }

    return hr;
}


HRESULT CFtpFolder::_GetLegacyPidl(LPCITEMIDLIST pidl, LPITEMIDLIST * ppidlLegacy)
{
    IBindCtx * pbc = NULL;
    HRESULT hr = _GetBindCtx(&pbc);

    *ppidlLegacy = NULL;
    if (SUCCEEDED(hr))   // Can fail with out of memory.
    {
        WCHAR wzUrl[MAX_URL_STRING];

        hr = _GetLegacyURL(pidl, pbc, wzUrl, ARRAYSIZE(wzUrl));
        if (EVAL(SUCCEEDED(hr)))
        {
            TraceMsg(TF_FTPISF, "_BindToObject_OriginalFtpSupport() navigating to=%ls", wzUrl);
            hr = IEParseDisplayNameWithBCW(CP_ACP, wzUrl, pbc, ppidlLegacy);
        }

        pbc->Release();
    }

    return hr;
}


HRESULT CFtpFolder::_InitLegacyShellFolder(IShellFolder * psfLegacy, LPCITEMIDLIST pidlInit)
{
    IPersistFolder * ppf;
    HRESULT hr = psfLegacy->QueryInterface(IID_IPersistFolder, (void **) &ppf);

    if (SUCCEEDED(hr))
    {
        hr = ppf->Initialize(pidlInit);
        ppf->Release();
    }

    return hr;
}


HRESULT CFtpFolder::_INetBindToObject(LPCITEMIDLIST pidl, IBindCtx * pbc, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_OUTOFMEMORY;

    LPITEMIDLIST pidlFirst = GetPublicPidlRootIDClone();
    if (pidlFirst)
    {
        IShellFolder * psfInternetSF;

        hr = IEBindToObject(pidlFirst, &psfInternetSF);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = _InitLegacyShellFolder(psfInternetSF, pidlFirst);
            if (SUCCEEDED(hr))
            {
                // Note the I use ILNext() in order to skip past the Desktop ItemID,
                // which is internal knowledge I should not have.
                hr = psfInternetSF->BindToObject(_ILNext(pidl), pbc, riid, ppvObj);
            }

            psfInternetSF->Release();
        }

        ILFree(pidlFirst);
    }

    return hr;
}



HRESULT CFtpFolder::_BindToObject_OriginalFtpSupport(LPCITEMIDLIST pidl, REFIID riid, LPVOID * ppvObj)
{
    LPBC pbc = NULL;
    HRESULT hr = CreateBindCtx(NULL, &pbc);
    
    if (EVAL(SUCCEEDED(hr)))
    {
        hr = pbc->RegisterObjectParam(STR_SKIP_BINDING_CLSID, SAFECAST(this, IShellIcon *));    // We want IUnknown, not IShellIcon, but this is to disambigiuate.
        if (EVAL(SUCCEEDED(hr)))
        {
            LPITEMIDLIST pidlLegacy;

            hr = _GetLegacyPidl(pidl, &pidlLegacy);
            if (EVAL(SUCCEEDED(hr)))
            {
                hr = _INetBindToObject(pidlLegacy, pbc, riid, ppvObj);
                ILFree(pidlLegacy);
            }
        }

        pbc->Release();
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION:   _IsValidPidlParameter

    DESCRIPTION:
        If this IShellFolder is rooted within our name space, then the pidl needs
    to be a valid relative pidl.  If we are rooted at the base of our name space,
    then it needs to be a full pidl.
\*****************************************************************************/
BOOL CFtpFolder::_IsValidPidlParameter(LPCITEMIDLIST pidl)
{
    BOOL fResult = TRUE;

    if (IsRoot())
        fResult = FtpPidl_IsValidFull(pidl);
    else
        fResult = FtpPidl_IsValidRelative(pidl);

    return fResult;
}


/*****************************************************************************\
    FUNCTION: IShellFolder::_BindToObject

    DESCRIPTION:
        We are now sure that we want to handle the support, so check what they
    want.
\*****************************************************************************/
HRESULT CFtpFolder::_BindToObject(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlFull, IBindCtx * pbc, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // Indicate we want the old functionality to kick in.

    if (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IShellFolder2))
    {
        LPITEMIDLIST pidlTarget = ILCombine(GetPublicTargetPidlReference(), pidl);
        LPITEMIDLIST pidlRoot = (GetFolderPidl() ? ILCombine(GetFolderPidl(), pidl) : NULL);

        //  There's no point trying to verify that it's folders all
        //  the way down, because it's the caller's job not to combine
        //  pidls randomly.  Furthermore, they might not actually be marked
        //  as folders if we got them via ParseDisplayName.

        // NOTE: Binding will succeed even if the pidl isn't valid on the
        //       server.  In the future we may want to verify now so we
        //       don't hand out a IEnumIDList that won't work.  Currently,
        //       IEnumIDList will fail and cause a renavigation if it can
        //       connect to the server in a different way (different username
        //       password pair).  It would be better to do a redirect because
        //       the renavigation causes the bad entry in the navigation stack.
        //       We can't verify the item exists on the server if we have a WebProxy
        //       installed.

        hr = CFtpFolder_Create(pidlTarget, pidlRoot, GetPidlByteOffset(), riid, ppvObj);
        //TraceMsg(TF_FOLDER_SHRTCUTS, "CFtpFolder::_BindToObject() creating an FTP IShellFolder psf=%#08lx, pidlTarget=%#08lx, pidlRoot=%#08lx", *ppvObj, pidlTarget, pidlRoot);
        if (EVAL(SUCCEEDED(hr)))
        {
            IUnknown * punk = (IUnknown *) *ppvObj;
            IDelegateFolder * pdf;

            hr = punk->QueryInterface(IID_IDelegateFolder, (LPVOID *) &pdf);
            if (EVAL(SUCCEEDED(hr)))
            {
                if (EVAL(SUCCEEDED(hr)))
                    hr = pdf->SetItemAlloc(m_pm);
                pdf->Release();
            }
        }

        ILFree(pidlTarget);
        ILFree(pidlRoot);
        //TraceMsg(TF_FTPISF, "CFtpFolder::BindToObject() IID_IShellFolder hr=%#08lx", hr);
    }
    else if (IsEqualIID(riid, IID_IMoniker))
    {
        hr = _PidlToMoniker(pidlFull, (IMoniker **) ppvObj);
    }
    else if (IsEqualIID(riid, IID_CFtpFolder))
    {
        IShellFolder * psf;

        // Nothing like a little recursion to keep the code clean.
        // The fact that we use IID_IShellFolder guarantees the breaking
        // of the recursion.
        hr = BindToObject(pidl, pbc, IID_IShellFolder, (void **) &psf);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = psf->QueryInterface(riid, ppvObj);
            psf->Release();
        }
    }
    else
    {
        TraceMsg(TF_FTPISF, "CFtpFolder::BindToObject() unsupported interface hr=E_NOINTERFACE");
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}


/*****************************************************************************\
     FUNCTION: _ConvertPidlForRootedFix
 
    DESCRIPTION:
        If an FTP URL has a login name, that login may root the user under a directory other than "/".
    The FTP URL spec (RFC 1738) says that URL paths need to be relative to the rooted directory.  For example:
    If UserA's rooted account is in \usr\GroupA\UserA and the url is:
    ftp://UserA:FooBar@server/test/file.txt, then the real path is \usr\GroupA\UserA\test\file.txt.
    The problem is that the old FTP code doesn't respect this and requires:
    ftp://UserA:FooBar@server/usr/GroupA/UserA/test/file.txt, so we fix that here.
 
    PARAMETERS:
        pidlBefore [IN]: This will be a public pidl to the item to navigate to.
            This means it will be: [TheINet][FtpServerID][...]
        *ppidlWithVRoot [OUT]: This will be the same public pidl that was passed in except
            any ItemIDs that come from pfs->GetVirtualRootReference() will be inserted
            between the ServerID and ItemIDs.
\*****************************************************************************/
HRESULT CFtpFolder::_ConvertPidlForRootedFix(LPCITEMIDLIST pidlBefore, LPITEMIDLIST * ppidlWithVRoot)
{
    CFtpSite * pfs;
    HRESULT hr = SiteCache_PidlLookup(pidlBefore, FALSE, m_pm, &pfs);

    *ppidlWithVRoot = NULL;
    if (EVAL(SUCCEEDED(hr) && pfs))
    {
        if (pfs->HasVirtualRoot())
        {
            LPCITEMIDLIST pidlVirtualRoot = pfs->GetVirtualRootReference();
            LPITEMIDLIST pidlUrlPath = (LPITEMIDLIST)pidlBefore;

            // Skip past non-FTP Server/ItemIDs. (TheInternet)
            while (pidlUrlPath && !ILIsEmpty(pidlUrlPath) && !FtpID_IsServerItemID(pidlUrlPath))
                pidlUrlPath = _ILNext(pidlUrlPath);

            if (FtpID_IsServerItemID(pidlUrlPath))
                pidlUrlPath = _ILNext(pidlUrlPath);

            if (EVAL(pidlUrlPath))
            {
                LPITEMIDLIST pidlFullWithVRoot;
                USHORT cb = pidlUrlPath->mkid.cb;

                pidlUrlPath->mkid.cb = 0;
                pidlFullWithVRoot = ILCombine(pidlBefore, pidlVirtualRoot);
                pidlUrlPath->mkid.cb = cb;

                if (EVAL(pidlFullWithVRoot))
                {
                    FtpPidl_InsertVirtualRoot(pidlFullWithVRoot, pidlUrlPath, ppidlWithVRoot);
                    ILFree(pidlFullWithVRoot);
                }
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_FAIL;

        pfs->Release();
    }

    if (FAILED(hr))
    {
        *ppidlWithVRoot = ILClone(pidlBefore);
        if (*ppidlWithVRoot)
            hr = S_OK;
    }

    return hr;
}


BOOL CFtpFolder::IsUTF8Supported(void)
{
    if (EVAL(m_pfs))
        return m_pfs->IsUTF8Supported();

    return FALSE;
}

/*****************************************************************************\
     FUNCTION: IShellFolder::_PidlToMoniker
 
    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpFolder::_PidlToMoniker(LPCITEMIDLIST pidl, IMoniker ** ppmk)
{
    HRESULT hr = E_INVALIDARG;

    *ppmk = NULL;
    if (EVAL(pidl))
    {
        IBindCtx * pbc;

        hr = _GetBindCtx(&pbc);
        if (SUCCEEDED(hr))
        {
            WCHAR wzUrl[MAX_URL_STRING];

            // URLMON expects incorrectly formatted URLs (where the virtual
            //        root is included in the url path).  We need to fix that
            //        here.
            hr = _GetLegacyURL(pidl, pbc, wzUrl, ARRAYSIZE(wzUrl));
            if (SUCCEEDED(hr))
            {
                hr = CreateURLMoniker(NULL, wzUrl, ppmk);
            }

            pbc->Release();
        }
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppmk, hr);
    return hr;
}


HRESULT CFtpFolder::_CreateShellView(HWND hwndOwner, void ** ppvObj)
{
    IShellFolderViewCB * psfvCallBack;

    HRESULT hr = CFtpView_Create(this, hwndOwner, IID_IShellFolderViewCB, (LPVOID *) &psfvCallBack);
    if (EVAL(SUCCEEDED(hr)))
    {
        // GetPublicTargetPidlReference() is used because it's passed to SFVM_GETNOTIFY
        // to synch ChangeNotify messages.
        hr = CBaseFolder::_CreateShellView(hwndOwner, ppvObj, FTP_SHCNE_EVENTS, 
                        FVM_DETAILS, psfvCallBack, GetPublicTargetPidlReference(), CBaseFolderViewCB::_IShellFolderViewCallBack);
        psfvCallBack->Release();
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}

HKEY ClassKeyFromExtension(LPCWIRESTR pszExt)
{
    HKEY hkey = NULL;
    WIRECHAR szProgID[MAX_PATH];
    DWORD cbProgID = sizeof(szProgID);

    if (ERROR_SUCCESS == SHGetValueA(HKEY_CLASSES_ROOT, pszExt, NULL, NULL, (void *)szProgID, &cbProgID))
    {
        // the entension points to a ProgID, use that.
        RegOpenKeyA(HKEY_CLASSES_ROOT, szProgID, &hkey);
    }
    else
    {
        // No ProgID, use the extension as the program ID.
        RegOpenKeyA(HKEY_CLASSES_ROOT, pszExt, &hkey);
    }

    return hkey;
}

#define SZ_REGVALUE_DOCOBJECT            TEXT("DocObject")
#define SZ_REGVALUE_BROWSEINPLACE        TEXT("BrowseInPlace")

BOOL _IsDocObjViewerInstalled(LPCITEMIDLIST pidl)
{
    BOOL fResult = FALSE;

    // Return FALSE if it's just pointing to an FTP server.
    if (!FtpID_IsServerItemID(ILFindLastID(pidl)))
    {
        LPCWIRESTR pwWireFileName = FtpPidl_GetLastItemWireName(pidl);
        LPCWIRESTR pszExt = PathFindExtensionA(pwWireFileName);

        if (pszExt)
        {
            HKEY hkey = ClassKeyFromExtension(pszExt);
            if (hkey)
            {
                if ((ERROR_SUCCESS == RegQueryValue(hkey, SZ_REGVALUE_DOCOBJECT, 0, NULL)) ||
                    (ERROR_SUCCESS == RegQueryValue(hkey, SZ_REGVALUE_BROWSEINPLACE, 0, NULL)))
                {
                    fResult = TRUE;
                }

                RegCloseKey(hkey);
            }
        }
    }

    return fResult;
}


ULONG FtpGetAttributesOf(LPCITEMIDLIST pidl)
{
    ASSERT(IsValidPIDL(pidl));

    DWORD dwAttributes = FtpPidl_GetAttributes(pidl);   // Get File based attributes.
    ULONG rgfInOut = Misc_SfgaoFromFileAttributes(dwAttributes); // Turn them into IShellFolder attributes.
    return rgfInOut;
}





//===========================
// *** IShellFolder2 Interface ***
//===========================

STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb)
{
    HRESULT hres;
    SAFEARRAY *psa = SafeArrayCreateVector(VT_UI1, 0, cb);   // create a one-dimensional safe array
    if (psa) 
    {
        memcpy(psa->pvData, pv, cb);

        memset(pvar, 0, sizeof(*pvar));  // VariantInit()
        pvar->vt = VT_ARRAY | VT_UI1;
        pvar->parray = psa;
        hres = S_OK;
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}


/*****************************************************************************\
    FUNCTION: IShellFolder2::GetDetailsEx

    DESCRIPTION:
        This function will be called when the caller wants detailed info about
    and item.  SHGetDataFromIDList() is one such caller and that is commonly
    called by the Shell Object model when using CSDFldrItem::get_Size(LONG *pul)
    and other such APIs.
\*****************************************************************************/
HRESULT CFtpFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = S_OK;

    if (IsEqualGUID(pscid->fmtid, FMTID_ShellDetails) && (PID_FINDDATA == pscid->pid))
    {
        WIN32_FIND_DATAW wfd;

        // I can handle this.
        LPITEMIDLIST pidlFull = CreateFullPrivatePidl(pidl);

        if (pidlFull)
        {
            hr = Win32FindDataFromPidl(pidlFull, &wfd, TRUE, TRUE);
            ILFree(pidlFull);
        }

        if (SUCCEEDED(hr))
        {
            hr = InitVariantFromBuffer(pv, (PVOID)&wfd, sizeof(wfd));
        }
    }
    else
        hr = CBaseFolder::GetDetailsEx(pidl, pscid, pv);

    return hr;
}



//===========================
// *** IShellFolder Interface ***
//===========================

/*****************************************************************************\
    FUNCTION: IShellFolder:: ParseDisplayName

    DESCRIPTION:
        The incoming name is %-encoded, but if we see an illegal %-sequence,
    just leave the % alone.

    For now, we disallow backslash, "*" and "?" from filenames.
    Backslashes don't sit well with wininet, and wildcards
    screw up the "quick FindFirst to see if the file exists".

    We also disallow encoded slashes, because they screw up the way
    we manage subpidls.

    Annoying feature: You can't pass -1 as the output buffer size.
    NLS returns ERROR_INVALID_PARAMETER if you try.  So you have to pass
    the actual size.  Sigh.
\*****************************************************************************/
HRESULT CFtpFolder::ParseDisplayName(HWND hwnd, LPBC pbcReserved, LPOLESTR pwszDisplayName,
                        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = S_OK;

    *ppidl = NULL;
    if (pchEaten)
        *pchEaten = 0;

    // PERF: log 2 (sizeof(m_pflHfpl))
    hr = _GetCachedPidlFromDisplayName(pwszDisplayName, ppidl); 
    if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
    {
        // Are we are rooted within an FTP Server?
        if (IsRoot())
        {
            // No, so parse the entire thing

            // There is only one case where we want to hide the password,
            // and that is when the user entered it into the "Login"
            // dialog.  Since we entering it into the dialog will cause a
            // redirect to an URL with that password in it, we need to determie
            // if we are being called during this redirect.  If so,
            // the password just came from the Login dialog and we need to hide it.

            // This will work for fully qualified Ftp URLs
            hr = CreateFtpPidlFromUrl(pwszDisplayName, GetCWireEncoding(), pchEaten, ppidl, m_pm, FALSE);
            if (SUCCEEDED(hr))
            {
                CFtpSite * pfs;

                hr = SiteCache_PidlLookup(*ppidl, TRUE, m_pm, &pfs);
                if (EVAL(SUCCEEDED(hr)))
                {
                    // If we are using a hidden password, then ::GetDisplayNameOf() hands out
                    // these "ftp://user@server/dir/" URLs and the password is hidden.  If
                    // ::ParseDisplayName() is given one of these URLs and we are currently in
                    // that server w/that user name, then ::ParseDisplayNameOf() needs to hand
                    // out a pidl with the correct hidden password cookie.
                    //
                    // Is pidlNav the same as GetPublicPidlReference() except pidlNav doesn't
                    // have a password.  The same means that the servers match, and the user names
                    // match.
                    EVAL(SUCCEEDED(pfs->UpdateHiddenPassword(*ppidl)));
                    pfs->Release();
                }
            }
        }
        else
        {
            // Yes, so do a relative parse

            // Sometimes the user will enter incorrect information without knowing.
            // We would catch this if we verified everything that was entered, but
            // we don't, we just take it on faith until we do the IEnumIDList.
            // This is great for perf but sucks for catching these kinds of things.
            // An example of this is the user using the File.Open dialog and going to
            // "ftp://myserver/dir/".  They then enter "ftp://myserver/dir/file.txt"
            // which will try to parse relative but it's an absolute path.
            hr = _FilterBadInput(pwszDisplayName, ppidl);
            if (SUCCEEDED(hr))
            {
                CFtpDir * pfd = GetFtpDir();
                hr = CreateFtpPidlFromDisplayPath(pwszDisplayName, pfd->GetFtpSite()->GetCWireEncoding(), pchEaten, ppidl, FALSE, FALSE);
                pfd->Release();
            }
        }
    }

    if (SUCCEEDED(hr) && pdwAttributes)
    {
        hr = GetAttributesOf(1, (LPCITEMIDLIST *) ppidl, pdwAttributes);
        if (FAILED(hr))
            ILFree(*ppidl);
    }

#ifdef DEBUG
    TCHAR szUrlDebug[MAX_URL_STRING];
    
    szUrlDebug[0] = 0;
    if (*ppidl)
        UrlCreateFromPidl(*ppidl, SHGDN_FORPARSING, szUrlDebug, ARRAYSIZE(szUrlDebug), ICU_USERNAME, FALSE);

    TraceMsg(TF_FTPISF, "CFtpFolder::ParseDisplayName(%ls) CreateFtpPidlFromUrl() returned hres=%#08lx %ls", pwszDisplayName, hr, szUrlDebug);
    ASSERT(FAILED(hr) || IsValidPIDL(*ppidl));
#endif // DEBUG

    ASSERT_POINTER_MATCHES_HRESULT(*ppidl, hr);
    return hr;
}

IMalloc * CFtpFolder::GetIMalloc(void)
{
    IMalloc * pm = NULL;

    IUnknown_Set(&pm, m_pm);
    ASSERT(pm);
    return pm;
}


/*****************************************************************************\
    FUNCTION: IShellFolder::EnumObjects

    DESCRIPTION:
        Design subtlety: If we couldn't create an enumeration on the server,
    succeed, but return an enumerator that shows no objects.

    This is necessary so that our IShellView callback can put
    up error UI.  If we failed the create, the shell would
    destroy the view without giving us a chance to say what's
    up.

    It's also important for write-only directories like /incoming,
    so that the user can drag files into the directory without
    necessarily being able to drag files out.
\*****************************************************************************/
HRESULT CFtpFolder::EnumObjects(HWND hwndOwner, DWORD grfFlags, IEnumIDList ** ppenumIDList)
{
    HRESULT hres = E_FAIL;
    CFtpDir * pfd = GetFtpDir();

    // This will happen if some TARD tries to just CoCreateInstance our
    // Name Space extension and see what contents we have.  TweakUI
    // is an example of one such abuser.  Since we can only populate
    // our contents after we navigate to a FTP server, we are empty.
    *ppenumIDList = NULL;
    if (pfd)
    {
        //    Create a new enumeration object for the caller.
        ASSERT(m_pm);
        hres = CFtpEidl_Create(pfd, this, hwndOwner, grfFlags, ppenumIDList);
        TraceMsg(TF_FTPISF, "CFtpFolder::EnumObjects() CFtpEidl_Create() returned hres=%#08lx", hres);

        if (!EVAL(SUCCEEDED(hres)))
        {
            ASSERT(*ppenumIDList);            // We failed, so free what we have.
        }

        pfd->Release();
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppenumIDList, hres);
    return hres;
}


BOOL CFtpFolder::_NeedToFallBackRelative(LPCITEMIDLIST pidl, BOOL * pfDisplayProxyFallBackDlg)
{
    LPITEMIDLIST pidlFull = CreateFullPrivatePidl(pidl);
    BOOL fFallBack = FALSE;

    if (pidlFull)
    {
        fFallBack = _NeedToFallBack(pidl, pfDisplayProxyFallBackDlg);
        ILFree(pidlFull);
    }

    return fFallBack;
}



/*****************************************************************************\
    FUNCTION: _NeedToFallBack

    DESCRIPTION:
        We need to fall back to the old URLMON support in these cases:
    #1 It's a file, we let the old code use URLMON to do the download.
    #2 The app (WebOC host) has bugs that cause us to fail.
    #3 The user turned off the New FTP UI. (For whatever reason)
    #4 The proxy is a web proxy and allows URLMON but not WININET access,
       so fall back to the old support.
    #5 WININET doesn't support VMS servers, so we need to fall back in that case.

    NOTE: The order is important because we always need to calc
          fIsProxyBlockingFTP so we only display the fallback dlg
          in the correct case.
\*****************************************************************************/
BOOL CFtpFolder::_NeedToFallBack(LPCITEMIDLIST pidlFull, BOOL * pfDisplayProxyFallBackDlg)
{
    BOOL fNeedToFallBack = TRUE;

    *pfDisplayProxyFallBackDlg = FALSE;

    // TweakUI sends us an Empty pidls so don't fault. NT #396234.
    if (pidlFull && !ILIsEmpty(pidlFull))
    {
        BOOL fIsDirectory;

        if (IsFtpPidlQuestionable(pidlFull))
            _FixQuestionablePidl(pidlFull);

        fIsDirectory = (!FtpPidl_HasPath(pidlFull) || FtpPidl_IsDirectory(pidlFull, FALSE));
        if (fIsDirectory)       // #1
        {
            if (IsAppFTPCompatible())   // #2
            {
                if (!SHRegGetBoolUSValue(SZ_REGKEY_FTPFOLDER, SZ_REGKEY_USE_OLD_UI, FALSE, FALSE)) // #3
                {
                    // The binding code passes us a bind context and that would be a good
                    // key to determine if were are about to navigate to the site.  The
                    // problem is that we can't skip the proxy checking because we will
                    // fail later.
                    //
                    // #224285 is an example where navigating from ftp://ftp.microsoft.com/ to
                    // "www.microsoft.com" will cause CShellUrl to call :: BindToObject and then
                    // our IEnumIDList::Next() which will give an error message.
                    //
                    // Are we unable to get access to the server because there is
                    // a CERN type proxy blocking us?
                    // PERF: Only check for the proxy if we have a bind context because
                    //       the only place we are called from to navigate is
                    //       CDocObjectFolder:: BindToObject() and we are guaranteed that they
                    //       pass it to us.
                    if (!_IsProxyBlockingSite(pidlFull))   // #4
                    {
                        // Is this a VMS Server? If yes, fallback
                        // to URLMON support because wininet doesn't work with this kind of server.
                        if (!_IsServerVMS(pidlFull))
                        {
                            // Only continue if the user didn't turn the new UI Off.
                            fNeedToFallBack = FALSE;
                        }
                    }
                    else
                        *pfDisplayProxyFallBackDlg = TRUE;
                }
            }
        }
    }

    return fNeedToFallBack;
}


/*****************************************************************************\
    FUNCTION: IShellFolder:: BindToObject

    DESCRIPTION:
        First thing we need to do, is see if we want to over ride the default
    IE FTP support.  If we do, we call otherwise, we just fallback to the old
    support.  We want the new UI if: a) it's a directory, b) the web proxy doesn't
    block us, and c) the user didn't turn us off.

    PERF/TODO:
        OrderItem_GetSystemImageListIndexFromCache (\shell\lib\dpastuff.cpp)
    uses riid=IShellFolder when trying to find out the icon.  We don't want
    to hit the net in that case, so force them to pass a pbc to indicate skipping
    the net in that case.
\*****************************************************************************/
HRESULT CFtpFolder::BindToObject(LPCITEMIDLIST pidl, IBindCtx * pbc, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // Indicate we want the old functionality to kick in.

    if (ppvObj)
        *ppvObj = NULL;

    if (!pidl || ILIsEmpty(pidl) || !_IsValidPidlParameter(pidl))
    {
        // Caller, are you smoking crack?  What's the idea of passing
        // an empty pidl. (Comdlg32 is known to do this)
        hr = E_INVALIDARG;
    }
    else
    {
        BOOL fDisplayProxyFallBackDlg = FALSE;
        LPITEMIDLIST pidlFull = CreateFullPrivatePidl(pidl);

        // We need to handle it.
        hr = _BindToObject(pidl, pidlFull, pbc, riid, ppvObj);

        // Maybe we still need to handle it if 
        ASSERT(HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr);
        ILFree(pidlFull);
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}


/*****************************************************************************\
     FUNCTION: IShellFolder::BindToStorage
 
    DESCRIPTION:
         We need to implement this so the user can Open and Save files in
    the standard Open Dialog and Save Dialog.
\*****************************************************************************/
HRESULT CFtpFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;

    if (!EVAL(_IsValidPidlParameter(pidl)))
        return E_INVALIDARG;

    *ppvObj = 0;
    if (EVAL(pidl))
    {
        IMoniker * pmk;

        hr = _PidlToMoniker(pidl, &pmk);
        if (SUCCEEDED(hr))
        {
            hr =  pmk->BindToStorage(pbc, NULL, riid, ppvObj);
            pmk->Release();
        }
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    TraceMsg(TF_FTPISF, "CFtpFolder::BindToStorage() hr=%#08lx", hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IShellFolder::CompareIDs

    DESCRIPTION:
        ici - column on which to sort.  Note! that we rely on the fact that
    IShellFolders are uniform; we do not need to bind to the shell folder in
    order to compare its sub-itemids.

    _UNDOCUMENTED_: The documentation does not say whether or not
    complex pidls can be received.  In fact, they can.
\*****************************************************************************/
HRESULT CFtpFolder::CompareIDs(LPARAM ici, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    ASSERT(IsValidPIDL(pidl1));
    ASSERT(IsValidPIDL(pidl2));

    return FtpItemID_CompareIDs(ici, pidl1, pidl2, FCMP_GROUPDIRS);
}


HRESULT CFtpFolder::_CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_FAIL;
    CFtpDir * pfd = GetFtpDir();

    *ppvObj = NULL;            // Explorer relies on this
    //TraceMsg(TF_FTPISF, "CFtpObj::CreateViewObject() ");
    if (pfd)
    {
        if (IsEqualIID(riid, IID_IDropTarget))
        {
            //  Don't create a drop target for the root FTP folder.
            if (IsRoot())
                hr = E_NOINTERFACE;
            else
            {
                CFtpDrop * pfm;
                hr = CFtpDrop_Create(this, hwndOwner, &pfm);
                if (EVAL(SUCCEEDED(hr)))
                {
                    hr = pfm->QueryInterface(riid, ppvObj);
                    pfm->Release();
                }
            }
        }
        else
            hr = E_NOINTERFACE;

        // TODO: IID_IShellDetails
        pfd->Release();
    }
    else
        hr = E_FAIL;            // Can't do that yet - Never _Initialize'd

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    if (FAILED(hr))
        hr = CBaseFolder::CreateViewObject(hwndOwner, riid, ppvObj);

    return hr;
}


IShellFolder * CFtpFolder::_GetLegacyShellFolder(void)
{
    IShellFolder * psfLegacy = NULL;

    // I assert that this succeeds because I need to make
    // sure every install case has this CLSID publicly available.
    if (EVAL(SUCCEEDED(CoCreateInstance(CLSID_CDocObjectFolder, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (void **)&psfLegacy))))
    {
        LPITEMIDLIST pidl = GetPrivatePidlClone();

        if (pidl && !ILIsEmpty(pidl))
        {
            LPITEMIDLIST pidlLast = (LPITEMIDLIST) ILGetLastID(pidl);
            LPITEMIDLIST pidlLegacy;

            if (!FtpID_IsServerItemID(pidlLast))
            {
                // NT #291513: We want to force the last item to always be marked as a file
                //          because then it will not have the trailing '/' in the URL when we
                //          pass it to URLMON.  This way, we leave wether it's a file or dir
                //          ambigious for URLMON to figure out.  This is because we can't
                //          disambiguate because the proxy blocks us but URLMON handles it
                //          correctly.
                FtpPidl_SetFileItemType(pidlLast, FALSE);
                FtpPidl_SetAttributes(pidl, FILE_ATTRIBUTE_NORMAL);
            }

            if (SUCCEEDED(_GetLegacyPidl(pidl, &pidlLegacy)))
            {
                if (FAILED(_InitLegacyShellFolder(psfLegacy, pidlLegacy)))
                    ATOMICRELEASE(psfLegacy);

                ILFree(pidlLegacy);
            }
            
            ILFree(pidl);
        }
    }

    return psfLegacy;
}


/*****************************************************************************\
    FUNCTION: IShellFolder:: CreateViewObject

    DESCRIPTION:
    _UNDOCUMENTED_: This entire method is not documented.

    _UNDOCUMENTED_: It is not documented that you need to
    provide an IDropTarget object if you want the view to
    act as a drop target.

    IDropTarget produces a droptarget for the folder itself.

    _UNDOCUMENTED_: The IShellView interface is not documented.

    IShellView produces a shell view for the folder itself.

    _UNOBVIOUS_:  Not obvious that this is how the shell gets
    a context menu for the folder itself.  (You might think it
    comes from GetUIObjectOf...)

    IContextMenu produces a context menu for the folder itself.
    This is important for supporting things like New and Paste.

    IShellDetails (undocumented) is the direct interface to
    GetDetailsOf and ColumnClick, which is now obsolete, replaced
    by the DVM_GETDETAILSOF and DVM_COLUMNCLICK notifications.

    _UNDOCUMENTED_: SHCreateShellFolderViewEx is not documented.

    Yes, it's annoying how some things are handled by CreateViewObject
    and some things are handled by GetUIObjectOf(cpidl = 0), so we
    keep having to forward the requests back and forth.  Particularly
    annoying because the shell actually comes through both ways.

    For example, if the user drags something onto a folder,
    it does a CreateViewObject(IDropTarget), because it might not
    be able to bind to the parent to get the IDropTarget (if the
    folder is the root of a namespace).

    But if you drag an object onto a subfolder of a folder, the shell
    asks for a GetUIObjectOf(pidl, IDropTarget) so it can talk to
    the drop target of the subobject.  It does this to allow the
    shell folder to create a quick IDropTarget without necessarily
    binding to the subobject first.

    We don't do any such optimization, so GetUIObjectOf() simply
    binds to the subfolder and uses CreateViewObject().


    If the IShellFolder doesn't have an FtpSite (typically because it
    has not been IPersistFolder::Initialize'd), then fail any attempt
    to create a view object.
\*****************************************************************************/
HRESULT CFtpFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_FAIL;
    BOOL fDisplayProxyFallBackDlg = TRUE;

    // We don't need to wory about falling back with a few interfaces, 
    // IResolveShellLink is one of them.  There may be others, and we should
    // add them if they are used often or in common scenarios because of the
    // HUGE PERF IMPACT of _NeedToFallBack() which checks the net for a proxy
    // blocking us.  TODO: Investigate if IID_IDropTarget, IID_IContextMenu can be added.
    if (!IsEqualIID(riid, IID_IResolveShellLink) && !IsEqualIID(riid, IID_IShellDetails) &&
        _NeedToFallBack(GetPrivatePidlReference(), &fDisplayProxyFallBackDlg))
    {
        IShellFolder * psfLegacy = _GetLegacyShellFolder();

        // We only want to display the proxy blocking dialog when we are creating the view.
        if (fDisplayProxyFallBackDlg && IsEqualIID(riid, IID_IShellView))
        {
            EVAL(SUCCEEDED(DisplayBlockingProxyDialog(GetPrivatePidlReference(), hwndOwner)));
        }

        if (psfLegacy)
        {
            hr = psfLegacy->CreateViewObject(hwndOwner, riid, ppvObj);
            psfLegacy->Release();
        }
    }
    else
    {
        hr = _CreateViewObject(hwndOwner, riid, ppvObj);
    }
    

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IShellFolder::GetAttributesOf

    DESCRIPTION:
        If we are given cpidl = 0, then we are being asked for attributes
    on the folder itself.  But note that some people pass slightly
    confused versions of cpidl = 0, as noted in the comment block below.

    If the SFGAO_VALIDATE bit is set with cpidl = 0, then the view
    object is warning us that it is about to refresh, so we should
    throw away any cached information.

    NOTE!  ftpcm.cpp relies heavily on the fact that this routine will
    fail when given complex pidls.  (This allows ftpcm.cpp to assume
    that all the pidls are directly in the affected folder.)

    _UNDOCUMENTED_: The documentation does not say whether or not
    complex pidls can be received.  I don't know whether or not
    they can, so I'll code defensively and watch out for them.

    BUGBUG: Does a server need to return SFGAO_HASSUBFOLDER?

\*****************************************************************************/
HRESULT CFtpFolder::GetAttributesOf(UINT cpidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    HRESULT hr;
    DWORD dwMask = *rgfInOut;

    //    _UNDOCUMENTED_:
    //    Some stupid people pass cpidl = 1 but ILIsEmpty(apidl[0]),
    //    intending to pass cpidl = 0.  While we're being distracted
    //    by stupid people, may as well catch apidl[0] == 0 also...
    //    Oh, and defview sometimes passes cpidl = 1 but apidl == 0...
    if (cpidl > 0 && apidl && apidl[0] && !ILIsEmpty(apidl[0]))
    {
        UINT ipidl;

        //  Can't multi-rename because there's nowhere to pass the new names.
        //  Can't multi-paste since you don't know where it really goes.
        if (cpidl > 1)
        {
            *rgfInOut &= ~SFGAO_CANRENAME;
            *rgfInOut &= ~SFGAO_DROPTARGET;
        }
        hr = S_OK;
        for (ipidl = 0; ipidl < cpidl; ipidl++)
        {
            // This maybe a fully qualified pidl or relative pidl
            LPITEMIDLIST pidlFull;

            if (ILIsSimple(apidl[ipidl]))
                pidlFull = CreateFullPrivatePidl(apidl[0]);
            else
                pidlFull = (LPITEMIDLIST) apidl[0];

            *rgfInOut &= FtpGetAttributesOf(pidlFull);

            // BUG NT #166783: shell32 v3 & v4 (Win95 & NT4 orig) won't allow you
            // to allow SFGAO_CANLINK but disallow the link to be created in your
            // own folder.  So we need to disable this item in browser only.
            if (SHELL_VERSION_NT5 != GetShellVersion())
                *rgfInOut &= ~SFGAO_CANLINK;

            if (apidl[0] != pidlFull)
                ILFree(pidlFull);       // We alloced it so we free it.
        }
    }
    else
    {
        // At top-level, SFGAO_DROPTARGET is also disabled
        if (IsRoot())
            *rgfInOut &= ~SFGAO_DROPTARGET;

        *rgfInOut &= ~(SFGAO_GHOSTED | SFGAO_LINK | SFGAO_READONLY |
                   SFGAO_SHARE | SFGAO_REMOVABLE);

        if (*rgfInOut & SFGAO_VALIDATE)
            InvalidateCache();      // About to refresh...

        hr = S_OK;
    }

    // We can create a moniker for anything, a file or a folder.
    *rgfInOut |= (SFGAO_CANMONIKER & dwMask);

//    TraceMsg(TF_FTPISF, "CFtpFolder::GetAttributesOf() *rgfInOut=%#08lx, hr=%#08lx", *rgfInOut, hr);
    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        Creates an pflHfpl and asks CFtpFolder_GetUIObjectOfHfpl (qv)
    to do the real work.

    Note that we always release the pflHfpl.  If GetUIObjectOfHfpl
    needs to keep the pflHfpl, it will do its own AddRef().
\*****************************************************************************/
HRESULT CFtpFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST rgpidl[],
                                REFIID riid, UINT * prgfInOut, LPVOID * ppvObj)
{
     return _GetUIObjectOf(hwndOwner, cidl, rgpidl, riid, prgfInOut, ppvObj, FALSE);
}


/*****************************************************************************\
    DESCRIPTION:
        Creates an pflHfpl and asks CFtpFolder_GetUIObjectOfHfpl (qv)
    to do the real work.

    Note that we always release the pflHfpl.  If GetUIObjectOfHfpl
    needs to keep the pflHfpl, it will do its own AddRef().
\*****************************************************************************/
HRESULT CFtpFolder::_GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST rgpidl[],
                                REFIID riid, UINT * prgfInOut, LPVOID * ppvObj, BOOL fFromCreateViewObject)
{
    CFtpPidlList * pflHfpl = NULL;
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlFull;
    
    if (rgpidl)
        pidlFull = CreateFullPrivatePidl(rgpidl[0]);
    else
        pidlFull = GetPrivatePidlClone();
    
    if (ppvObj)
        *ppvObj = NULL;

    // Is the proxy blocking us?  If yes, don't do anything
    // because we don't want our Context Menu to appear for the
    // original FTP UI.
    // It's not blocking so go ahead.
    hr = CFtpPidlList_Create(cidl, rgpidl, &pflHfpl);
    if (EVAL(SUCCEEDED(hr)))
    {
        _InitFtpSite(); // GetUIObjectOfHfpl() will later need m_pfs. 
        hr = GetUIObjectOfHfpl(hwndOwner, pflHfpl, riid, ppvObj, fFromCreateViewObject);
        pflHfpl->Release();
    }

    if (pidlFull)
        ILFree(pidlFull);

//    TraceMsg(TF_FTPISF, "CFtpFolder::GetUIObjectOf() hres=%#08lx", hr);
    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IShellFolder::GetDisplayNameOf

    DESCRIPTION:
        Note! that since we do not support junctions (duh), we can
    safely walk down the pidl generating goop as we go, secure
    in the knowledge that we are in charge of every subpidl.

    FTP UNICODE ISSUE:
        The FTP spec (RFC 959 (?)) says that FTP uses 8-bit BYTEs as
    names.  If the 8th bit is zero, these are treated as ANSI.
    But it's not specified what the 8th bit means when it's set?
    Some lame FTP clients have been pushing DBCS/MBCS up using the 8th bit
    but this incurs data loss because the code page is lost and cross
    code page strings are not supported.  For that reason, a combination
    of UTF-8 (by default) should be used and fall back to DBCS with
    code page guessing (maybe need UI to guess code page).

    We need to use WININET BYTE APIs (BYTE means ANSI with an ambiguous 8th bit).
    We then need to store those bytes in our cache (CFtpDir).  When we display
    these strings in UI, we need to convert them to unicode and guess weather
    it's UTF-8 or DBCS encoded.
\*****************************************************************************/
HRESULT CFtpFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD shgno, LPSTRRET pStrRet)
{
    HRESULT hr = E_FAIL;

    // It's invalid for someone to pass us an empty pidl, but some retards do.
    // (comdlg32\GetPathFromLocation)  Reject it now.
    if (ILIsEmpty(pidl) || !EVAL(_IsValidPidlParameter(pidl)))
        return E_INVALIDARG;

    LPITEMIDLIST pidlFull = CreateFullPrivatePidl(pidl);
    if (EVAL(pidlFull))
    {
        ASSERT(IsValidPIDL(pidlFull));

        hr = StrRetFromFtpPidl(pStrRet, shgno, pidlFull);
        ILFree(pidlFull);
    }

//    TraceMsg(TF_FTPISF, "CFtpFolder::GetDisplayNameOf() szName=%hs, hres=%#08lx", pStrRet->cStr, hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IShellFolder::SetNameOf

    DESCRIPTION:
        The real work is done by SetNameOf.
\*****************************************************************************/
HRESULT CFtpFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR pwszName,
                                DWORD dwReserved, LPITEMIDLIST *ppidlOut)
{
    HRESULT hr = S_FALSE;
    CFtpDir * pfd = GetFtpDir();
    TCHAR szPath[MAX_PATH];
    BOOL fContinueToRename = TRUE;

    if (!EVAL(_IsValidPidlParameter(pidl)))
        return E_INVALIDARG;

    ASSERT(IsValidPIDL(pidl));
    SHUnicodeToTChar(pwszName, szPath, ARRAYSIZE(szPath));

    // Does the new item not have an extension and this isn't a directory?
    if (0 == PathFindExtension(szPath)[0] && !FtpPidl_IsDirectory(pidl, FALSE))
    {
        LPCWIRESTR pszFileName = FtpPidl_GetLastItemWireName(pidl);
        // Yes, then we are scared that they may be loosing an extension.

        // Did the original name have an extension?
        if (pszFileName && PathFindExtensionA(pszFileName)[0])
        {
            // Yes, so now we are scared they may loose it and not be able
            // to find the src app.  Ask the user if they really want to do this
            // rename if that will mean the file will no longer have an extension.

            // Hey browser, can I display UI?
            if (EVAL(hwndOwner))
            {
                // Hay browser, cover me, I'm going to do UI.
                IUnknown_EnableModless(_punkSite, FALSE);

                TCHAR szTitle[MAX_PATH];
                TCHAR szReplaceMsg[MAX_PATH*4];

                EVAL(LoadString(HINST_THISDLL, IDS_FTPERR_RENAME_TITLE, szTitle, ARRAYSIZE(szTitle)));
                EVAL(LoadString(HINST_THISDLL, IDS_FTPERR_RENAME_EXT_WRN, szReplaceMsg, ARRAYSIZE(szReplaceMsg)));
                if (IDNO == MessageBox(hwndOwner, szReplaceMsg, szTitle, (MB_YESNO | MB_ICONEXCLAMATION)))
                    fContinueToRename = FALSE;  // Cancel the rename.

                IUnknown_EnableModless(_punkSite, TRUE);
            }
        }
    }

    if (fContinueToRename)
    {
        if (EVAL(pfd))
        {
            hr = pfd->SetNameOf(this, hwndOwner, pidl, szPath, dwReserved, ppidlOut);
            if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr) && hwndOwner)
            {
                DisplayWininetError(hwndOwner, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_FILERENAME, IDS_FTPERR_WININET, MB_OK, NULL);
            }
        }

        TraceMsg(TF_FTPISF, "CFtpFolder::SetNameOf(%ls) hres=%#08lx", pwszName, hr);
        //        shell32.dll in IE4 (maybe earlier also) will infinitely call
        //    CFtpFolder::SetNameOf() over and over if it returns FAILED(hr);
        if (FAILED(hr))
            hr = S_FALSE;
    }

    if (ppidlOut)
    {
        ASSERT_POINTER_MATCHES_HRESULT(*ppidlOut, hr);
    }

    if (pfd)
        pfd->Release();

    return hr;
}


//===========================
// *** IPersistFolder Interface ***
//===========================

/*****************************************************************************\
    FUNCTION: IPersistFolder::Initialize

    DESCRIPTION:
        This is called when the shell creates a new "root".

    Note that we pass a fake "null" ID list as the second
    pidl to ::_Initialize, q.v., for explanation.

    _UNDOCUMENTED_: Undocumented method in undocumented interface.
\*****************************************************************************/
HRESULT CFtpFolder::Initialize(LPCITEMIDLIST pidl)
{
    ASSERT(IsValidPIDL(pidl));

    HRESULT hr = _Initialize(pidl, NULL, ILGetSize(pidl) - sizeof(pidl->mkid.cb));
    //TraceMsg(TF_FTPISF, "CFtpFolder::Initialize() hres=%#08lx", hr);
    return hr;
}


//===========================
// *** IShellIcon Interface ***
//===========================

/*****************************************************************************\
    FUNCTION: IShellIcon::GetIconOf

    DESCRIPTION:
        Get the system icon imagelist index for a pidl.  Subtlety - If we are
    enumerating children of the root, their icon is a computer.

    _UNDOCUMENTED_: Undocumented method in undocumented interface.
\*****************************************************************************/
HRESULT CFtpFolder::GetIconOf(LPCITEMIDLIST pidl, UINT gil, LPINT pnIcon)
{
    HRESULT hr = S_OK;

    if (!EVAL(_IsValidPidlParameter(pidl)))
        return E_INVALIDARG;

    // BUGBUG: Fix this to get custom icons for DefView ListView
    *pnIcon = GetFtpIcon(gil, IsRoot());

    ASSERT(IsValidPIDL(pidl));
    if (EVAL(!IsRoot()))    // GetFtpIcon() is wrong so either fix it or verify we never try to use it.
    {
        SHFILEINFO sfi;
        hr = FtpPidl_GetFileInfo(pidl, &sfi, SHGFI_SYSICONINDEX |
                    ((gil & GIL_OPENICON) ? SHGFI_OPENICON : 0));

        if (SUCCEEDED(hr))
        {
            *pnIcon = sfi.iIcon;
            if (sfi.hIcon)
                DestroyIcon(sfi.hIcon);
        }
    }

//    TraceMsg(TF_FTPISF, "CFtpFolder::GetIconOf() hres=%#08lx", hr);
    return hr;
}



//===========================
// *** IShellIconOverlay Interface ***
//===========================
HRESULT CFtpFolder::GetOverlayIndexHelper(LPCITEMIDLIST pidl, int * pIndex, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;

    *pIndex = 0;
    // Is this a soft link? (Meaning it won't have a windows link
    // extension (.lnk, .url, ...) but we still want the shortcut cue.
    if (pidl && FtpPidl_IsSoftLink(pidl))
    {
        if (!m_psiom)
            EVAL(SUCCEEDED(hr = CoCreateInstance(CLSID_CFSIconOverlayManager, NULL, CLSCTX_INPROC_SERVER, IID_IShellIconOverlayManager, (void **)&m_psiom)));

        if (EVAL(m_psiom))
        {
            hr = m_psiom->GetReservedOverlayInfo(L"", FtpPidl_GetAttributes(pidl), pIndex, dwFlags, SIOM_RESERVED_LINK);
        }
    }

    return hr;
}


//===========================
// *** IDelegateFolder Interface ***
//===========================
/*****************************************************************************\
    FUNCTION: IDelegateFolder::SetItemAlloc

    DESCRIPTION:
        Gives us the pidl allocator.
\*****************************************************************************/
HRESULT CFtpFolder::SetItemAlloc(IMalloc *pm)
{
    IUnknown_Set(&m_pm, pm);

//    TraceMsg(TF_FTPISF, "CFtpFolder::SetItemAlloc(IMalloc *pm=%#08lx) hres=%#08lx", pm, S_OK);
    return S_OK;
}


//===========================
// *** IBrowserFrameOptions Interface ***
//===========================
/*****************************************************************************\
    DESCRIPTION:
        Tell the browser/host what behaviors we want.  This lets the caller
    know when we want to act like the shell, the browser, or even unique.
\*****************************************************************************/
HRESULT CFtpFolder::GetFrameOptions(IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions)
{
    // This function is called in the follow situations:
    // ftp://bryanst/ (w/ & w/o folder shortcut)
    // ftp://bryanst/default.htm (w/ & w/o folder shortcut)
    // ftp://bryanst/notes.txt (w/ & w/o folder shortcut)
    // ftp://bryanst/resume.doc (w/ & w/o folder shortcut)
    // ftp://bryanst/ (w/ & w/o folder shortcut)
    // ftp://bryanst/ (w/ & w/o folder shortcut)
    // ftp://bryanst/ (w/ & w/o folder shortcut)
    HRESULT hr = E_INVALIDARG;

    if (pdwOptions)
    {
        // We want both "Internet Options" and "Folder Options".
        *pdwOptions = dwMask & (BFO_BOTH_OPTIONS | BFO_BROWSE_NO_IN_NEW_PROCESS |
                                BFO_NO_REOPEN_NEXT_RESTART |
                                BFO_ENABLE_HYPERLINK_TRACKING | BFO_USE_IE_LOGOBANDING |
                                BFO_ADD_IE_TOCAPTIONBAR | BFO_USE_DIALUP_REF);
        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: CFtpFolder_Create

    DESCRIPTION:
        Note that we release the pff that Common_New created, because we
    are done with it.  The real refcount is handled by the
    CFtpFolder_QueryInterface.
\*****************************************************************************/
HRESULT CFtpFolder_Create(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres = E_OUTOFMEMORY;
    CFtpFolder * pff = new CFtpFolder();

    *ppvObj = NULL;
    if (EVAL(pff))
    {
        hres = pff->QueryInterface(riid, ppvObj);
        pff->Release();
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hres);
    return hres;
}


/*****************************************************************************\
    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpFolder_Create(LPCITEMIDLIST pidlTarget, LPCITEMIDLIST pidlRoot, int ib, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = S_OK;
    CFtpFolder * pff = new CFtpFolder();

    ASSERT(IsValidPIDL(pidlTarget));
    ASSERT(!pidlRoot || IsValidPIDL(pidlRoot));

    *ppvObj = NULL;
    if (!EVAL(pff))
        return E_OUTOFMEMORY;
    else
    {
        hr = pff->_Initialize(pidlTarget, pidlRoot, ib);
        if (EVAL(SUCCEEDED(hr)))
            hr = pff->QueryInterface(riid, ppvObj);

        pff->Release();
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppvObj, hr);
    return hr;
}


/****************************************************\
    Constructor
\****************************************************/
CFtpFolder::CFtpFolder() : CBaseFolder((LPCLSID) &CLSID_FtpFolder)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pfs);
    ASSERT(!m_pm);
    ASSERT(!m_puhs);
    ASSERT(!m_psiom);

    // Needed because we need to call CoCreateInstance() on Browser Only.
    LEAK_ADDREF(LEAK_CFtpFolder);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpFolder::~CFtpFolder()
{
    ATOMICRELEASE(m_pfs);
    ATOMICRELEASE(m_pm);
    ATOMICRELEASE(m_puhs);
    ATOMICRELEASE(m_psiom);

    if (m_hinstInetCpl)
        FreeLibrary(m_hinstInetCpl);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpFolder);
}


//===========================
// *** IUnknown Interface ***
//===========================

HRESULT CFtpFolder::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellIcon))
    {
        *ppvObj = SAFECAST(this, IShellIcon*);
    }
//  This causes all icons to use my custom ftp folder icon, so I will do this when that is fixed.
#ifdef FEATURE_SOFTLINK_SHORTCUT_ICONOVERLAY
    else if (IsEqualIID(riid, IID_IShellIconOverlay))
    {
        *ppvObj = SAFECAST(this, IShellIconOverlay*);
    }
#endif // FEATURE_SOFTLINK_SHORTCUT_ICONOVERLAY
    else if (IsEqualIID(riid, IID_IPersistFolder))
    {
        *ppvObj = SAFECAST(this, IPersistFolder*);
    }
    else if (IsEqualIID(riid, IID_IDelegateFolder))
    {
        *ppvObj = SAFECAST(this, IDelegateFolder*);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite*);
    }
    else if (IsEqualIID(riid, IID_IPersistFolder2))
    {
        *ppvObj = SAFECAST(this, IPersistFolder2*);
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppvObj = SAFECAST(this, IShellPropSheetExt*);
    }
    else if (IsEqualIID(riid, IID_IBrowserFrameOptions))
    {
        *ppvObj = SAFECAST(this, IBrowserFrameOptions*);
    }
    else if (IsEqualIID(riid, IID_CFtpFolder))
    {
        // Only valid if caller lives in msieftp.dll
        *ppvObj = (void *)this;
    }
    else
        return CBaseFolder::QueryInterface(riid, ppvObj);

    AddRef();
    return S_OK;
}
