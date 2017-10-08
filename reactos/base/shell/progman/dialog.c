/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle
 * Copyright 2002 Andriy Palamarchuk
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

/*
 * PROJECT:         ReactOS Program Manager
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            base/shell/progman/dialog.c
 * PURPOSE:         ProgMan dialog boxes
 * PROGRAMMERS:     Ulrich Schmid
 *                  Sylvain Petreolle
 *                  Andriy Palamarchuk
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "progman.h"

#include <commdlg.h>

/***********************************************************************
 *
 *           DIALOG_AddFilterItem and DIALOG_BrowseXXX
 */

static
VOID
DIALOG_AddFilterItem(LPWSTR* p, UINT ids, LPCWSTR filter)
{
    LoadStringW(Globals.hInstance, ids, *p, MAX_STRING_LEN);
    *p += wcslen(*p) + 1;
    lstrcpyW(*p, filter);
    *p += wcslen(*p) + 1;
    **p = '\0';
}

static
BOOL
DIALOG_Browse(HWND hWnd, LPCWSTR lpszzFilter, LPWSTR lpstrFile, INT nMaxFile)
{
    OPENFILENAMEW openfilename;
    WCHAR szDir[MAX_PATH];

    ZeroMemory(&openfilename, sizeof(openfilename));

    GetCurrentDirectoryW(ARRAYSIZE(szDir), szDir);

    openfilename.lStructSize       = sizeof(openfilename);
    openfilename.hwndOwner         = hWnd;
    openfilename.hInstance         = Globals.hInstance;
    openfilename.lpstrFilter       = lpszzFilter;
    openfilename.lpstrFile         = lpstrFile;
    openfilename.nMaxFile          = nMaxFile;
    openfilename.lpstrInitialDir   = szDir;
    openfilename.Flags             = 0;
    openfilename.lpstrDefExt       = L"exe";
    openfilename.lpstrCustomFilter = NULL;
    openfilename.nMaxCustFilter    = 0;
    openfilename.nFilterIndex      = 0;
    openfilename.lpstrFileTitle    = NULL;
    openfilename.nMaxFileTitle     = 0;
    openfilename.lpstrTitle        = NULL;
    openfilename.nFileOffset       = 0;
    openfilename.nFileExtension    = 0;
    openfilename.lCustData         = 0;
    openfilename.lpfnHook          = NULL;
    openfilename.lpTemplateName    = NULL;

    return GetOpenFileNameW(&openfilename);
}

static
BOOL
DIALOG_BrowsePrograms(HWND hWnd, LPWSTR lpszFile, INT nMaxFile)
{
    WCHAR szzFilter[2 * MAX_STRING_LEN + 100];
    LPWSTR p = szzFilter;

    DIALOG_AddFilterItem(&p, IDS_PROGRAMS , L"*.exe;*.pif;*.com;*.bat;*.cmd");
    DIALOG_AddFilterItem(&p, IDS_ALL_FILES, L"*.*");

    return DIALOG_Browse(hWnd, szzFilter, lpszFile, nMaxFile);
}

static
BOOL
DIALOG_BrowseSymbols(HWND hWnd, LPWSTR lpszFile, INT nMaxFile)
{
    WCHAR szzFilter[5 * MAX_STRING_LEN + 100];
    LPWSTR p = szzFilter;

    DIALOG_AddFilterItem(&p, IDS_SYMBOL_FILES, L"*.ico;*.exe;*.dll");
    DIALOG_AddFilterItem(&p, IDS_PROGRAMS, L"*.exe");
    DIALOG_AddFilterItem(&p, IDS_LIBRARIES_DLL, L"*.dll");
    DIALOG_AddFilterItem(&p, IDS_SYMBOLS_ICO, L"*.ico");
    DIALOG_AddFilterItem(&p, IDS_ALL_FILES, L"*.*");

    return DIALOG_Browse(hWnd, szzFilter, lpszFile, nMaxFile);
}



/***********************************************************************
 *
 *           DIALOG_New
 */

LPCWSTR GroupFormatToFormatName(GROUPFORMAT Format)
{
    static const LPCWSTR FormatNames[] =
    {
        L"Windows 3.1",
        L"NT Ansi",
        L"NT Unicode"
    };

    if (Format > NT_Unicode)
        return NULL;
    else
        return FormatNames[Format];
}

typedef struct _NEW_ITEM_CONTEXT
{
    INT nDefault;
    INT nResult;
} NEW_ITEM_CONTEXT, *PNEW_ITEM_CONTEXT;

static
INT_PTR
CALLBACK
DIALOG_NEW_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PNEW_ITEM_CONTEXT pNewItem;
    GROUPFORMAT format;
    INT iItem;

    pNewItem = (PNEW_ITEM_CONTEXT)GetWindowLongPtrW(hDlg, 8);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pNewItem = (PNEW_ITEM_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, 8, lParam);

            for (format = Win_311; format <= NT_Unicode; ++format)
            {
                iItem = SendDlgItemMessageW(hDlg, PM_FORMAT, CB_INSERTSTRING, 0, (LPARAM)GroupFormatToFormatName(format));
                if (iItem != CB_ERR && iItem != CB_ERRSPACE)
                    SendDlgItemMessageW(hDlg, PM_FORMAT, CB_SETITEMDATA, iItem, format);
            }

            SendDlgItemMessageW(hDlg, PM_FORMAT, CB_SETCURSEL, 0, 0);
            CheckRadioButton(hDlg, PM_NEW_GROUP, PM_NEW_PROGRAM, pNewItem->nDefault);
            CheckRadioButton(hDlg, PM_PERSONAL_GROUP, PM_COMMON_GROUP, PM_PERSONAL_GROUP);

            EnableDlgItem(hDlg, PM_NEW_PROGRAM, GROUP_ActiveGroup() != NULL);

            SendMessageW(hDlg, WM_COMMAND, pNewItem->nDefault, 0);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case PM_NEW_GROUP:
                {
                    CheckRadioButton(hDlg, PM_NEW_GROUP, PM_NEW_PROGRAM, wParam);
                    EnableDlgItem(hDlg, PM_COMMON_GROUP  , TRUE);
                    EnableDlgItem(hDlg, PM_PERSONAL_GROUP, TRUE);
                    EnableDlgItem(hDlg, PM_FORMAT_TXT, TRUE);
                    EnableDlgItem(hDlg, PM_FORMAT    , TRUE);
                    return TRUE;
                }

                case PM_NEW_PROGRAM:
                {
                    CheckRadioButton(hDlg, PM_NEW_GROUP, PM_NEW_PROGRAM, wParam);
                    EnableDlgItem(hDlg, PM_COMMON_GROUP  , FALSE);
                    EnableDlgItem(hDlg, PM_PERSONAL_GROUP, FALSE);
                    EnableDlgItem(hDlg, PM_FORMAT_TXT, FALSE);
                    EnableDlgItem(hDlg, PM_FORMAT    , FALSE);
                    return TRUE;
                }

                case IDHELP:
                    MAIN_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
                    return TRUE;

                case IDOK:
                {
                    iItem = SendDlgItemMessageW(hDlg, PM_FORMAT, CB_GETCURSEL, 0, 0);

                    format = SendDlgItemMessageW(hDlg, PM_FORMAT, CB_GETITEMDATA, iItem, 0);
                    format = min(max(format, Win_311), NT_Unicode);

                    pNewItem->nResult  =  !!IsDlgButtonChecked(hDlg, PM_NEW_GROUP);
                    pNewItem->nResult |= (!!IsDlgButtonChecked(hDlg, PM_COMMON_GROUP) << 1);
                    pNewItem->nResult |= (format << 2);

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
    }

    return FALSE;
}

BOOL DIALOG_New(INT nDefault, PINT pnResult)
{
    INT_PTR ret;
    NEW_ITEM_CONTEXT NewItem;

    *pnResult = 0;

    NewItem.nDefault = nDefault;
    NewItem.nResult  = 0;
    ret = DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_NEW), Globals.hMainWnd, DIALOG_NEW_DlgProc, (LPARAM)&NewItem);
    if (ret == IDOK)
        *pnResult = NewItem.nResult;

    return (ret == IDOK);
}


/***********************************************************************
 *
 *           DIALOG_CopyMove
 */

typedef struct _COPY_MOVE_CONTEXT
{
    PROGRAM*   Program;
    PROGGROUP* hToGroup;
    BOOL       bMove;
} COPY_MOVE_CONTEXT, *PCOPY_MOVE_CONTEXT;

static
INT_PTR
CALLBACK
DIALOG_COPY_MOVE_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PROGGROUP* hGrpItem;
    PROGGROUP* hGroup;

    PCOPY_MOVE_CONTEXT pCopyMove;
    INT iItem;

    WCHAR text[MAX_STRING_LEN];

    pCopyMove = (PCOPY_MOVE_CONTEXT)GetWindowLongPtrW(hDlg, 8);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pCopyMove = (PCOPY_MOVE_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, 8, lParam);

            if (pCopyMove->bMove)
            {
                LoadStringW(Globals.hInstance, IDS_MOVE_PROGRAM_1, text, ARRAYSIZE(text));
                SetWindowTextW(hDlg, text);
                LoadStringW(Globals.hInstance, IDS_MOVE_PROGRAM_2, text, ARRAYSIZE(text));
                SetDlgItemTextW(hDlg, PM_COPY_MOVE_TXT, text);
            }

            /* List all the group names but the source group, in case we are doing a move */
            for (hGroup = Globals.hGroups; hGroup; hGroup = hGroup->hNext)
            {
                if (!pCopyMove->bMove || hGroup != pCopyMove->Program->hGroup)
                {
                    iItem = SendDlgItemMessageW(hDlg, PM_TO_GROUP, CB_ADDSTRING, 0, (LPARAM)hGroup->hName);
                    if (iItem != CB_ERR && iItem != CB_ERRSPACE)
                        SendDlgItemMessageW(hDlg, PM_TO_GROUP, CB_SETITEMDATA, iItem, (LPARAM)hGroup);
                }
            }
            SendDlgItemMessageW(hDlg, PM_TO_GROUP, CB_SETCURSEL, 0, 0);
            SetDlgItemTextW(hDlg, PM_PROGRAM   , pCopyMove->Program->hName);
            SetDlgItemTextW(hDlg, PM_FROM_GROUP, pCopyMove->Program->hGroup->hName);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDHELP:
                    MAIN_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
                    return TRUE;

                case IDOK:
                {
                    /* Get the selected group */
                    iItem = SendDlgItemMessageW(hDlg, PM_TO_GROUP, CB_GETCURSEL, 0, 0);
                    hGrpItem = (PROGGROUP *)SendDlgItemMessageW(hDlg, PM_TO_GROUP, CB_GETITEMDATA, iItem, 0);
                    /* Check that it is indeed in the group list */
                    for (hGroup = Globals.hGroups; hGroup && hGroup != hGrpItem; hGroup = hGroup->hNext)
                        ;
                    if (pCopyMove->bMove)
                    {
                        if (hGrpItem == pCopyMove->Program->hGroup)
                            hGrpItem = NULL;
                    }
                    pCopyMove->hToGroup = hGrpItem;
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
    }

    return FALSE;
}

PROGGROUP* DIALOG_CopyMove(PROGRAM* hProgram, BOOL bMove)
{
    COPY_MOVE_CONTEXT CopyMove;
    INT_PTR ret;

    CopyMove.bMove    = bMove;
    CopyMove.Program  = hProgram;
    CopyMove.hToGroup = NULL;
    ret = DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_COPY_MOVE), Globals.hMainWnd, DIALOG_COPY_MOVE_DlgProc, (LPARAM)&CopyMove);

    return (ret == IDOK ? CopyMove.hToGroup : NULL);
}

/***********************************************************************
 *
 *           DIALOG_Delete
 */

BOOL DIALOG_Delete(UINT ids_text_s, LPCWSTR lpszName)
{
    return (MAIN_MessageBoxIDS_s(ids_text_s, lpszName, IDS_DELETE, MB_YESNO | MB_DEFBUTTON2) == IDYES);
}


/* Adapted from dll/win32/shell32/dialogs/dialogs.cpp!EnableOkButtonFromEditContents */
BOOL ValidateEditContents(HWND hDlg, INT nIDEditItem)
{
    BOOL Enable = FALSE;
    LPWSTR psz;
    INT Length, n;
    HWND Edit;

    Edit = GetDlgItem(hDlg, nIDEditItem);
    Length = GetWindowTextLengthW(Edit);

    if (Length <= 0)
        return FALSE;

    psz = Alloc(0, (Length + 1) * sizeof(WCHAR));
    if (psz)
    {
        GetWindowTextW(Edit, psz, Length + 1);
        for (n = 0; n < Length && !Enable; ++n)
            Enable = (psz[n] != ' ');
        Free(psz);
    }
    else
    {
        Enable = TRUE;
    }

    return Enable;
}


/***********************************************************************
 *
 *           DIALOG_GroupAttributes
 */

typedef struct _GROUP_ATTRIBUTES_CONTEXT
{
    GROUPFORMAT format;
    LPWSTR lpszTitle;
    LPWSTR lpszGrpFile;
    INT nSize;
} GROUP_ATTRIBUTES_CONTEXT, *PGROUP_ATTRIBUTES_CONTEXT;

static
INT_PTR
CALLBACK
DIALOG_GROUP_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PGROUP_ATTRIBUTES_CONTEXT pGroupAttributes;

    pGroupAttributes = (PGROUP_ATTRIBUTES_CONTEXT)GetWindowLongPtrW(hDlg, 8);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            DWORD evMask;

            pGroupAttributes = (PGROUP_ATTRIBUTES_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, 8, lParam);

            /* Configure Richedit control for sending notification changes */
            evMask = SendDlgItemMessageW(hDlg, PM_DESCRIPTION, EM_GETEVENTMASK, 0, 0) | ENM_CHANGE;
            SendDlgItemMessageW(hDlg, PM_DESCRIPTION, EM_SETEVENTMASK, 0, (LPARAM)evMask);

            SetDlgItemTextW(hDlg, PM_DESCRIPTION, pGroupAttributes->lpszTitle);

            if (pGroupAttributes->format != Win_311)
            {
                EnableDlgItem(hDlg, PM_FILE, FALSE);
            }
            else
            {
                EnableDlgItem(hDlg, PM_FILE, TRUE);
                SetDlgItemTextW(hDlg, PM_FILE, pGroupAttributes->lpszGrpFile);
            }

            EnableDlgItem(hDlg, IDOK, ValidateEditContents(hDlg, PM_DESCRIPTION));
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDHELP:
                    MAIN_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
                    return TRUE;

                case PM_DESCRIPTION:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                        EnableDlgItem(hDlg, IDOK, ValidateEditContents(hDlg, PM_DESCRIPTION));
                    return FALSE;
                }

                case IDOK:
                {
                    GetDlgItemTextW(hDlg, PM_DESCRIPTION, pGroupAttributes->lpszTitle, pGroupAttributes->nSize);
                    if (pGroupAttributes->format)
                        *pGroupAttributes->lpszGrpFile = '\0';
                    else
                        GetDlgItemTextW(hDlg, PM_FILE, pGroupAttributes->lpszGrpFile, pGroupAttributes->nSize);
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
    }

    return FALSE;
}

BOOL DIALOG_GroupAttributes(GROUPFORMAT format, LPWSTR lpszTitle, LPWSTR lpszGrpFile, INT nSize)
{
    INT_PTR ret;
    GROUP_ATTRIBUTES_CONTEXT GroupAttributes;

    GroupAttributes.format      = format;
    GroupAttributes.nSize       = nSize;
    GroupAttributes.lpszTitle   = lpszTitle;
    GroupAttributes.lpszGrpFile = lpszGrpFile;

    ret = DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_GROUP), Globals.hMainWnd, DIALOG_GROUP_DlgProc, (LPARAM)&GroupAttributes);

    return (ret == IDOK);
}


/***********************************************************************
 *
 *           DIALOG_Symbol
 */

/* Adapted from dll/win32/shell32/dialogs/dialogs.cpp!EnumPickIconResourceProc */
static
BOOL
CALLBACK
EnumPickIconResourceProc(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
{
    HICON hIcon;
    HWND hDlgCtrl = (HWND)lParam;
    WCHAR szName[100];

    if (IS_INTRESOURCE(lpszName))
        StringCbPrintfW(szName, sizeof(szName), L"%u", (unsigned)(UINT_PTR)lpszName);
    else
        StringCbCopyW(szName, sizeof(szName), lpszName);

    hIcon = (HICON)LoadImageW(hModule, lpszName, IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    if (hIcon == NULL)
        return TRUE;

    SendMessageW(hDlgCtrl, LB_ADDSTRING, 0, (LPARAM)hIcon);
    return TRUE;
}

static
VOID
DestroyIconList(HWND hDlgCtrl)
{
    HICON hIcon;
    UINT count;

    count = SendMessageA(hDlgCtrl, LB_GETCOUNT, 0, 0);
    if (count == LB_ERR || count == 0)
        return;

    while (count-- > 0)
    {
        hIcon = (HICON)SendMessageA(hDlgCtrl, LB_GETITEMDATA, 0, 0);
        DestroyIcon(hIcon);
        SendMessageA(hDlgCtrl, LB_DELETESTRING, 0, 0);
    }
}

typedef struct _PICK_ICON_CONTEXT
{
    HMODULE hLibrary;
    HWND hDlgCtrl;
    WCHAR szName[MAX_PATH]; // LPWSTR lpszIconFile; // INT nSize;
    INT Index;
    HICON hIcon;
} PICK_ICON_CONTEXT, *PPICK_ICON_CONTEXT;

#if 0

static struct
{
  LPSTR  lpszIconFile;
  INT    nSize;
  HICON  *lphIcon;
  INT    *lpnIconIndex;
} Symbol;

#endif

static
INT_PTR
CALLBACK
DIALOG_SYMBOL_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR filename[MAX_PATHNAME_LEN];
    PPICK_ICON_CONTEXT pIconContext;

    pIconContext = (PPICK_ICON_CONTEXT)GetWindowLongPtrW(hDlg, 8);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pIconContext = (PPICK_ICON_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, 8, lParam);

            pIconContext->hDlgCtrl = GetDlgItem(hDlg, PM_SYMBOL_LIST);
            SetDlgItemTextW(hDlg, PM_ICON_FILE, pIconContext->szName);
            SendMessageA(pIconContext->hDlgCtrl, LB_SETITEMHEIGHT, 0, 32);

            pIconContext->hLibrary = LoadLibraryExW(pIconContext->szName, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
            if (pIconContext->hLibrary)
            {
                EnumResourceNamesW(pIconContext->hLibrary,
                                   (LPCWSTR)RT_GROUP_ICON,
                                   EnumPickIconResourceProc,
                                   (LONG_PTR)pIconContext->hDlgCtrl);
                FreeLibrary(pIconContext->hLibrary);
                pIconContext->hLibrary = NULL;
            }
            SendMessageA(pIconContext->hDlgCtrl, LB_SETCURSEL, pIconContext->Index, 0);
            return TRUE;
        }

        case WM_MEASUREITEM:
        {
            PMEASUREITEMSTRUCT measure = (PMEASUREITEMSTRUCT)lParam;
            measure->itemWidth  = 32;
            measure->itemHeight = 32;
            return TRUE;
        }

        case WM_DRAWITEM:
        {
            PDRAWITEMSTRUCT dis = (PDRAWITEMSTRUCT)lParam;

            if (dis->itemState & ODS_SELECTED)
                FillRect(dis->hDC, &dis->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
            else
                FillRect(dis->hDC, &dis->rcItem, (HBRUSH)(COLOR_WINDOW + 1));

            DrawIcon(dis->hDC, dis->rcItem.left, dis->rcItem.top, (HICON)dis->itemData);
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case PM_BROWSE:
                {
                    filename[0] = '\0';
                    if (!DIALOG_BrowseSymbols(hDlg, filename, ARRAYSIZE(filename)))
                        return TRUE;

                    if (_wcsnicmp(pIconContext->szName, filename, ARRAYSIZE(pIconContext->szName)) == 0)
                        return TRUE;

                    SetDlgItemTextW(hDlg, PM_ICON_FILE, filename);
                    DestroyIconList(pIconContext->hDlgCtrl);
                    pIconContext->hLibrary = LoadLibraryExW(filename, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
                    if (pIconContext->hLibrary)
                    {
                        EnumResourceNamesW(pIconContext->hLibrary,
                                           (LPCWSTR)RT_GROUP_ICON,
                                           EnumPickIconResourceProc,
                                           (LONG_PTR)pIconContext->hDlgCtrl);
                        FreeLibrary(pIconContext->hLibrary);
                        pIconContext->hLibrary = NULL;
                    }
                    SendMessageA(pIconContext->hDlgCtrl, LB_SETCURSEL, 0, 0);
                    return TRUE;
                }

                case IDHELP:
                    MAIN_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
                    return TRUE;

                case IDOK:
                {
                    INT nCurSel = SendMessageW(pIconContext->hDlgCtrl, LB_GETCURSEL, 0, 0);
                    GetDlgItemTextW(hDlg, PM_ICON_FILE, pIconContext->szName, ARRAYSIZE(pIconContext->szName));
                    pIconContext->hIcon = (HICON)SendMessageA(pIconContext->hDlgCtrl, LB_GETITEMDATA, nCurSel, 0);
                    pIconContext->hIcon = CopyIcon(pIconContext->hIcon);
                    DestroyIconList(pIconContext->hDlgCtrl);
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    DestroyIconList(pIconContext->hDlgCtrl);
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
    }

    return FALSE;
}

static
VOID
DIALOG_Symbol(HWND hWnd, HICON *lphIcon, LPWSTR lpszIconFile, INT *lpnIconIndex, INT nSize)
{
    PICK_ICON_CONTEXT IconContext;

    IconContext.Index = *lpnIconIndex;
    StringCchCopyNW(IconContext.szName, ARRAYSIZE(IconContext.szName), lpszIconFile, nSize);

    if (DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_SYMBOL), hWnd, DIALOG_SYMBOL_DlgProc, (LPARAM)&IconContext) != IDOK)
        return;

    StringCchCopyNW(lpszIconFile, nSize, IconContext.szName, ARRAYSIZE(IconContext.szName));
    *lpnIconIndex = IconContext.Index;
    *lphIcon = IconContext.hIcon;
}


/***********************************************************************
 *
 *           DIALOG_ProgramAttributes
 */

typedef struct _PROGRAM_ATTRIBUTES_CONTEXT
{
    LPWSTR lpszTitle;
    LPWSTR lpszCmdLine;
    LPWSTR lpszWorkDir;
    LPWSTR lpszIconFile;
    LPWSTR lpszTmpIconFile;
    INT    nSize;
    INT*   lpnCmdShow;
    INT*   lpnHotKey;
    // HWND   hSelGroupWnd;
    BOOL*  lpbNewVDM; // unused!
    HICON* lphIcon;
    HICON  hTmpIcon;
    INT*   lpnIconIndex;
    INT    nTmpIconIndex;
    BOOL   bCheckBinaryType;
} PROGRAM_ATTRIBUTES_CONTEXT, *PPROGRAM_ATTRIBUTES_CONTEXT;

static
INT_PTR
CALLBACK
DIALOG_PROGRAM_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bEnable;
    WCHAR filename[MAX_PATHNAME_LEN];
    DWORD evMask;
    DWORD dwBinaryType;
    PPROGRAM_ATTRIBUTES_CONTEXT pProgramAttributes;

    pProgramAttributes = (PPROGRAM_ATTRIBUTES_CONTEXT)GetWindowLongPtrW(hDlg, 8);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pProgramAttributes = (PPROGRAM_ATTRIBUTES_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, 8, lParam);

            evMask = SendDlgItemMessageW(hDlg, PM_COMMAND_LINE, EM_GETEVENTMASK, 0, 0) | ENM_CHANGE;
            SendDlgItemMessageW(hDlg, PM_COMMAND_LINE, EM_SETEVENTMASK, 0, evMask);

            SetDlgItemTextW(hDlg, PM_DESCRIPTION , pProgramAttributes->lpszTitle);
            SetDlgItemTextW(hDlg, PM_COMMAND_LINE, pProgramAttributes->lpszCmdLine);
            SetDlgItemTextW(hDlg, PM_DIRECTORY   , pProgramAttributes->lpszWorkDir);

            /*                                                  0x0F                                          0x06 */
            SendDlgItemMessageW(hDlg, PM_HOT_KEY, HKM_SETRULES, HKCOMB_C | HKCOMB_A | HKCOMB_S | HKCOMB_NONE, HOTKEYF_CONTROL | HOTKEYF_ALT);
            SendDlgItemMessageW(hDlg, PM_HOT_KEY, HKM_SETHOTKEY, *pProgramAttributes->lpnHotKey, 0);

            bEnable = ValidateEditContents(hDlg, PM_COMMAND_LINE);
            EnableWindow(GetDlgItem(hDlg, IDOK), bEnable);
            EnableWindow(GetDlgItem(hDlg, PM_OTHER_SYMBOL), bEnable);

            CheckDlgButton(hDlg, PM_SYMBOL, *pProgramAttributes->lpnCmdShow == SW_SHOWMINNOACTIVE);

            if (pProgramAttributes->bCheckBinaryType &&
                (!GetBinaryTypeW(pProgramAttributes->lpszCmdLine, &dwBinaryType) || dwBinaryType != SCS_WOW_BINARY) )
            {
                EnableWindow(GetDlgItem(hDlg, PM_NEW_VDM), FALSE);
                CheckDlgButton(hDlg, PM_NEW_VDM, BST_CHECKED);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, PM_NEW_VDM), TRUE);
                CheckDlgButton(hDlg, PM_NEW_VDM, *pProgramAttributes->lpbNewVDM);
            }
            SendDlgItemMessageW(hDlg, PM_ICON, STM_SETICON, (WPARAM)pProgramAttributes->hTmpIcon, 0);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case PM_NEW_VDM:
                    CheckDlgButton(hDlg, PM_NEW_VDM, !IsDlgButtonChecked(hDlg, PM_NEW_VDM));
                    return TRUE;

                case PM_BROWSE:
                {
                    filename[0] = '\0';
                    if (DIALOG_BrowsePrograms(hDlg, filename, ARRAYSIZE(filename)))
                    {
                        SetDlgItemTextW(hDlg, PM_COMMAND_LINE, filename);
                        if (pProgramAttributes->bCheckBinaryType &&
                            (!GetBinaryTypeW(filename, &dwBinaryType) || dwBinaryType != SCS_WOW_BINARY))
                        {
                            EnableWindow(GetDlgItem(hDlg, PM_NEW_VDM), FALSE);
                            CheckDlgButton(hDlg, PM_NEW_VDM, BST_CHECKED);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg, PM_NEW_VDM), TRUE);
                            CheckDlgButton(hDlg, PM_NEW_VDM, BST_UNCHECKED);
                        }
                    }
                    return TRUE;
                }

                case IDHELP:
                    MAIN_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
                    return TRUE;

                case PM_SYMBOL:
                    CheckDlgButton(hDlg, PM_SYMBOL, !IsDlgButtonChecked(hDlg, PM_SYMBOL));
                    return TRUE;

                case PM_OTHER_SYMBOL:
                {
                    DIALOG_Symbol(hDlg,
                                  &pProgramAttributes->hTmpIcon,
                                  pProgramAttributes->lpszTmpIconFile,
                                  &pProgramAttributes->nTmpIconIndex,
                                  MAX_PATHNAME_LEN);

                    SendDlgItemMessageW(hDlg, PM_ICON, STM_SETICON, (WPARAM)pProgramAttributes->hTmpIcon, 0);
                    return TRUE;
                }

                case PM_COMMAND_LINE:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        bEnable = ValidateEditContents(hDlg, PM_COMMAND_LINE);
                        EnableWindow(GetDlgItem(hDlg, IDOK), bEnable);
                        EnableWindow(GetDlgItem(hDlg, PM_OTHER_SYMBOL), bEnable);
                    }
                    return FALSE;
                }

                case IDOK:
                {
                    GetDlgItemTextW(hDlg, PM_DESCRIPTION , pProgramAttributes->lpszTitle  , pProgramAttributes->nSize);
                    GetDlgItemTextW(hDlg, PM_COMMAND_LINE, pProgramAttributes->lpszCmdLine, pProgramAttributes->nSize);
                    GetDlgItemTextW(hDlg, PM_DIRECTORY   , pProgramAttributes->lpszWorkDir, pProgramAttributes->nSize);
                    if (pProgramAttributes->hTmpIcon)
                    {
#if 0
    if (*pProgramAttributes->lphIcon)
        DestroyIcon(*pProgramAttributes->lphIcon);
#endif
                        *pProgramAttributes->lphIcon = pProgramAttributes->hTmpIcon;
                        *pProgramAttributes->lpnIconIndex = pProgramAttributes->nTmpIconIndex;
                        lstrcpynW(pProgramAttributes->lpszIconFile, pProgramAttributes->lpszTmpIconFile, pProgramAttributes->nSize);
                    }
                    *pProgramAttributes->lpnHotKey  = SendDlgItemMessageW(hDlg, PM_HOT_KEY, HKM_GETHOTKEY, 0, 0);
                    *pProgramAttributes->lpnCmdShow = IsDlgButtonChecked(hDlg, PM_SYMBOL) ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL;
                    *pProgramAttributes->lpbNewVDM  = IsDlgButtonChecked(hDlg, PM_NEW_VDM);
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
    }

    return FALSE;
}

BOOL
DIALOG_ProgramAttributes(LPWSTR lpszTitle, LPWSTR lpszCmdLine, LPWSTR lpszWorkDir, LPWSTR lpszIconFile,
                         HICON* lphIcon, INT* lpnIconIndex, INT* lpnHotKey, INT* lpnCmdShow, BOOL* lpbNewVDM, INT nSize)
{
    INT_PTR ret;
    WCHAR szTmpIconFile[MAX_PATHNAME_LEN];
    PROGRAM_ATTRIBUTES_CONTEXT ProgramAttributes;
    DWORD dwSize;
    DWORD dwType;

    ProgramAttributes.nSize        = nSize;
    ProgramAttributes.lpszTitle    = lpszTitle;
    ProgramAttributes.lpszCmdLine  = lpszCmdLine;
    ProgramAttributes.lpszWorkDir  = lpszWorkDir;
    ProgramAttributes.lpszIconFile = lpszIconFile;
    ProgramAttributes.lpnCmdShow   = lpnCmdShow;
    ProgramAttributes.lpnHotKey    = lpnHotKey;
    ProgramAttributes.lpbNewVDM    = lpbNewVDM;
    ProgramAttributes.lphIcon      = lphIcon;
    ProgramAttributes.lpnIconIndex = lpnIconIndex;

    dwSize = sizeof(ProgramAttributes.bCheckBinaryType);
    if (RegQueryValueExW(Globals.hKeyPMSettings,
                         L"CheckBinaryType",
                         0,
                         &dwType,
                         (LPBYTE)&ProgramAttributes.bCheckBinaryType,
                         &dwSize) != ERROR_SUCCESS
      || dwType != REG_DWORD)
    {
        ProgramAttributes.bCheckBinaryType = TRUE;
    }

#if 0
    ProgramAttributes.hTmpIcon = NULL;
#else
    ProgramAttributes.hTmpIcon        = *lphIcon;
#endif
    ProgramAttributes.nTmpIconIndex   = *lpnIconIndex;
    ProgramAttributes.lpszTmpIconFile = szTmpIconFile;
    wcsncpy(szTmpIconFile, lpszIconFile, ARRAYSIZE(szTmpIconFile));

    ret = DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_PROGRAM), Globals.hMainWnd, DIALOG_PROGRAM_DlgProc, (LPARAM)&ProgramAttributes);
    return (ret == IDOK);
}


/***********************************************************************
 *
 *           DIALOG_Execute
 */

typedef struct _EXECUTE_CONTEXT
{
    HKEY  hKeyPMRecentFilesList;
    DWORD dwMaxFiles;
    BOOL  bCheckBinaryType;
} EXECUTE_CONTEXT, *PEXECUTE_CONTEXT;

static
INT_PTR
CALLBACK
DIALOG_EXECUTE_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR szFile[MAX_PATHNAME_LEN]; // filename
    DWORD BinaryType;
    PEXECUTE_CONTEXT pExecuteContext;

    pExecuteContext = (PEXECUTE_CONTEXT)GetWindowLongPtrW(hDlg, 8);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pExecuteContext = (PEXECUTE_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, 8, lParam);

            EnableDlgItem(hDlg, IDOK, ValidateEditContents(hDlg, PM_COMMAND));

            if (pExecuteContext->bCheckBinaryType)
            {
                EnableDlgItem(hDlg, PM_NEW_VDM, FALSE);
                CheckDlgButton(hDlg, PM_NEW_VDM, BST_CHECKED);
            }
            else
            {
                EnableDlgItem(hDlg, PM_NEW_VDM, TRUE);
            }

            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case PM_SYMBOL:
                    CheckDlgButton(hDlg, PM_SYMBOL, !IsDlgButtonChecked(hDlg, PM_SYMBOL));
                    return TRUE;

                case PM_NEW_VDM:
                    CheckDlgButton(hDlg, PM_NEW_VDM, !IsDlgButtonChecked(hDlg, PM_NEW_VDM));
                    return TRUE;

                case PM_BROWSE:
                {
                    szFile[0] = '\0';
                    if (DIALOG_BrowsePrograms(hDlg, szFile, ARRAYSIZE(szFile)))
                    {
                        SetWindowTextW(GetDlgItem(hDlg, PM_COMMAND), szFile);
                        SetFocus(GetDlgItem(hDlg, IDOK));
                        if (pExecuteContext->bCheckBinaryType &&
                            (!GetBinaryTypeW(szFile, &BinaryType) || BinaryType != SCS_WOW_BINARY) )
                        {
                            EnableDlgItem(hDlg, PM_NEW_VDM, FALSE);
                            CheckDlgButton(hDlg, PM_NEW_VDM, BST_CHECKED);
                        }
                        else
                        {
                            EnableDlgItem(hDlg, PM_NEW_VDM, TRUE);
                            CheckDlgButton(hDlg, PM_NEW_VDM, BST_UNCHECKED);
                        }
                    }
                    return TRUE;
                }

                case PM_COMMAND:
                {
                    if (HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        EnableDlgItem(hDlg, IDOK, ValidateEditContents(hDlg, PM_COMMAND));
                    }
                    return FALSE;
                }

                case IDHELP:
                    MAIN_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
                    return TRUE;

                case IDOK:
                {
                    GetDlgItemTextW(hDlg, PM_COMMAND, szFile, ARRAYSIZE(szFile));
                    ShellExecuteW(NULL, NULL, szFile, NULL, NULL, IsDlgButtonChecked(hDlg, PM_SYMBOL) ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL);
                    if (Globals.bMinOnRun)
                        CloseWindow(Globals.hMainWnd);
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            return FALSE;
    }

    return FALSE;
}

VOID DIALOG_Execute(VOID)
{
    EXECUTE_CONTEXT ExecuteContext;
    LONG lRet;
    DWORD dwSize;
    DWORD dwType;

    ExecuteContext.hKeyPMRecentFilesList = NULL;
    ExecuteContext.bCheckBinaryType = TRUE;

    lRet = RegCreateKeyExW(Globals.hKeyProgMan,
                           L"Recent File List",
                           0,
                           NULL,
                           0,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &ExecuteContext.hKeyPMRecentFilesList,
                           NULL);
    if (lRet == ERROR_SUCCESS)
    {
        dwSize = sizeof(ExecuteContext.dwMaxFiles);
        lRet = RegQueryValueExW(ExecuteContext.hKeyPMRecentFilesList,
                                L"Max Files",
                                NULL,
                                &dwType,
                                (LPBYTE)&ExecuteContext.dwMaxFiles,
                                &dwSize);
        if (lRet != ERROR_SUCCESS || dwType != REG_DWORD)
        {
            ExecuteContext.dwMaxFiles = 4;
            dwSize = sizeof(ExecuteContext.dwMaxFiles);
            lRet = RegSetValueExW(ExecuteContext.hKeyPMRecentFilesList,
                                  L"Max Files",
                                  0,
                                  REG_DWORD,
                                  (LPBYTE)&ExecuteContext.dwMaxFiles,
                                  sizeof(ExecuteContext.dwMaxFiles));
        }

        dwSize = sizeof(ExecuteContext.bCheckBinaryType);
        lRet = RegQueryValueExW(Globals.hKeyPMSettings,
                                L"CheckBinaryType",
                                NULL,
                                &dwType,
                                (LPBYTE)&ExecuteContext.bCheckBinaryType,
                                &dwSize);
        if (lRet != ERROR_SUCCESS || dwType != REG_DWORD)
        {
            ExecuteContext.bCheckBinaryType = TRUE;
        }
    }

    DialogBoxParamW(Globals.hInstance, MAKEINTRESOURCEW(IDD_EXECUTE), Globals.hMainWnd, DIALOG_EXECUTE_DlgProc, (LPARAM)&ExecuteContext);

    if (ExecuteContext.hKeyPMRecentFilesList)
        RegCloseKey(ExecuteContext.hKeyPMRecentFilesList);
}
