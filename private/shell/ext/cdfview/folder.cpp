//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// folder.cpp 
//
//   IShellFolder for the cdfview class.
//
//   History:
//
//       3/16/97  edwardp   Created.
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
#include "cdfview.h"
#include "enum.h"
#include "view.h"
#include "exticon.h"
#include "itemmenu.h"
#include "tooltip.h"



//
// IShellFolder methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::ParseDisplayName ***
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
STDMETHODIMP
CCdfView::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbcReserved,
    LPOLESTR lpszDisplayName,
    ULONG* pchEaten,
    LPITEMIDLIST* ppidl,
    ULONG* pdwAttributes
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::EnumObjects ***
//
//
// Description:
//     Returns an enumerator for this folder.
//
// Parameters:
//     [In]  hwndOwner    - Handle of the owner window.  Ignored.
//     [In]  grfFlags     - A combination of Folders, NonFolders and Include
//                          Hidden.
//     [Out] ppenumIdList - A pointer to receive the IEnumIDList interface.
//
// Return:
//     S_OK if the enumrator was created and returned.
//     E_OUTOFMEMORY if the enumerator couldn't be created.
//
// Comments:
//     The caller must Release() the returned enumerator.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::EnumObjects(
    HWND hwndOwner,
    DWORD grfFlags,
    LPENUMIDLIST* ppIEnumIDList
)
{
    ASSERT(ppIEnumIDList);

    TraceMsg(TF_CDFENUM, "<IN>  EnumObjects tid:0x%x", GetCurrentThreadId());

    HRESULT hr = S_OK;

    if (!m_bCdfParsed)
    {
        TraceMsg(TF_CDFPARSE, "IShellFolder EnumObjects(%s) %s",
                 hwndOwner ? TEXT("HWND") : TEXT("NULL"),
                 PathFindFileName(m_szPath));
        hr = ParseCdfFolder(NULL, PARSE_LOCAL);
    }

    if (SUCCEEDED(hr))
    {
        *ppIEnumIDList = (IEnumIDList*) new CCdfEnum(m_pIXMLElementCollection,
                                                     grfFlags, m_pcdfidl);

        hr = *ppIEnumIDList ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        *ppIEnumIDList = NULL;
    }


    ASSERT((SUCCEEDED(hr) && *ppIEnumIDList) ||
           (FAILED(hr) && NULL == *ppIEnumIDList));

    TraceMsg(TF_CDFENUM, "<OUT> EnumObjects tid:0x%x", GetCurrentThreadId());

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::BindToObject ***
//
//
// Description:
//     Creates an IShellFolder for a given subfolder.
//
// Parameters:
//     [In]  pidl        - Pointer to the id list of the subfolder.
//     []    pdcReserved - Not used.
//     [In]  riid        - The requested interface.
//     [Out] ppvOut      - A pointer to receive the returned interface.  
//
// Return:
//     S_OK if the request folder is created and the interface returned.
//     E_OUTOFMEMORY if there isn't enough memory to create the folder.
//     E_NOINTERFACE if the requested interface isn't supported.
//
// Comments:
//     This function is generaly called on a member of the current folder
//     to create a subfolder.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID* ppvOut
)
{
    ASSERT(ppvOut);

    //
    // REVIEW:  Hack to get around shell pidls.  Bug in shell!
    //

#if 1 //Hack
    while(!ILIsEmpty(pidl) && !CDFIDL_IsValidId((PCDFITEMID)&pidl->mkid))
        pidl = _ILNext(pidl);

    if (ILIsEmpty(pidl))
    {
        HRESULT hr = S_OK;

        if (!m_bCdfParsed)
        {
            TraceMsg(TF_CDFPARSE, "IShellFolder BindToObject (Hack) %s",
                     PathFindFileName(m_szPath));
            hr = ParseCdfFolder(NULL, PARSE_LOCAL);
        }

        if (SUCCEEDED(hr))
        {
            AddRef();
            *ppvOut = (void**)(IShellFolder*)this;
        }

        return hr;
    }
#endif //Hack

    ASSERT(CDFIDL_IsValid((PCDFITEMIDLIST)pidl));

    //
    // REVIEW: nsc.cpp calls this function with non-folder pidls.
    //         Currently remove the ASSERT and replace it with a check.  nsc
    //         shouldn't make this call with non-folder pidls.
    //

    //ASSERT(CDFIDL_IsFolderId((PCDFITEMID)&pidl->mkid));

    HRESULT hr = S_OK;

    *ppvOut = NULL;

    if (CDFIDL_IsFolderId((PCDFITEMID)&pidl->mkid))
    {
        if (!m_bCdfParsed)
        {
            TraceMsg(TF_CDFPARSE, "IShellFolder BindToObject %s",
                     PathFindFileName(m_szPath));
            hr = ParseCdfFolder(NULL, PARSE_LOCAL);
        }

        if (SUCCEEDED(hr))
        {
            ASSERT(XML_IsCdfidlMemberOf(m_pIXMLElementCollection,
                                        (PCDFITEMIDLIST)pidl));

            CCdfView* pCCdfView = (CCdfView*)new CCdfView((PCDFITEMIDLIST)pidl,
                                                          m_pidlPath,
                                                          m_pIXMLElementCollection);

            if (pCCdfView)
            {
                if (ILIsEmpty(_ILNext(pidl)))
                {
                    hr = pCCdfView->QueryInterface(riid, ppvOut);
                }
                else
                {
                    hr = pCCdfView->BindToObject(_ILNext(pidl), pbcReserved, riid,
                                                 ppvOut);
                }

                pCCdfView->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }


    ASSERT((SUCCEEDED(hr) && *ppvOut) || (FAILED(hr) && NULL == *ppvOut));
    
    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::BindToStorage ***
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
STDMETHODIMP
CCdfView::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID* ppvObj
)
{
    return E_NOTIMPL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::CompareIDs ***
//
//
// Description:
//     Determines the relative ordering of two objects given their id lists.
//
// Parameters:
//     [In]  lParam - Value specifying the type of comparison to perform.
//                    Currently ignored.  Always sort by name.
//     [In]  pidl1  - The id list of the first item to compare.
//     [In]  pidl2  - The id list of the second item to compare.
//
// Return:
//     The SCODE of the HRESULT (low word) is <0 if pidl1 comes before pidl2,
//     =0 if pidl1 is the same as pidl2 and >0 if pidl1 comes after pidl2.
//
// Comments:
//     Shell expects this function to never fail.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::CompareIDs(
    LPARAM lParam,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2
)
{
    ASSERT(CDFIDL_IsValid((PCDFITEMIDLIST)pidl1));
    ASSERT(CDFIDL_IsValid((PCDFITEMIDLIST)pidl2));

    SHORT sRes = CDFIDL_Compare((PCDFITEMIDLIST)pidl1,(PCDFITEMIDLIST)pidl2);

    return 0x0000ffff & sRes;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::CreateViewObject ***
//
//
// Description:
//     Creates a com object for the current folder implementing the specified
//     interface 
//
// Parameters:
//     [In]  hwndOwner - Owner window.  Ignored.
//     [In]  riid      - The interface to create.
//     [Out] ppvOut    - A pointer that receives the new object.
//
// Return:
//     S_OK if the requested object was successfully created.
//     E_NOINTERFACE if the object is not suppported.
//     E_OUTOFMEMORY if the pidl couldn't be cloned.
//     The return value from SHCreateShellFolderViewEx otherwise.
//
// Comments:
//     It is important to remember that the COM object created by
//     CreateViewObject must be a different object than the shell folder object.
//     The Explorer may call CreateViewObject more than once to create more than
//     one view object and expects them to behave as independent objects. A new
//     view object must be created for each call.
//
//     Request for IShellView return a default Shell implementation. 
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::CreateViewObject(
    HWND hwndOwner,
    REFIID riid,
    LPVOID* ppvOut
)
{
    ASSERT(ppvOut);

    //
    // This function is called when the cdf hasn't been parsed.  m_pcdfidl is
    // likely NULL in this case.  This doesn't appear to be a problem so the
    // ASSERT has been commented out.
    //

    // ASSERT(m_bCdfParsed);

    HRESULT hr;

    if (IID_IShellView == riid)
    {
        hr = CreateDefaultShellView((IShellFolder*)this,
                                    (LPITEMIDLIST)m_pidlPath,
                                    (IShellView**)ppvOut);
    }
    else
    {
        *ppvOut = NULL;

        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppvOut) || (FAILED(hr) && NULL == *ppvOut));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::GetAttributesOf ***
//
//
// Description:
//     Returns the common attributes of the given id lists.
//
// Parameters:
//     [In]  cidl            - The number of id lists passed in.
//     [In]  apidl           - An array of id list pointers.
//     [Out] pfAttributesOut - Address to receive the common attributes. These
//                             attributes are defined with the SFGAO_ prefix.
//                             For example SFGAO_FOLDER and SFGAO_CANDELETE.
//
// Return:
//     S_OK if the attributes of the given id lists could be determined.
//     E_FAIL otherwise.
//
// Comments:
//     The attributes of the given id lists are AND'ed to obtain the common
//     members.
//
//     Shell calls this on the root folder with cidl set to zero to get the
//     attributes of the root folder.  It also doesn't bother to check the
//     return value so make sure the attributes are set correctly for this
//     case.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::GetAttributesOf(
    UINT cidl,
    LPCITEMIDLIST* apidl,
    ULONG* pfAttributesOut
)
{
    ASSERT(apidl || cidl == 0);
    ASSERT(pfAttributesOut);

    ULONG fAttributeFilter = *pfAttributesOut;

    if (!m_bCdfParsed)
    {
        TraceMsg(TF_CDFPARSE, "IShellFolder GetAttributesOf %s",
                 PathFindFileName(m_szPath));
        ParseCdfFolder(NULL, PARSE_LOCAL);
    }

    if (m_pIXMLElementCollection)
    {
        if (cidl)
        {

            *pfAttributesOut = (ULONG)-1;

            while(cidl-- && *pfAttributesOut)
            {
                ASSERT(CDFIDL_IsValid((PCDFITEMIDLIST)apidl[cidl]));
                ASSERT(ILIsEmpty(_ILNext(apidl[cidl])));
                ASSERT(XML_IsCdfidlMemberOf(m_pIXMLElementCollection,
                                            (PCDFITEMIDLIST)apidl[cidl]));

                //
                // CDFIDL_GetAttributes returns zero on failure.
                //

                *pfAttributesOut &= CDFIDL_GetAttributes(
                                                   m_pIXMLElementCollection,
                                                   (PCDFITEMIDLIST)apidl[cidl],
                                                   fAttributeFilter);
            }
        }
        else
        {
            //
            // Return this folder's attributes.
            //

            *pfAttributesOut = SFGAO_FOLDER;

            if (XML_ContainsFolder(m_pIXMLElementCollection))
                *pfAttributesOut |= SFGAO_HASSUBFOLDER;
        }
    }
    else
    {
        //
        // m_pIXMLElementCollection == NULL in low memory situations.
        //

        *pfAttributesOut = 0;
    }

    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::GetUIObjectOf ***
//
//
// Description:
//     Creates a COM object implemeneting the requested interface for the
//     specidied id lists.
//
// Parameters:
//     [In]  hwndOwner - The owner window.
//     [In]  cidl      - The number of idlist passed in.
//     [In]  apild     - An array of id list pointers.
//     [In]  riid      - The requested interface.  Can be IExtractIcon,
//                       IContextMenu, IDataObject or IDropTarget.
//     []    prgfInOut - Not used.
//     [Out] ppvOut    - The pointer to receive the requested COM object.
//
// Return:
//     S_OK if the interface was created.
//     E_OUTOFMEMORY if the COM object couldn't be created.
//     E_NOINTERFACE if the requested interface isn't supported.
//     E_FAIL if cidl is zero.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    LPCITEMIDLIST* apidl,
    REFIID riid,
    UINT* prgfInOut,
    LPVOID * ppvOut
)
{
    ASSERT(apidl || 0 == cidl);
    ASSERT(ppvOut);

    // ASSERT(m_bCdfParsed);  Called when cdf is not parsed.

    #ifdef DEBUG
        for(UINT i = 0; i < cidl; i++)
        {
            ASSERT(CDFIDL_IsValid((PCDFITEMIDLIST)apidl[i]));
            ASSERT(ILIsEmpty(_ILNext(apidl[i])));
            ASSERT(XML_IsCdfidlMemberOf(m_pIXMLElementCollection,
                                        (PCDFITEMIDLIST)apidl[i]));
        }
    #endif // DEBUG

    HRESULT hr;

    *ppvOut = NULL;

    if (cidl)
    {
        if (IID_IExtractIcon == riid
#ifdef UNICODE
            || IID_IExtractIconA == riid
#endif
            )
        {
            ASSERT(1 == cidl);

            if (!m_bCdfParsed)
            {
                TraceMsg(TF_CDFPARSE, "IShellFolder IExtractIcon %s",
                         PathFindFileName(m_szPath));
                ParseCdfFolder(NULL, PARSE_LOCAL);
            }

#ifdef UNICODE
            CExtractIcon *pxi = new CExtractIcon((PCDFITEMIDLIST)apidl[0],
                                                      m_pIXMLElementCollection);

            if (riid == IID_IExtractIconW)
                *ppvOut = (IExtractIconW *)pxi;
            else
                *ppvOut = (IExtractIconA *)pxi;
#else
            *ppvOut = (IExtractIcon*)new CExtractIcon((PCDFITEMIDLIST)apidl[0],
                                                      m_pIXMLElementCollection); 
#endif
            hr = *ppvOut ? S_OK : E_OUTOFMEMORY;
        }
        else if (IID_IContextMenu == riid)
        {

        #if USE_DEFAULT_MENU_HANDLER

            hr = CDefFolderMenu_Create((LPITEMIDLIST)m_pcdfidl, hwndOwner, cidl,
                                       apidl, (IShellFolder*)this, MenuCallBack,
                                       NULL, NULL, (IContextMenu**)ppvOut);
        #else // USE_DEFAULT_MENU_HANDLER

            *ppvOut = (IContextMenu*)new CContextMenu((PCDFITEMIDLIST*)apidl,
                                                      m_pidlPath, cidl);

            hr = *ppvOut ? S_OK : E_OUTOFMEMORY;

        #endif // USE_DEFAULT_MENU_HANDLER

        }
        else if (IID_IQueryInfo == riid)
        {
            ASSERT(1 == cidl);
            
            if (!m_bCdfParsed)
            {
                TraceMsg(TF_CDFPARSE, "IShellFolder IQueryInfo %s",
                         PathFindFileName(m_szPath));
                ParseCdfFolder(NULL, PARSE_LOCAL);
            }

            *ppvOut = (IQueryInfo*)new CQueryInfo((PCDFITEMIDLIST)apidl[0],
                                                   m_pIXMLElementCollection);

            hr = *ppvOut ? S_OK : E_OUTOFMEMORY;
        } 
        else if (IID_IShellLink  == riid || IID_IDataObject == riid
#ifdef UNICODE
                || IID_IShellLinkA == riid
#endif
                )
        {
            ASSERT(1 == cidl); // IDataObject should handle cidl > 1!

            hr = QueryInternetShortcut((PCDFITEMIDLIST)apidl[0], riid, ppvOut);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        ASSERT(0);  // Is this ever called with cidl == 0?

        hr = E_FAIL;
    }

    ASSERT((SUCCEEDED(hr) && *ppvOut) || (FAILED(hr) && NULL == *ppvOut));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::GetDisplayNameOf ***
//
//
// Description:
//     Returns the diaply name for the specified Id list.
//
// Parameters:
//     [In]  pidl   - A pointer to the id list.
//     [In]  uFlags - SHGDN_NORMAL, SHGN_INFOLDER or SHGDN_FORPARSING.
//     [Out] lpName - A pointer to a STRRET structure that receives the name.
//
// Return:
//     S_OK if the name can be determined.
//     E_FAIL otherwise.
//
// Comments:
//     This may be called on the root element in which case the pidl is a shell
//     id list and not a cdf id list.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    DWORD uFlags,
    LPSTRRET lpName
)
{
    ASSERT(CDFIDL_IsValid((PCDFITEMIDLIST)pidl));
    ASSERT(ILIsEmpty(_ILNext(pidl)));
    ASSERT(XML_IsCdfidlMemberOf(m_pIXMLElementCollection,
                                (PCDFITEMIDLIST)pidl));
    ASSERT(lpName);

    return CDFIDL_GetDisplayName((PCDFITEMIDLIST)pidl, lpName);
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::SetNameOf ***
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
STDMETHODIMP
CCdfView::SetNameOf(
    HWND hwndOwner,
    LPCITEMIDLIST pidl,
    LPCOLESTR lpszName,
    DWORD uFlags,
    LPITEMIDLIST* ppidlOut
)
{
    return E_NOTIMPL;
}


//
// IPersistFolder method,
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::Initialize ***
//
//
// Description:
//     This function is called with the fully qualified id list (location) of
//     the selected cdf file.
//
// Parameters:
//     [In]  pidl - The pidl of the selected cdf file.  This pidl conatins the
//                  full path to the CDF.
//
// Return:
//     S_OK if content for the cdf file could be created.
//     E_OUTOFMEMORY otherwise.
//
// Comments:
//     This function can be called more than once for a given folder.  When a
//     CDFView is being instantiated from a desktop.ini file the shell calls
//     Initialize once before it calls GetUIObjectOf asking for IDropTarget.
//     After the GetUIObjectOf call the folder is Released.  It then calls
//     Initialize again on a new folder.  This time it keeps the folder and it
//     ends up being displayed.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCdfView::Initialize(
    LPCITEMIDLIST pidl
)
{
    ASSERT(pidl);

    HRESULT hr;

    ASSERT(NULL == m_pidlPath);

    m_pidlPath = ILClone(pidl);

    if (m_pidlPath)
    {
        hr = CPersist::Initialize(pidl);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


//
// Helper functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::ParseCdf ***
//
//
// Description:
//     Parses the cdf file associated with this folder.
//
// Parameters:
//     [In]  hwndOwner    - The parent window of any dialogs that need to be
//                          displayed.
//     [In]  dwFParseType - PARSE_LOCAL, PARSE_NET and PARSE_REPARSE.
//
// Return:
//     S_OK if the cdf file was found and successfully parsed.
//     E_FAIL otherwise.
//
// Comments:
//     Uses the m_pidlRoot that was set during IPersistFolder::Initialize.
//     
////////////////////////////////////////////////////////////////////////////////
HRESULT
CCdfView::ParseCdfFolder(
    HWND hwndOwner,
    DWORD dwParseFlags
)
{
    HRESULT hr;

    //
    // Parse the file and get the first channel element.
    //

    IXMLDocument* pIXMLDocument = NULL;

    hr = CPersist::ParseCdf(hwndOwner, &pIXMLDocument, dwParseFlags);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLDocument);

        IXMLElement*    pIXMLElement;
        LONG            nIndex;

        hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);
            //ASSERT(NULL == m_pcdfidl); Can be non-NULL on a reparse.

            if (m_pcdfidl)
                CDFIDL_Free(m_pcdfidl);

            if (m_pIXMLElementCollection)
                m_pIXMLElementCollection->Release();

            m_pcdfidl = CDFIDL_CreateFromXMLElement(pIXMLElement, nIndex);

            HRESULT hr2 = pIXMLElement->get_children(&m_pIXMLElementCollection);
            if(!m_pIXMLElementCollection)
            {
                ASSERT(hr2 != S_OK);
                hr = E_FAIL; 
            }
            ASSERT((S_OK == hr2 && m_pIXMLElementCollection) ||
                   (S_OK != hr2 && NULL == m_pIXMLElementCollection));

            pIXMLElement->Release();
        }
    }
    if (pIXMLDocument)
        pIXMLDocument->Release();

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::QueryInternetShortcut ***
//
//
// Description:
//     Sets up an internet shorcut object for the given URL.
//
// Parameters:
//     [In]  pszURL  - The URL.
//     [In]  riid    - The requested interface on the shortcut object.
//     [Out] ppvOut  - A pointer that receives the interface.
//
// Return:
//     S_OK if the object is created and the interface is found.
//     A COM error code otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
QueryInternetShortcut(
    LPCTSTR pszURL,
    REFIID riid,
    void** ppvOut
)
{
    ASSERT(pszURL);
    ASSERT(ppvOut);

    HRESULT hr = E_FAIL;

    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];

    if (SHTCharToUnicode(pszURL, wszURL, ARRAYSIZE(wszURL)))
    {
        BSTR bstrURL = SysAllocString(wszURL);

        if (bstrURL)
        {
            CDFITEM cdfi;

            cdfi.nIndex = 1;
            cdfi.cdfItemType = CDF_Folder;
            cdfi.bstrName = bstrURL;
            cdfi.bstrURL  = bstrURL;

            PCDFITEMIDLIST pcdfidl = CDFIDL_Create(&cdfi);

            if (pcdfidl)
            {
                hr = QueryInternetShortcut(pcdfidl, riid, ppvOut);

                CDFIDL_Free(pcdfidl);
            }

            SysFreeString(bstrURL);
        }
    }

    return hr;
}
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CCdfView::QueryInternetShortcut ***
//
//
// Description:
//     Sets up an internet shorcut object for the given pidl.
//
// Parameters:
//     [In]  pcdfidl - The shortcut object is created for the URL stored in this
//                     cdf item id list.
//     [In]  riid    - The requested interface on the shortcut object.
//     [Out] ppvOut  - A pointer that receives the interface.
//
// Return:
//     S_OK if the object is created and the interface is found.
//     A COM error code otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
QueryInternetShortcut(
    PCDFITEMIDLIST pcdfidl,
    REFIID riid,
    void** ppvOut
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)pcdfidl)));
    ASSERT(ppvOut);

    HRESULT hr;

    *ppvOut = NULL;

    //
    // Only create a shell link object if the CDF contains an URL
    //
    if (*(CDFIDL_GetURL(pcdfidl)) != 0)
    {
        IShellLinkA * pIShellLink;

        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IShellLinkA, (void**)&pIShellLink);


        BOOL bCoInit = FALSE;

        if ((CO_E_NOTINITIALIZED == hr || REGDB_E_IIDNOTREG == hr) &&
            SUCCEEDED(CoInitialize(NULL)))
        {
            bCoInit = TRUE;
            hr = CoCreateInstance(CLSID_InternetShortcut, NULL,
                                  CLSCTX_INPROC_SERVER, IID_IShellLinkA,
                                  (void**)&pIShellLink);
        }

        if (SUCCEEDED(hr))
        {
            ASSERT(pIShellLink);

#ifdef UNICODE
            CHAR szUrlA[INTERNET_MAX_URL_LENGTH];

            SHTCharToAnsi(CDFIDL_GetURL(pcdfidl), szUrlA, ARRAYSIZE(szUrlA));
            hr = pIShellLink->SetPath(szUrlA);
#else
            hr = pIShellLink->SetPath(CDFIDL_GetURL(pcdfidl));
#endif

            if (SUCCEEDED(hr))
            {
                //
                // The description ends up being the file name created.
                //

                TCHAR szPath[MAX_PATH];
#ifdef UNICODE
                CHAR  szPathA[MAX_PATH];
#endif

                StrCpyN(szPath, CDFIDL_GetName(pcdfidl), ARRAYSIZE(szPath) - 5);
                StrCat(szPath, TEXT(".url"));
#ifdef UNICODE
                SHTCharToAnsi(szPath, szPathA, ARRAYSIZE(szPathA));
                pIShellLink->SetDescription(szPathA);
#else
                pIShellLink->SetDescription(szPath);
#endif

                hr = pIShellLink->QueryInterface(riid, ppvOut);
            }

            pIShellLink->Release();
        }

        if (bCoInit)
            CoUninitialize();

    }
    else
    {
        hr = E_FAIL;
    }

    ASSERT((SUCCEEDED(hr) && *ppvOut) || (FAILED(hr) && NULL == *ppvOut));

    return hr;
}
