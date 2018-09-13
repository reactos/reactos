//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

/*Included Files------------------------------------------------------------*/
#include "msrating.h"
#include "ratings.h"
#include <npassert.h>
#include <npstring.h>

#include "mslubase.h"
#include "roll.h"
#include "rors.h"
#include "picsrule.h"

typedef HRESULT (STDAPICALLTYPE *PFNCoInitialize)(LPVOID pvReserved);
typedef void (STDAPICALLTYPE *PFNCoUninitialize)(void);
typedef HRESULT (STDAPICALLTYPE *PFNCoGetMalloc)(
    DWORD dwMemContext, LPMALLOC FAR* ppMalloc);
typedef HRESULT (STDAPICALLTYPE *PFNCoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                    DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);
typedef HRESULT (STDAPICALLTYPE *PFNCLSIDFromString)(LPOLESTR lpsz, LPCLSID pclsid);

PFNCoInitialize pfnCoInitialize = NULL;
PFNCoUninitialize pfnCoUninitialize = NULL;
PFNCoGetMalloc pfnCoGetMalloc = NULL;
PFNCoCreateInstance pfnCoCreateInstance = NULL;
PFNCLSIDFromString pfnCLSIDFromString = NULL;

#undef CoInitialize
#undef CoUninitialize
#undef CoGetMalloc
#undef CoCreateInstance
#undef CLSIDFromString

#define CoInitialize pfnCoInitialize
#define CoUninitialize pfnCoUninitialize
#define CoGetMalloc pfnCoGetMalloc
#define CoCreateInstance pfnCoCreateInstance
#define CLSIDFromString pfnCLSIDFromString

struct {
    FARPROC *ppfn;
    LPCSTR pszName;
} aOLEImports[] = {
    { (FARPROC *)&pfnCoInitialize, "CoInitialize" },
    { (FARPROC *)&pfnCoUninitialize, "CoUninitialize" },
    { (FARPROC *)&pfnCoGetMalloc, "CoGetMalloc" },
    { (FARPROC *)&pfnCoCreateInstance, "CoCreateInstance" },
    { (FARPROC *)&pfnCLSIDFromString, "CLSIDFromString" },
};

const UINT cOLEImports = sizeof(aOLEImports) / sizeof(aOLEImports[0]);

HINSTANCE hOLE32 = NULL;
BOOL fTriedOLELoad = FALSE;

BOOL LoadOLE(void)
{
    if (fTriedOLELoad)
        return (hOLE32 != NULL);

    fTriedOLELoad = TRUE;

    hOLE32 = ::LoadLibrary("OLE32.DLL");
    if (hOLE32 == NULL)
        return FALSE;

    for (UINT i=0; i<cOLEImports; i++) {
        *(aOLEImports[i].ppfn) = ::GetProcAddress(hOLE32, aOLEImports[i].pszName);
        if (*(aOLEImports[i].ppfn) == NULL) {
            CleanupOLE();
            return FALSE;
        }
    }

    return TRUE;
}


void CleanupOLE(void)
{
    if (hOLE32 != NULL) {
        for (UINT i=0; i<cOLEImports; i++) {
            *(aOLEImports[i].ppfn) = NULL;
        }
        ::FreeLibrary(hOLE32);
        hOLE32 = NULL;
    }
}


/*Obtain Rating Data--------------------------------------------------------*/
class RatingObtainData
{
    public:
        NLS_STR  nlsTargetUrl;
        HANDLE   hAbortEvent;
        DWORD    dwUserData;
        void (*fCallback)(DWORD dwUserData, HRESULT hr, LPCTSTR pszRating, LPVOID lpvRatingDetails) ;

        RatingObtainData(LPCTSTR pszTargetUrl);
        ~RatingObtainData();
};

RatingObtainData::RatingObtainData(LPCTSTR pszTargetUrl)
    : nlsTargetUrl(pszTargetUrl)
{
    hAbortEvent  = NULL;
    dwUserData   = 0;
    fCallback    = NULL;
}
RatingObtainData::~RatingObtainData()
{
    if (hAbortEvent) CloseHandle(hAbortEvent);
}


struct RatingHelper {
    CLSID clsid;
    DWORD dwSort;
};

array<RatingHelper> *paRatingHelpers = NULL;
BOOL fTriedLoadingHelpers = FALSE;


void InitRatingHelpers()
{
    if (fTriedLoadingHelpers || !LoadOLE())
        return;

    fTriedLoadingHelpers = TRUE;

    paRatingHelpers = new array<RatingHelper>;
    if (paRatingHelpers == NULL)
        return;

    HKEY hkey;

    /* BUGBUG - should this be in the policy file?  it shouldn't be per-user, that's for sure. */
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szRATINGHELPERS, &hkey) != ERROR_SUCCESS)
        return;

    UINT iValue = 0;
    LONG err = ERROR_SUCCESS;
    char szValue[39];       /* just big enough for a null-terminated GUID string */
    WCHAR wszValue[39];     /* unicode version */

    /* Note, special care is taken to make sure that we only CoInitialize for
     * as long as we need to, and CoUninitialize when we're done.  We cannot
     * CoUninitialize on a different thread than we initialized on, nor do we
     * want to call CoUninitialize at thread-detach time (would require using
     * TLS).  This is done here and in the asynchronous thread that actually
     * calls into the rating helpers to get ratings.
     */
    BOOL fCOMInitialized = FALSE;

    do {
        DWORD cchValue = sizeof(szValue);
        err = RegEnumValue(hkey, iValue, szValue, &cchValue, NULL, NULL, NULL, NULL);
        if (err == ERROR_SUCCESS) {
            if (!fCOMInitialized) {
                if (FAILED(CoInitialize(NULL)))
                    break;
                fCOMInitialized = TRUE;
            }

            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szValue, -1, wszValue, sizeof(wszValue))) {
                RatingHelper helper;
                if (SUCCEEDED(CLSIDFromString(wszValue, &helper.clsid))) {
                    IObtainRating *pHelper;

                    if (SUCCEEDED(CoCreateInstance(helper.clsid, NULL,
                                                   CLSCTX_INPROC_SERVER,
                                                   IID_IObtainRating,
                                                   (LPVOID *)&pHelper))) {
                        helper.dwSort = pHelper->GetSortOrder();
                        if (!paRatingHelpers->Append(helper))
                            err = ERROR_NOT_ENOUGH_MEMORY;
                        pHelper->Release();
#ifdef DEBUG
                        pHelper = NULL;
#endif
                    }
                }
            }
        }
        iValue++;
    } while (err == ERROR_SUCCESS);

    if (fCOMInitialized)
        CoUninitialize();

    /* If more than one helper, sort them by their reported sort orders.
     * We will rarely have more than two or three of these guys, and this
     * is one time code, so we don't need a super-slick sort algorithm.
     *
     * CODEWORK: could modify array<> template to support an Insert()
     * method which reallocates the buffer like Append() does, but inserts
     * at a specific location.
     */
    if (paRatingHelpers->Length() > 1) {
        for (INT i=0; i < paRatingHelpers->Length() - 1; i++) {
            for (INT j=i+1; j < paRatingHelpers->Length(); j++) {
                if ((*paRatingHelpers)[i].dwSort > (*paRatingHelpers)[j].dwSort) {
                    RatingHelper temp = (*paRatingHelpers)[i];
                    (*paRatingHelpers)[i] = (*paRatingHelpers)[j];
                    (*paRatingHelpers)[j] = temp;
                }
            }
        }
    }

    RegCloseKey(hkey);
}


void CleanupRatingHelpers(void)
{
    if (paRatingHelpers != NULL) {
        delete paRatingHelpers;
        paRatingHelpers = NULL;
    }
    fTriedLoadingHelpers = FALSE;
}


/*
    This procedure runs on its own thread (1 per request).
    This cycles through all the helper DLLs looking for a ratings.
    It goes on down the list one at a time until either a rating 
    is found, or the this is aborted by the programmer.
*/
DWORD __stdcall RatingCycleThread(LPVOID pData)
{
    RatingObtainData *pOrd = (RatingObtainData*) pData;
    LPVOID            lpvRatingDetails = NULL;
    HRESULT           hrRet;
    int               nProc;
    BOOL              fAbort;
    IMalloc *pAllocator = NULL;
    LPSTR pszRating = NULL;
    BOOL fCOMInitialized;

    ASSERT(pOrd);

    /* Note that CoInitialize and CoUninitialize must be done once per thread. */
    if (SUCCEEDED(CoInitialize(NULL))) {
        fCOMInitialized = TRUE;
        hrRet = CoGetMalloc(MEMCTX_TASK, &pAllocator);
        if (SUCCEEDED(hrRet)) {

            //Cycle through list of rating procs till one gives us the answer, we abort, or there are no more
            int nRatingHelperProcs = ::paRatingHelpers->Length();
            for (nProc = 0; nProc < nRatingHelperProcs; ++nProc)
            {
                IObtainRating *pHelper;
                if (SUCCEEDED(CoCreateInstance((*paRatingHelpers)[nProc].clsid, NULL,
                                               CLSCTX_INPROC_SERVER,
                                               IID_IObtainRating,
                                               (LPVOID *)&pHelper))) {
                    hrRet  = pHelper->ObtainRating(pOrd->nlsTargetUrl.QueryPch(),
                                    pOrd->hAbortEvent, pAllocator, &pszRating);
                    pHelper->Release();
#ifdef DEBUG
                    pHelper = NULL;
#endif
                }
                fAbort = (WAIT_OBJECT_0 == WaitForSingleObject(pOrd->hAbortEvent, 0));
                if (fAbort || SUCCEEDED(hrRet)) break;
            }
        }
    }
    else
        hrRet = E_RATING_NOT_FOUND;

    /*return results to user*/
    if (!fAbort)
    {
        /*
         * If one of the providers found a rating, we must call CheckUserAccess
         * and tell the client whether the user has access or not.  If we did
         * not find a rating, then we tell the client that, by passing the
         * callback a code of E_RATING_NOT_FOUND.
         *
         * The provider may also return S_RATING_ALLOW or S_RATING_DENY, which
         * means that it has already checked the user's access (for example,
         * against a system-wide exclusion list).
         */
        if (hrRet == S_RATING_FOUND)
        {
            hrRet = RatingCheckUserAccess(NULL, pOrd->nlsTargetUrl.QueryPch(),
                                          pszRating, NULL, PICS_LABEL_FROM_BUREAU,
                                          &lpvRatingDetails);
        }

        /* Range-check other success codes to make sure they're not anything
         * that the browser callback isn't expecting.
         */
        if (SUCCEEDED(hrRet) && (hrRet != S_RATING_ALLOW && hrRet != S_RATING_DENY))
            hrRet = E_RATING_NOT_FOUND;
        (*pOrd->fCallback)(pOrd->dwUserData, hrRet, pszRating, (LPVOID) lpvRatingDetails);
    }

    /*cleanup*/
    delete pOrd;

    if (pAllocator != NULL) {
        pAllocator->Free(pszRating);
        pAllocator->Release();
    }

    if (fCOMInitialized)
        CoUninitialize();

    return (DWORD) fAbort;
}




/*Public Functions----------------------------------------------------------*/

//Startup thread that finds rating, return immediately
HRESULT WINAPI RatingObtainQuery(LPCTSTR pszTargetUrl, DWORD dwUserData, void (*fCallback)(DWORD dwUserData, HRESULT hr, LPCTSTR pszRating, LPVOID lpvRatingDetails), HANDLE *phRatingObtainQuery)
{
    RatingObtainData *pOrd;
    HANDLE hThread;
    DWORD dwThid;

    CheckGlobalInfoRev();

    if (::RatingEnabledQuery() != S_OK ||
        !gPRSI->fSettingsValid)         /* ratings not enabled? fail immediately. */
        return E_RATING_NOT_FOUND;

    InitRatingHelpers();

    if (::paRatingHelpers == NULL || ::paRatingHelpers->Length() < 1)
        return E_RATING_NOT_FOUND;

    if (fCallback && pszTargetUrl)
    {
        pOrd = new RatingObtainData(pszTargetUrl);
        if (pOrd)
        {
            if (pOrd->nlsTargetUrl.QueryError() == ERROR_SUCCESS) {
                pOrd->dwUserData   = dwUserData;
                pOrd->fCallback    = fCallback;
                pOrd->hAbortEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

                if (pOrd->hAbortEvent)
                {
                    hThread = CreateThread(NULL, 0, RatingCycleThread, (LPVOID) pOrd, 0, &dwThid);
                    if (hThread)
                    {
                        CloseHandle(hThread);
                        if (phRatingObtainQuery) *phRatingObtainQuery = pOrd->hAbortEvent;
                        return NOERROR;
                    }
                    CloseHandle(pOrd->hAbortEvent);
                }
            }
            delete pOrd;
        }
    }

    return E_FAIL;
}

//Cancel an existing query
HRESULT WINAPI RatingObtainCancel(HANDLE hRatingObtainQuery)
{
    //what happens if hRatingObtainQuery has already been closed?!?!
    if (hRatingObtainQuery)
    {
        if (SetEvent(hRatingObtainQuery)) return NOERROR;
    }
    return E_HANDLE;
}

