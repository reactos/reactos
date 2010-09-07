/*
 *	Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <janderwald@reactos.org>
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

#include <precomp.h>


WINE_DEFAULT_DEBUG_CHANNEL (fprop);
#define MAX_PROPERTY_SHEET_PAGE (32)

/// Folder Options:
/// CLASSKEY = HKEY_CLASSES_ROOT\CLSID\{6DFD7C5C-2451-11d3-A299-00C04F8EF6AF}
/// DefaultIcon = %SystemRoot%\system32\SHELL32.dll,-210
/// Verbs: Open / RunAs
///       Cmd: rundll32.exe shell32.dll,Options_RunDLL 0

/// ShellFolder Attributes: 0x0

typedef struct
{
   DWORD cFiles;
   DWORD cFolder;
   LARGE_INTEGER bSize;
   HWND hwndDlg;
   WCHAR szFolderPath[MAX_PATH];
}FOLDER_PROPERTIES_CONTEXT, *PFOLDER_PROPERTIES_CONTEXT;

typedef struct
{
    WCHAR FileExtension[30];
    WCHAR FileDescription[100];
    WCHAR ClassKey[MAX_PATH];
}FOLDER_FILE_TYPE_ENTRY, *PFOLDER_FILE_TYPE_ENTRY;

typedef struct
{
   LPCWSTR szKeyName;
   UINT ResourceID;
}FOLDER_VIEW_ENTRY, PFOLDER_VIEW_ENTRY;
/*
static FOLDER_VIEW_ENTRY s_Options[] =
{
    { L"AlwaysShowMenus", IDS_ALWAYSSHOWMENUS },
    { L"AutoCheckSelect", -1 }, 
    { L"ClassicViewState", -1 }, 
    { L"DontPrettyPath",  -1 },
    { L"Filter", -1 },
    { L"FolderContentsInfoTip", IDS_FOLDERCONTENTSTIP },
    { L"FriendlyTree", -1 },
    { L"Hidden", -1, },
    { L"HideFileExt", IDS_HIDEFILEEXT },
    { L"HideIcons", -1},
    { L"IconsOnly", -1},
    { L"ListviewAlphaSelect", -1},
    { L"ListviewShadow", -1},
    { L"ListviewWatermark", -1},
    { L"MapNetDrvBtn", -1},
    { L"PersistBrowsers", -1},
    { L"SeperateProcess", IDS_SEPERATEPROCESS},
    { L"ServerAdminUI", -1},
    { L"SharingWizardOn", IDS_USESHAREWIZARD},
    { L"ShowCompColor", IDS_COMPCOLOR},
    { L"ShowInfoTip", IDS_SHOWINFOTIP},
    { L"ShowPreviewHandlers", -1},
    { L"ShowSuperHidden", IDS_HIDEOSFILES},
    { L"ShowTypeOverlay", -1},
    { L"Start_ShowMyGames", -1},
    { L"StartMenuInit", -1},
    { L"SuperHidden", -1},
    { L"TypeAhead", -1},
    { L"Webview", -1},
    { NULL, -1}

};
*/

HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);

INT_PTR
CALLBACK
FolderOptionsGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{



    return FALSE;
}

static 
VOID
InitializeFolderOptionsListCtrl(HWND hwndDlg)
{
    RECT clientRect;
    LVCOLUMNW col;
    WCHAR szName[50];
    HWND hDlgCtrl;

	hDlgCtrl = GetDlgItem(hwndDlg, 14003);

    if (!LoadStringW(shell32_hInstance, IDS_COLUMN_EXTENSION, szName, sizeof(szName) / sizeof(WCHAR)))
        szName[0] = 0;
    szName[(sizeof(szName)/sizeof(WCHAR))-1] = 0;

    GetClientRect(hDlgCtrl, &clientRect);
    ZeroMemory(&col, sizeof(LV_COLUMN));
    col.mask      = LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
    col.iSubItem  = 0;
    col.pszText = szName;
    col.fmt = LVCFMT_LEFT;
    col.cx        = (clientRect.right - clientRect.left) - GetSystemMetrics(SM_CXVSCROLL);
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMN, 0, (LPARAM)&col);



}


INT_PTR
CALLBACK
FolderOptionsViewDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            InitializeFolderOptionsListCtrl(hwndDlg);
            return TRUE;
    }

    return FALSE;

}

VOID
InitializeFileTypesListCtrlColumns(HWND hDlgCtrl)
{
    RECT clientRect;
    LVCOLUMNW col;
    WCHAR szName[50];
    DWORD dwStyle;
    int columnSize = 140;


    if (!LoadStringW(shell32_hInstance, IDS_COLUMN_EXTENSION, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        /* default to english */
        wcscpy(szName, L"Extensions");
    }

    /* make sure its null terminated */
    szName[(sizeof(szName)/sizeof(WCHAR))-1] = 0;

    GetClientRect(hDlgCtrl, &clientRect);
    ZeroMemory(&col, sizeof(LV_COLUMN));
    columnSize = 140; //FIXME
    col.iSubItem   = 0;
    col.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    col.fmt = LVCFMT_FIXED_WIDTH;
    col.cx         = columnSize | LVCFMT_LEFT;
    col.cchTextMax = wcslen(szName);
    col.pszText    = szName;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    if (!LoadStringW(shell32_hInstance, IDS_FILE_TYPES, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        /* default to english */
        wcscpy(szName, L"FileTypes");
    }

   col.iSubItem   = 1;
   col.cx         = clientRect.right - clientRect.left - columnSize;
   col.cchTextMax = wcslen(szName);
   col.pszText    = szName;
   (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

   /* set full select style */
   dwStyle = (DWORD) SendMessage(hDlgCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
   dwStyle = dwStyle | LVS_EX_FULLROWSELECT;
   SendMessage(hDlgCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

}

INT
FindItem(HWND hDlgCtrl, WCHAR * ItemName)
{
    LVFINDINFOW findInfo;
    ZeroMemory(&findInfo, sizeof(LVFINDINFOW));

    findInfo.flags = LVFI_STRING;
    findInfo.psz = ItemName;
    return ListView_FindItem(hDlgCtrl, 0, &findInfo);
}

VOID
InsertFileType(HWND hDlgCtrl, WCHAR * szName, PINT iItem, WCHAR * szFile)
{
    PFOLDER_FILE_TYPE_ENTRY Entry;
    HKEY hKey;
    LVITEMW lvItem;
    DWORD dwSize;

    if (szName[0] != L'.')
    {
        /* FIXME handle URL protocol handlers */
        return;
    }
 
    /* allocate file type entry */
    Entry = (PFOLDER_FILE_TYPE_ENTRY)HeapAlloc(GetProcessHeap(), 0, sizeof(FOLDER_FILE_TYPE_ENTRY));

    if (!Entry)
        return;

    /* open key */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szName, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    /* FIXME check for duplicates */

    /* query for the default key */
    dwSize = sizeof(Entry->ClassKey);
    if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)Entry->ClassKey, &dwSize) != ERROR_SUCCESS)
    {
         /* no link available */
         Entry->ClassKey[0] = 0;
    }

    if (Entry->ClassKey[0])
    {
        HKEY hTemp;
        /* try open linked key */
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, Entry->ClassKey, 0, KEY_READ, &hTemp) == ERROR_SUCCESS)
        {
            /* use linked key */
            RegCloseKey(hKey);
            hKey = hTemp;
        }
    }

    /* read friendly type name */
    if (RegLoadMUIStringW(hKey, L"FriendlyTypeName", Entry->FileDescription, sizeof(Entry->FileDescription), NULL, 0, NULL) != ERROR_SUCCESS)
    {
        /* read file description */
        dwSize = sizeof(Entry->FileDescription);
        Entry->FileDescription[0] = 0;

        /* read default key */
        RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)Entry->FileDescription, &dwSize);
    }

    /* close key */
    RegCloseKey(hKey);

    /* convert extension to upper case */
    wcscpy(Entry->FileExtension, szName);
    _wcsupr(Entry->FileExtension);

    if (!Entry->FileDescription[0])
    {
        /* construct default 'FileExtensionFile' */
        wcscpy(Entry->FileDescription, &Entry->FileExtension[1]);
        wcscat(Entry->FileDescription, L" ");
        wcscat(Entry->FileDescription, szFile);
    }

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.pszText = &Entry->FileExtension[1];
    lvItem.iItem = *iItem;
    lvItem.lParam = (LPARAM)Entry;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = Entry->FileDescription;
    lvItem.iItem = *iItem;
    lvItem.iSubItem = 1;

    (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&lvItem);
    (*iItem)++;
}

int
CALLBACK
ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    PFOLDER_FILE_TYPE_ENTRY Entry1, Entry2;

    Entry1 = (PFOLDER_FILE_TYPE_ENTRY)lParam1;
    Entry2 = (PFOLDER_FILE_TYPE_ENTRY)lParam2;

    return wcsicmp(Entry1->FileExtension, Entry2->FileExtension);
}

BOOL
InitializeFileTypesListCtrl(HWND hwndDlg)
{
    HWND hDlgCtrl;
    DWORD dwIndex = 0;
    WCHAR szName[50];
    WCHAR szFile[100];
    DWORD dwName;
    LVITEMW lvItem;
    INT iItem = 0;

    hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    InitializeFileTypesListCtrlColumns(hDlgCtrl);

    szFile[0] = 0;
    if (!LoadStringW(shell32_hInstance, IDS_SHV_COLUMN1, szFile, sizeof(szFile) / sizeof(WCHAR)))
    {
        /* default to english */
        wcscpy(szFile, L"File");
    }
    szFile[(sizeof(szFile)/sizeof(WCHAR))-1] = 0;

    dwName = sizeof(szName) / sizeof(WCHAR);

    while(RegEnumKeyExW(HKEY_CLASSES_ROOT, dwIndex++, szName, &dwName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        InsertFileType(hDlgCtrl, szName, &iItem, szFile);
        dwName = sizeof(szName) / sizeof(WCHAR);
    }

    /* sort list */
    ListView_SortItems(hDlgCtrl, ListViewCompareProc, NULL);

    /* select first item */
    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = (UINT)-1;
    lvItem.state = LVIS_FOCUSED|LVIS_SELECTED;
    lvItem.iItem = 0;
    (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&lvItem);

    return TRUE;
}

PFOLDER_FILE_TYPE_ENTRY
FindSelectedItem(
    HWND hDlgCtrl)
{
    UINT Count, Index;
    LVITEMW lvItem;

    Count = ListView_GetItemCount(hDlgCtrl);

    for (Index = 0; Index < Count; Index++)
    {
        ZeroMemory(&lvItem, sizeof(LVITEM));
        lvItem.mask = LVIF_PARAM | LVIF_STATE;
        lvItem.iItem = Index;
        lvItem.stateMask = (UINT)-1;

        if (ListView_GetItem(hDlgCtrl, &lvItem))
        {
            if (lvItem.state & LVIS_SELECTED)
               return (PFOLDER_FILE_TYPE_ENTRY)lvItem.lParam;
        }
    }

    return NULL;
}


INT_PTR
CALLBACK
FolderOptionsFileTypesDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMLISTVIEW lppl;
    LVITEMW lvItem;
    WCHAR Buffer[200], FormatBuffer[100];
    PFOLDER_FILE_TYPE_ENTRY pItem;
    OPENASINFO Info;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            InitializeFileTypesListCtrl(hwndDlg);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case 14006:
                    pItem = FindSelectedItem(GetDlgItem(hwndDlg, 14000));
                    if (pItem)
                    {
                        Info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_REGISTER_EXT;
                        Info.pcszClass = pItem->FileExtension;
                        SHOpenWithDialog(hwndDlg, &Info);
                    }
                    break;
            }

            break;
       case WM_NOTIFY:
            lppl = (LPNMLISTVIEW) lParam;

            if (lppl->hdr.code == LVN_ITEMCHANGING)
            {
                    ZeroMemory(&lvItem, sizeof(LVITEM));
                    lvItem.mask = LVIF_PARAM;
                    lvItem.iItem = lppl->iItem;
                    if (!SendMessageW(lppl->hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&lvItem))
                        return TRUE;

                    pItem = (PFOLDER_FILE_TYPE_ENTRY)lvItem.lParam;
                    if (!pItem)
                        return TRUE;

                    if (!(lppl->uOldState & LVIS_FOCUSED) && (lppl->uNewState & LVIS_FOCUSED))
                    {
                        /* new focused item */
                        if (!LoadStringW(shell32_hInstance, IDS_FILE_DETAILS, FormatBuffer, sizeof(FormatBuffer) / sizeof(WCHAR)))
                        {
                            /* use default english format string */
                            wcscpy(FormatBuffer, L"Details for '%s' extension");
                        }

                        /* format buffer */
                        swprintf(Buffer, FormatBuffer, &pItem->FileExtension[1]);
                        /* update dialog */
                        SendDlgItemMessageW(hwndDlg, 14003, WM_SETTEXT, 0, (LPARAM)Buffer);
                    }
           }
           break;
    }

    return FALSE;
}


VOID
ShowFolderOptionsDialog(HWND hWnd, HINSTANCE hInst)
{
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[3];
    HPROPSHEETPAGE hpage;
    UINT num_pages = 0;
    WCHAR szOptions[100];

    hpage = SH_CreatePropertySheetPage("FOLDER_OPTIONS_GENERAL_DLG", FolderOptionsGeneralDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage("FOLDER_OPTIONS_VIEW_DLG", FolderOptionsViewDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage("FOLDER_OPTIONS_FILETYPES_DLG", FolderOptionsFileTypesDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    szOptions[0] = L'\0';
    LoadStringW(shell32_hInstance, IDS_FOLDER_OPTIONS, szOptions, sizeof(szOptions) / sizeof(WCHAR));
    szOptions[(sizeof(szOptions)/sizeof(WCHAR))-1] = L'\0';

    memset(&pinfo, 0x0, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP;
    pinfo.nPages = num_pages;
    pinfo.u3.phpage = hppages;
    pinfo.pszCaption = szOptions;

    PropertySheetW(&pinfo);
}

VOID
Options_RunDLLCommon(HWND hWnd, HINSTANCE hInst, int fOptions, DWORD nCmdShow)
{
    switch(fOptions)
    {
    case 0:
        ShowFolderOptionsDialog(hWnd, hInst);
        break;
    case 1:
        // show taskbar options dialog
        FIXME("notify explorer to show taskbar options dialog");
        //PostMessage(GetShellWindow(), WM_USER+22, fOptions, 0);
        break;
    default:
        FIXME("unrecognized options id %d\n", fOptions);
    }
}

/*************************************************************************
 *              Options_RunDLL (SHELL32.@)
 */
VOID WINAPI Options_RunDLL(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}
/*************************************************************************
 *              Options_RunDLLA (SHELL32.@)
 */
VOID WINAPI Options_RunDLLA(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}

/*************************************************************************
 *              Options_RunDLLW (SHELL32.@)
 */
VOID WINAPI Options_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntW(cmd), nCmdShow);
}

static
DWORD WINAPI
CountFolderAndFiles(LPVOID lParam)
{
    WIN32_FIND_DATAW FindData;
    HANDLE hFile;
    UINT Length;
    LPWSTR pOffset;
    BOOL ret;
    PFOLDER_PROPERTIES_CONTEXT pContext = (PFOLDER_PROPERTIES_CONTEXT) lParam;

    pOffset = PathAddBackslashW(pContext->szFolderPath);
    if (!pOffset)
       return 0;

    Length = pOffset - pContext->szFolderPath;

    wcscpy(pOffset, L"*.*");
    hFile = FindFirstFileW(pContext->szFolderPath, &FindData);
    if (hFile == INVALID_HANDLE_VALUE)
       return 0;

    do
    {
        ret = FindNextFileW(hFile, &FindData);
        if (ret)
        {
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (FindData.cFileName[0] == L'.' && FindData.cFileName[1] == L'.' &&
                    FindData.cFileName[2] == L'\0')
                    continue;

                pContext->cFolder++;
                wcscpy(pOffset, FindData.cFileName);
                CountFolderAndFiles((LPVOID)pContext);
                pOffset[0] = L'\0';
            }
            else
            {
                pContext->cFiles++;
                pContext->bSize.u.LowPart += FindData.nFileSizeLow;
                pContext->bSize.u.HighPart += FindData.nFileSizeHigh;
            }	
        }
        else if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            break;
        }
    }while(1);

    FindClose(hFile);
    return 1;
}

static
VOID
InitializeFolderGeneralDlg(PFOLDER_PROPERTIES_CONTEXT pContext)
{
    LPWSTR pFolderName;
    WIN32_FILE_ATTRIBUTE_DATA FolderAttribute;
    FILETIME ft;
    SYSTEMTIME dt;
    WCHAR szBuffer[MAX_PATH+5];
    WCHAR szFormat[30] = {0};

    static const WCHAR wFormat[] = {'%','0','2','d','/','%','0','2','d','/','%','0','4','d',' ',' ','%','0','2','d',':','%','0','2','u',0};

    pFolderName = wcsrchr(pContext->szFolderPath, L'\\');
    if (!pFolderName)
        return;

    /* set folder name */
    SendDlgItemMessageW(pContext->hwndDlg, 14001, WM_SETTEXT, 0, (LPARAM) (pFolderName + 1));
    /* set folder location */
    pFolderName[0] = L'\0';
    if (wcslen(pContext->szFolderPath) == 2)
    {
        /* folder is located at root */
        WCHAR szDrive[4] = {L'C',L':',L'\\',L'\0'};
        szDrive[0] = pContext->szFolderPath[0];
        SendDlgItemMessageW(pContext->hwndDlg, 14007, WM_SETTEXT, 0, (LPARAM) szDrive);
    }
    else
    {
        SendDlgItemMessageW(pContext->hwndDlg, 14007, WM_SETTEXT, 0, (LPARAM) pContext->szFolderPath);
    }
    pFolderName[0] = L'\\';
    /* get folder properties */
    if (GetFileAttributesExW(pContext->szFolderPath, GetFileExInfoStandard, (LPVOID)&FolderAttribute))
    {
        if (FolderAttribute.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            /* check readonly button */
            SendDlgItemMessage(pContext->hwndDlg, 14021, BM_SETCHECK, BST_CHECKED, 0);
        }

        if (FolderAttribute.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        {
            /* check hidden button */
            SendDlgItemMessage(pContext->hwndDlg, 14022, BM_SETCHECK, BST_CHECKED, 0);
        }

       if (FileTimeToLocalFileTime(&FolderAttribute.ftCreationTime, &ft))
       {
           FileTimeToSystemTime(&ft, &dt);
           swprintf (szBuffer, wFormat, dt.wDay, dt.wMonth, dt.wYear, dt.wHour, dt.wMinute);
           SendDlgItemMessageW(pContext->hwndDlg, 14015, WM_SETTEXT, 0, (LPARAM) szBuffer);
       }
    }
    /* now enumerate enumerate contents */
    wcscpy(szBuffer, pContext->szFolderPath);
    CountFolderAndFiles((LPVOID)pContext);
    wcscpy(pContext->szFolderPath, szBuffer);
    /* set folder details */
    LoadStringW(shell32_hInstance, IDS_FILE_FOLDER, szFormat, sizeof(szFormat)/sizeof(WCHAR));
    szFormat[(sizeof(szFormat)/sizeof(WCHAR))-1] = L'\0';
    swprintf(szBuffer, szFormat, pContext->cFiles, pContext->cFolder);
    SendDlgItemMessageW(pContext->hwndDlg, 14011, WM_SETTEXT, 0, (LPARAM) szBuffer);

    if (StrFormatByteSizeW(pContext->bSize.QuadPart, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        /* store folder size */
        SendDlgItemMessageW(pContext->hwndDlg, 14009, WM_SETTEXT, 0, (LPARAM) szBuffer);
    }
}


INT_PTR
CALLBACK
FolderPropertiesGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPPROPSHEETPAGEW ppsp;
    PFOLDER_PROPERTIES_CONTEXT pContext;
    HICON hIcon;
    WIN32_FILE_ATTRIBUTE_DATA FolderAttribute;
    LONG res;
    LPPSHNOTIFY lppsn;
    DWORD Attribute;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            ppsp = (LPPROPSHEETPAGEW)lParam;
            if (ppsp == NULL)
                break;
            hIcon = LoadIconW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_FOLDER_OPEN));
            if (hIcon)
               SendDlgItemMessageW(hwndDlg, 14000, STM_SETICON,  (WPARAM)hIcon, 0);

            pContext = SHAlloc(sizeof(FOLDER_PROPERTIES_CONTEXT));
            if (pContext)
            {
                ZeroMemory(pContext, sizeof(FOLDER_PROPERTIES_CONTEXT));
                pContext->hwndDlg = hwndDlg;
                wcscpy(pContext->szFolderPath, (LPWSTR)ppsp->lParam);
                SetWindowLongPtr(hwndDlg, DWL_USER, (LONG_PTR)pContext);
                InitializeFolderGeneralDlg(pContext);
            }
            return TRUE;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
               PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;
        case WM_DESTROY:
            pContext = (PFOLDER_PROPERTIES_CONTEXT)GetWindowLongPtr(hwndDlg, DWL_USER);
            SHFree((LPVOID)pContext);
            break;
        case WM_NOTIFY:
            pContext = (PFOLDER_PROPERTIES_CONTEXT)GetWindowLongPtr(hwndDlg, DWL_USER);
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                if (GetFileAttributesExW(pContext->szFolderPath, GetFileExInfoStandard, (LPVOID)&FolderAttribute))
                {
                    res = SendDlgItemMessageW(hwndDlg, 14021, BM_GETCHECK, 0, 0);
                    if (res == BST_CHECKED)
                        FolderAttribute.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
                    else
                        FolderAttribute.dwFileAttributes &= (~FILE_ATTRIBUTE_READONLY);

                    res = SendDlgItemMessageW(hwndDlg, 14022, BM_GETCHECK, 0, 0);
                    if (res == BST_CHECKED)
                        FolderAttribute.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
                    else
                        FolderAttribute.dwFileAttributes &= (~FILE_ATTRIBUTE_HIDDEN);

                    Attribute = FolderAttribute.dwFileAttributes & 
(FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_TEMPORARY);

                    SetFileAttributesW(pContext->szFolderPath, Attribute);
                }
                SetWindowLongPtr( hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR );
                return TRUE;
            }
            break;
    }
    return FALSE;
}

static
BOOL
CALLBACK
FolderAddPropSheetPageProc(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADERW *ppsh = (PROPSHEETHEADERW *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        ppsh->u3.phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

BOOL
SH_ShowFolderProperties(LPWSTR pwszFolder,  LPCITEMIDLIST pidlFolder, LPCITEMIDLIST * apidl)
{
    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    HPROPSHEETPAGE hpage;
    PROPSHEETHEADERW psh;
    BOOL ret;
    WCHAR szName[MAX_PATH] = {0};
    HPSXA hpsx = NULL;
    LPWSTR pFolderName;
    IDataObject * pDataObj = NULL;

    if (!PathIsDirectoryW(pwszFolder))
        return FALSE;

    pFolderName = wcsrchr(pwszFolder, L'\\');
    if (!pFolderName)
        return FALSE;

    wcscpy(szName, pFolderName + 1);

    hpage = SH_CreatePropertySheetPage("SHELL_FOLDER_GENERAL_DLG", FolderPropertiesGeneralDlg, (LPARAM)pwszFolder, NULL);
    if (!hpage)
        return FALSE;

    ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
    hppages[psh.nPages] = hpage;
    psh.nPages++;
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = NULL;
    psh.u3.phpage = hppages;
    psh.pszCaption = szName;


    if (SHCreateDataObject(pidlFolder, 1, apidl, NULL, &IID_IDataObject, (void**)&pDataObj) == S_OK)
    {
        hpsx = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"Directory", MAX_PROPERTY_SHEET_PAGE-1, pDataObj);
        if (hpsx)
        {
            SHAddFromPropSheetExtArray(hpsx,
                                      (LPFNADDPROPSHEETPAGE)FolderAddPropSheetPageProc,
                                      (LPARAM)&psh);
        }
    }

    ret = PropertySheetW(&psh);
   if (pDataObj)
       IDataObject_Release(pDataObj);

   if (hpsx)
       SHDestroyPropSheetExtArray(hpsx);

    if (ret < 0)
        return FALSE;
    else
        return TRUE;
}


