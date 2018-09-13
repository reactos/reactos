/*++

    Implements IEnumIDList.

--*/

#include <windows.h>
#include <shlobj.h>

#include "pstore.h"

#include "utility.h"

#include "enumid.h"
#include "shfolder.h"



extern LONG       g_DllRefCount;



CEnumIDList::CEnumIDList(
    LPITEMIDLIST pidl,
    BOOL bEnumItems
    )
{


    m_pIEnumProviders = NULL;
    m_pIPStoreProvider = NULL;
    m_pIEnumTypes = NULL;
    m_pIEnumTypesGlobal = NULL;
    m_pIEnumSubtypes = NULL;
    m_pIEnumItems = NULL;

    m_bEnumItems = bEnumItems;


    //
    // get the shell's IMalloc pointer
    // we'll keep this until we get destroyed
    //

    if(FAILED(SHGetMalloc(&m_pMalloc)))
        delete this;

    m_KeyType = PST_KEY_CURRENT_USER;

    if(pidl == NULL) {

        if( PStoreEnumProviders(0, &m_pIEnumProviders) != S_OK )
            m_pIEnumProviders = NULL;

        m_dwType = PIDL_TYPE_PROVIDER;  // top-level
    } else {
        m_dwType = GetLastPidlType(pidl) + 1;   // parent + 1
    }

    //
    // get provider interface
    //

    if(m_dwType > PIDL_TYPE_PROVIDER) {
        PST_PROVIDERID      *ProviderId = GetPidlGuid(pidl);
        if(PStoreCreateInstance(&m_pIPStoreProvider, ProviderId, NULL, 0) != S_OK)
            m_pIPStoreProvider = NULL;
    }

    //
    // prepare two type enumerators, if appropriate:
    // PST_KEY_CURRENT_USER and PST_KEY_LOCAL_MACHINE
    //

    if(m_dwType == PIDL_TYPE_TYPE && m_pIPStoreProvider) {
        m_pIPStoreProvider->EnumTypes(PST_KEY_CURRENT_USER, 0, &m_pIEnumTypes);
        m_pIPStoreProvider->EnumTypes(PST_KEY_LOCAL_MACHINE, 0, &m_pIEnumTypesGlobal);
    }

    //
    // prepare subtype enumerator
    //

    if(m_dwType == PIDL_TYPE_SUBTYPE && m_pIPStoreProvider) {
        GUID    *pguidType;

        m_KeyType = GetLastPidlKeyType(pidl);
        pguidType = GetLastPidlGuid(pidl);
        CopyMemory(&m_guidType, pguidType, sizeof(GUID));

        m_pIPStoreProvider->EnumSubtypes(
                m_KeyType,
                pguidType,
                0,
                &m_pIEnumSubtypes
                );
    }

    //
    // prepare item enumerator if appropriate.
    //

    if(m_dwType == PIDL_TYPE_ITEM && m_bEnumItems && m_pIPStoreProvider) {
        GUID    *pguidType;
        GUID    *pguidSubtype;

        m_KeyType = GetLastPidlKeyType(pidl);
        pguidSubtype = GetLastPidlGuid(pidl);
        CopyMemory(&m_guidSubtype, pguidSubtype, sizeof(GUID));

        pguidType = GetPidlGuid(SearchPidlByType(pidl, PIDL_TYPE_TYPE));
        CopyMemory(&m_guidType, pguidType, sizeof(GUID));

        m_pIPStoreProvider->EnumItems(
                m_KeyType,
                pguidType,
                pguidSubtype,
                0,
                &m_pIEnumItems
                );
    }

    m_ulCurrent = 0;
    m_ObjRefCount = 1;

    InterlockedIncrement(&g_DllRefCount);
}


CEnumIDList::~CEnumIDList()
{
    //
    // free any interfaces we established.
    //

    if(m_pIEnumProviders) {
        m_pIEnumProviders->Release();
        m_pIEnumProviders = NULL;
    }

    if(m_pIPStoreProvider) {
        m_pIPStoreProvider->Release();
        m_pIPStoreProvider = NULL;
    }

    if(m_pIEnumTypes) {
        m_pIEnumTypes->Release();
        m_pIEnumTypes = NULL;
    }

    if(m_pIEnumTypesGlobal) {
        m_pIEnumTypesGlobal->Release();
        m_pIEnumTypesGlobal = NULL;
    }

    if(m_pIEnumSubtypes) {
        m_pIEnumSubtypes->Release();
        m_pIEnumSubtypes = NULL;
    }

    if(m_pIEnumItems) {
        m_pIEnumItems->Release();
        m_pIEnumItems = NULL;
    }

    if(m_pMalloc) {
        m_pMalloc->Release();
        m_pMalloc = NULL;
    }

    InterlockedDecrement(&g_DllRefCount);
}



STDMETHODIMP
CEnumIDList::QueryInterface(
    REFIID riid,
    LPVOID *ppReturn
    )
{
    *ppReturn = NULL;

    if(IsEqualIID(riid, IID_IUnknown))
        *ppReturn = (IUnknown*)(CEnumIDList*)this;
    else if(IsEqualIID(riid, IID_IEnumIDList))
        *ppReturn = (CEnumIDList*)this;

    if(*ppReturn == NULL)
        return E_NOINTERFACE;

    (*(LPUNKNOWN*)ppReturn)->AddRef();
    return S_OK;
}


STDMETHODIMP_(DWORD)
CEnumIDList::AddRef()
{
    return InterlockedIncrement(&m_ObjRefCount);
}


STDMETHODIMP_(DWORD)
CEnumIDList::Release()
{
    LONG lDecremented = InterlockedDecrement(&m_ObjRefCount);

    if(lDecremented == 0)
        delete this;

    return lDecremented;
}

STDMETHODIMP
CEnumIDList::Next(
    ULONG ulElements,
    LPITEMIDLIST *pPidl,
    ULONG *pulFetched
    )
/*++

    Retrieves the specified number of item identifiers in the enumeration
    sequence and advances the current position.

    Returns the NOERROR value if successful,
    Returns S_FALSE value if there are no more items in the enumeration
    or an OLE-defined error value if an error occurs.

--*/
{
    HRESULT hr;

    *pPidl = NULL;
    *pulFetched = 0;

    if(m_bEnumItems) {
        if(m_dwType > PIDL_TYPE_ITEM)
            return S_FALSE;
    } else {
        if(m_dwType > PIDL_TYPE_SUBTYPE)
            return S_FALSE; // nothing left to enumerate
    }


    if(m_dwType == PIDL_TYPE_PROVIDER && m_pIEnumProviders)
    {
        PPST_PROVIDERINFO   pProvInfo;

        if( m_pIEnumProviders->Next(1, &pProvInfo, &m_ulCurrent) != S_OK ) {

            //
            // enumeration of providers complete
            //

            return S_FALSE;
        }

        hr = CreateIDList(m_dwType, m_KeyType, &(pProvInfo->ID), pProvInfo->szProviderName, pPidl);

        CoTaskMemFree(pProvInfo);

        if(hr != S_OK)
            return S_FALSE;

        *pulFetched = 1;
        return NOERROR;
    }

    //
    // must have a valid provider interface at this point
    //

    if(m_pIPStoreProvider == NULL)
        return S_FALSE;

    if(m_dwType == PIDL_TYPE_TYPE) {

        IEnumPStoreTypes    *pIEnumTypes;
        PST_KEY             KeyType = PST_KEY_CURRENT_USER;
        GUID                guidType;

type_enum:

        if(KeyType == PST_KEY_LOCAL_MACHINE)
            pIEnumTypes = m_pIEnumTypesGlobal;
        else
            pIEnumTypes = m_pIEnumTypes;

        if(pIEnumTypes == NULL)
            return S_FALSE;

        if(pIEnumTypes->Next(1, &guidType, &m_ulCurrent) != S_OK) {

            //
            // if enumeration at PST_KEY_CURRENT_USER level complete,
            // continue at the PST_KEY_LOCAL_MACHINE level.
            //

            if(KeyType == PST_KEY_CURRENT_USER) {
                KeyType = PST_KEY_LOCAL_MACHINE;
                goto type_enum;
            }

            //
            // enumeration of types complete
            //

            return S_FALSE;
        }

        PST_TYPEINFO        *pTypeInfo = NULL;

        if(S_OK != m_pIPStoreProvider->GetTypeInfo(
                    KeyType,
                    &guidType,
                    &pTypeInfo,
                    0
                    )) return S_FALSE;

        hr = CreateIDList(m_dwType, KeyType, &guidType, pTypeInfo->szDisplayName, pPidl);

        //
        // free pTypeInfo data
        //

        CoTaskMemFree(pTypeInfo);

        if(hr != S_OK)
            return S_FALSE;

        *pulFetched = 1;
        return NOERROR;
    }

    if (m_dwType == PIDL_TYPE_SUBTYPE && m_pIEnumSubtypes) {

        GUID                guidSubtype;
        GUID                *pguidType = &m_guidType;

        if(m_pIEnumSubtypes->Next(1, &guidSubtype, &m_ulCurrent) != S_OK) {

            //
            // enumeration of types complete
            //

            return S_FALSE;
        }

        PST_TYPEINFO        *pTypeInfo = NULL;

        if(m_pIPStoreProvider->GetSubtypeInfo(m_KeyType, pguidType, &guidSubtype, &pTypeInfo, 0) != S_OK)
            return S_FALSE;

        hr = CreateIDList(m_dwType, m_KeyType, &guidSubtype, pTypeInfo->szDisplayName, pPidl);

        //
        // free pTypeInfo data
        //

        CoTaskMemFree(pTypeInfo);

        if(hr != S_OK)
            return S_FALSE;

        *pulFetched = 1;
        return NOERROR;
    }

    if(m_dwType == PIDL_TYPE_ITEM && m_pIEnumItems) {

        LPWSTR pszItem;

        //
        // enumerate and add items associated with specified subtype
        //

        if(m_pIEnumItems->Next(1, &pszItem, &m_ulCurrent) != S_OK) {

            //
            // enumeration of items complete
            //

            return S_FALSE;
        }

        hr = CreateIDList(
                    m_dwType,
                    m_KeyType,
                    NULL, // no item guid
                    pszItem,
                    pPidl
                    );

        //
        // free pszItem data
        //

        CoTaskMemFree(pszItem);

        if(hr != S_OK)
            return S_FALSE;

        *pulFetched = 1;
        return NOERROR;
    }

    return S_FALSE;
}


STDMETHODIMP
CEnumIDList::CreateIDList(
    DWORD dwType,
    PST_KEY KeyType,
    GUID *guid,
    LPCWSTR szString,
    LPITEMIDLIST *pPidlOut
    )
{
    DWORD cbString = 0;
    DWORD cbPidlContent;

    if(szString) {
        cbString = lstrlenW(szString);
    }

    cbString = (cbString + 1) * sizeof(WCHAR);

    cbPidlContent = sizeof(PIDL_CONTENT) + cbString;

    //
    // allocate the memory
    //

    *pPidlOut = (LPITEMIDLIST)m_pMalloc->Alloc(
            sizeof(ITEMIDLIST) +    // this item's ITEMIDLIST
            cbPidlContent +         // this item's data
            sizeof(ITEMIDLIST)      // terminal ITEMIDLIST
            );

    if(*pPidlOut == NULL)
        return E_OUTOFMEMORY;


    LPITEMIDLIST pidlTemp = *pPidlOut;
    LPPIDL_CONTENT pidlContent = (LPPIDL_CONTENT)&(pidlTemp->mkid.abID);


    //
    // set the size of this item
    //

    pidlTemp->mkid.cb = sizeof(ITEMIDLIST) + cbPidlContent;

    //
    // set the data for this item
    //

    pidlContent->dwType = dwType;
    pidlContent->KeyType = KeyType;

    if( guid ) {
        CopyMemory(&(pidlContent->guid), guid, sizeof(GUID));
    } else {
        ZeroMemory(&(pidlContent->guid), sizeof(GUID));
    }

    if(szString) {
        CopyMemory((LPBYTE)(pidlContent+1), szString, cbString);
    } else {
        ((LPWSTR)(pidlContent+1))[0] = L'\0';
    }

    //
    // advance and terminate item ID list
    //

    pidlTemp = (LPITEMIDLIST)GetPidlNextItem(*pPidlOut);
    pidlTemp->mkid.cb = 0;
    pidlTemp->mkid.abID[0] = 0;

    return NOERROR;
}



STDMETHODIMP
CEnumIDList::Skip(
    ULONG ulSkip
    )
{
    m_ulCurrent += ulSkip;

    return NOERROR;
}


STDMETHODIMP
CEnumIDList::Reset(
    void
    )
{
    m_ulCurrent = 0;

    return NOERROR;
}


STDMETHODIMP
CEnumIDList::Clone(
    LPENUMIDLIST *ppEnum
    )
{
    return E_NOTIMPL;
}
