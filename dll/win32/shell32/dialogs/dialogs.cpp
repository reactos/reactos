/*
 *    common shell dialogs
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 * Copyright 2021 Arnav Bhatt <arnavbhatt288@gmail.com>
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

#include "precomp.h"

typedef struct
{
    HWND hwndOwner;
    HICON hIcon;
    LPCWSTR lpstrDirectory;
    LPCWSTR lpstrTitle;
    LPCWSTR lpstrDescription;
    UINT uFlags;
    BOOL bCoInited;
} RUNFILEDLGPARAMS;

typedef struct
{
    BOOL bFriendlyUI;
    BOOL bIsButtonHot[2];
    HBITMAP hImageStrip;
    HBRUSH hBrush;
    HFONT hfFont;
    WNDPROC OldButtonProc;
} LOGOFF_DLG_CONTEXT, *PLOGOFF_DLG_CONTEXT;

typedef BOOL (WINAPI * LPFNOFN) (OPENFILENAMEW *);

WINE_DEFAULT_DEBUG_CHANNEL(shell);
static INT_PTR CALLBACK RunDlgProc(HWND, UINT, WPARAM, LPARAM);
static void FillList(HWND, LPWSTR, UINT, BOOL);


/*************************************************************************
 * PickIconDlg                    [SHELL32.62]
 *
 */

typedef struct
{
    HMODULE hLibrary;
    HWND hDlgCtrl;
    WCHAR szPath[MAX_PATH];
    INT Index;
    INT nIcons;
    HICON *phIcons;
} PICK_ICON_CONTEXT, *PPICK_ICON_CONTEXT;

BOOL CALLBACK EnumPickIconResourceProc(HMODULE hModule,
    LPCWSTR lpszType,
    LPWSTR lpszName,
    LONG_PTR lParam)
{
    PPICK_ICON_CONTEXT pIconContext = PPICK_ICON_CONTEXT(lParam);
    HWND hDlgCtrl = pIconContext->hDlgCtrl;

    if (IS_INTRESOURCE(lpszName))
        lParam = LOWORD(lpszName);
    else
        lParam = -1;

    SendMessageW(hDlgCtrl, LB_ADDSTRING, 0, lParam);

    return TRUE;
}

static void
DestroyIconList(HWND hDlgCtrl, PPICK_ICON_CONTEXT pIconContext)
{
    int count;
    int index;

    count = SendMessageW(hDlgCtrl, LB_GETCOUNT, 0, 0);
    if (count == LB_ERR)
        return;

    for(index = 0; index < count; index++)
    {
        DestroyIcon(pIconContext->phIcons[index]);
        pIconContext->phIcons[index] = NULL;
    }
}

static BOOL
DoLoadIcons(HWND hwndDlg, PPICK_ICON_CONTEXT pIconContext, LPCWSTR pszFile)
{
    WCHAR szExpandedPath[MAX_PATH];

    // Destroy previous icons
    DestroyIconList(pIconContext->hDlgCtrl, pIconContext);
    SendMessageW(pIconContext->hDlgCtrl, LB_RESETCONTENT, 0, 0);
    delete[] pIconContext->phIcons;

    // Store the path
    StringCchCopyW(pIconContext->szPath, _countof(pIconContext->szPath), pszFile);
    ExpandEnvironmentStringsW(pszFile, szExpandedPath, _countof(szExpandedPath));

    // Load the module if possible
    HMODULE hLibrary = LoadLibraryExW(szExpandedPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (pIconContext->hLibrary)
        FreeLibrary(pIconContext->hLibrary);
    pIconContext->hLibrary = hLibrary;

    if (pIconContext->hLibrary)
    {
        // Load the icons from the module
        pIconContext->nIcons = ExtractIconExW(szExpandedPath, -1, NULL, NULL, 0);
        pIconContext->phIcons = new HICON[pIconContext->nIcons];

        if (ExtractIconExW(szExpandedPath, 0, pIconContext->phIcons, NULL, pIconContext->nIcons))
        {
            EnumResourceNamesW(pIconContext->hLibrary, RT_GROUP_ICON, EnumPickIconResourceProc, (LPARAM)pIconContext);
        }
        else
        {
            pIconContext->nIcons = 0;
        }
    }
    else
    {
        // .ico file
        pIconContext->nIcons = 1;
        pIconContext->phIcons = new HICON[1];

        if (ExtractIconExW(szExpandedPath, 0, pIconContext->phIcons, NULL, pIconContext->nIcons))
        {
            SendMessageW(pIconContext->hDlgCtrl, LB_ADDSTRING, 0, 0);
        }
        else
        {
            pIconContext->nIcons = 0;
        }
    }

    // Set the text and reset the edit control's modification flag
    SetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, pIconContext->szPath);
    SendDlgItemMessage(hwndDlg, IDC_EDIT_PATH, EM_SETMODIFY, FALSE, 0);

    if (pIconContext->nIcons == 0)
    {
        delete[] pIconContext->phIcons;
        pIconContext->phIcons = NULL;
    }

    return (pIconContext->nIcons > 0);
}

static void NoIconsInFile(HWND hwndDlg, PPICK_ICON_CONTEXT pIconContext)
{
    // Show an error message
    CStringW strText, strTitle(MAKEINTRESOURCEW(IDS_PICK_ICON_TITLE));
    strText.Format(IDS_NO_ICONS, pIconContext->szPath);
    MessageBoxW(hwndDlg, strText, strTitle, MB_ICONWARNING);

    // Load the default icons
    DoLoadIcons(hwndDlg, pIconContext, g_pszShell32);
}

// Icon size
#define CX_ICON     GetSystemMetrics(SM_CXICON)
#define CY_ICON     GetSystemMetrics(SM_CYICON)

// Item size
#define CX_ITEM     (CX_ICON + 4)
#define CY_ITEM     (CY_ICON + 12)

INT_PTR CALLBACK PickIconProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LPMEASUREITEMSTRUCT lpmis;
    LPDRAWITEMSTRUCT lpdis;
    HICON hIcon;
    INT index, count;
    WCHAR szText[MAX_PATH], szFilter[100];
    CStringW strTitle;
    OPENFILENAMEW ofn;

    PPICK_ICON_CONTEXT pIconContext = (PPICK_ICON_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            pIconContext = (PPICK_ICON_CONTEXT)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pIconContext);
            pIconContext->hDlgCtrl = GetDlgItem(hwndDlg, IDC_PICKICON_LIST);

            SendMessageW(pIconContext->hDlgCtrl, LB_SETCOLUMNWIDTH, CX_ITEM, 0);

            // Load the icons
            if (!DoLoadIcons(hwndDlg, pIconContext, pIconContext->szPath))
                NoIconsInFile(hwndDlg, pIconContext);

            // Set the selection
            count = SendMessageW(pIconContext->hDlgCtrl, LB_GETCOUNT, 0, 0);
            if (count != LB_ERR)
            {
                if (pIconContext->Index < 0)
                {
                    // A negative value will be interpreted as a negated resource ID.
                    LPARAM lParam = -pIconContext->Index;
                    pIconContext->Index = (INT)SendMessageW(pIconContext->hDlgCtrl, LB_FINDSTRINGEXACT, -1, lParam);
                }

                if (pIconContext->Index < 0 || count <= pIconContext->Index)
                    pIconContext->Index = 0;

                SendMessageW(pIconContext->hDlgCtrl, LB_SETCURSEL, pIconContext->Index, 0);
                SendMessageW(pIconContext->hDlgCtrl, LB_SETTOPINDEX, pIconContext->Index, 0);
            }

            SHAutoComplete(GetDlgItem(hwndDlg, IDC_EDIT_PATH), SHACF_DEFAULT);
            return TRUE;
        }

        case WM_DESTROY:
        {
            DestroyIconList(pIconContext->hDlgCtrl, pIconContext);
            delete[] pIconContext->phIcons;

            if (pIconContext->hLibrary)
                FreeLibrary(pIconContext->hLibrary);
            break;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case IDOK:
            {
                /* Check whether the path edit control has been modified; if so load the icons instead of validating */
                if (SendDlgItemMessage(hwndDlg, IDC_EDIT_PATH, EM_GETMODIFY, 0, 0))
                {
                    /* Reset the edit control's modification flag and retrieve the text */
                    SendDlgItemMessage(hwndDlg, IDC_EDIT_PATH, EM_SETMODIFY, FALSE, 0);
                    GetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, szText, _countof(szText));

                    // Load the icons
                    if (!DoLoadIcons(hwndDlg, pIconContext, szText))
                        NoIconsInFile(hwndDlg, pIconContext);

                    // Set the selection
                    SendMessageW(pIconContext->hDlgCtrl, LB_SETCURSEL, 0, 0);
                    break;
                }

                /* The path edit control has not been modified, return the selection */
                pIconContext->Index = (INT)SendMessageW(pIconContext->hDlgCtrl, LB_GETCURSEL, 0, 0);
                GetDlgItemTextW(hwndDlg, IDC_EDIT_PATH, pIconContext->szPath, _countof(pIconContext->szPath));
                EndDialog(hwndDlg, 1);
                break;
            }

            case IDCANCEL:
                EndDialog(hwndDlg, 0);
                break;

            case IDC_PICKICON_LIST:
                switch (HIWORD(wParam))
                {
                    case LBN_SELCHANGE:
                        InvalidateRect((HWND)lParam, NULL, TRUE);
                        break;

                    case LBN_DBLCLK:
                        SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
                        break;
                }
                break;

            case IDC_BUTTON_PATH:
            {
                // Choose the module path
                szText[0] = 0;
                szFilter[0] = 0;
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwndDlg;
                ofn.lpstrFile = szText;
                ofn.nMaxFile = _countof(szText);
                strTitle.LoadString(IDS_PICK_ICON_TITLE);
                ofn.lpstrTitle = strTitle;
                LoadStringW(shell32_hInstance, IDS_PICK_ICON_FILTER, szFilter, _countof(szFilter));
                ofn.lpstrFilter = szFilter;
                if (!GetOpenFileNameW(&ofn))
                    break;

                // Load the icons
                if (!DoLoadIcons(hwndDlg, pIconContext, szText))
                    NoIconsInFile(hwndDlg, pIconContext);

                // Set the selection
                SendMessageW(pIconContext->hDlgCtrl, LB_SETCURSEL, 0, 0);
                break;
            }

            default:
                break;
            }
            break;

        case WM_MEASUREITEM:
            lpmis = (LPMEASUREITEMSTRUCT)lParam;
            lpmis->itemHeight = CY_ITEM;
            return TRUE;

        case WM_DRAWITEM:
        {
            lpdis = (LPDRAWITEMSTRUCT)lParam;
            if (lpdis->itemID == (UINT)-1)
                break;
            switch (lpdis->itemAction)
            {
                case ODA_SELECT:
                case ODA_DRAWENTIRE:
                {
                    index = SendMessageW(pIconContext->hDlgCtrl, LB_GETCURSEL, 0, 0);
                    hIcon = pIconContext->phIcons[lpdis->itemID];

                    if (lpdis->itemID == (UINT)index)
                        FillRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
                    else
                        FillRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)(COLOR_WINDOW + 1));

                    // Centering
                    INT x = lpdis->rcItem.left + (CX_ITEM - CX_ICON) / 2;
                    INT y = lpdis->rcItem.top + (CY_ITEM - CY_ICON) / 2;

                    DrawIconEx(lpdis->hDC, x, y, hIcon, 0, 0, 0, NULL, DI_NORMAL);
                    break;
                }
            }
            return TRUE;
        }
    }

    return FALSE;
}

BOOL WINAPI PickIconDlg(
    HWND hWndOwner,
    LPWSTR lpstrFile,
    UINT nMaxFile,
    INT* lpdwIconIndex)
{
    int res;
    WCHAR szExpandedPath[MAX_PATH];

    // Initialize the dialog
    PICK_ICON_CONTEXT IconContext = { NULL };
    IconContext.Index = *lpdwIconIndex;
    StringCchCopyW(IconContext.szPath, _countof(IconContext.szPath), lpstrFile);
    ExpandEnvironmentStringsW(lpstrFile, szExpandedPath, _countof(szExpandedPath));

    if (!szExpandedPath[0] ||
        GetFileAttributesW(szExpandedPath) == INVALID_FILE_ATTRIBUTES)
    {
        if (szExpandedPath[0])
        {
            // No such file
            CStringW strText, strTitle(MAKEINTRESOURCEW(IDS_PICK_ICON_TITLE));
            strText.Format(IDS_FILE_NOT_FOUND, lpstrFile);
            MessageBoxW(hWndOwner, strText, strTitle, MB_ICONWARNING);
        }

        // Set the default value
        StringCchCopyW(IconContext.szPath, _countof(IconContext.szPath), g_pszShell32);
    }

    // Show the dialog
    res = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_PICK_ICON), hWndOwner, PickIconProc, (LPARAM)&IconContext);
    if (res)
    {
        // Store the selected icon
        StringCchCopyW(lpstrFile, nMaxFile, IconContext.szPath);
        *lpdwIconIndex = IconContext.Index;
    }

    return res;
}

/*************************************************************************
 * RunFileDlg                    [internal]
 *
 * The Unicode function that is available as ordinal 61 on Windows NT/2000/XP/...
 */
void WINAPI RunFileDlg(
    HWND hWndOwner,
    HICON hIcon,
    LPCWSTR lpstrDirectory,
    LPCWSTR lpstrTitle,
    LPCWSTR lpstrDescription,
    UINT uFlags)
{
    TRACE("\n");

    RUNFILEDLGPARAMS rfdp;
    rfdp.hwndOwner        = hWndOwner;
    rfdp.hIcon            = hIcon;
    rfdp.lpstrDirectory   = lpstrDirectory;
    rfdp.lpstrTitle       = lpstrTitle;
    rfdp.lpstrDescription = lpstrDescription;
    rfdp.uFlags           = uFlags;

    DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_RUN), hWndOwner, RunDlgProc, (LPARAM)&rfdp);
}


/* find the directory that contains the file being run */
static LPWSTR RunDlg_GetParentDir(LPCWSTR cmdline)
{
    const WCHAR *src;
    WCHAR *dest, *result, *result_end=NULL;

    result = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*(strlenW(cmdline)+5));

    if (NULL == result)
    {
        TRACE("HeapAlloc couldn't allocate %d bytes\n", sizeof(WCHAR)*(strlenW(cmdline)+5));
        return NULL;
    }

    src = cmdline;
    dest = result;

    if (*src == '"')
    {
        src++;
        while (*src && *src != '"')
        {
            if (*src == '\\')
                result_end = dest;
            *dest++ = *src++;
        }
    }
    else {
        while (*src)
        {
            if (isspaceW(*src))
            {
                *dest = 0;
                if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(result))
                    break;
                strcatW(dest, L".exe");
                if (INVALID_FILE_ATTRIBUTES != GetFileAttributesW(result))
                    break;
            }
            else if (*src == '\\')
                result_end = dest;
            *dest++ = *src++;
        }
    }

    if (result_end)
    {
        *result_end = 0;
        return result;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, result);
        return NULL;
    }
}

static void EnableOkButtonFromEditContents(HWND hwnd)
{
    BOOL Enable = FALSE;
    INT Length, n;
    HWND Edit = GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH);
    Length = GetWindowTextLengthW(Edit);
    if (Length > 0)
    {
        PWCHAR psz = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, (Length + 1) * sizeof(WCHAR));
        if (psz)
        {
            GetWindowTextW(Edit, psz, Length + 1);
            for (n = 0; n < Length && !Enable; ++n)
                Enable = psz[n] != ' ';
            HeapFree(GetProcessHeap(), 0, psz);
        }
        else
            Enable = TRUE;
    }
    EnableWindow(GetDlgItem(hwnd, IDOK), Enable);
}

/* Dialog procedure for RunFileDlg */
static INT_PTR CALLBACK RunDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RUNFILEDLGPARAMS *prfdp = (RUNFILEDLGPARAMS *)GetWindowLongPtrW(hwnd, DWLP_USER);
    HWND hwndCombo, hwndEdit;
    COMBOBOXINFO ComboInfo;

    switch (message)
    {
        case WM_INITDIALOG:
            prfdp = (RUNFILEDLGPARAMS *)lParam;
            SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)prfdp);

            if (prfdp->lpstrTitle)
                SetWindowTextW(hwnd, prfdp->lpstrTitle);
            if (prfdp->lpstrDescription)
                SetWindowTextW(GetDlgItem(hwnd, IDC_RUNDLG_DESCRIPTION), prfdp->lpstrDescription);
            if (prfdp->uFlags & RFF_NOBROWSE)
            {
                HWND browse = GetDlgItem(hwnd, IDC_RUNDLG_BROWSE);
                ShowWindow(browse, SW_HIDE);
                EnableWindow(browse, FALSE);
            }
            if (prfdp->uFlags & RFF_NOLABEL)
                ShowWindow(GetDlgItem(hwnd, IDC_RUNDLG_LABEL), SW_HIDE);
            if (prfdp->uFlags & RFF_NOSEPARATEMEM)
            {
                FIXME("RFF_NOSEPARATEMEM not supported\n");
            }

            /* Use the default Shell Run icon if no one is specified */
            if (prfdp->hIcon == NULL)
                prfdp->hIcon = LoadIconW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_RUN));
            /*
             * NOTE: Starting Windows Vista, the "Run File" dialog gets a
             * title icon that remains the same as the default one, even if
             * the user specifies a custom icon.
             * Since we currently imitate Windows 2003, therefore do not show
             * any title icon.
             */
            // SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)prfdp->hIcon);
            // SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)prfdp->hIcon);
            SendMessageW(GetDlgItem(hwnd, IDC_RUNDLG_ICON), STM_SETICON, (WPARAM)prfdp->hIcon, 0);

            hwndCombo = GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH);
            FillList(hwndCombo, NULL, 0, (prfdp->uFlags & RFF_NODEFAULT) == 0);
            EnableOkButtonFromEditContents(hwnd);

            ComboInfo.cbSize = sizeof(ComboInfo);
            GetComboBoxInfo(hwndCombo, &ComboInfo);
            hwndEdit = ComboInfo.hwndItem;
            ASSERT(::IsWindow(hwndEdit));

            // SHAutoComplete needs co init
            prfdp->bCoInited = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

            SHAutoComplete(hwndEdit, SHACF_FILESYSTEM | SHACF_FILESYS_ONLY | SHACF_URLALL);

            SetFocus(hwndCombo);
            return TRUE;

        case WM_DESTROY:
            if (prfdp->bCoInited)
                CoUninitialize();
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    LRESULT lRet;
                    HWND htxt = GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH);
                    INT ic;
                    WCHAR *psz, *pszExpanded, *parent = NULL;
                    DWORD cchExpand;
                    SHELLEXECUTEINFOW sei;
                    NMRUNFILEDLGW nmrfd;

                    ic = GetWindowTextLengthW(htxt);
                    if (ic == 0)
                    {
                        EndDialog(hwnd, IDCANCEL);
                        return TRUE;
                    }

                    ZeroMemory(&sei, sizeof(sei));
                    sei.cbSize = sizeof(sei);

                    /*
                     * Allocate a new MRU entry, we need to add two characters
                     * for the terminating "\\1" part, then the NULL character.
                     */
                    psz = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (ic + 2 + 1)*sizeof(WCHAR));
                    if (!psz)
                    {
                        EndDialog(hwnd, IDCANCEL);
                        return TRUE;
                    }

                    GetWindowTextW(htxt, psz, ic + 1);
                    sei.hwnd = hwnd;
                    sei.nShow = SW_SHOWNORMAL;
                    sei.lpFile = psz;
                    StrTrimW(psz, L" \t");

                    if (wcschr(psz, L'%') != NULL)
                    {
                        cchExpand = ExpandEnvironmentStringsW(psz, NULL, 0);
                        pszExpanded = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, cchExpand * sizeof(WCHAR));
                        if (!pszExpanded)
                        {
                            HeapFree(GetProcessHeap(), 0, psz);
                            EndDialog(hwnd, IDCANCEL);
                            return TRUE;
                        }
                        ExpandEnvironmentStringsW(psz, pszExpanded, cchExpand);
                        StrTrimW(pszExpanded, L" \t");
                    }
                    else
                    {
                        pszExpanded = psz;
                    }

                    /*
                     * The precedence is the following: first the user-given
                     * current directory is used; if there is none, a current
                     * directory is computed if the RFF_CALCDIRECTORY is set,
                     * otherwise no current directory is defined.
                     */
                    LPCWSTR pszStartDir;
                    if (prfdp->lpstrDirectory)
                    {
                        sei.lpDirectory = prfdp->lpstrDirectory;
                        pszStartDir = prfdp->lpstrDirectory;
                    }
                    else if (prfdp->uFlags & RFF_CALCDIRECTORY)
                    {
                        sei.lpDirectory = parent = RunDlg_GetParentDir(sei.lpFile);
                        pszStartDir = parent = RunDlg_GetParentDir(pszExpanded);
                    }
                    else
                    {
                        sei.lpDirectory = NULL;
                        pszStartDir = NULL;
                    }

                    /* Hide the dialog for now on, we will show it up in case of retry */
                    ShowWindow(hwnd, SW_HIDE);

                    /*
                     * As shown by manual tests on Windows, modifying the contents
                     * of the notification structure will not modify what the
                     * Run-Dialog will use for the nShow parameter. However the
                     * lpFile and lpDirectory pointers are set to the buffers used
                     * by the Run-Dialog, as a consequence they can be modified by
                     * the notification receiver, as long as it respects the lengths
                     * of the buffers (to avoid buffer overflows).
                     */
                    nmrfd.hdr.code = RFN_VALIDATE;
                    nmrfd.hdr.hwndFrom = hwnd;
                    nmrfd.hdr.idFrom = 0;
                    nmrfd.lpFile = pszExpanded;
                    nmrfd.lpDirectory = pszStartDir;
                    nmrfd.nShow = SW_SHOWNORMAL;

                    lRet = SendMessageW(prfdp->hwndOwner, WM_NOTIFY, 0, (LPARAM)&nmrfd.hdr);

                    switch (lRet)
                    {
                        case RF_CANCEL:
                            EndDialog(hwnd, IDCANCEL);
                            break;

                        case RF_OK:
                            /* We use SECL_NO_UI because we don't want to see
                             * errors here, but we will try again below and
                             * there we will output our errors. */
                            if (SUCCEEDED(ShellExecCmdLine(hwnd, pszExpanded, pszStartDir, SW_SHOWNORMAL, NULL,
                                                           SECL_ALLOW_NONEXE | SECL_NO_UI)))
                            {
                                /* Call GetWindowText again in case the contents of the edit box have changed. */
                                GetWindowTextW(htxt, psz, ic + 1);
                                FillList(htxt, psz, ic + 2 + 1, FALSE);
                                EndDialog(hwnd, IDOK);
                                break;
                            }
                            else if (SUCCEEDED(ShellExecuteExW(&sei)))
                            {
                                /* Call GetWindowText again in case the contents of the edit box have changed. */
                                GetWindowTextW(htxt, psz, ic + 1);
                                FillList(htxt, psz, ic + 2 + 1, FALSE);
                                EndDialog(hwnd, IDOK);
                                break;
                            }

                        /* Fall-back */
                        case RF_RETRY:
                        default:
                            SendMessageW(htxt, CB_SETEDITSEL, 0, MAKELPARAM (0, -1));
                            /* Show back the dialog */
                            ShowWindow(hwnd, SW_SHOW);
                            break;
                    }

                    HeapFree(GetProcessHeap(), 0, parent);
                    HeapFree(GetProcessHeap(), 0, psz);
                    if (psz != pszExpanded)
                        HeapFree(GetProcessHeap(), 0, pszExpanded);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;

                case IDC_RUNDLG_BROWSE:
                {
                    HMODULE hComdlg = NULL;
                    LPFNOFN ofnProc = NULL;
                    WCHAR szFName[1024] = {0};
                    WCHAR filter[MAX_PATH], szCaption[MAX_PATH];
                    OPENFILENAMEW ofn;

                    LoadStringW(shell32_hInstance, IDS_RUNDLG_BROWSE_FILTER, filter, _countof(filter));
                    LoadStringW(shell32_hInstance, IDS_RUNDLG_BROWSE_CAPTION, szCaption, _countof(szCaption));

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = filter;
                    ofn.lpstrFile = szFName;
                    ofn.nMaxFile = _countof(szFName) - 1;
                    ofn.lpstrTitle = szCaption;
                    ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_EXPLORER;
                    ofn.lpstrInitialDir = prfdp->lpstrDirectory;

                    if (NULL == (hComdlg = LoadLibraryExW(L"comdlg32", NULL, 0)) ||
                        NULL == (ofnProc = (LPFNOFN)GetProcAddress(hComdlg, "GetOpenFileNameW")))
                    {
                        ERR("Couldn't get GetOpenFileName function entry (lib=%p, proc=%p)\n", hComdlg, ofnProc);
                        ShellMessageBoxW(shell32_hInstance, hwnd, MAKEINTRESOURCEW(IDS_RUNDLG_BROWSE_ERROR), NULL, MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    if (ofnProc(&ofn))
                    {
                        SetFocus(GetDlgItem(hwnd, IDOK));
                        SetWindowTextW(GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH), szFName);
                        SendMessageW(GetDlgItem(hwnd, IDC_RUNDLG_EDITPATH), CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                        EnableOkButtonFromEditContents(hwnd);
                        SetFocus(GetDlgItem(hwnd, IDOK));
                    }

                    FreeLibrary(hComdlg);

                    return TRUE;
                }
                case IDC_RUNDLG_EDITPATH:
                {
                    if (HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        EnableOkButtonFromEditContents(hwnd);
                    }
                    return TRUE;
                }
            }
            return TRUE;
    }
    return FALSE;
}

/*
 * This function grabs the MRU list from the registry and fills the combo-list
 * for the "Run" dialog above. fShowDefault is ignored if pszLatest != NULL.
 */
// FIXME: Part of this code should be part of some MRUList API,
// that is scattered amongst shell32, comctl32 (?!) and comdlg32.
static void FillList(HWND hCb, LPWSTR pszLatest, UINT cchStr, BOOL fShowDefault)
{
    HKEY hkey;
    WCHAR *pszList = NULL, *pszCmd = NULL, *pszTmp = NULL, cMatch = 0, cMax = 0x60;
    WCHAR szIndex[2] = L"-";
    UINT cchLatest;
    DWORD dwType, icList = 0, icCmd = 0;
    LRESULT lRet;
    UINT Nix;

    /*
     * Retrieve the string length of pszLatest and check whether its buffer size
     * (cchStr in number of characters) is large enough to add the terminating "\\1"
     * (and the NULL character).
     */
    if (pszLatest)
    {
        cchLatest = wcslen(pszLatest);
        if (cchStr < cchLatest + 2 + 1)
        {
            TRACE("pszLatest buffer is not large enough (%d) to hold the MRU terminator.\n", cchStr);
            return;
        }
    }
    else
    {
        cchStr = 0;
    }

    SendMessageW(hCb, CB_RESETCONTENT, 0, 0);

    lRet = RegCreateKeyExW(HKEY_CURRENT_USER,
                           L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU",
                           0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL);
    if (lRet != ERROR_SUCCESS)
    {
        TRACE("Unable to open or create the RunMRU key, error %d\n", GetLastError());
        return;
    }

    lRet = RegQueryValueExW(hkey, L"MRUList", NULL, &dwType, NULL, &icList);
    if (lRet == ERROR_SUCCESS && dwType == REG_SZ && icList > sizeof(WCHAR))
    {
        pszList = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, icList);
        if (!pszList)
        {
            TRACE("HeapAlloc failed to allocate %d bytes\n", icList);
            goto Continue;
        }
        pszList[0] = L'\0';

        lRet = RegQueryValueExW(hkey, L"MRUList", NULL, NULL, (LPBYTE)pszList, &icList);
        if (lRet != ERROR_SUCCESS)
        {
            TRACE("Unable to grab MRUList, error %d\n", GetLastError());
            pszList[0] = L'\0';
        }
    }
    else
    {
Continue:
        icList = sizeof(WCHAR);
        pszList = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, icList);
        if (!pszList)
        {
            TRACE("HeapAlloc failed to allocate %d bytes\n", icList);
            RegCloseKey(hkey);
            return;
        }
        pszList[0] = L'\0';
    }

    /* Convert the number of bytes from MRUList into number of characters (== number of indices) */
    icList /= sizeof(WCHAR);

    for (Nix = 0; Nix < icList - 1; Nix++)
    {
        if (pszList[Nix] > cMax)
            cMax = pszList[Nix];

        szIndex[0] = pszList[Nix];

        lRet = RegQueryValueExW(hkey, szIndex, NULL, &dwType, NULL, &icCmd);
        if (lRet != ERROR_SUCCESS || dwType != REG_SZ)
        {
            TRACE("Unable to grab size of index, error %d\n", GetLastError());
            continue;
        }

        if (pszCmd)
        {
            pszTmp = (WCHAR*)HeapReAlloc(GetProcessHeap(), 0, pszCmd, icCmd);
            if (!pszTmp)
            {
                TRACE("HeapReAlloc failed to reallocate %d bytes\n", icCmd);
                continue;
            }
            pszCmd = pszTmp;
        }
        else
        {
            pszCmd = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, icCmd);
            if (!pszCmd)
            {
                TRACE("HeapAlloc failed to allocate %d bytes\n", icCmd);
                continue;
            }
        }

        lRet = RegQueryValueExW(hkey, szIndex, NULL, NULL, (LPBYTE)pszCmd, &icCmd);
        if (lRet != ERROR_SUCCESS)
        {
            TRACE("Unable to grab index, error %d\n", GetLastError());
            continue;
        }

        /*
         * Generally the command string will end up with "\\1".
         * Find the last backslash in the string and NULL-terminate.
         * Windows does not seem to check for what comes next, so that
         * a command of the form:
         *     c:\\my_dir\\myfile.exe
         * will be cut just after "my_dir", whereas a command of the form:
         *     c:\\my_dir\\myfile.exe\\1
         * will be cut just after "myfile.exe".
         */
        pszTmp = wcsrchr(pszCmd, L'\\');
        if (pszTmp)
            *pszTmp = L'\0';

        /*
         * In the following we try to add pszLatest to the MRU list.
         * We suppose that our caller has already correctly allocated
         * the string with enough space for us to append a "\\1".
         *
         * FIXME: TODO! (At the moment we don't append it!)
         */

        if (pszLatest)
        {
            if (wcsicmp(pszCmd, pszLatest) == 0)
            {
                SendMessageW(hCb, CB_INSERTSTRING, 0, (LPARAM)pszCmd);
                SetWindowTextW(hCb, pszCmd);
                SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));

                cMatch = pszList[Nix];
                memmove(&pszList[1], pszList, Nix * sizeof(WCHAR));
                pszList[0] = cMatch;
                continue;
            }
        }

        if (icList - 1 != 26 || icList - 2 != Nix || cMatch || pszLatest == NULL)
        {
            SendMessageW(hCb, CB_ADDSTRING, 0, (LPARAM)pszCmd);
            if (!Nix && fShowDefault)
            {
                SetWindowTextW(hCb, pszCmd);
                SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
            }
        }
        else
        {
            SendMessageW(hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest);
            SetWindowTextW(hCb, pszLatest);
            SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));

            cMatch = pszList[Nix];
            memmove(&pszList[1], pszList, Nix * sizeof(WCHAR));
            pszList[0] = cMatch;
            szIndex[0] = cMatch;

            wcscpy(&pszLatest[cchLatest], L"\\1");
            RegSetValueExW(hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, (cchLatest + 2 + 1) * sizeof(WCHAR));
            pszLatest[cchLatest] = L'\0';
        }
    }

    if (!cMatch && pszLatest != NULL)
    {
        SendMessageW(hCb, CB_INSERTSTRING, 0, (LPARAM)pszLatest);
        SetWindowTextW(hCb, pszLatest);
        SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM (0, -1));

        cMatch = ++cMax;

        if (pszList)
        {
            pszTmp = (WCHAR*)HeapReAlloc(GetProcessHeap(), 0, pszList, (++icList) * sizeof(WCHAR));
            if (!pszTmp)
            {
                TRACE("HeapReAlloc failed to reallocate enough bytes\n");
                goto Cleanup;
            }
            pszList = pszTmp;
        }
        else
        {
            pszList = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, (++icList) * sizeof(WCHAR));
            if (!pszList)
            {
                TRACE("HeapAlloc failed to allocate enough bytes\n");
                goto Cleanup;
            }
        }

        memmove(&pszList[1], pszList, (icList - 1) * sizeof(WCHAR));
        pszList[0] = cMatch;
        szIndex[0] = cMatch;

        wcscpy(&pszLatest[cchLatest], L"\\1");
        RegSetValueExW(hkey, szIndex, 0, REG_SZ, (LPBYTE)pszLatest, (cchLatest + 2 + 1) * sizeof(WCHAR));
        pszLatest[cchLatest] = L'\0';
    }

Cleanup:
    RegSetValueExW(hkey, L"MRUList", 0, REG_SZ, (LPBYTE)pszList, (wcslen(pszList) + 1) * sizeof(WCHAR));

    HeapFree(GetProcessHeap(), 0, pszCmd);
    HeapFree(GetProcessHeap(), 0, pszList);

    RegCloseKey(hkey);
}


/*************************************************************************
 * ConfirmDialog                [internal]
 *
 * Put up a confirm box, return TRUE if the user confirmed
 */
static BOOL ConfirmDialog(HWND hWndOwner, UINT PromptId, UINT TitleId)
{
    WCHAR Prompt[256];
    WCHAR Title[256];

    LoadStringW(shell32_hInstance, PromptId, Prompt, _countof(Prompt));
    LoadStringW(shell32_hInstance, TitleId, Title, _countof(Title));
    return MessageBoxW(hWndOwner, Prompt, Title, MB_YESNO | MB_ICONQUESTION) == IDYES;
}

typedef HRESULT (WINAPI *tShellDimScreen)(IUnknown** Unknown, HWND* hWindow);

BOOL
CallShellDimScreen(IUnknown** pUnknown, HWND* hWindow)
{
    static tShellDimScreen ShellDimScreen;
    static BOOL Initialized = FALSE;
    if (!Initialized)
    {
        HMODULE mod = LoadLibraryW(L"msgina.dll");
        ShellDimScreen = (tShellDimScreen)GetProcAddress(mod, (LPCSTR)16);
        Initialized = TRUE;
    }

    HRESULT hr = E_FAIL;
    if (ShellDimScreen)
        hr = ShellDimScreen(pUnknown, hWindow);
    return SUCCEEDED(hr);
}


/* Used to get the shutdown privilege */
static BOOL
EnablePrivilege(LPCWSTR lpszPrivilegeName, BOOL bEnablePrivilege)
{
    BOOL   Success;
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES,
                               &hToken);
    if (!Success) return Success;

    Success = LookupPrivilegeValueW(NULL,
                                    lpszPrivilegeName,
                                    &tp.Privileges[0].Luid);
    if (!Success) goto Quit;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = (bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0);

    Success = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

Quit:
    CloseHandle(hToken);
    return Success;
}

/*************************************************************************
 * RestartDialogEx                [SHELL32.730]
 */

int WINAPI RestartDialogEx(HWND hWndOwner, LPCWSTR lpwstrReason, DWORD uFlags, DWORD uReason)
{
    TRACE("(%p)\n", hWndOwner);

    CComPtr<IUnknown> fadeHandler;
    HWND parent;

    if (!CallShellDimScreen(&fadeHandler, &parent))
        parent = hWndOwner;

    /* FIXME: use lpwstrReason */
    if (ConfirmDialog(parent, IDS_RESTART_PROMPT, IDS_RESTART_TITLE))
    {
        EnablePrivilege(L"SeShutdownPrivilege", TRUE);
        ExitWindowsEx(EWX_REBOOT, uReason);
        EnablePrivilege(L"SeShutdownPrivilege", FALSE);
    }

    return 0;
}

/* Functions and macros used for fancy log off dialog box */
#define IS_PRODUCT_VERSION_WORKSTATION          0x300
#define FRIENDLY_LOGOFF_IS_NOT_ENFORCED         0x0

#define FONT_POINT_SIZE                 13

#define DARK_GREY_COLOR                 RGB(244, 244, 244)
#define LIGHT_GREY_COLOR                RGB(38, 38, 38)

/* Bitmap's size for buttons */
#define CX_BITMAP                       33
#define CY_BITMAP                       33

#define NUMBER_OF_BUTTONS               2

/* After determining the button as well as its state paint the image strip bitmap using these predefined positions */
#define BUTTON_SWITCH_USER              0
#define BUTTON_SWITCH_USER_PRESSED      (CY_BITMAP + BUTTON_SWITCH_USER)
#define BUTTON_SWITCH_USER_FOCUSED      (CY_BITMAP + BUTTON_SWITCH_USER_PRESSED)
#define BUTTON_LOG_OFF                  (CY_BITMAP + BUTTON_SWITCH_USER_FOCUSED)
#define BUTTON_LOG_OFF_PRESSED          (CY_BITMAP + BUTTON_LOG_OFF)
#define BUTTON_LOG_OFF_FOCUSED          (CY_BITMAP + BUTTON_LOG_OFF_PRESSED)
#define BUTTON_SWITCH_USER_DISABLED     (CY_BITMAP + BUTTON_LOG_OFF_FOCUSED) // Temporary

BOOL DrawIconOnOwnerDrawnButtons(DRAWITEMSTRUCT* pdis, PLOGOFF_DLG_CONTEXT pContext)
{
    BOOL bRet = FALSE;
    HDC hdcMem = NULL;
    HBITMAP hbmOld = NULL;
    int y = 0;
    RECT rect;

    hdcMem = CreateCompatibleDC(pdis->hDC);
    hbmOld = (HBITMAP)SelectObject(hdcMem, pContext->hImageStrip);
    rect = pdis->rcItem;

    /* Check the button ID for revelant bitmap to be used */
    switch (pdis->CtlID)
    {
        case IDC_LOG_OFF_BUTTON:
        {
            switch (pdis->itemAction)
            {
                case ODA_DRAWENTIRE:
                case ODA_FOCUS:
                case ODA_SELECT:
                {
                    y = BUTTON_LOG_OFF;
                    if (pdis->itemState & ODS_SELECTED)
                    {
                        y = BUTTON_LOG_OFF_PRESSED;
                    }
                    else if (pContext->bIsButtonHot[0] || (pdis->itemState & ODS_FOCUS))
                    {
                        y = BUTTON_LOG_OFF_FOCUSED;
                    }
                    break;
                }
            }
            break;
        }

        case IDC_SWITCH_USER_BUTTON:
        {
            switch (pdis->itemAction)
            {
                case ODA_DRAWENTIRE:
                case ODA_FOCUS:
                case ODA_SELECT:
                {
                    y = BUTTON_SWITCH_USER;
                    if (pdis->itemState & ODS_SELECTED)
                    {
                        y = BUTTON_SWITCH_USER_PRESSED;
                    }
                    else if (pContext->bIsButtonHot[1] || (pdis->itemState & ODS_FOCUS))
                    {
                        y = BUTTON_SWITCH_USER_FOCUSED;
                    }

                    /*
                     * Since switch user functionality isn't implemented yet therefore the button has been disabled
                     * temporarily hence show the disabled state
                     */
                    else if (pdis->itemState & ODS_DISABLED)
                    {
                        y = BUTTON_SWITCH_USER_DISABLED;
                    }
                    break;
                }
            }
            break;
        }
    }

    /* Draw it on the required button */
    bRet = BitBlt(pdis->hDC,
                  (rect.right - rect.left - CX_BITMAP) / 2,
                  (rect.bottom - rect.top - CY_BITMAP) / 2,
                  CX_BITMAP, CY_BITMAP, hdcMem, 0, y, SRCCOPY);

    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);

    return bRet;
}

INT_PTR CALLBACK OwnerDrawButtonSubclass(HWND hButton, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PLOGOFF_DLG_CONTEXT pContext;
    pContext = (PLOGOFF_DLG_CONTEXT)GetWindowLongPtrW(hButton, GWLP_USERDATA);

    int buttonID = GetDlgCtrlID(hButton);

    switch (uMsg)
    {
        case WM_MOUSEMOVE:
        {
            HWND hwndTarget = NULL;
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};;

            if (GetCapture() != hButton)
            {
                SetCapture(hButton);
                if (buttonID == IDC_LOG_OFF_BUTTON)
                {
                    pContext->bIsButtonHot[0] = TRUE;
                }
                else if (buttonID == IDC_SWITCH_USER_BUTTON)
                {
                    pContext->bIsButtonHot[1] = TRUE;
                }
                SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_HAND)));
            }

            ClientToScreen(hButton, &pt);
            hwndTarget = WindowFromPoint(pt);

            if (hwndTarget != hButton)
            {
                ReleaseCapture();
                if (buttonID == IDC_LOG_OFF_BUTTON)
                {
                    pContext->bIsButtonHot[0] = FALSE;
                }
                else if (buttonID == IDC_SWITCH_USER_BUTTON)
                {
                    pContext->bIsButtonHot[1] = FALSE;
                }
            }
            InvalidateRect(hButton, NULL, FALSE);
            break;
        }

        /* Whenever one of the buttons gets the keyboard focus, set it as default button */
        case WM_SETFOCUS:
        {
            SendMessageW(GetParent(hButton), DM_SETDEFID, buttonID, 0);
            break;
        }

        /* Otherwise, set IDCANCEL as default button */
        case WM_KILLFOCUS:
        {
            SendMessageW(GetParent(hButton), DM_SETDEFID, IDCANCEL, 0);
            break;
        }
    }
    return CallWindowProcW(pContext->OldButtonProc, hButton, uMsg, wParam, lParam);
}

VOID CreateToolTipForButtons(int controlID, int detailID, HWND hDlg, int titleID)
{
    HWND hwndTool = NULL, hwndTip = NULL;
    WCHAR szBuffer[256];
    TTTOOLINFOW tool;

    hwndTool = GetDlgItem(hDlg, controlID);

    tool.cbSize = sizeof(tool);
    tool.hwnd = hDlg;
    tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tool.uId = (UINT_PTR)hwndTool;

    /* Create the tooltip */
    hwndTip = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL,
                              WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              hDlg, NULL, shell32_hInstance, NULL);

    /* Associate the tooltip with the tool. */
    LoadStringW(shell32_hInstance, detailID, szBuffer, _countof(szBuffer));
    tool.lpszText = szBuffer;
    SendMessageW(hwndTip, TTM_ADDTOOLW, 0, (LPARAM)&tool);
    LoadStringW(shell32_hInstance, titleID, szBuffer, _countof(szBuffer));
    SendMessageW(hwndTip, TTM_SETTITLEW, TTI_NONE, (LPARAM)szBuffer);
    SendMessageW(hwndTip, TTM_SETMAXTIPWIDTH, 0, 250);
}

static BOOL IsFriendlyUIActive(VOID)
{
    DWORD dwType = 0, dwValue = 0, dwSize = 0;
    HKEY hKey = NULL;
    LONG lRet = 0;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Control\\Windows",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        return FALSE;

    /* First check an optional ReactOS specific override, that Windows does not check.
       We use this to allow users pairing 'Server'-configuration with FriendlyLogoff.
       Otherwise users would have to change CSDVersion or LogonType (side-effects AppCompat) */
    dwValue = 0;
    dwSize = sizeof(dwValue);
    lRet = RegQueryValueExW(hKey,
                            L"EnforceFriendlyLogoff",
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize);

    if (lRet == ERROR_SUCCESS && dwType == REG_DWORD && dwValue != FRIENDLY_LOGOFF_IS_NOT_ENFORCED)
    {
        RegCloseKey(hKey);
        return TRUE;
    }

    /* Check product version number */
    dwValue = 0;
    dwSize = sizeof(dwValue);
    lRet = RegQueryValueExW(hKey,
                            L"CSDVersion",
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize);
    RegCloseKey(hKey);

    if (lRet != ERROR_SUCCESS || dwType != REG_DWORD || dwValue != IS_PRODUCT_VERSION_WORKSTATION)
    {
        /* Allow Friendly UI only on Workstation */
        return FALSE;
    }

    /* Check LogonType value */
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        return FALSE;

    dwValue = 0;
    dwSize = sizeof(dwValue);
    lRet = RegQueryValueExW(hKey,
                            L"LogonType",
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize);
    RegCloseKey(hKey);

    if (lRet != ERROR_SUCCESS || dwType != REG_DWORD)
        return FALSE;

    return (dwValue != 0);
}

static VOID FancyLogoffOnInit(HWND hwnd, PLOGOFF_DLG_CONTEXT pContext)
{
    HDC hdc = NULL;
    LONG lfHeight = NULL;

    hdc = GetDC(NULL);
    lfHeight = -MulDiv(FONT_POINT_SIZE, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    pContext->hfFont = CreateFontW(lfHeight, 0, 0, 0, FW_MEDIUM, FALSE, 0, 0, 0, 0, 0, 0, 0, L"MS Shell Dlg");
    SendDlgItemMessageW(hwnd, IDC_LOG_OFF_TEXT_STATIC, WM_SETFONT, (WPARAM)pContext->hfFont, TRUE);

    pContext->hBrush = CreateSolidBrush(DARK_GREY_COLOR);

    pContext->hImageStrip = LoadBitmapW(shell32_hInstance, MAKEINTRESOURCEW(IDB_IMAGE_STRIP));

    CreateToolTipForButtons(IDC_LOG_OFF_BUTTON, IDS_LOG_OFF_DESC, hwnd, IDS_LOG_OFF_TITLE);
    CreateToolTipForButtons(IDC_SWITCH_USER_BUTTON, IDS_SWITCH_USER_DESC, hwnd, IDS_SWITCH_USER_TITLE);

    /* Gather old button func */
    pContext->OldButtonProc = (WNDPROC)GetWindowLongPtrW(GetDlgItem(hwnd, IDC_LOG_OFF_BUTTON), GWLP_WNDPROC);

    /* Make buttons to remember pContext and subclass the buttons as well as set bIsButtonHot boolean flags to false */
    for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
    {
        pContext->bIsButtonHot[i] = FALSE;
        SetWindowLongPtrW(GetDlgItem(hwnd, IDC_LOG_OFF_BUTTON + i), GWLP_USERDATA, (LONG_PTR)pContext);
        SetWindowLongPtrW(GetDlgItem(hwnd, IDC_LOG_OFF_BUTTON + i), GWLP_WNDPROC, (LONG_PTR)OwnerDrawButtonSubclass);
    }
}

/*************************************************************************
 * LogOffDialogProc
 *
 * NOTES: Used to make the Log Off dialog work
 */
INT_PTR CALLBACK LogOffDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
    PLOGOFF_DLG_CONTEXT pContext;
    pContext = (PLOGOFF_DLG_CONTEXT)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pContext = (PLOGOFF_DLG_CONTEXT)lParam;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pContext);

            if (pContext->bFriendlyUI)
                FancyLogoffOnInit(hwnd, pContext);
            return TRUE;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            PostQuitMessage(IDCANCEL);
            break;

        /*
        * If the user deactivates the log off dialog (it loses its focus
        * while the dialog is not being closed), then destroy the dialog
        * box.
        */
        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                DestroyWindow(hwnd);
                PostQuitMessage(0);
            }
            return FALSE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_LOG_OFF_BUTTON:
                case IDOK:
                    ExitWindowsEx(EWX_LOGOFF, 0);
                    break;

                case IDCANCEL:
                    DestroyWindow(hwnd);
                    PostQuitMessage(IDCANCEL);
                    break;
            }
            break;

        case WM_DESTROY:
            DeleteObject(pContext->hBrush);
            DeleteObject(pContext->hImageStrip);
            DeleteObject(pContext->hfFont);

            /* Remove the subclass from the buttons */
            for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
            {
                SetWindowLongPtrW(GetDlgItem(hwnd, IDC_LOG_OFF_BUTTON + i), GWLP_WNDPROC, (LONG_PTR)pContext->OldButtonProc);
            }
            return TRUE;

        case WM_CTLCOLORSTATIC:
        {
            /* Either make background transparent or fill it with color for required static controls */
            HDC hdcStatic = (HDC)wParam;
            UINT StaticID = (UINT)GetWindowLongPtrW((HWND)lParam, GWL_ID);

            switch (StaticID)
            {
                case IDC_LOG_OFF_TEXT_STATIC:
                   SetTextColor(hdcStatic, DARK_GREY_COLOR);
                   SetBkMode(hdcStatic, TRANSPARENT);
                   return (INT_PTR)GetStockObject(HOLLOW_BRUSH);

                case IDC_LOG_OFF_STATIC:
                case IDC_SWITCH_USER_STATIC:
                    SetTextColor(hdcStatic, LIGHT_GREY_COLOR);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (LONG_PTR)pContext->hBrush;
            }
            return FALSE;
        }
        break;

        case WM_DRAWITEM:
        {
            /* Draw bitmaps on required buttons */
            switch (pdis->CtlID)
            {
                case IDC_LOG_OFF_BUTTON:
                case IDC_SWITCH_USER_BUTTON:
                    return DrawIconOnOwnerDrawnButtons(pdis, pContext);
            }
        }
        break;

        default:
            break;
    }
    return FALSE;
}

/*************************************************************************
 * LogoffWindowsDialog  [SHELL32.54]
 */

EXTERN_C int WINAPI LogoffWindowsDialog(HWND hWndOwner)
{
    CComPtr<IUnknown> fadeHandler;
    BOOL bIsAltKeyPressed = FALSE;
    MSG Msg;
    HWND parent = NULL;
    HWND hWndChild = NULL;
    WCHAR szBuffer[30];
    DWORD LogoffDialogID = IDD_LOG_OFF;
    LOGOFF_DLG_CONTEXT Context;

    if (!CallShellDimScreen(&fadeHandler, &parent))
        parent = hWndOwner;

    Context.bFriendlyUI = IsFriendlyUIActive();
    if (Context.bFriendlyUI)
    {
        LogoffDialogID = IDD_LOG_OFF_FANCY;
    }

    hWndChild = CreateDialogParamW(shell32_hInstance, MAKEINTRESOURCEW(LogoffDialogID), parent, LogOffDialogProc, (LPARAM)&Context);
    ShowWindow(hWndChild, SW_SHOWNORMAL);

     /* Detect either Alt key has been pressed */
    while (GetMessageW(&Msg, NULL, 0, 0))
    {
        if(!IsDialogMessageW(hWndChild, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }

        switch (Msg.message)
        {
            case WM_SYSKEYDOWN:
            {
                /* If the Alt key has been pressed once, add prefix to static controls */
                if (Msg.wParam == VK_MENU && !bIsAltKeyPressed && Context.bFriendlyUI)
                {
                    for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
                    {
                        GetDlgItemTextW(hWndChild, IDC_LOG_OFF_BUTTON + i, szBuffer, _countof(szBuffer));
                        SetDlgItemTextW(hWndChild, IDC_LOG_OFF_STATIC + i, szBuffer);
                    }
                    bIsAltKeyPressed = TRUE;
                }
            }
            break;
        }
    }
    return 0;
}

/*************************************************************************
 * RestartDialog                [SHELL32.59]
 */

int WINAPI RestartDialog(HWND hWndOwner, LPCWSTR lpstrReason, DWORD uFlags)
{
    return RestartDialogEx(hWndOwner, lpstrReason, uFlags, 0);
}

/*************************************************************************
 * ExitWindowsDialog_backup
 *
 * NOTES
 *     Used as a backup solution to shutdown the OS in case msgina.dll
 *     somehow cannot be found.
 */
VOID ExitWindowsDialog_backup(HWND hWndOwner)
{
    TRACE("(%p)\n", hWndOwner);

    if (ConfirmDialog(hWndOwner, IDS_SHUTDOWN_PROMPT, IDS_SHUTDOWN_TITLE))
    {
        EnablePrivilege(L"SeShutdownPrivilege", TRUE);
        ExitWindowsEx(EWX_SHUTDOWN, 0);
        EnablePrivilege(L"SeShutdownPrivilege", FALSE);
    }
}

/*************************************************************************
 * ExitWindowsDialog                [SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
/*
 * TODO:
 * - Implement the ability to show either the Welcome Screen or the classic dialog boxes based upon the
 *   registry value: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\LogonType.
 */
void WINAPI ExitWindowsDialog(HWND hWndOwner)
{
    typedef DWORD (WINAPI *ShellShFunc)(HWND hParent, WCHAR *Username, BOOL bHideLogoff);
    HINSTANCE msginaDll = LoadLibraryW(L"msgina.dll");

    TRACE("(%p)\n", hWndOwner);

    CComPtr<IUnknown> fadeHandler;
    HWND parent;
    if (!CallShellDimScreen(&fadeHandler, &parent))
        parent = hWndOwner;

    /* If the DLL cannot be found for any reason, then it simply uses a
       dialog box to ask if the user wants to shut down the computer. */
    if (!msginaDll)
    {
        TRACE("Unable to load msgina.dll.\n");
        ExitWindowsDialog_backup(parent);
        return;
    }

    ShellShFunc pShellShutdownDialog = (ShellShFunc)GetProcAddress(msginaDll, "ShellShutdownDialog");

    if (pShellShutdownDialog)
    {
        /* Actually call the function */
        DWORD returnValue = pShellShutdownDialog(parent, NULL, FALSE);

        switch (returnValue)
        {
        case 0x01: /* Log off user */
        {
            ExitWindowsEx(EWX_LOGOFF, 0);
            break;
        }
        case 0x02: /* Shut down */
        {
            EnablePrivilege(L"SeShutdownPrivilege", TRUE);
            ExitWindowsEx(EWX_SHUTDOWN, 0);
            EnablePrivilege(L"SeShutdownPrivilege", FALSE);
            break;
        }
        case 0x03: /* Install Updates/Shutdown (?) */
        {
            break;
        }
        case 0x04: /* Reboot */
        {
            EnablePrivilege(L"SeShutdownPrivilege", TRUE);
            ExitWindowsEx(EWX_REBOOT, 0);
            EnablePrivilege(L"SeShutdownPrivilege", FALSE);
            break;
        }
        case 0x10: /* Sleep */
        {
            if (IsPwrSuspendAllowed())
            {
                EnablePrivilege(L"SeShutdownPrivilege", TRUE);
                SetSuspendState(FALSE, FALSE, FALSE);
                EnablePrivilege(L"SeShutdownPrivilege", FALSE);
            }
            break;
        }
        case 0x40: /* Hibernate */
        {
            if (IsPwrHibernateAllowed())
            {
                EnablePrivilege(L"SeShutdownPrivilege", TRUE);
                SetSuspendState(TRUE, FALSE, TRUE);
                EnablePrivilege(L"SeShutdownPrivilege", FALSE);
            }
            break;
        }
        /* If the option is any other value */
        default:
            break;
        }
    }
    else
    {
        /* If the function cannot be found, then revert to using the backup solution */
        TRACE("Unable to find the 'ShellShutdownDialog' function");
        ExitWindowsDialog_backup(parent);
    }

    FreeLibrary(msginaDll);
}
