/*
 * PROJECT:     shlextdbg
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shell extension debug utility
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include <windows.h>
#include <shlobj.h>
#include <atlbase.h>   // thanks gcc
#include <atlcom.h>    // thanks gcc
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <conio.h>
#include <shellutils.h>

enum WaitType
{
    Wait_None,
    Wait_Infinite,
    Wait_OpenWindows,
    Wait_Input,
};

CLSID g_CLSID = { 0 };
CStringW g_DLL;
CStringW g_ShellExtInit;
bool g_bIShellPropSheetExt = false;
CStringA g_ContextMenu;
WaitType g_Wait = Wait_None;

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

    hr = spShellExtInit->Initialize(pidl, spDataObject, NULL);
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
    return !wcsicmp(argv[n] + 1, check);
}

static void PrintHelp(PCWSTR ExtraLine)
{
    if (ExtraLine)
        wprintf(L"%s\n", ExtraLine);

    wprintf(L"shlextdbg /clsid={clsid} [/dll=dllname] /IShellExtInit=filename |shlextype| |waitoptions|\n");
    wprintf(L"    {clsid}: The CLSID or ProgID of the object to create\n");
    wprintf(L"    dll: Optional dllname to create the object from, instead of CoCreateInstance\n");
    wprintf(L"    filename: The filename to pass to IShellExtInit->Initialze\n");
    wprintf(L"    shlextype: The type of shell extention to run:\n");
    wprintf(L"               /IShellPropSheetExt to create a property sheet\n");
    wprintf(L"               /IContextMenu=verb to activate the specified verb\n");
    wprintf(L"    waitoptions: Specify how to wait:\n");
    wprintf(L"                 /infinite: Keep on waiting infinitely\n");
    wprintf(L"                 /openwindows: Wait for all windows from the current application to close\n");
    wprintf(L"                 /input: Wait for input\n");
    wprintf(L"\n");
}

/*
Examples:

/clsid={513D916F-2A8E-4F51-AEAB-0CBC76FB1AF8} /IShellExtInit=C:\RosBE\Uninstall.exe /IShellPropSheetExt
/clsid=CompressedFolder /IShellExtInit=e:\test.zip /IContextMenu=extract /openwindows
/clsid=CompressedFolder /IShellExtInit=e:\test.zip /IContextMenu=extract /openwindows /dll=R:\build\dev\devenv\dll\shellext\zipfldr\Debug\zipfldr.dll

*/
extern "C"  // and another hack for gcc
int wmain(int argc, WCHAR **argv)
{
    bool failArgs = false;
    for (int n = 1; n < argc; ++n)
    {
        WCHAR* cmd = argv[n];
        if (cmd[0] == '-' || cmd[0] == '/')
        {
            PCWSTR arg;
            if (isCmdWithArg(argc, argv, n, L"clsid", arg))
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

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LINK_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    CoInitialize(NULL);

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

    switch (g_Wait)
    {
    case Wait_None:
        break;
    case Wait_Infinite:
        while (true) {
            Sleep(1000);
        }
        break;
    case Wait_OpenWindows:
        WaitWindows();
        break;
    case Wait_Input:
        wprintf(L"Press any key to continue...\n");
        _getch();
        break;

    }
    return 0;
}
