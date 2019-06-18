/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Network property page provider
 * COPYRIGHT:   Copyright 2018 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

typedef enum _PARAM_TYPE
{
    NO_TYPE,
    INT_TYPE,
    LONG_TYPE,
    WORD_TYPE,
    DWORD_TYPE,
    EDIT_TYPE,
    ENUM_TYPE,
} PARAM_TYPE, *PPARAM_TYPE;

typedef struct _ENUM_OPTION
{
    PWSTR pszValue;
    PWSTR pszName;
} ENUM_OPTION, *PENUM_OPTION;

typedef struct _PARAMETER
{
    PWSTR pszName;
    PWSTR pszDescription;
    PWSTR pszValue;
    DWORD cchValueLength;
    PWSTR pszDefault;
    BOOL bOptional;
    BOOL bPresent;
    PARAM_TYPE Type;

    DWORD dwEnumOptions;
    PENUM_OPTION pEnumOptions;

    BOOL bUpperCase;
    INT iTextLimit;

    INT iBase;
    INT iStep;

    union
    {
        struct
        {
            LONG lMin;
            LONG lMax;
        } l;
        struct
        {
            DWORD dwMin;
            DWORD dwMax;
        } dw;
    } u;
} PARAMETER, *PPARAMETER;

typedef struct _PARAMETER_ARRAY
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    PPARAMETER pCurrentParam;
    DWORD dwCount;
    PARAMETER Array[0];
} PARAMETER_ARRAY, *PPARAMETER_ARRAY;


static
VOID
FreeParameterArray(
    _In_ PPARAMETER_ARRAY ParamArray)
{
    INT i, j;

    if (ParamArray == NULL)
        return;

    for (i = 0; i < ParamArray->dwCount; i++)
    {
        if (ParamArray->Array[i].pszName != NULL)
            HeapFree(GetProcessHeap(), 0, ParamArray->Array[i].pszName);

        if (ParamArray->Array[i].pszDescription != NULL)
            HeapFree(GetProcessHeap(), 0, ParamArray->Array[i].pszDescription);

        if (ParamArray->Array[i].pszDefault != NULL)
            HeapFree(GetProcessHeap(), 0, ParamArray->Array[i].pszDefault);


        if (ParamArray->Array[i].pEnumOptions != NULL)
        {
            for (j = 0; j < ParamArray->Array[i].dwEnumOptions; j++)
            {
                if (ParamArray->Array[i].pEnumOptions[j].pszValue != NULL)
                    HeapFree(GetProcessHeap(), 0, ParamArray->Array[i].pEnumOptions[j].pszValue);

                if (ParamArray->Array[i].pEnumOptions[j].pszName != NULL)
                    HeapFree(GetProcessHeap(), 0, ParamArray->Array[i].pEnumOptions[j].pszName);
            }

            HeapFree(GetProcessHeap(), 0, ParamArray->Array[i].pEnumOptions);
        }
    }

    HeapFree(GetProcessHeap(), 0, ParamArray);
}


static DWORD
GetStringValue(
    _In_ HKEY hKey,
    _In_ PWSTR pValueName,
    _Out_ PWSTR *pString,
    _Out_opt_ PDWORD pdwStringLength)
{
    PWSTR pBuffer;
    DWORD dwLength = 0;
    DWORD dwRegType;
    DWORD dwError;

    *pString = NULL;

    RegQueryValueExW(hKey, pValueName, NULL, &dwRegType, NULL, &dwLength);

    if (dwLength == 0 || dwRegType != REG_SZ)
        return ERROR_FILE_NOT_FOUND;

    pBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength + sizeof(WCHAR));
    if (pBuffer == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    dwError = RegQueryValueExW(hKey, pValueName, NULL, NULL, (LPBYTE)pBuffer, &dwLength);
    if (dwError != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, pBuffer);
        return dwError;
    }

    pBuffer[dwLength / sizeof(WCHAR)] = UNICODE_NULL;

    *pString = pBuffer;
    if (pdwStringLength)
        *pdwStringLength = dwLength;

    return ERROR_SUCCESS;
}


static DWORD
GetBooleanValue(
    _In_ HKEY hKey,
    _In_ PWSTR pValueName,
    _In_ BOOL bDefault,
    _Out_ PBOOL pValue)
{
    WCHAR szBuffer[16];
    DWORD dwLength = 0;
    DWORD dwRegType;

    *pValue = bDefault;

    dwLength = sizeof(szBuffer);
    RegQueryValueExW(hKey,
                     pValueName,
                     NULL,
                     &dwRegType,
                     (LPBYTE)szBuffer,
                     &dwLength);

    if (dwRegType == REG_SZ && dwLength >= sizeof(WCHAR))
    {
        if (szBuffer[0] == L'0')
            *pValue = FALSE;
        else
            *pValue = TRUE;
    }

    return ERROR_SUCCESS;
}


static DWORD
GetIntValue(
    _In_ HKEY hKey,
    _In_ PWSTR pValueName,
    _In_ INT iDefault,
    _Out_ PINT pValue)
{
    WCHAR szBuffer[24];
    DWORD dwLength = 0;
    DWORD dwRegType;

    *pValue = iDefault;

    dwLength = sizeof(szBuffer);
    RegQueryValueExW(hKey,
                     pValueName,
                     NULL,
                     &dwRegType,
                     (LPBYTE)szBuffer,
                     &dwLength);

    if (dwRegType == REG_SZ && dwLength >= sizeof(WCHAR))
    {
        *pValue = _wtoi(szBuffer);
    }

    return ERROR_SUCCESS;
}


static DWORD
GetLongValue(
    _In_ HKEY hKey,
    _In_ PWSTR pValueName,
    _In_ LONG lDefault,
    _Out_ PLONG pValue)
{
    WCHAR szBuffer[24];
    DWORD dwLength = 0;
    DWORD dwRegType;
    PWSTR ptr = NULL;

    dwLength = sizeof(szBuffer);
    RegQueryValueExW(hKey,
                     pValueName,
                     NULL,
                     &dwRegType,
                     (LPBYTE)szBuffer,
                     &dwLength);

    if (dwRegType == REG_SZ && dwLength >= sizeof(WCHAR))
    {
        *pValue = wcstol(szBuffer, &ptr, 10);
        if (*pValue == 0 && ptr != NULL)
            *pValue = lDefault;
    }
    else
    {
        *pValue = lDefault;
    }

    return ERROR_SUCCESS;
}


static DWORD
GetDWordValue(
    _In_ HKEY hKey,
    _In_ PWSTR pValueName,
    _In_ DWORD dwDefault,
    _Out_ PDWORD pValue)
{
    WCHAR szBuffer[24];
    DWORD dwLength = 0;
    DWORD dwRegType;
    PWSTR ptr = NULL;

    dwLength = sizeof(szBuffer);
    RegQueryValueExW(hKey,
                     pValueName,
                     NULL,
                     &dwRegType,
                     (LPBYTE)szBuffer,
                     &dwLength);

    if (dwRegType == REG_SZ && dwLength >= sizeof(WCHAR))
    {
        *pValue = wcstoul(szBuffer, &ptr, 10);
        if (*pValue == 0 && ptr != NULL)
            *pValue = dwDefault;
    }
    else
    {
        *pValue = dwDefault;
    }

    return ERROR_SUCCESS;
}


static
DWORD
GetEnumOptions(
    _In_ HKEY hKey,
    _In_ PPARAMETER pParameter)
{
    HKEY hEnumKey = NULL;
    PENUM_OPTION pOptions = NULL;
    DWORD dwValues, dwMaxValueNameLen, dwMaxValueLen;
    DWORD dwValueNameLength, dwValueLength;
    DWORD i;
    DWORD dwError;

    dwError = RegOpenKeyExW(hKey,
                            L"enum",
                            0,
                            KEY_READ,
                            &hEnumKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = RegQueryInfoKeyW(hEnumKey,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               &dwValues,
                               &dwMaxValueNameLen,
                               &dwMaxValueLen,
                               NULL,
                               NULL);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed (Error %lu)\n", dwError);
        goto done;
    }

    pOptions = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         dwValues * sizeof(ENUM_OPTION));
    if (pOptions == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    for (i = 0; i < dwValues; i++)
    {
        dwValueNameLength = dwMaxValueNameLen + sizeof(WCHAR);
        pOptions[i].pszValue = HeapAlloc(GetProcessHeap(),
                                        0,
                                        dwValueNameLength * sizeof(WCHAR));
        if (pOptions[i].pszValue == NULL)
        {
            dwError = ERROR_OUTOFMEMORY;
            goto done;
        }

        dwValueLength = dwMaxValueLen;
        pOptions[i].pszName = HeapAlloc(GetProcessHeap(),
                                        0,
                                        dwValueLength);
        if (pOptions[i].pszName == NULL)
        {
            dwError = ERROR_OUTOFMEMORY;
            goto done;
        }

        dwError = RegEnumValueW(hEnumKey,
                                i,
                                pOptions[i].pszValue,
                                &dwValueNameLength,
                                NULL,
                                NULL,
                                (PBYTE)pOptions[i].pszName,
                                &dwValueLength);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = ERROR_SUCCESS;
            goto done;
        }
        else if (dwError != ERROR_SUCCESS)
        {
            goto done;
        }
    }

    pParameter->pEnumOptions = pOptions;
    pParameter->dwEnumOptions = dwValues;
    pOptions = NULL;

done:
    if (pOptions != NULL)
    {
        for (i = 0; i < dwValues; i++)
        {
            if (pOptions[i].pszValue != NULL)
                HeapFree(GetProcessHeap(), 0, pOptions[i].pszValue);

            if (pOptions[i].pszName != NULL)
                HeapFree(GetProcessHeap(), 0, pOptions[i].pszName);
        }

        HeapFree(GetProcessHeap(), 0, pOptions);
    }

    if (hEnumKey != NULL)
        RegCloseKey(hEnumKey);

    return dwError;
}


static
INT
FindEnumOption(
    _In_ PPARAMETER pParameter,
    _In_ PWSTR pszValue)
{
    INT i;

    if ((pParameter->pEnumOptions == NULL) ||
        (pParameter->dwEnumOptions == 0))
        return -1;

    for (i = 0; i < pParameter->dwEnumOptions; i++)
    {
        if (_wcsicmp(pParameter->pEnumOptions[i].pszValue, pszValue) == 0)
            return i;
    }

    return -1;
}


static
BOOL
BuildParameterArray(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _Out_ PPARAMETER_ARRAY *ParameterArray)
{
    HKEY hDriverKey = INVALID_HANDLE_VALUE;
    HKEY hParamsKey = INVALID_HANDLE_VALUE;
    HKEY hParamKey;
    PPARAMETER_ARRAY ParamArray = NULL;
    DWORD dwSubKeys, dwMaxSubKeyLen, dwKeyLen, dwIndex;
    PWSTR pszType = NULL;
    LONG lError;
    LONG lDefaultMin, lDefaultMax;
    DWORD dwDefaultMin, dwDefaultMax;
    BOOL ret = FALSE;

    hDriverKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                      DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DRV,
                                      KEY_READ);
    if (hDriverKey == INVALID_HANDLE_VALUE)
    {
        ERR("SetupDiOpenDevRegKey() failed\n");
        return FALSE;
    }

    lError = RegOpenKeyExW(hDriverKey,
                           L"Ndi\\Params",
                           0,
                           KEY_READ,
                           &hParamsKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed (Error %lu)\n", lError);
        goto done;
    }

    lError = RegQueryInfoKeyW(hParamsKey,
                              NULL,
                              NULL,
                              NULL,
                              &dwSubKeys,
                              &dwMaxSubKeyLen,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed (Error %lu)\n", lError);
        goto done;
    }

    TRACE("Sub keys: %lu\n", dwSubKeys);

    if (dwSubKeys == 0)
    {
        TRACE("No sub keys. Done!\n");
        goto done;
    }

    ParamArray = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           sizeof(PARAMETER_ARRAY) + (dwSubKeys * sizeof(PARAMETER)));
    if (ParamArray == NULL)
    {
        ERR("Parameter array allocation failed!\n");
        goto done;
    }

    ParamArray->DeviceInfoSet = DeviceInfoSet;
    ParamArray->DeviceInfoData = DeviceInfoData;
    ParamArray->dwCount = dwSubKeys;

    dwMaxSubKeyLen++;

    for (dwIndex = 0; dwIndex < dwSubKeys; dwIndex++)
    {
        ParamArray->Array[dwIndex].pszName = HeapAlloc(GetProcessHeap(),
                                                       0,
                                                       dwMaxSubKeyLen * sizeof(WCHAR));
        if (ParamArray->Array[dwIndex].pszName == NULL)
        {
            ERR("Parameter array allocation failed!\n");
            goto done;
        }

        dwKeyLen = dwMaxSubKeyLen;
        lError = RegEnumKeyExW(hParamsKey,
                               dwIndex,
                               ParamArray->Array[dwIndex].pszName,
                               &dwKeyLen,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (lError != ERROR_SUCCESS)
            break;

        TRACE("Sub key '%S'\n", ParamArray->Array[dwIndex].pszName);

        lError = RegOpenKeyExW(hParamsKey,
                               ParamArray->Array[dwIndex].pszName,
                               0,
                               KEY_READ,
                               &hParamKey);
        if (lError == ERROR_SUCCESS)
        {
            GetStringValue(hParamKey,
                           L"ParamDesc",
                           &ParamArray->Array[dwIndex].pszDescription,
                           NULL);

            GetStringValue(hParamKey,
                           L"Type",
                           &pszType,
                           NULL);
            if (pszType != NULL)
            {
                if (_wcsicmp(pszType, L"int") == 0)
                    ParamArray->Array[dwIndex].Type = INT_TYPE;
                else if (_wcsicmp(pszType, L"long") == 0)
                    ParamArray->Array[dwIndex].Type = LONG_TYPE;
                else if (_wcsicmp(pszType, L"word") == 0)
                    ParamArray->Array[dwIndex].Type = WORD_TYPE;
                else if (_wcsicmp(pszType, L"dword") == 0)
                    ParamArray->Array[dwIndex].Type = DWORD_TYPE;
                else if (_wcsicmp(pszType, L"edit") == 0)
                    ParamArray->Array[dwIndex].Type = EDIT_TYPE;
                else if (_wcsicmp(pszType, L"enum") == 0)
                    ParamArray->Array[dwIndex].Type = ENUM_TYPE;
                else
                    ParamArray->Array[dwIndex].Type = NO_TYPE;

                HeapFree(GetProcessHeap(), 0, pszType);
                pszType = NULL;
            }

            GetStringValue(hParamKey,
                           L"Default",
                           &ParamArray->Array[dwIndex].pszDefault,
                           NULL);

            GetBooleanValue(hParamKey,
                            L"Optional",
                            FALSE,
                            &ParamArray->Array[dwIndex].bOptional);

            if (ParamArray->Array[dwIndex].Type == INT_TYPE ||
                ParamArray->Array[dwIndex].Type == LONG_TYPE ||
                ParamArray->Array[dwIndex].Type == WORD_TYPE ||
                ParamArray->Array[dwIndex].Type == DWORD_TYPE)
            {
                if (ParamArray->Array[dwIndex].Type == INT_TYPE)
                {
                    lDefaultMin = -32768L; //MIN_SHORT;
                    lDefaultMax = 32767L;  //MAX_SHORT;
                }
                else if (ParamArray->Array[dwIndex].Type == LONG_TYPE)
                {
                    lDefaultMin = (-2147483647L - 1); // MIN_LONG;
                    lDefaultMax = 2147483647L;        // MAX_LONG;
                }
                else if (ParamArray->Array[dwIndex].Type == WORD_TYPE)
                {
                    dwDefaultMin = 0UL;
                    dwDefaultMax = 65535UL; // MAX_WORD;
                }
#if 0
                else if (ParamArray->Array[dwIndex].Type == DWORD_TYPE)
                {
                    dwDefaultMin = 0UL;
                    dwDefaultMax = 4294967295UL; //MAX_DWORD;
                }
#endif

                if (ParamArray->Array[dwIndex].Type == INT_TYPE ||
                    ParamArray->Array[dwIndex].Type == LONG_TYPE)
                {
                    GetLongValue(hParamKey,
                                 L"Min",
                                 lDefaultMin,
                                 &ParamArray->Array[dwIndex].u.l.lMin);

                    GetLongValue(hParamKey,
                                 L"Max",
                                 lDefaultMax,
                                 &ParamArray->Array[dwIndex].u.l.lMax);
                }
                else if (ParamArray->Array[dwIndex].Type == WORD_TYPE ||
                         ParamArray->Array[dwIndex].Type == DWORD_TYPE)
                {
                    GetDWordValue(hParamKey,
                                  L"Min",
                                  dwDefaultMin,
                                  &ParamArray->Array[dwIndex].u.dw.dwMin);

                    GetDWordValue(hParamKey,
                                  L"Max",
                                  dwDefaultMax,
                                  &ParamArray->Array[dwIndex].u.dw.dwMax);
                }

                GetIntValue(hParamKey,
                            L"Base",
                            10,
                            &ParamArray->Array[dwIndex].iBase);

                GetIntValue(hParamKey,
                            L"Step",
                            1,
                            &ParamArray->Array[dwIndex].iStep);
            }
            else if (ParamArray->Array[dwIndex].Type == EDIT_TYPE)
            {
                GetBooleanValue(hParamKey,
                                L"UpperCase",
                                FALSE,
                                &ParamArray->Array[dwIndex].bUpperCase);

                GetIntValue(hParamKey,
                            L"TextLimit",
                            0,
                            &ParamArray->Array[dwIndex].iTextLimit);
            }
            else if (ParamArray->Array[dwIndex].Type == ENUM_TYPE)
            {
                GetEnumOptions(hParamKey,
                               &ParamArray->Array[dwIndex]);
            }

            RegCloseKey(hParamKey);
        }

        lError = GetStringValue(hDriverKey,
                                ParamArray->Array[dwIndex].pszName,
                                &ParamArray->Array[dwIndex].pszValue,
                                &ParamArray->Array[dwIndex].cchValueLength);
        if ((lError == ERROR_SUCCESS) ||
            (ParamArray->Array[dwIndex].pszDefault != NULL))
        {
            ParamArray->Array[dwIndex].bPresent = TRUE;
        }
    }

    *ParameterArray = ParamArray;
    ret = TRUE;

done:
    if (ret == FALSE && ParamArray != NULL)
        FreeParameterArray(ParamArray);

    if (hParamsKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hParamsKey);

    if (hDriverKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hDriverKey);

    return ret;
}


static
VOID
ReadParameterValue(
     HWND hwnd,
     PPARAMETER pParam)
{
    INT iIndex, iLength;

    if (pParam->Type == ENUM_TYPE)
    {
        iIndex = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST));
        if (iIndex != CB_ERR && iIndex < pParam->dwEnumOptions)
        {
            iLength = wcslen(pParam->pEnumOptions[iIndex].pszValue);
            if (iLength > pParam->cchValueLength)
            {
                if (pParam->pszValue != NULL)
                    HeapFree(GetProcessHeap(), 0, pParam->pszValue);

                pParam->pszValue = HeapAlloc(GetProcessHeap(), 0, (iLength + 1) * sizeof(WCHAR));
            }

            if (pParam->pszValue != NULL)
            {
                wcscpy(pParam->pszValue,
                       pParam->pEnumOptions[iIndex].pszValue);
                pParam->cchValueLength = iLength;
            }
        }
    }
    else
    {
        iLength = Edit_GetTextLength(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT));
        if (iLength > pParam->cchValueLength)
        {
            if (pParam->pszValue != NULL)
                HeapFree(GetProcessHeap(), 0, pParam->pszValue);

            pParam->pszValue = HeapAlloc(GetProcessHeap(), 0, (iLength + 1) * sizeof(WCHAR));
        }

        if (pParam->pszValue != NULL)
        {
            Edit_GetText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT),
                         pParam->pszValue,
                         iLength + 1);
            pParam->cchValueLength = iLength;
        }
    }
}


static
VOID
WriteParameterArray(
    _In_ HWND hwnd,
    _In_ PPARAMETER_ARRAY ParamArray)
{
    PPARAMETER Param;
    HKEY hDriverKey;
    INT i;

    if (ParamArray == NULL)
        return;

    hDriverKey = SetupDiOpenDevRegKey(ParamArray->DeviceInfoSet,
                                      ParamArray->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DRV,
                                      KEY_WRITE);
    if (hDriverKey == INVALID_HANDLE_VALUE)
    {
        ERR("SetupDiOpenDevRegKey() failed\n");
        return;
    }

    for (i = 0; i < ParamArray->dwCount; i++)
    {
        Param = &ParamArray->Array[i];

        if (Param == ParamArray->pCurrentParam)
        {
            ReadParameterValue(hwnd, Param);
        }

        if (Param->bPresent)
        {
            TRACE("Set '%S' --> '%S'\n", Param->pszName, Param->pszValue);
            RegSetValueExW(hDriverKey,
                           Param->pszName,
                           0,
                           REG_SZ,
                           (LPBYTE)Param->pszValue,
                           (wcslen(Param->pszValue) + 1) * sizeof(WCHAR));
        }
        else
        {
            TRACE("Delete '%S'\n", Param->pszName);
            RegDeleteValueW(hDriverKey,
                            Param->pszName);
        }
    }

    RegCloseKey(hDriverKey);
}


static
VOID
DisplayParameter(
    _In_ HWND hwnd,
    _In_ PPARAMETER Parameter)
{
    HWND hwndControl;
    LONG_PTR Style;
    INT idx;
    DWORD i;

    ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_PRESENT), (Parameter->bOptional) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_NOT_PRESENT), (Parameter->bOptional) ? SW_SHOW : SW_HIDE);
    if (Parameter->bOptional)
    {
        if (Parameter->bPresent)
            Button_SetCheck(GetDlgItem(hwnd, IDC_PROPERTY_PRESENT), BST_CHECKED);
        else
            Button_SetCheck(GetDlgItem(hwnd, IDC_PROPERTY_NOT_PRESENT), BST_CHECKED);
    }

    switch (Parameter->Type)
    {
        case INT_TYPE:
        case LONG_TYPE:
        case WORD_TYPE:
        case DWORD_TYPE:
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST), SW_HIDE);

            hwndControl = GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN);

            if (Parameter->Type != DWORD_TYPE)
            {
                EnableWindow(hwndControl, Parameter->bPresent);
                ShowWindow(hwndControl, SW_SHOW);
            }

            if (Parameter->Type == WORD_TYPE || Parameter->Type == DWORD_TYPE)
                SendMessage(hwndControl, UDM_SETBASE, Parameter->iBase, 0);
            else
                SendMessage(hwndControl, UDM_SETBASE, 10, 0);

            if (Parameter->Type == INT_TYPE || Parameter->Type == LONG_TYPE)
            {
                TRACE("SetMin %ld  SetMax %ld\n", Parameter->u.l.lMin, Parameter->u.l.lMax);
                SendMessage(hwndControl, UDM_SETRANGE32, Parameter->u.l.lMin, Parameter->u.l.lMax);
            }
            else if (Parameter->Type == WORD_TYPE)
            {
                TRACE("SetMin %lu  SetMax %lu\n", Parameter->u.dw.dwMin, Parameter->u.dw.dwMax);
                SendMessage(hwndControl, UDM_SETRANGE32, (INT)Parameter->u.dw.dwMin, (INT)Parameter->u.dw.dwMax);
            }

            hwndControl = GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT);
            EnableWindow(hwndControl, Parameter->bPresent);
            ShowWindow(hwndControl, SW_SHOW);

            Style = GetWindowLongPtr(hwndControl, GWL_STYLE);
            Style |= ES_NUMBER;
            SetWindowLongPtr(hwndControl, GWL_STYLE, Style);

            Edit_LimitText(hwndControl, 0);

            if (Parameter->pszValue)
                Edit_SetText(hwndControl, Parameter->pszValue);
            else if (Parameter->pszDefault)
                Edit_SetText(hwndControl, Parameter->pszDefault);
            break;

        case EDIT_TYPE:
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST), SW_HIDE);

            hwndControl = GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT);
            EnableWindow(hwndControl, Parameter->bPresent);
            ShowWindow(hwndControl, SW_SHOW);

            Style = GetWindowLongPtr(hwndControl, GWL_STYLE);
            Style &= ~ES_NUMBER;
            if (Parameter->bUpperCase)
                Style |= ES_UPPERCASE;
            else
                Style &= ~ES_UPPERCASE;
            SetWindowLongPtr(hwndControl, GWL_STYLE, Style);

            Edit_LimitText(hwndControl, Parameter->iTextLimit);

            if (Parameter->pszValue)
                Edit_SetText(hwndControl, Parameter->pszValue);
            else if (Parameter->pszDefault)
                Edit_SetText(hwndControl, Parameter->pszDefault);
            break;

        case ENUM_TYPE:
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN), SW_HIDE);

            hwndControl = GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST);
            EnableWindow(hwndControl, Parameter->bPresent);
            ShowWindow(hwndControl, SW_SHOW);

            ComboBox_ResetContent(hwndControl);

            if (Parameter->pEnumOptions != NULL && Parameter->dwEnumOptions != 0)
            {
                for (i = 0; i < Parameter->dwEnumOptions; i++)
                {
                    ComboBox_AddString(hwndControl, Parameter->pEnumOptions[i].pszName);
                }
            }

            if (Parameter->pszValue)
            {
                idx = FindEnumOption(Parameter, Parameter->pszValue);
                if (idx != CB_ERR)
                    ComboBox_SetCurSel(hwndControl, idx);
            }
            else if (Parameter->pszDefault)
            {
                idx = FindEnumOption(Parameter, Parameter->pszDefault);
                if (idx != CB_ERR)
                    ComboBox_SetCurSel(hwndControl, idx);
            }
            break;

        default:
            break;
    }
}


static
BOOL
OnInitDialog(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    PPARAMETER_ARRAY pParamArray;
    HWND hwndControl;
    PWSTR pszText;
    DWORD i;
    INT idx;

    TRACE("OnInitDialog()\n");

    pParamArray = (PPARAMETER_ARRAY)((LPPROPSHEETPAGEW)lParam)->lParam;
    if (pParamArray == NULL)
    {
        ERR("pParamArray is NULL\n");
        return FALSE;
    }

    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pParamArray);

    hwndControl = GetDlgItem(hwnd, IDC_PROPERTY_NAME);
    if (hwndControl)
    {
        for (i = 0; i < pParamArray->dwCount; i++)
        {
            if (pParamArray->Array[i].pszDescription != NULL)
                pszText = pParamArray->Array[i].pszDescription;
            else
                pszText = pParamArray->Array[i].pszName;

            idx = ListBox_AddString(hwndControl, pszText);
            if (idx != LB_ERR)
                ListBox_SetItemData(hwndControl, idx, (LPARAM)&pParamArray->Array[i]);
        }

        if (pParamArray->dwCount > 0)
        {
            ListBox_SetCurSel(hwndControl, 0);
            pParamArray->pCurrentParam = (PPARAMETER)ListBox_GetItemData(hwndControl, 0);
            DisplayParameter(hwnd, pParamArray->pCurrentParam);
        }
    }

    return TRUE;
}


static
VOID
OnCommand(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    PPARAMETER_ARRAY pParamArray;
    INT iIndex;

    TRACE("OnCommand()\n");

    pParamArray = (PPARAMETER_ARRAY)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pParamArray == NULL)
    {
        ERR("pParamArray is NULL\n");
        return;
    }

    if ((LOWORD(wParam) == IDC_PROPERTY_NAME) && (HIWORD(wParam) == LBN_SELCHANGE))
    {
        if (pParamArray->pCurrentParam != NULL)
        {
            ReadParameterValue(hwnd, pParamArray->pCurrentParam);
        }

        iIndex = ListBox_GetCurSel((HWND)lParam);
        if (iIndex != LB_ERR && iIndex < pParamArray->dwCount)
        {
            pParamArray->pCurrentParam = (PPARAMETER)ListBox_GetItemData((HWND)lParam, iIndex);
            DisplayParameter(hwnd, pParamArray->pCurrentParam);
        }
    }
    else if ((LOWORD(wParam) == IDC_PROPERTY_PRESENT) && (HIWORD(wParam) == BN_CLICKED))
    {
        EnableWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST), TRUE);
        pParamArray->pCurrentParam->bPresent = TRUE;
    }
    else if ((LOWORD(wParam) == IDC_PROPERTY_NOT_PRESENT) && (HIWORD(wParam) == BN_CLICKED))
    {
        EnableWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST), FALSE);
        pParamArray->pCurrentParam->bPresent = FALSE;
    }
}


static
VOID
OnNotify(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    PPARAMETER_ARRAY pParamArray;

    TRACE("OnNotify()\n");

    pParamArray = (PPARAMETER_ARRAY)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pParamArray == NULL)
    {
        ERR("pParamArray is NULL\n");
        return;
    }

    if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY)
    {
        TRACE("PSN_APPLY!\n");
        WriteParameterArray(hwnd, pParamArray);
    }
    else if (((LPNMHDR)lParam)->code == (UINT)UDN_DELTAPOS)
    {
        LPNMUPDOWN pUpDown = (LPNMUPDOWN)lParam;
        pUpDown->iDelta *= pParamArray->pCurrentParam->iStep;
    }
}


static
VOID
OnDestroy(
    HWND hwnd)
{
    PPARAMETER_ARRAY pParamArray;

    TRACE("OnDestroy()\n");

    pParamArray = (PPARAMETER_ARRAY)GetWindowLongPtr(hwnd, DWLP_USER);
    if (pParamArray == NULL)
    {
        ERR("pParamArray is NULL\n");
        return;
    }

    FreeParameterArray(pParamArray);
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)NULL);
}


static
INT_PTR
CALLBACK
NetPropertyPageDlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return OnInitDialog(hwnd, wParam, lParam);

        case WM_COMMAND:
            OnCommand(hwnd, wParam, lParam);
            break;

        case WM_NOTIFY:
            OnNotify(hwnd, wParam, lParam);
            break;

        case WM_DESTROY:
            OnDestroy(hwnd);
            break;

        default:
            break;
    }

    return FALSE;
}


BOOL
WINAPI
NetPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;
    PPARAMETER_ARRAY ParameterArray = NULL;

    TRACE("NetPropPageProvider(%p %p %lx)\n",
          lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    if (!BuildParameterArray(lpPropSheetPageRequest->DeviceInfoSet,
                             lpPropSheetPageRequest->DeviceInfoData,
                             &ParameterArray))
        return FALSE;

    if (lpPropSheetPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        TRACE("SPPSR_ENUM_ADV_DEVICE_PROPERTIES\n");

        PropSheetPage.dwSize = sizeof(PROPSHEETPAGEW);
        PropSheetPage.dwFlags = 0;
        PropSheetPage.hInstance = netcfgx_hInstance;
        PropSheetPage.u.pszTemplate = MAKEINTRESOURCE(IDD_NET_PROPERTY_DLG);
        PropSheetPage.pfnDlgProc = NetPropertyPageDlgProc;
        PropSheetPage.lParam = (LPARAM)ParameterArray;
        PropSheetPage.pfnCallback = NULL;

        hPropSheetPage = CreatePropertySheetPageW(&PropSheetPage);
        if (hPropSheetPage == NULL)
        {
            ERR("CreatePropertySheetPageW() failed!\n");
            return FALSE;
        }

        if (!(*lpfnAddPropSheetPageProc)(hPropSheetPage, lParam))
        {
            ERR("lpfnAddPropSheetPageProc() failed!\n");
            DestroyPropertySheetPage(hPropSheetPage);
            return FALSE;
        }
    }

    TRACE("Done!\n");

    return TRUE;
}

/* EOF */
