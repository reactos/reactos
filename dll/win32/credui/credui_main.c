/*
 * Credentials User Interface
 *
 * Copyright 2006 Robert Shearman (for CodeWeavers)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winuser.h"
#include "wincred.h"
#include "rpc.h"
#include "sspi.h"
#include "commctrl.h"

#include "credui_resources.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(credui);

#define TOOLID_INCORRECTPASSWORD    1
#define TOOLID_CAPSLOCKON           2

#define ID_CAPSLOCKPOP              1

struct pending_credentials
{
    struct list entry;
    PWSTR pszTargetName;
    PWSTR pszUsername;
    PWSTR pszPassword;
    BOOL generic;
};

static HINSTANCE hinstCredUI;

static struct list pending_credentials_list = LIST_INIT(pending_credentials_list);

static CRITICAL_SECTION csPendingCredentials;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csPendingCredentials,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": csPendingCredentials") }
};
static CRITICAL_SECTION csPendingCredentials = { &critsect_debug, -1, 0, 0, 0, 0 };


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    struct pending_credentials *entry, *cursor2;
    TRACE("(0x%p, %ld, %p)\n",hinstDLL,fdwReason,lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        hinstCredUI = hinstDLL;
        InitCommonControls();
        break;

    case DLL_PROCESS_DETACH:
        if (lpvReserved) break;
        LIST_FOR_EACH_ENTRY_SAFE(entry, cursor2, &pending_credentials_list, struct pending_credentials, entry)
        {
            list_remove(&entry->entry);

            HeapFree(GetProcessHeap(), 0, entry->pszTargetName);
            HeapFree(GetProcessHeap(), 0, entry->pszUsername);
            SecureZeroMemory(entry->pszPassword, lstrlenW(entry->pszPassword) * sizeof(WCHAR));
            HeapFree(GetProcessHeap(), 0, entry->pszPassword);
            HeapFree(GetProcessHeap(), 0, entry);
        }
        DeleteCriticalSection(&csPendingCredentials);
        break;
    }

    return TRUE;
}

static DWORD save_credentials(PCWSTR pszTargetName, PCWSTR pszUsername,
                              PCWSTR pszPassword, BOOL generic)
{
    CREDENTIALW cred;

    TRACE("saving servername %s with username %s\n", debugstr_w(pszTargetName), debugstr_w(pszUsername));

    cred.Flags = 0;
    cred.Type = generic ? CRED_TYPE_GENERIC : CRED_TYPE_DOMAIN_PASSWORD;
    cred.TargetName = (LPWSTR)pszTargetName;
    cred.Comment = NULL;
    cred.CredentialBlobSize = lstrlenW(pszPassword) * sizeof(WCHAR);
    cred.CredentialBlob = (LPBYTE)pszPassword;
    cred.Persist = CRED_PERSIST_ENTERPRISE;
    cred.AttributeCount = 0;
    cred.Attributes = NULL;
    cred.TargetAlias = NULL;
    cred.UserName = (LPWSTR)pszUsername;

    if (CredWriteW(&cred, 0))
        return ERROR_SUCCESS;
    else
    {
        DWORD ret = GetLastError();
        ERR("CredWriteW failed with error %ld\n", ret);
        return ret;
    }
}

struct cred_dialog_params
{
    PCWSTR pszTargetName;
    PCWSTR pszMessageText;
    PCWSTR pszCaptionText;
    HBITMAP hbmBanner;
    PWSTR pszUsername;
    ULONG ulUsernameMaxChars;
    PWSTR pszPassword;
    ULONG ulPasswordMaxChars;
    BOOL fSave;
    DWORD dwFlags;
    HWND hwndBalloonTip;
    BOOL fBalloonTipActive;
};

static void CredDialogFillUsernameCombo(HWND hwndUsername, const struct cred_dialog_params *params)
{
    DWORD count;
    DWORD i;
    PCREDENTIALW *credentials;

    if (!CredEnumerateW(NULL, 0, &count, &credentials))
        return;

    for (i = 0; i < count; i++)
    {
        COMBOBOXEXITEMW comboitem;
        DWORD j;
        BOOL duplicate = FALSE;

        if (!credentials[i]->UserName)
            continue;

        if (params->dwFlags & CREDUI_FLAGS_GENERIC_CREDENTIALS)
        {
            if (credentials[i]->Type != CRED_TYPE_GENERIC)
            {
                credentials[i]->UserName = NULL;
                continue;
            }
        }
        else if (credentials[i]->Type == CRED_TYPE_GENERIC)
        {
            credentials[i]->UserName = NULL;
            continue;
        }

        /* don't add another item with the same name if we've already added it */
        for (j = 0; j < i; j++)
            if (credentials[j]->UserName
                && !lstrcmpW(credentials[i]->UserName, credentials[j]->UserName))
            {
                duplicate = TRUE;
                break;
            }

        if (duplicate)
            continue;

        comboitem.mask = CBEIF_TEXT;
        comboitem.iItem = -1;
        comboitem.pszText = credentials[i]->UserName;
        SendMessageW(hwndUsername, CBEM_INSERTITEMW, 0, (LPARAM)&comboitem);
    }

    CredFree(credentials);
}

static void CredDialogCreateBalloonTip(HWND hwndDlg, struct cred_dialog_params *params)
{
    TTTOOLINFOW toolinfo;
    WCHAR wszText[256];

    if (params->hwndBalloonTip)
        return;

    params->hwndBalloonTip = CreateWindowExW(WS_EX_TOOLWINDOW, TOOLTIPS_CLASSW,
        NULL, WS_POPUP | TTS_NOPREFIX | TTS_BALLOON, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwndDlg, NULL,
        hinstCredUI, NULL);
    SetWindowPos(params->hwndBalloonTip, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    if (!LoadStringW(hinstCredUI, IDS_INCORRECTPASSWORD, wszText, ARRAY_SIZE(wszText)))
    {
        ERR("failed to load IDS_INCORRECTPASSWORD\n");
        return;
    }

    toolinfo.cbSize = sizeof(toolinfo);
    toolinfo.uFlags = TTF_TRACK;
    toolinfo.hwnd = hwndDlg;
    toolinfo.uId = TOOLID_INCORRECTPASSWORD;
    SetRectEmpty(&toolinfo.rect);
    toolinfo.hinst = NULL;
    toolinfo.lpszText = wszText;
    toolinfo.lParam = 0;
    toolinfo.lpReserved = NULL;
    SendMessageW(params->hwndBalloonTip, TTM_ADDTOOLW, 0, (LPARAM)&toolinfo);

    if (!LoadStringW(hinstCredUI, IDS_CAPSLOCKON, wszText, ARRAY_SIZE(wszText)))
    {
        ERR("failed to load IDS_CAPSLOCKON\n");
        return;
    }

    toolinfo.uId = TOOLID_CAPSLOCKON;
    SendMessageW(params->hwndBalloonTip, TTM_ADDTOOLW, 0, (LPARAM)&toolinfo);
}

static void CredDialogShowIncorrectPasswordBalloon(HWND hwndDlg, struct cred_dialog_params *params)
{
    TTTOOLINFOW toolinfo;
    RECT rcPassword;
    INT x;
    INT y;
    WCHAR wszTitle[256];

    /* user name likely wrong so balloon would be confusing. focus is also
     * not set to the password edit box, so more notification would need to be
     * handled */
    if (!params->pszUsername[0])
        return;

    /* don't show two balloon tips at once */
    if (params->fBalloonTipActive)
        return;

    if (!LoadStringW(hinstCredUI, IDS_INCORRECTPASSWORDTITLE, wszTitle, ARRAY_SIZE(wszTitle)))
    {
        ERR("failed to load IDS_INCORRECTPASSWORDTITLE\n");
        return;
    }

    CredDialogCreateBalloonTip(hwndDlg, params);

    memset(&toolinfo, 0, sizeof(toolinfo));
    toolinfo.cbSize = sizeof(toolinfo);
    toolinfo.hwnd = hwndDlg;
    toolinfo.uId = TOOLID_INCORRECTPASSWORD;

    SendMessageW(params->hwndBalloonTip, TTM_SETTITLEW, TTI_ERROR, (LPARAM)wszTitle);

    GetWindowRect(GetDlgItem(hwndDlg, IDC_PASSWORD), &rcPassword);
    /* centered vertically and in the right side of the password edit control */
    x = rcPassword.right - 12;
    y = (rcPassword.top + rcPassword.bottom) / 2;
    SendMessageW(params->hwndBalloonTip, TTM_TRACKPOSITION, 0, MAKELONG(x, y));

    SendMessageW(params->hwndBalloonTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolinfo);

    params->fBalloonTipActive = TRUE;
}

static void CredDialogShowCapsLockBalloon(HWND hwndDlg, struct cred_dialog_params *params)
{
    TTTOOLINFOW toolinfo;
    RECT rcPassword;
    INT x;
    INT y;
    WCHAR wszTitle[256];

    /* don't show two balloon tips at once */
    if (params->fBalloonTipActive)
        return;

    if (!LoadStringW(hinstCredUI, IDS_CAPSLOCKONTITLE, wszTitle, ARRAY_SIZE(wszTitle)))
    {
        ERR("failed to load IDS_IDSCAPSLOCKONTITLE\n");
        return;
    }

    CredDialogCreateBalloonTip(hwndDlg, params);

    memset(&toolinfo, 0, sizeof(toolinfo));
    toolinfo.cbSize = sizeof(toolinfo);
    toolinfo.hwnd = hwndDlg;
    toolinfo.uId = TOOLID_CAPSLOCKON;

    SendMessageW(params->hwndBalloonTip, TTM_SETTITLEW, TTI_WARNING, (LPARAM)wszTitle);

    GetWindowRect(GetDlgItem(hwndDlg, IDC_PASSWORD), &rcPassword);
    /* just inside the left side of the password edit control */
    x = rcPassword.left + 12;
    y = rcPassword.bottom - 3;
    SendMessageW(params->hwndBalloonTip, TTM_TRACKPOSITION, 0, MAKELONG(x, y));

    SendMessageW(params->hwndBalloonTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolinfo);

    SetTimer(hwndDlg, ID_CAPSLOCKPOP,
             SendMessageW(params->hwndBalloonTip, TTM_GETDELAYTIME, TTDT_AUTOPOP, 0),
             NULL);

    params->fBalloonTipActive = TRUE;
}

static void CredDialogHideBalloonTip(HWND hwndDlg, struct cred_dialog_params *params)
{
    TTTOOLINFOW toolinfo;

    if (!params->hwndBalloonTip)
        return;

    memset(&toolinfo, 0, sizeof(toolinfo));

    toolinfo.cbSize = sizeof(toolinfo);
    toolinfo.hwnd = hwndDlg;
    toolinfo.uId = 0;
    SendMessageW(params->hwndBalloonTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolinfo);
    toolinfo.uId = 1;
    SendMessageW(params->hwndBalloonTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolinfo);

    params->fBalloonTipActive = FALSE;
}

static inline BOOL CredDialogCapsLockOn(void)
{
    return (GetKeyState(VK_CAPITAL) & 0x1) != 0;
}

static LRESULT CALLBACK CredDialogPasswordSubclassProc(HWND hwnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    struct cred_dialog_params *params = (struct cred_dialog_params *)dwRefData;
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_CAPITAL)
        {
            HWND hwndDlg = GetParent(hwnd);
            if (CredDialogCapsLockOn())
                CredDialogShowCapsLockBalloon(hwndDlg, params);
            else
                CredDialogHideBalloonTip(hwndDlg, params);
        }
        break;
    case WM_DESTROY:
        RemoveWindowSubclass(hwnd, CredDialogPasswordSubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static BOOL CredDialogInit(HWND hwndDlg, struct cred_dialog_params *params)
{
    HWND hwndUsername = GetDlgItem(hwndDlg, IDC_USERNAME);
    HWND hwndPassword = GetDlgItem(hwndDlg, IDC_PASSWORD);

    SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)params);

    if (params->hbmBanner)
        SendMessageW(GetDlgItem(hwndDlg, IDB_BANNER), STM_SETIMAGE,
                     IMAGE_BITMAP, (LPARAM)params->hbmBanner);

    if (params->pszMessageText)
        SetDlgItemTextW(hwndDlg, IDC_MESSAGE, params->pszMessageText);
    else
    {
        WCHAR format[256];
        WCHAR message[256];
        LoadStringW(hinstCredUI, IDS_MESSAGEFORMAT, format, ARRAY_SIZE(format));
        swprintf(message, ARRAY_SIZE(message), format, params->pszTargetName);
        SetDlgItemTextW(hwndDlg, IDC_MESSAGE, message);
    }
    SetWindowTextW(hwndUsername, params->pszUsername);
    SetWindowTextW(hwndPassword, params->pszPassword);

    CredDialogFillUsernameCombo(hwndUsername, params);

    if (params->pszUsername[0])
    {
        /* prevent showing a balloon tip here */
        params->fBalloonTipActive = TRUE;
        SetFocus(hwndPassword);
        params->fBalloonTipActive = FALSE;
    }
    else
        SetFocus(hwndUsername);

    if (params->pszCaptionText)
        SetWindowTextW(hwndDlg, params->pszCaptionText);
    else
    {
        WCHAR format[256];
        WCHAR title[256];
        LoadStringW(hinstCredUI, IDS_TITLEFORMAT, format, ARRAY_SIZE(format));
        swprintf(title, ARRAY_SIZE(title), format, params->pszTargetName);
        SetWindowTextW(hwndDlg, title);
    }

    if (params->dwFlags & CREDUI_FLAGS_PERSIST ||
        (params->dwFlags & CREDUI_FLAGS_DO_NOT_PERSIST &&
         !(params->dwFlags & CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX)))
        ShowWindow(GetDlgItem(hwndDlg, IDC_SAVE), SW_HIDE);
    else if (params->fSave)
        CheckDlgButton(hwndDlg, IDC_SAVE, BST_CHECKED);

    /* setup subclassing for Caps Lock detection */
    SetWindowSubclass(hwndPassword, CredDialogPasswordSubclassProc, 1, (DWORD_PTR)params);

    if (params->dwFlags & CREDUI_FLAGS_INCORRECT_PASSWORD)
        CredDialogShowIncorrectPasswordBalloon(hwndDlg, params);
    else if ((GetFocus() == hwndPassword) && CredDialogCapsLockOn())
        CredDialogShowCapsLockBalloon(hwndDlg, params);

    return FALSE;
}

static void CredDialogCommandOk(HWND hwndDlg, struct cred_dialog_params *params)
{
    HWND hwndUsername = GetDlgItem(hwndDlg, IDC_USERNAME);
    LPWSTR user;
    INT len;
    INT len2;

    len = GetWindowTextLengthW(hwndUsername);
    user = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    GetWindowTextW(hwndUsername, user, len + 1);

    if (!user[0])
    {
        HeapFree(GetProcessHeap(), 0, user);
        return;
    }

    if (!wcschr(user, '\\') && !wcschr(user, '@'))
    {
        ULONG len_target = lstrlenW(params->pszTargetName);
        memcpy(params->pszUsername, params->pszTargetName,
               min(len_target, params->ulUsernameMaxChars) * sizeof(WCHAR));
        if (len_target + 1 < params->ulUsernameMaxChars)
            params->pszUsername[len_target] = '\\';
        if (len_target + 2 < params->ulUsernameMaxChars)
            params->pszUsername[len_target + 1] = '\0';
    }
    else if (params->ulUsernameMaxChars > 0)
        params->pszUsername[0] = '\0';

    len2 = lstrlenW(params->pszUsername);
    memcpy(params->pszUsername + len2, user, min(len, params->ulUsernameMaxChars - len2) * sizeof(WCHAR));
    if (params->ulUsernameMaxChars)
        params->pszUsername[len2 + min(len, params->ulUsernameMaxChars - len2 - 1)] = '\0';

    HeapFree(GetProcessHeap(), 0, user);

    GetDlgItemTextW(hwndDlg, IDC_PASSWORD, params->pszPassword,
                    params->ulPasswordMaxChars);

    params->fSave = IsDlgButtonChecked(hwndDlg, IDC_SAVE) == BST_CHECKED;

    EndDialog(hwndDlg, IDOK);
}

static INT_PTR CALLBACK CredDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                       LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            struct cred_dialog_params *params = (struct cred_dialog_params *)lParam;

            return CredDialogInit(hwndDlg, params);
        }
        case WM_COMMAND:
            switch (wParam)
            {
                case MAKELONG(IDOK, BN_CLICKED):
                {
                    struct cred_dialog_params *params =
                        (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                    CredDialogCommandOk(hwndDlg, params);
                    return TRUE;
                }
                case MAKELONG(IDCANCEL, BN_CLICKED):
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
                case MAKELONG(IDC_PASSWORD, EN_SETFOCUS):
                    if (CredDialogCapsLockOn())
                    {
                        struct cred_dialog_params *params =
                            (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                        CredDialogShowCapsLockBalloon(hwndDlg, params);
                    }
                    /* don't allow another window to steal focus while the
                     * user is typing their password */
                    LockSetForegroundWindow(LSFW_LOCK);
                    return TRUE;
                case MAKELONG(IDC_PASSWORD, EN_KILLFOCUS):
                {
                    struct cred_dialog_params *params =
                        (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                    /* the user is no longer typing their password, so allow
                     * other windows to become foreground ones */
                    LockSetForegroundWindow(LSFW_UNLOCK);
                    CredDialogHideBalloonTip(hwndDlg, params);
                    return TRUE;
                }
                case MAKELONG(IDC_PASSWORD, EN_CHANGE):
                {
                    struct cred_dialog_params *params =
                        (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                    CredDialogHideBalloonTip(hwndDlg, params);
                    return TRUE;
                }
            }
            return FALSE;
        case WM_TIMER:
            if (wParam == ID_CAPSLOCKPOP)
            {
                struct cred_dialog_params *params =
                    (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
                CredDialogHideBalloonTip(hwndDlg, params);
                return TRUE;
            }
            return FALSE;
        case WM_DESTROY:
        {
            struct cred_dialog_params *params =
                (struct cred_dialog_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
            if (params->hwndBalloonTip) DestroyWindow(params->hwndBalloonTip);
            return TRUE;
        }
        default:
            return FALSE;
    }
}

static BOOL find_existing_credential(const WCHAR *target, WCHAR *username, ULONG len_username,
                                     WCHAR *password, ULONG len_password)
{
    DWORD count, i;
    CREDENTIALW **credentials;

    if (!CredEnumerateW(target, 0, &count, &credentials)) return FALSE;
    for (i = 0; i < count; i++)
    {
        if (credentials[i]->Type != CRED_TYPE_DOMAIN_PASSWORD &&
            credentials[i]->Type != CRED_TYPE_GENERIC)
        {
            FIXME("no support for type %lu credentials\n", credentials[i]->Type);
            continue;
        }
        if ((!*username || !lstrcmpW(username, credentials[i]->UserName)) &&
            lstrlenW(credentials[i]->UserName) < len_username &&
            credentials[i]->CredentialBlobSize / sizeof(WCHAR) < len_password)
        {
            TRACE("found existing credential for %s\n", debugstr_w(credentials[i]->UserName));

            lstrcpyW(username, credentials[i]->UserName);
            memcpy(password, credentials[i]->CredentialBlob, credentials[i]->CredentialBlobSize);
            password[credentials[i]->CredentialBlobSize / sizeof(WCHAR)] = 0;

            CredFree(credentials);
            return TRUE;
        }
    }
    CredFree(credentials);
    return FALSE;
}

/******************************************************************************
 *           CredUIPromptForCredentialsW [CREDUI.@]
 */
DWORD WINAPI CredUIPromptForCredentialsW(PCREDUI_INFOW pUIInfo,
                                         PCWSTR pszTargetName,
                                         PCtxtHandle Reserved,
                                         DWORD dwAuthError,
                                         PWSTR pszUsername,
                                         ULONG ulUsernameMaxChars,
                                         PWSTR pszPassword,
                                         ULONG ulPasswordMaxChars, PBOOL pfSave,
                                         DWORD dwFlags)
{
    INT_PTR ret;
    struct cred_dialog_params params;
    DWORD result = ERROR_SUCCESS;

    TRACE("(%p, %s, %p, %ld, %s, %ld, %p, %ld, %p, 0x%08lx)\n", pUIInfo,
          debugstr_w(pszTargetName), Reserved, dwAuthError, debugstr_w(pszUsername),
          ulUsernameMaxChars, pszPassword, ulPasswordMaxChars, pfSave, dwFlags);

    if ((dwFlags & (CREDUI_FLAGS_ALWAYS_SHOW_UI|CREDUI_FLAGS_GENERIC_CREDENTIALS)) == CREDUI_FLAGS_ALWAYS_SHOW_UI)
        return ERROR_INVALID_FLAGS;

    if (!pszTargetName)
        return ERROR_INVALID_PARAMETER;

    if ((dwFlags & CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX) && !pfSave)
        return ERROR_INVALID_PARAMETER;

    if (!(dwFlags & CREDUI_FLAGS_ALWAYS_SHOW_UI) &&
        !(dwFlags & CREDUI_FLAGS_INCORRECT_PASSWORD) &&
        find_existing_credential(pszTargetName, pszUsername, ulUsernameMaxChars, pszPassword, ulPasswordMaxChars))
        return ERROR_SUCCESS;

    params.pszTargetName = pszTargetName;
    if (pUIInfo)
    {
        params.pszMessageText = pUIInfo->pszMessageText;
        params.pszCaptionText = pUIInfo->pszCaptionText;
        params.hbmBanner  = pUIInfo->hbmBanner;
    }
    else
    {
        params.pszMessageText = NULL;
        params.pszCaptionText = NULL;
        params.hbmBanner = NULL;
    }
    params.pszUsername = pszUsername;
    params.ulUsernameMaxChars = ulUsernameMaxChars;
    params.pszPassword = pszPassword;
    params.ulPasswordMaxChars = ulPasswordMaxChars;
    params.fSave = pfSave ? *pfSave : FALSE;
    params.dwFlags = dwFlags;
    params.hwndBalloonTip = NULL;
    params.fBalloonTipActive = FALSE;

    ret = DialogBoxParamW(hinstCredUI, MAKEINTRESOURCEW(IDD_CREDDIALOG),
                          pUIInfo ? pUIInfo->hwndParent : NULL,
                          CredDialogProc, (LPARAM)&params);
    if (ret <= 0)
        return GetLastError();

    if (ret == IDCANCEL)
    {
        TRACE("dialog cancelled\n");
        return ERROR_CANCELLED;
    }

    if (pfSave)
        *pfSave = params.fSave;

    if (params.fSave)
    {
        if (dwFlags & CREDUI_FLAGS_EXPECT_CONFIRMATION)
        {
            BOOL found = FALSE;
            struct pending_credentials *entry;
            int len;

            EnterCriticalSection(&csPendingCredentials);

            /* find existing pending credentials for the same target and overwrite */
            /* FIXME: is this correct? */
            LIST_FOR_EACH_ENTRY(entry, &pending_credentials_list, struct pending_credentials, entry)
                if (!lstrcmpW(pszTargetName, entry->pszTargetName))
                {
                    found = TRUE;
                    HeapFree(GetProcessHeap(), 0, entry->pszUsername);
                    SecureZeroMemory(entry->pszPassword, lstrlenW(entry->pszPassword) * sizeof(WCHAR));
                    HeapFree(GetProcessHeap(), 0, entry->pszPassword);
                }

            if (!found)
            {
                entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
                len = lstrlenW(pszTargetName);
                entry->pszTargetName = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
                memcpy(entry->pszTargetName, pszTargetName, (len + 1)*sizeof(WCHAR));
                list_add_tail(&pending_credentials_list, &entry->entry);
            }

            len = lstrlenW(params.pszUsername);
            entry->pszUsername = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
            memcpy(entry->pszUsername, params.pszUsername, (len + 1)*sizeof(WCHAR));
            len = lstrlenW(params.pszPassword);
            entry->pszPassword = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
            memcpy(entry->pszPassword, params.pszPassword, (len + 1)*sizeof(WCHAR));
            entry->generic = (dwFlags & CREDUI_FLAGS_GENERIC_CREDENTIALS) != 0;

            LeaveCriticalSection(&csPendingCredentials);
        }
        else if (!(dwFlags & CREDUI_FLAGS_DO_NOT_PERSIST))
            result = save_credentials(pszTargetName, pszUsername, pszPassword,
                                      (dwFlags & CREDUI_FLAGS_GENERIC_CREDENTIALS) != 0);
    }

    return result;
}

/******************************************************************************
 *           CredUIConfirmCredentialsW [CREDUI.@]
 */
DWORD WINAPI CredUIConfirmCredentialsW(PCWSTR pszTargetName, BOOL bConfirm)
{
    struct pending_credentials *entry;
    DWORD result = ERROR_NOT_FOUND;

    TRACE("(%s, %s)\n", debugstr_w(pszTargetName), bConfirm ? "TRUE" : "FALSE");

    if (!pszTargetName)
        return ERROR_INVALID_PARAMETER;

    EnterCriticalSection(&csPendingCredentials);

    LIST_FOR_EACH_ENTRY(entry, &pending_credentials_list, struct pending_credentials, entry)
    {
        if (!lstrcmpW(pszTargetName, entry->pszTargetName))
        {
            if (bConfirm)
                result = save_credentials(entry->pszTargetName, entry->pszUsername,
                                          entry->pszPassword, entry->generic);
            else
                result = ERROR_SUCCESS;

            list_remove(&entry->entry);

            HeapFree(GetProcessHeap(), 0, entry->pszTargetName);
            HeapFree(GetProcessHeap(), 0, entry->pszUsername);
            SecureZeroMemory(entry->pszPassword, lstrlenW(entry->pszPassword) * sizeof(WCHAR));
            HeapFree(GetProcessHeap(), 0, entry->pszPassword);
            HeapFree(GetProcessHeap(), 0, entry);

            break;
        }
    }

    LeaveCriticalSection(&csPendingCredentials);

    return result;
}

/******************************************************************************
 *           CredUIParseUserNameW [CREDUI.@]
 */
DWORD WINAPI CredUIParseUserNameW(PCWSTR pszUserName, PWSTR pszUser,
                                  ULONG ulMaxUserChars, PWSTR pszDomain,
                                  ULONG ulMaxDomainChars)
{
    PWSTR p;

    TRACE("(%s, %p, %ld, %p, %ld)\n", debugstr_w(pszUserName), pszUser,
          ulMaxUserChars, pszDomain, ulMaxDomainChars);

    if (!pszUserName || !pszUser || !ulMaxUserChars || !pszDomain ||
        !ulMaxDomainChars)
        return ERROR_INVALID_PARAMETER;

    /* FIXME: handle marshaled credentials */

    p = wcschr(pszUserName, '\\');
    if (p)
    {
        if (p - pszUserName > ulMaxDomainChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        if (lstrlenW(p + 1) > ulMaxUserChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        lstrcpyW(pszUser, p + 1);
        memcpy(pszDomain, pszUserName, (p - pszUserName)*sizeof(WCHAR));
        pszDomain[p - pszUserName] = '\0';

        return ERROR_SUCCESS;
    }

    p = wcsrchr(pszUserName, '@');
    if (p)
    {
        if (p + 1 - pszUserName > ulMaxUserChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        if (lstrlenW(p + 1) > ulMaxDomainChars - 1)
            return ERROR_INSUFFICIENT_BUFFER;
        lstrcpyW(pszDomain, p + 1);
        memcpy(pszUser, pszUserName, (p - pszUserName)*sizeof(WCHAR));
        pszUser[p - pszUserName] = '\0';

        return ERROR_SUCCESS;
    }

    if (lstrlenW(pszUserName) > ulMaxUserChars - 1)
        return ERROR_INSUFFICIENT_BUFFER;
    lstrcpyW(pszUser, pszUserName);
    pszDomain[0] = '\0';

    return ERROR_SUCCESS;
}

/******************************************************************************
 *           CredUIStoreSSOCredA [CREDUI.@]
 */
DWORD WINAPI CredUIStoreSSOCredA(PCSTR pszRealm, PCSTR pszUsername,
                                 PCSTR pszPassword, BOOL bPersist)
{
    FIXME("(%s, %s, %p, %d)\n", debugstr_a(pszRealm), debugstr_a(pszUsername),
          pszPassword, bPersist);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *           CredUIStoreSSOCredW [CREDUI.@]
 */
DWORD WINAPI CredUIStoreSSOCredW(PCWSTR pszRealm, PCWSTR pszUsername,
                                 PCWSTR pszPassword, BOOL bPersist)
{
    FIXME("(%s, %s, %p, %d)\n", debugstr_w(pszRealm), debugstr_w(pszUsername),
          pszPassword, bPersist);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *           CredUIReadSSOCredA [CREDUI.@]
 */
DWORD WINAPI CredUIReadSSOCredA(PCSTR pszRealm, PSTR *ppszUsername)
{
    FIXME("(%s, %p)\n", debugstr_a(pszRealm), ppszUsername);
    if (ppszUsername)
        *ppszUsername = NULL;
    return ERROR_NOT_FOUND;
}

/******************************************************************************
 *           CredUIReadSSOCredW [CREDUI.@]
 */
DWORD WINAPI CredUIReadSSOCredW(PCWSTR pszRealm, PWSTR *ppszUsername)
{
    FIXME("(%s, %p)\n", debugstr_w(pszRealm), ppszUsername);
    if (ppszUsername)
        *ppszUsername = NULL;
    return ERROR_NOT_FOUND;
}

/******************************************************************************
 * CredUIInitControls [CREDUI.@]
 */
BOOL WINAPI CredUIInitControls(void)
{
    FIXME("() stub\n");
    return TRUE;
}

/******************************************************************************
 * SspiPromptForCredentialsW [CREDUI.@]
 */
ULONG SEC_ENTRY SspiPromptForCredentialsW( PCWSTR target, void *info,
                                           DWORD error, PCWSTR package,
                                           PSEC_WINNT_AUTH_IDENTITY_OPAQUE input_id,
                                           PSEC_WINNT_AUTH_IDENTITY_OPAQUE *output_id,
                                           BOOL *save, DWORD sspi_flags )
{
    WCHAR username[CREDUI_MAX_USERNAME_LENGTH + 1] = {0};
    WCHAR password[CREDUI_MAX_PASSWORD_LENGTH + 1] = {0};
    DWORD len_username = ARRAY_SIZE(username);
    DWORD len_password = ARRAY_SIZE(password);
    DWORD ret, flags;
    CREDUI_INFOW *cred_info = info;
    SEC_WINNT_AUTH_IDENTITY_W *id = input_id;

    FIXME( "(%s, %p, %lu, %s, %p, %p, %p, %lx) stub\n", debugstr_w(target), info,
           error, debugstr_w(package), input_id, output_id, save, sspi_flags );

    if (!target) return ERROR_INVALID_PARAMETER;
    if (!package || (wcsicmp( package, L"Basic" ) && wcsicmp( package, L"NTLM" ) &&
                     wcsicmp( package, L"Negotiate" )))
    {
        FIXME( "package %s not supported\n", debugstr_w(package) );
        return ERROR_NO_SUCH_PACKAGE;
    }

    flags = CREDUI_FLAGS_ALWAYS_SHOW_UI | CREDUI_FLAGS_GENERIC_CREDENTIALS;

    if (sspi_flags & SSPIPFC_CREDPROV_DO_NOT_SAVE)
        flags |= CREDUI_FLAGS_DO_NOT_PERSIST;

    if (!(sspi_flags & SSPIPFC_NO_CHECKBOX))
        flags |= CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX;

    if (!id) find_existing_credential( target, username, len_username, password, len_password );
    else
    {
        if (id->User && id->UserLength > 0 && id->UserLength <= CREDUI_MAX_USERNAME_LENGTH)
        {
            memcpy( username, id->User, id->UserLength * sizeof(WCHAR) );
            username[id->UserLength] = 0;
        }
        if (id->Password && id->PasswordLength > 0 && id->PasswordLength <= CREDUI_MAX_PASSWORD_LENGTH)
        {
            memcpy( password, id->Password, id->PasswordLength * sizeof(WCHAR) );
            password[id->PasswordLength] = 0;
        }
    }

    if (!(ret = CredUIPromptForCredentialsW( cred_info, target, NULL, error, username,
                                             len_username, password, len_password, save, flags )))
    {
        DWORD size = sizeof(*id), len_domain = 0;
        WCHAR *ptr, *user = username, *domain = NULL;

        if ((ptr = wcschr( username, '\\' )))
        {
            user = ptr + 1;
            len_username = lstrlenW( user );
            if (!wcsicmp( package, L"NTLM" ) || !wcsicmp( package, L"Negotiate" ))
            {
                domain = username;
                len_domain = ptr - username;
            }
            *ptr = 0;
        }
        else len_username = lstrlenW( username );
        len_password = lstrlenW( password );

        size += (len_username + 1) * sizeof(WCHAR);
        size += (len_domain + 1) * sizeof(WCHAR);
        size += (len_password + 1) * sizeof(WCHAR);
        if (!(id = HeapAlloc( GetProcessHeap(), 0, size ))) return ERROR_OUTOFMEMORY;
        ptr = (WCHAR *)(id + 1);

        memcpy( ptr, user, (len_username + 1) * sizeof(WCHAR) );
        id->User           = ptr;
        id->UserLength     = len_username;
        ptr += len_username + 1;
        if (len_domain)
        {
            memcpy( ptr, domain, (len_domain + 1) * sizeof(WCHAR) );
            id->Domain         = ptr;
            id->DomainLength   = len_domain;
            ptr += len_domain + 1;
        }
        else
        {
            id->Domain         = NULL;
            id->DomainLength   = 0;
        }
        memcpy( ptr, password, (len_password + 1) * sizeof(WCHAR) );
        id->Password       = ptr;
        id->PasswordLength = len_password;
        id->Flags          = 0;

        *output_id = id;
    }

    return ret;
}

/******************************************************************************
 * CredUIPromptForWindowsCredentialsW [CREDUI.@]
 */
DWORD WINAPI CredUIPromptForWindowsCredentialsW( CREDUI_INFOW *info, DWORD error, ULONG *package,
                                                 const void *in_buf, ULONG in_buf_size, void **out_buf,
                                                 ULONG *out_buf_size, BOOL *save, DWORD flags )
{
    FIXME( "(%p, %lu, %p, %p, %lu, %p, %p, %p, %08lx) stub\n", info, error, package, in_buf, in_buf_size,
           out_buf, out_buf_size, save, flags );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * CredPackAuthenticationBufferW [CREDUI.@]
 */
BOOL  WINAPI CredPackAuthenticationBufferW( DWORD flags, WCHAR *username, WCHAR *password, BYTE *buf,
                                            DWORD *size )
{
    FIXME( "(%08lx, %s, %p, %p, %p) stub\n", flags, debugstr_w(username), password, buf, size );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * CredUnPackAuthenticationBufferW [CREDUI.@]
 */
BOOL  WINAPI CredUnPackAuthenticationBufferW( DWORD flags, void *buf, DWORD size, WCHAR *username,
                                              DWORD *len_username, WCHAR *domain, DWORD *len_domain,
                                              WCHAR *password, DWORD *len_password )
{
    FIXME( "(%08lx, %p, %lu, %p, %p, %p, %p, %p, %p) stub\n", flags, buf, size, username, len_username,
           domain, len_domain, password, len_password );
    return ERROR_CALL_NOT_IMPLEMENTED;
}
