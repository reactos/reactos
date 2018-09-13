/*
 * isbase.cpp - IUnknown implementation for Intshcut class.
 */

#include "priv.h"
#include "sccls.h"
#include "ishcut.h"


HRESULT IsProtocolRegistered(LPCTSTR pcszProtocol)
{
    HRESULT hres = S_OK;

    ASSERT(IS_VALID_STRING_PTR(pcszProtocol, -1));

    if (NO_ERROR != SHGetValue(HKEY_CLASSES_ROOT, pcszProtocol, TEXT("URL Protocol"),
                               NULL, NULL, NULL))
    {
        TraceMsg(TF_ERROR, "IsProtocolRegistered(): Protocol \"%s\" is not registered.",
                 pcszProtocol);

        hres = URL_E_UNREGISTERED_PROTOCOL;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Takes the given URL and returns an allocated string
         containing the protocol.  Also optionally returns the
         parsed-url structure.

Returns: S_OK if the URL was parsed
Cond:    --
*/
STDAPI
CopyURLProtocol(
    IN  LPCTSTR     pcszURL,
    OUT LPTSTR *    ppszProtocol,
    OUT PARSEDURL * ppu)            OPTIONAL
{
    HRESULT hr;
    PARSEDURL pu;

    ASSERT(IS_VALID_STRING_PTR(pcszURL, -1));
    ASSERT(IS_VALID_WRITE_PTR(ppszProtocol, PTSTR));

    if (NULL == ppu)
        ppu = &pu;

    *ppszProtocol = NULL;

    ppu->cbSize = SIZEOF(*ppu);
    hr = ParseURL(pcszURL, ppu);

    if (hr == S_OK)
    {
        *ppszProtocol = (LPTSTR)LocalAlloc(LPTR, CbFromCch(ppu->cchProtocol + 1));
        if (*ppszProtocol)
        {
            // Just copy the protocol portion of string
            StrCpyN(*ppszProtocol, ppu->pszProtocol, ppu->cchProtocol + 1);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    ASSERT(FAILED(hr) ||
           (hr == S_OK &&
            IS_VALID_STRING_PTR(*ppszProtocol, -1)));

    return(hr);
}


HRESULT ValidateURL(LPCTSTR pcszURL)
{
    HRESULT hr;
    LPTSTR pszProtocol;

    ASSERT(IS_VALID_STRING_PTR(pcszURL, -1));

    hr = CopyURLProtocol(pcszURL, &pszProtocol, NULL);

    if (hr == S_OK)
    {
        hr = IsProtocolRegistered(pszProtocol);

        LocalFree(pszProtocol);
    }

    return(hr);
}

HRESULT ValidateWorkingDirectory(LPCTSTR pcszWorkingDirectory)
{
    ASSERT(IS_VALID_STRING_PTR(pcszWorkingDirectory, -1));

    return(PathIsDirectory(pcszWorkingDirectory) ? S_OK : E_PATH_NOT_FOUND);
}

#define SUBS_DEL_TIMEOUT 3000

//  BUGBUG - We need to hold a ref on shdocvw.dll in case we get unloaded
//  before our timeout.  rfirth is adding ExitThreadAndFreeLibrary to the
//  thread pool apis.
void DeleteSubsOnShortcutDelete(void *pData, BOOLEAN)
{
    IS_SUBS_DEL_DATA *pSubsDelData = (IS_SUBS_DEL_DATA *)pData;
    ASSERT(NULL != pData);
    ASSERT(0 != pSubsDelData->m_szFile[0]);
    ASSERT(0 != pSubsDelData->m_pwszURL[0]);

    if ((((DWORD)-1) == GetFileAttributes(pSubsDelData->m_szFile)) &&
        (ERROR_FILE_NOT_FOUND == GetLastError()))
    {
        if (SUCCEEDED(CoInitialize(NULL)))
        {
            ISubscriptionMgr2 *pSubsMgr2;

            if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr,
                                           NULL,
                                           CLSCTX_INPROC_SERVER,
                                           IID_ISubscriptionMgr2,
                                           (void **)&pSubsMgr2)))
            {
                pSubsMgr2->DeleteSubscription(pSubsDelData->m_pwszURL, NULL);
                pSubsMgr2->Release();
            }
            CoUninitialize();
        }
    }
    delete pSubsDelData;
}

#ifdef DEBUG

BOOL IsValidPCIntshcut(PCIntshcut pcintshcut)
{
    return(IS_VALID_READ_PTR(pcintshcut, CIntshcut) &&
           FLAGS_ARE_VALID(pcintshcut->m_dwFlags, ISF_ALL) &&
           (! pcintshcut->m_pszFile ||
            IS_VALID_STRING_PTR(pcintshcut->m_pszFile, -1)) &&
           EVAL(! pcintshcut->m_pszFolder ||
                IsValidPath(pcintshcut->m_pszFolder)) &&
           EVAL(! pcintshcut->m_pprop ||
                IS_VALID_STRUCT_PTR(pcintshcut->m_pprop, CIntshcutProp)) &&
           EVAL(! pcintshcut->m_psiteprop ||
                IS_VALID_STRUCT_PTR(pcintshcut->m_psiteprop, CIntsiteProp)));
}

#endif

Intshcut::Intshcut(void) : m_cRef(1)
{
    DllAddRef();
   // Intshcut objects should always be allocated
   ASSERT(ISF_DEFAULT == m_dwFlags);
   ASSERT(NULL == m_pszFile);
   ASSERT(NULL == m_pszFolder);
   ASSERT(NULL == m_pprop);
   ASSERT(NULL == m_psiteprop);
   ASSERT(NULL == _punkSite);
   ASSERT(NULL == m_pszTempFileName);
   ASSERT(NULL == m_pszDescription);
   ASSERT(NULL == m_pszFileToLoad);
   ASSERT(!m_fMustLoadSync);
   ASSERT(!m_bCheckForDelete);
   // Init our registered data formats
   InitClipboardFormats();

   ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

   return;
}

Intshcut::~Intshcut(void)
{
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    if (m_bCheckForDelete)
    {
        if (NULL != m_pszFile)
        {
            IS_SUBS_DEL_DATA *pSubsDelData = new IS_SUBS_DEL_DATA;

            if (NULL != pSubsDelData)
            {
                HRESULT hr = GetURL(&pSubsDelData->m_pwszURL);

                if (SUCCEEDED(hr))
                {
                    StrCpyN(pSubsDelData->m_szFile, m_pszFile, ARRAYSIZE(pSubsDelData->m_szFile));
                    HANDLE hTimer = SHSetTimerQueueTimer(NULL,
                                                         DeleteSubsOnShortcutDelete,
                                                         pSubsDelData,
                                                         SUBS_DEL_TIMEOUT,
                                                         0,
                                                         NULL,
                                                         FALSE);
                    if (NULL == hTimer)
                    {
                        hr = E_FAIL;
                    }
                }

                if (FAILED(hr))
                {
                    delete pSubsDelData;
                }
                else
                {
                    DbgRemoveFromMemList((LPVOID)(pSubsDelData)); // We do not free this guy
                }
            }
        }
        else
        {
            ASSERT(FALSE);  //  m_bCheckForDelete only gets set to TRUE in the context menu code
        }
    }

    Str_SetPtr(&m_pszFile, NULL);
    Str_SetPtr(&m_pszFileToLoad, NULL);
    if (m_pprop)
    {
        delete m_pprop;
        m_pprop = NULL;
    }

    if (m_psiteprop)
    {
        delete m_psiteprop;
        m_psiteprop = NULL;
    }

    if (m_pInitDataObject)
    {
        m_pInitDataObject->Release();
        m_pInitDataObject = NULL;
    }

    SetSite(NULL);

    if(m_pszTempFileName)
    {
        DeleteFile(m_pszTempFileName);
        Str_SetPtr(&m_pszTempFileName, NULL);
    }

    Str_SetPtr(&m_pszFolder, NULL);
    Str_SetPtr(&m_pszDescription, NULL);
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    ATOMICRELEASE(_punkLink);
    
    DllRelease();

    return;
}

/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface handler for Intshcut

Returns:
Cond:    --
*/
STDMETHODIMP Intshcut::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    BOOL fMustLoadSyncNow = TRUE;

    if (IsEqualIID(riid, IID_IExtractIconW) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        fMustLoadSyncNow = FALSE;
        *ppvObj = SAFECAST(this, IExtractIconW *);
    }
    else if (IsEqualIID(riid, IID_IExtractIconA))
    {
        fMustLoadSyncNow = FALSE;
        *ppvObj = SAFECAST(this, IExtractIconA *);
    }
    else if (IsEqualIID(riid, IID_IContextMenu) ||
             IsEqualIID(riid, IID_IContextMenu2))
    {
        *ppvObj = SAFECAST(this, IContextMenu2 *);
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
        *ppvObj = SAFECAST(this, IDataObject *);
    }
    else if (IsEqualIID(riid, IID_INewShortcutHookW))
    {
        *ppvObj = SAFECAST(this, INewShortcutHookW *);
    }
    else if (IsEqualIID(riid, IID_INewShortcutHookA))
    {
        *ppvObj = SAFECAST(this, INewShortcutHookA *);
    }
    else if (IsEqualIID(riid, IID_IPersistFile) ||
             IsEqualIID(riid, IID_IPersist))
    {
        fMustLoadSyncNow = FALSE;
        *ppvObj = SAFECAST(this, IPersistFile *);
    }
    else if (IsEqualIID(riid, IID_IPersistStream))
    {
        *ppvObj = SAFECAST(this, IPersistStream *);
    }
    else if (IsEqualIID(riid, IID_IPropertySetStorage))
    {
        *ppvObj = SAFECAST(this, IPropertySetStorage *);
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppvObj = SAFECAST(this, IShellExtInit *);
    }
    else if (IsEqualIID(riid, IID_IShellLinkA))
    {
        *ppvObj = SAFECAST(this, IShellLinkA *);
    }
    else if (IsEqualIID(riid, IID_IShellLinkW))
    {
        *ppvObj = SAFECAST(this, IShellLinkW *);
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppvObj = SAFECAST(this, IShellPropSheetExt *);
    }
    else if (IsEqualIID(riid, IID_IUniformResourceLocatorA))
    {
        *ppvObj = SAFECAST(this, IUniformResourceLocatorA *);
    }
    else if (IsEqualIID(riid, IID_IUniformResourceLocatorW))
    {
        *ppvObj = SAFECAST(this, IUniformResourceLocatorW *);
    }
    else if (IsEqualIID(riid, IID_IQueryInfo))
    {
        *ppvObj = SAFECAST(this, IQueryInfo *);
    }
    else if (IsEqualIID(riid, IID_IQueryCodePage))
    {
        *ppvObj = SAFECAST(this, IQueryCodePage *);
    }
    else if (IsEqualIID(riid, IID_INamedPropertyBag))
    {
        *ppvObj = SAFECAST(this, INamedPropertyBag *);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite *);
    }
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
    {
        *ppvObj = SAFECAST(this, IOleCommandTarget *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    m_fMustLoadSync |= fMustLoadSyncNow;
    if (m_fMustLoadSync)
    {
        LoadFromAsyncFileNow();
        ASSERT(NULL == m_pszFileToLoad);
    }
    return NOERROR;
}

STDMETHODIMP_(ULONG) Intshcut::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) Intshcut::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP Intshcut::InitProp()
{
    HRESULT hres;

    if (m_pprop)
        hres = S_OK;
    else
    {
        m_pprop = new IntshcutProp;
        if (m_pprop)
        {
            // m_pszFile may be NULL here
            hres = m_pprop->InitFromFile(m_pszFile);
            if (FAILED(hres))
            {
                delete m_pprop;
                m_pprop = NULL;
            }
        }
        else
            hres = E_OUTOFMEMORY;
    }

    return hres;
}

STDMETHODIMP Intshcut::InitSiteProp(void)
{
    HRESULT hres = InitProp();
    if (SUCCEEDED(hres))
    {
        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        hres = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));
        if (NULL == m_psiteprop && SUCCEEDED( hres ))
        {
            m_psiteprop = new IntsiteProp;
            if (m_psiteprop)
            {
                hres = m_psiteprop->InitFromDB(szURL, this, TRUE);
                if (FAILED(hres))
                {
                    delete m_psiteprop;
                    m_psiteprop = NULL;
                }
            }
            else
                hres = E_OUTOFMEMORY;
        }
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: Only copy the property if it is different.  Return
         TRUE if it was.

*/
BOOL CopyChangedProperty(IntshcutProp * pprop, PROPID pid,
                         IntsiteProp * psiteprop, PROPID pidSite,
                         BOOL bCopyToDB)
{
    BOOL bRet = FALSE;
    TCHAR szBuf[1024];
    TCHAR szBufSite[1024];

    pprop->GetProp(pid, szBuf, SIZECHARS(szBuf));
    psiteprop->GetProp(pidSite, szBufSite, SIZECHARS(szBufSite));
    StrTrim(szBuf, TEXT(" "));
    StrTrim(szBufSite, TEXT(" "));
    if (StrCmp(szBuf, szBufSite))
    {
        if (bCopyToDB)
            psiteprop->SetProp(pidSite, szBuf);
        else
            pprop->SetProp(pid, szBufSite);
        bRet = TRUE;
    }
    return bRet;
}


/*----------------------------------------------------------
Purpose: Mirror the following properties between FMTID_INTSHCUT
         and FMTID_INTSITE:

            PID_IS_WHATSNEW     <---->  PID_INTSITE_WHATSNEW
            PID_IS_DESCRIPTION  <---->  PID_INTSITE_DESCRIPTION
            PID_IS_AUTHOR       <---->  PID_INTSITE_AUTHOR
            PID_IS_COMMENT      <---->  PID_INTSITE_COMMENT

Returns:
Cond:    --
*/
STDMETHODIMP Intshcut::MirrorProperties(void)
{
    HRESULT hres = InitSiteProp();
    if (SUCCEEDED(hres))
    {
        STATPROPSETSTG statSite;
        STATPROPSETSTG stat;
        LONG lRet;

        // Get the times that the properties were set.  The later
        // time becomes the source.
        m_psiteprop->Stat(&statSite);
        m_pprop->Stat(&stat);

        // Don't do anything if the times are equal

        lRet = CompareFileTime(&stat.mtime, &statSite.mtime);
        if (0 != lRet)
        {
            BOOL bChanged = FALSE;
            BOOL bCopyToDB = (0 < lRet);

            bChanged |= CopyChangedProperty(m_pprop, PID_IS_WHATSNEW, m_psiteprop, PID_INTSITE_WHATSNEW, bCopyToDB);
            bChanged |= CopyChangedProperty(m_pprop, PID_IS_DESCRIPTION, m_psiteprop, PID_INTSITE_DESCRIPTION, bCopyToDB);
            bChanged |= CopyChangedProperty(m_pprop, PID_IS_AUTHOR, m_psiteprop, PID_INTSITE_AUTHOR, bCopyToDB);
            bChanged |= CopyChangedProperty(m_pprop, PID_IS_COMMENT, m_psiteprop, PID_INTSITE_COMMENT, bCopyToDB);

            if (bChanged)
            {
                if (bCopyToDB)
                {
                    m_psiteprop->SetTimes(&stat.mtime, NULL, NULL);
                    m_psiteprop->Commit(STGC_DEFAULT);

                    TraceMsg(TF_INTSHCUT, "Mirroring properties of %s to the central database", Dbg_SafeStr(m_pszFile));
                }
                else
                {
                    m_pprop->SetTimes(&statSite.mtime, NULL, NULL);
                    m_pprop->Commit(STGC_DEFAULT);

                    TraceMsg(TF_INTSHCUT, "Mirroring properties of %s to the .url file", Dbg_SafeStr(m_pszFile));
                }
            }
        }

        hres = S_OK;
    }

    return hres;
}


STDMETHODIMP_(void) Intshcut::ChangeNotify(LONG wEventId, UINT uFlags)
{
    if (m_pszFile)
        SHChangeNotify(wEventId, uFlags | SHCNF_PATH, m_pszFile, 0);
}


STDAPI
CIntShcut_CreateInstance(
    IUnknown * punkOuter,
    IUnknown ** ppunk,
    LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    HRESULT hres = E_OUTOFMEMORY;
    Intshcut *pis = new Intshcut;
    if (pis)
    {
        *ppunk = SAFECAST(pis, IDataObject *);
        hres = S_OK;
    }
    return hres;
}


