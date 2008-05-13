/*
 * PROJECT:         ReactOS System Control Panel
 * FILE:            base/applications/control/control.c
 * PURPOSE:         ReactOS System Control Panel
 * PROGRAMMERS:     Gero Kuehn (reactos.filter@gkware.com)
 *                  Colin Finck (mail@colinfinck.de)
 */

#include "control.h"

static const TCHAR szWindowClass[] = _T("DummyControlClass");

HANDLE hProcessHeap;
HINSTANCE hInst;

static INT
OpenShellFolder(LPTSTR lpFolderCLSID)
{
    TCHAR szParameters[MAX_PATH];

    /* Open a shell folder using "explorer.exe".
       The passed CLSID's are all subfolders of the "Control Panel" shell folder. */
    _tcscpy(szParameters, _T("/n,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"));
    _tcscat(szParameters, lpFolderCLSID);

    return (int)ShellExecute(NULL, _T("open"), _T("explorer.exe"), szParameters, NULL, SW_SHOWDEFAULT) > 32;
}

static INT
RunControlPanel(LPTSTR lpCmd)
{
    TCHAR szParameters[MAX_PATH];

    _tcscpy(szParameters, _T("shell32.dll,Control_RunDLL "));
    _tcscat(szParameters, lpCmd);

    return RUNDLL(szParameters);
}

static VOID
PopulateCPLList(HWND hLisCtrl)
{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    TCHAR pszSearchPath[MAX_PATH];
    HIMAGELIST hImgListSmall;
    HIMAGELIST hImgListLarge;
    HMODULE hDll;
    CPLAPPLETFUNC pFunc;
    TCHAR pszPath[MAX_PATH];
    TCHAR szPanelNum[CCH_UINT_MAX + 1];
    DEVMODE pDevMode;

    /* Icon drawing mode */
    pDevMode.dmSize = sizeof(DEVMODE);
    pDevMode.dmDriverExtra = 0;

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &pDevMode);
    hImgListSmall = ImageList_Create(16, 16, pDevMode.dmBitsPerPel | ILC_MASK, 5, 5);
    hImgListLarge = ImageList_Create(32, 32, pDevMode.dmBitsPerPel | ILC_MASK, 5, 5);

    GetSystemDirectory(pszSearchPath, MAX_PATH);
    _tcscat(pszSearchPath, _T("\\*.cpl"));

    hFind = FindFirstFile(pszSearchPath, &fd);

    while (hFind != INVALID_HANDLE_VALUE)
    {
        _tcscpy(pszPath, pszSearchPath);
        *_tcsrchr(pszPath, '\\') = 0;
        _tcscat(pszPath, _T("\\"));
        _tcscat(pszPath, fd.cFileName);

        hDll = LoadLibrary(pszPath);
        pFunc = (CPLAPPLETFUNC)GetProcAddress(hDll, "CPlApplet");

        if (pFunc && pFunc(hLisCtrl, CPL_INIT, 0, 0))
        {
            UINT i, uPanelCount;

            uPanelCount = (UINT)pFunc(hLisCtrl, CPL_GETCOUNT, 0, 0);

            for (i = 0; i < uPanelCount; i++)
            {
                CPLINFO CplInfo;
                HICON hIcon;
                TCHAR Name[MAX_PATH];
                int index;
                LPTSTR pszCmd;

                pszCmd = (LPTSTR) HeapAlloc(hProcessHeap, 0, MAX_PATH * sizeof(TCHAR));
                if(!pszCmd)
                    return;

                /* Build the command, which is later passed to RunControlPanel */
                _tcscpy(pszCmd, fd.cFileName);
                _tcscat(pszCmd, _T(" @"));
                _itot(i, szPanelNum, 10);
                _tcscat(pszCmd, szPanelNum);

                pFunc(hLisCtrl, CPL_INQUIRE, (LPARAM)i, (LPARAM)&CplInfo);

                hIcon = LoadImage(hDll, MAKEINTRESOURCE(CplInfo.idIcon), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
                index = ImageList_AddIcon(hImgListSmall, hIcon);
                DestroyIcon(hIcon);

                hIcon = LoadImage(hDll, MAKEINTRESOURCE(CplInfo.idIcon), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
                ImageList_AddIcon(hImgListLarge, hIcon);
                DestroyIcon(hIcon);

                if (LoadString(hDll, CplInfo.idName, Name, MAX_PATH))
                {
                    INT nIndex;
                    LV_ITEM lvi = {0};

                    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
                    lvi.pszText = Name;
                    lvi.state = 0;
                    lvi.iImage = index;
                    lvi.lParam = (LPARAM)pszCmd;
                    nIndex = ListView_InsertItem(hLisCtrl, &lvi);

                    if (LoadString(hDll, CplInfo.idInfo, Name, MAX_PATH))
                        ListView_SetItemText(hLisCtrl, nIndex, 1, Name);
                }
            }
        }

        if (!FindNextFile(hFind, &fd))
            hFind = INVALID_HANDLE_VALUE;
    }

    (void)ListView_SetImageList(hLisCtrl, hImgListSmall, LVSIL_SMALL);
    (void)ListView_SetImageList(hLisCtrl, hImgListLarge, LVSIL_NORMAL);
}

LRESULT CALLBACK
MyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hListView;
    TCHAR szBuf[1024];

    switch (uMsg)
    {
        case WM_CREATE:
        {
            RECT rect;
            LV_COLUMN column = {0};

            GetClientRect(hWnd, &rect);
            hListView = CreateWindow(WC_LISTVIEW, NULL, LVS_REPORT | LVS_ALIGNLEFT | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, rect.right, rect.bottom, hWnd, NULL, hInst, 0);

            column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
            column.fmt = LVCFMT_LEFT;
            column.cx = (rect.right - rect.left) / 3;
            column.iSubItem = 0;
            LoadString(hInst, IDS_NAME, szBuf, sizeof(szBuf) / sizeof(TCHAR));
            column.pszText = szBuf;
            (void)ListView_InsertColumn(hListView, 0, &column);

            column.cx = (rect.right - rect.left) - ((rect.right - rect.left) / 3) - 1;
            column.iSubItem = 1;
            LoadString(hInst, IDS_COMMENT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
            column.pszText = szBuf;
            (void)ListView_InsertColumn(hListView, 1, &column);

            PopulateCPLList(hListView);

            (void)ListView_SetColumnWidth(hListView, 2, LVSCW_AUTOSIZE_USEHEADER);
            (void)ListView_Update(hListView, 0);

            SetFocus(hListView);

            return 0;
        }

        case WM_DESTROY:
        {
            LV_ITEM lvi;
            INT nItems;

            lvi.mask = LVIF_PARAM;

            /* Free the memory used for the command strings */
            for(nItems = ListView_GetItemCount(hListView); --nItems >= 0;)
            {
                lvi.iItem = nItems;
                (void)ListView_GetItem(hListView, &lvi);
                HeapFree(hProcessHeap, 0, (LPVOID)lvi.lParam);
            }

            PostQuitMessage(0);
            return 0;
        }

        case WM_SIZE:
        {
            RECT rect;

            GetClientRect(hWnd, &rect);
            MoveWindow(hListView, 0, 0, rect.right, rect.bottom, TRUE);

            return 0;
        }

        case WM_NOTIFY:
        {
            NMHDR *phdr;
            phdr = (NMHDR*)lParam;

            switch(phdr->code)
            {
                case NM_RETURN:
                case NM_DBLCLK:
                {
                    int nSelect;
                    LV_ITEM lvi = {0};
                    LPTSTR pszCmd;

                    nSelect = SendMessage(hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_FOCUSED);

                    if (nSelect == -1)
                    {
                        /* no items */
                        LoadString(hInst, IDS_NO_ITEMS, szBuf, sizeof(szBuf) / sizeof(TCHAR));
                        MessageBox(hWnd, (LPCTSTR)szBuf, NULL, MB_OK | MB_ICONINFORMATION);
                        break;
                    }

                    lvi.iItem = nSelect;
                    lvi.mask = LVIF_PARAM;
                    (void)ListView_GetItem(hListView, &lvi);

                    pszCmd = (LPTSTR)lvi.lParam;

                    if (pszCmd)
                        RunControlPanel(pszCmd);

                    return 0;
                }
            }
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_LARGEICONS:
                    SetWindowLong(hListView,GWL_STYLE,LVS_ICON | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
                    return 0;

                case IDM_SMALLICONS:
                    SetWindowLong(hListView,GWL_STYLE,LVS_SMALLICON | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
                    return 0;

                case IDM_LIST:
                    SetWindowLong(hListView,GWL_STYLE,LVS_LIST | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
                    return 0;

                case IDM_DETAILS:
                    SetWindowLong(hListView,GWL_STYLE,LVS_REPORT | LVS_ALIGNLEFT | LVS_AUTOARRANGE | LVS_SINGLESEL   | WS_VISIBLE | WS_CHILD|WS_BORDER|WS_TABSTOP);
                    return 0;

                case IDM_CLOSE:
                    DestroyWindow(hWnd);
                    return 0;

                case IDM_ABOUT:
                {
                    TCHAR Title[256];

                    LoadString(hInst, IDS_ABOUT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
                    LoadString(hInst, IDS_ABOUT_TITLE, Title, sizeof(Title) / sizeof(TCHAR));

                    MessageBox(hWnd, (LPCTSTR)szBuf, (LPCTSTR)Title, MB_OK | MB_ICONINFORMATION);

                    return 0;
                }
            }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


static INT
RunControlPanelWindow(int nCmdShow)
{
    MSG msg;
    HWND hMainWnd;
    INITCOMMONCONTROLSEX icex;
    WNDCLASSEX wcex = {0};
    TCHAR szBuf[256];

    wcex.cbSize = sizeof(wcex);
    wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAINICON));
    wcex.lpszClassName = MYWNDCLASS;
    wcex.lpfnWndProc = MyWindowProc;
    RegisterClassEx(&wcex);

    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    LoadString(hInst, IDS_WINDOW_TITLE, szBuf, sizeof(szBuf) / sizeof(TCHAR));

    hMainWnd = CreateWindowEx(WS_EX_CLIENTEDGE,
                              MYWNDCLASS,
                              (LPCTSTR)szBuf,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              NULL,
                              LoadMenu(hInst, MAKEINTRESOURCE(IDM_MAINMENU)),
                              hInst,
                              0);
    if (!hMainWnd)
        return 1;

    ShowWindow(hMainWnd, nCmdShow);

    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

int WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    HKEY hKey;

    hInst = hInstance;
    hProcessHeap = GetProcessHeap();

    /* Show the control panel window if no argument or "panel" was passed */
    if(lpCmdLine[0] == 0 || !_tcsicmp(lpCmdLine, _T("panel")))
        return RunControlPanelWindow(nCmdShow);

    /* Check one of the built-in control panel handlers */
    if (!_tcsicmp(lpCmdLine, _T("admintools")))           return OpenShellFolder(_T("\\::{D20EA4E1-3957-11d2-A40B-0C5020524153}"));
    else if (!_tcsicmp(lpCmdLine, _T("color")))           return RunControlPanel(_T("desk.cpl"));       /* TODO: Switch to the "Apperance" tab */
    else if (!_tcsicmp(lpCmdLine, _T("date/time")))       return RunControlPanel(_T("timedate.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("desktop")))         return RunControlPanel(_T("desk.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("folders")))         return RUNDLL(_T("shell32.dll,Options_RunDLL"));
    else if (!_tcsicmp(lpCmdLine, _T("fonts")))           return OpenShellFolder(_T("\\::{D20EA4E1-3957-11d2-A40B-0C5020524152}"));
    else if (!_tcsicmp(lpCmdLine, _T("infrared")))        return RunControlPanel(_T("irprops.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("international")))   return RunControlPanel(_T("intl.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("keyboard")))        return RunControlPanel(_T("main.cpl @1"));
    else if (!_tcsicmp(lpCmdLine, _T("mouse")))           return RunControlPanel(_T("main.cpl @0"));
    else if (!_tcsicmp(lpCmdLine, _T("netconnections")))  return OpenShellFolder(_T("\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}"));
    else if (!_tcsicmp(lpCmdLine, _T("netware")))         return RunControlPanel(_T("nwc.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("ports")))           return RunControlPanel(_T("sysdm.cpl"));      /* TODO: Switch to the "Computer Name" tab */
    else if (!_tcsicmp(lpCmdLine, _T("printers")))        return OpenShellFolder(_T("\\::{2227A280-3AEA-1069-A2DE-08002B30309D}"));
    else if (!_tcsicmp(lpCmdLine, _T("scannercamera")))   return OpenShellFolder(_T("\\::{E211B736-43FD-11D1-9EFB-0000F8757FCD}"));
    else if (!_tcsicmp(lpCmdLine, _T("schedtasks")))      return OpenShellFolder(_T("\\::{D6277990-4C6A-11CF-8D87-00AA0060F5BF}"));
    else if (!_tcsicmp(lpCmdLine, _T("telephony")))       return RunControlPanel(_T("telephon.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("userpasswords")))   return RunControlPanel(_T("nusrmgr.cpl"));       /* Graphical User Account Manager */
    else if (!_tcsicmp(lpCmdLine, _T("userpasswords2")))  return RUNDLL(_T("netplwiz.dll,UsersRunDll"));   /* Dialog based advanced User Account Manager */

    /* It is none of them, so look for a handler in the registry */
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwIndex;

        for(dwIndex = 0; ; ++dwIndex)
        {
            DWORD dwDataSize;
            DWORD dwValueSize = MAX_VALUE_NAME;
            TCHAR szValueName[MAX_VALUE_NAME];

            /* Get the value name and data size */
            if(RegEnumValue(hKey, dwIndex, szValueName, &dwValueSize, 0, NULL, NULL, &dwDataSize) != ERROR_SUCCESS)
                break;

            /* Check if the parameter is the value name */
            if(!_tcsicmp(lpCmdLine, szValueName))
            {
                LPTSTR pszData;

                /* Allocate memory for the data plus two more characters, so we can quote the file name if required */
                pszData = (LPTSTR) HeapAlloc(hProcessHeap, 0, dwDataSize + 2 * sizeof(TCHAR));
                ++pszData;

                /* This value is the one we are looking for, so get the data. It is the path to a .cpl file */
                if(RegQueryValueEx(hKey, szValueName, 0, NULL, (LPBYTE)pszData, &dwDataSize) == ERROR_SUCCESS)
                {
                    INT nReturnValue;

                    /* Quote the file name if required */
                    if(*pszData != '\"')
                    {
                        *(--pszData) = '\"';
                        pszData[dwDataSize / sizeof(TCHAR)] = '\"';
                        pszData[(dwDataSize / sizeof(TCHAR)) + 1] = 0;
                    }

                    nReturnValue = RunControlPanel(pszData);
                    HeapFree(hProcessHeap, 0, pszData);
                    RegCloseKey(hKey);

                    return nReturnValue;
                }

                HeapFree(hProcessHeap, 0, pszData);
            }
        }

        RegCloseKey(hKey);
    }

    /* It's none of the known parameters, so interpret the parameter as the file name of a control panel applet */
    return RunControlPanel(lpCmdLine);
}
