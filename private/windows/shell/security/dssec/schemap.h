//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       schemap.h
//
//--------------------------------------------------------------------------

#ifndef _SCHEMAP_H_
#define _SCHEMAP_H_

//
// Structures used by the cache
//
typedef struct _IdCacheEntry
{
    GUID    guid;
    WCHAR   szLdapName[ANYSIZE_ARRAY];
} ID_CACHE_ENTRY, *PID_CACHE_ENTRY;

typedef struct _ER_ENTRY
{
    GUID  guid;
    DWORD mask;
    DWORD dwFlags;
    WCHAR szName[ANYSIZE_ARRAY];
} ER_ENTRY, *PER_ENTRY;

typedef struct _InheritTypeArray
{
    GUID            guidObjectType;
    DWORD           dwFlags;
    ULONG           cInheritTypes;
    SI_INHERIT_TYPE aInheritType[ANYSIZE_ARRAY];
} INHERIT_TYPE_ARRAY, *PINHERIT_TYPE_ARRAY;

typedef struct _AccessArray
{
    GUID        guidObjectType;
    DWORD       dwFlags;
    ULONG       cAccesses;
    ULONG       iDefaultAccess;
    SI_ACCESS   aAccess[ANYSIZE_ARRAY];
} ACCESS_ARRAY, *PACCESS_ARRAY;

//
// CSchemaCache object definition
//
class CSchemaCache
{
protected:
    BSTR        m_strSchemaSearchPath;
    BSTR        m_strERSearchPath;
    BSTR        m_strFilterFile;
    HDPA        m_hClassCache;
    HDPA        m_hPropertyCache;
    HANDLE      m_hClassThread;
    HANDLE      m_hPropertyThread;
    HRESULT     m_hrClassResult;
    HRESULT     m_hrPropertyResult;
    PINHERIT_TYPE_ARRAY m_pInheritTypeArray;
    HDPA        m_hAccessList;
    CRITICAL_SECTION m_AccessListCritSec;
    int         m_nDsListObjectEnforced;

public:
    CSchemaCache(LPCWSTR pszServer);
    ~CSchemaCache();

    LPCWSTR GetClassName(LPCGUID pguidObjectType);
    HRESULT GetInheritTypes(LPCGUID pguidObjectType,
                            DWORD dwFlags,
                            PSI_INHERIT_TYPE *ppInheritTypes,
                            ULONG *pcInheritTypes);
    HRESULT GetAccessRights(LPCGUID pguidObjectType,
                            LPCWSTR pszClassName,
                            LPCWSTR pszSchemaPath,
                            DWORD dwFlags,
                            PSI_ACCESS *ppAccesses,
                            ULONG *pcAccesses,
                            ULONG *piDefaultAccess);

protected:
    HRESULT WaitOnClassThread()
        { WaitOnThread(&m_hClassThread); return m_hrClassResult; }
    HRESULT WaitOnPropertyThread()
        { WaitOnThread(&m_hPropertyThread); return m_hrPropertyResult; }

private:
    PID_CACHE_ENTRY LookupID(HDPA hCache, LPCWSTR pszLdapName);
    PID_CACHE_ENTRY LookupClass(LPCGUID pguidObjectType);
    LPCGUID LookupClassID(LPCWSTR pszClass);
    LPCGUID LookupPropertyID(LPCWSTR pszProperty);

    int GetListObjectEnforced(void);
    BOOL HideListObjectAccess(void);

    HRESULT BuildAccessArray(LPCGUID pguidObjectType,
                             LPCWSTR pszClassName,
                             LPCWSTR pszSchemaPath,
                             DWORD dwFlags,
                             PACCESS_ARRAY *ppAccessArray);
    HRESULT BuildPropertyAccessArray(LPCGUID pguidObjectType,
                                     LPCWSTR pszClassName,
                                     LPCWSTR pszSchemaPath,
                                     DWORD dwFlags,
                                     PACCESS_ARRAY *ppAccessArray);
    HRESULT EnumVariantList(LPVARIANT pvarList,
                            HDPA hTempList,
                            DWORD dwFlags,
                            IDsDisplaySpecifier *pDisplaySpec,
                            LPCWSTR pszPropertyClass = NULL);
    UINT AddTempListToAccessList(HDPA hTempList,
                                 PSI_ACCESS *ppAccess,
                                 LPBYTE *ppData,
                                 DWORD dwFlags);

    static DWORD WINAPI SchemaClassThread(LPVOID pvThreadData);
    static DWORD WINAPI SchemaPropertyThread(LPVOID pvThreadData);

    HRESULT BuildInheritTypeArray(DWORD dwFlags);
};
typedef CSchemaCache *PSCHEMACACHE;

extern PSCHEMACACHE g_pSchemaCache;

#endif  // _SCHEMAP_H_
