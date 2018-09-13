//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// exticon.cpp 
//
//   IExtractIcon com object.  Used by the shell to obtain icons.
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
#include "exticon.h"
#include "dll.h"
#include "persist.h"

//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::CExtractIcon ***
//
//    Constructor.
//
////////////////////////////////////////////////////////////////////////////////
CExtractIcon::CExtractIcon (
    PCDFITEMIDLIST pcdfidl,
    IXMLElementCollection *pIXMLElementCollection
)
: m_cRef(1)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)pcdfidl)));
    ASSERT(XML_IsCdfidlMemberOf(pIXMLElementCollection, pcdfidl));

    ASSERT(NULL == m_bstrIconURL);
    ASSERT(FALSE == m_fGleam);

    //
    // Set the default icon type.
    //

    if (CDFIDL_IsFolderId(&pcdfidl->mkid))
    {
        m_iconType = IDI_CLOSESUBCHANNEL;
    }
    else
    {
        m_iconType = IDI_STORY;
    }

    //
    // Get the URL for the custom icon.
    //

    if (pIXMLElementCollection)
    {
        IXMLElement* pIXMLElement;

        HRESULT hr;

        if (CDFIDL_GetIndex(pcdfidl) != -1)
        {
            hr = XML_GetElementByIndex(pIXMLElementCollection,
                                       CDFIDL_GetIndex(pcdfidl), &pIXMLElement);
        }
        else
        {
            IXMLElement *pIXMLElementChild;

            hr = XML_GetElementByIndex(pIXMLElementCollection, 0, &pIXMLElementChild);

            if (pIXMLElementChild)
            {
                hr = pIXMLElementChild->get_parent(&pIXMLElement);
                if (!pIXMLElement)
                {
                    ASSERT(FALSE);
                    hr = E_FAIL;
                }
                pIXMLElementChild->Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            m_bstrIconURL = XML_GetAttribute(pIXMLElement, XML_ICON);

            pIXMLElement->Release();
        }
    }

    //
    // Don't allow the DLL to unload.
    //

    TraceMsg(TF_OBJECTS, "+ IExtractIcon");

    DllAddRef();

    return;
}

// Used for initializing the Root Element
CExtractIcon::CExtractIcon (
    PCDFITEMIDLIST pcdfidl,
    IXMLElement *pElem
)
: m_cRef(1)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)pcdfidl)));
    ASSERT(NULL == m_bstrIconURL);
    ASSERT(FALSE == m_fGleam);

    //
    // Set the default icon type.
    //


    m_iconType = IDI_CHANNEL;
    

    //
    // Get the URL for the custom icon.
    //

    if (pElem)
    {
        HRESULT hr; 
        IXMLElement *pDeskElem;
        LONG nIndex;
        
        hr = XML_GetDesktopElementFromChannelElement(pElem, &pDeskElem, &nIndex);
        if (SUCCEEDED(hr))
        {
            m_iconType = IDI_DESKTOP;
            pDeskElem->Release();
        }
            
        m_bstrIconURL = XML_GetAttribute(pElem, XML_ICON);
    }

    //
    // Don't allow the DLL to unload.
    //

    TraceMsg(TF_OBJECTS, "+ IExtractIcon");

    DllAddRef();

    return;
}

// this constructor is used for the default channel case where
// we draw the icon information from the desktop.ini case 
// to avoid having to parse the XML stuff

CExtractIcon::CExtractIcon( BSTR pstrPath ) : m_cRef(1)
{
    ASSERT(NULL == m_bstrIconURL);
    ASSERT(FALSE == m_fGleam);
    
    m_iconType = IDI_CHANNEL;
    
    m_bstrIconURL = SysAllocString( pstrPath );
    
    DllAddRef();
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::~CExtractIcon ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CExtractIcon::~CExtractIcon (
    void
)
{
    ASSERT(0 == m_cRef);

    if (m_bstrIconURL)
        SysFreeString(m_bstrIconURL);

    //
    // Matching Release for the constructor Addref.
    //

    TraceMsg(TF_OBJECTS, "- IExtractIcon");

    DllRelease();

    return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::QueryInterface ***
//
//    CExtractIcon QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CExtractIcon::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    if (IID_IUnknown == riid || IID_IExtractIcon == riid)
    {
        AddRef();
        *ppv = (IExtractIcon*)this;
        hr = S_OK;
    }
#ifdef UNICODE
    else if (IID_IExtractIconA == riid)
    {
        AddRef();
        *ppv = (IExtractIconA*)this;
        hr = S_OK;
	}
#endif
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::AddRef ***
//
//    CExtractIcon AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CExtractIcon::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::Release ***
//
//    CExtractIcon Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CExtractIcon::Release (
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
// *** CExtractIcon::GetIconLocation ***
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
//     The shell can cache an icon associated with a name index pair. This
//     improves performance on subsequent calls for an icon with the same name
//     index pair.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CExtractIcon::GetIconLocation(
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

    HRESULT hr = E_FAIL;

    //TraceMsg(TF_CDFICON, "<IN>  CExtractIcon::GetLocation (icon) tid:0x%x",
    //         GetCurrentThreadId());

    if (m_bstrIconURL && (uFlags & GIL_ASYNC))
    {
        hr = E_PENDING;
    }
    else
    {
        if (m_bstrIconURL)
        {
            hr = GetCustomIconLocation(uFlags, szIconFile, cchMax, piIndex,
                                       pwFlags);

            if (FAILED(hr))
            {
                SysFreeString(m_bstrIconURL);
                m_bstrIconURL = NULL;
            }
        }

        if (FAILED(hr))
        {
            hr = GetDefaultIconLocation(uFlags, szIconFile, cchMax, piIndex,
                                        pwFlags);
        }

        //
        // If szIconFile is a path the shell will only use the filename part
        // of the path as the cache index.  To ensure a unique index the full
        // path must be used.  This is accomplished by modifying the path string
        // so it is no longer recognized as a path.
        //

        if (SUCCEEDED(hr) && INDEX_IMAGE == *piIndex)
            MungePath(szIconFile);

        if (FAILED(hr))
        {
            *szIconFile = TEXT('\0');
            *piIndex = 0;

            hr = S_FALSE;  // The shell will use a default icon.
        }


        ASSERT((S_OK == hr && *szIconFile) ||
               (S_FALSE == hr && 0 == *szIconFile));
    }

    //TraceMsg(TF_CDFICON, "<OUT> CExtractIcon::GetLocation (icon) tid:0x%x",
    //         GetCurrentThreadId());

    return hr;
}
#ifdef UNICODE
// IExtractIconA methods.
STDMETHODIMP
CExtractIcon::GetIconLocation(
    UINT uFlags,
    LPSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags
)
{
    HRESULT hr;
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
// *** CExtractIcon::Extract ***
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
//     If the icon index indicates that the icon is specified by an internet
//     image then custom extraction is required.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CExtractIcon::Extract(
    LPCTSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize
)
{
    HRESULT hr;

    TCHAR  szPath[MAX_PATH];
    TCHAR* pszPath = szPath;

    StrCpyN(szPath, pszFile, ARRAYSIZE(szPath) - 1);

    //TraceMsg(TF_CDFICON, "<IN>  CExtractIcon::Extract (icon) tid:0x%x",
    //         GetCurrentThreadId());

    if (INDEX_IMAGE == nIconIndex)
    {
        DemungePath(pszPath);

        if (m_fGleam && *pszPath == TEXT('G'))
        {
            pszPath++;
        }

        IImgCtx* pIImgCtx;

        HANDLE hExitThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (hExitThreadEvent)
        {
#ifdef UNIX
            unixEnsureFileScheme(pszPath);
#endif /* UNIX */
            hr = SynchronousDownload(pszPath, &pIImgCtx, hExitThreadEvent);

            if (SUCCEEDED(hr))
            {
                ASSERT(pIImgCtx);

                *phiconLarge = ExtractImageIcon(LOWORD(nIconSize), pIImgCtx,
                                                m_fGleam);
                *phiconSmall = ExtractImageIcon(HIWORD(nIconSize), pIImgCtx,
                                                m_fGleam);
                pIImgCtx->Release();
            }

            SetEvent(hExitThreadEvent);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (m_fGleam)
    {
        // Add gleam to icon for the shell

        hr = ExtractGleamedIcon(pszPath + 1, nIconIndex, 0, 
                phiconLarge, phiconSmall, nIconSize);
    }
    else
    {
        hr = S_FALSE;  // Let shell extract it.
    }

    //TraceMsg(TF_CDFICON, "<OUT> CExtractIcon::Extract (icon) tid:0x%x",
    //         GetCurrentThreadId());

    return hr;
}
#ifdef UNICODE
STDMETHODIMP
CExtractIcon::Extract(
    LPCSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    HRESULT hr;
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
// Helper functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::GetCustomIconLocation ***
//
//
// Description:
//     Gets the location string name for an icon specified via a LOGO element
//     in a cdf.
//
// Parameters:
//     [In]  uFlags     - GIL_FORSHELL, GIL_OPENICON.
//     [Out] szIconFile - The address of the buffer that receives the associated
//                        icon name.
//     [In]  cchMax     - Size of the buffer that receives the icon location.
//     [Out] piIndex    - A pointer that receives the icon's index.
//     [Out] pwFlags    - A pointer the receives flags about the icon.
//
// Return:
//     S_OK if the custom icon location was determined.
//     E_FAIL if the location couldn't be determined.
//
// Comments:
//     If the extension of the image url isn't .ico then it's treated as an
//     internet image file.  IImgCtx is used to convert these files into icons.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CExtractIcon::GetCustomIconLocation(
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

    ASSERT(m_bstrIconURL);

    *piIndex = 0;
    *pwFlags = 0;

    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
  
    if (SHUnicodeToTChar(m_bstrIconURL, szURL, ARRAYSIZE(szURL)))
    {
        hr = URLGetLocalFileName(szURL, szIconFile, cchMax, NULL);

        #ifdef DEBUG
            if (SUCCEEDED(hr))
            {
                TraceMsg(TF_CDFICON, "[URLGetLocalFileName %s]", szIconFile);
            }
            else
            {
                TraceMsg(TF_CDFICON, "[URLGetLocalFileName %s FAILED]",
                         szURL);
            }
        #endif // DEBUG

        //hr = URLDownloadToCacheFile(NULL, szURL, szIconFile, cchMax, 0, NULL);

        if (SUCCEEDED(hr))
        {
            LPTSTR pszExt = PathFindExtension(szIconFile);

            if (*pszExt != TEXT('.') || 0 != StrCmpI(pszExt, TSTR_ICO_EXT))
                *piIndex = INDEX_IMAGE;
        }
    }
    else
    {
        *szIconFile = TEXT('\0');

        hr = E_FAIL;
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::GetDefaultIconLocation ***
//
//
// Description:
//     Return the location of the defualt icon.
//
// Parameters:
//     [In]  uFlags     - GIL_FORSHELL, GIL_OPENICON.
//     [Out] szIconFile - The address of the buffer that receives the associated
//                        icon name.
//     [In]  cchMax     - Size of the buffer that receives the icon location.
//     [Out] piIndex    - A pointer that receives the icon's index.
//     [Out] pwFlags    - A pointer the receives flags about the icon.
//
// Return:
//     S_OK if the default location is returned.
//     E_FAIL otherwise.
//
// Comments:
//     The default icons are in the resource file.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CExtractIcon::GetDefaultIconLocation(
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

    *pwFlags = 0;

    ASSERT(g_szModuleName[0]);

    StrCpyN(szIconFile, g_szModuleName, cchMax);

    if (*szIconFile)
    {
        switch (m_iconType)
        {
            case IDI_STORY:
            case IDI_CHANNEL:
            case IDI_DESKTOP:
                *piIndex = - m_iconType;
                break;

            default:
                *piIndex = (uFlags & GIL_OPENICON) ? 
                                (-IDI_OPENSUBCHANNEL) : 
                                (-IDI_CLOSESUBCHANNEL);
                break;
        }
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    ASSERT((SUCCEEDED(hr) && *szIconFile) || FAILED(hr));

    return hr;
}

struct ThreadData
{
    HANDLE hEvent;
    HANDLE hExitThreadEvent;
    IImgCtx * pImgCtx;
    LPCWSTR pszBuffer;
    HRESULT * pHr;
};

DWORD CALLBACK SyncDownloadThread( LPVOID pData )
{
    ThreadData * pTD = (ThreadData * ) pData;

    HANDLE hExitThreadEvent = pTD->hExitThreadEvent;

    CoInitialize(NULL);
    pTD->pImgCtx = NULL;
    
    HRESULT hr;
    hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                          IID_IImgCtx, (void**)&(pTD->pImgCtx));
    if (SUCCEEDED(hr))
    {
        hr = pTD->pImgCtx->Load(pTD->pszBuffer, 0);

        if (SUCCEEDED(hr))
        {
            ULONG fState;
            SIZE  sz;
            BOOL fDone = FALSE;

            pTD->pImgCtx->GetStateInfo(&fState, &sz, TRUE);

            if (!(fState & (IMGLOAD_COMPLETE | IMGLOAD_ERROR)))
            {
                BOOL fDone = FALSE;

                hr = pTD->pImgCtx->SetCallback(ImgCtx_Callback, &fDone);

                if (SUCCEEDED(hr))
                {
                    hr = pTD->pImgCtx->SelectChanges(IMGCHG_COMPLETE, 0, TRUE);

                    if (SUCCEEDED(hr))
                    {
                        MSG msg;
                        BOOL fMsg;

                        // HACK: restrict the message pump to those messages we know that URLMON and
                        // HACK: the imageCtx stuff needs, otherwise we will be pumping messages for
                        // HACK: windows we shouldn't be pumping right now...
                        while(!fDone )
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

            }

            hr = pTD->pImgCtx->GetStateInfo(&fState, &sz, TRUE);

            if (SUCCEEDED(hr))
                hr = (fState & IMGLOAD_ERROR) ? E_FAIL : S_OK;
        }

        // Must disconnect on the same thread that SetCallback is
        // done.  This object becomes a primary object on the thread
        // which connects the callback function.  The primary object
        // count is decremented when Disconnect is called, or when the
        // object is released.  In this case, the release is definitely
        // going to happen on a different thread than this one, so we
        // need to disconnect the callback function right now before
        // returning.  There is no further needs for callbacks at this
        // point.

        pTD->pImgCtx->Disconnect();
    }

    if ( FAILED( hr ) && pTD->pImgCtx )
    {
        pTD->pImgCtx->Release();
        pTD->pImgCtx = NULL;
    }
    
    *(pTD->pHr) = hr;
    
    SetEvent( pTD->hEvent );

    //
    // Wait for the calling thread to finish up with IImgCtx before 
    // CoUninitialize gets called.
    //

    WaitForSingleObject(hExitThreadEvent, INFINITE);
    CloseHandle(hExitThreadEvent);

    CoUninitialize();

    return 0;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::SynchronousDownload ***
//
//
// Description:
//     Synchronously downloads the image associated with the image context.
//
// Parameters:
//     [In]  szFile           - The local (already in cache) file name of the
//                              image.
//     [In]  pIImgCtx         - A pointer to the image context.
//     [In]  hExitThreadEvent - An event that gets signaled when the IImgCtx
//                              object is no longer in use.
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
CExtractIcon::SynchronousDownload(
    LPCTSTR  pszFile,
    IImgCtx** ppIImgCtx,
    HANDLE hExitThreadEvent
)
{
    ASSERT(ppIImgCtx);

    HRESULT hr;

    TraceMsg(TF_CDFPARSE, "[*** IImgCtx downloading logo %s ***]",
             pszFile);
    TraceMsg(TF_CDFICON, "[*** IImgCtx downloading logo %s ***]",
             pszFile);

    WCHAR szFileW[MAX_PATH];

    SHTCharToUnicode(pszFile, szFileW, ARRAYSIZE(szFileW));

    ThreadData rgData;
    rgData.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( rgData.hEvent == NULL )
    {
        CloseHandle(hExitThreadEvent);
        return E_OUTOFMEMORY;
    }

    rgData.hExitThreadEvent = hExitThreadEvent;
    rgData.pszBuffer = szFileW;
    rgData.pHr = &hr;

    *ppIImgCtx = NULL;
    
    if ( SHCreateThread( SyncDownloadThread, &rgData, 0, NULL ))
    {
        WaitForSingleObject( rgData.hEvent, INFINITE );
        *ppIImgCtx = rgData.pImgCtx;
    }
    else
    {
        CloseHandle(hExitThreadEvent);
        hr = E_OUTOFMEMORY;
    }

    CloseHandle( rgData.hEvent );

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::ExtractImageIcon ***
//
//
// Description:
//     Returns an HICON for the image in IImgCtx.
//
// Parameters:
//     [In]  wSize      - The height and width of the icon,
//     [In]  pIImgCtx   - The image to convert into an icon.
//     [In]  fDrawGleam - TRUE if a gleam should be added, FALSE otherwise.
//
// Return:
//     An hicon of size nSize for the given IImgCtx.
//     NULL on failure.
//
// Comments:
//     Uses the image in IImgCtx to create bitmaps to pass to
//     CreateIconIndirect.
//
////////////////////////////////////////////////////////////////////////////////
HICON
CExtractIcon::ExtractImageIcon(
    WORD wSize,
    IImgCtx* pIImgCtx,
    BOOL fDrawGleam
)
{
    ASSERT(pIImgCtx);

    HICON hiconRet = NULL;

    HDC hdcScreen = GetDC(NULL);

    if (hdcScreen)
    {
        HBITMAP hbmImage = CreateCompatibleBitmap(hdcScreen, wSize, wSize);

        if (hbmImage)
        {
            HBITMAP hbmMask = CreateBitmap(wSize, wSize, 1, 1, NULL);

            if (hbmMask)
            {
                SIZE sz;
                sz.cx = sz.cy = wSize;

                if (SUCCEEDED(CreateImageAndMask(pIImgCtx, hdcScreen, &sz,
                                                 &hbmImage, &hbmMask,
                                                 fDrawGleam)))
                {
                    ICONINFO ii;

                    ii.fIcon    = TRUE;
                    ii.hbmMask  = hbmMask;
                    ii.hbmColor = hbmImage;

                    hiconRet = CreateIconIndirect(&ii); 
                }

                DeleteObject(hbmMask);
            }

            DeleteObject(hbmImage);
        }

        ReleaseDC(NULL, hdcScreen);
    }

    return hiconRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::CreateImageAndMask ***
//
//
// Description:
//     Create the image and mask bitmaps that get used by CreateIconIndirect,
//
// Parameters:
//    [In]  IImgCtx       - The internet image.
//    [In]  hdcScreen     - The screen hdc.
//    [In]  pSize         - The size of the image and mask bitmaps.
//    [In Out] phbmImage  - A pointer to the handle of the Image bitmap.
//    [In Out] phbmMask   - A pointer to the handle of the Mask bitmap.
//    [In]  fDrawGleam    - TRUE if a gleam should be added, FALSE otherwise.
//
// Return:
//    S_OK if the image and mask bitmaps where successfully created.
//    E_FAIL if the image or mask couldn't be created.
//
// Comments:
//    The image bitmap has the opaque section come through and the transparent
//    sections set to black.
//
//    The mask has the transparent sections set to 1 and the opaque sections to
//    0.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CExtractIcon::CreateImageAndMask(
    IImgCtx* pIImgCtx,
    HDC hdcScreen,
    SIZE* pSize,
    HBITMAP* phbmImage,
    HBITMAP* phbmMask,
    BOOL fDrawGleam
)
{
    ASSERT(pIImgCtx);
    ASSERT(phbmImage);
    ASSERT(phbmMask);

    HRESULT hr = E_FAIL;

    HDC hdcImgDst = CreateCompatibleDC(NULL);
    if (hdcImgDst)
    {
        HGDIOBJ hbmOld = SelectObject(hdcImgDst, *phbmImage);
        if (hbmOld)
        {
            if (ColorFill(hdcImgDst, pSize, COLOR1))
            {
                hr = StretchBltImage(pIImgCtx, pSize, hdcImgDst, fDrawGleam);

                if (SUCCEEDED(hr))
                {
                    hr = CreateMask(pIImgCtx, hdcScreen, hdcImgDst, pSize,
                                    phbmMask, fDrawGleam); 
                }
            }
            SelectObject(hdcImgDst, hbmOld);
        }
        DeleteDC(hdcImgDst);
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::StretchBltImage ***
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
//     The destination DC already has a bitmap of pSize selected into it.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CExtractIcon::StretchBltImage(
    IImgCtx* pIImgCtx,
    const SIZE* pSize,
    HDC hdcDst,
    BOOL fDrawGleam
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
        hr = pIImgCtx->StretchBlt(hdcDst, 0, 0, pSize->cx, pSize->cy, 0, 0,
                                  sz.cx, sz.cy, SRCCOPY);

        ASSERT(SUCCEEDED(hr) && "Icon extraction pIImgCtx->StretchBlt failed!");

        if (fDrawGleam)
        {
            hr = E_FAIL;

            HANDLE hGleam = LoadImage(g_hinst, TEXT("ICONGLEAM"),
                                      IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

            if (hGleam)
            {
                if (DrawIconEx(hdcDst, 0, 0, (HICON)hGleam, pSize->cx, pSize->cy, 0, NULL,DI_NORMAL))
                    hr = S_OK;

                DeleteObject(hGleam);
            }            
        }
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::CreateMask ***
//
//
// Description:
//     Creates the mask for an icon and also adjusts the image bitmap for use
//     with the mask.
//
// Parameters:
//     [In]  pIImgCtx   - The original image.
//     [In]  hdcScreen  - A screen dc.
//     [In/Out] hdc1    - The DC containing the image bitmap.
//     [In]  pSize      - The size of the bitmaps.
//     [In/Out] phbMask - A pointer to the handle of the mask bitmap
//
// Return:
//     S_OK if the mask is properly constructed.
//     E_FAIL otherwise.
//
// Comments:
//     The mask is created by first drawing the original image into a bitmap
//     with background COLOR1.  Then the same image is drawn into another
//     bitmap but this bitmap has background of COLOR2.  These two bitmaps
//     are XOR'ed and the opaque sections come out 0 while the transparent
//     sections are COLOR1 XOR COLOR2.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CExtractIcon::CreateMask(
    IImgCtx* pIImgCtx,
    HDC hdcScreen,
    HDC hdc1,
    const SIZE* pSize,
    HBITMAP* phbMask,
    BOOL fDrawGleam
)
{
    ASSERT(hdc1);
    ASSERT(pSize);
    ASSERT(phbMask);

    HRESULT hr = E_FAIL;

    HDC hdc2 = CreateCompatibleDC(NULL);
    if (hdc2)
    {
        HBITMAP hbm2 = CreateCompatibleBitmap(hdcScreen, pSize->cx, pSize->cy);
        if (hbm2)
        {
            HGDIOBJ hbmOld2 = SelectObject(hdc2, hbm2);
            if (hbmOld2)
            {
                ColorFill(hdc2, pSize, COLOR2);

                hr = StretchBltImage(pIImgCtx, pSize, hdc2, fDrawGleam);

#ifndef UNIX
                if (SUCCEEDED(hr) &&
                    BitBlt(hdc2, 0, 0, pSize->cx, pSize->cy, hdc1, 0, 0,
                           SRCINVERT))
                {
                    if (GetDeviceCaps(hdcScreen, BITSPIXEL) <= 8)
                    {
                        //
                        // 6 is the XOR of the index for COLOR1 and the index
                        // for COLOR2.
                        //

                        SetBkColor(hdc2, PALETTEINDEX(6));
                    }
                    else
                    {
                        SetBkColor(hdc2, (COLORREF)(COLOR1 ^ COLOR2));
                    }

                    HDC hdcMask = CreateCompatibleDC(NULL);
                    if (hdcMask)
                    {
                        HGDIOBJ hbmOld = SelectObject(hdcMask, *phbMask);
                        if (hbmOld)
                        {
                            if (BitBlt(hdcMask, 0, 0, pSize->cx, pSize->cy, hdc2, 0,
                                       0, SRCCOPY))
                            {
                                //
                                // RasterOP 0x00220326 does a copy of the ~mask bits
                                // of hdc1 and sets everything else to 0 (Black).
                                //

                                if (BitBlt(hdc1, 0, 0, pSize->cx, pSize->cy, hdcMask,
                                           0, 0, 0x00220326))
                                {
                                    hr = S_OK;
                                }
                            }
                            SelectObject(hdcMask, hbmOld);
                        }
                        DeleteDC(hdcMask);
                    }
                }
#else
        SetBkColor(hdc2, COLOR2);
        HDC hdcMask = CreateCompatibleDC(NULL);
        if (hdcMask)
        {
            HGDIOBJ hbmOld = SelectObject(hdcMask, *phbMask);
            if (hbmOld)
            {
            if (BitBlt(hdcMask, 0, 0, pSize->cx, pSize->cy, hdc2, 0,
                   0, SRCCOPY))
                {
                            //
                            // RasterOP 0x00220326 does a copy of the ~mask bits
                            // of hdc1 and sets everything else to 0 (Black).
                            //

                if (BitBlt(hdc1, 0, 0, pSize->cx, pSize->cy, hdcMask,
                       0, 0, 0x00220326))
                  {
                hr = S_OK;
                  }
              }
              SelectObject(hdcMask, hbmOld);
              }
              DeleteDC(hdcMask);
          }
#endif /* UNIX */
                SelectObject(hdc2, hbmOld2);
            }

            DeleteObject(hbm2);
        }

        DeleteDC(hdc2);
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** ImgCtx_Callback ***
//
//
// Description:
//     Callback function for IImgCtx loads.
//
// Parameters:
//     [In]  pIImgCtx - Not Used.
//     [Out] pfDone   - Set to TRUE on this callback.  
//
// Return:
//     None.
//
// Comments:
//     This callback gets called if IImgCtx is finished downloading an image.
//     It is used in CExtractIcon and CIconHandler.
//
////////////////////////////////////////////////////////////////////////////////
void
CALLBACK
ImgCtx_Callback(
    void* pIImgCtx,
    void* pfDone
)
{
    ASSERT(pfDone);

    *(BOOL*)pfDone = TRUE;

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::ColorFill ***
//
//
// Description:
//     Fills the given bitmap with the given color.
//
// Parameters:
//     [In/Out] hdc - The hdc that contains the bitmap.
//     [In]  pSize  - the size of the bitmap.
//     [In]  clr    - The color used to fill in the bitmap.
//
// Return:
//     TRUE if the bitmap was filled with color clr.
//     FALSE if the itmap wasn't filled.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
CExtractIcon::ColorFill(
    HDC hdc,
    const SIZE* pSize,
    COLORREF clr
)
{
    ASSERT(hdc);

    BOOL fRet = FALSE;

    HBRUSH hbSolid = CreateSolidBrush(clr);
    if (hbSolid)
    {
        HGDIOBJ hbOld = SelectObject(hdc, hbSolid);
        if (hbOld)
        {
            PatBlt(hdc, 0, 0, pSize->cx, pSize->cy, PATCOPY);
            fRet = TRUE;

            SelectObject(hdc, hbOld);
        }
        DeleteObject(hbSolid);
    }

    return fRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::ExtractGleamedIcon ***
//
//
// Description:
//     Extracts icon resources and applies gleams to them.
//
// Parameters:
//      [In] pszIconFile - path to the icon
//      [In] iIndex - index of icon with the file
//      [In] uFlags - ignore, pass 0
//      [Out] phiconLarge - HICON in large format with gleam
//      [Out] phiconSmall - HICON in small format with gleam
//
// Return:
//      S_OK if success
//      S_FALSE if the file has no icons (or not the asked for icon)
//      E_FAIL for files on a slow link.
//      E_FAIL if cant access the file
//      E_FAIL if gleam icon construction failed
//
////////////////////////////////////////////////////////////////////////////////
HRESULT 
CExtractIcon::ExtractGleamedIcon(
    LPCTSTR pszIconFile, 
    int iIndex, 
    UINT uFlags,
    HICON *phiconLarge, 
    HICON *phiconSmall, 
    UINT nIconSize)
{
    HICON   hIconLargeShell, hIconSmallShell;
    HRESULT hr;

    hr = Priv_SHDefExtractIcon(pszIconFile, iIndex, uFlags, 
                &hIconLargeShell, &hIconSmallShell, nIconSize);

    if (FAILED(hr))
        goto cleanup1;

    if (hIconLargeShell)
    {
        hr = ApplyGleamToIcon(hIconLargeShell, LOWORD(nIconSize), phiconLarge);
        if (FAILED(hr))
            goto cleanup2;
    }

    if (hIconSmallShell)
    {
        hr = ApplyGleamToIcon(hIconSmallShell, HIWORD(nIconSize), phiconSmall);
        if (FAILED(hr))
            goto cleanup3;
    }
    
cleanup3:
    if (FAILED(hr) && *phiconLarge)
    {
        DestroyIcon(*phiconLarge);
        *phiconLarge = NULL;
    }
    
cleanup2:
    if (hIconLargeShell)
        DestroyIcon(hIconLargeShell);
        
    if (hIconSmallShell)        
        DestroyIcon(hIconSmallShell);
        
cleanup1:
    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CExtractIcon::ApplyGleamToIcon ***
//
//
// Description:
//     Makes a gleamed version of an icon.
//
// Parameters:
//      [In] hIcon - icon that needs to be gleamed
//      [In] nSize - size of icon in pixels
//      [Out] phGleamIcon - variable to contain the gleamed icon
//      
//
// Return:
//      S_OK if success
//      E_FAIL if unsuccessful
//
////////////////////////////////////////////////////////////////////////////////
HRESULT 
CExtractIcon::ApplyGleamToIcon(
    HICON hIcon1, 
    ULONG nSize,
    HICON *phGleamedIcon)
{
    HRESULT hr = E_FAIL;

    HICON hIcon2 = (HICON)LoadImage(g_hinst, TEXT("ICONGLEAM"), IMAGE_ICON,
                                    nSize, nSize, 0);

    if (hIcon2)
    {
        HDC dc = GetDC(NULL);

        if (dc)
        {
            ICONINFO ii1, ii2;

            if (GetIconInfo(hIcon1, &ii1) && GetIconInfo(hIcon2, &ii2))
            {
                HDC dcSrc = CreateCompatibleDC(dc);

                if (dcSrc)
                {

                    HDC dcDst = CreateCompatibleDC(dc);

                    if (dcDst)
                    {
                        HBITMAP bmMask = CreateBitmap(nSize, nSize, 1, 1, NULL);

                        if (bmMask)
                        {
                            HBITMAP bmImage = CreateCompatibleBitmap(dc, nSize,
                                                                     nSize);

                            if (bmImage)
                            {
                                int cx1, cy1, cx2, cy2;
                                GetBitmapSize(ii1.hbmMask, &cx1, &cy1);
                                GetBitmapSize(ii2.hbmMask, &cx2, &cy2);

                                //
                                // Mask
                                //

                                HBITMAP hbmpOldDst = (HBITMAP)SelectObject(
                                                                        dcDst,
                                                                        bmMask);

                                HBITMAP hbmpOldSrc = (HBITMAP)SelectObject(
                                                                   dcSrc,
                                                                   ii1.hbmMask);
                                StretchBlt(dcDst, 0, 0, nSize, nSize, dcSrc, 0,
                                           0, cx1, cy1, SRCCOPY);

                                SelectObject(dcSrc, ii2.hbmMask);

                                StretchBlt(dcDst, 0, 0, nSize, nSize, dcSrc, 0,
                                           0, cx2, cy2, SRCAND);

                                //
                                // Image.
                                //

                                SelectObject(dcDst, bmImage);

                                SelectObject(dcSrc, ii1.hbmColor);

                                StretchBlt(dcDst, 0, 0, nSize, nSize, dcSrc, 0,
                                           0, cx1, cy1, SRCCOPY);

                                SelectObject(dcSrc, ii2.hbmMask);

                                StretchBlt(dcDst, 0, 0, nSize, nSize, dcSrc, 0,
                                           0, cx2, cy2, SRCAND);

                                SelectObject(dcSrc, ii2.hbmColor);

                                StretchBlt(dcDst, 0, 0, nSize, nSize, dcSrc, 0,
                                           0, cx2, cy2, SRCINVERT);

                                ii1.hbmMask  = bmMask;
                                ii1.hbmColor = bmImage;

                                *phGleamedIcon = CreateIconIndirect(&ii1);

                                if (*phGleamedIcon)
                                    hr = S_OK;

                                SelectObject(dcSrc, hbmpOldSrc);
                                SelectObject(dcDst, hbmpOldDst);

                                DeleteObject(bmImage);
                            }

                            DeleteObject(bmMask);
                        }

                        DeleteDC(dcDst);
                    }

                    DeleteDC(dcSrc);
                }
            }

            ReleaseDC(NULL, dc);
        }

        DestroyIcon(hIcon2);
    }

    return hr;
}

//
// Get the size of the given bitmap.
//

BOOL
CExtractIcon::GetBitmapSize(HBITMAP hbmp, int* pcx, int* pcy)
{
    BOOL fRet;

    BITMAP bm;

    if (GetObject(hbmp, sizeof(bm), &bm))
    {
        *pcx = bm.bmWidth;
        *pcy = bm.bmHeight;
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

//
//  Replace '\' with '*' so the path is nolonger a recognized path name.  This
//  is done in-place and can be called multiple times on the same string.
//

void
MungePath(LPTSTR pszPath)
{
    ASSERT(pszPath);

    while(*pszPath)
    {
        if (TEXT(FILENAME_SEPARATOR) == *pszPath)
            *pszPath = TEXT('*');

        pszPath++;
    }

    return;
}

//
//  Replace '*' with '\'.
//

void
DemungePath(LPTSTR pszPath)
{
    ASSERT(pszPath);

    while(*pszPath)
    {
        if (TEXT('*') == *pszPath)
            *pszPath = TEXT(FILENAME_SEPARATOR);

        pszPath++;
    }

    return;
}
