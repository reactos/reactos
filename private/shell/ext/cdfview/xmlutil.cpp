//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// xmlutil.cpp 
//
//   XML item helper functions.
//
//   History:
//
//       4/1/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "winineti.h"
#include <ocidl.h>         // IPersistStreamInit.

//
// Function prototypes.
//


//
// XML helper functions.
//

////////////////////////////////////////////////////////////////////////////////
//
// *** XML_MarkCacheEntrySticky ***
//
// Description:
//     Marks the cache entry for the given URL as sticky by setting its
//     expiration delta to be very high
//
// Parameters:
//     [In]  lpszUrl       - url for cache entry to make sticky
//
// Return:
//     S_OK if the url entry was successfully marked sticky
//     E_FAIL otherwise.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT XML_MarkCacheEntrySticky(LPTSTR lpszURL)
{
    char chBuf[MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFO lpInfo = (LPINTERNET_CACHE_ENTRY_INFO) chBuf;

    DWORD dwSize = sizeof(chBuf);
    lpInfo->dwStructSize = dwSize;

    if (GetUrlCacheEntryInfo(lpszURL, lpInfo, &dwSize))
    {
        lpInfo->dwExemptDelta = 0xFFFFFFFF; // make VERY sticky
        if (SetUrlCacheEntryInfo(lpszURL, lpInfo, CACHE_ENTRY_EXEMPT_DELTA_FC))
        {
            return S_OK;
        }
    }
    return E_FAIL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_SynchronousParse ***
//
//
// Description:
//     Synchronously parses the given URL.
//
// Parameters:
//     [In]  pIXMLDocument - An interface pointer to an XML document object.
//     [In]  pidl          - The pidl of the cdf file (contains the full path).
//
// Return:
//     S_OK if the object was parsed successfully.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_SynchronousParse(
    IXMLDocument* pIXMLDocument,
    LPTSTR szPath
)
{
    ASSERT(pIXMLDocument);
    ASSERT(szPath);

    HRESULT hr;

    IPersistStreamInit* pIPersistStreamInit;

    hr = pIXMLDocument->QueryInterface(IID_IPersistStreamInit,
                                       (void**)&pIPersistStreamInit);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIPersistStreamInit);

        IStream* pIStream;

        //
        // URLOpenBlockingStream pumps window messages!  Don't use it!
        //

        //hr = URLOpenBlockingStream(NULL, szPath, &pIStream, 0, NULL);

        hr = SHCreateStreamOnFile(szPath, STGM_READ, &pIStream);

        TraceMsg(TF_CDFPARSE, "[%s SHCreateStreamOnFileW %s %s %s]",
                 PathIsURL(szPath) ? TEXT("*** ") : TEXT(""), szPath,
                 SUCCEEDED(hr) ? TEXT("SUCCEEDED") : TEXT("FAILED"),
                 PathIsURL(szPath) ? TEXT("***") : TEXT(""));

        if (SUCCEEDED(hr))
        {
            ASSERT(pIStream);

            //
            // Load loads and parses the file.  If this call succeeds the cdf
            // will be displayed.  If it fails none of the cdf is displayed.
            //

            hr = pIPersistStreamInit->Load(pIStream);

            TraceMsg(TF_CDFPARSE, "[XML Parser %s]", 
                     SUCCEEDED(hr) ? TEXT("SUCCEEDED") : TEXT("FAILED"));

            pIStream->Release();

            //
            // If CDFVIEW is downloading a CDF from the net mark it as sticky
            // in the cache
            //
            if (PathIsURL(szPath))
            {
                XML_MarkCacheEntrySticky(szPath);
            }
        }

        pIPersistStreamInit->Release();
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_DownloadImages ***
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
XML_DownloadLogo(
    IXMLDocument *pIXMLDocument
)
{
    ASSERT(pIXMLDocument);

    HRESULT hr;

    IXMLElement* pIXMLElement;
    LONG         nIndex;

    hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLElement);
        
        BSTR bstrURL = XML_GetAttribute(pIXMLElement, XML_LOGO);

        if (bstrURL)
        {
            hr = XML_DownloadImage(bstrURL);

            SysFreeString(bstrURL);
        }

        //
        // Download the wide logo also.
        //

        bstrURL = XML_GetAttribute(pIXMLElement, XML_LOGO_WIDE);

        if (bstrURL)
        {
            hr = XML_DownloadImage(bstrURL);

            SysFreeString(bstrURL);
        }


        pIXMLElement->Release();
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_DownloadImages ***
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
XML_DownloadImages(
    IXMLDocument *pIXMLDocument
)
{
    ASSERT(pIXMLDocument);

    HRESULT hr;

    IXMLElement* pIXMLElement;
    LONG         nIndex;

    hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement, &nIndex);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLElement);
        
        hr = XML_RecursiveImageDownload(pIXMLElement);

        pIXMLElement->Release();
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_RecuriveImageDownload ***
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
XML_RecursiveImageDownload(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    HRESULT hr = S_OK;

    BSTR bstrTagName;

    HRESULT hr2 = pIXMLElement->get_tagName(&bstrTagName);

    if (SUCCEEDED(hr2) && bstrTagName)
    {
        if (StrEqlW(bstrTagName, WSTR_LOGO))
        {
            BSTR bstrURL = XML_GetAttribute(pIXMLElement, XML_HREF);

            if (bstrURL && *bstrURL != 0)
            {
                hr = XML_DownloadImage(bstrURL);

                SysFreeString(bstrURL);
            }
        }
        else if (XML_IsCdfDisplayable(pIXMLElement))
        {
            IXMLElementCollection* pIXMLElementCollection;

            hr2 = pIXMLElement->get_children(&pIXMLElementCollection);

            if (SUCCEEDED(hr2) && pIXMLElementCollection)
            {
                ASSERT(pIXMLElementCollection);

                LONG nCount;

                hr2 = pIXMLElementCollection->get_length(&nCount);

                ASSERT(SUCCEEDED(hr2) || (FAILED(hr2) && 0 == nCount));

                for (int i = 0; i < nCount; i++)
                {
                    IXMLElement* pIXMLElementChild;

                    hr2 = XML_GetElementByIndex(pIXMLElementCollection, i,
                                                &pIXMLElementChild);

                    if (SUCCEEDED(hr2))
                    {
                        ASSERT (pIXMLElementChild);

                        XML_RecursiveImageDownload(pIXMLElementChild);

                        pIXMLElementChild->Release();
                    }
                }

                pIXMLElementCollection->Release();
            }
        }

        SysFreeString(bstrTagName);
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_DownloadImage ***
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
XML_DownloadImage(
    LPCWSTR pwszURL
)
{
    ASSERT (pwszURL);

    HRESULT hr;

    WCHAR szFileW[MAX_PATH];

    hr = URLDownloadToCacheFileW(NULL, pwszURL, szFileW,
                                 ARRAYSIZE(szFileW), 0, NULL);

    //
    // Mark the logo in the cache as sticky
    //

    if (SUCCEEDED(hr))
    {
        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        SHUnicodeToTChar(pwszURL, szURL, ARRAYSIZE(szURL));

        XML_MarkCacheEntrySticky(szURL);
    }

    #ifdef DEBUG

        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        SHUnicodeToTChar(pwszURL, szURL, ARRAYSIZE(szURL));

        TraceMsg(TF_CDFPARSE,
                 "[*** Image URLDownloadToCacheFileW %s %s ***]",
                  szURL, SUCCEEDED(hr) ? TEXT("SUCCEEDED") :
                                         TEXT("FAILED"));

    #endif // DEBUG

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetDocType ***
//
//
// Description:
//     Returns the type of the given xml document.
//
// Parameters:
//     [In]  pIXMLDocument - A pointer to the xml document.
//
// Return:
//     DOC_CHANNEL, DOC_DESKTOPCOMPONENT, DOC_SOFTWAREUPDATE, or DOC_UNKNOWN.
//
// Comments:
//     If at the root level of a channel an ITEM contains a USAGE type of
//     DesktopComponent then the document is a Desktop Component otherwise
//     it is a channel.
//
////////////////////////////////////////////////////////////////////////////////
XMLDOCTYPE
XML_GetDocType(IXMLDocument* pIXMLDocument)
{
    ASSERT(pIXMLDocument);

    XMLDOCTYPE xdtRet;

    IXMLElement* pIXMLElement;
    LONG         nIndex;

    HRESULT hr = XML_GetFirstDesktopComponentElement(pIXMLDocument,
                                                     &pIXMLElement,
                                                     &nIndex);
    
    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLElement);

        xdtRet = DOC_DESKTOPCOMPONENT;

        pIXMLElement->Release();
    }
    else
    {
        hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLElement,
                                        &nIndex);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            BSTR bstr = XML_GetAttribute( pIXMLElement, XML_USAGE_SOFTWAREUPDATE );

            if (bstr)
            {
                SysFreeString(bstr);
                xdtRet = DOC_SOFTWAREUPDATE;
            }
            else
            {
                xdtRet = DOC_CHANNEL;
            }


            pIXMLElement->Release();
        }
        else
        {
            xdtRet = DOC_UNKNOWN;
        }
    }

    return xdtRet;
}



//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetChildElementCollection ***
//
//
// Description:
//     Returns an element collection given the parent collection and an index.
//
// Parameters:
//     [In]  pParentIXMLElementCollection - The parent collection.
//     [In]  nIndex                       - Index to the requested collection.
//     [Out] ppIXMLElementCollection      - A pointer that receives the
//                                          requested collection.
//
// Return:
//     S_OK if the collection is returned.
//     E_FAIL otherwise.
//
// Comments:
//     The caller is responsible for calling Release() on the returned interface
//     pointer.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetChildElementCollection(
    IXMLElementCollection *pParentIXMLElementCollection,
    LONG nIndex,
    IXMLElementCollection** ppIXMLElementCollection
)
{
    ASSERT(pParentIXMLElementCollection);
    ASSERT(ppIXMLElementCollection);

    HRESULT hr;

    IXMLElement* pIXMLElement;

    hr = XML_GetElementByIndex(pParentIXMLElementCollection, nIndex,
                               &pIXMLElement);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLElement);

        hr = pIXMLElement->get_children(ppIXMLElementCollection);
        if(SUCCEEDED(hr) && !(*ppIXMLElementCollection))
            hr = E_FAIL;

        pIXMLElement->Release();
    }

    ASSERT((SUCCEEDED(hr) && (*ppIXMLElementCollection)) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetElementByIndex ***
//
//
// Description:
//     Returns the nIndex'th element of the given collection.
//
// Parameters:
//     [In]  pIXMLElementCollection - A pointer to the collection.
//     [In]  nIndex                 - The index of the item to retrieve.
//     [Out] ppIXMLElement          - A pointer that receives the item.
//
// Return:
//     S_OK if the item was retrieved.
//     E_FAIL otherwise.
//
// Comments:
//     The caller is responsible for calling Release() on the returned interface
//     pointer.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetElementByIndex(
    IXMLElementCollection* pIXMLElementCollection,
    LONG nIndex,
    IXMLElement** ppIXMLElement
)
{
    ASSERT(pIXMLElementCollection);
    ASSERT(ppIXMLElement);

    HRESULT hr;

    VARIANT var1, var2;

    VariantInit(&var1);
    VariantInit(&var2);

    var1.vt   = VT_I4;
    var1.lVal = nIndex;

    IDispatch* pIDispatch;

    hr = pIXMLElementCollection->item(var1, var2, &pIDispatch);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIDispatch);

        hr = pIDispatch->QueryInterface(IID_IXMLElement, (void**)ppIXMLElement);

        pIDispatch->Release();
    }
    else
    {
        *ppIXMLElement = NULL;
    }

    ASSERT((SUCCEEDED(hr) && *ppIXMLElement) || 
           (FAILED(hr) && NULL == *ppIXMLElement));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetElementByName ***
//
//
// Description:
//     Returns the first element with the given tag name.
//
// Parameters:
//     [In]  pIXMLElementCollection - A pointer to the collection.
//     [In]  nszNameW               - The tag name of the item to retrieve.
//     [Out] ppIXMLElement          - A pointer that receives the item.
//
// Return:
//     S_OK if the item was retrieved.
//     E_OUTOFMEMORY if a sys string could not be allocated.
//     E_FAIL otherwise.
//
// Comments:
//     The caller is responsible for calling Release() on the returned interface
//     pointer.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetElementByName(
    IXMLElementCollection* pIXMLElementCollection,
    LPWSTR szNameW,
    IXMLElement** ppIXMLElement
)
{
    ASSERT(pIXMLElementCollection);
    ASSERT(ppIXMLElement);

    HRESULT hr = E_FAIL;

    LONG nCount;

    HRESULT hr2 = pIXMLElementCollection->get_length(&nCount);

    ASSERT(SUCCEEDED(hr2) || (FAILED(hr2) && 0 == nCount));

    for (int i = 0, bElement = FALSE; (i < nCount) && !bElement; i++)
    {
        IXMLElement* pIXMLElement;

        hr2 = XML_GetElementByIndex(pIXMLElementCollection, i, &pIXMLElement);

        if (SUCCEEDED(hr2))
        {
            ASSERT(pIXMLElement);

            BSTR pStr;

            hr2 = pIXMLElement->get_tagName(&pStr);

            if (SUCCEEDED(hr2) && pStr)
            {
                ASSERT(pStr);

                if (bElement = StrEqlW(pStr, szNameW))
                {
                    pIXMLElement->AddRef();
                    *ppIXMLElement = pIXMLElement;
                    
                    hr = S_OK;
                }

                SysFreeString(pStr);
            }

            pIXMLElement->Release();
        }
    }

    hr = FAILED(hr2) ? hr2 : hr;

    /* Enable this when pIXMLElementCollection->item works with VT_BSTR

    VARIANT var1, var2;

    VariantInit(&var1);
    VariantInit(&var2);

    var1.vt      = VT_BSTR;
    var1.bstrVal = SysAllocString(szNameW);

    var2.vt      = VT_I4
    var2.lVal    = 1;

    if (var1.bstrVal)
    {

        IDispatch* pIDispatch;

        hr = pIXMLElementCollection->item(var1, var2, &pIDispatch);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIDispatch);

            hr = pIDispatch->QueryInterface(IID_IXMLElement,
                                            (void**)ppIXMLElement);

            pIDispatch->Release();
        }

        SysFreeString(var1.bstrVal);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    */

    ASSERT((SUCCEEDED(hr) && ppIXMLElement) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetFirstChannelElement ***
//
//
// Description:
//     Returns the IXMLElement of the first channel in the XML document.
//
// Parameters:
//     [In]  pIXMLDocument - A pointer to the XML document object.
//     [Out] ppIXMLElement - The pointer that receives the element.
//     [Out] pnIndex       - The index of the element.
//
// Return:
//     S_OK if the first channel element was returned.
//     E_FAIL if the element couldn't be returned.
//
// Comments:
//     This function can't call XML_GetElementByName to find the first channel.
//     XML channels can have a tag name of "Channel" or "CHAN".
//     XML_GetElementByName wouldn't be able to determine which of the items
//     came first if both where present in the XML doc.
//
//     The caller is responsible for calling Release() on the returned interface
//     pointer.  The return pointer is not NULL'ed out on error.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetFirstChannelElement(
    IXMLDocument* pIXMLDocument,
    IXMLElement** ppIXMLElement,
    PLONG pnIndex)
{
    ASSERT(pIXMLDocument);
    ASSERT(ppIXMLElement);
    ASSERT(pnIndex);
    IXMLElement *pRootElem = NULL;
    HRESULT hr = E_FAIL;

    *pnIndex = 0;
    hr = pIXMLDocument->get_root(&pRootElem);

    if (SUCCEEDED(hr) && pRootElem)
    {
        ASSERT(pRootElem);

        if (XML_IsChannel(pRootElem))
        {
            *ppIXMLElement = pRootElem;
            hr = S_OK;
        }
        else
        {
            pRootElem->Release();
            hr = E_FAIL;
        }
            
    }
    else
    {
        hr = E_FAIL;
    }

    ASSERT((SUCCEEDED(hr) && (*ppIXMLElement)) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetDesktopElementFromChannelElement ***
//
//
// Description:
//     Returns the IXMLElement of the first dekstop component in the channel.
//
// Parameters:
//     [In]  pChannelIXMLElement - A pointer to the XML channel element.
//     [Out] ppIXMLElement       - The pointer that receives the element.
//     [Out] pnIndex             - The index of the element.
//
// Return:
//     S_OK if the first desktop element was returned.
//     E_FAIL if the element couldn't be returned.
//
// Comments:
//     This looks for the first ITEM with a usage of DesktopComponent.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT
XML_GetDesktopElementFromChannelElement(
    IXMLElement* pChannelIXMLElement,
    IXMLElement** ppIXMLElement,
    PLONG pnIndex)
{
    ASSERT(pChannelIXMLElement);
    ASSERT(ppIXMLElement);
    ASSERT(pnIndex);

    HRESULT hr;

    IXMLElementCollection* pIXMLElementCollection;

    hr = pChannelIXMLElement->get_children(&pIXMLElementCollection);

    if (SUCCEEDED(hr) && pIXMLElementCollection)
    {
        ASSERT(pIXMLElementCollection);

        LONG nCount;

        hr = pIXMLElementCollection->get_length(&nCount);

        ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

        hr = E_FAIL;

        for (int i = 0, bComponent = FALSE; (i < nCount) && !bComponent;
             i++)
        {
            IXMLElement* pIXMLElement;

            HRESULT hr2 = XML_GetElementByIndex(pIXMLElementCollection, i,
                                                &pIXMLElement);

            if (SUCCEEDED(hr2))
            {
                ASSERT(pIXMLElement);

                if (bComponent = XML_IsDesktopComponent(pIXMLElement))
                {
                    pIXMLElement->AddRef();
                    *ppIXMLElement = pIXMLElement;
                    *pnIndex = i;

                    hr = S_OK;
                }

                pIXMLElement->Release();
            }

            hr = FAILED(hr2) ? hr2 : hr;
        }

        pIXMLElementCollection->Release();
    }        
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetFirstDesktopComponentElement ***
//
//
// Description:
//     Returns the IXMLElement of the first dekstop component in the channel.
//
// Parameters:
//     [In]  pIXMLDocument - A pointer to the XML document object.
//     [Out] ppIXMLElement - The pointer that receives the element.
//     [Out] pnIndex       - The index of the element.
//
// Return:
//     S_OK if the first channel element was returned.
//     E_FAIL if the element couldn't be returned.
//
// Comments:
//     This function gets the first channel and then looks for the first
//     top-level ITEM with a usage of DesktopComponent.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetFirstDesktopComponentElement(
    IXMLDocument* pIXMLDocument,
    IXMLElement** ppIXMLElement,
    PLONG pnIndex)
{
    ASSERT(pIXMLDocument);
    ASSERT(ppIXMLElement);
    ASSERT(pnIndex);

    HRESULT hr;

    IXMLElement* pChannelIXMLElement;
    LONG         nIndex;

    hr = XML_GetFirstChannelElement(pIXMLDocument, &pChannelIXMLElement,
                                    &nIndex);

    if (SUCCEEDED(hr))
    {
        ASSERT(pChannelIXMLElement);

        hr = XML_GetDesktopElementFromChannelElement(pChannelIXMLElement, 
                                                     ppIXMLElement, 
                                                     pnIndex);

        pChannelIXMLElement->Release();
    }

    ASSERT((SUCCEEDED(hr) && *ppIXMLElement) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetFirstDesktopComponentUsageElement ***
//
//
// Description:
//     Returns the first USAGE VALUE="DesktopComponent" element of the first
//     desktop component.
//
// Parameters:
//     [In]  pIXMLDocument - A pointer to the the document.
//     [Out] pIXMLElement  - A pointer the receives the element.
//
// Return:
//     S_OK if the element was found.
//     E_FAIL if the element wasn't found.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetFirstDesktopComponentUsageElement(
    IXMLDocument* pIXMLDocument,
    IXMLElement** ppIXMLElement
)
{
    ASSERT(pIXMLDocument);
    ASSERT(ppIXMLElement);

    HRESULT hr;

    IXMLElement* pParentIXMLElement;
    LONG         nIndex;

    hr = XML_GetFirstDesktopComponentElement(pIXMLDocument, &pParentIXMLElement,
                                             &nIndex);

    if (SUCCEEDED(hr))
    {
        IXMLElementCollection* pIXMLElementCollection;

        hr = pParentIXMLElement->get_children(&pIXMLElementCollection);

        if (SUCCEEDED(hr) && pIXMLElementCollection)
        {
            ASSERT(pIXMLElementCollection);

            LONG nCount;

            HRESULT hr = pIXMLElementCollection->get_length(&nCount);

            ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

            hr = E_FAIL;

            for (int i = 0, bUsage = FALSE; (i < nCount) && !bUsage; i++)
            {
                IXMLElement* pIXMLElement;

                HRESULT hr2 = XML_GetElementByIndex(pIXMLElementCollection, i,
                                                    &pIXMLElement);

                if (SUCCEEDED(hr2))
                {
                    ASSERT(pIXMLElement);

                    if (bUsage = XML_IsDesktopComponentUsage(pIXMLElement))
                    {
                        pIXMLElement->AddRef();
                        *ppIXMLElement = pIXMLElement;
                        //*pnIndex = i;

                        hr = S_OK;
                    }

                    pIXMLElement->Release();
                }

                hr = FAILED(hr2) ? hr2 : hr;
            }

            pIXMLElementCollection->Release();
        } 
        else
        {
            hr = E_FAIL;
        }

        pParentIXMLElement->Release();
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetDesktopComponentInfo ***
//
//
// Description:
//     Fills in the desktop component information structure.
//
// Parameters:
//     [In]  pIXMLDocument - A ponter to the document.
//     [Out] pInfo         - A desktop component information structure.
//
// Return:
//     S_OK if the given document is desktop component document.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetDesktopComponentInfo(
    IXMLDocument* pIXMLDocument,
    COMPONENT* pInfo
)
{
    ASSERT(pIXMLDocument);
    ASSERT(pInfo);

    HRESULT hr;

    IXMLElement* pIXMLElement;

    hr = XML_GetFirstDesktopComponentUsageElement(pIXMLDocument, &pIXMLElement);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLElement);

        memset(pInfo, 0, sizeof(COMPONENT));

        pInfo->dwSize        = sizeof(COMPONENT);
        pInfo->fChecked      = TRUE;
        pInfo->fDirty        = TRUE;
        pInfo->fNoScroll     = FALSE;
        pInfo->cpPos.dwSize  = sizeof(COMPPOS);
        pInfo->cpPos.izIndex = COMPONENT_TOP;
        pInfo->dwCurItemState = IS_NORMAL;

        BSTR bstrValue;

        if (bstrValue = XML_GetAttribute(pIXMLElement, XML_OPENAS))
        {
            if (!(0 == StrCmpIW(bstrValue, WSTR_IMAGE)))
            {
                pInfo->iComponentType = COMP_TYPE_WEBSITE;
            }
            else
            {
                pInfo->iComponentType = COMP_TYPE_PICTURE;
            }

            SysFreeString(bstrValue);
        }

        if (bstrValue = XML_GetAttribute(pIXMLElement, XML_WIDTH))
        {
            pInfo->cpPos.dwWidth = StrToIntW(bstrValue);
            SysFreeString(bstrValue);
        }

        if (bstrValue = XML_GetAttribute(pIXMLElement, XML_HEIGHT))
        {
            pInfo->cpPos.dwHeight = StrToIntW(bstrValue);
            SysFreeString(bstrValue);
        }

        if (bstrValue = XML_GetAttribute(pIXMLElement, XML_ITEMSTATE))
        {
            if(!StrCmpIW(bstrValue, WSTR_NORMAL))
                pInfo->dwCurItemState = IS_NORMAL;
            else
            {
                if(!StrCmpIW(bstrValue, WSTR_FULLSCREEN))
                    pInfo->dwCurItemState = IS_FULLSCREEN;
                else
                    pInfo->dwCurItemState = IS_SPLIT;
            }
            SysFreeString(bstrValue);
        }
        
        if (bstrValue = XML_GetAttribute(pIXMLElement, XML_CANRESIZE))
        {
            pInfo->cpPos.fCanResize = StrEqlW(bstrValue, WSTR_YES);
            SysFreeString(bstrValue);
        }
        else
        {
            if (bstrValue = XML_GetAttribute(pIXMLElement, XML_CANRESIZEX))
            {
                pInfo->cpPos.fCanResizeX = StrEqlW(bstrValue, WSTR_YES);
                SysFreeString(bstrValue);
            }

            if (bstrValue = XML_GetAttribute(pIXMLElement, XML_CANRESIZEY))
            {
                pInfo->cpPos.fCanResizeY = StrEqlW(bstrValue, WSTR_YES);
                SysFreeString(bstrValue);
            }
        }

        if (bstrValue = XML_GetAttribute(pIXMLElement, XML_PREFERREDLEFT))
        {
            if (StrChrW(bstrValue, L'%'))
            {
                pInfo->cpPos.iPreferredLeftPercent = StrToIntW(bstrValue);
            }
            else
            {
                pInfo->cpPos.iLeft = StrToIntW(bstrValue);
            }

            SysFreeString(bstrValue);
        }

        if (bstrValue = XML_GetAttribute(pIXMLElement, XML_PREFERREDTOP))
        {
            if (StrChrW(bstrValue, L'%'))
            {
                pInfo->cpPos.iPreferredTopPercent = StrToIntW(bstrValue);
            }
            else
            {
                pInfo->cpPos.iTop = StrToIntW(bstrValue);
            }

            SysFreeString(bstrValue);        
        }

        IXMLElement *pIXMLElementParent;

        hr = pIXMLElement->get_parent(&pIXMLElementParent);
        if(!pIXMLElementParent)
            hr = E_FAIL;
        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElementParent);

            if (bstrValue = XML_GetAttribute(pIXMLElementParent, XML_TITLE))
            {
                StrCpyNW(pInfo->wszFriendlyName, bstrValue,
                         ARRAYSIZE(pInfo->wszFriendlyName));
                SysFreeString(bstrValue);
            }

            if (bstrValue = XML_GetAttribute(pIXMLElementParent, XML_HREF))
            {
                if (*bstrValue)
                {
                    StrCpyNW(pInfo->wszSource, bstrValue,
                             ARRAYSIZE(pInfo->wszSource));
                    SysFreeString(bstrValue);
                }
                else
                {
                    hr = E_FAIL;
                }
            }

            if (SUCCEEDED(hr))
            {
                IXMLElement *pIXMLChannel;
                LONG nIndex;

                hr = XML_GetFirstChannelElement(pIXMLDocument, &pIXMLChannel,
                                                &nIndex);
                if (SUCCEEDED(hr))
                {
                    ASSERT(pIXMLChannel);
                    if (bstrValue = XML_GetAttribute(pIXMLChannel, XML_SELF))
                    {
                        StrCpyNW(pInfo->wszSubscribedURL, bstrValue,
                                 ARRAYSIZE(pInfo->wszSubscribedURL));
                        SysFreeString(bstrValue);
                    }

                    pIXMLChannel->Release();
                }
            }

            pIXMLElementParent->Release();
        }

        pIXMLElement->Release();
    }

    return hr;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetAttribute ***
//
//
// Description:
//     Returns a bstr representing the requested attribute of the given element.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the XML element.
//     [In]  attribute    - The requested attribute.
//
// Return:
//     A bstr with the attribute value.
//     NULL if there wasn't enough memory to allocated the bstr.
//
// Comments:
//     This function keeps a table of attributes and their properties.  It bases
//     attribute look up on this table.
//
////////////////////////////////////////////////////////////////////////////////
BSTR
XML_GetAttribute(
    IXMLElement* pIXMLElement,
    XML_ATTRIBUTE attrIndex
)
{
    //
    // A table is used to read values associated with a given xml element.  The
    // xml element can have attributes (values inside the elements tag) or child
    // elements (elements between tags).
    //
    // Rules:
    //        1) If child is NULL. Read the Attribute from the current item.
    //        2) If child is not NULL, read the Attribute from the child
    //           item.
    //        3) If AttributeType is NULL, use the Attribute value to read the
    //           attribute.
    //        4) If AttributeType is not NULL, verify that the item contains
    //           the AttributeType attribute before using Attribute to read
    //           the value.
    //        5) If the value is not found use Default as the return value.
    //

    static const struct _tagXML_ATTRIBUTE_ARRAY
    {
        LPWSTR                  szChildW;
        LPWSTR                  szAttributeW;
        LPWSTR                  szQualifierW;
        LPWSTR                  szQualifierValueW;
        XML_ATTRIBUTE           attrSecondary;
        LPWSTR                  szDefaultW;
        BOOL                    fUseBaseURL;
        XML_ATTRIBUTE           attribute;  // Only used in ASSERT.
    }
    aAttribTable[] =
    {
/*
Child           Attribute      Qualifier   Qual. Value   Secondary Lookup   Default     Base URL Enum Check
--------------  -------------- ----------- ------------  -----------------  ----------  -------- -----------------*/
{WSTR_TITLE,    NULL,          NULL,       NULL,         XML_TITLE_ATTR,    WSTR_EMPTY, FALSE,   XML_TITLE        },
{NULL,          WSTR_TITLE,    NULL,       NULL,         XML_HREF,          WSTR_EMPTY, FALSE,   XML_TITLE_ATTR   },
{NULL,          WSTR_HREF,     NULL,       NULL,         XML_A_HREF,        WSTR_EMPTY, TRUE,    XML_HREF         },
{WSTR_ABSTRACT, NULL,          NULL,       NULL,         XML_ABSTRACT_ATTR, WSTR_EMPTY, FALSE,   XML_ABSTRACT     },
{NULL,          WSTR_ABSTRACT, NULL,       NULL,         XML_HREF,          WSTR_EMPTY, FALSE,   XML_ABSTRACT_ATTR},
{WSTR_LOGO,     WSTR_HREF,     WSTR_STYLE, WSTR_ICON,    XML_NULL,          NULL,       TRUE,    XML_ICON         },
{WSTR_LOGO,     WSTR_HREF,     WSTR_STYLE, WSTR_IMAGE,   XML_LOGO_DEFAULT,  NULL,       TRUE,    XML_LOGO         },
{WSTR_LOGO,     WSTR_HREF,     NULL,       NULL,         XML_NULL,          NULL,       TRUE,    XML_LOGO_DEFAULT },
{NULL,          WSTR_SELF,     NULL,       NULL,         XML_SELF_OLD,      NULL,       TRUE,    XML_SELF         },
{WSTR_SELF,     WSTR_HREF,     NULL,       NULL,         XML_NULL,          NULL,       TRUE,    XML_SELF_OLD     },
{NULL,          WSTR_BASE,     NULL,       NULL,         XML_NULL,          NULL,       FALSE,   XML_BASE         },
{WSTR_USAGE,    WSTR_VALUE,    NULL,       NULL,         XML_SHOW,          NULL,       FALSE,   XML_USAGE        },
{WSTR_USAGE,    WSTR_VALUE,    WSTR_VALUE, WSTR_CHANNEL, XML_SHOW_CHANNEL,  NULL,       FALSE,   XML_USAGE_CHANNEL},
{WSTR_USAGE,    WSTR_VALUE,    WSTR_VALUE, WSTR_DSKCMP,  XML_SHOW_DSKCMP,   NULL,       FALSE,   XML_USAGE_DSKCMP },
{WSTR_WIDTH,    WSTR_VALUE,    NULL,       NULL,         XML_NULL,          WSTR_ZERO,  FALSE,   XML_WIDTH        },
{WSTR_HEIGHT,   WSTR_VALUE,    NULL,       NULL,         XML_NULL,          WSTR_ZERO,  FALSE,   XML_HEIGHT       },
{WSTR_RESIZE,   WSTR_VALUE,    NULL,       NULL,         XML_NULL,          NULL,       FALSE,   XML_CANRESIZE    },
{WSTR_RESIZEX,  WSTR_VALUE,    NULL,       NULL,         XML_NULL,          WSTR_YES,   FALSE,   XML_CANRESIZEX   },
{WSTR_RESIZEY,  WSTR_VALUE,    NULL,       NULL,         XML_NULL,          WSTR_YES,   FALSE,   XML_CANRESIZEY   },
{WSTR_PREFLEFT, WSTR_VALUE,    NULL,       NULL,         XML_NULL,          NULL,       FALSE,   XML_PREFERREDLEFT},
{WSTR_PREFTOP,  WSTR_VALUE,    NULL,       NULL,         XML_NULL,          NULL,       FALSE,   XML_PREFERREDTOP },
{WSTR_OPENAS,   WSTR_VALUE,    NULL,       NULL,         XML_NULL,          WSTR_HTML,  FALSE,   XML_OPENAS       },
{NULL,          WSTR_SHOW,     NULL,       NULL,         XML_NULL,          NULL,       FALSE,   XML_SHOW         },
{NULL,          WSTR_SHOW,     WSTR_SHOW,  WSTR_CHANNEL, XML_NULL,          NULL,       FALSE,   XML_SHOW_CHANNEL },
{NULL,          WSTR_SHOW,     WSTR_SHOW,  WSTR_DSKCMP,  XML_NULL,          NULL,       FALSE,   XML_SHOW_DSKCMP  },
{WSTR_A,        WSTR_HREF,     NULL,       NULL,         XML_INFOURI,       WSTR_EMPTY, TRUE,    XML_A_HREF       },
{NULL,          WSTR_INFOURI,  NULL,       NULL,         XML_NULL,          WSTR_EMPTY, TRUE,    XML_INFOURI      },
{WSTR_LOGO,     WSTR_HREF,     WSTR_STYLE, WSTR_IMAGEW,  XML_NULL,          NULL,       TRUE,    XML_LOGO_WIDE    },
{WSTR_LOGIN,    NULL,          NULL,       NULL,         XML_NULL,          NULL,       FALSE,   XML_LOGIN        },

{WSTR_USAGE,    WSTR_VALUE,    WSTR_VALUE, WSTR_SOFTWAREUPDATE, XML_SHOW_SOFTWAREUPDATE,  NULL,       FALSE,   XML_USAGE_SOFTWAREUPDATE},
{NULL,          WSTR_SHOW,     WSTR_SHOW,  WSTR_SOFTWAREUPDATE, XML_NULL,                 NULL,       FALSE,   XML_SHOW_SOFTWAREUPDATE },
{WSTR_ITEMSTATE,WSTR_VALUE,    NULL,       NULL,         XML_NULL,          WSTR_NORMAL,FALSE,   XML_ITEMSTATE   },
    };

    ASSERT(pIXMLElement);

    //
    // REVIEW: aAttribTable attribute field only used in debug builds.
    //

    ASSERT(attrIndex == aAttribTable[attrIndex].attribute);

    BSTR bstrRet = NULL;

    if (NULL == aAttribTable[attrIndex].szAttributeW)
    {
        bstrRet = XML_GetGrandChildContent(pIXMLElement,
                                      aAttribTable[attrIndex].szChildW);
    
    }
    else if (NULL != aAttribTable[attrIndex].szChildW)
    {
        bstrRet = XML_GetChildAttribute(pIXMLElement,
                                     aAttribTable[attrIndex].szChildW,
                                     aAttribTable[attrIndex].szAttributeW, 
                                     aAttribTable[attrIndex].szQualifierW,
                                     aAttribTable[attrIndex].szQualifierValueW);
    }
    else
    {
        bstrRet = XML_GetElementAttribute(pIXMLElement,
                                     aAttribTable[attrIndex].szAttributeW,
                                     aAttribTable[attrIndex].szQualifierW,
                                     aAttribTable[attrIndex].szQualifierValueW);
    }

    //
    // If the title or tooltip aren't displayable on the local system use the
    // URL in their place.
    //

    if (bstrRet && (XML_TITLE == attrIndex || XML_TITLE_ATTR == attrIndex ||
                    XML_ABSTRACT == attrIndex))
    {
        if (!StrLocallyDisplayable(bstrRet))
        {
            SysFreeString(bstrRet);
            bstrRet = NULL;
        }
    }
    
    //
    // Special cases:
    //     TITLE can also be an attribute.
    //     ABSTRACT can also be an attribute.
    //     LOGO elements don't have to have the TYPE="IMAGE"
    //     SELF is now an attribute SELF_OLD can be removed in the future.
    //     USAGE can also be specified via the SHOW attribute.
    //     USAGE_CHANNEL should also check for SHOW="Channel".
    //     USAGE_DSKCMP should also check for SHOW="DesktopComponent"
    //

    if (NULL == bstrRet && XML_NULL != aAttribTable[attrIndex].attrSecondary)
    {
        bstrRet = XML_GetAttribute(pIXMLElement,
                                   aAttribTable[attrIndex].attrSecondary);
    }

    //
    // Combine URL if required.
    //

    if (bstrRet && aAttribTable[attrIndex].fUseBaseURL)
    {
        BSTR bstrBaseURL = XML_GetBaseURL(pIXMLElement);

        if (bstrBaseURL)
        {
            
            BSTR bstrCombinedURL = XML_CombineURL(bstrBaseURL, bstrRet);

            if (bstrCombinedURL)
            {
                SysFreeString(bstrRet);
                bstrRet = bstrCombinedURL;
            }

            SysFreeString(bstrBaseURL);
        }
    }

    /* The following prevent long urls from over-running the pidl buffer */

    if (bstrRet &&
        (attrIndex == XML_HREF) &&
        (SysStringLen(bstrRet) > INTERNET_MAX_URL_LENGTH))
    {
       SysReAllocStringLen(&bstrRet, bstrRet, INTERNET_MAX_URL_LENGTH-1);
    }

    //
    // Set default return value.
    //

    if (NULL == bstrRet && aAttribTable[attrIndex].szDefaultW)
        bstrRet = SysAllocString(aAttribTable[attrIndex].szDefaultW);

    
    return bstrRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetFirstChildContent ***
//
// Description:
//     Returns a bstr value from first child of the given element.
//
// Parameters:
//     [In]  pIXMLElement     - A pointer to the element.
//     The caller is responsible for freeing the returned bstr.
//
// Comments:
//     If pIElement represents
//
//         <Title>Harvey is a Cool Cat<B>this will be ignored</B></Title>
//
//         Then this function will return
//
//             "Harvey is a Cool Cat"
//
// REVIEW THIS IS A TEMPORARY ROUTINE UNTIL THE XML PARSER SUPPORTS THIS DIRECTLY
//
////////////////////////////////////////////////////////////////////////////////
BSTR
XML_GetFirstChildContent(
    IXMLElement* pIXMLElement
)
{    
    ASSERT(pIXMLElement);

    BSTR bstrRet = NULL;

    IXMLElementCollection* pIXMLElementCollection;

    if ((SUCCEEDED(pIXMLElement->get_children(&pIXMLElementCollection)))
            && pIXMLElementCollection)
    {
        ASSERT(pIXMLElementCollection);

        LONG nCount;

        HRESULT hr = pIXMLElementCollection->get_length(&nCount);

        ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

        if (nCount >= 1)
        {
            IXMLElement* pChildIXMLElement;

            if (SUCCEEDED(XML_GetElementByIndex(pIXMLElementCollection, 0,
                                                &pChildIXMLElement)))
            {
                ASSERT(pChildIXMLElement);

                if (FAILED(pChildIXMLElement->get_text(&bstrRet)))
                {
                    bstrRet = NULL;
                }

                pChildIXMLElement->Release();
            }
        }

        pIXMLElementCollection->Release();
    }
    
    return bstrRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetGrandChildContent ***
//
// Description:
//     Returns a bstr value from the requested child of the given element.
//
// Parameters:
//     [In]  pIXMLElement     - A pointer to the element.
//     [In]  szChildW         - The name of the child element.
//     The caller is responsible for freeing the returned bstr.
//
// Comments:
//     If pIElement represents
//
//             <Channel>
//                 <Title>Harvey is a Cool Cat</Title>
//             </Channel>
//
//         Then this function will return
//
//             "Harvey is a Cool Cat" for "TITLE",  
//
////////////////////////////////////////////////////////////////////////////////

BSTR
XML_GetGrandChildContent(
    IXMLElement* pIXMLElement,
    LPWSTR szChildW
)
{    
    ASSERT(pIXMLElement);
    ASSERT(szChildW);    

    BSTR bstrRet = NULL;

    IXMLElementCollection* pIXMLElementCollection;

    if ((SUCCEEDED(pIXMLElement->get_children(&pIXMLElementCollection)))
         && pIXMLElementCollection)
    {
        ASSERT(pIXMLElementCollection);

        LONG nCount;

        HRESULT hr = pIXMLElementCollection->get_length(&nCount);

        ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

        for (int i = 0; (i < nCount) && !bstrRet; i++)
        {
            IXMLElement* pChildIXMLElement;

            if (SUCCEEDED(XML_GetElementByIndex(pIXMLElementCollection, i,
                                                &pChildIXMLElement)))
            {
                ASSERT(pChildIXMLElement);

                BSTR bstrTagName;

                if (SUCCEEDED(pChildIXMLElement->get_tagName(&bstrTagName)) && bstrTagName)
                {
                    ASSERT(bstrTagName);

                    if (StrEqlW(bstrTagName, szChildW))
                    {
                        bstrRet = XML_GetFirstChildContent(pChildIXMLElement);

                        //
                        // If the tag exists, but it is empty, return the empty
                        // string.
                        //

                        if (NULL == bstrRet)
                            bstrRet = SysAllocString(L"");
                    }

                    SysFreeString(bstrTagName);
                }


                pChildIXMLElement->Release();
            }
        }

        pIXMLElementCollection->Release();
    }
    
    return bstrRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetChildAttribute ***
//
//
// Description:
//     Returns a bstr value from the requested child of the given element.
//
// Parameters:
//     [In]  pIXMLElement      - A pointer to the element.
//     [In]  szChildW          - The name of the child element.
//     [In]  szAttributeW      - The name of the attribute.
//     [In]  szQualifierW      - The name of the attribute qualifier.
//     [In]  szQualifierValueW - The required value of the qaulifier.
//
// Return:
//     A bstr of the value contained in the child element if it is found.
//     NULL if the child element or its value isn't found.
//
// Comments:
//     This function will return atributes found in the child elements of the
//     given element.  For example:
//
//         If pIElement represents
//
//             <Channel>
//                 <Title VALUE="foo">
//                 <Author VALUE="bar">
//                 <Logo HRREF="url" TYPE="ICON">
//             </Channel>
//
//         Then this function will return
//
//             "foo" for "TITLE",  "VALUE", "", ""
//             "bar" for "AUTHOR", "VALUE", "", ""
//             "url" for "LOGO",   "HREF",  "TYPE", "ICON" 
//
//             NULL  when the names have any other values.
//
//     The caller is responsible for freeing the returned bstr.
//
////////////////////////////////////////////////////////////////////////////////
BSTR
XML_GetChildAttribute(
    IXMLElement* pIXMLElement,
    LPWSTR szChildW,
    LPWSTR szAttributeW,
    LPWSTR szQualifierW,
    LPWSTR szQualifierValueW
)
{
    ASSERT(pIXMLElement);
    ASSERT(szChildW);
    ASSERT(szAttributeW);

    BSTR bstrRet = NULL;

    IXMLElementCollection* pIXMLElementCollection;

    if ((SUCCEEDED(pIXMLElement->get_children(&pIXMLElementCollection)))
        && pIXMLElementCollection)
    {
        ASSERT(pIXMLElementCollection);

        LONG nCount;

        //
        // REVIEW:  hr only used in debug builds.
        //

        HRESULT hr = pIXMLElementCollection->get_length(&nCount);

        ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

        for (int i = 0; (i < nCount) && !bstrRet; i++)
        {
            IXMLElement* pChildIXMLElement;

            if (SUCCEEDED(XML_GetElementByIndex(pIXMLElementCollection, i,
                                                &pChildIXMLElement)))
            {
                ASSERT(pChildIXMLElement);

                BSTR bstrTagName;

                if (SUCCEEDED(pChildIXMLElement->get_tagName(&bstrTagName)) && bstrTagName)
                {
                    ASSERT(bstrTagName);

                    if (StrEqlW(bstrTagName, szChildW))
                    {
                        bstrRet = XML_GetElementAttribute(pChildIXMLElement,
                                                          szAttributeW,
                                                          szQualifierW,
                                                          szQualifierValueW);
                    }

                    SysFreeString(bstrTagName);
                }


                pChildIXMLElement->Release();
            }
        }

        pIXMLElementCollection->Release();
    }
    
    return bstrRet;
}
    
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetElementAttribute ***
//
//
// Description:
//     Returns the bstr value of of the requested attribute if it is found.
//
// Parameters:
//     [In]  pIXMLElement      - The element that contains the attribute.
//     [In]  szAttributeW      - The name of the attribute.
//     [In]  szQualifierW      - The type qualifier for the atribute.
//     [In]  szQualifierValueW - The required value of the qaulifier.
//
// Return:
//     A bstr containig the attributes value if it was found.
//     NULL if the attribute wasn't found.
//
// Comments:
//     The function will return attributes found inside of tags.  For
//     example:
//
//         If pIXMLElement represents
//
//             <Channel HREF="foo" Cloneable="NO">
//             <USAGE VALUE="Channel">
//             <USAGE VALUE="Screen Saver">
//
//         Then this function will return
//
//             "foo"     for "HREF",      "",          ""
//             "NO"      for "Cloneable", "",          ""
//             "CHANNEL" for "VALUE",     "VALUE",     "CHANNEL"
//             NULL      for "VALUE",     "VALUE",     "NONE"
//             "foo"     for "HREF",      "CLONEABLE", "NO"
//
//     The caller is responsible for freeing the returned bstr.
//
////////////////////////////////////////////////////////////////////////////////
BSTR
XML_GetElementAttribute(
    IXMLElement* pIXMLElement,
    LPWSTR szAttributeW,
    LPWSTR szQualifierW,
    LPWSTR szQualifierValueW
)
{
    ASSERT(pIXMLElement);
    ASSERT(szAttributeW);
    ASSERT((NULL == szQualifierW && NULL == szQualifierValueW) ||
           (szQualifierW && szQualifierValueW));

    BSTR bstrRet = NULL;

    VARIANT var;

    VariantInit(&var);

    if (NULL == szQualifierW)
    {
        if (SUCCEEDED(pIXMLElement->getAttribute(szAttributeW, &var)))
        {
            ASSERT(var.vt == VT_BSTR || NULL == var.bstrVal);

            bstrRet = var.bstrVal;
        }
    }
    else
    {
        if (SUCCEEDED(pIXMLElement->getAttribute(szQualifierW, &var)))
        {
            ASSERT(var.vt == VT_BSTR || NULL == var.bstrVal);

            if(var.bstrVal)
            {
                if (0 == StrCmpIW(var.bstrVal, szQualifierValueW))
                {
                    bstrRet = XML_GetElementAttribute(pIXMLElement, szAttributeW,
                                                      NULL, NULL);
                }
            }
            VariantClear(&var);
        }
    }

    return bstrRet;
}
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetScreenSaverURL ***
//
//
// Description:
//     Returns the screen saver URL of the first screen saver component in the channel.
//
// Parameters:
//     [In]  pXMLDocument  - An XML document
//     [Out] pbstrSSURL    - The pointer that receives the screen saver URL.
//
// Return:
//     S_OK if the screen saver URL was returned.
//     E_FAIL if the screen saver URL couldn't be returned.
//
// Comments:
//     This function gets the first screen saver element and then looks
//     for the first top-level ITEM with a usage of ScreenSaver.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetScreenSaverURL(
    IXMLDocument *  pXMLDocument,
    BSTR *          pbstrSSURL)
{
    HRESULT hr;

    ASSERT(pXMLDocument);
    ASSERT(pbstrSSURL);
    
    IXMLElement* pIXMLElement;
    LONG lDontCare;

    hr = XML_GetFirstChannelElement(pXMLDocument, &pIXMLElement, &lDontCare);
    if (SUCCEEDED(hr))
    {
        IXMLElement* pSSElement;

        ASSERT(pIXMLElement);

        hr = XML_GetScreenSaverElement(pIXMLElement, &pSSElement);

        if (SUCCEEDED(hr))
        {
            ASSERT(pSSElement);

            *pbstrSSURL = XML_GetAttribute(pSSElement, XML_HREF);

            hr = *pbstrSSURL ? S_OK : E_FAIL;
            
            pSSElement->Release();
        }
        pIXMLElement->Release();
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetScreenSaverElement ***
//
//
// Description:
//     Returns the IXMLElement of the first screen saver component in the channel.
//
// Parameters:
//     [In]  pXMLElemet    - An XML element
//     [Out] ppIXMLElement - The pointer that receives the screen saver element.
//
// Return:
//     S_OK if the first screen saver element was returned.
//     E_FAIL if the element couldn't be returned.
//
// Comments:
//     This function gets the first screen saver element and then looks
//     for the first top-level ITEM with a usage of ScreenSaver.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetScreenSaverElement(
    IXMLElement *   pXMLElement,
    IXMLElement **  ppScreenSaverElement)
{
    ASSERT(pXMLElement);
    ASSERT(ppScreenSaverElement);

    IXMLElementCollection * pIXMLElementCollection;
    HRESULT                 hr;

    hr = pXMLElement->get_children(&pIXMLElementCollection);
    if (SUCCEEDED(hr) && pIXMLElementCollection)
    {
        LONG nCount;

        hr = pIXMLElementCollection->get_length(&nCount);

        ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

        hr = E_FAIL;

        BOOL bScreenSaver = FALSE;
        for (int i = 0; (i < nCount) && !bScreenSaver; i++)
        {
            IXMLElement * pIXMLElement;

            HRESULT hr2 = XML_GetElementByIndex(pIXMLElementCollection,
                                                i,
                                                &pIXMLElement);

            if (SUCCEEDED(hr2))
            {
                ASSERT(pIXMLElement != NULL);

                if (bScreenSaver = XML_IsScreenSaver(pIXMLElement))
                {
                    pIXMLElement->AddRef();
                    *ppScreenSaverElement = pIXMLElement;

                    hr = S_OK;
                }

                pIXMLElement->Release();
            }

            hr = FAILED(hr2) ? hr2 : hr;
        }

        pIXMLElementCollection->Release();
    }        
    else
        hr = E_FAIL;

    ASSERT((SUCCEEDED(hr) && *ppScreenSaverElement) || FAILED(hr));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetSubscriptionInfo ***
//
//
// Description:
//     Fills a structure with the subscription info for the given element.
//
// Parameters:
//     [In]  pIXMLElement - An xml element.  The element doesn't have to be a
//                          subscription element.
//     [Out] psi          - The subscription info structure used by the
//                          subscription manager.
//
// Return:
//     S_OK if any information was obtained.
//
// Comments:
//     This function uses a webcheck API that fills a task trigger with
//     subscription information.
//
//     This function assumes that the psi->pTrigger points to a TASK_TRIGGER.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
XML_GetSubscriptionInfo(
    IXMLElement* pIXMLElement,
    SUBSCRIPTIONINFO* psi
)
{
    ASSERT(pIXMLElement);
    ASSERT(psi);
    ASSERT(psi->pTrigger);

    HRESULT hr = E_FAIL;

#ifndef UNIX
    HINSTANCE hinst = LoadLibrary(TEXT("webcheck.dll"));

    if (hinst)
    {
        typedef (*PFTRIGGERFUNCTION)(IXMLElement* pIXMLElement,
                                     TASK_TRIGGER* ptt);

        PFTRIGGERFUNCTION XMLSheduleElementToTaskTrigger;

        XMLSheduleElementToTaskTrigger = (PFTRIGGERFUNCTION)
                                         GetProcAddress(hinst,
                                             "XMLScheduleElementToTaskTrigger");

        if (XMLSheduleElementToTaskTrigger)
        {
            ((TASK_TRIGGER*)(psi->pTrigger))->cbTriggerSize = 
                                                           sizeof(TASK_TRIGGER);

            hr = XMLSheduleElementToTaskTrigger(pIXMLElement,
                                                (TASK_TRIGGER*)psi->pTrigger);

            if (FAILED(hr))
                psi->pTrigger = NULL;
        }

        FreeLibrary(hinst);
    }

    // See if there is a screen saver available.
    IXMLElement * pScreenSaverElement;
    if (SUCCEEDED(XML_GetScreenSaverElement(   pIXMLElement,
                                                    &pScreenSaverElement)))
    {
        psi->fUpdateFlags |= SUBSINFO_CHANNELFLAGS;
        psi->fChannelFlags |= CHANNEL_AGENT_PRECACHE_SCRNSAVER;
        pScreenSaverElement->Release();
    }

    BSTR bstrLogin = XML_GetAttribute(pIXMLElement, XML_LOGIN);

    if (bstrLogin)
    {
        psi->bNeedPassword = TRUE;
        psi->fUpdateFlags |= SUBSINFO_NEEDPASSWORD;     //this member is now valid
        SysFreeString(bstrLogin);
    }

#endif /* !UNIX */

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_GetBaseURL ***
//
//
// Description:
//     Returns the base url for the given collection.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to an XML element.
//
// Return:
//     A bstr containing the base URL if there is one.
//     NULL if ther isn't a base URL.
//
// Comments:
//     If the current element has a BASE attribute return this attributes value.
//     Else return the BASE attribute of its parent.
//
////////////////////////////////////////////////////////////////////////////////
BSTR
XML_GetBaseURL(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    BSTR bstrRet = XML_GetAttribute(pIXMLElement, XML_BASE);

    if (NULL == bstrRet)
    {
        IXMLElement* pParentIXMLElement;

        if (SUCCEEDED(pIXMLElement->get_parent(&pParentIXMLElement)) && pParentIXMLElement)
        {
            ASSERT(pParentIXMLElement);

            bstrRet = XML_GetBaseURL(pParentIXMLElement);

            pParentIXMLElement->Release();
        }
    }

    return bstrRet;
}
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_CombineURL ***
//
//
// Description:
//     Combine the given URL with the base URL.
//
// Parameters:
//     [In]  bstrBaseURL - The base URL.
//     [In]  bstrRelURL  - The relative URL.
//
// Return:
//     A combination of the base and relative URL.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BSTR
XML_CombineURL(
    BSTR bstrBaseURL,
    BSTR bstrRelURL
)
{
    ASSERT(bstrBaseURL);
    ASSERT(bstrRelURL);

    BSTR bstrRet = NULL;

    WCHAR wszCombinedURL[INTERNET_MAX_URL_LENGTH];
    DWORD cch = ARRAYSIZE(wszCombinedURL);

    if (InternetCombineUrlW(bstrBaseURL, bstrRelURL, wszCombinedURL, &cch, 0))
        bstrRet = SysAllocString(wszCombinedURL);

    return bstrRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsCdfDisplayable ***
//
//
// Description:
//     Determines if the given item should be displayed in the cdf view.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the IXMLElement interface of an object.
//
// Return:
//     TRUE if the object should be displayed.
//     FALSE otherwise.
//
// Comments:
//     aCDFTypes contains the tag names of XML items that the cdf shell
//     shell extension displays.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsCdfDisplayable(
    IXMLElement* pIXMLElement
)
{
    #define     KEYWORDS  (sizeof(aCDFTypes) / sizeof(aCDFTypes[0]))

    static const LPWSTR aCDFTypes[] = { 
                                        WSTR_ITEM,
                                        WSTR_CHANNEL,
                                        WSTR_SOFTDIST
                                      };

    ASSERT(pIXMLElement);

    BOOL bRet = FALSE;

    BSTR pStr;

    HRESULT hr = pIXMLElement->get_tagName(&pStr);

    if (SUCCEEDED(hr) && pStr)
    {
        ASSERT(pStr);

        for(int i = 0; (i < KEYWORDS) && !bRet; i++)
            bRet = StrEqlW(pStr, aCDFTypes[i]);

        if (bRet)
            bRet = XML_IsUsageChannel(pIXMLElement);

        //
        // Special processing.
        //

        if (bRet && StrEqlW(pStr, WSTR_SOFTDIST))
            bRet = XML_IsSoftDistDisplayable(pIXMLElement);

        SysFreeString(pStr);
    }

    return bRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsSoftDistDisplayable ***
//
//
// Description:
//     Determins if the given software distribution element should be displayed.
//
// Parameters:
//     [In]  pIXMLElement - Pointer to the software distribution xml element.
//
// Return:
//     TRUE if the element should be displayed.
//     FALSE if the element shouldn't be displayed.
//
// Comments:
//     This function asks the software disribution COM object if this software
//     distribution tag should be displayed on this users machine.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsSoftDistDisplayable(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    ISoftDistExt* pISoftDistExt;

    HRESULT hr = CoCreateInstance(CLSID_SoftDistExt, NULL, CLSCTX_INPROC_SERVER,
                                  IID_ISoftDistExt, (void**)&pISoftDistExt);

    if (SUCCEEDED(hr))
    {
        ASSERT(pISoftDistExt);

        hr = pISoftDistExt->ProcessSoftDist(NULL, pIXMLElement, 0);

        pISoftDistExt->Release();
    }

    return SUCCEEDED(hr) ? TRUE : FALSE;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsUsageChannel ***
//
//
// Description:
//     Determines if this item should be displayed in channel view based on its
//     usage tag.
//
// Parameters:
//     [In]  pIXMLelement - A pointer to the element.
//
// Return:
//     TRUE if the item should be displayed in the channel view.
//     FALSE otherwise.
//
// Comments:
//     If an element doesn't have a USAGE tag then it gets displayed.  If an
//     element has any numberf of usage tags one of them must have a value of
//     CHANNEL or will not get displayed.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsUsageChannel(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    BOOL bRet;

    //
    // First check if there are any USAGE elements.
    //

    BSTR bstrUsage = XML_GetAttribute(pIXMLElement, XML_USAGE);

    if (bstrUsage)
    {
        //
        // See if USAGE is CHANNEL.
        //

        if (StrEqlW(bstrUsage, WSTR_CHANNEL))
        {
            bRet = TRUE;
        }
        else
        {
            //
            // Check if there are any other USAGE tags with value CHANNEL.
            //

            BSTR bstrChannel = XML_GetAttribute(pIXMLElement,
                                                XML_USAGE_CHANNEL);

            if (bstrChannel)
            {
                SysFreeString(bstrChannel);
                bRet = TRUE;
            }
            else
            {
                bRet = FALSE;
            }
        }

        SysFreeString(bstrUsage);
    }
    else
    {
        bRet = TRUE;  // No USAGE tag defaults channel usage.
    }

    return bRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsChannel ***
//
//
// Description:
//     Determines if the given XML item is a channel.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the IXMLElement interface of an object.
//
// Return:
//     TRUE if the object is a channel.
//     FALSE otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsChannel(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    BOOL bRet = FALSE;

    BSTR pStr;

    HRESULT hr = pIXMLElement->get_tagName(&pStr);

    if (SUCCEEDED(hr) && pStr)
    {
        ASSERT(pStr);

        bRet = StrEqlW(pStr, WSTR_CHANNEL);

        SysFreeString(pStr);
    }

    return bRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsDesktopComponent ***
//
//
// Description:
//     Determines if the given XML item is a desktop component.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the IXMLElement interface of an object.
//
// Return:
//     TRUE if the object is a desktop component.
//     FALSE otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsDesktopComponent(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    BOOL bRet;

    BSTR bstr = XML_GetAttribute(pIXMLElement, XML_USAGE_DSKCMP);

    if (bstr)
    {
        SysFreeString(bstr);
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsScreenSaver ***
//
//
// Description:
//     Determines if the given XML item is a screen saver.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the IXMLElement interface of an object.
//
// Return:
//     TRUE if the object is a screen saver
//     FALSE otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsScreenSaver(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    BOOL bRet;

    BSTR bstrUsage = XML_GetAttribute(pIXMLElement, XML_USAGE);

    if (bstrUsage)
    {
        bRet =  (
                (StrCmpIW(bstrUsage, WSTR_SCRNSAVE) == 0)
                ||
                (StrCmpIW(bstrUsage, WSTR_SMARTSCRN) == 0)
                );

        SysFreeString(bstrUsage);
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsDesktopComponentUsage ***
//
//
// Description:
//     Determines if the given XML item is a desktop component usage element.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the IXMLElement interface of an object.
//
// Return:
//     TRUE if the object is a desktop component usage element.
//     FALSE otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsDesktopComponentUsage(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    BOOL bRet = FALSE;

    BSTR bstrName;

    if (SUCCEEDED(pIXMLElement->get_tagName(&bstrName)) && bstrName)
    {
        ASSERT(bstrName);

        if (StrEqlW(bstrName, WSTR_USAGE))
        {
            BSTR bstrValue = XML_GetElementAttribute(pIXMLElement, WSTR_VALUE, NULL,
                                                NULL);

            if (bstrValue)
            {
                bRet = (0 == StrCmpIW(bstrValue, WSTR_DSKCMP));

                SysFreeString(bstrValue);
            }
        }

        SysFreeString(bstrName);
    }

    return bRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsFolder ***
//
//
// Description:
//     Determines if the given item is a folder.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the IXMLElement interface of an object.
//
// Return:
//     TRUE if the object contains other cdf displayable objects.
//     FALSE otherwise.
//
// Comments:
//     An item is a folder if at least one of its children is displayable as a
//     cdf item.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsFolder(
    IXMLElement* pIXMLElement
)
{
    ASSERT(pIXMLElement);

    BOOL bRet = FALSE;

    IXMLElementCollection* pIXMLElementCollection;

    HRESULT hr = pIXMLElement->get_children(&pIXMLElementCollection);

    if (SUCCEEDED(hr) && pIXMLElementCollection)
    {
        ASSERT(pIXMLElementCollection);

        LONG nCount;

        hr = pIXMLElementCollection->get_length(&nCount);

        ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

        for (int i = 0; (i < nCount) && !bRet; i++)
        {
            IXMLElement* pIXMLElementTemp;

            hr = XML_GetElementByIndex(pIXMLElementCollection, i, &pIXMLElementTemp);

            if (SUCCEEDED(hr))
            {
                ASSERT(pIXMLElementTemp);

                if (XML_IsCdfDisplayable(pIXMLElementTemp))
                    bRet = TRUE;

                pIXMLElementTemp->Release();
            }
        }

        pIXMLElementCollection->Release();
    }

    return bRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_ContainsFolder ***
//
//
// Description:
//     Determines if there are any cdf folders in the given collection.
//
// Parameters:
//     [In]  pIXMLElementCollection - A pointer to the collection.
//
// Return:
//     TRUE if the collection contains a cf folder.
//     FALSE otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_ContainsFolder(
    IXMLElementCollection* pIXMLElementCollection
)
{
    ASSERT(pIXMLElementCollection);

    BOOL bContainsFolder = FALSE;

    LONG nCount;

    HRESULT hr = pIXMLElementCollection->get_length(&nCount);

    ASSERT(SUCCEEDED(hr) || (FAILED(hr) && 0 == nCount));

    for (int i = 0; (i < nCount) && !bContainsFolder; i++)
    {
        IXMLElement* pIXMLElement;

        hr = XML_GetElementByIndex(pIXMLElementCollection, i, &pIXMLElement);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            bContainsFolder = XML_IsFolder(pIXMLElement);

            pIXMLElement->Release();
        }
    }

    return bContainsFolder;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_ChildContainsFolder ***
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
BOOL
XML_ChildContainsFolder(
    IXMLElementCollection *pIXMLElementCollectionParent,
    ULONG nIndexChild
)
{
    BOOL bRet = FALSE;

    IXMLElement* pIXMLElement;

    HRESULT hr = XML_GetElementByIndex(pIXMLElementCollectionParent,
                                       nIndexChild, &pIXMLElement);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLElement);

        IXMLElementCollection* pIXMLElementCollection;

        hr = pIXMLElement->get_children(&pIXMLElementCollection);

        if (SUCCEEDED(hr) && pIXMLElementCollection)
        {
            ASSERT(pIXMLElementCollection);

            bRet = XML_ContainsFolder(pIXMLElementCollection);

            pIXMLElementCollection->Release();
        }

        pIXMLElement->Release();
    }

    return bRet;
}

#ifdef DEBUG

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** XML_IsCdfidlMemberOf ***
//
//
// Description:
//     Checks if the cdf item id list is associated with a member of the given
//     element collection.
//
// Parameters:
//     [In]  pIXMLElementCollection - The element collection to check.
//     [In]  pcdfidl                - A pointer to cdf item id list
//
// Return:
//     TRUE if the given id list can be associated with an elemnt of the given 
//     collection.
//     FALSE otherwise.
//
// Comments:
//     This function checks if the last id in the list could have been
//     generated from its corresponding element in the element collection.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
XML_IsCdfidlMemberOf(
    IXMLElementCollection* pIXMLElementCollection,
    PCDFITEMIDLIST pcdfidl
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));

    BOOL bRet = FALSE;

    //
    // pIXMLElementCollection is NULL when a Folder hasn't been initialized.
    // It isn't always neccessary to parse the cdf to get pidl info from
    // the pidl.  pIXMLElement collection will be NULL in low memory situations
    // also.  Don't return FALSE for these cases.  Also check for special
    // pidls that aren't in element collections.
    //

    if (pIXMLElementCollection &&
        CDFIDL_GetIndexId(&pcdfidl->mkid) != INDEX_CHANNEL_LINK)
    {
        IXMLElement* pIXMLElement;

        HRESULT hr = XML_GetElementByIndex(pIXMLElementCollection,
                                           CDFIDL_GetIndexId(&pcdfidl->mkid),
                                           &pIXMLElement);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIXMLElement);

            PCDFITEMIDLIST pcdfidlElement;

            pcdfidlElement = CDFIDL_CreateFromXMLElement(pIXMLElement,
                                             CDFIDL_GetIndexId(&pcdfidl->mkid));

            if (pcdfidlElement)
            {
                ASSERT(CDFIDL_IsValid(pcdfidlElement));

                bRet = (0 == CDFIDL_CompareId(&pcdfidl->mkid,
                                              &pcdfidlElement->mkid));

                CDFIDL_Free(pcdfidlElement);
            }

            pIXMLElement->Release();
        }
    }
    else
    {
        bRet = TRUE;
    }

    return bRet;
}

#endif //DEBUG

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//                                                           
// *** XML_IsStrEqualW ***
//
//
// Description:
//     Determines if two WCHAR strings are equal.
//
// Parameters:
//     [In]  p1 - The first string to compare.
//     [In]  p2 - The second string to compare.
//
// Return:
//     TRUE if the strings are equal.
//     FALSE otherwise.
//
// Comments:
//     lstrcmpW doesn't work on W95 so this function has its own strcmp logic.
//
////////////////////////////////////////////////////////////////////////////////
#if 0
inline
BOOL
XML_IsStrEqualW(
    LPWSTR p1,
    LPWSTR p2
)
{
    ASSERT(p1);
    ASSERT(p2);

    while ((*p1 == *p2) && *p1 && *p2)
    {
        p1++; p2++;
    }

    return (*p1 == *p2);
}
#endif

