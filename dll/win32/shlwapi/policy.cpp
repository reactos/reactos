/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement SHWindowsPolicyGetValue
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define _ATL_NO_EXCEPTIONS
#include <windef.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <shlguid_undoc.h>
#include <atlstr.h>
#include <strsafe.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(policy);

typedef enum tagPOLICY_STATE
{
    POLICY_STATE_UNCACHED  = 0, // Uncached
    POLICY_STATE_NOT_FOUND = 1, // Not found
    POLICY_STATE_CACHED    = 2, // Cached
} POLICY_STATE;

// Result
typedef struct tagSHPOLICY_RESULT
{
    POLICY_STATE state;
    DWORD dwValue;
} SHPOLICY_RESULT, *PSHPOLICY_RESULT;

// Contraints
typedef struct tagSHPOLICY_CONSTRAINT
{
    WORD wFlags;
    WORD wMinSize;
    DWORD dwMin;
    DWORD dwMax;
} SHPOLICY_CONSTRAINT, *PSHPOLICY_CONSTRAINT;

static const SHPOLICY_CONSTRAINT c_Bool     = { SRRF_RT_DWORD,  sizeof(DWORD), 0, 1 };
static const SHPOLICY_CONSTRAINT c_String   = { SRRF_RT_REG_SZ, sizeof(WCHAR), 0, 0 };
static const SHPOLICY_CONSTRAINT c_TriValue = { SRRF_RT_DWORD,  sizeof(DWORD), 1, 3 };
static const SHPOLICY_CONSTRAINT c_Special  = { SRRF_RT_DWORD,  sizeof(DWORD), 0x1806, 0x1808 };

// Items
typedef struct tagSHPOLICY_ITEM
{
    REFGUID rpolid; // POLID (policy descriptor)
    PCWSTR pszKeyName;
    PCWSTR pszValueName;
    const SHPOLICY_CONSTRAINT *pConstraint;
} SHPOLICY_ITEM, *PSHPOLICY_ITEM;

static const SHPOLICY_ITEM g_PolicyItems[] =
{
    { POLID_UsePathEnvVarForCommandTemplates, L"Explorer", L"UsePathEnvVarForCommandTemplates",
      &c_Bool },
    { POLID_ScanWithAntiVirus, L"Attachments", L"ScanWithAntiVirus", &c_TriValue },
    { POLID_SaveZoneInformation, L"Attachments", L"SaveZoneInformation", &c_TriValue },
    { POLID_UseTrustedHandlers, L"Attachments", L"UseTrustedHandlers", &c_TriValue },
    { POLID_HideZoneInfoOnProperties, L"Attachments", L"HideZoneInfoOnProperties", &c_Bool },
    { POLID_DefaultFileTypeRisk, L"Associations", L"DefaultFileTypeRisk", &c_Special },
    { POLID_HighRiskFileTypes, L"Associations", L"HighRiskFileTypes", &c_String },
    { POLID_ModRiskFileTypes, L"Associations", L"ModRiskFileTypes", &c_String },
    { POLID_LowRiskFileTypes, L"Associations", L"LowRiskFileTypes", &c_String },
    { POLID_PreXPSP2ShellProtocolBehavior, L"Explorer", L"PreXPSP2ShellProtocolBehavior", &c_Bool },
    { POLID_CompareJunctionness, L"Explorer", L"CompareJunctionness", &c_Bool },
};

/**************************************************************************
 * CPolicyCache
 */
class CPolicyCache
{
public:
    virtual ~CPolicyCache()
    {
        LocalFree(m_pResults);

        if (m_hGlobalCounter)
            CloseHandle(m_hGlobalCounter);
    }

    BOOL Initialize(const SHPOLICY_ITEM *pItems, UINT cItems)
    {
        m_pszRootKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies";
        m_pItems = pItems;
        m_cItems = cItems;
        m_pResults = (PSHPOLICY_RESULT)LocalAlloc(LPTR, cItems * sizeof(SHPOLICY_RESULT));
        m_hGlobalCounter = SHGlobalCounterCreate(GUID_Restrictions);
        return m_pResults && m_hGlobalCounter;
    }

    HRESULT GetValue(_In_ REFGUID rpolid, _Out_opt_ PVOID pvValue, _Out_opt_ PDWORD pcbValue);;

protected:
    LPCWSTR m_pszRootKey = NULL;
    HANDLE m_hGlobalCounter = NULL;
    LONG m_nCounterValue = -1;
    DWORD m_cItems = 0;
    const SHPOLICY_ITEM *m_pItems = NULL;
    PSHPOLICY_RESULT m_pResults = NULL;

    void _ValidateCachedResults()
    {
        LONG Value = SHGlobalCounterGetValue(m_hGlobalCounter);
        if (m_nCounterValue != Value)
        {
            m_nCounterValue = Value;
            ZeroMemory(m_pResults, m_cItems * sizeof(SHPOLICY_RESULT));
        }
    }

    HRESULT _GetValue(
        _In_ LPCWSTR pszSubKey,
        _In_ LPCWSTR pszValueName,
        _In_ const SHPOLICY_CONSTRAINT* pConstraint,
        _Out_opt_ PDWORD pdwType,
        _Out_opt_ PVOID pvData,
        _Inout_opt_ PDWORD pcbData);

    static void _CacheResult(
        const SHPOLICY_CONSTRAINT *pConstraint,
        PVOID pvValue,
        PDWORD pcbValue,
        PSHPOLICY_RESULT pResult);
};

HRESULT
CPolicyCache::GetValue(_In_ REFGUID rpolid, _Out_opt_ PVOID pvValue, _Out_opt_ PDWORD pcbValue)
{
    _ValidateCachedResults();

    if (m_cItems == 0)
        return E_UNEXPECTED;

    UINT iItem;
    for (iItem = 0; iItem < m_cItems; ++iItem)
    {
        if (IsEqualGUID(m_pItems[iItem].rpolid, rpolid))
            break;
    }

    if (iItem >= m_cItems)
        return E_UNEXPECTED;

    const SHPOLICY_ITEM *pItem = &m_pItems[iItem];
    PSHPOLICY_RESULT pResult = &m_pResults[iItem];

    if (pResult->state == POLICY_STATE_NOT_FOUND)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (pvValue && pcbValue && *pcbValue == sizeof(DWORD) && pResult->state == POLICY_STATE_CACHED)
    {
        *(PDWORD)pvValue = pResult->dwValue;
        return S_OK;
    }

    HRESULT hr = _GetValue(pItem->pszKeyName, pItem->pszValueName, pItem->pConstraint, NULL,
                           pvValue, pcbValue);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        pResult->state = POLICY_STATE_NOT_FOUND;
    }
    else if (SUCCEEDED(hr) && pvValue)
    {
        _CacheResult(pItem->pConstraint, pvValue, pcbValue, pResult);
    }

    return hr;
}

HRESULT CPolicyCache::_GetValue(
    _In_ LPCWSTR pszSubKey,
    _In_ LPCWSTR pszValueName,
    _In_ const SHPOLICY_CONSTRAINT* pConstraint,
    _Out_opt_ PDWORD pdwType,
    _Out_opt_ PVOID pvData,
    _Inout_opt_ PDWORD pcbData)
{
    CStringW szFullKey = CStringW(m_pszRootKey) + L"\\" + pszSubKey;

    DWORD cbDataSaved = pcbData ? *pcbData : 0;
    WORD wFlags = pConstraint->wFlags;

    LSTATUS error = RegGetValueW(HKEY_LOCAL_MACHINE, szFullKey, pszValueName, wFlags, pdwType,
                                 pvData, pcbData);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        if (pcbData)
            *pcbData = cbDataSaved;
        error = RegGetValueW(HKEY_CURRENT_USER, szFullKey, pszValueName, wFlags, pdwType,
                             pvData, pcbData);
    }

    if (error)
        return HRESULT_FROM_WIN32(error);

    if (!pvData)
        return S_OK;

    if (pConstraint->wFlags == SRRF_RT_DWORD && pConstraint->wMinSize == sizeof(DWORD))
    {
        DWORD dwValue = *(PDWORD)pvData;
        if (dwValue < pConstraint->dwMin || pConstraint->dwMax < dwValue)
            return E_DATATYPE_MISMATCH;
    }

    return S_OK;
}

void
CPolicyCache::_CacheResult(
    const SHPOLICY_CONSTRAINT *pConstraint,
    PVOID pvValue,
    PDWORD pcbValue,
    PSHPOLICY_RESULT pResult)
{
    if (pConstraint->wFlags == SRRF_RT_DWORD)
    {
        pResult->dwValue = *(PDWORD)pvValue;
        pResult->state = POLICY_STATE_CACHED;
    }
}

/***************************************************************************/

CPolicyCache* g_pPolicyCache = NULL;
CRITICAL_SECTION g_csPolicyLock;

static BOOL SHPolicyCache_Create(VOID)
{
    if (g_pPolicyCache)
        return TRUE;

    CPolicyCache *pCache = new CPolicyCache();
    if (!pCache)
        return FALSE;

    if (!pCache->Initialize(g_PolicyItems, _countof(g_PolicyItems)))
    {
        delete pCache;
        return FALSE;
    }

    PVOID pOldCache =
        InterlockedCompareExchangePointer((PVOID volatile*)&g_pPolicyCache, pCache, NULL);
    if (pOldCache)
        delete pCache;

    return TRUE;
}

EXTERN_C VOID SHPolicyCache_DllProcessAttach(VOID)
{
    InitializeCriticalSection(&g_csPolicyLock);
}

EXTERN_C VOID SHPolicyCache_DllProcessDetach(VOID)
{
    DeleteCriticalSection(&g_csPolicyLock);

    CPolicyCache* pCache =
        (CPolicyCache*)InterlockedExchangePointer((PVOID volatile*)&g_pPolicyCache, NULL);
    if (pCache)
        delete pCache;
}

/**************************************************************************
 *  SHWindowsPolicyGetValue (SHLWAPI.560)
 *
 * https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/winpolicy/getvalue.htm
 */
EXTERN_C
HRESULT WINAPI
SHWindowsPolicyGetValue(
    _In_ REFGUID rpolid,
    _Out_opt_ PVOID pvValue,
    _Out_opt_ PDWORD pcbValue)
{
    if (!SHPolicyCache_Create())
        return E_FAIL;

    EnterCriticalSection(&g_csPolicyLock);
    HRESULT hr = g_pPolicyCache->GetValue(rpolid, pvValue, pcbValue);
    LeaveCriticalSection(&g_csPolicyLock);
    return hr;
}
