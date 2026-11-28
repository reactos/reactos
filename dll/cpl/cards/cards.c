/* User-wide appearance settings for games using cards.dll. */
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <winerror.h>
#include <cpl.h>
#include <cardgames.h>
#include "resource.h"

static HINSTANCE instance;

static INT_PTR CALLBACK CardsDlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    HKEY key;
    DWORD style, type, size;
    LONG error;
    WCHAR message[256], title[128];

    UNREFERENCED_PARAMETER(lp);
    switch (msg)
    {
        case WM_INITDIALOG:
            style = CARDGAMES_DEFAULT;
            size = sizeof(style);
            if (RegOpenKeyExW(HKEY_CURRENT_USER, CARDGAMES_REGKEY, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
            {
                if (RegQueryValueExW(key, CARDGAMES_STYLE_VALUE, NULL, &type,
                                    (BYTE *)&style, &size) != ERROR_SUCCESS ||
                    type != REG_DWORD || size != sizeof(style) || style != CARDGAMES_BAVARIAN)
                    style = CARDGAMES_DEFAULT;
                RegCloseKey(key);
            }
            CheckRadioButton(hwnd, IDC_DEFAULT, IDC_BAVARIAN,
                             style == CARDGAMES_BAVARIAN ? IDC_BAVARIAN : IDC_DEFAULT);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wp))
            {
                case IDOK:
                    style = IsDlgButtonChecked(hwnd, IDC_BAVARIAN) == BST_CHECKED ?
                            CARDGAMES_BAVARIAN : CARDGAMES_DEFAULT;
                    error = RegCreateKeyExW(HKEY_CURRENT_USER, CARDGAMES_REGKEY, 0, NULL,
                                            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, NULL);
                    if (error == ERROR_SUCCESS)
                    {
                        error = RegSetValueExW(key, CARDGAMES_STYLE_VALUE, 0, REG_DWORD,
                                               (const BYTE *)&style, sizeof(style));
                        RegCloseKey(key);
                    }
                    if (error != ERROR_SUCCESS)
                    {
                        LoadStringW(instance, IDS_SAVE_ERROR, message, ARRAYSIZE(message));
                        LoadStringW(instance, IDS_NAME, title, ARRAYSIZE(title));
                        MessageBoxW(hwnd, message, title, MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                    EndDialog(hwnd, IDOK);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

LONG CALLBACK CPlApplet(HWND hwnd, UINT msg, LPARAM lp1, LPARAM lp2)
{
    UNREFERENCED_PARAMETER(lp1);
    switch (msg)
    {
        case CPL_INIT:
            return TRUE;
        case CPL_GETCOUNT:
            return 1;
        case CPL_INQUIRE:
        {
            CPLINFO *info = (CPLINFO *)lp2;
            info->idIcon = IDI_CARDS;
            info->idName = IDS_NAME;
            info->idInfo = IDS_DESCRIPTION;
            info->lData = 0;
            break;
        }
        case CPL_DBLCLK:
            DialogBoxW(instance, MAKEINTRESOURCEW(IDD_CARDS), hwnd, CardsDlgProc);
            break;
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(reserved);
    if (reason == DLL_PROCESS_ATTACH)
        instance = module;
    return TRUE;
}
