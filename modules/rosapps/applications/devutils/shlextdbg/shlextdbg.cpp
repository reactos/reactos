/*
 * PROJECT:     shlextdbg
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shell extension debug utility
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <atlbase.h>   // thanks gcc
#include <atlcom.h>    // thanks gcc
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <conio.h>
#include <shellutils.h>
#include <shlwapi_undoc.h>

static void PrintHelp(PCWSTR ExtraLine = NULL)
{
    if (ExtraLine)
        wprintf(L"%s\n\n", ExtraLine);

    wprintf(L"shlextdbg /clsid={clsid} [/dll=dllname] /IShellExtInit=filename |shlextype| |waitoptions|\n");
    wprintf(L"    {clsid}: The CLSID or ProgID of the object to create\n");
    wprintf(L"    dll: Optional dllname to create the object from, instead of CoCreateInstance\n");
    wprintf(L"    filename: The filename to pass to IShellExtInit->Initialze\n");
    wprintf(L"    shlextype: The type of shell extention to run:\n");
    wprintf(L"               /IShellPropSheetExt to create a property sheet\n");
    wprintf(L"               /IContextMenu=verb to activate the specified verb\n");
    wprintf(L"    waitoptions: Specify how to wait:\n");
    wprintf(L"                 /explorerinstance: Wait for SHGetInstanceExplorer (Default)\n");
    wprintf(L"                 /infinite: Keep on waiting infinitely\n");
    wprintf(L"                 /openwindows: Wait for all windows from the current application to close\n");
    wprintf(L"                 /input: Wait for input\n");
    wprintf(L"                 /nowait\n");
    wprintf(L"\n");
    wprintf(L"shlextdbg /shgfi=path\n");
    wprintf(L"    Call SHGetFileInfo. Prefix path with $ to parse as a pidl.\n");
    wprintf(L"\n");
    wprintf(L"shlextdbg /assocq <[{bhid}]path> <string|data|key> <type> <initflags> <queryflags> <initstring> [extra] [maxsize]\n");
    wprintf(L"    Uses the default implementation from AssocCreate if path is empty.\n");
    wprintf(L"\n");
    wprintf(L"shlextdbg /shellexec=path [/see] [verb] [class]\n");
    wprintf(L"\n");
    wprintf(L"shlextdbg /dumpmenu=[{clsid}]path [/cmf]\n");
}

/*
Examples:

/clsid={513D916F-2A8E-4F51-AEAB-0CBC76FB1AF8} /IShellExtInit=C:\RosBE\Uninstall.exe /IShellPropSheetExt
/clsid=CompressedFolder /IShellExtInit=e:\test.zip /IContextMenu=extract /openwindows
/clsid=CompressedFolder /IShellExtInit=e:\test.zip /IContextMenu=extract /openwindows /dll=R:\build\dev\devenv\dll\shellext\zipfldr\Debug\zipfldr.dll
/shgfi=c:\freeldr.ini
/assocq "" string 1 0 0 .txt
/assocq "" string friendlytypename 0x400 0 .txt "" 10
/openwindows /shellexec=c: /invoke properties
/dumpmenu=%windir%\explorer.exe /extended
/dumpmenu {D969A300-E7FF-11d0-A93B-00A0C90F2719}c:

*/

static LONG StrToNum(PCWSTR in)
{
    PWCHAR end;
    LONG v = wcstol(in, &end, 0);
    return (end > in) ? v : 0;
}

static int ErrMsg(int Error)
{
    WCHAR buf[400];
    for (UINT e = Error, cch; ;)
    {
        lstrcpynW(buf, L"?", _countof(buf));
        cch = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, e, 0, buf, _countof(buf), NULL);
        while (cch && buf[cch - 1] <= ' ')
            buf[--cch] = UNICODE_NULL; // Remove trailing newlines
        if (cch || HIWORD(e) != HIWORD(HRESULT_FROM_WIN32(1)))
            break;
        e = HRESULT_CODE(e); // "WIN32_FROM_HRESULT"
    }
    wprintf(Error < 0 ? L"Error 0x%.8X %s\n" : L"Error %d %s\n", Error, buf);
    return Error;
}

template<class T>
static bool CLSIDPrefix(T& String, CLSID& Clsid)
{
    WCHAR buf[38 + 1];
    if (String[0] == '{')
    {
        lstrcpynW(buf, String, _countof(buf));
        if (SUCCEEDED(CLSIDFromString(buf, &Clsid)))
        {
            String = String + 38;
            return true;
        }
    }
    return false;
}

static HRESULT GetUIObjectOfAbsolute(LPCITEMIDLIST pidl, REFIID riid, void** ppv)
{
    CComPtr<IShellFolder> shellFolder;
    PCUITEMID_CHILD child;
    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &shellFolder), &child);
    if (SUCCEEDED(hr))
        hr = shellFolder->GetUIObjectOf(NULL, 1, &child, riid, NULL, ppv);
    return hr;
}

static HRESULT CreateShellItemFromParse(PCWSTR Path, IShellItem** ppSI)
{
    PIDLIST_ABSOLUTE pidl = NULL;
    HRESULT hr = SHParseDisplayName(Path, NULL, &pidl, 0, NULL);
    if (SUCCEEDED(hr))
    {
        hr = SHCreateShellItem(NULL, NULL, pidl, ppSI);
        SHFree(pidl);
    }
    return hr;
}

static void GetAssocClass(LPCWSTR Path, LPCITEMIDLIST pidl, HKEY& hKey)
{
    hKey = NULL;
    IQueryAssociations* pQA;
    if (SUCCEEDED(GetUIObjectOfAbsolute(pidl, IID_PPV_ARG(IQueryAssociations, &pQA))))
    {
        pQA->GetKey(0, ASSOCKEY_CLASS, NULL, &hKey); // Not implemented in ROS
        pQA->Release();
    }
    if (!hKey)
    {
        DWORD cb;
        WCHAR buf[MAX_PATH];
        PWSTR ext = PathFindExtensionW(Path);
        SHFILEINFOW info;
        info.dwAttributes = 0;
        SHGetFileInfoW((LPWSTR)pidl, 0, &info, sizeof(info), SHGFI_PIDL | SHGFI_ATTRIBUTES);
        if (info.dwAttributes & SFGAO_FOLDER)
        {
            ext = const_cast<LPWSTR>(L"Directory");
        }
        else if (info.dwAttributes & SFGAO_BROWSABLE)
        {
            ext = const_cast<LPWSTR>(L"Folder"); // Best guess
        }
        else
        {
            cb = sizeof(buf);
            if (!SHGetValueW(HKEY_CLASSES_ROOT, ext, NULL, NULL, buf, &cb))
            {
                RegOpenKeyExW(HKEY_CLASSES_ROOT, buf, 0, KEY_READ, &hKey);
            }
        }
        if (!hKey)
        {
            RegOpenKeyExW(HKEY_CLASSES_ROOT, ext, 0, KEY_READ, &hKey);
        }
    }
}

static void DumpBytes(const void *Data, SIZE_T cb)
{
    for (SIZE_T i = 0; i < cb; ++i)
    {
        wprintf(L"%s%.2X", i ? L" " : L"", ((LPCBYTE)Data)[i]);
    }
    wprintf(L"\n");
}

static HRESULT GetCommandString(IContextMenu& CM, UINT Id, UINT Type, LPWSTR buf, UINT cchMax)
{
    if (cchMax < 1) return E_INVALIDARG;
    *buf = UNICODE_NULL;

    // First try to retrieve the UNICODE string directly
    HRESULT hr = CM.GetCommandString(Id, Type | GCS_UNICODE, 0, (char*)buf, cchMax);
    if (FAILED(hr))
    {
        // It failed, try to retrieve an ANSI string instead then convert it to UNICODE
        STRRET sr;
        sr.uType = STRRET_CSTR;
        hr = CM.GetCommandString(Id, Type & ~GCS_UNICODE, 0, sr.cStr, _countof(sr.cStr));
        if (SUCCEEDED(hr))
            hr = StrRetToBufW(&sr, NULL, buf, cchMax);
    }
    return hr;
}

static void DumpMenu(HMENU hMenu, UINT IdOffset, IContextMenu* pCM, BOOL FakeInit, UINT Indent)
{
    bool recurse = Indent != UINT(-1);
    WCHAR buf[MAX_PATH];
    MENUITEMINFOW mii;
    mii.cbSize = FIELD_OFFSET(MENUITEMINFOW, hbmpItem);

    for (UINT i = 0, defid = GetMenuDefaultItem(hMenu, FALSE, 0); ; ++i)
    {
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = buf;
        mii.cch = _countof(buf);
        *buf = UNICODE_NULL;
        if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
            lstrcpynW(buf, L"?", _countof(buf)); // Tolerate string failure
        mii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_FTYPE;
        mii.hSubMenu = NULL;
        mii.dwTypeData = NULL;
        mii.cch = 0;
        if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
            break;

        BOOL sep = mii.fType & MFT_SEPARATOR;
        wprintf(L"%-4d", (sep || mii.wID == UINT(-1)) ? mii.wID : (mii.wID - IdOffset));
        for (UINT j = 0; j < Indent && recurse; ++j)
            wprintf(L" ");
        wprintf(L"%s%s", mii.hSubMenu ? L">" : L"|", sep ? L"----------" : buf);
        if (!sep && pCM && SUCCEEDED(GetCommandString(*pCM, mii.wID - IdOffset,
                                                      GCS_VERB, buf, _countof(buf))))
        {
            wprintf(L" [%s]", buf);
        }
        wprintf(L"%s\n", (defid == mii.wID && defid != UINT(-1)) ? L" (Default)" : L"");
        if (mii.hSubMenu && recurse)
        {
            if (FakeInit)
                SHForwardContextMenuMsg(pCM, WM_INITMENUPOPUP, (WPARAM)mii.hSubMenu, LOWORD(i), NULL, TRUE);
            DumpMenu(mii.hSubMenu, IdOffset, pCM, FakeInit, Indent + 1);
        }
    }
}

static int SHGFI(PCWSTR Path)
{
    PIDLIST_ABSOLUTE pidl = NULL;
    UINT flags = 0, ret = 0;

    if (*Path == L'$')
    {
        HRESULT hr = SHParseDisplayName(++Path, NULL, &pidl, 0, NULL);
        if (FAILED(hr))
            return ErrMsg(hr);
        flags |= SHGFI_PIDL;
        Path = (LPCWSTR)pidl;
    }
    else if (GetFileAttributes(Path) == INVALID_FILE_ATTRIBUTES)
    {
        flags |= SHGFI_USEFILEATTRIBUTES;
    }
    SHFILEINFOW info;
    if (!SHGetFileInfoW(Path, 0, &info, sizeof(info), flags |
                        SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES | SHGFI_TYPENAME))
    {
        info.szDisplayName[0] = info.szTypeName[0] = UNICODE_NULL;
        info.dwAttributes = 0;
        ret = ERROR_FILE_NOT_FOUND;
    }
    wprintf(L"Display: %s\n", info.szDisplayName);
    wprintf(L"Attributes: 0x%x\n", info.dwAttributes);
    wprintf(L"Type: %s\n", info.szTypeName);

    if (!SHGetFileInfoW(Path, 0, &info, sizeof(info), flags | SHGFI_ICONLOCATION))
    {
        info.szDisplayName[0] = UNICODE_NULL;
        info.iIcon = -1;
    }
    wprintf(L"Icon: %s,%d\n", info.szDisplayName, info.iIcon);

    if (!SHGetFileInfoW(Path, 0, &info, sizeof(info), flags | SHGFI_SYSICONINDEX))
    {
        info.iIcon = -1;
    }
    wprintf(L"Index: %d\n", info.iIcon);
    SHFree(pidl);
    return ret;
}

static HRESULT AssocQ(int argc, WCHAR **argv)
{
    UINT qtype = StrToNum(argv[2]);
    ASSOCF iflags = StrToNum(argv[3]);
    ASSOCF qflags = StrToNum(argv[4]);
    PCWSTR extra = (argc > 6 && *argv[6]) ? argv[6] : NULL;
    WCHAR buf[MAX_PATH * 2];
    DWORD maxSize = (argc > 7 && *argv[7]) ? StrToNum(argv[7]) : sizeof(buf);

    HRESULT hr;
    CComPtr<IQueryAssociations> qa;
    PWSTR path = argv[0];
    if (*path)
    {
        CLSID clsid, *pclsid = NULL;
        if (CLSIDPrefix(path, clsid))
            pclsid = &clsid;
        CComPtr<IShellItem> si;
        if (SUCCEEDED(hr = CreateShellItemFromParse(path, &si)))
        {
            hr = si->BindToHandler(NULL, pclsid ? *pclsid : BHID_AssociationArray, IID_PPV_ARG(IQueryAssociations, &qa));
            if (FAILED(hr) && !pclsid)
                hr = si->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IQueryAssociations, &qa));
        }
    }
    else
    {
        hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &qa));
    }
    if (FAILED(hr))
        return ErrMsg(hr);
    hr = qa->Init(iflags, argv[5], NULL, NULL);
    if (FAILED(hr))
        return ErrMsg(hr);

    DWORD size = maxSize;
    if (!_wcsicmp(argv[1], L"string"))
    {
        if (!_wcsicmp(argv[2], L"COMMAND"))
            qtype = ASSOCSTR_COMMAND;
        if (!_wcsicmp(argv[2], L"EXECUTABLE"))
            qtype = ASSOCSTR_EXECUTABLE;
        if (!_wcsicmp(argv[2], L"FRIENDLYDOCNAME") || !_wcsicmp(argv[2], L"FriendlyTypeName"))
            qtype = ASSOCSTR_FRIENDLYDOCNAME;
        if (!_wcsicmp(argv[2], L"DEFAULTICON"))
            qtype = ASSOCSTR_DEFAULTICON;

        buf[0] = UNICODE_NULL;
        size /= sizeof(buf[0]); // Convert to number of characters
        hr = qa->GetString(qflags, (ASSOCSTR)qtype, extra, buf, &size);
        size *= sizeof(buf[0]); // Convert back to bytes
        if (SUCCEEDED(hr) ||
            hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            wprintf(L"0x%.8X: %s\n", hr, buf);
        }
        else
        {
            wprintf(size != maxSize ? L"%u " : L"", size);
            ErrMsg(hr);
        }
    }
    else if (!_wcsicmp(argv[1], L"data"))
    {
        if (!_wcsicmp(argv[2], L"EDITFLAGS"))
            qtype = ASSOCDATA_EDITFLAGS;
        if (!_wcsicmp(argv[2], L"VALUE"))
            qtype = ASSOCDATA_VALUE;

        hr = qa->GetData(qflags, (ASSOCDATA)qtype, extra, buf, &size);
        if (SUCCEEDED(hr))
        {
            wprintf(L"0x%.8X: %u byte(s) ", hr, size);
            DumpBytes(buf, min(size, maxSize));
        }
        else
        {
            wprintf(size != maxSize ? L"%u " : L"", size);
            ErrMsg(hr);
        }
    }
    else if (!_wcsicmp(argv[1], L"key"))
    {
        HKEY hKey = NULL;
        hr = qa->GetKey(qflags, (ASSOCKEY)qtype, extra, &hKey);
        if (SUCCEEDED(hr))
        {
            wprintf(L"0x%.8X: hKey %p\n", hr, hKey);
            RegQueryValueExW(hKey, L"shlextdbg", 0, NULL, NULL, NULL); // Filter by this in Process Monitor
            RegCloseKey(hKey);
        }
        else
        {
            ErrMsg(hr);
        }
    }
    else
    {
        PrintHelp(L"Unknown query");
        return ErrMsg(ERROR_INVALID_PARAMETER);
    }
    return hr;
}

enum WaitType
{
    Wait_None,
    Wait_Infinite,
    Wait_OpenWindows,
    Wait_Input,
    Wait_ExplorerInstance,
};

CLSID g_CLSID = { 0 };
CStringW g_DLL;
CStringW g_ShellExtInit;
bool g_bIShellPropSheetExt = false;
CStringA g_ContextMenu;
WaitType g_Wait = Wait_ExplorerInstance;

HRESULT CreateIDataObject(CComHeapPtr<ITEMIDLIST>& pidl, CComPtr<IDataObject>& dataObject, PCWSTR FileName)
{
    HRESULT hr = SHParseDisplayName(FileName, NULL, &pidl, 0, NULL);
    if (!SUCCEEDED(hr))
    {
        wprintf(L"Failed to create pidl from '%s': 0x%x\n", FileName, hr);
        return hr;
    }

    CComPtr<IShellFolder> shellFolder;
    PCUITEMID_CHILD childs;
    hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &shellFolder), &childs);
    if (!SUCCEEDED(hr))
    {
        wprintf(L"Failed to bind to parent: 0x%x\n", hr);
        return hr;
    }
    hr = shellFolder->GetUIObjectOf(NULL, 1, &childs, IID_IDataObject, NULL, (PVOID*)&dataObject);
    if (!SUCCEEDED(hr))
    {
        wprintf(L"Failed to query IDataObject: 0x%x\n", hr);
    }
    return hr;
}

HRESULT LoadAndInitialize(REFIID riid, LPVOID* ppv)
{
    CComPtr<IShellExtInit> spShellExtInit;
    HRESULT hr;
    if (g_DLL.IsEmpty())
    {
        hr = CoCreateInstance(g_CLSID, NULL, CLSCTX_ALL, IID_PPV_ARG(IShellExtInit, &spShellExtInit));
        if (!SUCCEEDED(hr))
        {
            WCHAR Buffer[100];
            StringFromGUID2(g_CLSID, Buffer, _countof(Buffer));
            wprintf(L"Failed to Create %s:IShellExtInit: 0x%x\n", Buffer, hr);
            return hr;
        }
    }
    else
    {
        typedef HRESULT (STDAPICALLTYPE *tDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
        HMODULE mod = LoadLibraryW(g_DLL);
        if (!mod)
        {
            wprintf(L"Failed to Load %s:(0x%x)\n", g_DLL.GetString(), GetLastError());
            return E_FAIL;
        }
        tDllGetClassObject DllGet = (tDllGetClassObject)GetProcAddress(mod, "DllGetClassObject");
        if (!DllGet)
        {
            wprintf(L"%s does not export DllGetClassObject\n", g_DLL.GetString());
            return E_FAIL;
        }
        CComPtr<IClassFactory> spClassFactory;
        hr = DllGet(g_CLSID, IID_PPV_ARG(IClassFactory, &spClassFactory));
        if (!SUCCEEDED(hr))
        {
            wprintf(L"Failed to create IClassFactory: 0x%x\n", hr);
            return hr;
        }
        hr = spClassFactory->CreateInstance(NULL, IID_PPV_ARG(IShellExtInit, &spShellExtInit));
        if (!SUCCEEDED(hr))
        {
            wprintf(L"Failed to Request IShellExtInit from IClassFactory: 0x%x\n", hr);
            return hr;
        }
    }

    CComPtr<IDataObject> spDataObject;
    CComHeapPtr<ITEMIDLIST> pidl;
    hr = CreateIDataObject(pidl, spDataObject, g_ShellExtInit.GetString());
    if (!SUCCEEDED(hr))
        return hr;

    HKEY hKey = NULL;
    GetAssocClass(g_ShellExtInit.GetString(), pidl, hKey);
    hr = spShellExtInit->Initialize(pidl, spDataObject, hKey);
    if (hKey)
        RegCloseKey(hKey);
    if (!SUCCEEDED(hr))
    {
        wprintf(L"IShellExtInit->Initialize failed: 0x%x\n", hr);
        return hr;
    }
    hr = spShellExtInit->QueryInterface(riid, ppv);
    if (!SUCCEEDED(hr))
    {
        WCHAR Buffer[100];
        StringFromGUID2(riid, Buffer, _countof(Buffer));
        wprintf(L"Failed to query %s from IShellExtInit: 0x%x\n", Buffer, hr);
    }
    return hr;
}


CSimpleArray<HWND> g_Windows;
HWND g_ConsoleWindow;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (hwnd != g_ConsoleWindow)
    {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == GetCurrentProcessId())
        {
            g_Windows.Add(hwnd);
        }
    }
    return TRUE;
}

void WaitWindows()
{
    /* Give the windows some time to spawn */
    Sleep(2000);
    g_ConsoleWindow = GetConsoleWindow();
    while (true)
    {
        g_Windows.RemoveAll();
        EnumWindows(EnumWindowsProc, NULL);
        if (g_Windows.GetSize() == 0)
            break;
        Sleep(500);
    }
    wprintf(L"All windows closed (ignoring console window)\n");
}

struct ExplorerInstance : public IUnknown
{
    HWND m_hWnd;
    volatile LONG m_rc;

    ExplorerInstance() : m_hWnd(NULL), m_rc(1) {}
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv)
    {
        static const QITAB rgqit[] = { { 0 } };
        return QISearch(this, rgqit, riid, ppv);
    }
    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        if (g_Wait == Wait_ExplorerInstance)
            wprintf(L"INFO: SHGetInstanceExplorer\n");
        return InterlockedIncrement(&m_rc);
    }
    virtual ULONG STDMETHODCALLTYPE Release()
    {
        if (g_Wait == Wait_ExplorerInstance)
            wprintf(L"INFO: Release ExplorerInstance\n");
        ULONG r = InterlockedDecrement(&m_rc);
        if (!r)
            PostMessage(m_hWnd, WM_CLOSE, 0, 0);
        return r;
    }
    void Wait()
    {
        SHSetInstanceExplorer(NULL);
        m_hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, TEXT("STATIC"), NULL, WS_POPUP,
                                0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
        BOOL loop = InterlockedDecrement(&m_rc) != 0;
        MSG msg;
        while (loop && (int)GetMessage(&msg, NULL, 0, 0) > 0)
        {
            if (msg.hwnd == m_hWnd && msg.message == WM_CLOSE)
                PostMessage(m_hWnd, WM_QUIT, 0, 0);
            DispatchMessage(&msg);
        }
    }
} g_EI;

static void Wait()
{
    LPCWSTR nag = L"(Please use SHGetInstanceExplorer in your code instead)";
    switch (g_Wait)
    {
    case Wait_None:
        break;
    case Wait_Infinite:
        _putws(nag);
        while (true)
            Sleep(1000);
        break;
    case Wait_OpenWindows:
        _putws(nag);
        WaitWindows();
        break;
    case Wait_Input:
        wprintf(L"Press any key to continue... %s\n", nag);
        _getch();
        break;
    case Wait_ExplorerInstance:
        g_EI.Wait();
        break;
    }
}

CSimpleArray<HPROPSHEETPAGE> g_Pages;
static BOOL CALLBACK cb_AddPage(HPROPSHEETPAGE page, LPARAM lParam)
{
    g_Pages.Add(page);
    if (lParam != (LPARAM)&g_Pages)
    {
        wprintf(L"Propsheet failed to pass lParam, got: 0x%Ix\n", lParam);
    }
    return TRUE;
}

static bool isCmdWithArg(int argc, WCHAR** argv, int& n, PCWSTR check, PCWSTR &arg)
{
    arg = NULL;
    size_t len = wcslen(check);
    if (!_wcsnicmp(argv[n] + 1, check, len))
    {
        PCWSTR cmd = argv[n] + len + 1;
        if (*cmd == ':' || *cmd == '=')
        {
            arg = cmd + 1;
            return true;
        }
        if (n + 1 < argc)
        {
            arg = argv[n+1];
            n++;
            return true;
        }
        wprintf(L"Command %s has no required argument!\n", check);
        return false;
    }
    return false;
}

static bool isCmd(int argc, WCHAR** argv, int n, PCWSTR check)
{
    return !_wcsicmp(argv[n] + 1, check);
}

extern "C"  // and another hack for gcc
int wmain(int argc, WCHAR **argv)
{
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LINK_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    CoInitialize(NULL);
    SHSetInstanceExplorer(static_cast<IUnknown*>(&g_EI));

    bool failArgs = false;
    for (int n = 1; n < argc; ++n)
    {
        WCHAR* cmd = argv[n];
        if (cmd[0] == '-' || cmd[0] == '/')
        {
            PCWSTR arg;
            if (isCmdWithArg(argc, argv, n, L"shgfi", arg))
            {
                failArgs = true;
                if (*arg)
                    return SHGFI(arg);
            }
            else if (isCmd(argc, argv, n, L"assocq"))
            {
                failArgs = true;
                if (argc - (n + 1) >= 6 && argc - (n + 1) <= 8)
                    return AssocQ(argc - (n + 1), &argv[(n + 1)]);
            }
            else if (isCmdWithArg(argc, argv, n, L"shellexec", arg))
            {
                PIDLIST_ABSOLUTE pidl = NULL;
                HRESULT hr = SHParseDisplayName(arg, NULL, &pidl, 0, NULL);
                if (FAILED(hr))
                    return ErrMsg(hr);
                SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_IDLIST | SEE_MASK_UNICODE };
                sei.lpIDList = pidl;
                sei.nShow = SW_SHOW;
                while (++n < argc)
                {
                    if (argv[n][0] != '-' && argv[n][0] != '/')
                        break;
                    else if (isCmd(argc, argv, n, L"INVOKE"))
                        sei.fMask |= SEE_MASK_INVOKEIDLIST;
                    else if (isCmd(argc, argv, n, L"NOUI"))
                        sei.fMask |= SEE_MASK_FLAG_NO_UI;
                    else if (isCmd(argc, argv, n, L"ASYNCOK"))
                        sei.fMask |= SEE_MASK_ASYNCOK ;
                    else if (isCmd(argc, argv, n, L"NOASYNC"))
                        sei.fMask |= SEE_MASK_NOASYNC;
                    else
                        wprintf(L"WARN: Ignoring switch %s\n", argv[n]);
                }
                if (n < argc && *argv[n++])
                {
                    sei.lpVerb = argv[n - 1];
                }
                if (n < argc && *argv[n++])
                {
                    sei.lpClass = argv[n - 1];
                    sei.fMask |= SEE_MASK_CLASSNAME;
                }
                UINT succ = ShellExecuteExW(&sei), gle = GetLastError();
                SHFree(pidl);
                if (!succ)
                    return ErrMsg(gle);
                Wait();
                return 0;
            }
            else if (isCmdWithArg(argc, argv, n, L"dumpmenu", arg))
            {
                HRESULT hr;
                CComPtr<IContextMenu> cm;
                if (CLSIDPrefix(arg, g_CLSID))
                {
                    g_ShellExtInit = arg;
                    hr = LoadAndInitialize(IID_PPV_ARG(IContextMenu, &cm));
                }
                else
                {
                    CComPtr<IShellItem> si;
                    hr = CreateShellItemFromParse(arg, &si);
                    if (SUCCEEDED(hr))
                        hr = si->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IContextMenu, &cm));
                }
                if (SUCCEEDED(hr))
                {
                    UINT first = 10, last = 9000;
                    UINT cmf = 0, nosub = 0, fakeinit = 0;
                    while (++n < argc)
                    {
                        if (argv[n][0] != '-' && argv[n][0] != '/')
                            break;
                        else if (isCmd(argc, argv, n, L"DEFAULTONLY"))
                            cmf |= CMF_DEFAULTONLY;
                        else if (isCmd(argc, argv, n, L"NODEFAULT"))
                            cmf |= CMF_NODEFAULT;
                        else if (isCmd(argc, argv, n, L"DONOTPICKDEFAULT"))
                            cmf |= CMF_DONOTPICKDEFAULT;
                        else if (isCmd(argc, argv, n, L"EXTENDED") || isCmd(argc, argv, n, L"EXTENDEDVERBS"))
                            cmf |= CMF_EXTENDEDVERBS;
                        else if (isCmd(argc, argv, n, L"SYNCCASCADEMENU"))
                            cmf |= CMF_SYNCCASCADEMENU;
                        else if (isCmd(argc, argv, n, L"EXPLORE"))
                            cmf |= CMF_EXPLORE;
                        else if (isCmd(argc, argv, n, L"VERBSONLY"))
                            cmf |= CMF_VERBSONLY;
                        else if (isCmd(argc, argv, n, L"NOVERBS"))
                            cmf |= CMF_NOVERBS;
                        else if (isCmd(argc, argv, n, L"DISABLEDVERBS"))
                            cmf |= CMF_DISABLEDVERBS;
                        else if (isCmd(argc, argv, n, L"OPTIMIZEFORINVOKE"))
                            cmf |= CMF_OPTIMIZEFORINVOKE;
                        else if (isCmd(argc, argv, n, L"CANRENAME"))
                            cmf |= CMF_CANRENAME;
                        else if (isCmd(argc, argv, n, L"NOSUBMENU"))
                            nosub++;
                        else if (isCmd(argc, argv, n, L"INITMENUPOPUP"))
                            fakeinit++; // Tickle async submenus
                        else
                            wprintf(L"WARN: Ignoring switch %s\n", argv[n]);
                    }
                    HMENU hMenu = CreatePopupMenu();
                    hr = cm->QueryContextMenu(hMenu, 0, first, last, cmf);
                    if (SUCCEEDED(hr))
                    {
                        DumpMenu(hMenu, first, cm, fakeinit, nosub ? -1 : 0);
                    }
                    DestroyMenu(hMenu);
                }
                if (FAILED(hr))
                    return ErrMsg(hr);
                return 0;
            }
            else if (isCmdWithArg(argc, argv, n, L"clsid", arg))
            {
                HRESULT hr = CLSIDFromString(arg, &g_CLSID);
                if (!SUCCEEDED(hr))
                {
                    wprintf(L"Failed to convert %s to CLSID\n", arg);
                    failArgs = true;
                }
            }
            else if (isCmdWithArg(argc, argv, n, L"dll", arg))
            {
                g_DLL = arg;
            }
            else if (isCmdWithArg(argc, argv, n, L"IShellExtInit", arg))
            {
                g_ShellExtInit = arg;
            }
            else if (isCmd(argc, argv, n, L"IShellPropSheetExt"))
            {
                g_bIShellPropSheetExt = true;
            }
            else if (isCmdWithArg(argc, argv, n, L"IContextMenu", arg))
            {
                g_ContextMenu = arg;
            }
            else if (isCmd(argc, argv, n, L"infinite"))
            {
                g_Wait = Wait_Infinite;
            }
            else if (isCmd(argc, argv, n, L"openwindows"))
            {
                g_Wait = Wait_OpenWindows;
            }
            else if (isCmd(argc, argv, n, L"input"))
            {
                g_Wait = Wait_Input;
            }
            else if (isCmd(argc, argv, n, L"explorerinstance"))
            {
                g_Wait = Wait_ExplorerInstance;
            }
            else if (isCmd(argc, argv, n, L"nowait"))
            {
                g_Wait = Wait_None;
            }
            else
            {
                wprintf(L"Unknown argument: %s\n", cmd);
                failArgs = true;
            }
        }
    }

    if (failArgs)
    {
        PrintHelp(NULL);
        return E_INVALIDARG;
    }

    CLSID EmptyCLSID = { 0 };
    if (EmptyCLSID == g_CLSID)
    {
        PrintHelp(L"No CLSID specified");
        return E_INVALIDARG;
    }

    if (g_ShellExtInit.IsEmpty())
    {
        PrintHelp(L"No filename specified");
        return E_INVALIDARG;
    }

    HRESULT hr;
    if (g_bIShellPropSheetExt)
    {
        CComPtr<IShellPropSheetExt> spSheetExt;
        hr = LoadAndInitialize(IID_PPV_ARG(IShellPropSheetExt, &spSheetExt));
        if (!SUCCEEDED(hr))
            return hr;

        hr = spSheetExt->AddPages(cb_AddPage, (LPARAM)&g_Pages);
        if (!SUCCEEDED(hr))
        {
            wprintf(L"IShellPropSheetExt->AddPages failed: 0x%x\n", hr);
            return hr;
        }

        USHORT ActivePage = HRESULT_CODE(hr);
        PROPSHEETHEADERW psh = { 0 };

        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_PROPTITLE;
        psh.pszCaption = L"shlextdbg";
        psh.phpage = g_Pages.GetData();
        psh.nPages = g_Pages.GetSize();
        psh.nStartPage = ActivePage ? (ActivePage-1) : 0;
        hr = PropertySheetW(&psh);

        wprintf(L"PropertySheetW returned: 0x%x\n", hr);
    }
    if (!g_ContextMenu.IsEmpty())
    {
        CComPtr<IContextMenu> spContextMenu;
        hr = LoadAndInitialize(IID_PPV_ARG(IContextMenu, &spContextMenu));
        if (!SUCCEEDED(hr))
            return hr;

        // FIXME: Must call QueryContextMenu before InvokeCommand?

        CMINVOKECOMMANDINFO cm = { sizeof(cm), 0 };
        cm.lpVerb = g_ContextMenu.GetString();
        cm.nShow = SW_SHOW;
        hr = spContextMenu->InvokeCommand(&cm);

        if (!SUCCEEDED(hr))
        {
            wprintf(L"IContextMenu->InvokeCommand failed: 0x%x\n", hr);
            return hr;
        }
        wprintf(L"IContextMenu->InvokeCommand returned: 0x%x\n", hr);
    }

    Wait();
    return 0;
}
