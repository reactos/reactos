//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       hlinkez.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//              5-15-96   Ramesh G -  Major modifications
//              5-17-96   Ramesh G -  Added Frames support
//                        Ramesh G -  Modified variable names to Hungarian Notation
//              6-19-96   Ramesh G -  Modifications
//              7-25-96   Ramesh G -  Modifications
//                              8-05-96   Ramesh G -  Merged HlinkSimple...String() and Moniker()
//                                                                        HlinkSimpleNavigateToString() creates the moniker
//                                                                        and calls HlinkSimpleNavigateToMoniker()
//----------------------------------------------------------------------------
#define USE_SYSTEM_URL_MONIKER
#define INITGUID
#define STR_SIZE        20

#include "hlink.h"
#include "ocidl.h"
#include "docobj.h"
#include "exdisp.h"
#include "shellapi.h"
#include "servprov.h"
#include "urlhlink.h"
#include "htiface.h"
#include "wininet.h"
#include <shlwapi.h>
#include <shlwapip.h>

#include "sdll.hxx"

class HLinkDll
{
public:
    HLinkDll();
        HRESULT HlinkCreateFromMoniker(
                                IMoniker* pmkSource,
                                LPCWSTR szLocation,
                                LPCWSTR szFriendlyName,
                                IHlinkSite* phlSite,
                                DWORD dwSiteData,
                                IUnknown* punkOuter,
                                REFIID riid,
                                void** ppv);
private:

    BOOL LoadFunc( LPCSTR lpProcName, FARPROC & fp );
    typedef HRESULT (STDAPICALLTYPE * LPFNHlinkCreateFromMoniker)(
                                IMoniker* pmkSource,
                                LPCWSTR szLocation,
                                LPCWSTR szFriendlyName,
                                IHlinkSite* phlSite,
                                DWORD dwSiteData,
                                IUnknown* punkOuter,
                                REFIID riid,
                                void** ppv);
    LPFNHlinkCreateFromMoniker m_lpfnHlinkCreateFromMoniker;

    HMODULE m_hmodule;
    BOOL    m_error;

};

inline HRESULT HLinkDll::HlinkCreateFromMoniker(
                                IMoniker* pmkSource,
                                LPCWSTR szLocation,
                                LPCWSTR szFriendlyName,
                                IHlinkSite* phlSite,
                                DWORD dwSiteData,
                                IUnknown* punkOuter,
                                REFIID riid,
                                void** ppv)
{

    if( !LoadFunc("HlinkCreateFromMoniker",*(FARPROC*)&m_lpfnHlinkCreateFromMoniker) )
        return(E_FAIL);

     return m_lpfnHlinkCreateFromMoniker(
                                pmkSource,
                                szLocation,
                                szFriendlyName,
                                phlSite,
                                dwSiteData,
                                punkOuter,
                                riid,
                                ppv);
}


HLinkDll::HLinkDll()
{
    m_hmodule = 0;
    m_error = 0;
    m_lpfnHlinkCreateFromMoniker = 0;
}

#if 0
HLinkDll::~HLinkDll()
{
    if( m_hmodule )
        ::FreeLibrary( m_hmodule );
}
#endif

BOOL HLinkDll::LoadFunc( LPCSTR lpProcName, FARPROC & fp )
{
    if( m_error )
        return(0);

    if( fp )
        return(1);

    if( !m_hmodule )
    {
        m_hmodule = ::LoadLibrary( "HLINK.DLL" );

        if( !m_hmodule )
        {
           m_error = 1;
           return(0);
        }

    }

    fp = ::GetProcAddress( m_hmodule, lpProcName );

    return( fp != 0 );
}

static HLinkDll hlink;


//////////////////
static int wclen(LPCWSTR szStr)
{
    int cbStr=0;
    if(szStr!=NULL)
        while(szStr[cbStr]!=NULL)
            ++cbStr;

    return cbStr;
}

////////////////////////////

static HRESULT GetAnInterface
(
    IUnknown    *   punk,
    const IID &     riid,
    void **         pout,

    BOOL            bCheckServiceProvider,
    const IID &     siid,
    const IID &     siid_riid,
    void **         sout
)
{
    IOleObject *      oleObj    = 0;
    IOleClientSite *  oleSite   = 0;
    IOleContainer *   container = 0;
    IUnknown *        service   = 0;

    HRESULT           hr = E_FAIL;

    // Initialize passed in interface pointers: calling code assumes NULL for failure
    if(pout)
            *pout = NULL;
    if(sout)
            *sout = NULL;

    if(punk)
            hr = punk->QueryInterface( IID_IOleObject, (void **)&oleObj );

    // BUBUG: I think this returns a wrong hr if QS fails but the QI passes - jp
    while( SUCCEEDED(hr) && oleObj )
    {
            if( oleSite )
            {
                    //oleSite->Release();
                    oleSite = 0;
            }

            hr = oleObj->GetClientSite(&oleSite);

            if( FAILED(hr) || !oleSite)
                    break;

            if( bCheckServiceProvider)
            {
                    IServiceProvider * servProv;

                    hr = oleSite->QueryInterface( IID_IServiceProvider, (void**)&servProv);

                    if( SUCCEEDED(hr) )
                    {
                            hr = servProv->QueryService
                                                                    (
                                                                            siid,
                                                                            siid_riid,
                                                                            (void **)&service
                                                                    );

                            servProv->Release();
                    }

                    if( SUCCEEDED(hr) )
                    {
                            bCheckServiceProvider = FALSE;

                            hr = service->QueryInterface( riid, pout );
                    }

                    if( SUCCEEDED(hr) )
                            break;

            }

            if( container )
            {
                    container->Release();
                    container = 0;
            }

            hr = oleSite->GetContainer( &container );

            if( FAILED(hr) )
                    break;

            hr = container->QueryInterface( riid, pout );

            if( SUCCEEDED(hr) )
                    break;

            oleObj->Release();
            oleObj = 0;

            hr = container->QueryInterface( IID_IOleObject, (void**)&oleObj );

    }

    if( oleSite )
    {
        oleSite->Release();
        oleSite = 0;
    }

    if( oleObj )
        oleObj->Release();

    if( container )
        container->Release();

    if( service )
    {
        if (sout)
            *sout = service;
        else
            service->Release();
    }

    return( hr );
}

//
//  GetUrlScheme() returns one of the URL_SCHEME_* constants as
//  defined in shlwapip.h
//  example "http://foo" returns URL_SCHEME_HTTP
//
static DWORD GetUrlSchemeW(IN LPCWSTR pcszUrl)
{
    if(pcszUrl)
    {
        PARSEDURLW pu;
        pu.cbSize = sizeof(pu);
        if(SUCCEEDED(ParseURLW(pcszUrl, &pu)))
            return pu.nScheme;
    }
    return URL_SCHEME_INVALID;
}



BOOL IsSpecialUrl(WCHAR *pchURL)
{
    UINT      uProt;
    uProt = GetUrlSchemeW(pchURL);
    return (URL_SCHEME_JAVASCRIPT == uProt || 
            URL_SCHEME_VBSCRIPT == uProt ||
            URL_SCHEME_ABOUT == uProt);
}

HRESULT WrapSpecialUrlFlat(LPWSTR pszUrl, DWORD cchUrl)
{
    HRESULT     hr = S_OK;

    if (IsSpecialUrl(pszUrl))
    {
        //
        // If this is javascript:, vbscript: or about:, append the
        // url of this document so that on the other side we can
        // decide whether or not to allow script execution.
        //

        // QFE 2735 (Georgi XDomain): [alanau]
        //
        // If the special URL contains an %00 sequence, then it will be converted to a Null char when
        // encoded.  This will effectively truncate the Security ID.  For now, simply disallow this
        // sequence, and display a "Permission Denied" script error.
        //
        if (StrStrW(pszUrl, L"%00"))
        {
            hr = E_ACCESSDENIED;
        }
        else
        {
            // munge the url in place
            //

            // someone could put in a string like this:
            //     %2501 OR %252501 OR %25252501
            // which, depending on the number of decoding steps, will bypass security
            // so, just keep decoding while there are %s and the string is getting shorter
            int nPreviousLen = 0;
            while ( (nPreviousLen != lstrlenW(pszUrl)) && (StrChrW(pszUrl, L'%')))
            {
                nPreviousLen = lstrlenW(pszUrl);
                int nNumPercents;
                int nNumPrevPercents = 0;

                // Reduce the URL
                //
                for (;;)
                {
                    // Count the % signs.
                    //
                    nNumPercents = 0;

                    WCHAR *pch = pszUrl;
                    while (pch = StrChrW(pch, L'%'))
                    {
                        pch++;
                        nNumPercents++;
                    }

                    // If the number of % signs has changed, we've reduced the URL one iteration.
                    //
                    if (nNumPercents != nNumPrevPercents)
                    {
                        WCHAR szBuf[INTERNET_MAX_URL_LENGTH];
                        DWORD dwSize;

                        // Encode the URL 
                        hr = CoInternetParseUrl(pszUrl, 
                            PARSE_ENCODE, 
                            0,
                            szBuf,
                            INTERNET_MAX_URL_LENGTH,
                            &dwSize,
                            0);

                        StrCpyNW(pszUrl, szBuf, cchUrl);

                        nNumPrevPercents = nNumPercents;
                    }
                    else
                    {
                        // The URL is fully reduced.  Break out of loop.
                        //
                        break;
                    }
                }
            }

            // Now scan for '\1' characters.
            //
            if (StrChrW(pszUrl, L'\1'))
            {
                // If there are '\1' characters, we can't guarantee the safety.  Put up "Permission Denied".
                //
                hr = E_ACCESSDENIED;
            }
        }
    }

    return hr;
}


static HRESULT GetAMoniker
(
     IUnknown *                         pUnk,
     LPCWSTR                            szTarget,
     IMoniker * *                       ppMoniker
)
{
   HRESULT hr;
   IBindHost * pBindHost = 0;

   hr = GetAnInterface
       (
           pUnk,
           IID_IBindHost,
           (void**)&pBindHost,
           TRUE,
           IID_IBindHost,
           IID_IBindHost,
           NULL
       );

   if( pBindHost )
   {
      hr = pBindHost->CreateMoniker((LPWSTR)szTarget,NULL,ppMoniker,0);
          pBindHost->Release();
   }
   else
      hr = ::CreateURLMoniker(0,szTarget,ppMoniker);

   return(hr);
}


STDAPI HlinkSimpleNavigateToString
(
    /* [in] */ LPCWSTR  szTarget,      // required - target document - null if local jump w/in doc
    /* [in] */ LPCWSTR  szLocation,    // optional, for navigation into middle of a doc
    /* [in] */ LPCWSTR  szTargetFrame,   // optional, for targeting frame-sets
    /* [in] */ IUnknown *pUnk,         // required - we'll search this for other necessary interfaces
    /* [in] */ IBindCtx *pBndctx,          // optional. caller may register an IBSC in this
    /* [in] */ IBindStatusCallback * pBscb,
    /* [in] */ DWORD    grfHLNF,       // flags (TBD - HadiP needs to define this correctly?)
    /* [in] */ DWORD    dwReserved     // for future use, must be NULL
)
{
    IWebBrowserApp  *pExplorer = 0;
    IHlinkFrame     *pHlframe  = 0;
    ITargetFrame    *pTargetFrame = 0;
        IMoniker            *pMoniker = 0;

    HRESULT         hr = S_OK;

    if ( szTarget && *szTarget )
        hr = GetAMoniker( pUnk, szTarget, &pMoniker );

        if ( SUCCEEDED(hr) )
                hr = HlinkSimpleNavigateToMoniker (
                           pMoniker,
                           szLocation,
                           szTargetFrame,
                           pUnk,
                           pBndctx,
                           pBscb,
                           grfHLNF,
                           dwReserved );

        if ( pMoniker )
                pMoniker->Release();

    return( hr );

}




STDAPI HlinkSimpleNavigateToMoniker
(
    /* [in] */ IMoniker *pmkTarget,    // required - target document - (may be null if local jump w/in doc)
    /* [in] */ LPCWSTR  szLocation,    // optional, for navigation into middle of a doc
    /* [in] */ LPCWSTR  szTargetFrame, // optional, for targeting frame-sets
    /* [in] */ IUnknown *pUnk,         // required - we'll search this for other necessary interfaces
    /* [in] */ IBindCtx *pBndctx,      // optional. caller may register an IBSC in this
    /* [in] */ IBindStatusCallback * pBscb,
    /* [in] */ DWORD    grfHLNF,       // flags
    /* [in] */ DWORD    dwReserved     // for future use, must be NULL
)
{
    IWebBrowserApp * pExplorer = 0;
    IHlinkFrame *   pHlframe  = 0;
    ITargetFrame *  pTargetFrame = 0;
    CShellDll       sdll;

    HRESULT         hr, htemp;

    if (pmkTarget == NULL)
    {
         if (szLocation &&  *szLocation)
         {
              grfHLNF |= HLNF_INTERNALJUMP;
              hr = S_OK;
         }
         else
              return E_INVALIDARG; //no location specified for internal jump
    }
    else
    {
        DWORD dwId;

        if (SUCCEEDED(pmkTarget->IsSystemMoniker(&dwId)) &&
            MKSYS_URLMONIKER == dwId)
        {
            LPWSTR pszUrl;

            if (SUCCEEDED(pmkTarget->GetDisplayName(NULL, NULL, &pszUrl)))
            {
                HRESULT hrSecure = WrapSpecialUrlFlat(pszUrl, lstrlenW(pszUrl) + 1);

                CoTaskMemFree(pszUrl);

                if (FAILED(hrSecure))
                    return hrSecure;
            }
        }
    }

    if (pUnk)
        hr = GetAnInterface
              (
               pUnk,
               IID_IWebBrowserApp,
               (void**)&pExplorer,
               TRUE,
               IID_IHlinkFrame,
               IID_IHlinkFrame,
               (void**)&pHlframe
              );

    IHlink * pLink = 0;

    hr = hlink.HlinkCreateFromMoniker
              (
               pmkTarget,
               szLocation,
               L"TheName",
               0, // hlsite,
               0, NULL,
               IID_IHlink,
               (void**)&pLink
              );

    if (SUCCEEDED(hr) && pHlframe)
    {
         if (szTargetFrame && *szTargetFrame)
         {
              long len = (lstrlenW(szTargetFrame) + 1) * sizeof(char);
              char szTargetFrameName[STR_SIZE+1];
              len  = (len > STR_SIZE) ? STR_SIZE : len;
              WideCharToMultiByte(CP_ACP, 0, szTargetFrame, -1, (LPSTR)szTargetFrameName, len, NULL, NULL);
              if (lstrcmpi(szTargetFrameName,"_blank") == 0)
                  grfHLNF |= HLNF_OPENINNEWWINDOW;
              else
              {
                  htemp = GetAnInterface
                           (
                             pUnk,
                             IID_ITargetFrame,
                             (void**)&pTargetFrame,
                             TRUE,
                             IID_ITargetFrame,
                             IID_ITargetFrame,
                             NULL
                            );
                  IUnknown *punkTargetFrame = 0;
                  if (SUCCEEDED(htemp))
                      htemp = pTargetFrame->FindFrame(szTargetFrame,
                                                pHlframe,
                                                FINDFRAME_JUSTTESTEXISTENCE,
                                                &punkTargetFrame);
                  IHlinkFrame *pTargetHlinkFrame = 0;
                  if (punkTargetFrame)
                  {
                      IServiceProvider *pIsp = 0;
                      //      Get the IHlinkFrame for the target'ed frame.
                      htemp = punkTargetFrame->QueryInterface(IID_IServiceProvider, (LPVOID *)&pIsp);
                      if (pIsp != NULL)
                      {
                          // BUGBUG: SID_SHLinkFrame should be the guidService
                          htemp = pIsp->QueryService(IID_IWebBrowserApp, IID_IHlinkFrame, (LPVOID*) &pTargetHlinkFrame);
                          pIsp->Release();
                      }
                  }
                  else
                  {
                    grfHLNF |= HLNF_OPENINNEWWINDOW;
                  }
                  if (punkTargetFrame)
                      punkTargetFrame->Release();
                  if (pTargetFrame)
                      pTargetFrame->Release();

                  if (pTargetHlinkFrame)
                  {
                      pHlframe->Release();
                      pHlframe = pTargetHlinkFrame;
                  }
              }
         }
         hr = pHlframe->Navigate(grfHLNF,
                         pBndctx,
                         pBscb,
                         pLink);
    }
    else
        hr = E_FAIL;

    if ( FAILED(hr) && pHlframe)
    {       // Navigation through pHlframe failed, we will retrieve the
            // corresponding IWebBrowserApp interface and try navigation.
            if (pExplorer)
            {
                    pExplorer->Release();
                    pExplorer = 0;
            }
            pHlframe->QueryInterface(IID_IWebBrowserApp, (void **)&pExplorer);
    }

    if (FAILED(hr) && pExplorer && pmkTarget)
    {
        LPOLESTR szTarget;
        hr = pmkTarget->GetDisplayName(pBndctx,NULL,&szTarget);

            if (SUCCEEDED(hr))
                    hr = pExplorer->Navigate(
                            szTarget,
                            0,
                            0,
                            0,
                            0);

            CoTaskMemFree(szTarget);
    }

// pExplorer->Navigate  ShellExecute's
// We need not ShellExecute when pExplorer is not NULL

    if (FAILED(hr) && !pExplorer && pmkTarget)
    {
       // Our container does not support hyperlinking. We need to shell execute
       // explorer and go to the link.

       // We need to translate the string to ANSI
        CHAR szPath[MAX_PATH];
        DWORD cchPath = MAX_PATH;
        LPOLESTR szTarget;
        pmkTarget->GetDisplayName(pBndctx,NULL,&szTarget);
        int cbStr = 2 * wclen(szTarget + 1);
        
        char *pszAnsiTarget = new char[cbStr];
        if( !pszAnsiTarget )
            goto cleanup;

        // ASSERT(pszAnsiTarget)


        WideCharToMultiByte(CP_ACP, 0, szTarget, -1, pszAnsiTarget, cbStr, 0, 0);
        pszAnsiTarget[cbStr-1] = '\0';


        if(SUCCEEDED(PathCreateFromUrl(pszAnsiTarget, szPath, &cchPath, 0)))
        {
            HANDLE hFile = CreateFile(szPath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                CloseHandle(hFile);

                HINSTANCE hInst = NULL;
                if( S_OK == sdll.Init() )
                    hInst = sdll.ShellExecute(
                        NULL, "open", "iexplore.exe", 
                        szPath, NULL, SW_SHOWNORMAL );

                //
                // Return value < 32 indicates error
                //
                hr = ((ULONG_PTR)hInst > 32) ? S_OK : E_FAIL;
            }
            else
                hr = E_FAIL;
        }
        else
        {
            HINSTANCE hInst = NULL;
            if( S_OK == sdll.Init() )
                hInst = sdll.ShellExecute(
                            NULL, "open", "iexplore.exe", 
                            (LPCTSTR)pszAnsiTarget, NULL, SW_SHOWNORMAL );
            //
            // Return value < 32 indicates error
            //
            hr = ((ULONG_PTR)hInst > 32) ? S_OK : E_FAIL;
        }
cleanup:
        if( pszAnsiTarget )
            delete[] pszAnsiTarget;

        CoTaskMemFree(szTarget);
    }

    if (pExplorer)
        pExplorer->Release();
    if (pHlframe)
        pHlframe->Release();
    if (pLink)
        pLink->Release();

    return( hr );
}

//////////////////////////////////////////////////////////////////////////
//
//    HlinkGoBack
//
STDAPI HlinkGoBack(IUnknown *pUnk)
{
   IWebBrowserApp *     pExplorer = 0;
   IHlinkFrame *        pHlframe  = 0;
   HRESULT              hr;


   hr = GetAnInterface
         (
          pUnk,
          IID_IWebBrowserApp,
          (void**)&pExplorer,
          TRUE,
          IID_IHlinkFrame,
          IID_IHlinkFrame,
          (void**)&pHlframe
         );

   if ( SUCCEEDED(hr) )
        hr = pHlframe->Navigate(HLNF_NAVIGATINGBACK, 0, 0, 0);

   if ( FAILED(hr) && pExplorer )
        hr = pExplorer->GoBack();

   if ( pExplorer )
            pExplorer->Release();

   if ( pHlframe )
            pHlframe->Release();

   return (hr);
}

//////////////////////////////////////////////////////////////////////////
//
//    HlinkGoForward
//
//////////////////////////////////////////////////////////////////////////

STDAPI HlinkGoForward(IUnknown *pUnk)
{
   IWebBrowserApp *     pExplorer = 0;
   IHlinkFrame *        pHlframe  = 0;
   HRESULT              hr;

   hr = GetAnInterface
         (
          pUnk,
          IID_IWebBrowserApp,
          (void**)&pExplorer,
          TRUE,
          IID_IHlinkFrame,
          IID_IHlinkFrame,
          (void**)&pHlframe
         );

   if (SUCCEEDED(hr))
       hr = pHlframe->Navigate(HLNF_NAVIGATINGFORWARD, 0, 0, 0);

   if ( FAILED(hr) && pExplorer )
       hr = pExplorer->GoForward();

   if ( pExplorer )
           pExplorer->Release();

   if ( pHlframe )
           pHlframe->Release();

   return (hr);
}

//////////////////////////////////////////////////////////////////////////
//
//    HlinkNavigateString
//
//////////////////////////////////////////////////////////////////////////

STDAPI HlinkNavigateString(IUnknown *pUnk, LPCWSTR szTarget)
{
   HRESULT  hr;

   hr = HlinkSimpleNavigateToString(szTarget, NULL, NULL, pUnk, NULL, 0, 0, 0);

   return (hr);
}

//////////////////////////////////////////////////////////////////////////
//
//    HlinkNavigateMoniker
//
//////////////////////////////////////////////////////////////////////////

STDAPI HlinkNavigateMoniker(IUnknown *pUnk, IMoniker *pmkTarget)
{
   HRESULT  hr;

   hr = HlinkSimpleNavigateToMoniker(pmkTarget, NULL, NULL, pUnk, NULL,NULL, 0, 0);

   return (hr);
}

