/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement SHAutoComplete
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <windef.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <shlguid_undoc.h>
#include <strsafe.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(policy);

#define E_DATATYPE_MISMATCH 0x8007065D

typedef enum tagPOLICY_STATE
{
    POLICY_STATE_UNCACHED  = 0,  // Uncached
    POLICY_STATE_NOT_FOUND = 1,  // Not found
    POLICY_STATE_CACHED    = 2,  // Cached
} POLICY_STATE;

typedef struct tagSHPOLICY_RESULT
{
    POLICY_STATE state;
    DWORD dwValue;
} SHPOLICY_RESULT, *PSHPOLICY_RESULT;

typedef struct tagSHPOLICY_CONSTRAINT
{
    DWORD dwFlags;
    DWORD dwMin;
    DWORD dwMax;
} SHPOLICY_CONSTRAINT, *PSHPOLICY_CONSTRAINT;

typedef struct tagSHPOLICY_ITEM
{
    REFGUID rpolid;
    LPCWSTR key;
    LPCWSTR value;
    const SHPOLICY_CONSTRAINT *pConstraint;
} SHPOLICY_ITEM, *PSHPOLICY_ITEM;

static const SHPOLICY_CONSTRAINT c_spcBool     = { MAKELONG(SRRF_RT_DWORD,  sizeof(DWORD)), 0, 1 };
static const SHPOLICY_CONSTRAINT c_spcString   = { MAKELONG(SRRF_RT_REG_SZ, sizeof(WCHAR)), 0, 0 };
static const SHPOLICY_CONSTRAINT c_spcTriValue = { MAKELONG(SRRF_RT_DWORD,  sizeof(DWORD)), 1, 3 };
static const SHPOLICY_CONSTRAINT c_spcSpecial  = { MAKELONG(SRRF_RT_DWORD,  sizeof(DWORD)), 0x1806, 0x1808 };

static const SHPOLICY_ITEM g_PolicyItems[] =
{
    { POLID_UsePathEnvVarForCommandTemplates, L"Explorer", L"UsePathEnvVarForCommandTemplates", &c_spcBool },
    { POLID_ScanWithAntiVirus, L"Attachments", L"ScanWithAntiVirus", &c_spcTriValue },
    { POLID_SaveZoneInformation, L"Attachments", L"SaveZoneInformation", &c_spcTriValue },
    { POLID_UseTrustedHandlers, L"Attachments", L"UseTrustedHandlers", &c_spcTriValue },
    { POLID_HideZoneInfoOnProperties, L"Attachments", L"HideZoneInfoOnProperties", &c_spcBool },
    { POLID_DefaultFileTypeRisk, L"Associations", L"DefaultFileTypeRisk", &c_spcSpecial },
    { POLID_HighRiskFileTypes, L"Associations", L"HighRiskFileTypes", &c_spcString },
    { POLID_ModRiskFileTypes, L"Associations", L"ModRiskFileTypes", &c_spcString },
    { POLID_LowRiskFileTypes, L"Associations", L"LowRiskFileTypes", &c_spcString },
    { POLID_PreXPSP2ShellProtocolBehavior, L"Explorer", L"PreXPSP2ShellProtocolBehavior", &c_spcBool },
    { POLID_CompareJunctionness, L"Explorer", L"CompareJunctionness", &c_spcBool },
};

static HRESULT
SHPolicyGetValue(
    LPCWSTR                    pszBaseKey,
    LPCWSTR                    pszSubKey,
    LPCWSTR                    pszValueName,
    const SHPOLICY_CONSTRAINT* pConstraint,
    DWORD*                     pdwType,
    PVOID                      pvData,
    DWORD*                     pcbData)
{
    WCHAR szFullKey[MAX_PATH];
    HRESULT hr = StringCchPrintfW(szFullKey, _countof(szFullKey), L"%s\\%s", pszBaseKey, pszSubKey);
    if (FAILED(hr))
        return hr;

    DWORD cbDataSaved = pcbData ? *pcbData : 0;

    DWORD dwFlags = LOWORD(pConstraint->dwFlags);

    LSTATUS error;
    error = RegGetValueW(HKEY_LOCAL_MACHINE, szFullKey, pszValueName, dwFlags, pdwType,
                         pvData, pcbData);
    if (error == ERROR_FILE_NOT_FOUND)
    {
        if (pcbData)
            *pcbData = cbDataSaved;
        error = RegGetValueW(HKEY_CURRENT_USER, szFullKey, pszValueName, dwFlags, pdwType,
                             pvData, pcbData);
    }

    if (error)
        return HRESULT_FROM_WIN32(error);

    if (!pvData)
        return hr;

    DWORD dwValue = *(PDWORD)pvData;
    if (pConstraint->dwFlags == MAKELONG(SRRF_RT_DWORD, sizeof(DWORD)) &&
        (dwValue < pConstraint->dwMin || pConstraint->dwMax < dwValue))
    {
        return E_DATATYPE_MISMATCH;
    }

    return hr;
}

static void
SHPolicy_CacheResult(
    const SHPOLICY_CONSTRAINT *pConstraint,
    PVOID pvValue,
    PDWORD pcbValue,
    PSHPOLICY_RESULT pResult)
{
    if (LOWORD(pConstraint->dwFlags) == SRRF_RT_DWORD)
    {
        pResult->dwValue = *(PDWORD)pvValue;
        pResult->state = POLICY_STATE_CACHED;
    }
}

/**************************************************************************
 * CPolicyCache
 */
class CPolicyCache
{
public:
    CPolicyCache()
        : m_pszRootKey(NULL)
        , m_hGlobalCounter(NULL)
        , m_nCounterValue(-1)
        , m_cItems(0)
        , m_pItems(NULL)
        , m_pResults(NULL)
    {
    }

    virtual ~CPolicyCache()
    {
        LocalFree(m_pResults);
        CloseHandle(m_hGlobalCounter);
    }

    BOOL Initialize(LPCWSTR pszRootKey, REFGUID rguid, const SHPOLICY_ITEM *pItems, UINT cItems)
    {
        m_pszRootKey = pszRootKey;
        m_pItems = pItems;
        m_cItems = cItems;
        m_pResults = (PSHPOLICY_RESULT)LocalAlloc(LPTR, cItems * sizeof(SHPOLICY_RESULT));
        m_hGlobalCounter = SHGlobalCounterCreate(rguid);
        return m_pResults && m_hGlobalCounter;
    }

    HRESULT GetValue(_In_ REFGUID rpolid, _Out_opt_ PVOID pvValue, _Out_opt_ PDWORD pcbValue);;

protected:
    LPCWSTR m_pszRootKey;
    HANDLE m_hGlobalCounter;
    LONG m_nCounterValue;
    DWORD m_cItems;
    const SHPOLICY_ITEM *m_pItems;
    PSHPOLICY_RESULT m_pResults;

    void _ValidateCachedResults()
    {
        LONG Value = SHGlobalCounterGetValue(m_hGlobalCounter);
        if (m_nCounterValue != Value)
        {
            SIZE_T cbItems = 8 * m_cItems;
            m_nCounterValue = Value;
            ZeroMemory(m_pResults, cbItems);
        }
    }
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
        if (memcmp(&m_pItems[iItem].rpolid, &rpolid, sizeof(GUID)) == 0)
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

    if (pvValue && *pcbValue == sizeof(DWORD) && pResult->state == POLICY_STATE_CACHED)
    {
        *static_cast<PDWORD>(pvValue) = pResult->dwValue;
        return S_OK;
    }

    HRESULT hr = SHPolicyGetValue(m_pszRootKey, pItem->key, pItem->value,
                                  pItem->pConstraint, NULL, pvValue, pcbValue);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        pResult->state = POLICY_STATE_NOT_FOUND;
    }
    else if (SUCCEEDED(hr) && pvValue)
    {
        SHPolicy_CacheResult(pItem->pConstraint, pvValue, pcbValue, pResult);
    }

    return hr;
}

CPolicyCache* g_pPolicyCache = NULL;
CRITICAL_SECTION g_csPolicy;

EXTERN_C BOOL SHPolicyCache_DllProcessAttach(VOID)
{
    CPolicyCache *pPolicyCache = new CPolicyCache();
    if (!pPolicyCache)
        return FALSE;

    if (!pPolicyCache->Initialize(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies",
                                  GUID_Restrictions, g_PolicyItems, _countof(g_PolicyItems)))
    {
        delete pPolicyCache;
        return FALSE;
    }

    InitializeCriticalSection(&g_csPolicy);
    return TRUE;
}

EXTERN_C VOID SHPolicyCache_DllProcessDetach(VOID)
{
    delete g_pPolicyCache;
    g_pPolicyCache = NULL;
    DeleteCriticalSection(&g_csPolicy);
}

static HRESULT
SHPolicyCacheGetValue(
    CPolicyCache *pCache,
    REFGUID rpolid,
    PVOID pvValue,
    PDWORD pcbValue)
{
    if (!pCache)
    {
        ERR("!pCache\n");
        return E_FAIL;
    }
    return pCache->GetValue(rpolid, pvValue, pcbValue);
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
    _Out_opt_ PVOID pvData,
    _Out_opt_ PDWORD pcbData)
{
    EnterCriticalSection(&g_csPolicy);
    HRESULT hr = SHPolicyCacheGetValue(g_pPolicyCache, rpolid, pvData, pcbData);
    LeaveCriticalSection(&g_csPolicy);
    return hr;
}
