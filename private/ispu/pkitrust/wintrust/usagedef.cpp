//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       usagedef.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider Model
//
//  Functions:  WintrustAddDefaultForUsage
//              WintrustGetDefaultForUsage
//
//  History:    07-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "cryptreg.h"


BOOL WINAPI WintrustAddDefaultForUsage(const char *pszUsageOID, 
                                       CRYPT_PROVIDER_REGDEFUSAGE *psDefUsage)
{
    HKEY            hKey;
    WCHAR           wsz[REG_MAX_KEY_NAME];
    WCHAR           wszUsage[REG_MAX_FUNC_NAME];
    WCHAR           wszGuid[REG_MAX_GUID_TEXT];
    DWORD           dwDisposition;
    HRESULT         hr;
    int             cchUsage;

    if (!(pszUsageOID) ||
        !(psDefUsage) ||
        !(_ISINSTRUCT(CRYPT_PROVIDER_REGDEFUSAGE, 
                      psDefUsage->cbStruct, 
                      pwszFreeCallbackDataFunctionName)) ||
        !(psDefUsage->pgActionID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (!(guid2wstr(psDefUsage->pgActionID, &wszGuid[0])))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    wszUsage[0] = L'\0';
    cchUsage = MultiByteToWideChar(0, 0, pszUsageOID, -1, &wszUsage[0],
        REG_MAX_FUNC_NAME);
    if (0 >= cchUsage)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (REG_MAX_KEY_NAME < wcslen(REG_TRUST_USAGE_KEY) + 1 + cchUsage + 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    wcscpy(&wsz[0], REG_TRUST_USAGE_KEY);
    wcscat(&wsz[0], L"\\");
    wcscat(&wsz[0], &wszUsage[0]);

    if (RegCreateKeyExU(HKEY_LOCAL_MACHINE,
                        &wsz[0],
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                        &hKey, &dwDisposition) != ERROR_SUCCESS)
    {
        return(FALSE);
    }

    hr = RegSetValueExU(hKey, REG_DEF_FOR_USAGE,
                        0, REG_SZ,
                        (BYTE *)&wszGuid[0],
                        (wcslen(&wszGuid[0]) + 1) * sizeof(WCHAR));

    if (psDefUsage->pwszDllName)
    {
        hr |= RegSetValueExU(hKey, REG_DLL_NAME,
                            0, REG_SZ,
                            (BYTE *)psDefUsage->pwszDllName,
                            (wcslen(psDefUsage->pwszDllName) + 1) * sizeof(WCHAR));

        if (psDefUsage->pwszLoadCallbackDataFunctionName)
        {
            hr |= RegSetValueExA(hKey, REG_DEF_CALLBACK_ALLOC,
                                0, REG_SZ,
                                (BYTE *)psDefUsage->pwszLoadCallbackDataFunctionName,
                                strlen(psDefUsage->pwszLoadCallbackDataFunctionName) + 1);

            if (psDefUsage->pwszFreeCallbackDataFunctionName)
            {
                hr |= RegSetValueExA(hKey, REG_DEF_CALLBACK_FREE,
                                    0, REG_SZ,
                                    (BYTE *)psDefUsage->pwszFreeCallbackDataFunctionName,
                                    strlen(psDefUsage->pwszFreeCallbackDataFunctionName) + 1);
            }
        }
    }

    RegCloseKey(hKey);

    return((hr == ERROR_SUCCESS) ? TRUE : FALSE);
}

BOOL WINAPI WintrustGetDefaultForUsage(DWORD dwAction, const char *pszUsageOID,
                                       CRYPT_PROVIDER_DEFUSAGE *psUsage)
{
    BOOL                        fRet;
    HKEY                        hKey;
    WCHAR                       wsz[REG_MAX_KEY_NAME];
    char                        szFunc[REG_MAX_FUNC_NAME];
    WCHAR                       wszUsage[REG_MAX_FUNC_NAME];
    WCHAR                       wszGuid[REG_MAX_GUID_TEXT];
    DWORD                       dwType;
    DWORD                       dwSize;
    HINSTANCE                   hDll;
    PFN_ALLOCANDFILLDEFUSAGE    pfnAlloc;
    PFN_FREEDEFUSAGE            pfnFree;
    int                         cchUsage;

    fRet        = TRUE;
    hKey        = NULL;
    hDll        = NULL;
    pfnAlloc    = NULL;
    pfnFree     = NULL;

    if (!(pszUsageOID) ||
        !(psUsage) ||
        !(_ISINSTRUCT(CRYPT_PROVIDER_DEFUSAGE, 
                      psUsage->cbStruct, 
                      pDefSIPClientData)))
    {
        goto InvalidParamError;
    }

    memset(&psUsage->gActionID, 0x00, sizeof(GUID));

    wszUsage[0] = L'\0';
    cchUsage = MultiByteToWideChar(0, 0, pszUsageOID, -1, &wszUsage[0], 
        REG_MAX_FUNC_NAME);
    if (0 >= cchUsage)
    {
        goto InvalidParamError;
    }

    if (REG_MAX_KEY_NAME < wcslen(REG_TRUST_USAGE_KEY) + 1 + cchUsage + 1)
    {
        goto InvalidParamError;
    }
    wcscpy(&wsz[0], REG_TRUST_USAGE_KEY);
    wcscat(&wsz[0], L"\\");
    wcscat(&wsz[0], &wszUsage[0]);

    if (RegOpenKeyExU(  HKEY_LOCAL_MACHINE,
                        &wsz[0],
                        0,
                        KEY_READ,
                        &hKey) != ERROR_SUCCESS)
    {
        goto RegOpenError;
    }

    // 
    //  get the dll name and function entry points
    //
    dwType = 0;
    dwSize = REG_MAX_KEY_NAME * sizeof(WCHAR);

    if (RegQueryValueExU(hKey, REG_DLL_NAME, NULL, &dwType, (BYTE *)&wsz[0], &dwSize) == ERROR_SUCCESS)
    {
        if (hDll = LoadLibraryU(&wsz[0]))
        {
            dwType = 0;
            dwSize = REG_MAX_FUNC_NAME;

            if (RegQueryValueExA(hKey, 
                                (dwAction == DWACTION_FREE) ? REG_DEF_CALLBACK_FREE : REG_DEF_CALLBACK_ALLOC,
                                NULL, &dwType, (BYTE *)&szFunc[0], &dwSize) == ERROR_SUCCESS)
            {
                if (dwAction == DWACTION_FREE)
                {
                    pfnFree = (PFN_FREEDEFUSAGE)GetProcAddress(hDll, &szFunc[0]);

                    if (pfnFree)
                    {
                        (*pfnFree)(pszUsageOID, psUsage);
                    }

                    fRet = TRUE;
                    goto CommonReturn;
                }
                
                pfnAlloc = (PFN_ALLOCANDFILLDEFUSAGE)GetProcAddress(hDll, &szFunc[0]);
            }
        }
    }

    if (dwAction != DWACTION_ALLOCANDFILL)
    {
        goto CommonReturn;
    }

    dwType = 0;
    dwSize = REG_MAX_GUID_TEXT * sizeof(WCHAR);

    wszGuid[0] = NULL;

    if (RegQueryValueExU(   hKey, 
                            REG_DEF_FOR_USAGE,
                            NULL, 
                            &dwType,
                            (BYTE *)&wszGuid[0],
                            &dwSize) != ERROR_SUCCESS)
    {
        goto RegQueryError;
    }

    if (!(wstr2guid(&wszGuid[0], &psUsage->gActionID)))
    {
        goto GuidError;
    }

    if (pfnAlloc)
    {
        if (!(*pfnAlloc)(pszUsageOID, psUsage))
        {
            goto UsageAllocError;
        }
    }

    CommonReturn:
        if (hKey)
        {
            RegCloseKey(hKey);
        }

        if (hDll)
        {
            FreeLibrary(hDll);
        }

        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, RegOpenError);
    TRACE_ERROR_EX(DBG_SS, RegQueryError);
    TRACE_ERROR_EX(DBG_SS, GuidError);
    TRACE_ERROR_EX(DBG_SS, UsageAllocError);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParamError, ERROR_INVALID_PARAMETER);
}
