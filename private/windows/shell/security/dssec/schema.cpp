//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       schema.cpp
//
//  This file contains the implementation of the Schema Cache
//
//--------------------------------------------------------------------------

#include "pch.h"


//
// CSchemaCache object definition
//
#include "schemap.h"

PSCHEMACACHE g_pSchemaCache = NULL;


//
// Page size used for paging query result sets (better performance)
//
#define PAGE_SIZE       16


// The following array defines the permission names for DS objects.
SI_ACCESS g_siDSAccesses[] =
{
    { &GUID_NULL, DS_GENERIC_ALL,           MAKEINTRESOURCE(IDS_DS_GENERIC_ALL),        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DS_GENERIC_READ,          MAKEINTRESOURCE(IDS_DS_GENERIC_READ),       SI_ACCESS_GENERAL },
    { &GUID_NULL, DS_GENERIC_WRITE,         MAKEINTRESOURCE(IDS_DS_GENERIC_WRITE),      SI_ACCESS_GENERAL },
    { &GUID_NULL, ACTRL_DS_LIST,            MAKEINTRESOURCE(IDS_ACTRL_DS_LIST),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_LIST_OBJECT,     MAKEINTRESOURCE(IDS_ACTRL_DS_LIST_OBJECT),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_READ_PROP,       MAKEINTRESOURCE(IDS_ACTRL_DS_READ_PROP),    SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { &GUID_NULL, ACTRL_DS_WRITE_PROP,      MAKEINTRESOURCE(IDS_ACTRL_DS_WRITE_PROP),   SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { &GUID_NULL, DELETE,                   MAKEINTRESOURCE(IDS_ACTRL_DELETE),          SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_DELETE_TREE,     MAKEINTRESOURCE(IDS_ACTRL_DS_DELETE_TREE),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,             MAKEINTRESOURCE(IDS_ACTRL_READ_CONTROL),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,                MAKEINTRESOURCE(IDS_ACTRL_CHANGE_ACCESS),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,              MAKEINTRESOURCE(IDS_ACTRL_CHANGE_OWNER),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,                        MAKEINTRESOURCE(IDS_NO_ACCESS),             0 },
    { &GUID_NULL, ACTRL_DS_SELF,            MAKEINTRESOURCE(IDS_ACTRL_DS_SELF),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_CONTROL_ACCESS,  MAKEINTRESOURCE(IDS_ACTRL_DS_CONTROL_ACCESS),SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_CREATE_CHILD,    MAKEINTRESOURCE(IDS_ACTRL_DS_CREATE_CHILD), SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_DELETE_CHILD,    MAKEINTRESOURCE(IDS_ACTRL_DS_DELETE_CHILD), SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
};
#define g_iDSRead       1   // DS_GENERIC_READ
#define g_iDSListObject 4   // ACTRL_DS_LIST_OBJECT
#define g_iDSProperties 5   // Read/Write properties
#define g_iDSDefAccess  g_iDSRead


// The following array defines the inheritance types common to all DS containers.
SI_INHERIT_TYPE g_siDSInheritTypes[] =
{
    { &GUID_NULL, 0,                                        MAKEINTRESOURCE(IDS_DS_CONTAINER_ONLY)     },
    { &GUID_NULL, CONTAINER_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_DS_CONTAINER_SUBITEMS) },
    { &GUID_NULL, CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, MAKEINTRESOURCE(IDS_DS_SUBITEMS_ONLY)      },
};


typedef struct _temp_info
{
    LPCGUID pguid;
    DWORD   dwFilter;
    LPCWSTR pszLdapName;
    WCHAR   szDisplayName[ANYSIZE_ARRAY];
} TEMP_INFO, *PTEMP_INFO;


//
// Helper functions for cleaning up DPA lists
//
int CALLBACK
_LocalFreeCB(LPVOID pVoid, LPVOID /*pData*/)
{
    LocalFree(pVoid);
    return 1;
}

void
DestroyDPA(HDPA hList)
{
    if (hList != NULL)
        DPA_DestroyCallback(hList, _LocalFreeCB, 0);
}


BSTR
GetFilterFilePath(void)
{
    WCHAR szFilterFile[MAX_PATH];
    UINT cch = GetSystemDirectory(szFilterFile, ARRAYSIZE(szFilterFile));
    if (0 == cch || cch >= ARRAYSIZE(szFilterFile))
        return NULL;
    if (szFilterFile[cch-1] != L'\\')
        szFilterFile[cch++] = L'\\';
    lstrcpynW(szFilterFile + cch, c_szFilterFile, ARRAYSIZE(szFilterFile) - cch);
    return SysAllocString(szFilterFile);
}


//
// Local prototypes
//
HRESULT
Schema_Search(LPWSTR pszSchemaSearchPath,
              LPCWSTR pszFilter,
              HDPA *phCache);

HRESULT
Schema_GetExtendedRightsForClass(LPWSTR pszSchemaSearchPath,
                                 LPCGUID pguidClass,
                                 DWORD dwPageFlags,
                                 HDPA *phERList);

//
// C wrappers for the schema cache object
//
HRESULT
SchemaCache_Create(LPCWSTR pszServer)
{
    HRESULT hr = S_OK;

    if (g_pSchemaCache == NULL)
    {
        g_pSchemaCache = new CSchemaCache(pszServer);

        if (g_pSchemaCache  == NULL)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


void
SchemaCache_Destroy(void)
{
    delete g_pSchemaCache;
    g_pSchemaCache = NULL;
}


HRESULT
SchemaCache_GetInheritTypes(LPCGUID pguidObjectType,
                            DWORD dwFlags,
                            PSI_INHERIT_TYPE *ppInheritTypes,
                            ULONG *pcInheritTypes)
{
    HRESULT hr = E_UNEXPECTED;

    if (g_pSchemaCache)
        hr = g_pSchemaCache->GetInheritTypes(pguidObjectType, dwFlags, ppInheritTypes, pcInheritTypes);

    return hr;
}


HRESULT
SchemaCache_GetAccessRights(LPCGUID pguidObjectType,
                            LPCWSTR pszClassName,
                            LPCWSTR pszSchemaPath,
                            DWORD dwFlags,
                            PSI_ACCESS *ppAccesses,
                            ULONG *pcAccesses,
                            ULONG *piDefaultAccess)
{
    HRESULT hr = E_UNEXPECTED;

    if (g_pSchemaCache)
        hr = g_pSchemaCache->GetAccessRights(pguidObjectType,
                                             pszClassName,
                                             pszSchemaPath,
                                             dwFlags,
                                             ppAccesses,
                                             pcAccesses,
                                             piDefaultAccess);
    return hr;
}


//
// DPA comparison function used for sorting and searching the cache lists
//
int CALLBACK
Schema_CompareLdapName(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    int nResult = 0;
    PID_CACHE_ENTRY pEntry1 = (PID_CACHE_ENTRY)p1;
    PID_CACHE_ENTRY pEntry2 = (PID_CACHE_ENTRY)p2;
    LPCWSTR pszFind = (LPCWSTR)lParam;

    if (pEntry1)
        pszFind = pEntry1->szLdapName;

    if (pszFind && pEntry2)
    {
        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 pszFind,
                                 -1,
                                 pEntry2->szLdapName,
                                 -1) - CSTR_EQUAL;
    }

    return nResult;
}


//
// Callback function used to sort based on display name
//
int CALLBACK
Schema_CompareTempDisplayName(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    int nResult = 0;
    PTEMP_INFO pti1 = (PTEMP_INFO)p1;
    PTEMP_INFO pti2 = (PTEMP_INFO)p2;

    if (pti1 && pti2)
    {
        LPCWSTR psz1 = pti1->szDisplayName;
        LPCWSTR psz2 = pti2->szDisplayName;

        if (!*psz1)
            psz1 = pti1->pszLdapName;
        if (!*psz2)
            psz2 = pti2->pszLdapName;

        // Note that we are sorting backwards
        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 (LPCWSTR)psz2,
                                 -1,
                                 (LPCWSTR)psz1,
                                 -1) - CSTR_EQUAL;
    }

    return nResult;
}


//
// CSchemaCache object implementation
//
CSchemaCache::CSchemaCache(LPCWSTR pszServer)
{
    HRESULT hr;
    IADsPathname *pPath = NULL;
    BSTR strRootDSEPath = NULL;
    IADs *pRootDSE = NULL;
    VARIANT var = {0};
    DWORD dwThreadID;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::CSchemaCache");

    // Initialize everything
    ZeroMemory(this, sizeof(CSchemaCache));
    m_hrClassResult = E_UNEXPECTED;
    m_hrPropertyResult = E_UNEXPECTED;
    m_nDsListObjectEnforced = -1;
    InitializeCriticalSection(&m_AccessListCritSec);

    if (pszServer && !*pszServer)
        pszServer = NULL;

    // Create a path object for manipulating ADS paths
    hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pPath);
    FailGracefully(hr, "Unable to create ADsPathname object");

    // Build RootDSE path with server
    hr = pPath->Set((LPWSTR)c_szRootDsePath, ADS_SETTYPE_FULL);
    FailGracefully(hr, "Unable to initialize path object");
    if (pszServer)
    {
        hr = pPath->Set((LPWSTR)pszServer, ADS_SETTYPE_SERVER);
        FailGracefully(hr, "Unable to initialize path object");
    }
    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &strRootDSEPath);
    FailGracefully(hr, "Unable to retrieve RootDSE path from path object");

    // Bind to the RootDSE object
    hr = ADsOpenObject(strRootDSEPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       IID_IADs,
                       (LPVOID*)&pRootDSE);
    if (FAILED(hr) && pszServer)
    {
        // Try again with no server
        SysFreeString(strRootDSEPath);

        hr = pPath->Retrieve(ADS_FORMAT_WINDOWS_NO_SERVER, &strRootDSEPath);
        FailGracefully(hr, "Unable to retrieve RootDSE path from path object");

        hr = ADsOpenObject(strRootDSEPath,
                           NULL,
                           NULL,
                           ADS_SECURE_AUTHENTICATION,
                           IID_IADs,
                           (LPVOID*)&pRootDSE);
    }
    FailGracefully(hr, "Failed to bind to root DSE");

    // Build the schema root path
    hr = pRootDSE->Get((LPWSTR)c_szSchemaContext, &var);
    FailGracefully(hr, "Unable to get schema naming context");

    TraceAssert(V_VT(&var) == VT_BSTR);
    hr = pPath->Set(V_BSTR(&var), ADS_SETTYPE_DN);
    FailGracefully(hr, "Unable to initialize path object");

    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &m_strSchemaSearchPath);
    FailGracefully(hr, "Unable to retrieve schema search path from path object");

    // Build the Extended Rights container path
    VariantClear(&var);
    hr = pRootDSE->Get((LPWSTR)c_szConfigContext, &var);
    FailGracefully(hr, "Unable to get configuration naming context");

    TraceAssert(V_VT(&var) == VT_BSTR);
    hr = pPath->Set(V_BSTR(&var), ADS_SETTYPE_DN);
    FailGracefully(hr, "Unable to initialize path object");

    hr = pPath->AddLeafElement((LPWSTR)c_szERContainer);
    FailGracefully(hr, "Unable to build Extended Rights path");

    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &m_strERSearchPath);
    FailGracefully(hr, "Unable to retrieve Extended Rights search path from path object");

    // Start a thread to enumerate the schema classes
    m_hClassThread = CreateThread(NULL,
                                  0,
                                  SchemaClassThread,
                                  this,
                                  0,
                                  &dwThreadID);

    // Start a thread to enumerate the schema properties
    m_hPropertyThread = CreateThread(NULL,
                                     0,
                                     SchemaPropertyThread,
                                     this,
                                     0,
                                     &dwThreadID);
exit_gracefully:

    VariantClear(&var);
    DoRelease(pRootDSE);
    DoRelease(pPath);
    SysFreeString(strRootDSEPath);

    TraceLeaveVoid();
}


CSchemaCache::~CSchemaCache()
{
    HANDLE ahWait[2];
    DWORD cWait = 0;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::~CSchemaCache");

    if (m_hClassThread != NULL)
    {
        ahWait[cWait] = m_hClassThread;
        cWait++;
    }

    if (m_hPropertyThread != NULL)
    {
        ahWait[cWait] = m_hPropertyThread;
        cWait++;
    }

    if (cWait > 0)
    {
        WaitForMultipleObjects(cWait,
                               ahWait,
                               TRUE,
                               INFINITE);
        do
        {
            cWait--;
            CloseHandle(ahWait[cWait]);
        }
        while (cWait > 0);
    }

    SysFreeString(m_strSchemaSearchPath);
    SysFreeString(m_strERSearchPath);
    SysFreeString(m_strFilterFile);
    DeleteCriticalSection(&m_AccessListCritSec);

    DestroyDPA(m_hClassCache);
    DestroyDPA(m_hPropertyCache);
    DestroyDPA(m_hAccessList);

    if (m_pInheritTypeArray != NULL)
        LocalFree(m_pInheritTypeArray);

    TraceMsg("CSchemaCache::~CSchemaCache exiting");
    TraceLeaveVoid();
}


LPCWSTR
CSchemaCache::GetClassName(LPCGUID pguidObjectType)
{
    LPCWSTR pszLdapName = NULL;
    PID_CACHE_ENTRY pCacheEntry;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::GetClassName");

    pCacheEntry = LookupClass(pguidObjectType);

    if (pCacheEntry != NULL)
        pszLdapName = pCacheEntry->szLdapName;

    TraceLeaveValue(pszLdapName);
}


HRESULT
CSchemaCache::GetInheritTypes(LPCGUID pguidObjectType,
                              DWORD dwFlags,
                              PSI_INHERIT_TYPE *ppInheritTypes,
                              ULONG *pcInheritTypes)
{
    // We're going to find the inherit type array corresponding to the passed-in
    // object type - pInheritTypeArray will point to it!
    PINHERIT_TYPE_ARRAY pInheritTypeArray;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::GetInheritTypes");
    TraceAssert(ppInheritTypes != NULL);
    TraceAssert(pcInheritTypes != NULL);

    *pcInheritTypes = 0;
    *ppInheritTypes = NULL;

    // If the filter state is changing, free everything
    if (m_pInheritTypeArray &&
        (m_pInheritTypeArray->dwFlags & SCHEMA_NO_FILTER) != (dwFlags & SCHEMA_NO_FILTER))
    {
        LocalFree(m_pInheritTypeArray);
        m_pInheritTypeArray = NULL;
    }

    // Build m_pInheritTypeArray if necessary
    if (m_pInheritTypeArray == NULL)
    {
        BuildInheritTypeArray(dwFlags);
    }

    // Return m_pInheritTypeArray if we have it, otherwise
    // fall back on the static types
    if (m_pInheritTypeArray)
    {
        *pcInheritTypes = m_pInheritTypeArray->cInheritTypes;
        *ppInheritTypes = m_pInheritTypeArray->aInheritType;
    }
    else
    {
        TraceMsg("Returning default inherit information");
        *ppInheritTypes = g_siDSInheritTypes;
        *pcInheritTypes = ARRAYSIZE(g_siDSInheritTypes);
    }

    TraceLeaveResult(S_OK); // always succeed
}


HRESULT
CSchemaCache::GetAccessRights(LPCGUID pguidObjectType,
                              LPCWSTR pszClassName,
                              LPCWSTR pszSchemaPath,
                              DWORD dwFlags,
                              PSI_ACCESS *ppAccesses,
                              ULONG *pcAccesses,
                              ULONG *piDefaultAccess)
{
    HRESULT hr = S_OK;
    PACCESS_ARRAY pAccessArray = NULL;
    HCURSOR hcur;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetAccessRights");
    TraceAssert(ppAccesses != NULL);
    TraceAssert(pcAccesses != NULL);
    TraceAssert(piDefaultAccess != NULL);

    *ppAccesses = NULL;
    *pcAccesses = 0;
    *piDefaultAccess = 0;

    //
    // If the SCHEMA_COMMON_PERM flag is on, just return the permissions
    // that are common to all DS objects (including containers).
    //
    if (dwFlags & SCHEMA_COMMON_PERM)
    {
        *ppAccesses = g_siDSAccesses;
        *pcAccesses = ARRAYSIZE(g_siDSAccesses);
        *piDefaultAccess = g_iDSDefAccess;

        TraceLeaveResult(S_OK);
    }

    TraceAssert(pguidObjectType != NULL);
    //TraceAssert(!IsEqualGUID(*pguidObjectType, GUID_NULL));

    //
    // See if we've already got it in the list
    //
    EnterCriticalSection(&m_AccessListCritSec);
    if (m_hAccessList != NULL)
    {
        UINT cItems = DPA_GetPtrCount(m_hAccessList);

        while (cItems > 0)
        {
            pAccessArray = (PACCESS_ARRAY)DPA_FastGetPtr(m_hAccessList, --cItems);

             if ((pAccessArray->dwFlags & (SI_ADVANCED | SI_EDIT_PROPERTIES | SCHEMA_NO_FILTER))
                           == (dwFlags & (SI_ADVANCED | SI_EDIT_PROPERTIES | SCHEMA_NO_FILTER))
                && IsEqualGUID(pAccessArray->guidObjectType, *pguidObjectType))
            {
                // Found a match
                ExitGracefully(hr, S_OK, "Returning existing access array");
            }
        }
    }
    pAccessArray = NULL;

    //
    // Nope, build it
    //
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (dwFlags & SI_EDIT_PROPERTIES)
        hr = BuildPropertyAccessArray(pguidObjectType,
                                      pszClassName,
                                      pszSchemaPath,
                                      dwFlags,
                                      &pAccessArray);
    else
        hr = BuildAccessArray(pguidObjectType,
                              pszClassName,
                              pszSchemaPath,
                              dwFlags,
                              &pAccessArray);
    if (SUCCEEDED(hr))
    {
        // Save it in the list
        if (m_hAccessList == NULL)
            m_hAccessList = DPA_Create(4);

        DPA_AppendPtr(m_hAccessList, pAccessArray);
    }

    SetCursor(hcur);

exit_gracefully:

    LeaveCriticalSection(&m_AccessListCritSec);

    if (pAccessArray != NULL)
    {
        *ppAccesses = pAccessArray->aAccess;
        *pcAccesses = pAccessArray->cAccesses;
        *piDefaultAccess = pAccessArray->iDefaultAccess;
    }

    TraceLeaveResult(hr);
}


PID_CACHE_ENTRY
CSchemaCache::LookupID(HDPA hCache, LPCWSTR pszLdapName)
{
    PID_CACHE_ENTRY pCacheEntry = NULL;
    int iEntry;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::LookupID");
    TraceAssert(hCache != NULL);
    TraceAssert(pszLdapName != NULL && *pszLdapName);

    iEntry = DPA_Search(hCache,
                        NULL,
                        0,
                        Schema_CompareLdapName,
                        (LPARAM)pszLdapName,
                        DPAS_SORTED);

    if (iEntry != -1)
        pCacheEntry = (PID_CACHE_ENTRY)DPA_FastGetPtr(hCache, iEntry);

    TraceLeaveValue(pCacheEntry);
}


PID_CACHE_ENTRY
CSchemaCache::LookupClass(LPCGUID pguidObjectType)
{
    PID_CACHE_ENTRY pCacheEntry = NULL;
    HRESULT hr;
    UINT cItems;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::LookupClass");
    TraceAssert(pguidObjectType != NULL);
    TraceAssert(!IsEqualGUID(*pguidObjectType, GUID_NULL));

    hr = WaitOnClassThread();
    FailGracefully(hr, "Class cache unavailable");

    TraceAssert(m_hClassCache != NULL);

    cItems = DPA_GetPtrCount(m_hClassCache);

    while (cItems > 0)
    {
        PID_CACHE_ENTRY pTemp = (PID_CACHE_ENTRY)DPA_FastGetPtr(m_hClassCache, --cItems);

        if (IsEqualGUID(*pguidObjectType, pTemp->guid))
        {
            pCacheEntry = pTemp;
            break;
        }
    }

exit_gracefully:

    TraceLeaveValue(pCacheEntry);
}


LPCGUID
CSchemaCache::LookupClassID(LPCWSTR pszClass)
{
    LPCGUID pID = NULL;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::LookupClassID");
    TraceAssert(pszClass != NULL);

    if (SUCCEEDED(WaitOnClassThread()))
    {
        TraceAssert(m_hClassCache != NULL);
        PID_CACHE_ENTRY pCacheEntry = LookupID(m_hClassCache, pszClass);
        if (pCacheEntry)
            pID = &pCacheEntry->guid;
    }

    TraceLeaveValue(pID);
}


LPCGUID
CSchemaCache::LookupPropertyID(LPCWSTR pszProperty)
{
    LPCGUID pID = NULL;

    TraceEnter(TRACE_SCHEMAPROP, "CSchemaCache::LookupPropertyID");
    TraceAssert(pszProperty != NULL);

    if (SUCCEEDED(WaitOnPropertyThread()))
    {
        TraceAssert(m_hPropertyCache != NULL);
        PID_CACHE_ENTRY pCacheEntry = LookupID(m_hPropertyCache, pszProperty);
        if (pCacheEntry)
            pID = &pCacheEntry->guid;
    }

    TraceLeaveValue(pID);
}


WCHAR const c_szDsHeuristics[] = L"dSHeuristics";

int
CSchemaCache::GetListObjectEnforced(void)
{
    int nListObjectEnforced = 0;    // Assume "not enforced"
    HRESULT hr;
    IADsPathname *pPath = NULL;
    const LPWSTR aszServicePath[] =
    {
        L"CN=Services",
        L"CN=Windows NT",
        L"CN=Directory Service",
    };
    BSTR strServicePath = NULL;
    IDirectoryObject *pDirectoryService = NULL;
    LPWSTR pszDsHeuristics = (LPWSTR)c_szDsHeuristics;
    PADS_ATTR_INFO pAttributeInfo = NULL;
    DWORD dwAttributesReturned;
    LPWSTR pszHeuristicString;
    int i;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetListObjectEnforced");

    // Create a path object for manipulating ADS paths
    hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pPath);
    FailGracefully(hr, "Unable to create ADsPathname object");

    hr = pPath->Set(m_strERSearchPath, ADS_SETTYPE_FULL);
    FailGracefully(hr, "Unable to initialize ADsPathname object");

    hr = pPath->RemoveLeafElement();
    for (i = 0; i < ARRAYSIZE(aszServicePath); i++)
    {
        hr = pPath->AddLeafElement(aszServicePath[i]);
        FailGracefully(hr, "Unable to build path to 'Directory Service' object");
    }

    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &strServicePath);
    FailGracefully(hr, "Unable to build path to 'Directory Service' object");

    hr = ADsGetObject(strServicePath,
                      IID_IDirectoryObject,
                      (LPVOID*)&pDirectoryService);
    FailGracefully(hr, "Unable to bind to 'Directory Service' object for heuristics");

    hr = pDirectoryService->GetObjectAttributes(&pszDsHeuristics,
                                                1,
                                                &pAttributeInfo,
                                                &dwAttributesReturned);
    if (!pAttributeInfo)
        ExitGracefully(hr, hr, "GetObjectAttributes failed to read dSHeuristics property");

    TraceAssert(ADSTYPE_DN_STRING <= pAttributeInfo->dwADsType);
    TraceAssert(ADSTYPE_NUMERIC_STRING >= pAttributeInfo->dwADsType);
    TraceAssert(1 == pAttributeInfo->dwNumValues);

    pszHeuristicString = pAttributeInfo->pADsValues->NumericString;
    if (pszHeuristicString &&
        lstrlenW(pszHeuristicString) > 2 &&
        L'0' != pszHeuristicString[2])
    {
        nListObjectEnforced = 1;
    }

exit_gracefully:

    if (pAttributeInfo)
        FreeADsMem(pAttributeInfo);

    DoRelease(pDirectoryService);
    DoRelease(pPath);

    SysFreeString(strServicePath);

    TraceLeaveValue(nListObjectEnforced);
}

BOOL
CSchemaCache::HideListObjectAccess(void)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::HideListObjectAccess");

    if (-1 == m_nDsListObjectEnforced)
    {
        m_nDsListObjectEnforced = GetListObjectEnforced();
    }

    TraceLeaveValue(0 == m_nDsListObjectEnforced);
}


#define ACCESS_LENGTH_0 (sizeof(SI_ACCESS) + MAX_TYPENAME_LENGTH * sizeof(WCHAR))
#define ACCESS_LENGTH_1 (sizeof(SI_ACCESS) + sizeof(GUID) + MAX_TYPENAME_LENGTH * sizeof(WCHAR))
#define ACCESS_LENGTH_2 (3 * sizeof(SI_ACCESS) + sizeof(GUID) + 3 * MAX_TYPENAME_LENGTH * sizeof(WCHAR))

HRESULT
CSchemaCache::BuildAccessArray(LPCGUID pguidObjectType,
                               LPCWSTR pszClassName,
                               LPCWSTR pszSchemaPath,
                               DWORD dwFlags,
                               PACCESS_ARRAY *ppAccessArray)
{
    HRESULT hr = S_OK;
    BOOL bContainer = FALSE;
    VARIANT varContainment = {0};
    UINT cChildClasses = 0;
    DWORD dwBufferLength;
    PACCESS_ARRAY pAccessArray = NULL;
    UINT cMaxAccesses;
    PSI_ACCESS pNewAccess;
    PBYTE pData;
    UINT i;
    HDPA hExtRightList = NULL;
    ULONG cBaseRights = ARRAYSIZE(g_siDSAccesses);
    ULONG cExtendedRights = 0;
    HDPA hClassList = NULL;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::BuildAccessArray");
    TraceAssert(pguidObjectType != NULL);
    TraceAssert(ppAccessArray != NULL);

    *ppAccessArray = NULL;

    //
    // Lookup the name of this class
    //
    if (pszClassName == NULL)
        pszClassName = GetClassName(pguidObjectType);

    //
    // Get the list of Extended Rights for this page
    //
    if (pguidObjectType &&
        SUCCEEDED(Schema_GetExtendedRightsForClass(m_strERSearchPath,
                                                   pguidObjectType,
                                                   dwFlags,
                                                   &hExtRightList)))
    {
        TraceAssert(NULL != hExtRightList);
        cExtendedRights = DPA_GetPtrCount(hExtRightList);
    }

    //
    // Figure out if the object is a container by getting the list of child
    // classes. These will be added to the list if the advanced page is asking
    // for the access rights.
    //
    if (pszClassName != NULL)
    {
        IADsClass *pDsClass;

        // Get the schema object for this class
        if (SUCCEEDED(Schema_BindToObject(pszSchemaPath,
                                          pszClassName,
                                          IID_IADsClass,
                                          (LPVOID*)&pDsClass)))
        {
            // Get the list of possible child classes
            if (SUCCEEDED(pDsClass->get_Containment(&varContainment)))
            {
                if (V_VT(&varContainment) == (VT_ARRAY | VT_VARIANT))
                {
                    LPSAFEARRAY psa = V_ARRAY(&varContainment);
                    TraceAssert(psa && psa->cDims == 1);
                    if (psa->rgsabound[0].cElements > 0)
                        bContainer = TRUE;
                }
                else if (V_VT(&varContainment) == VT_BSTR) // single entry
                {
                    TraceAssert(V_BSTR(&varContainment));
                    bContainer = TRUE;
                }

                // Only want to add the individual "Create child type" & "Delete
                // child type" entries if this is for the advanced page.
                // (Requires the schema class enumeration thread to complete first,
                // and it's usually not done yet the first time we get here.)
                if (bContainer && (dwFlags & SI_ADVANCED))
                {
                    hClassList = DPA_Create(8);
                    if (hClassList)
                    {
                        IDsDisplaySpecifier *pDisplaySpec = NULL;

                        // Get the display specifier object
                        CoCreateInstance(CLSID_DsDisplaySpecifier,
                                         NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_IDsDisplaySpecifier,
                                         (void**)&pDisplaySpec);

                        // Filter the list & get display names
                        EnumVariantList(&varContainment,
                                        hClassList,
                                        SCHEMA_CLASS | (dwFlags & SCHEMA_NO_FILTER),
                                        pDisplaySpec);

                        cChildClasses = DPA_GetPtrCount(hClassList);
                        DoRelease(pDisplaySpec);
                    }
                }
            }
            DoRelease(pDsClass);
        }
    }

    if (!bContainer)
        cBaseRights -= 2; // skip DS_CREATE_CHILD and DS_DELETE_CHILD

    cMaxAccesses = cBaseRights + cExtendedRights + 3 * cChildClasses;
    TraceAssert(cMaxAccesses > 0);

    //
    // Allocate a buffer for the access array
    //
    dwBufferLength = sizeof(ACCESS_ARRAY) - sizeof(SI_ACCESS)
                        + cBaseRights * sizeof(SI_ACCESS)
                        + cExtendedRights * ACCESS_LENGTH_1
                        + cChildClasses * ACCESS_LENGTH_2;
    pAccessArray = (PACCESS_ARRAY)LocalAlloc(LPTR, dwBufferLength);
    if (pAccessArray == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pAccessArray->guidObjectType = *pguidObjectType;
    pAccessArray->dwFlags = (dwFlags & (SI_ADVANCED | SCHEMA_NO_FILTER));
    pAccessArray->cAccesses = 0;
    pAccessArray->iDefaultAccess = g_iDSDefAccess;

    pNewAccess = pAccessArray->aAccess;
    pData = (PBYTE)(pNewAccess + cMaxAccesses);

    //
    // Add normal entries (no GUIDs)
    //
    CopyMemory(pNewAccess, g_siDSAccesses, cBaseRights * sizeof(SI_ACCESS));
    pNewAccess += cBaseRights;
    pAccessArray->cAccesses += cBaseRights;

    if (HideListObjectAccess())
    {
        pAccessArray->aAccess[g_iDSRead].mask &= ~ACTRL_DS_LIST_OBJECT;
        pAccessArray->aAccess[g_iDSListObject].dwFlags = 0;
    }

    //
    // Add entries for Extended Rights
    //
    for (i = 0; i < cExtendedRights; i++)
    {
        PER_ENTRY pER = (PER_ENTRY)DPA_FastGetPtr(hExtRightList, i);

        pNewAccess->mask = pER->mask;
        pNewAccess->dwFlags = pER->dwFlags;

        pNewAccess->pguid = (LPCGUID)pData;
        *(LPGUID)pData = pER->guid;
        pData += sizeof(GUID);

        pNewAccess->pszName = (LPCWSTR)pData;
        lstrcpynW((LPWSTR)pData, pER->szName, MAX_TYPENAME_LENGTH);
        pData += StringByteSize(pNewAccess->pszName);

        pNewAccess++;
        pAccessArray->cAccesses++;
    }

    //
    // Add entries for creating & deleting child objects
    //
    if (cChildClasses > 0)
    {
        TraceAssert(NULL != hClassList);

        // Sort by display name
        DPA_Sort(hClassList, Schema_CompareTempDisplayName, 0);
        pAccessArray->cAccesses += AddTempListToAccessList(hClassList,
                                                           &pNewAccess,
                                                           &pData,
                                                           SCHEMA_CLASS | (dwFlags & SCHEMA_NO_FILTER));
    }

exit_gracefully:

    VariantClear(&varContainment);

    DestroyDPA(hExtRightList);
    DestroyDPA(hClassList);

    if (FAILED(hr) && pAccessArray)
    {
        LocalFree(pAccessArray);
        pAccessArray = NULL;
    }

    *ppAccessArray = pAccessArray;

    TraceLeaveResult(hr);
}


HRESULT
CSchemaCache::BuildPropertyAccessArray(LPCGUID pguidObjectType,
                                       LPCWSTR pszClassName,
                                       LPCWSTR pszSchemaPath,
                                       DWORD dwFlags,
                                       PACCESS_ARRAY *ppAccessArray)
{
    HRESULT hr;
    IADsClass *pDsClass = NULL;
    VARIANT varMandatoryProperties = {0};
    VARIANT varOptionalProperties = {0};
    LPSAFEARRAY psa;
    UINT cProperties = 0;
    DWORD dwBufferLength;
    PACCESS_ARRAY pAccessArray = NULL;
    UINT cMaxAccesses;
    PSI_ACCESS pNewAccess;
    PBYTE pData;
    UINT i;
    HDPA hExtRightList = NULL;
    ULONG cExtendedRights = 0;
    IDsDisplaySpecifier *pDisplaySpec = NULL;
    HDPA hPropertyList = NULL;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::BuildPropertyAccessArray");
    TraceAssert(pguidObjectType != NULL);
    TraceAssert(ppAccessArray != NULL);

    *ppAccessArray = NULL;

    // Initial length
    // (includes 2 SI_ACCESS entries for "Read/Write All Properties")
    dwBufferLength = sizeof(ACCESS_ARRAY) + sizeof(SI_ACCESS);

    //
    // Get the schema object for this class
    //
    if (pszClassName == NULL)
        pszClassName = GetClassName(pguidObjectType);

    if (pszClassName == NULL)
        ExitGracefully(hr, E_UNEXPECTED, "Unknown child object GUID");

    hr = Schema_BindToObject(pszSchemaPath,
                             pszClassName,
                             IID_IADsClass,
                             (LPVOID*)&pDsClass);
    FailGracefully(hr, "Unable to create schema object");

    //
    // Get the list of Extended Rights for this page
    //
    if (pguidObjectType &&
        SUCCEEDED(Schema_GetExtendedRightsForClass(m_strERSearchPath,
                                                   pguidObjectType,
                                                   dwFlags,
                                                   &hExtRightList)))
    {
        TraceAssert(NULL != hExtRightList);
        cExtendedRights = DPA_GetPtrCount(hExtRightList);
        dwBufferLength += cExtendedRights * ACCESS_LENGTH_1;
    }

    // Get the display specifier object
    CoCreateInstance(CLSID_DsDisplaySpecifier,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IDsDisplaySpecifier,
                     (void**)&pDisplaySpec);

    hPropertyList = DPA_Create(8);
    if (!hPropertyList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create DPA");

    //
    // Get mandatory and optional property lists
    //
    if (SUCCEEDED(pDsClass->get_MandatoryProperties(&varMandatoryProperties)))
    {
        EnumVariantList(&varMandatoryProperties,
                        hPropertyList,
                        (dwFlags & SCHEMA_NO_FILTER),
                        pDisplaySpec,
                        pszClassName);
    }
    if (SUCCEEDED(pDsClass->get_OptionalProperties(&varOptionalProperties)))
    {
        EnumVariantList(&varOptionalProperties,
                        hPropertyList,
                        (dwFlags & SCHEMA_NO_FILTER),
                        pDisplaySpec,
                        pszClassName);
    }
    cProperties = DPA_GetPtrCount(hPropertyList);
    dwBufferLength += cProperties * ACCESS_LENGTH_2;

    cMaxAccesses = 3 * cProperties + 2 + cExtendedRights;
    if (cMaxAccesses == 0)
        ExitGracefully(hr, E_FAIL, "No properties found");

    //
    // Allocate a buffer for the access array
    //
    pAccessArray = (PACCESS_ARRAY)LocalAlloc(LPTR, dwBufferLength);

    if (pAccessArray == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pAccessArray->guidObjectType = *pguidObjectType;
    pAccessArray->dwFlags = (dwFlags & (SI_ADVANCED | SI_EDIT_PROPERTIES | SCHEMA_NO_FILTER));
    pAccessArray->cAccesses = 0;
    pAccessArray->iDefaultAccess = 0;

    pNewAccess = pAccessArray->aAccess;
    pData = (PBYTE)(pNewAccess + cMaxAccesses);

    //
    // Add "Read All Properties" and "Write All Properties"
    //
    CopyMemory(pNewAccess, &g_siDSAccesses[g_iDSProperties], 2 * sizeof(SI_ACCESS));
    pNewAccess += 2;
    pAccessArray->cAccesses += 2;

    //
    // Add entries for Extended Rights
    //
    for (i = 0; i < cExtendedRights; i++)
    {
        PER_ENTRY pER = (PER_ENTRY)DPA_FastGetPtr(hExtRightList, i);

        pNewAccess->mask = pER->mask;
        pNewAccess->dwFlags = pER->dwFlags;

        pNewAccess->pguid = (LPCGUID)pData;
        *(LPGUID)pData = pER->guid;
        pData += sizeof(GUID);

        pNewAccess->pszName = (LPCWSTR)pData;
        lstrcpynW((LPWSTR)pData, pER->szName, MAX_TYPENAME_LENGTH);
        pData += StringByteSize(pNewAccess->pszName);

        pNewAccess++;
        pAccessArray->cAccesses++;
    }

    //
    // Add property entries
    //
    if (cProperties > 0)
    {
        // Sort by display name
        DPA_Sort(hPropertyList, Schema_CompareTempDisplayName, 0);
        pAccessArray->cAccesses += AddTempListToAccessList(hPropertyList,
                                                           &pNewAccess,
                                                           &pData,
                                                           (dwFlags & SCHEMA_NO_FILTER));
    }

exit_gracefully:

    VariantClear(&varMandatoryProperties);
    VariantClear(&varOptionalProperties);

    DestroyDPA(hExtRightList);
    DestroyDPA(hPropertyList);

    DoRelease(pDsClass);
    DoRelease(pDisplaySpec);

    if (FAILED(hr) && pAccessArray)
    {
        LocalFree(pAccessArray);
        pAccessArray = NULL;
    }

    *ppAccessArray = pAccessArray;

    TraceLeaveResult(hr);
}


HRESULT
CSchemaCache::EnumVariantList(LPVARIANT pvarList,
                              HDPA hTempList,
                              DWORD dwFlags,
                              IDsDisplaySpecifier *pDisplaySpec,
                              LPCWSTR pszPropertyClass)
{
    HRESULT hr = S_OK;
    LPVARIANT pvarItem = NULL;
    int cItems;
    BOOL bSafeArrayLocked = FALSE;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::EnumVariantList");
    TraceAssert(pvarList != NULL);
    TraceAssert(hTempList != NULL);

    if (V_VT(pvarList) == (VT_ARRAY | VT_VARIANT))
    {
        hr = SafeArrayAccessData(V_ARRAY(pvarList), (LPVOID*)&pvarItem);
        FailGracefully(hr, "Unable to access SafeArray");
        bSafeArrayLocked = TRUE;
        cItems = V_ARRAY(pvarList)->rgsabound[0].cElements;
    }
    else if (V_VT(pvarList) == VT_BSTR) // Single entry in list
    {
        pvarItem = pvarList;
        cItems = 1;
    }
    else
    {
        // Unknown format
        ExitGracefully(hr, E_INVALIDARG, "Unexpected VARIANT type");
    }

    if (NULL == m_strFilterFile)
        m_strFilterFile = GetFilterFilePath();

    // Enumerate the variant list and get information about each
    // (filter, guid, display name)
    for ( ; cItems > 0; pvarItem++, cItems--)
    {
        LPWSTR pszItem;
        DWORD dwFilter = 0;
        LPCGUID pguid;
        WCHAR wszDisplayName[MAX_PATH];
        PTEMP_INFO pti;

        TraceAssert(V_VT(pvarItem) == VT_BSTR);
        pszItem = V_BSTR(pvarItem);

        // Check for nonexistent or empty strings
        if (!pszItem || !*pszItem)
            continue;

        if (m_strFilterFile && !(dwFlags & SCHEMA_NO_FILTER))
        {
            if (dwFlags & SCHEMA_CLASS)
            {
                dwFilter = GetPrivateProfileIntW(pszItem,
                                                 c_szClassKey,
                                                 0,
                                                 m_strFilterFile);
            }
            else if (pszPropertyClass)
            {
                dwFilter = GetPrivateProfileIntW(pszPropertyClass,
                                                 pszItem,
                                                 0,
                                                 m_strFilterFile);
            }
        }

        // Note that IDC_CLASS_NO_CREATE == IDC_PROP_NO_READ
        // and IDC_CLASS_NO_DELETE == IDC_PROP_NO_WRITE
        dwFilter &= (IDC_CLASS_NO_CREATE | IDC_CLASS_NO_DELETE);
        if ((IDC_CLASS_NO_CREATE | IDC_CLASS_NO_DELETE) == dwFilter)
            continue;

        if (dwFlags & SCHEMA_CLASS)
            pguid = LookupClassID(pszItem);
        else
            pguid = LookupPropertyID(pszItem);

        if (pguid == NULL)
            continue;

        wszDisplayName[0] = L'\0';

        if (pDisplaySpec)
        {
            if (dwFlags & SCHEMA_CLASS)
            {
                pDisplaySpec->GetFriendlyClassName(pszItem,
                                                   wszDisplayName,
                                                   ARRAYSIZE(wszDisplayName));
            }
            else if (pszPropertyClass)
            {
                pDisplaySpec->GetFriendlyAttributeName(pszPropertyClass,
                                                       pszItem,
                                                       wszDisplayName,
                                                       ARRAYSIZE(wszDisplayName));
            }
        }

        // Remember what we've got so far
        pti = (PTEMP_INFO)LocalAlloc(LPTR, sizeof(TEMP_INFO) + sizeof(WCHAR)*lstrlenW(wszDisplayName));
        if (pti)
        {
            pti->pguid = pguid;
            pti->dwFilter = dwFilter;
            pti->pszLdapName = pszItem;
            lstrcpyW(pti->szDisplayName, wszDisplayName);
            DPA_AppendPtr(hTempList, pti);
        }
    }

exit_gracefully:

    if (bSafeArrayLocked)
        SafeArrayUnaccessData(V_ARRAY(pvarList));

    TraceLeaveResult(hr);
}


UINT
CSchemaCache::AddTempListToAccessList(HDPA hTempList,
                                      PSI_ACCESS *ppAccess,
                                      LPBYTE *ppData,
                                      DWORD dwFlags)
{
    UINT cTotalEntries = 0;
    int cItems;
    DWORD dwAccess1;
    DWORD dwAccess2;
    DWORD dwAccessFlags;
    WCHAR szFmt1[MAX_TYPENAME_LENGTH];
    WCHAR szFmt2[MAX_TYPENAME_LENGTH];
    WCHAR szFmt3[MAX_TYPENAME_LENGTH];

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::AddTempListToAccessList");
    TraceAssert(ppAccess != NULL);
    TraceAssert(ppData != NULL);

    // Do we have anything?
    cItems = DPA_GetPtrCount(hTempList);
    if (0 == cItems)
        ExitGracefully(cTotalEntries, 0, "empty list");

    if (dwFlags & SCHEMA_CLASS)
    {
        dwAccess1 = ACTRL_DS_CREATE_CHILD;
        dwAccess2 = ACTRL_DS_DELETE_CHILD;
        dwAccessFlags = SI_ACCESS_SPECIFIC;
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_CREATE_CHILD_TYPE, szFmt1, ARRAYSIZE(szFmt1));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_DELETE_CHILD_TYPE, szFmt2, ARRAYSIZE(szFmt2));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_CREATEDELETE_TYPE, szFmt3, ARRAYSIZE(szFmt3));
    }
    else
    {
        dwAccess1 = ACTRL_DS_READ_PROP;
        dwAccess2 = ACTRL_DS_WRITE_PROP;
        dwAccessFlags = SI_ACCESS_PROPERTY;
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_READ_PROP_TYPE,  szFmt1, ARRAYSIZE(szFmt1));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_WRITE_PROP_TYPE, szFmt2, ARRAYSIZE(szFmt2));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_READWRITE_TYPE,  szFmt3, ARRAYSIZE(szFmt3));
    }

    // Enumerate the list and make up to 2 entries for each
    while (cItems > 0)
    {
        PTEMP_INFO pti;
        LPCGUID pguid;
        LPBYTE pData;
        PSI_ACCESS pNewAccess;
        LPCWSTR pszDisplayName;
        int cch;
        DWORD dwAccess3;

        pti = (PTEMP_INFO)DPA_FastGetPtr(hTempList, --cItems);
        if (!pti)
            continue;

        pData = *ppData;
        pNewAccess = *ppAccess;

        if (pti->szDisplayName[0])
            pszDisplayName = pti->szDisplayName;
        else
            pszDisplayName = pti->pszLdapName;

        // Store the GUID and set pguid to point to the copy
        *(LPGUID)pData = *pti->pguid;
        pguid = (LPCGUID)pData;
        pData += sizeof(GUID);

        dwAccess3 = 0;
        if (!(pti->dwFilter & IDC_CLASS_NO_CREATE))
        {
            pNewAccess->mask = dwAccess1;
            pNewAccess->dwFlags = dwAccessFlags;
            pNewAccess->pguid = pguid;

            pNewAccess->pszName = (LPCWSTR)pData;
            cch = wsprintfW((LPWSTR)pData, szFmt1, pszDisplayName);
            pData += (cch + 1) * sizeof(WCHAR);

            cTotalEntries++;
            pNewAccess++;

            dwAccess3 |= dwAccess1;
        }

        if (!(pti->dwFilter & IDC_CLASS_NO_DELETE))
        {
            pNewAccess->mask = dwAccess2;
            pNewAccess->dwFlags = dwAccessFlags;
            pNewAccess->pguid = pguid;

            pNewAccess->pszName = (LPCWSTR)pData;
            cch = wsprintfW((LPWSTR)pData, szFmt2, pszDisplayName);
            pData += (cch + 1) * sizeof(WCHAR);

            cTotalEntries++;
            pNewAccess++;

            dwAccess3 |= dwAccess2;
        }

        if (dwAccess3 == (dwAccess1 | dwAccess2))
        {
            // Add a hidden entry for
            //     "Read/write <prop>"
            // or
            //     "Create/delete <child>"
            pNewAccess->mask = dwAccess3;
            // dwFlags = 0 means it will never show as a checkbox, but it
            // may be used for the name displayed on the Advanced page.
            pNewAccess->dwFlags = 0;
            pNewAccess->pguid = pguid;

            pNewAccess->pszName = (LPCWSTR)pData;
            cch = wsprintfW((LPWSTR)pData, szFmt3, pszDisplayName);
            pData += (cch + 1) * sizeof(WCHAR);

            cTotalEntries++;
            pNewAccess++;
        }

        if (*ppAccess != pNewAccess)
        {
            *ppAccess = pNewAccess; // move past new entries
            *ppData = pData;
        }
    }

exit_gracefully:

    TraceLeaveValue(cTotalEntries);
}


DWORD WINAPI
CSchemaCache::SchemaClassThread(LPVOID pvThreadData)
{
    PSCHEMACACHE pCache;

    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);
    InterlockedIncrement(&GLOBAL_REFCOUNT);

    pCache = (PSCHEMACACHE)pvThreadData;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::SchemaClassThread");
    TraceAssert(pCache != NULL);
    TraceAssert(pCache->m_strSchemaSearchPath != NULL);

#if DBG
    DWORD dwTime = GetTickCount();
#endif

    ThreadCoInitialize();

    pCache->m_hrClassResult = Schema_Search(pCache->m_strSchemaSearchPath,
                                            c_szClassFilter,
                                            &pCache->m_hClassCache);

    ThreadCoUninitialize();

#if DBG
    Trace((TEXT("SchemaClassThread complete, elapsed time: %d ms"), GetTickCount() - dwTime));
#endif

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    FreeLibraryAndExitThread(hInstThisDll, 0);
    return 0;
}


DWORD WINAPI
CSchemaCache::SchemaPropertyThread(LPVOID pvThreadData)
{
    PSCHEMACACHE pCache;

    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);
    InterlockedIncrement(&GLOBAL_REFCOUNT);

    pCache = (PSCHEMACACHE)pvThreadData;

    TraceEnter(TRACE_SCHEMAPROP, "CSchemaCache::SchemaPropertyThread");
    TraceAssert(pCache != NULL);
    TraceAssert(pCache->m_strSchemaSearchPath != NULL);

#if DBG
    DWORD dwTime = GetTickCount();
#endif

    ThreadCoInitialize();

    pCache->m_hrPropertyResult = Schema_Search(pCache->m_strSchemaSearchPath,
                                               c_szPropertyFilter,
                                               &pCache->m_hPropertyCache);

    ThreadCoUninitialize();

#if DBG
    Trace((TEXT("SchemaPropertyThread complete, elapsed time: %d ms"), GetTickCount() - dwTime));
#endif

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    FreeLibraryAndExitThread(hInstThisDll, 0);
    return 0;
}


HRESULT
CSchemaCache::BuildInheritTypeArray(DWORD dwFlags)
{
    HRESULT hr = S_OK;
    int cItems;
    DWORD cbNames = 0;
    DWORD dwBufferLength;
    PINHERIT_TYPE_ARRAY pInheritTypeArray = NULL;
    PSI_INHERIT_TYPE pNewInheritType;
    PBYTE pData;
    WCHAR szFormat[MAX_TYPENAME_LENGTH];
    HDPA hTempList = NULL;
    PTEMP_INFO pti;
    IDsDisplaySpecifier *pDisplaySpec = NULL;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::BuildInheritTypeArray");
    TraceAssert(m_pInheritTypeArray == NULL);   // Don't want to build this twice

    if (NULL == m_strFilterFile)
        m_strFilterFile = GetFilterFilePath();

    hr = WaitOnClassThread();
    FailGracefully(hr, "Class cache unavailable");

    cItems = DPA_GetPtrCount(m_hClassCache);
    if (cItems == 0)
        ExitGracefully(hr, E_FAIL, "No schema classes available");

    hTempList = DPA_Create(cItems);
    if (!hTempList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create DPA");

    // Get the display specifier object
    CoCreateInstance(CLSID_DsDisplaySpecifier,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IDsDisplaySpecifier,
                     (void**)&pDisplaySpec);

    // Enumerate child types, apply filtering, and get display names
    while (cItems > 0)
    {
        PID_CACHE_ENTRY pCacheEntry;
        WCHAR wszDisplayName[MAX_PATH];

        pCacheEntry = (PID_CACHE_ENTRY)DPA_FastGetPtr(m_hClassCache, --cItems);

        if (!pCacheEntry)
            continue;
        
        if (m_strFilterFile && !(dwFlags & SCHEMA_NO_FILTER))
        {
            DWORD dwFilter = GetPrivateProfileIntW(pCacheEntry->szLdapName,
                                                   c_szClassKey,
                                                   0,
                                                   m_strFilterFile);
            if (dwFilter & IDC_CLASS_NO_INHERIT)
                continue;
        }

        wszDisplayName[0] = L'\0';

        if (pDisplaySpec)
        {
            pDisplaySpec->GetFriendlyClassName(pCacheEntry->szLdapName,
                                               wszDisplayName,
                                               ARRAYSIZE(wszDisplayName));
        }

        if (L'\0' != wszDisplayName[0])
            cbNames += StringByteSize(wszDisplayName);
        else
            cbNames += StringByteSize(pCacheEntry->szLdapName);

        pti = (PTEMP_INFO)LocalAlloc(LPTR, sizeof(TEMP_INFO) + sizeof(WCHAR)*lstrlenW(wszDisplayName));
        if (pti)
        {
            pti->pguid = &pCacheEntry->guid;
            pti->pszLdapName = pCacheEntry->szLdapName;
            lstrcpyW(pti->szDisplayName, wszDisplayName);
            DPA_AppendPtr(hTempList, pti);
        }
    }

    // Sort by display name
    DPA_Sort(hTempList, Schema_CompareTempDisplayName, 0);

    // Get an accurate count
    cItems = DPA_GetPtrCount(hTempList);

    //
    // Allocate a buffer for the inherit type array
    //
    dwBufferLength = sizeof(INHERIT_TYPE_ARRAY) - sizeof(SI_INHERIT_TYPE)
        + sizeof(g_siDSInheritTypes)
        + cItems * (sizeof(SI_INHERIT_TYPE) + sizeof(GUID) + MAX_TYPENAME_LENGTH*sizeof(WCHAR))
        + cbNames;

    pInheritTypeArray = (PINHERIT_TYPE_ARRAY)LocalAlloc(LPTR, dwBufferLength);
    if (pInheritTypeArray == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pInheritTypeArray->cInheritTypes = ARRAYSIZE(g_siDSInheritTypes);

    pNewInheritType = pInheritTypeArray->aInheritType;
    pData = (PBYTE)(pNewInheritType + pInheritTypeArray->cInheritTypes + cItems);

    // Copy static entries
    CopyMemory(pNewInheritType, g_siDSInheritTypes, sizeof(g_siDSInheritTypes));
    pNewInheritType += ARRAYSIZE(g_siDSInheritTypes);

    // Load format string
    LoadString(GLOBAL_HINSTANCE,
               IDS_DS_INHERIT_TYPE,
               szFormat,
               ARRAYSIZE(szFormat));

    // Enumerate child types and make an entry for each
    while (cItems > 0)
    {
        int cch;
        LPCWSTR pszDisplayName;

        pti = (PTEMP_INFO)DPA_FastGetPtr(hTempList, --cItems);
        if (!pti)
            continue;

        if (pti->szDisplayName[0])
            pszDisplayName = pti->szDisplayName;
        else
            pszDisplayName = pti->pszLdapName;

        pNewInheritType->dwFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

        // The class entry name is the child class name, e.g. "Domain" or "User"
        pNewInheritType->pszName = (LPCWSTR)pData;
        cch = wsprintfW((LPWSTR)pData, szFormat, pszDisplayName);
        pData += (cch + 1) * sizeof(WCHAR);

        pNewInheritType->pguid = (LPCGUID)pData;
        *(LPGUID)pData = *pti->pguid;
        pData += sizeof(GUID);

        pNewInheritType++;
        pInheritTypeArray->cInheritTypes++;
    }

exit_gracefully:

    DoRelease(pDisplaySpec);

    if (SUCCEEDED(hr))
    {
        m_pInheritTypeArray = pInheritTypeArray;
        // Set this master inherit type array's GUID to null
        m_pInheritTypeArray->guidObjectType = GUID_NULL;
        m_pInheritTypeArray->dwFlags = (dwFlags & SCHEMA_NO_FILTER);
    }
    else if (pInheritTypeArray != NULL)
    {
        LocalFree(pInheritTypeArray);
    }

    DestroyDPA(hTempList);

    TraceLeaveResult(hr);
}


HRESULT
Schema_BindToObject(LPCWSTR pszSchemaPath,
                    LPCWSTR pszName,
                    REFIID riid,
                    LPVOID *ppv)
{
    HRESULT hr;
    WCHAR szPath[MAX_PATH];
    UINT nSchemaRootLen;
    WCHAR chTemp;

    TraceEnter(TRACE_SCHEMA, "Schema_BindToObject");
    TraceAssert(pszSchemaPath != NULL);
    TraceAssert(pszName == NULL || *pszName);
    TraceAssert(ppv != NULL);

    if (pszSchemaPath == NULL)
    {
        ExitGracefully(hr, E_INVALIDARG, "No schema path provided");
    }

    nSchemaRootLen = lstrlenW(pszSchemaPath);

    //
    // Build the schema path to this object
    //
    lstrcpynW(szPath, pszSchemaPath, nSchemaRootLen + 1);
    chTemp = szPath[nSchemaRootLen-1];
    if (pszName != NULL)
    {
        // If there is no trailing slash, add it
        if (chTemp != TEXT('/'))
        {
            szPath[nSchemaRootLen] = TEXT('/');
            nSchemaRootLen++;
        }

        // Add the class or property name onto the end
        lstrcpynW(szPath + nSchemaRootLen,
                 pszName,
                 ARRAYSIZE(szPath) - nSchemaRootLen);
    }
    else if (nSchemaRootLen > 0)
    {
        // If there is a trailing slash, remove it
        if (chTemp == TEXT('/'))
            szPath[nSchemaRootLen-1] = TEXT('\0');
    }
    else
    {
        ExitGracefully(hr, E_INVALIDARG, "Empty schema path");
    }

    //
    // Instantiate the schema object
    //
    ThreadCoInitialize();
    hr = ADsOpenObject(szPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       riid,
                       ppv);

exit_gracefully:

    TraceLeaveResult(hr);
}


HRESULT
Schema_GetObjectID(IADs *pObj, LPGUID pGUID)
{
    HRESULT hr;
    VARIANT varID = {0};

    TraceEnter(TRACE_SCHEMA, "Schema_GetObjectID(IADs*)");
    TraceAssert(pObj != NULL);
    TraceAssert(pGUID != NULL && !IsBadWritePtr(pGUID, sizeof(GUID)));

    // Get the "schemaIDGUID" property
    hr = pObj->Get((LPWSTR)c_szSchemaIDGUID, &varID);

    if (SUCCEEDED(hr))
    {
        LPGUID pID = NULL;

        TraceAssert(V_VT(&varID) == (VT_ARRAY | VT_UI1));
        TraceAssert(V_ARRAY(&varID) && varID.parray->cDims == 1);
        TraceAssert(V_ARRAY(&varID)->rgsabound[0].cElements >= sizeof(GUID));

        hr = SafeArrayAccessData(V_ARRAY(&varID), (LPVOID*)&pID);
        if (SUCCEEDED(hr))
        {
            *pGUID = *pID;
            SafeArrayUnaccessData(V_ARRAY(&varID));
        }
        VariantClear(&varID);
    }

    TraceLeaveResult(hr);
}


HRESULT
Schema_Search(LPWSTR pszSchemaSearchPath,
              LPCWSTR pszFilter,
              HDPA *phCache)
{
    HRESULT hr = S_OK;
    HDPA hCache = NULL;
    IDirectorySearch *pSchemaSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[3];
    const LPCWSTR pProperties[] =
    {
        c_szLDAPDisplayName,            // "lDAPDisplayName"
        c_szSchemaIDGUID,               // "schemaIDGUID"
    };

    TraceEnter(lstrcmp(pszFilter, c_szPropertyFilter) ? TRACE_SCHEMACLASS : TRACE_SCHEMAPROP, "Schema_Search");
    TraceAssert(pszSchemaSearchPath != NULL);
    TraceAssert(phCache != NULL);

    //
    // Create DPA if necessary
    //
    if (*phCache == NULL)
        *phCache = DPA_Create(100);

    if (*phCache == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create failed");

    hCache = *phCache;

    // Get the schema search object
    hr = ADsOpenObject(pszSchemaSearchPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       IID_IDirectorySearch,
                       (LPVOID*)&pSchemaSearch);
    FailGracefully(hr, "Failed to get schema search object");

    // Set preferences to Asynchronous, Deep search, Paged results
    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = PAGE_SIZE;

    hr = pSchemaSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));

    // Do the search
    hr = pSchemaSearch->ExecuteSearch((LPWSTR)pszFilter,
                                      (LPWSTR*)pProperties,
                                      ARRAYSIZE(pProperties),
                                      &hSearch);
    FailGracefully(hr, "IDirectorySearch::ExecuteSearch failed");

    // Loop through the rows, getting the name and ID of each property or class
    for (;;)
    {
        ADS_SEARCH_COLUMN colLdapName;
        ADS_SEARCH_COLUMN colGuid;
        LPWSTR pszLdapName;
        LPGUID pID;
        PID_CACHE_ENTRY pCacheEntry;

        hr = pSchemaSearch->GetNextRow(hSearch);

        if (FAILED(hr) || hr == S_ADS_NOMORE_ROWS)
            break;

        // Get class/property internal name
        hr = pSchemaSearch->GetColumn(hSearch, (LPWSTR)c_szLDAPDisplayName, &colLdapName);
        if (FAILED(hr))
        {
            TraceMsg("lDAPDisplayName not found for class/property");
            continue;
        }

        TraceAssert(colLdapName.dwADsType >= ADSTYPE_DN_STRING
                    && colLdapName.dwADsType <= ADSTYPE_NUMERIC_STRING);
        TraceAssert(colLdapName.dwNumValues == 1);

        pszLdapName = colLdapName.pADsValues->CaseIgnoreString;

        // Get the GUID column
        hr = pSchemaSearch->GetColumn(hSearch, (LPWSTR)c_szSchemaIDGUID, &colGuid);
        if (FAILED(hr))
        {
            Trace((TEXT("GUID not found for \"%s\""), pszLdapName));
            pSchemaSearch->FreeColumn(&colLdapName);
            continue;
        }

        // Get GUID from column
        TraceAssert(colGuid.dwADsType == ADSTYPE_OCTET_STRING);
        TraceAssert(colGuid.dwNumValues == 1);
        TraceAssert(colGuid.pADsValues->OctetString.dwLength == sizeof(GUID));

        pID = (LPGUID)(colGuid.pADsValues->OctetString.lpValue);

        pCacheEntry = (PID_CACHE_ENTRY)LocalAlloc(LPTR,
                                  sizeof(ID_CACHE_ENTRY)
                                  + sizeof(WCHAR)*lstrlenW(pszLdapName));
        if (pCacheEntry != NULL)
        {
            // Copy the item name and ID
            pCacheEntry->guid = *pID;
            lstrcpyW(pCacheEntry->szLdapName, pszLdapName);

            // Insert into cache
            DPA_AppendPtr(hCache, pCacheEntry);
        }

        pSchemaSearch->FreeColumn(&colLdapName);
        pSchemaSearch->FreeColumn(&colGuid);
    }

    DPA_Sort(hCache, Schema_CompareLdapName, 0);

exit_gracefully:

    if (hSearch != NULL)
        pSchemaSearch->CloseSearchHandle(hSearch);

    DoRelease(pSchemaSearch);

    if (FAILED(hr))
    {
        DestroyDPA(hCache);
        *phCache = NULL;
    }

    TraceLeaveResult(hr);
}


//
// DPA comparison function used for sorting the Extended Rights list
//
int CALLBACK
Schema_CompareER(LPVOID p1, LPVOID p2, LPARAM /*lParam*/)
{
    int nResult = 0;
    PER_ENTRY pEntry1 = (PER_ENTRY)p1;
    PER_ENTRY pEntry2 = (PER_ENTRY)p2;

    if (pEntry1 && pEntry2)
    {
        if (!(pEntry1->mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)))
        {
            if (pEntry2->mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP))
                nResult = -1;
            else
                nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                         0,
                                         pEntry1->szName,
                                         -1,
                                         pEntry2->szName,
                                         -1) - CSTR_EQUAL;
        }
        else if (!(pEntry2->mask & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)))
            nResult = 1;
    }

    return nResult;
}

void
AddExtendedRight(HDPA hList, LPCGUID pguid, DWORD dwMask, DWORD dwFlags, LPCWSTR pszName, LPCWSTR pszFmt = NULL)
{
    ULONG nSize = sizeof(ER_ENTRY) + StringByteSize(pszName);
    if (pszFmt)
        nSize += StringByteSize(pszFmt);

    PER_ENTRY pER = (PER_ENTRY)LocalAlloc(LPTR, nSize);
    if (pER != NULL)
    {
        pER->guid = *pguid;
        pER->mask = dwMask;
        pER->dwFlags = dwFlags;
        if (pszFmt)
            wsprintfW(pER->szName, pszFmt, pszName);
        else
            lstrcpyW(pER->szName, pszName);

        // Insert into list
        DPA_AppendPtr(hList, pER);
    }
}

HRESULT
Schema_GetExtendedRightsForClass(LPWSTR pszSchemaSearchPath,
                                 LPCGUID pguidClass,
                                 DWORD dwPageFlags,
                                 HDPA *phERList)
{
    HRESULT hr = S_OK;
    IDirectorySearch *pSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[3];
    const LPCWSTR pProperties[] =
    {
        c_szDisplayName,                // "displayName"
        c_szDisplayID,                  // "localizationDisplayId"
        c_szRightsGuid,                 // "rightsGuid"
        c_szValidAccesses,              // "validAccesses"
    };
    WCHAR szFilter[100];
    WCHAR szFmtReadPS[MAX_TYPENAME_LENGTH];
    WCHAR szFmtWritePS[MAX_TYPENAME_LENGTH];
    WCHAR szFmtReadWritePS[MAX_TYPENAME_LENGTH];
    HDPA hExtRightList = NULL;

    TraceEnter(TRACE_SCHEMA, "Schema_GetExtendedRightsForClass");
    TraceAssert(pguidClass != NULL && !IsEqualGUID(*pguidClass, GUID_NULL));
    TraceAssert(phERList != NULL);

    *phERList = NULL;

    if (!pszSchemaSearchPath)
        ExitGracefully(hr, E_FAIL, "Missing Extended Rights container path");

    // Build the filter string
    wsprintfW(szFilter, c_szERFilterFormat,
              pguidClass->Data1, pguidClass->Data2, pguidClass->Data3,
              pguidClass->Data4[0], pguidClass->Data4[1],
              pguidClass->Data4[2], pguidClass->Data4[3],
              pguidClass->Data4[4], pguidClass->Data4[5],
              pguidClass->Data4[6], pguidClass->Data4[7]);
    Trace((TEXT("Filter \"%s\""), szFilter));

    //
    // Create DPA to hold results
    //
    hExtRightList = DPA_Create(8);

    if (hExtRightList == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create failed");

    // Get the schema search object
    hr = ADsOpenObject(pszSchemaSearchPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       IID_IDirectorySearch,
                       (LPVOID*)&pSearch);
    FailGracefully(hr, "Failed to get schema search object");

    // Set preferences to Asynchronous, OneLevel search, Paged results
    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = PAGE_SIZE;

    hr = pSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));

    // Do the search
    hr = pSearch->ExecuteSearch(szFilter,
                                (LPWSTR*)pProperties,
                                ARRAYSIZE(pProperties),
                                &hSearch);
    FailGracefully(hr, "IDirectorySearch::ExecuteSearch failed");

    LoadStringW(GLOBAL_HINSTANCE, IDS_DS_READ_PROP_TYPE,  szFmtReadPS,      ARRAYSIZE(szFmtReadPS));
    LoadStringW(GLOBAL_HINSTANCE, IDS_DS_WRITE_PROP_TYPE, szFmtWritePS,     ARRAYSIZE(szFmtWritePS));
    LoadStringW(GLOBAL_HINSTANCE, IDS_DS_READWRITE_TYPE,  szFmtReadWritePS, ARRAYSIZE(szFmtReadWritePS));

    // Loop through the rows, getting the name and ID of each property or class
    for (;;)
    {
        ADS_SEARCH_COLUMN col;
        GUID guid;
        DWORD dwValidAccesses;
        LPWSTR pszName = NULL;
        BOOL bAppliesTo = FALSE;
        DWORD i;
        WCHAR szDisplayName[MAX_PATH];

        hr = pSearch->GetNextRow(hSearch);

        if (FAILED(hr) || hr == S_ADS_NOMORE_ROWS)
            break;

        // Get the GUID
        if (FAILED(pSearch->GetColumn(hSearch, (LPWSTR)c_szRightsGuid, &col)))
        {
            TraceMsg("GUID not found for extended right");
            continue;
        }
        TraceAssert(col.dwADsType >= ADSTYPE_DN_STRING
                    && col.dwADsType <= ADSTYPE_NUMERIC_STRING);
        wsprintfW(szFilter, c_szGUIDFormat, col.pADsValues->CaseIgnoreString);
        CLSIDFromString(szFilter, &guid);
        pSearch->FreeColumn(&col);

        // Get the valid accesses mask
        if (FAILED(pSearch->GetColumn(hSearch, (LPWSTR)c_szValidAccesses, &col)))
        {
            TraceMsg("validAccesses not found for Extended Right");
            continue;
        }
        TraceAssert(col.dwADsType == ADSTYPE_INTEGER);
        TraceAssert(col.dwNumValues == 1);
        dwValidAccesses = (DWORD)(DS_GENERIC_ALL & col.pADsValues->Integer);
        pSearch->FreeColumn(&col);

        // Get the display name
        szDisplayName[0] = L'\0';
        if (SUCCEEDED(pSearch->GetColumn(hSearch, (LPWSTR)c_szDisplayID, &col)))
        {
            TraceAssert(col.dwADsType == ADSTYPE_INTEGER);
            TraceAssert(col.dwNumValues == 1);
            if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                              g_hInstance,
                              col.pADsValues->Integer,
                              0,
                              szDisplayName,
                              ARRAYSIZE(szDisplayName),
                              NULL))
            {
                pszName = szDisplayName;
            }
            pSearch->FreeColumn(&col);
        }

        if (NULL == pszName &&
            SUCCEEDED(pSearch->GetColumn(hSearch, (LPWSTR)c_szDisplayName, &col)))
        {
            TraceAssert(col.dwADsType >= ADSTYPE_DN_STRING
                        && col.dwADsType <= ADSTYPE_NUMERIC_STRING);
            lstrcpynW(szDisplayName, col.pADsValues->CaseIgnoreString, ARRAYSIZE(szDisplayName));
            pszName = szDisplayName;
            pSearch->FreeColumn(&col);
        }

        if (NULL == pszName)
        {
            TraceMsg("displayName not found for Extended Right");
            continue;
        }

        // Is it a Property Set?
        if (dwValidAccesses & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP))
        {
            if (dwValidAccesses & ACTRL_DS_READ_PROP)
            {
                Trace((TEXT("Adding Read PropSet \"%s\""), pszName));
                AddExtendedRight(hExtRightList,
                                 &guid,
                                 ACTRL_DS_READ_PROP,
                                 SI_ACCESS_GENERAL | SI_ACCESS_PROPERTY,
                                 pszName,
                                 szFmtReadPS);
            }
            if (dwValidAccesses & ACTRL_DS_WRITE_PROP)
            {
                Trace((TEXT("Adding Write PropSet \"%s\""), pszName));
                AddExtendedRight(hExtRightList,
                                 &guid,
                                 ACTRL_DS_WRITE_PROP,
                                 SI_ACCESS_GENERAL | SI_ACCESS_PROPERTY,
                                 pszName,
                                 szFmtWritePS);
            }
            if ((ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP) == (dwValidAccesses & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP)))
            {
                // Add a hidden entry for "Read/write <propset>".
                // dwFlags = 0 means it will never show as a checkbox, but it
                // may be used for the name displayed on the Advanced page.
                Trace((TEXT("Adding Read/Write PropSet \"%s\""), pszName));
                AddExtendedRight(hExtRightList,
                                 &guid,
                                 ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP,
                                 0, // not visible, for name only
                                 pszName,
                                 szFmtReadWritePS);
            }
            dwValidAccesses &= ~(ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP);
        }

        if (dwValidAccesses && !(dwPageFlags & SI_EDIT_PROPERTIES))
        {
            // Must be a Control Right, Validated Write, etc.
            // Add these everywhere except the Properties page.
            Trace((TEXT("Adding Extended Right \"%s\""), pszName));
            AddExtendedRight(hExtRightList,
                             &guid,
                             dwValidAccesses,
                             SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC,
                             pszName,
                             NULL);
        }
    }

    DPA_Sort(hExtRightList, Schema_CompareER, 0);

exit_gracefully:

    if (hSearch != NULL)
        pSearch->CloseSearchHandle(hSearch);

    DoRelease(pSearch);

    if (FAILED(hr))
    {
        DestroyDPA(hExtRightList);
        hExtRightList = NULL;
    }

    *phERList = hExtRightList;

    TraceLeaveResult(hr);
}
