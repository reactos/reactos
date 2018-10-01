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

typedef struct _PARAMETER
{
    PWSTR pszName;
    PWSTR pszDescription;
    PWSTR pszValue;
    PWSTR pszDefault;
    PARAM_TYPE Type;
    BOOL bUpperCase;
    BOOL bOptional;
    INT iLimitText;

} PARAMETER, *PPARAMETER;

typedef struct _PARAMETER_ARRAY
{
    DWORD dwCount;
    PARAMETER Array[0];
} PARAMETER_ARRAY, *PPARAMETER_ARRAY;


static
VOID
FreeParameterArray(
    _In_ PPARAMETER_ARRAY ParamArray)
{
    INT i;

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

    }

    HeapFree(GetProcessHeap(), 0, ParamArray);
}


static DWORD
GetValueString(
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    OUT LPWSTR *lpString)
{
    LPWSTR lpBuffer;
    DWORD dwLength = 0;
    DWORD dwRegType;
    DWORD rc;

    *lpString = NULL;

    RegQueryValueExW(hKey, lpValueName, NULL, &dwRegType, NULL, &dwLength);

    if (dwLength == 0 || dwRegType != REG_SZ)
        return ERROR_FILE_NOT_FOUND;

    lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength + sizeof(WCHAR));
    if (lpBuffer == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    rc = RegQueryValueExW(hKey, lpValueName, NULL, NULL, (LPBYTE)lpBuffer, &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, lpBuffer);
        return rc;
    }

    lpBuffer[dwLength / sizeof(WCHAR)] = UNICODE_NULL;

    *lpString = lpBuffer;

    return ERROR_SUCCESS;
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

    FIXME("Sub keys: %lu\n", dwSubKeys);

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

        FIXME("Sub key '%S'\n", ParamArray->Array[dwIndex].pszName);

        lError = RegOpenKeyExW(hParamsKey,
                               ParamArray->Array[dwIndex].pszName,
                               0,
                               KEY_READ,
                               &hParamKey);
        if (lError == ERROR_SUCCESS)
        {
            GetValueString(hParamKey,
                           L"ParamDesc",
                           &ParamArray->Array[dwIndex].pszDescription);

            GetValueString(hParamKey,
                           L"Type",
                           &pszType);
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

            GetValueString(hParamKey,
                           L"Default",
                           &ParamArray->Array[dwIndex].pszDefault);

            ParamArray->Array[dwIndex].bUpperCase = FALSE;
            ParamArray->Array[dwIndex].bOptional = FALSE;
            ParamArray->Array[dwIndex].iLimitText = 0;

            RegCloseKey(hParamKey);
        }

        GetValueString(hDriverKey,
                       ParamArray->Array[dwIndex].pszName,
                       &ParamArray->Array[dwIndex].pszValue);
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
DisplayParameter(
    HWND hwnd,
    PPARAMETER Parameter)
{
    LONG_PTR Style;

    ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_PRESENT), (Parameter->bOptional) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_NOT_PRESENT), (Parameter->bOptional) ? SW_SHOW : SW_HIDE);

    switch (Parameter->Type)
    {
        case INT_TYPE:
        case LONG_TYPE:
        case WORD_TYPE:
        case DWORD_TYPE:
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST), SW_HIDE);

            Style = GetWindowLongPtr(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), GWL_STYLE);
            Style |= ES_NUMBER;
            SetWindowLongPtr(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), GWL_STYLE, Style);

            Edit_LimitText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), 0);

            if (Parameter->pszValue)
                Edit_SetText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), Parameter->pszValue);
            else if (Parameter->pszDefault)
                Edit_SetText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), Parameter->pszDefault);
            break;

        case EDIT_TYPE:
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST), SW_HIDE);

            Style = GetWindowLongPtr(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), GWL_STYLE);
            Style &= ~ES_NUMBER;
            if (Parameter->bUpperCase)
                Style |= ES_UPPERCASE;
            else
                Style &= ~ES_UPPERCASE;
            SetWindowLongPtr(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), GWL_STYLE, Style);

            Edit_LimitText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), Parameter->iLimitText);

            if (Parameter->pszValue)
                Edit_SetText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), Parameter->pszValue);
            else if (Parameter->pszDefault)
                Edit_SetText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), Parameter->pszDefault);
            break;

        case ENUM_TYPE:
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_UPDN), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_LIST), SW_SHOW);

            if (Parameter->pszValue)
                Edit_SetText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), Parameter->pszValue);
            else if (Parameter->pszDefault)
                Edit_SetText(GetDlgItem(hwnd, IDC_PROPERTY_VALUE_EDIT), Parameter->pszDefault);
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

    FIXME("OnInitDialog()\n");

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

            ListBox_AddString(hwndControl, pszText);
        }

        if (pParamArray->dwCount > 0)
        {
            ListBox_SetCurSel(hwndControl, 0);
            DisplayParameter(hwnd, &pParamArray->Array[0]);
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
        iIndex = ListBox_GetCurSel((HWND)lParam);
        if (iIndex != LB_ERR && iIndex < pParamArray->dwCount)
        {
            DisplayParameter(hwnd, &pParamArray->Array[iIndex]);
        }
    }
}


static
VOID
OnDestroy(
    HWND hwnd)
{
    PPARAMETER_ARRAY pParamArray;

    FIXME("OnDestroy()\n");

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

    ERR("NetPropPageProvider(%p %p %lx)\n",
          lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    if (!BuildParameterArray(lpPropSheetPageRequest->DeviceInfoSet,
                             lpPropSheetPageRequest->DeviceInfoData,
                             &ParameterArray))
        return FALSE;

    if (lpPropSheetPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        ERR("SPPSR_ENUM_ADV_DEVICE_PROPERTIES\n");

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

    ERR("Done!\n");

    return TRUE;
}

/* EOF */
