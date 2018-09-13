#include "shellprv.h"
#pragma  hdrstop
#include "docfind.h"
#include "dsgetdc.h"
#include "ntdsapi.h"
#include "activeds.h"
#include "iadsp.h"
#include "lm.h"

#ifdef WINNT

//
// search the DS for computer objects
//

// BUGBUG: this should be const, but ADSI 
LPTSTR c_aszAttributes[] = { TEXT("DNShostname"), };

class CNetFindInDS : public IDFEnum
{
private:
    LPTSTR              _pszCompName;
    IDocFindFolder      *_pdfFolder;

    LONG                _cRef;
    IDirectorySearch*   _pds;
    IADsPathname*       _padp;
    ADS_SEARCH_HANDLE   _hSearch;

    // helper methods
    HRESULT _StrFromRow(LPTSTR pszProperty, LPTSTR pszBuffer, INT cchBuffer);
    HRESULT _InitSearch(LPCTSTR pszName);

public:
    CNetFindInDS(LPCTSTR pszCompName, IShellFolder *psf);
    ~CNetFindInDS();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IDFEnum methods ***
    STDMETHODIMP Next(LPITEMIDLIST *ppidl,
                       int *pcObjectSearched, int *pcFoldersSearched, BOOL *pfContinue,
                       int *pState, HWND hwnd);

    STDMETHODIMP Skip(int celt)
        { return E_NOTIMPL; };

    STDMETHODIMP Reset()
        { return E_NOTIMPL; };

    STDMETHODIMP StopSearch()
        { return E_NOTIMPL; };

    STDMETHODIMP_(BOOL) FQueryIsAsync()
        { return FALSE; };

    STDMETHODIMP GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone)
        { return E_NOTIMPL; };
        
    STDMETHODIMP GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl)
        { return E_NOTIMPL; };

    STDMETHODIMP GetExtendedDetailsOf(LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi)
        { return E_NOTIMPL; };

    STDMETHODIMP GetExtendedDetailsULong(LPCITEMIDLIST pidl, UINT iCol, ULONG *pul)
        { return E_NOTIMPL; };

    STDMETHODIMP GetItemID(UINT iItem, DWORD *puWorkID)
        { return E_NOTIMPL; };

    STDMETHODIMP SortOnColumn(UINT iCol, BOOL fAscending)
        { return E_NOTIMPL; };
};


//
// IUnknown
//

STDMETHODIMP CNetFindInDS::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);    
}

STDMETHODIMP_(ULONG) CNetFindInDS::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CNetFindInDS::Release()
{
   if (InterlockedDecrement(&_cRef))
        return _cRef;
 
    delete this;
    return 0;   
}


//
// construction / destruction
//

CNetFindInDS::CNetFindInDS(LPCTSTR pszCompName, IShellFolder* psf) :
    _pszCompName(NULL),
    _pdfFolder(NULL),
    _cRef(1),
    _pds(NULL),
    _hSearch(NULL)
{
    Str_SetPtr(&_pszCompName, pszCompName);
    psf->QueryInterface(IID_IDocFindFolder, (void **)&_pdfFolder);
}

CNetFindInDS::~CNetFindInDS()
{
    Str_SetPtr(&_pszCompName, NULL);

    if ( _hSearch )
        _pds->CloseSearchHandle(_hSearch);

    if ( _pds )
        _pds->Release();
    if ( _padp )
        _padp->Release();
    
    _pdfFolder->Release();
}


//
// given the ADS_SEARCH_COLUMN get a string back from it.
//

HRESULT CNetFindInDS::_StrFromRow(LPTSTR pszProperty, LPTSTR pszBuffer, INT cchBuffer)
{
    HRESULT hres = S_OK;
    ADS_SEARCH_COLUMN asc;

    hres = _pds->GetColumn(_hSearch, pszProperty, &asc);
    if ( SUCCEEDED(hres) )
    {
        switch ( asc.dwADsType )
        {
            case ADSTYPE_DN_STRING:
            case ADSTYPE_CASE_EXACT_STRING:
            case ADSTYPE_CASE_IGNORE_STRING:
            case ADSTYPE_PRINTABLE_STRING:
            case ADSTYPE_NUMERIC_STRING:
                StrCpyN(pszBuffer, asc.pADsValues[0].DNString, cchBuffer);
                break;

            default:
                hres = E_INVALIDARG;       // bad column type
                break;
        }
    }

    return hres;
}


//
// initialize the search ready with the path the GC:
//

HRESULT CNetFindInDS::_InitSearch(LPCTSTR pszName)
{
    HRESULT hres = E_FAIL;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;
    
    //
    // get the DNS of the domain forest we are part of
    //

    DWORD dwres = DsGetDcName(NULL, NULL, NULL, NULL, DS_RETURN_DNS_NAME|DS_DIRECTORY_SERVICE_REQUIRED, &pdci);
    if ( (NO_ERROR == dwres) && pdci->DnsForestName )
    {
        TCHAR szBuffer[MAX_PATH];
        wnsprintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("GC://%s"), pdci->DnsForestName);        

        hres = ADsOpenObject(szBuffer, 
                             NULL, NULL, ADS_SECURE_AUTHENTICATION,
                             IID_IDirectorySearch, (void **)&_pds);

        if ( SUCCEEDED(hres) )
        {
            ADS_SEARCHPREF_INFO prefInfo[3];

            //
            // do a sub tree, async paged search.
            //

            prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
            prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
            prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

            prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;     // async
            prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
            prefInfo[1].vValue.Boolean = TRUE;

            prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
            prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
            prefInfo[2].vValue.Integer = 32;

            hres = _pds->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));        
            if ( SUCCEEDED(hres) )
            {
                //
                // build a filter that will find all the computer objects whose name starts
                // or ends with the string we have been given, the DS is bad at "contains"
                //

                wnsprintf(szBuffer, ARRAYSIZE(szBuffer), 
                          TEXT("(&(sAMAccountType=805306369)(|(SAMAccountName=%s*)(SAMAccountName=*%s)))"),
                          pszName, pszName);

                hres = _pds->ExecuteSearch(szBuffer, c_aszAttributes, ARRAYSIZE(c_aszAttributes), &_hSearch);
            }
        }

        NetApiBufferFree(pdci);
    }

    return hres;
}


//
// itterate over results from a DS search
//

STDMETHODIMP CNetFindInDS::Next(LPITEMIDLIST *ppidl, int *pcObjectSearched, int *pcFoldersSearched, 
                                     BOOL *pfContinue, int *pState, HWND hwnd)
{
    HRESULT hres = S_OK;
    WCHAR szName[MAX_PATH], szUNC[MAX_PATH+2];

    *pState = GNF_DONE;
    *pfContinue = FALSE;

    //
    // if _hSearch == NULL then we have not searched yet, therefore initialize the
    // query against the DS.
    //

    if ( !_hSearch )
        hres = _InitSearch(_pszCompName);

    while ( SUCCEEDED(hres) && _hSearch )
    {
        hres = _pds->GetNextRow(_hSearch);
        if ( SUCCEEDED(hres) )
        {                    
            if ( (hres == S_ADS_NOMORE_ROWS) )
                break;

            *pcObjectSearched += 1;                    // we have another object

            //
            // get the DNSHostName of the object, this we can then use to construct a
            // UNC name for the object.   If this fails then we skip the result (the
            // column may not be defined, or perhaps its not a string, eitherway
            // we should ignore it).
            //

            if ( SUCCEEDED(_StrFromRow(TEXT("DNShostName"), szName, ARRAYSIZE(szName))) )
            {
                LPITEMIDLIST pidl;
                INT iFolder;    
                
                wnsprintf(szUNC, ARRAYSIZE(szUNC), TEXT("\\\\%s"), szName);

                pidl = ILCreateFromPath(szUNC);

                if ( !pidl )
                    continue;

                LPITEMIDLIST pidlParent = ILCloneParent(pidl);

                if (!pidlParent)
                    continue;

                hres = _pdfFolder->AddFolderToFolderList(pidl, FALSE, &iFolder);  
                ILFree(pidlParent);
                if ( SUCCEEDED(hres) )
                {
                    //  append attempts to resize an existing pidl
                    //  it will return NULL on failure.
                    *ppidl = DocFind_AppendIFolder(ILClone(ILFindLastID(pidl)), iFolder);
                    
                    if ( *ppidl )
                    {
                        ILFree(pidl);
                        *pState = GNF_MATCH;
                        *pfContinue = TRUE;
        
                        break;                  // success we have a result
                    }
                }

                ILFree(pidl);
                ILFree(*ppidl);
            }    
        }
    }

    return hres;
}

#endif


//
// return an enumerator if the user is logged into the DS, if they are
// not then we return S_FALSE.
//

STDAPI CNetFindInDS_CreateInstance(LPCTSTR pszCompName, IShellFolder *psf, IDFEnum **ppdfe)
{
#ifdef WINNT
    *ppdfe = NULL;

    if ( !GetEnvironmentVariable(TEXT("USERDNSDOMAIN"), NULL, 0) )
        return S_FALSE;

    *ppdfe = new CNetFindInDS(pszCompName, psf);
    if ( !*ppdfe )
        return E_OUTOFMEMORY;

    return S_OK;
#else
    return S_FALSE;         // S_FALSE == use old search
#endif
}
