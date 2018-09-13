// Implementation of the CEnumString classs

#include "zonepch.h"


// Constructor - destructor.

CEnumString::CEnumString() : m_ref() 
{
    pFirst = NULL;
    pLast = NULL;
    pCurrent = NULL;
}


CEnumString::~CEnumString( )
{
    // Delete all the strings we created.
    ListStr * pNext = pFirst;
    
    while (pNext != NULL)
    {
        ListStr * pDelete = pNext;
        pNext = pNext->pListNext;

        LocalFree(pDelete->lpsz);
        delete pDelete;
    }
}

HRESULT CEnumString::AddString(LPCWSTR lpsz)
{
    // First create the element.
    ListStr *pListStr = new ListStr;
    if (pListStr == NULL)
        return E_OUTOFMEMORY;

    pListStr->lpsz = StrDup(lpsz);
    if (pListStr->lpsz == NULL)
    {
        delete pListStr;
        return E_OUTOFMEMORY;
    }

    pListStr->pListNext = NULL;

    if (pFirst == NULL)
    {
        TransAssert(pCurrent == NULL);
        TransAssert(pLast == NULL);
        pFirst = pCurrent = pLast = pListStr;
    }
    else 
    {
        TransAssert(pLast != NULL);
        // We don't support adding strings while an enumeration is on.
        TransAssert(pFirst == pCurrent);
        
        pLast->pListNext = pListStr;
        pLast = pListStr;
    }

    return S_OK;
}
                
// IUnknown methods.

STDMETHODIMP_(ULONG) CEnumString::AddRef( )
{
    LONG lRet = ++m_ref;
    
    return lRet;
}

STDMETHODIMP_(ULONG) CEnumString::Release( )
{

    LONG lRet =  --m_ref;
    
    if (m_ref == 0)
    {
        delete this;
    }

    return lRet;
}

STDMETHODIMP  CEnumString::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = NOERROR;   
    
    if ( (riid == IID_IUnknown) || (riid == IID_IEnumString))
    { 
        *ppvObj = this;       
        AddRef();
    }
    else 
    {
        *ppvObj = NULL;
        hr  = E_NOINTERFACE;
    }

    return hr;
}


// IEnumString methods.


STDMETHODIMP
CEnumString::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = NOERROR;
    ULONG cFound = 0;

    // Check the arguments first.     
    if (celt == 0)
        return S_OK;
             
    if (!rgelt)
        return E_INVALIDARG;

    do 
    {
        if (pCurrent == NULL)
            break;
            
        DWORD cChars = lstrlenW(pCurrent->lpsz) + 1;
        rgelt[cFound] = (LPOLESTR)CoTaskMemAlloc(cChars * sizeof(OLECHAR));

        // If we don't have enough memory don't return anything. 
        if (rgelt[cFound] == NULL)
        {
            hr = E_OUTOFMEMORY;
            for (DWORD dwIndex = 0 ; dwIndex < cFound ; dwIndex++ )
            {
                CoTaskMemFree(rgelt[dwIndex]);
                rgelt[dwIndex] = 0;
            }
        } 
        
        StrCpyW(rgelt[cFound], pCurrent->lpsz);
        
        pCurrent = pCurrent->pListNext;
        
        cFound++;
    } while ( cFound < celt );
    
    if (hr == NOERROR)
    {
        if (pceltFetched)
            *pceltFetched = cFound;
            
        hr = (cFound == celt) ?  NOERROR : S_FALSE;
    }
    
    return hr;
}                                      

                    
    
STDMETHODIMP
CEnumString::Skip(ULONG celt)
{
    return E_NOTIMPL;
}


STDMETHODIMP
CEnumString::Reset( )
{
    pCurrent = pFirst;

    return S_OK;
}

STDMETHODIMP
CEnumString::Clone(IEnumString **ppEnumString)
{
    // This should be easy to implement if we do a deep copy.
    // However we should be able to seperate the list from the 
    // current pointer and do a much more efficient job.
    // Not impl for code complete.

    return E_NOTIMPL;
}