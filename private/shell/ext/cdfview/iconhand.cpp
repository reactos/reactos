//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// iconhand.cpp 
//
//   The registered icon handler for cdf files.  This handler returns icons for
//   .cdf files.
//
//   History:
//
//       3/21/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "resource.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "persist.h"
#include "iconhand.h"
#include "exticon.h"
#include "cdfview.h"
#include "tooltip.h"
#include "dll.h"
#include "chanapi.h"

#include <mluisupp.h>

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** MakeXMLErrorURL() ***
//
// Set the error based on the IXMLDocument * passed in
// Upon return CParseError will in an error state no matter what
// although it may not be in the CParseError::ERR_XML state
//
// This function returns the approriate URL to navigate to
//  given the current error. Always changes *ppsz, but *ppsz maybe NULL
//
////////////////////////////////////////////////////////////////////////////////


#define CDFERROR_MAX_FOUND      100  // max char length for xml error found string
#define CDFERROR_MAX_EXPECTED   100  // max char length for xml error expected string

// format string for wsprintf of res:// ... URL
const LPTSTR CDFERROR_URL_FORMAT_TRAILER = TEXT("#%u#%ls#%ls");

// this is the number or extra chars (incl null) that CDFERROR_URL_FORMAT_TRAILER
// may have in comparison to the output buffer of wsprinf
const unsigned int CDFERROR_URL_FORMAT_EXTRA = 6;

// upper char # bound on result of building res URL
const unsigned CDFERROR_MAX_URL_LENGTH =
                6 +                                         // "res://"
                MAX_PATH +                                  // path to resource DLL
                1 +                                         // "/"
                ARRAYSIZE(SZH_XMLERRORPAGE) +               // "xmlerror.htm"
                ARRAYSIZE(CDFERROR_URL_FORMAT_TRAILER) +
                _INTEGRAL_MAX_BITS +
                CDFERROR_MAX_EXPECTED +
                CDFERROR_MAX_FOUND;

// upper char # bound on result of InternetCanonicalizeUrl
//     with result from wsprintf with CDFERROR_URL_FORMAT
//  for each funky char in the found and expected substrs, might be encoded as "%xx"
const unsigned CDFERROR_MAX_URL_LENGTH_ENCODED =
  CDFERROR_MAX_URL_LENGTH + 2*(CDFERROR_MAX_EXPECTED + CDFERROR_MAX_FOUND);


HRESULT MakeXMLErrorURL( LPTSTR pszRet, DWORD dwRetLen, IXMLDocument *pXMLDoc )
{
    IXMLError *pXMLError = NULL;
    XML_ERROR xmle = { 0 };
    HRESULT hr;

    ASSERT(pXMLDoc);

    hr =
    ( pXMLDoc ? pXMLDoc->QueryInterface(IID_IXMLError, (void **)&pXMLError) :
        E_INVALIDARG );

    if ( SUCCEEDED(hr) )
    {
        ASSERT(pXMLError);
        hr = pXMLError->GetErrorInfo(&xmle);

        if ( SUCCEEDED(hr) )
        {
            TCHAR szTemp[CDFERROR_MAX_URL_LENGTH];
            WCHAR szExpected[CDFERROR_MAX_EXPECTED];
            WCHAR szFound[CDFERROR_MAX_FOUND];

            StrCpyNW( szExpected, xmle._pszExpected, ARRAYSIZE(szExpected) );
            StrCpyNW( szFound, xmle._pszFound, ARRAYSIZE(szFound) );

            // fill in the "res://<path>\cdfvwlc.dll" part of the res URL
            hr = MLBuildResURLWrap(TEXT("cdfvwlc.dll"),
                                   g_hinst,
                                   ML_CROSSCODEPAGE,
                                   SZH_XMLERRORPAGE,
                                   szTemp,
                                   ARRAYSIZE(szTemp),
                                   TEXT("cdfview.dll"));
            if (SUCCEEDED(hr))
            {
                int nCharsWritten;
                int count;

                nCharsWritten = lstrlen(szTemp);

                count = wnsprintf(szTemp+nCharsWritten, ARRAYSIZE(szTemp)-nCharsWritten,
                    CDFERROR_URL_FORMAT_TRAILER, xmle._nLine, szExpected, szFound );
                if ( count + CDFERROR_URL_FORMAT_EXTRA < ARRAYSIZE(CDFERROR_URL_FORMAT_TRAILER) )
                {
                    // not all the chars were successfully written
                    hr = E_FAIL;
                }
                else
                  if ( !InternetCanonicalizeUrl( szTemp, pszRet, &dwRetLen, 0 ) )
                    hr = E_FAIL;

                TraceMsg(TF_CDFPARSE, "Parse error string created: %s", pszRet );
            }

            SysFreeString(xmle._pszFound);
            SysFreeString(xmle._pszExpected);
            SysFreeString(xmle._pchBuf);
        }

        else
        {
            TraceMsg(TF_CDFPARSE, "Could not get IXMLError error info" );
        }

        pXMLError->Release();
    }
    else
    {
        TraceMsg(TF_CDFPARSE, "Could not get IXMLError" );
    }

    return hr;
}



//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::CIconHandler ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CIconHandler::CIconHandler (
    void
)
: m_cRef(1)
{
    ASSERT(NULL == m_pIExtractIcon);
    ASSERT(NULL == m_bstrImageURL);
    ASSERT(NULL == m_bstrImageWideURL);
    ASSERT(NULL == m_pszErrURL);

    TraceMsg(TF_OBJECTS, "+ handler object");

    DllAddRef();

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::~CIconHandler ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CIconHandler::~CIconHandler (
    void
)
{
    ASSERT(0 == m_cRef);

    if (m_pIExtractIcon)
        m_pIExtractIcon->Release();

    if (m_bstrImageURL)
        SysFreeString(m_bstrImageURL);

    if (m_bstrImageWideURL)
        SysFreeString(m_bstrImageWideURL);

    if (m_pcdfidl)
        CDFIDL_Free(m_pcdfidl);

    if (m_pszErrURL)
        delete[] m_pszErrURL;

    //
    // Matching Release for the constructor Addref.
    //

    TraceMsg(TF_OBJECTS, "- handler object");

    DllRelease();

    return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::QueryInterface ***
//
//    CIconHandler QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IExtractIcon == riid)
    {
        *ppv = (IExtractIcon*)this;
    }
#ifdef UNICODE
    else if (IID_IExtractIconA == riid) 
    {
        *ppv = (IExtractIconA*)this;
    }
#endif
    else if (IID_IPersistFile == riid || IID_IPersist == riid)
    {
        *ppv = (IPersistFile*)this;
    }
    else if (IID_IPersistFolder == riid)
    {
        *ppv = (IPersistFolder*)this;
    }
    else if (IID_IExtractImage == riid || IID_IExtractLogo == riid)
    {
        *ppv = (IExtractImage*)this;
    }
    else if (IID_IRunnableTask == riid)
    {
        *ppv = (IRunnableTask*)this;
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        hr = S_OK;
    }

    //
    // REVIEW:  QI on the following two objects doesn't come here.
    //

    else if (IID_IShellLink == riid
#ifdef UNICODE
        || IID_IShellLinkA == riid
#endif
        )

    {
        if (!m_bCdfParsed)
            ParseCdfShellLink();

        if (m_pcdfidl)
        {
            hr = QueryInternetShortcut(m_pcdfidl, riid, ppv);
        }
        else
        {
            if ( m_pszErrURL && *m_pszErrURL)
            {
                hr = QueryInternetShortcut(m_pszErrURL, riid, ppv);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    else if (IID_IQueryInfo == riid)
    {
        hr = ParseCdfInfoTip(ppv);
    }
    else 
    {
        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::AddRef ***
//
//    CExtractIcon AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CIconHandler::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::Release ***
//
//    CIconHandler Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CIconHandler::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}


//
// IExtractIcon methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::GetIconLocation ***
//
//
// Description:
//     Returns a name index pair for the icon associated with this cdf item.
//
// Parameters:
//     [In]  uFlags     - GIL_FORSHELL, GIL_OPENICON.
//     [Out] szIconFile - The address of the buffer that receives the associated
//                        icon name.  It can be a filename, but doesn't have to
//                        be.
//     [In]  cchMax     - Size of the buffer that receives the icon location.
//     [Out] piIndex    - A pointer that receives the icon's index.
//     [Out] pwFlags    - A pointer the receives flags about the icon.
//
// Return:
//     S_OK if an was found.
//     S_FALSE if the shell should supply a default icon.
//
// Comments:
//     
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::GetIconLocation(
    UINT uFlags,
    LPTSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags
)
{
    ASSERT(szIconFile);
    ASSERT(piIndex);
    ASSERT(pwFlags);

    HRESULT hr;

    TraceMsg(TF_CDFICON, "<IN > CIconHandler::GetIconLocation (Icon) tid:0x%x",
             GetCurrentThreadId());

    if (uFlags & GIL_ASYNC)
    {
        hr = E_PENDING;
    }
    else
    {
        hr = E_FAIL;

        if ( IsDefaultChannel())
        {
            m_bstrImageURL = CPersist::ReadFromIni( TSTR_INI_ICON );
            if ( m_bstrImageURL )
            {
                ASSERT( !m_pIExtractIcon );
                m_pIExtractIcon = (IExtractIcon*)new CExtractIcon( m_bstrImageURL );
            }
        }
        if (!m_pIExtractIcon && !m_bCdfParsed)
            ParseCdfIcon();

        if (m_pIExtractIcon)
        {
            hr = m_pIExtractIcon->GetIconLocation(uFlags, szIconFile, cchMax,
                                                  piIndex, pwFlags);
        }

        if (FAILED(hr) ||
            (StrEql(szIconFile, g_szModuleName) &&
            -IDI_CHANNEL == *piIndex)  )
        {
            //
            // Try and get the icon out of the desktop.ini file.
            //

            m_bstrImageURL = CPersist::ReadFromIni(TSTR_INI_ICON);

            if (m_bstrImageURL)
            {
                BOOL bRemovePrefix =
                                (0 == StrCmpNIW(L"file://", m_bstrImageURL, 7));

                if (SHUnicodeToTChar(
                            bRemovePrefix ? m_bstrImageURL + 7 : m_bstrImageURL,
                            szIconFile, cchMax))
                {
                    LPTSTR pszExt = PathFindExtension(szIconFile);

                    if (*pszExt != TEXT('.') ||
                        0 != StrCmpI(pszExt, TSTR_ICO_EXT))
                    {
                        *piIndex = INDEX_IMAGE;
                        MungePath(szIconFile);
                    }
                    else
                    {
                        *piIndex = 0;
                        *pwFlags = 0;
                    }

                    hr = S_OK;
                }
            }
        }

        if (FAILED(hr))
        {
            //
            // Try to return the default channel icon.
            //

            *pwFlags = 0;

            StrCpyN(szIconFile, g_szModuleName, cchMax);

            if (*szIconFile)
            {
                *piIndex = -IDI_CHANNEL;

                hr = S_OK;
            }
            else
            {
                *piIndex = 0;

                hr = S_FALSE;  // The shell will use a default icon.
            }
        }

        //
        // If this a generated icon and it should contain a gleam prepend
        // the string with a 'G'.
        //

        if (S_OK == hr && m_fDrawGleam)
        {
            TCHAR* pszBuffer = new TCHAR[cchMax];
            
            if (m_pIExtractIcon)
            {
                CExtractIcon *pExtract = (CExtractIcon *)m_pIExtractIcon;
                pExtract->SetGleam(m_fDrawGleam);
            }
            
            if (pszBuffer)
            {
                StrCpyN(pszBuffer, szIconFile, cchMax);

                *szIconFile = TEXT('G');
                cchMax--;

                StrCpyN(szIconFile+1, pszBuffer, cchMax);

                delete [] pszBuffer;
            }
        }

        *pwFlags = (m_fDrawGleam || INDEX_IMAGE == *piIndex) ? GIL_NOTFILENAME :
                                                               0;

        if (m_fDrawGleam)
            *piIndex += GLEAM_OFFSET;

        TraceMsg(TF_GLEAM, "%c Icon Location %s,%d", m_fDrawGleam ? '+' : '-',
                 SUCCEEDED(hr) ? szIconFile : TEXT("FAILED"), *piIndex);

        ASSERT((S_OK == hr && *szIconFile) ||
               (S_FALSE == hr && 0 == *szIconFile));
    }

    TraceMsg(TF_CDFICON, "<OUT> CIconHandler::GetIconLocation (Icon) %s",
             szIconFile);
    return hr;
}
#ifdef UNICODE
STDMETHODIMP
CIconHandler::GetIconLocation(
    UINT uFlags,
    LPSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags
)
{
    ASSERT(szIconFile);
    ASSERT(piIndex);
    ASSERT(pwFlags);

    HRESULT hr;

    TraceMsg(TF_CDFICON, "<IN > CIconHandler::GetIconLocationA (Icon) tid:0x%x",
             GetCurrentThreadId());

    WCHAR* pszIconFileW = new WCHAR[cchMax];
    if (pszIconFileW == NULL)
        return ERROR_OUTOFMEMORY;
    hr = GetIconLocation(uFlags, pszIconFileW, cchMax, piIndex, pwFlags);
    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(pszIconFileW, szIconFile, cchMax);

    delete [] pszIconFileW;
    return hr;
}
#endif

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::Extract ***
//
//
// Description:
//     Return an icon given the name index pair returned from GetIconLocation.
//
// Parameters:
//     [In]  pszFile     - A pointer to the name associated with the requested
//                         icon.
//     [In]  nIconIndex  - An index associated with the requested icon.
//     [Out] phiconLarge - Pointer to the variable that receives the handle of
//                         the large icon.
//     [Out] phiconSmall - Pointer to the variable that receives the handle of
//                         the small icon.
//     [Out] nIconSize   - Value specifying the size, in pixels, of the icon
//                         required. The LOWORD and HIWORD specify the size of
//                         the large and small icons, respectively.
//
// Return:
//     S_OK if the icon was extracted.
//     S_FALSE if the shell should extract the icon assuming the name is a
//     filename and the index is the icon index.
//
// Comments:
//     The shell may cache the icon returned from this function.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::Extract(
    LPCTSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize
)
{
    HRESULT hr;

    TraceMsg(TF_CDFICON, "<IN > CIconHandler::Extract (Icon) tid:0x%x",
             GetCurrentThreadId());

    DWORD dwType;
    DWORD dwVal;    //  Bits per pixel
    DWORD cbVal = sizeof(DWORD);
    
    if ((SHGetValue(HKEY_CURRENT_USER, c_szHICKey, c_szHICVal, &dwType, &dwVal, 
                   &cbVal) != ERROR_SUCCESS) 
                   &&
                   (REG_DWORD != dwType))
                   
    {
        dwVal = 0;
    }

    //  Convert bits per pixel to # colors
    m_dwClrDepth = (dwVal == 16) ? 256 : 16;

    if (m_fDrawGleam)
        nIconIndex -= GLEAM_OFFSET;
    
    if (m_pIExtractIcon)
    {
        hr = m_pIExtractIcon->Extract(pszFile, nIconIndex, phiconLarge,
                                      phiconSmall, nIconSize);

        //
        // If an icon couldn't be extracted, try and display the default icon.
        //

        if (FAILED(hr))
        {
            hr = Priv_SHDefExtractIcon(g_szModuleName, -IDI_CHANNEL, 0, 
                        phiconLarge, phiconSmall, nIconSize);
        }
    }
    else
    {
        hr = S_FALSE;
    }

    TraceMsg(TF_GLEAM, "%c Icon Extract  %s %s", m_fDrawGleam ? '+' : '-',
             pszFile, (S_OK == hr) ? TEXT("SUCCEEDED") : TEXT("FAILED"));

    TraceMsg(TF_CDFICON, "<OUT> CIconHandler::Extract (Icon) tid:0x%x",
             GetCurrentThreadId());

    return hr;
}

#ifdef UNICODE
STDMETHODIMP
CIconHandler::Extract(
    LPCSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize
)
{
    HRESULT hr;

    TraceMsg(TF_CDFICON, "<IN > CIconHandler::ExtractA (Icon) tid:0x%x",
             GetCurrentThreadId());

    int    cch = lstrlenA(pszFile) + 1; 
    WCHAR* pszFileW = new WCHAR[cch];
    if (pszFileW == NULL)
        return ERROR_OUTOFMEMORY;
    SHAnsiToUnicode(pszFile, pszFileW, cch);

    hr = Extract(pszFileW, nIconIndex, phiconLarge, phiconSmall, nIconSize);

    delete [] pszFileW;
    return hr;
}
#endif

//
// IExtractImage methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::GetLocation ***
//
//
// Description:
//     Returns a string to associate with this files image.
//
// Parameters:
//     [Out] pszPathBuffer - A buffer that receives this items string.
//     [In]  cch           - The size of the buffer.
//     [Out] pdwPriority   - The priority of this item's image.
//     [In/Out] pdwFlags   - Flags associated with this call.
//
// Return:
//     S_OK if a string is returned.
//     E_FAIL otherwise.
//
// Comments:
//     IExtractImage uses the returned value to share images accross multiple
//     items.  If three items in the same directory return "Default" all three
//     would use the same image.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::GetLocation(
    LPWSTR pszPathBuffer,
    DWORD cch,
    DWORD* pdwPriority,
    const SIZE * prgSize,
    DWORD dwRecClrDepth,
    DWORD* pdwFlags
)
{
    LPWSTR pstrURL = NULL;
    
    ASSERT(pszPathBuffer || 0 == cch);
    ASSERT(pdwFlags);

    HRESULT hr = E_FAIL;
    
    TraceMsg(TF_CDFLOGO, "<IN > CIconHandler::GetIconLocation (Logo) tid:0x%x",
             GetCurrentThreadId());

    if ( !prgSize )
    {
        return E_INVALIDARG;
    }
    
    m_rgSize = *prgSize;
    m_dwClrDepth = dwRecClrDepth;

    if ( IsDefaultChannel() && !UseWideLogo(prgSize->cx))
    {
        // avoid having to partse the CDF if possible by
        // pulling the entry from the desktop.ini file...
        pstrURL = m_bstrImageURL = CPersist::ReadFromIni(TSTR_INI_LOGO);
    }
    
    if (pstrURL == NULL && !m_bCdfParsed)
        ParseCdfImage(&m_bstrImageURL, &m_bstrImageWideURL);

    pstrURL = (UseWideLogo(prgSize->cx) && m_bstrImageWideURL) ?
                                           m_bstrImageWideURL :
                                           m_bstrImageURL;

    if (pstrURL)
    {
        ASSERT(0 != *m_bstrImageURL);

        if (m_fDrawGleam && cch > 0)
        {
            *pszPathBuffer++ = L'G';
            cch--;
        }

        if (StrCpyNW(pszPathBuffer, pstrURL, cch))
        {
            hr = S_OK;
        }
        else
        {
            if (m_bstrImageURL)
            {
                SysFreeString(m_bstrImageURL);
                m_bstrImageURL = NULL;
            }

            if (m_bstrImageWideURL)
            {
                SysFreeString(m_bstrImageWideURL);
                m_bstrImageWideURL = NULL;
            }
        }
    }

    if (FAILED(hr))
    {
        m_bstrImageURL     = CPersist::ReadFromIni(TSTR_INI_LOGO);
        m_bstrImageWideURL = CPersist::ReadFromIni(TSTR_INI_WIDELOGO);

        pstrURL = (UseWideLogo(prgSize->cx) && m_bstrImageWideURL) ?
                                                            m_bstrImageWideURL :
                                                            m_bstrImageURL;

        if (pstrURL)
        {
            if (m_fDrawGleam && cch > 0)
            {
                *pszPathBuffer++ = L'G';
                cch--;
            }

            if (StrCpyNW(pszPathBuffer, pstrURL, cch))
                hr = S_OK;
        }
    }

    BOOL bAsync = *pdwFlags & IEIFLAG_ASYNC;

    //
    // REVIEW: Long URLs truncated in pszPathBuffer.
    //

    //TSTRToWideChar(m_szPath, pszPathBuffer, cch);

    if (pdwPriority)
        *pdwPriority = ITSAT_DEFAULT_PRIORITY; //0x10000;  // Low priority since this could hit the net.

    *pdwFlags = m_fDrawGleam ? IEIFLAG_GLEAM : 0;

    TraceMsg(TF_GLEAM, "%c Logo Location %S", m_fDrawGleam ? '+' : '-',
             SUCCEEDED(hr) ? pszPathBuffer : L"FAILED");

    //
    // REVIEW: Proper IEIFLAG_ASYNC handling.
    //

    TraceMsg(TF_CDFLOGO, "<OUT> CIconHandler::GetIconLocation (Logo) tid:0x%x",
             GetCurrentThreadId());

    return (SUCCEEDED(hr) && bAsync) ? E_PENDING : hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::Extract ***
//
//
// Description:
//     Returns a hbitmap for use as a logo for this cdf file.
//
// Parameters:
//     [Out] phBmp         - The returned bitmap.
//
// Return:
//     S_OK if an image was extracted.
//     E_FAIL if an image couldn't be extracted.
//
// Comments:
//     The returned bitmap is stretched to pSize.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::Extract(
    HBITMAP * phBmp
)
{
    ASSERT(phBmp);

    HRESULT hr = E_FAIL;

    TraceMsg(TF_CDFLOGO, "<IN > CIconHandler::Extract (Logo) tid:0x%x",
             GetCurrentThreadId());

    if (m_bstrImageURL)
    {
        hr = ExtractCustomImage(&m_rgSize, phBmp);
    }
 
    // Let the extractor build a default logo.
    //if (FAILED(hr))
    //    hr = ExtractDefaultImage(pSize, phBmp);

    TraceMsg(TF_CDFLOGO, "<OUT> CIconHandler::Extract (Logo) tid:0x%x",
             GetCurrentThreadId());

    return hr;
}


//
// Helper functions.
//


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::IsDefaultChannel***
//
//
// Description:
//     Checks to see if the channel we are dealing with is a default one.
//
// Parameters:
//     None.
//
// Return:
//     TRUE if it is a default channel.
//     FALSE otherwise.
//
// Comments:
//     This is used in an attempt to avoid parsing the CDF if the only
//     information we need is already in the desktop.ini file.
//     
////////////////////////////////////////////////////////////////////////////////

BOOL CIconHandler::IsDefaultChannel()
{
    BOOL fDefault = FALSE;
    
    // get the desktop.ini path and see if it points to the systemdir\web
    BSTR pstrURL = CPersist::ReadFromIni( TSTR_INI_URL );
    if ( pstrURL )
    {
        fDefault = Channel_CheckURLMapping( pstrURL );
        SysFreeString( pstrURL );
    }
    return fDefault;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::ParseCdfIcon ***
//
//
// Description:
//     Parses the cdf file associated with this folder.
//
// Parameters:
//     None.
//
// Return:
//     S_OK if the cdf file was found and successfully parsed.
//     E_FAIL otherwise.
//
// Comments:
//     This parse function gets the root channel item and uses it to create
//     a CExtractIcon object.  The CExtractIcon object is later called to get
//     the icon location and extract the icon.
//     
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::ParseCdfIcon(
    void
)
{
    HRESULT hr;

    //
    // Parse the file and get back the root channel element.
    //

    IXMLDocument* pIXMLDocument = NULL;

    TraceMsg(TF_CDFICON, "Extracting icon URL for %s",
             PathFindFileName(m_szPath));
    TraceMsg(TF_CDFPARSE, "Extracting icon URL for %s",
             PathFindFileName(m_szPath));

    hr = CPersist::ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLDocument);

        m_fDrawGleam = CPersist::IsUnreadCdf();

        IXMLElement*    pIXMLElement;
        LONG            nIndex;

        hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            PCDFITEMIDLIST pcdfidl = CDFIDL_CreateFromXMLElement(pIXMLElement,
                                                                 nIndex);

            if (pcdfidl)
            {
                //
                // Create a CExtractIcon object for the root channel.
                //

                m_pIExtractIcon = (IExtractIcon*)new CExtractIcon(pcdfidl,
                                                    pIXMLElement);

                hr = m_pIExtractIcon ? S_OK : E_OUTOFMEMORY;

                CDFIDL_Free(pcdfidl);
            }

            pIXMLElement->Release();
        }
    }

    if (pIXMLDocument)
        pIXMLDocument->Release();

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::ParseCdfImage ***
//
//
// Description:
//     Parses the cdf file associated with this folder.
//
// Parameters:
//     [In]  pbstrURL - A pointer that receives the URL for the image associated
//                      with the root channel.
//
// Return:
//     S_OK if the URL was found.
//     E_FAIL if the URL wasn't found.
//
// Comments:
//     This function parses the cdf file and returns an URL to the image
//     associated with this cdf file.
//     
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::ParseCdfImage(
    BSTR* pbstrURL,
    BSTR* pbstrWURL
)
{
    ASSERT(pbstrURL);

    HRESULT hr;

    *pbstrURL = NULL;

    IXMLDocument* pIXMLDocument = NULL;

    //
    // Parse the file.
    //

    TraceMsg(TF_CDFPARSE, "Extracting logo URL for %s",
             PathFindFileName(m_szPath));
    TraceMsg(TF_CDFLOGO, "Extracting logo URL for %s",
             PathFindFileName(m_szPath));

    hr = CPersist::ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLDocument);

        m_fDrawGleam = CPersist::IsUnreadCdf();

        //
        // Get the first channel element.
        //

        IXMLElement*    pIXMLElement;
        LONG            nIndex;

        hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            //
            // Get the logo URL of the first channel element.
            //

            *pbstrURL = XML_GetAttribute(pIXMLElement, XML_LOGO);

            hr = *pbstrURL ? S_OK : E_FAIL;

            *pbstrWURL = XML_GetAttribute(pIXMLElement, XML_LOGO_WIDE);

            pIXMLElement->Release();
        }
    }

    if (pIXMLDocument)
        pIXMLDocument->Release();

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Name ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::ParseCdfShellLink(
    void
)
{
    HRESULT hr;

    //
    // Parse the file and get back the root channel element.
    //

    IXMLDocument* pIXMLDocument = NULL;

    TraceMsg(TF_CDFPARSE, "Extracting IShellLink for %s",
             PathFindFileName(m_szPath));

    hr = CPersist::ParseCdf(NULL, &pIXMLDocument,
                            PARSE_LOCAL | PARSE_REMOVEGLEAM);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLDocument);

        IXMLElement*    pIXMLElement;
        LONG            nIndex;

        hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            m_pcdfidl = CDFIDL_CreateFromXMLElement(pIXMLElement,
                                                    nIndex);

            pIXMLElement->Release();
        }
    }
    else if (OLE_E_NOCACHE == hr)
    {
        //
        // If it wasn't in the cache pass the url of the cdf so it gets reloaded.
        //

        BSTR bstrURL = CPersist::ReadFromIni(TSTR_INI_URL);

        if (bstrURL)
        {
            if (InternetGetConnectedState(NULL, 0))
            {
                int cch = StrLenW(bstrURL) + 1;
                m_pszErrURL = new TCHAR[cch];

                if (m_pszErrURL)
                {
                    if (!SHUnicodeToTChar(bstrURL, m_pszErrURL, cch))
                    {
                        delete []m_pszErrURL;
                        m_pszErrURL = NULL;
                    }
                }
            }
            else
            {
                TCHAR   szResURL[INTERNET_MAX_URL_LENGTH];
                HRESULT hr;

                ASSERT(NULL == m_pszErrURL);

                hr = MLBuildResURLWrap(TEXT("cdfvwlc.dll"),
                                       g_hinst,
                                       ML_CROSSCODEPAGE,
                                       TEXT("cacheerr.htm#"),
                                       szResURL,
                                       ARRAYSIZE(szResURL),
                                       TEXT("cdfview.dll"));
                if (SUCCEEDED(hr))
                {
                    int cchPrefix = StrLen(szResURL);

                    int cch = StrLenW(bstrURL) + cchPrefix + 1;

                    m_pszErrURL = new TCHAR[cch];

                    if (!StrCpy(m_pszErrURL, szResURL) ||
                        !SHUnicodeToTChar(bstrURL, m_pszErrURL + cchPrefix, cch - cchPrefix))
                    {
                            delete []m_pszErrURL;
                            m_pszErrURL = NULL;
                    }
                }
            }
             
            SysFreeString(bstrURL);
        }

    }
    else
    {
        DWORD dwSize = sizeof(TCHAR[CDFERROR_MAX_URL_LENGTH_ENCODED]);  // count in bytes
        if (NULL==m_pszErrURL)
          m_pszErrURL = new TCHAR[CDFERROR_MAX_URL_LENGTH_ENCODED];
        if (m_pszErrURL)
        {
            if (pIXMLDocument)
            {
                if ( FAILED(MakeXMLErrorURL(m_pszErrURL, dwSize, pIXMLDocument)) )
                {
                    delete[] m_pszErrURL;
                    m_pszErrURL = NULL;
                }
            }
        }
    }

    if (pIXMLDocument)
        pIXMLDocument->Release();

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Name ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::ParseCdfInfoTip(
    void** ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    //
    // Parse the file and get back the root channel element.
    //

    IXMLDocument* pIXMLDocument = NULL;

    TraceMsg(TF_CDFPARSE, "Extracting IQueryInfo for %s",
             PathFindFileName(m_szPath));

    hr = CPersist::ParseCdf(NULL, &pIXMLDocument, PARSE_LOCAL);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLDocument);

        IXMLElement*    pIXMLElement;
        LONG            nIndex;

        hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            *ppv = (IQueryInfo*)new CQueryInfo(pIXMLElement, XML_IsFolder(pIXMLElement));

            hr = *ppv ? S_OK : E_FAIL;

            pIXMLElement->Release();
        }
    }
    else
    {
        //
        // Even if the cdf isn't in the cache, return a IQueryInfo interface.
        // The caller can stil call GetInfoFlags.
        //

        *ppv = (IQueryInfo*)new CQueryInfo(NULL, FALSE);

        hr = *ppv ? S_OK : E_FAIL;
    }

    if (pIXMLDocument)
        pIXMLDocument->Release();

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::ExtractCustomImage ***
//
//
// Description:
//     Extract an image from an URL.
//
// Parameters:
//     [In]  pSize - The requested size of the image.
//     [Out] phBmp - The returned bitmap.
//
// Return:
//     S_OK if the bitmap was successfully extracted.
//     E_FAIL otherwise.
//
// Comments:
//     The URL of the image is in m_bstrImageURL and was set when the cdf
//     file was parsed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::ExtractCustomImage(
    const SIZE* pSize,
    HBITMAP* phBmp
)
{
    ASSERT(pSize);
    ASSERT(phBmp);

    HRESULT hr;

    IImgCtx* pIImgCtx;

    hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                          IID_IImgCtx, (void**)&pIImgCtx);

    BOOL bCoInit = FALSE;

    if ((CO_E_NOTINITIALIZED == hr || REGDB_E_IIDNOTREG == hr) &&
        SUCCEEDED(CoInitialize(NULL)))
    {
        bCoInit = TRUE;
        hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                              IID_IImgCtx, (void**)&pIImgCtx);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(pIImgCtx);

        hr = SynchronousDownload(pIImgCtx,
                                (UseWideLogo(pSize->cx) && m_bstrImageWideURL) ?
                                                            m_bstrImageWideURL :
                                                            m_bstrImageURL);

        //
        // If the load of the wide logo failed try and use the regular logo.
        //

        if (FAILED(hr) && UseWideLogo(pSize->cx) && m_bstrImageWideURL &&
            m_bstrImageURL)
        {
            hr = SynchronousDownload(pIImgCtx, m_bstrImageURL);

            SysFreeString(m_bstrImageWideURL);
            m_bstrImageWideURL = NULL;
        }

        if (SUCCEEDED(hr))
        {
            hr = GetBitmap(pIImgCtx, pSize, phBmp);
            
            if (FAILED(hr))
                *phBmp = NULL;
        }

        pIImgCtx->Release();
    }

    if (bCoInit)
        CoUninitialize();

    ASSERT((SUCCEEDED(hr) && *phBmp) || (FAILED(hr) && NULL == *phBmp));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::ExtractDefaultImage ***
//
//
// Description:
//     Returns the default channel bitmap.
//
//     [In]  pSize - The requested size of the image.
//     [Out] phBmp - The returned bitmap.
//
// Return:
//     S_OK if the bitmap was successfully extracted.
//     E_FAIL otherwise.
//
// Comments:
//     If a cdf doesn't specify a logo image or the logo image couldn't be
//     downloaded a default image is used.
//
////////////////////////////////////////////////////////////////////////////////
/*HRESULT
CIconHandler::ExtractDefaultImage(
    const SIZE* pSize,
    HBITMAP* phBmp
)
{
    ASSERT(pSize);
    ASSERT(phBmp);

    HRESULT hr;

    hr = GetBitmap(NULL, pSize, phBmp);

    if (FAILED(hr))
        *phBmp = NULL;

    ASSERT((SUCCEEDED(hr) && *phBmp) || (FAILED(hr) && NULL == *phBmp));

    return hr;
}*/

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::GetBitmap ***
//
//
// Description:
//     Gets the requested bitmap
//
// Parameters:
//     [In]  pIImgCtx - The ImgCtx of the image.  NULL if the default image is
//                      to be returned.
//     [In]  pSize    - The requested size of the image.
//     [Out] phBmp    - A pointer that receives the returned image.
//
// Return:
//     S_OK if the image was extracted.
//     E_FAIL otherwise.
//
// Comments:
//     This function conatins code that is shared by the custom image extractor
//     and the default image extractor.  The pIImhCtx parameter is used as a
//     flag indicating which image - default or custom - is to be extracted.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::GetBitmap(
    IImgCtx* pIImgCtx,
    const SIZE* pSize,
    HBITMAP* phBmp
)
{
    ASSERT(pSize);
    ASSERT(phBmp);

    HRESULT hr = E_FAIL;

    //
    // REVIEW: Pallete use for 8bpp DCs?
    //

    HDC hdcScreen = GetDC(NULL);

    if (hdcScreen)
    {
        HDC hdcDst = CreateCompatibleDC(NULL);

        if (hdcDst)
        {
            LPVOID lpBits;
            struct {
                BITMAPINFOHEADER bi;
                DWORD            ct[256];
            } dib;

            dib.bi.biSize            = sizeof(BITMAPINFOHEADER);
            dib.bi.biWidth           = pSize->cx;
            dib.bi.biHeight          = pSize->cy;
            dib.bi.biPlanes          = 1;
            dib.bi.biBitCount        = (WORD) m_dwClrDepth;
            dib.bi.biCompression     = BI_RGB;
            dib.bi.biSizeImage       = 0;
            dib.bi.biXPelsPerMeter   = 0;
            dib.bi.biYPelsPerMeter   = 0;
            dib.bi.biClrUsed         = ( m_dwClrDepth <= 8 ) ? (1 << m_dwClrDepth) : 0;
            dib.bi.biClrImportant    = 0;

            if ( m_dwClrDepth <= 8 )
            {
                HPALETTE hpal = NULL;
                // need to get the right palette....
                hr = pIImgCtx->GetPalette( & hpal );
                if ( SUCCEEDED( hr ) && hpal )
                {
                    GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY)&dib.ct[0]);
                    for (int i = 0; i < (int)dib.bi.biClrUsed; i ++)
                        dib.ct[i] = RGB(GetBValue(dib.ct[i]),GetGValue(dib.ct[i]),GetRValue(dib.ct[i]));
                }
            }
            
            *phBmp = CreateDIBSection(hdcDst, (LPBITMAPINFO)&dib, DIB_RGB_COLORS, &lpBits, NULL, 0);

            HBITMAP hOld = (HBITMAP) SelectObject( hdcDst, *phBmp );
            if (*phBmp && hOld)
            {
                RECT rc;
                rc.top = rc.left = 0;
                rc.bottom = pSize->cy;
                rc.right = pSize->cx;

                // black background...
                HBRUSH hbr = (HBRUSH) GetStockObject( BLACK_BRUSH );
                
                FillRect( hdcDst, &rc, hbr );
                DeleteObject( hbr );
        
                if (pIImgCtx)
                {
                    hr = StretchBltCustomImage(pIImgCtx, pSize, hdcDst);
                }
                else
                {
                    hr = E_FAIL; //StretchBltDefaultImage(pSize, hdcDst);
                }
                SelectObject( hdcDst, hOld );
            }

            DeleteDC(hdcDst);
        }

        ReleaseDC(NULL, hdcScreen);
    }

    ASSERT((SUCCEEDED(hr) && *phBmp) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::StretchBltCustomImage ***
//
//
// Description:
//     Stretches the image associated with IImgCtx to the given size and places
//     the result in the given DC.
//
// Parameters:
//     [In]  pIImgCtx  - The image context for the image.
//     [In]  pSize     - The size of the resultant image.
//     [In/Out] hdcDst - The destination DC of the stretch blt.
//
// Return:
//     S_OK if the image was successfully resized into the destination DC.
//     E_FAIL otherwise.
//
// Comments:
//     The destination DC already a bitmap of pSize selected into it.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::StretchBltCustomImage(
    IImgCtx* pIImgCtx,
    const SIZE* pSize,
    HDC hdcDst
)
{
    ASSERT(pIImgCtx);
    ASSERT(hdcDst);

    HRESULT hr;

    SIZE    sz;
    ULONG   fState;

    hr = pIImgCtx->GetStateInfo(&fState, &sz, FALSE);

    if (SUCCEEDED(hr))
    {
        HPALETTE hpal = NULL;
        HPALETTE hpalOld;

        hr = pIImgCtx->GetPalette( &hpal );
        if ( SUCCEEDED( hr ) && hpal )
        {
            hpalOld = SelectPalette( hdcDst, hpal, TRUE );
            RealizePalette( hdcDst );
        }

        if (UseWideLogo(pSize->cx) && NULL == m_bstrImageWideURL)
        {
            hr = pIImgCtx->StretchBlt(hdcDst, 0, 0, LOGO_WIDTH, pSize->cy, 0, 0,
                                      sz.cx, sz.cy, SRCCOPY);

            if (SUCCEEDED(hr))
            {
                //
                // Color fill the logo.
                //

                COLORREF clr = GetPixel(hdcDst, 0, 0);

                if (m_dwClrDepth <= 8)
                     clr = PALETTEINDEX(GetNearestPaletteIndex(hpal, clr));

                HBRUSH hbr = CreateSolidBrush(clr);

                if (hbr)
                {
                    RECT rc;
                    
                    rc.top    = 0;
                    rc.bottom = pSize->cy;
                    rc.left   = LOGO_WIDTH;
                    rc.right  = pSize->cx;

                    FillRect(hdcDst, &rc, hbr);

                    DeleteObject(hbr);
                }
            }
        }
        else
        {
            hr = pIImgCtx->StretchBlt(hdcDst, 0, 0, pSize->cx, pSize->cy, 0, 0,
                                      sz.cx, sz.cy, SRCCOPY);
        }

        if (SUCCEEDED(hr) && m_fDrawGleam)
            DrawGleam(hdcDst);

        if ( hpal )
        {
            SelectPalette( hdcDst, hpalOld, TRUE );
            RealizePalette( hdcDst );
        }
    }

    return hr;
}    

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::StretchBltDefaultImage ***
//
//
// Description:
//     Stretches the deafult channel image to fit the requested size.
//
// Parameters:
//     [In]  pSize      - The requested size of the image.
//     [In/Out] hdcDest - The destination DC for the resized image.
//
// Return:
//     S_OK if the image is successfully resized into the destination DC.
//     E_FAIL otherwise.
//
// Comments:
//     This function creates a source DC, copies the deafult bitmap into the
//     the source DC, then strch blts the source DC into the destination DC.
//
////////////////////////////////////////////////////////////////////////////////
/*HRESULT
CIconHandler::StretchBltDefaultImage(
    const SIZE* pSize,
    HDC hdcDst
)
{
    ASSERT(hdcDst);

    HRESULT hr = E_FAIL;

    HBITMAP hBmp = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_DEFAULT));

    if (hBmp)
    {
        HDC hdcSrc = CreateCompatibleDC(NULL);

        if (hdcSrc && SelectObject(hdcSrc, hBmp))
        {
            BITMAP bmp;

            if (GetObject(hBmp, sizeof(BITMAP), (void*)&bmp))
            {
                if (StretchBlt(hdcDst, 0, 0, pSize->cx, pSize->cy,
                               hdcSrc, 0, 0, bmp.bmWidth, bmp.bmHeight,
                               SRCCOPY))
                {
                    hr = S_OK;
                }
            }

            DeleteDC(hdcSrc);
        }

        DeleteObject(hBmp);
    }

    return hr;
}*/

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::DrawGleam ***
//
//
// Description:
//
// Parameters:
//
// Return:
//
// Comments:
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::DrawGleam(
    HDC hdcDst
)
{
    ASSERT(hdcDst)

    HRESULT hr = E_FAIL;

    HICON hGleam = (HICON)LoadImage(g_hinst, TEXT("LOGOGLEAM"),
                                    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

    if (hGleam)
    {
        if (DrawIcon(hdcDst, 1, 1, hGleam))
            hr = S_OK;

        DestroyIcon(hGleam);
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::SynchronousDownload ***
//
//
// Description:
//     Synchronously downloads the image associated with the image context.
//
// Parameters:
//     [In]  pIImgCtx - A pointer to the image context.
//
// Return:
//     S_OK if the image was successfully downloaded.
//     E_FAIL if the image wasn't downloaded.
//
// Comments:
//     The image context object doesn't directly support synchronous download.
//     Here a message loop is used to make sure ulrmon keeps geeting messages
//     and the download progresses.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CIconHandler::SynchronousDownload(
    IImgCtx* pIImgCtx,
    LPCWSTR pwszURL
)
{
    ASSERT(pIImgCtx);

    HRESULT hr;

    TCHAR szLocalFile[MAX_PATH];
    TCHAR szURL[INTERNET_MAX_URL_LENGTH];

    SHUnicodeToTChar(pwszURL, szURL, ARRAYSIZE(szURL));

    hr = URLGetLocalFileName(szURL, szLocalFile, ARRAYSIZE(szLocalFile), NULL);

    TraceMsg(TF_GLEAM, "%c Logo Extract  %s", m_fDrawGleam ? '+' : '-',
             szLocalFile);    

    if (SUCCEEDED(hr))
    {
        TraceMsg(TF_CDFLOGO, "[URLGetLocalFileName %s]", szLocalFile);

#ifdef UNIX
        unixEnsureFileScheme(szLocalFile);
#endif /* UNIX */

        WCHAR szLocalFileW[MAX_PATH];

        SHTCharToUnicode(szLocalFile, szLocalFileW, ARRAYSIZE(szLocalFileW));

        hr = pIImgCtx->Load(szLocalFileW, 0);

        if (SUCCEEDED(hr))
        {
            ULONG fState;
            SIZE  sz;

            pIImgCtx->GetStateInfo(&fState, &sz, TRUE);

            if (!(fState & (IMGLOAD_COMPLETE | IMGLOAD_ERROR)))
            {
                m_fDone = FALSE;

                hr = pIImgCtx->SetCallback(ImgCtx_Callback, &m_fDone);

                if (SUCCEEDED(hr))
                {
                    hr = pIImgCtx->SelectChanges(IMGCHG_COMPLETE, 0, TRUE);

                    if (SUCCEEDED(hr))
                    {
                        MSG msg;
                        BOOL fMsg;

                        // HACK: restrict the message pump to those messages we know that URLMON and
                        // HACK: the imageCtx stuff needs, otherwise we will be pumping messages for
                        // HACK: windows we shouldn't be pumping right now...
                        while(!m_fDone )
                        {
                            fMsg = PeekMessage(&msg, NULL, WM_USER + 1, WM_USER + 4, PM_REMOVE );

                            if (!fMsg)
                            {
                                fMsg = PeekMessage( &msg, NULL, WM_APP + 2, WM_APP + 2, PM_REMOVE );
                            }

                            if (!fMsg)
                            {
                                // go to sleep until we get a new message....
                                WaitMessage();
                                continue;
                            }

                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }
                }

                pIImgCtx->Disconnect();
            }

            hr = pIImgCtx->GetStateInfo(&fState, &sz, TRUE);

            if (SUCCEEDED(hr))
                hr = (fState & IMGLOAD_COMPLETE) ? S_OK : E_FAIL;
        }
    }
    else
    {
        TraceMsg(TF_CDFLOGO, "[URLGetLocalFileName %s FAILED]",szURL);
    }

    TraceMsg(TF_CDFPARSE, "[IImgCtx downloading logo %s %s]", szLocalFile,
             SUCCEEDED(hr) ? TEXT("SUCCEEDED") : TEXT("FAILED"));
    TraceMsg(TF_CDFLOGO, "[IImgCtx downloading logo %s %s]", szLocalFile,
             SUCCEEDED(hr) ? TEXT("SUCCEEDED") : TEXT("FAILED"));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::Run *** 
//
//   IRunnableTask method.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::Run(
    void
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::Kill *** 
//
//   IRunnableTask method.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::Kill(
    BOOL fWait
)
{
    m_fDone = TRUE;

    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::Suspend *** 
//
//   IRunnableTask method.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::Suspend(
    void
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::Resume *** 
//
//   IRunnableTask method.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIconHandler::Resume(
    void
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CIconHandler::IsRunning *** 
//
//   IRunnableTask method.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CIconHandler::IsRunning(
    void
)
{
    return E_NOTIMPL;
}

