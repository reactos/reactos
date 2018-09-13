/*****************************************************************************
 *
 *    ftpeidl.cpp - IEnumIDList interface
 *
 *    FtpNameCache
 *
 *    Enumerating an FTP site is an expensive operation, because
 *    it can entail dialing the phone, connecting to an ISP, then
 *    connecting to the site, logging in, cd'ing to the appropriate
 *    location, pumping over an "ls" command, parsing the result,
 *    then closing the connection.
 *
 *    So we cache the results of an enumeration inside a pidl list.
 *    If the user does a REFRESH, then we toss the list and create
 *    a new one.
 *
 *    NOTE! that the WinINet API does not allow a FindFirst to be
 *    interrupted.  In other words, once you do an FtpFindFirst,
 *    you must read the directory to completion and close the
 *    handle before you can do anything else to the site.
 *
 *    As a result, we cannot use lazy evaluation on the enumerated
 *    contents.  (Not that it helps any, because WinINet will just
 *    do an "ls", parse the output, and then hand the items back
 *    one element at a time via FtpFindNext.  You may as well suck them
 *    all down the moment they're ready.)
 *
\*****************************************************************************/

#include "priv.h"
#include "ftpeidl.h"
#include "view.h"
#include "util.h"


/*****************************************************************************
 *
 *    We actually cache the result of the enumeration in the parent
 *    FtpDir, because FTP enumeration is very expensive.
 *
 *    Since DVM_REFRESH forces us to re-enumerate, but we might have
 *    outstanding IEnumIDList's, we need to treat the object cache
 *    as yet another object that needs to be refcounted.
 *
 *****************************************************************************/


/*****************************************************************************
 *    _fFilter
 *
 *    Decides whether the file attributes agree with the filter criteria.
 *
 *    If hiddens are excluded, then exclude hiddens.  (Duh.)
 *
 *    Else, include or exclude based on folder/nonfolder-ness.
 *
 *    Let's look at that expression in slow motion.
 *
 *    "The attributes pass the filter if both...
 *        (1) it passes the INCLUDEHIDDEN criterion, and
 *        (2) it passes the FOLDERS/NONFOLDERS criterion.
 *
 *    The INCLUDEHIDDEN criterion is passed if FILE_ATTRIBUTE_HIDDEN
 *    implies SHCONTF_INCLUDEHIDDEN.
 *
 *    The FOLDERS/NONFOLDERS criterion is passed if the appropriate bit
 *    is set in the shcontf, based on the actual type of the file."
 *****************************************************************************/
BOOL CFtpEidl::_fFilter(DWORD shcontf, DWORD dwFAFLFlags)
{
    BOOL fResult = FALSE;

    if (shcontf & SHCONTF_FOLDERS)
        fResult |= dwFAFLFlags & FILE_ATTRIBUTE_DIRECTORY;

    if (shcontf & SHCONTF_NONFOLDERS)
        fResult |= !(dwFAFLFlags & FILE_ATTRIBUTE_DIRECTORY);

    if ((dwFAFLFlags & FILE_ATTRIBUTE_HIDDEN) && !(shcontf & SHCONTF_INCLUDEHIDDEN))
        fResult = FALSE;

    return fResult;
}


/*****************************************************************************\
 *    _AddFindDataToPidlList
 *
 *    Add information in a WIN32_FIND_DATA to the cache.
 *    Except that dot and dotdot don't go in.
\*****************************************************************************/
HRESULT CFtpEidl::_AddFindDataToPidlList(LPCITEMIDLIST pidl)
{
    HRESULT hr = E_FAIL;

    if (EVAL(m_pflHfpl))
    {
        ASSERT(IsValidPIDL(pidl));
        hr = m_pflHfpl->InsertSorted(pidl);
    }
    
    return hr;
}


/*****************************************************************************\
    FUNCTION: _HandleSoftLinks

    DESCRIPTION:
        A softlink is a file on an UNIX server that reference another file or
    directory.  We can detect these by the fact that (pwfd->dwFileAttribes == 0).
    If that is true, we have some work to do.  First we find out if it's a file
    or a directory by trying to ChangeCurrentWorking directories into it.  If we
    can we turn the dwFileAttributes from 0 to (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT).
    If it's just a softlink to a file, then we change it to
    (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_REPARSE_POINT).  We later use the
    FILE_ATTRIBUTE_REPARSE_POINT attribute to put the shortcut overlay on it to
    que the user.

    RETURN VALUE:
        HRESULT - If FAILED() is returned, the item will not be added to the
                  list view.
\*****************************************************************************/
HRESULT CFtpEidl::_HandleSoftLinks(HINTERNET hint, LPITEMIDLIST pidl, LPWIRESTR pwCurrentDir, DWORD cchSize)
{
    HRESULT hr = S_OK;

    // Is it a softlink? It just came in off the wire and wininet returns 0 (zero)
    // for softlinks.  This function will determine if it's a SoftLink to a file
    // or a directory and then set FILE_ATTRIBUTE_REPARSE_POINT or
    // (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT) respectively.
    if (0 == FtpPidl_GetAttributes(pidl))
    {
        LPCWIRESTR pwWireFileName = FtpPidl_GetFileWireName(pidl);

        // Yes, so I will need to attempt to CD into that directory to test if it's a directory.
        // I need to get back because ".." won't work.  I will cache the return so I don't keep
        // getting it if there is a directory full of them.

        // Did we get the current directory yet?  This is the bread crums so I can
        // find my way back.
        if (!pwCurrentDir[0])
            EVAL(SUCCEEDED(FtpGetCurrentDirectoryWrap(hint, TRUE, pwCurrentDir, cchSize)));

        // Yes, so is it a directory?
        if (SUCCEEDED(FtpSetCurrentDirectoryPidlWrap(hint, TRUE, pidl, FALSE, FALSE)))  // Relative CD
        {
            // Does it have a virtual root?
            if (m_pfd->GetFtpSite()->HasVirtualRoot())
            {
                LPCITEMIDLIST pidlVirtualRoot = m_pfd->GetFtpSite()->GetVirtualRootReference();
                LPITEMIDLIST pidlSoftLinkDest = NULL;
                CWireEncoding * pwe = m_pfd->GetFtpSite()->GetCWireEncoding();

                // Yes, so we need to make sure this dir softlink doesn't point
                // outside of the virtual root, or it would cause invalid FTP URLs.
                // File SoftLinks are fine because the old FTP Code abuses FTP URLs.
                // I'm just not ready to drop my morals just yet.
                if (EVAL(SUCCEEDED(FtpGetCurrentDirectoryPidlWrap(hint, TRUE, pwe, &pidlSoftLinkDest))))
                {
                    if (!FtpItemID_IsParent(pidlVirtualRoot, pidlSoftLinkDest))
                    {
                        // This is a Softlink or HardLink to a directory outside of the virtual root.
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);  // Skip this one.
                    }

                    ILFree(pidlSoftLinkDest);
                }
            }

            // Return to where we came from.
            //TraceMsg(TF_WININET_DEBUG, "_HandleSoftLinks FtpSetCurrentDirectory(%hs) worked", pwWireFileName);
            EVAL(SUCCEEDED(FtpSetCurrentDirectoryWrap(hint, TRUE, pwCurrentDir)));  // Absolute CD
            FtpPidl_SetAttributes(pidl, (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
            FtpPidl_SetFileItemType(pidl, TRUE);
        }
        else    // No, it's one of those files w/o extensions.
        {
            TraceMsg(TF_WININET_DEBUG, "_HandleSoftLinks FtpSetCurrentDirectory(%s) failed", pwWireFileName);
            FtpPidl_SetAttributes(pidl, (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_REPARSE_POINT));
            FtpPidl_SetFileItemType(pidl, FALSE);
        }
    }

    return hr;
}


/*****************************************************************************\
 *    CFtpEidl::_PopulateItem
 *
 *    Fill a cache with stuff.
 *
 *    BUGBUG -- EEK!  Some ftp servers (e.g., ftp.funet.fi) run with ls -F!
 *    This means that things get "*" appended to them if they are executable.
\*****************************************************************************/
HRESULT CFtpEidl::_PopulateItem(HINTERNET hint0, HINTPROCINFO * phpi)
{
    HRESULT hr = S_OK;
    HINTERNET hint;
    LPITEMIDLIST pidl;
    CMultiLanguageCache cmlc;
    CWireEncoding * pwe = m_pfd->GetFtpSite()->GetCWireEncoding();

    if (phpi->psb)
    {
        phpi->psb->SetStatusMessage(IDS_LS, NULL);
        EVAL(SUCCEEDED(_SetStatusBarZone(phpi->psb, phpi->pfd->GetFtpSite())));
    }

    hr = FtpFindFirstFilePidlWrap(hint0, TRUE, &cmlc, pwe, NULL, &pidl, 
                (INTERNET_NO_CALLBACK | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RESYNCHRONIZE | INTERNET_FLAG_RELOAD), NULL, &hint);
    if (hint)
    {
        WIRECHAR wCurrentDir[MAX_PATH];   // Used for _HandleSoftLinks().

        wCurrentDir[0] = 0;
        if (EVAL(m_pff))
        {
            // It would be better to CoCreateInstance the History object by using
            // shell32!_SHCoCreateInstance() because it doesn't require COM.
            // If any more bugs are found, see if it's exported in Win95 and use it.           
            if (FAILED(m_hrOleInited))
            {
                // Win95's background enum thread doesn't call CoInitialize() so this AddToUrlHistory will fail.
                // We init it ourselves.
                m_hrOleInited = SHCoInitialize();
            }
            m_pff->AddToUrlHistory(m_pfd->GetPidlReference());
        }

        //TraceMsg(TF_FTP_OTHER, "CFtpEidl::_PopulateItem() adding Name=%s", wCurrentDir);
        if (pidl && SUCCEEDED(_HandleSoftLinks(hint0, pidl, wCurrentDir, ARRAYSIZE(wCurrentDir))))
            hr = _AddFindDataToPidlList(pidl);

        ILFree(pidl);
        while (SUCCEEDED(hr))
        {
            hr = InternetFindNextFilePidlWrap(hint, TRUE, &cmlc, pwe, &pidl);
            if (SUCCEEDED(hr))
            {
                //TraceMsg(TF_FTP_OTHER, "CFtpEidl::_PopulateItem() adding Name=%hs", FtpPidl_GetLastItemWireName(pidl));
                // We may decide to not add it for some reasons.
                if (SUCCEEDED(_HandleSoftLinks(hint0, pidl, wCurrentDir, ARRAYSIZE(wCurrentDir))))
                    hr = _AddFindDataToPidlList(pidl);

                ILFree(pidl);
            }
            else
            {
                // We failed to get the next file.
                if (HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) != hr)
                {
                    DisplayWininetError(phpi->hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_FOLDERENUM, IDS_FTPERR_WININET, MB_OK, NULL);
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);       // Clean error to indicate we already displayed the error and don't need to do it later.
                }
                else
                    hr = S_OK;        // That's fine if there aren't any more files to get

                break;    // We are done here.
            }
        }

        EVAL(SUCCEEDED(pwe->ReSetCodePages(&cmlc, m_pflHfpl)));
        InternetCloseHandle(hint);
    }
    else
    {
        // This will happen in two cases.
        // 1. The folder is empty. (GetLastError() == ERROR_NO_MORE_FILES)
        // 2. The user doesn't have enough access to view the folder. (GetLastError() == ERROR_INTERNET_EXTENDED_ERROR)
        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) != hr)
        {
            DisplayWininetError(phpi->hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_OPENFOLDER, IDS_FTPERR_WININET, MB_OK, NULL);
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);       // Clean error to indicate we already displayed the error and don't need to do it later.
            WININET_ASSERT(SUCCEEDED(hr));
        }
        else
            hr = S_OK;

        TraceMsg(TF_FTP_IDENUM, "CFtpEnum_New() - Can't opendir. hres=%#08lx.", hr);
    }

    if (phpi->psb)
        phpi->psb->SetStatusMessage(IDS_EMPTY, NULL);

    return hr;
}


/*****************************************************************************\
 *    CFtpEidl::_Init
\*****************************************************************************/
HRESULT CFtpEidl::_Init(void)
{
    HRESULT hr = S_FALSE;
    
    ASSERT(m_pfd);
    IUnknown_Set(&m_pflHfpl, NULL);
    m_pflHfpl = m_pfd->GetHfpl();       // Use cached copy if it exists.

    if (m_pflHfpl)
    {
        // We will just use the previous copy because we already have the contents.
        // TODO: Maybe we want to purge the results if a certain amount of time as ellapsed.
        m_fInited = TRUE;
        hr = S_OK;
    }
    else if (!m_pfd->GetFtpSite()->IsSiteBlockedByRatings(m_hwndOwner))
    {
        CFtpPidlList_Create(0, NULL, &m_pflHfpl);
        if (EVAL(m_pflHfpl))
        {
            CStatusBar * psb = GetCStatusBarFromDefViewSite(_punkSite);

            ASSERT(!m_pfd->IsRoot());
            //TraceMsg(TF_ALWAYS, "CFtpEidl::_Init() and enumerating");
            hr = m_pfd->WithHint(psb, m_hwndOwner, CFtpEidl::_PopulateItemCB, this, _punkSite, m_pff);
            if (SUCCEEDED(hr))
            {
                m_pfd->SetCache(m_pflHfpl);
                m_fInited = TRUE;
                hr = S_OK;
            }
            else
                IUnknown_Set(&m_pflHfpl, NULL);
        }
    }

    return hr;
}


/*****************************************************************************
 *    CFtpEidl::_NextOne
 *****************************************************************************/
LPITEMIDLIST CFtpEidl::_NextOne(DWORD * pdwIndex)
{
    LPITEMIDLIST pidl = NULL;
    LPITEMIDLIST pidlResult = NULL;

    if (m_pflHfpl)
    {
        while ((*pdwIndex < (DWORD) m_pflHfpl->GetCount()) && (pidl = m_pflHfpl->GetPidl(*pdwIndex)))
        {
            ASSERT(IsValidPIDL(pidl));
            (*pdwIndex)++;

            if (_fFilter(m_shcontf, FtpPidl_GetAttributes(pidl)))
            {
                pidlResult = ILClone(pidl);
                break;  // We don't need to search any more.
            }
        }
    }

    return pidlResult;
}


//===========================
// *** IEnumIDList Interface ***
//===========================

/*****************************************************************************
 *
 *    IEnumIDList::Next
 *
 *    Creates a brand new enumerator based on an existing one.
 *
 *
 *    OLE random documentation of the day:  IEnumXXX::Next.
 *
 *    rgelt - Receives an array of size celt (or larger).
 *
 *    "Receives an array"?  No, it doesn't receive an array.
 *    It *is* an array.  The array receives *elements*.
 *
 *    "Or larger"?  Does this mean I can return more than the caller
 *    asked for?  No, of course not, because the caller didn't allocate
 *    enough memory to hold that many return values.
 *
 *    No semantics are assigned to the possibility of celt = 0.
 *    Since I am a mathematician, I treat it as vacuous success.
 *
 *    pcelt is documented as an INOUT parameter, but no semantics
 *    are assigned to its input value.
 *
 *    The dox don't say that you are allowed to return *pcelt < celt
 *    for reasons other than "no more elements", but the shell does
 *    it everywhere, so maybe it's legal...
 *
 *****************************************************************************/
HRESULT CFtpEidl::Next(ULONG celt, LPITEMIDLIST * rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl = NULL;
    DWORD dwIndex;
    // The shell on pre-NT5 enums us w/o ole initialized which causes problems
    // when we call CoCreateInstance().  This happens in the thunking code
    // of encode.cpp when thunking strings.
    HRESULT hrOleInit = SHOleInitialize(0);

    if (m_fDead)
        return E_FAIL;

    if (!m_fInited)
    {
        hr = _Init();
        if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
        {
            // Did we need to redirect because of a new password or username?
            if (HRESULT_FROM_WIN32(ERROR_NETWORK_ACCESS_DENIED) == hr)
            {
                m_fDead = TRUE;
                hr = E_FAIL;
            }
            else if (!m_fErrorDisplayed)
            {
                DisplayWininetError(m_hwndOwner, FALSE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_GETDIRLISTING, IDS_FTPERR_WININET, MB_OK, NULL);
                m_fErrorDisplayed = TRUE;
            }
        }
    }

    if (S_OK == hr)
    {
        // Do they want more and do we have more to give?
        for (dwIndex = 0; (dwIndex < celt) && (pidl = _NextOne(&m_nIndex)); dwIndex++)
            rgelt[dwIndex] = pidl;  // Yes, so give away...

        if (pceltFetched)
            *pceltFetched = dwIndex;

        // Were we able to give any?
        if (0 == dwIndex)
            hr = S_FALSE;
    }

    SHOleUninitialize(hrOleInit);
    return hr;
}


/*****************************************************************************
 *    IEnumIDList::Skip
 *****************************************************************************/

HRESULT CFtpEidl::Skip(ULONG celt)
{
    m_nIndex += celt;

    return S_OK;
}


/*****************************************************************************
 *    IEnumIDList::Reset
 *****************************************************************************/

HRESULT CFtpEidl::Reset(void)
{
    m_fErrorDisplayed = FALSE;
    if (!m_fInited)
        _Init();

    m_nIndex = 0;
    return S_OK;
}


/*****************************************************************************\
 *    IEnumIDList::Clone
 *
 *    Creates a brand new enumerator based on an existing one.
\*****************************************************************************/
HRESULT CFtpEidl::Clone(IEnumIDList **ppenum)
{
    return CFtpEidl_Create(m_pfd, m_pff, m_hwndOwner, m_shcontf, m_nIndex, ppenum);
}


/*****************************************************************************\
 *    CFtpEidl_Create
 *
 *    Creates a brand new enumerator based on an ftp site.
\*****************************************************************************/
HRESULT CFtpEidl_Create(CFtpDir * pfd, CFtpFolder * pff, HWND hwndOwner, DWORD shcontf, IEnumIDList ** ppenum)
{
    CFtpEidl * pfe;
    HRESULT hres = CFtpEidl_Create(pfd, pff, hwndOwner, shcontf, &pfe);

    *ppenum = NULL;
    if (EVAL(pfe))
    {
        hres = pfe->QueryInterface(IID_IEnumIDList, (LPVOID *) ppenum);
        pfe->Release();
    }

    return hres;
}


/*****************************************************************************
 *
 *    CFtpEidl_Create
 *
 *    Creates a brand new enumerator based on an ftp site.
 *
 *****************************************************************************/

HRESULT CFtpEidl_Create(CFtpDir * pfd, CFtpFolder * pff, HWND hwndOwner, DWORD shcontf, CFtpEidl ** ppfe)
{
    CFtpEidl * pfe = new CFtpEidl();
    HRESULT hr = E_OUTOFMEMORY;

    ASSERT(pfd && pff && ppfe);
    *ppfe = pfe;
    if (EVAL(pfe))
    {
        ATOMICRELEASE(pfe->m_pm);
        pfe->m_pm = pff->GetIMalloc();

        IUnknown_Set(&pfe->m_pff, pff);
        IUnknown_Set(&pfe->m_pfd, pfd);
        pfe->m_pflHfpl = pfd->GetHfpl();

        pfe->m_shcontf = shcontf;
        pfe->m_hwndOwner = hwndOwner;

    }

    return hr;
}


/*****************************************************************************\
 *    CFtpEidl_Create
 *
 *    Creates a brand new enumerator based on an ftp site.
\*****************************************************************************/
HRESULT CFtpEidl_Create(CFtpDir * pfd, CFtpFolder * pff, HWND hwndOwner, DWORD shcontf, DWORD dwIndex, IEnumIDList ** ppenum)
{
    CFtpEidl * pfe;
    HRESULT hres = CFtpEidl_Create(pfd, pff, hwndOwner, shcontf, &pfe);

    if (EVAL(SUCCEEDED(hres)))
    {
        pfe->m_nIndex = dwIndex;

        hres = pfe->QueryInterface(IID_IEnumIDList, (LPVOID *) ppenum);
        ASSERT(SUCCEEDED(hres));

        pfe->Release();
    }

    return hres;
}


/****************************************************\
    Constructor
\****************************************************/
CFtpEidl::CFtpEidl() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_fInited);
    ASSERT(!m_nIndex);
    ASSERT(!m_shcontf);
    ASSERT(!m_pflHfpl);
    ASSERT(!m_pfd);
    ASSERT(!m_pm);
    ASSERT(!m_hwndOwner);
    ASSERT(!m_fInited);
    ASSERT(!m_fDead);

    m_hrOleInited = E_FAIL;
    LEAK_ADDREF(LEAK_CFtpEidl);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpEidl::~CFtpEidl()
{
    IUnknown_Set(&m_pflHfpl, NULL);
    IUnknown_Set(&m_pm, NULL);
    IUnknown_Set(&m_pfd, NULL);
    IUnknown_Set(&m_pff, NULL);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpEidl);
    SHCoUninitialize(m_hrOleInited);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpEidl::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpEidl::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpEidl::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumIDList))
    {
        *ppvObj = SAFECAST(this, IEnumIDList*);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite*);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpEidl::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
