/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/syssetup/proppage.c
 * PURPOSE:     Property page providers
 * PROGRAMMERS: Copyright 2018 Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct _MOUSE_INFO
{
    HKEY hDeviceKey;
    DWORD dwSampleRateIndex;
    DWORD dwWheelDetection;
    DWORD dwInputBufferLength;
    DWORD dwInitPolled;
} MOUSE_INFO, *PMOUSE_INFO;

DWORD MouseSampleRates[] = {20, 40, 60, 80, 100, 200};

#define DEFAULT_SAMPLERATEINDEX   4
#define DEFAULT_WHEELDETECTION    2
#define DEFAULT_INPUTBUFFERSIZE 100
#define DEFAULT_MINBUFFERSIZE   100
#define DEFAULT_MAXBUFFERSIZE   300

/*
 * @implemented
 */
BOOL
WINAPI
CdromPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("CdromPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DiskPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("DiskPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
EisaUpHalPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("EisaUpHalPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
LegacyDriverPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT1("LegacyDriverPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    UNIMPLEMENTED;
    return FALSE;
}


static
DWORD
MouseGetSampleRateIndex(
    DWORD dwSampleRate)
{
    DWORD i;

    for (i = 0; i < ARRAYSIZE(MouseSampleRates); i++)
    {
        if (MouseSampleRates[i] == dwSampleRate)
            return i;
    }

    return DEFAULT_SAMPLERATEINDEX;
}


static
BOOL
MouseOnDialogInit(
    HWND hwndDlg,
    LPARAM lParam)
{
    PMOUSE_INFO pMouseInfo;
    WCHAR szBuffer[64];
    UINT i;
    DWORD dwType, dwSize;
    DWORD dwSampleRate;
    LONG lError;

    /* Get the pointer to the mouse info struct */
    pMouseInfo = (PMOUSE_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
    if (pMouseInfo == NULL)
        return FALSE;

    /* Keep the pointer to the mouse info struct */
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pMouseInfo);

    /* Add the sample rates */
    for (i = 0; i < ARRAYSIZE(MouseSampleRates); i++)
    {
        wsprintf(szBuffer, L"%lu", MouseSampleRates[i]);
        SendDlgItemMessageW(hwndDlg,
                            IDC_PS2MOUSESAMPLERATE,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szBuffer);
    }

    /* Read the SampleRate parameter */
    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(pMouseInfo->hDeviceKey,
                              L"SampleRate",
                              NULL,
                              &dwType,
                              (LPBYTE)&dwSampleRate,
                              &dwSize);
    if (lError == ERROR_SUCCESS && dwType == REG_DWORD && dwSize == sizeof(DWORD))
    {
        pMouseInfo->dwSampleRateIndex = MouseGetSampleRateIndex(dwSampleRate);
    }
    else
    {
        /* Set the default sample rate (100 samples per second) */
        pMouseInfo->dwSampleRateIndex = DEFAULT_SAMPLERATEINDEX;
    }

    /* Set the sample rate */
    SendDlgItemMessageW(hwndDlg,
                        IDC_PS2MOUSESAMPLERATE,
                        CB_SETCURSEL,
                        pMouseInfo->dwSampleRateIndex,
                        0);

    /* Add the detection options */
    for (i = IDS_DETECTIONDISABLED; i <= IDS_ASSUMEPRESENT; i++)
    {
        LoadStringW(hDllInstance, i, szBuffer, ARRAYSIZE(szBuffer));
        SendDlgItemMessageW(hwndDlg,
                            IDC_PS2MOUSEWHEEL,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szBuffer);
    }

    /* Read the EnableWheelDetection parameter */
    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(pMouseInfo->hDeviceKey,
                              L"EnableWheelDetection",
                              NULL,
                              &dwType,
                              (LPBYTE)&pMouseInfo->dwWheelDetection,
                              &dwSize);
    if (lError != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
    {
        /* Set the default wheel detection parameter */
        pMouseInfo->dwWheelDetection = DEFAULT_WHEELDETECTION;
    }

    /* Set the wheel detection parameter */
    SendDlgItemMessageW(hwndDlg,
                        IDC_PS2MOUSEWHEEL,
                        CB_SETCURSEL,
                        pMouseInfo->dwWheelDetection,
                        0);

    /* Set the input buffer length range: 100-300 */
    SendDlgItemMessageW(hwndDlg,
                        IDC_PS2MOUSEINPUTUPDN,
                        UDM_SETRANGE32,
                        DEFAULT_MINBUFFERSIZE,
                        DEFAULT_MAXBUFFERSIZE);

    /* Read the MouseDataQueueSize parameter */
    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(pMouseInfo->hDeviceKey,
                              L"MouseDataQueueSize",
                              NULL,
                              &dwType,
                              (LPBYTE)&pMouseInfo->dwInputBufferLength,
                              &dwSize);
    if (lError != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
    {
        /* Set the default input buffer length (100 packets) */
        pMouseInfo->dwInputBufferLength = DEFAULT_INPUTBUFFERSIZE;
    }

    /* Set the input buffer length */
    SendDlgItemMessageW(hwndDlg,
                        IDC_PS2MOUSEINPUTUPDN,
                        UDM_SETPOS32,
                        0,
                        pMouseInfo->dwInputBufferLength);

    /* Read the MouseInitializePolled parameter */
    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(pMouseInfo->hDeviceKey,
                              L"MouseInitializePolled",
                              NULL,
                              &dwType,
                              (LPBYTE)&pMouseInfo->dwInitPolled,
                              &dwSize);
    if (lError != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
    {
        /* Set the default init polled value (FALSE) */
        pMouseInfo->dwInitPolled = 0;
    }

    /* Set the fast initialization option */
    SendDlgItemMessage(hwndDlg,
                       IDC_PS2MOUSEFASTINIT,
                       BM_SETCHECK,
                       (pMouseInfo->dwInitPolled == 0) ? BST_CHECKED : 0,
                       0);

    return TRUE;
}


static
VOID
MouseOnCommand(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_PS2MOUSESAMPLERATE:
        case IDC_PS2MOUSEWHEEL:
        case IDC_PS2MOUSEINPUTLEN:
        case IDC_PS2MOUSEFASTINIT:
            if (HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_EDITCHANGE)
            {
                /* Enable the Apply button */
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case IDC_PS2MOUSEDEFAULTS:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                /* Sample rate: 100 */
                SendDlgItemMessageW(hwndDlg,
                                    IDC_PS2MOUSESAMPLERATE,
                                    CB_SETCURSEL,
                                    DEFAULT_SAMPLERATEINDEX,
                                    0);

                /* Wheel detection: Assume wheel present */
                SendDlgItemMessageW(hwndDlg,
                                    IDC_PS2MOUSEWHEEL,
                                    CB_SETCURSEL,
                                    DEFAULT_WHEELDETECTION,
                                    0);

                /* Input buffer length: 100 packets */
                SendDlgItemMessageW(hwndDlg,
                                    IDC_PS2MOUSEINPUTUPDN,
                                    UDM_SETPOS32,
                                    0,
                                    DEFAULT_INPUTBUFFERSIZE);

                /* Fast Initialization: Checked */
                SendDlgItemMessage(hwndDlg,
                                   IDC_PS2MOUSEFASTINIT,
                                   BM_SETCHECK,
                                   (WPARAM)BST_CHECKED,
                                   (LPARAM)0);

                /* Enable the Apply button */
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        default:
            break;
    }
}


static
VOID
MouseOnApply(
    HWND hwndDlg)
{
    PMOUSE_INFO pMouseInfo;
    DWORD dwSampleRateIndex = 0;
    DWORD dwSampleRate, dwWheelDetection;
    DWORD dwInputBufferLength;
    DWORD dwInitPolled;
    UINT uValue;
    BOOL bFailed;
    INT nIndex;

    pMouseInfo = (PMOUSE_INFO)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    /* Get the sample rate setting and store it if it was changed */
    nIndex = SendDlgItemMessageW(hwndDlg, IDC_PS2MOUSESAMPLERATE,
                                 CB_GETCURSEL, 0, 0);
    if (nIndex == CB_ERR)
        dwSampleRateIndex = DEFAULT_SAMPLERATEINDEX;
    else
        dwSampleRateIndex = (DWORD)nIndex;

    if (dwSampleRateIndex != pMouseInfo->dwSampleRateIndex)
    {
        dwSampleRate = MouseSampleRates[dwSampleRateIndex];
        RegSetValueExW(pMouseInfo->hDeviceKey,
                       L"SampleRate",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwSampleRate,
                       sizeof(dwSampleRate));
    }

    /* Get the wheel detection setting and store it if it was changed */
    nIndex = SendDlgItemMessageW(hwndDlg, IDC_PS2MOUSEWHEEL,
                                 CB_GETCURSEL, 0, 0);
    if (nIndex == CB_ERR)
        dwWheelDetection = DEFAULT_WHEELDETECTION;
    else
        dwWheelDetection = (DWORD)nIndex;

    if (dwWheelDetection != pMouseInfo->dwWheelDetection)
    {
        RegSetValueExW(pMouseInfo->hDeviceKey,
                       L"EnableWheelDetection",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwWheelDetection,
                       sizeof(dwWheelDetection));
    }

    /* Get the input buffer length setting and store it if it was changed */
    uValue = SendDlgItemMessageW(hwndDlg,
                                 IDC_PS2MOUSEINPUTUPDN,
                                 UDM_GETPOS32,
                                 0,
                                 (LPARAM)&bFailed);
    if (bFailed)
        dwInputBufferLength = DEFAULT_INPUTBUFFERSIZE;
    else
        dwInputBufferLength = (DWORD)uValue;

    if (dwInputBufferLength != pMouseInfo->dwInputBufferLength)
    {
        RegSetValueExW(pMouseInfo->hDeviceKey,
                       L"MouseDataQueueSize",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwInputBufferLength,
                       sizeof(dwInputBufferLength));
    }

    /* Get the fast initialization setting and store it if it was changed */
    uValue = SendDlgItemMessage(hwndDlg,
                                IDC_PS2MOUSEFASTINIT,
                                BM_GETCHECK,
                                0,
                                0);
    dwInitPolled = (uValue == BST_CHECKED) ? 0 : 1;

    if (dwInitPolled != pMouseInfo->dwInitPolled)
    {
        RegSetValueExW(pMouseInfo->hDeviceKey,
                       L"MouseInitializePolled",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwInitPolled,
                       sizeof(dwInitPolled));
    }
}


static
BOOL
MouseOnNotify(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (((LPNMHDR)lParam)->code)
    {
        case UDN_DELTAPOS:
            if (((LPNMHDR)lParam)->idFrom == IDC_PS2MOUSEINPUTUPDN)
            {
                ((LPNMUPDOWN)lParam)->iDelta *= 10;
                return FALSE;
            }
            break;

        case PSN_APPLY:
            MouseOnApply(hwndDlg);
            return TRUE;
    }

    return FALSE;
}


static
INT_PTR
MouseOnCtrlColorStatic(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    if ((HWND)lParam != GetDlgItem(hwndDlg, IDC_PS2MOUSEINPUTLEN))
        return 0;

    SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
    return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
}


static
INT_PTR
CALLBACK
MouseDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DPRINT("MouseDlgProc\n");

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return MouseOnDialogInit(hwndDlg, lParam);

        case WM_COMMAND:
            MouseOnCommand(hwndDlg, wParam, lParam);
            break;

        case WM_NOTIFY:
            return MouseOnNotify(hwndDlg, wParam, lParam);

        case WM_CTLCOLORSTATIC:
            return MouseOnCtrlColorStatic(hwndDlg, wParam, lParam);
    }

    return FALSE;
}


static
UINT
CALLBACK
MouseCallback(
    HWND hWnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp)
{
    PMOUSE_INFO pMouseInfo;

    pMouseInfo = (PMOUSE_INFO)ppsp->lParam;

    if (uMsg == PSPCB_RELEASE)
    {
        if (pMouseInfo->hDeviceKey != NULL)
            RegCloseKey(pMouseInfo->hDeviceKey);
        HeapFree(GetProcessHeap(), 0, pMouseInfo);
    }

    return 1;
}


/*
 * @implemented
 */
BOOL
WINAPI
PS2MousePropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;
    PMOUSE_INFO pMouseInfo;

    DPRINT("PS2MousePropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    if (lpPropSheetPageRequest->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
        return FALSE;

    pMouseInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MOUSE_INFO));
    if (pMouseInfo == NULL)
        return FALSE;

    pMouseInfo->hDeviceKey = SetupDiOpenDevRegKey(lpPropSheetPageRequest->DeviceInfoSet,
                                                  lpPropSheetPageRequest->DeviceInfoData,
                                                  DICS_FLAG_GLOBAL,
                                                  0,
                                                  DIREG_DEV,
                                                  KEY_ALL_ACCESS);
    if (pMouseInfo->hDeviceKey == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupDiOpenDevRegKey() failed (Error %lu)\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, pMouseInfo);
        return FALSE;
    }

    PropSheetPage.dwSize = sizeof(PROPSHEETPAGEW);
    PropSheetPage.dwFlags = 0;
    PropSheetPage.hInstance = hDllInstance;
    PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PS2MOUSEPROPERTIES);
    PropSheetPage.pfnDlgProc = MouseDlgProc;
    PropSheetPage.lParam = (LPARAM)pMouseInfo;
    PropSheetPage.pfnCallback = MouseCallback;

    hPropSheetPage = CreatePropertySheetPageW(&PropSheetPage);
    if (hPropSheetPage == NULL)
    {
        DPRINT1("CreatePropertySheetPageW() failed!\n");
        HeapFree(GetProcessHeap(), 0, pMouseInfo);
        return FALSE;
    }

    if (!(*lpfnAddPropSheetPageProc)(hPropSheetPage, lParam))
    {
        DPRINT1("lpfnAddPropSheetPageProc() failed!\n");
        DestroyPropertySheetPage(hPropSheetPage);
        HeapFree(GetProcessHeap(), 0, pMouseInfo);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
TapePropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("TapePropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}

/* EOF */
