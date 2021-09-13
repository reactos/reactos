#include "openglcfg.h"

#include <winreg.h>
#include <debug.h>

static PWCHAR *pOglDrivers = NULL;
static DWORD dwNumDrivers = 0;

static VOID InitSettings(HWND hWndDlg)
{
    HKEY hKeyRenderer;
    HKEY hKeyDrivers;
    DWORD dwType = 0;
    DWORD dwSize = MAX_KEY_LENGTH;
    WCHAR szBuffer[MAX_KEY_LENGTH];
    WCHAR szBultin[MAX_KEY_LENGTH];
    WCHAR szDriver[MAX_KEY_LENGTH];

    LoadString(hApplet, IDS_DEBUG_DNM, (LPTSTR)szBultin, 127);
    SendDlgItemMessageW(hWndDlg, IDC_DEBUG_OUTPUT, CB_ADDSTRING, 0, (LPARAM)szBultin);

    LoadString(hApplet, IDS_DEBUG_SET, (LPTSTR)szBultin, 127);
    SendDlgItemMessageW(hWndDlg, IDC_DEBUG_OUTPUT, CB_ADDSTRING, 0, (LPARAM)szBultin);

    LoadString(hApplet, IDS_DEBUG_CLEAR, (LPTSTR)szBultin, 127);
    SendDlgItemMessageW(hWndDlg, IDC_DEBUG_OUTPUT, CB_ADDSTRING, 0, (LPARAM)szBultin);

    SendDlgItemMessageW(hWndDlg, IDC_DEBUG_OUTPUT, CB_SETCURSEL, 0, 0);

    LoadString(hApplet, IDS_RENDERER_DEFAULT, (LPTSTR)szBultin, 127);
    SendDlgItemMessageW(hWndDlg, IDC_RENDERER, CB_ADDSTRING, 0, (LPARAM)szBultin);

    LoadString(hApplet, IDS_RENDERER_RSWR, (LPTSTR)szBultin, 127);
    SendDlgItemMessageW(hWndDlg, IDC_RENDERER, CB_ADDSTRING, 0, (LPARAM)szBultin);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, KEY_RENDERER, 0, NULL, 0, MAXIMUM_ALLOWED, NULL, &hKeyRenderer, NULL) != ERROR_SUCCESS)
        return;

    if (RegQueryValueExW(hKeyRenderer, NULL, NULL, &dwType, (LPBYTE)szDriver, &dwSize) != ERROR_SUCCESS || dwSize == sizeof(WCHAR))
        SendDlgItemMessageW(hWndDlg, IDC_RENDERER, CB_SETCURSEL, RENDERER_DEFAULT, 0);

    RegCloseKey(hKeyRenderer);

    if (dwType == REG_SZ)
    {
        DWORD ret;
        DWORD iKey;

        if (wcsncmp(szBultin, szDriver, MAX_KEY_LENGTH) == 0)
            SendDlgItemMessageW(hWndDlg, IDC_RENDERER, CB_SETCURSEL, RENDERER_RSWR, 0);

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_DRIVERS, 0, KEY_READ, &hKeyDrivers) != ERROR_SUCCESS)
            return;

        ret = RegQueryInfoKeyW(hKeyDrivers, NULL, NULL, NULL, &dwNumDrivers, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if (ret != ERROR_SUCCESS || dwNumDrivers == 0)
        {
            RegCloseKey(hKeyDrivers);
            return;
        }

        pOglDrivers = HeapAlloc(GetProcessHeap(), 0, dwNumDrivers * sizeof(PWCHAR));

        if (!pOglDrivers)
            dwNumDrivers = 0;

        for (iKey = 0; iKey < dwNumDrivers; iKey++)
        {
            dwSize = MAX_KEY_LENGTH;
            ret = RegEnumKeyEx(hKeyDrivers, iKey, szBuffer, &dwSize, NULL, NULL, NULL, NULL);

            if (ret != ERROR_SUCCESS)
                break;

            /* Mind the null terminator */
            dwSize++;

            pOglDrivers[iKey] = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));

            if (!pOglDrivers[iKey])
                break;

            SendDlgItemMessageW(hWndDlg, IDC_RENDERER, CB_ADDSTRING, 0, (LPARAM)szBuffer);

            StringCchCopy(pOglDrivers[iKey], dwSize, szBuffer);

            if (wcsncmp(szBuffer, szDriver, MAX_KEY_LENGTH) == 0)
                SendDlgItemMessageW(hWndDlg, IDC_RENDERER, CB_SETCURSEL, iKey + 2, 0);
        }

        RegCloseKey(hKeyDrivers);
    }

    return;
}

static VOID SaveSettings(HWND hWndDlg)
{
    HKEY hKeyRenderer;
    HKEY hKeyDebug;
    INT iSel = 0;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_RENDERER, 0, KEY_WRITE, &hKeyRenderer) != ERROR_SUCCESS)
        return;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_DEBUG_CHANNEL, 0, KEY_WRITE, &hKeyDebug) == ERROR_SUCCESS)
    {
        iSel = (INT)SendDlgItemMessageW(hWndDlg, IDC_DEBUG_OUTPUT, CB_GETCURSEL, 0, 0);

        switch (iSel)
        {
            case DEBUG_SET:
                RegSetValueExW(hKeyDebug, L"DEBUGCHANNEL", 0, REG_SZ, (PBYTE)L"+opengl,+wgl", 13 * sizeof(WCHAR));
                break;

            case DEBUG_CLEAR:
                RegSetValueExW(hKeyDebug, L"DEBUGCHANNEL", 0, REG_SZ, (PBYTE)L"", sizeof(WCHAR));
                break;
        }
        RegCloseKey(hKeyDebug);
    }

    iSel = (INT)SendDlgItemMessageW(hWndDlg, IDC_RENDERER, CB_GETCURSEL, 0, 0);

    switch (iSel)
    {
        case CB_ERR:
            break;

        case RENDERER_DEFAULT:
            RegSetValueExW(hKeyRenderer, L"", 0, REG_SZ, (PBYTE)L"", sizeof(WCHAR));
            break;

        case RENDERER_RSWR:
        {
            WCHAR szBuffer[MAX_KEY_LENGTH];
            LoadString(hApplet, IDS_RENDERER_RSWR, (LPTSTR)szBuffer, 127);
            RegSetValueExW(hKeyRenderer, L"", 0, REG_SZ, (PBYTE)szBuffer, (DWORD)((wcslen(szBuffer) + 1) * sizeof(WCHAR)));
            break;
        }

        default:
        {
            /* Adjustment for DEFAULT and RSWR renderers */
            iSel -= 2;

            if (iSel >= 0 && iSel < dwNumDrivers)
                RegSetValueExW(hKeyRenderer, L"", 0, REG_SZ, (PBYTE)pOglDrivers[iSel], (DWORD)((wcslen(pOglDrivers[iSel]) + 1) * sizeof(WCHAR)));

            break;
        }
    }

    RegCloseKey(hKeyRenderer);
}


INT_PTR CALLBACK GeneralPageProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPPSHNOTIFY lppsn;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            InitSettings(hWndDlg);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_RENDERER ||
                LOWORD(wParam) == IDC_DEBUG_OUTPUT)
            {
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    PropSheet_Changed(GetParent(hWndDlg), hWndDlg);
                }
            }
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                SaveSettings(hWndDlg);
                return TRUE;
            }
            break;

        case WM_DESTROY:
            if (pOglDrivers != NULL)
            {
                DWORD iKey;

                for (iKey = 0; iKey < dwNumDrivers; ++iKey)
                    HeapFree(GetProcessHeap(), 0, pOglDrivers[iKey]);

                HeapFree(GetProcessHeap(), 0, pOglDrivers);
            }
    }

    return FALSE;
}
