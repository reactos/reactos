/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for ACListISF objects
 * PROGRAMMER:      Mark Jansen
 */

#define _UNICODE
#define UNICODE
#include <apitest.h>
#include <shlobj.h>
#include <atlbase.h>
#include <tchar.h>      //
#include <atlcom.h>     // These 3 includes only exist here to make gcc happy about (unused) templates..
#include <atlwin.h>     //

// Yes, gcc at it again, let's validate everything found inside unused templates!
ULONG DbgPrint(PCH Format,...);

#include <stdio.h>
#include <shellutils.h>
#include <shlwapi.h>
#include <strsafe.h>

static bool g_ShowHidden;
static DWORD g_WinVersion;
#define WINVER_VISTA   0x0600


#define ok_hr(status, expected)     ok_hex(status, expected)

// We do not want our results to originate from the helper functions, so have them originate from the calls to them
#define test_at_end                 (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_at_end_imp
#define test_ExpectDrives           (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_ExpectDrives_imp
#define test_ExpectFolders          (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_ExpectFolders_imp
#define winetest_ok_hr(expression, expected) \
    do { \
        int _value = (expression); \
        winetest_ok(_value == (expected), "Wrong value for '%s', expected: " #expected " (0x%x), got: 0x%x\n", \
           #expression, (int)(expected), _value); \
    } while (0)




static void test_at_end_imp(CComPtr<IEnumString>& EnumStr)
{
    CComHeapPtr<OLECHAR> Result;
    ULONG Fetched = 12345;
    HRESULT hr = EnumStr->Next(1, &Result, &Fetched);
    winetest_ok(hr == S_FALSE, "Expected hr to be S_FALSE, was 0x%lx\n", hr);
    winetest_ok(Fetched == 0u, "Expected Fetched to be 0, was: %lu\n", Fetched);
    if (Fetched == 1u)
        winetest_ok(0, "Expected there not to be a result, got: %s\n", wine_dbgstr_w(Result));
}

static bool GetDisplayname(CComPtr<IShellFolder>& spDrives, CComHeapPtr<ITEMIDLIST>& pidl, CComHeapPtr<WCHAR>& DisplayName)
{
    STRRET StrRet;
    HRESULT hr;
    winetest_ok_hr(hr = spDrives->GetDisplayNameOf(pidl, SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, &StrRet), S_OK);
    if (!SUCCEEDED(hr))
        return false;

    winetest_ok_hr(hr = StrRetToStrW(&StrRet, NULL, &DisplayName), S_OK);
    if (!SUCCEEDED(hr))
        return false;
    return true;
}

enum ExpectOptions
{
    None = 0,
    IgnoreRoot = 1,
    CheckLast = 2,
    IgnoreHidden = 4,
    IgnoreFiles = 8,
};

// wtf c++
ExpectOptions operator | (const ExpectOptions& left, const ExpectOptions& right)
{
    return static_cast<ExpectOptions>(static_cast<int>(left) | static_cast<int>(right));
}


static void
test_ExpectFolders_imp(CComPtr<IEnumString>& EnumStr, LPITEMIDLIST pidlTarget, const WCHAR* Root, ExpectOptions options)
{
    CComPtr<IShellFolder> spDesktop;
    HRESULT hr = SHGetDesktopFolder(&spDesktop);

    CComPtr<IShellFolder> spTarget;
    if (pidlTarget)
    {
        winetest_ok_hr(hr = spDesktop->BindToObject(pidlTarget, NULL, IID_PPV_ARG(IShellFolder, &spTarget)), S_OK);
        if (!SUCCEEDED(hr))
            return;
    }
    else
    {
        spTarget = spDesktop;
    }

    SHCONTF EnumFlags = SHCONTF_FOLDERS | SHCONTF_INIT_ON_FIRST_NEXT;
    if (g_ShowHidden && !(options & IgnoreHidden))
        EnumFlags |= SHCONTF_INCLUDEHIDDEN;
    if (!(options & IgnoreFiles))
        EnumFlags |= SHCONTF_NONFOLDERS;

    CComPtr<IEnumIDList> spEnumIDList;
    winetest_ok_hr(hr = spTarget->EnumObjects(NULL, EnumFlags, &spEnumIDList), S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR Buffer[512];
    CComHeapPtr<ITEMIDLIST> pidl;
    INT Count = 0;
    while (spEnumIDList->Next(1, &pidl, NULL) == S_OK)
    {
        CComHeapPtr<WCHAR> DisplayName;
        if (!GetDisplayname(spTarget, pidl, DisplayName))
            break;

        CComHeapPtr<OLECHAR> Result;
        ULONG Fetched;
        hr = EnumStr->Next(1, &Result, &Fetched);
        winetest_ok_hr(hr, S_OK);


        if (hr != S_OK)
            break;


        StringCchPrintfW(Buffer, _ARRAYSIZE(Buffer), L"%s%s", (options & IgnoreRoot) ? L"" : Root, (WCHAR*)DisplayName);

        winetest_ok(!wcscmp(Buffer, Result), "Expected %s, got %s\n", wine_dbgstr_w(Buffer), wine_dbgstr_w(Result));

        pidl.Free();
        Count++;
    }
    if (options & CheckLast)
    {
        test_at_end_imp(EnumStr);
    }
}

static void
test_ExpectDrives_imp(CComPtr<IEnumString>& EnumStr, CComHeapPtr<ITEMIDLIST>& pidlTarget)
{
    test_ExpectFolders_imp(EnumStr, pidlTarget, NULL, IgnoreRoot | CheckLast);
}

static void
test_ACListISF_NONE()
{
    CComPtr<IEnumString> EnumStr;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_ALL, IID_PPV_ARG(IEnumString, &EnumStr));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IACList2> ACList;
    ok_hr(hr = EnumStr->QueryInterface(IID_IACList2, (void**)&ACList), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = ACList->SetOptions(ACLO_NONE), S_OK);
    test_at_end(EnumStr);


    WCHAR Buffer[MAX_PATH];
    GetSystemWindowsDirectoryW(Buffer, _ARRAYSIZE(Buffer));
    Buffer[3] = '\0';

    CComHeapPtr<ITEMIDLIST> pidlDiskRoot;
    ok_hr(hr = SHParseDisplayName(Buffer, NULL, &pidlDiskRoot, NULL, NULL), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = ACList->Expand(Buffer), S_OK);
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer, CheckLast | IgnoreHidden);

    ok_hr(hr = EnumStr->Reset(), S_OK);
    ok_hr(hr = ACList->Expand(Buffer), S_OK);
    ok_hr(hr = ACList->SetOptions(ACLO_NONE), S_OK);
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer, CheckLast);
}

static void
test_ACListISF_CURRENTDIR()
{
    CComPtr<IEnumString> EnumStr;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_ALL, IID_PPV_ARG(IEnumString, &EnumStr));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IACList2> ACList;
    ok_hr(hr = EnumStr->QueryInterface(IID_IACList2, (void**)&ACList), S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<ICurrentWorkingDirectory> CurrentWorkingDir;
    ok_hr(hr = EnumStr->QueryInterface(IID_ICurrentWorkingDirectory, (void**)&CurrentWorkingDir), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = ACList->SetOptions(ACLO_CURRENTDIR), S_OK);
    test_at_end(EnumStr);


    WCHAR Buffer[MAX_PATH] = { 1, 1, 1, 1, 1, 0 }, Buffer2[MAX_PATH];
    if (g_WinVersion < WINVER_VISTA)
        ok_hr(hr = CurrentWorkingDir->GetDirectory(Buffer, _ARRAYSIZE(Buffer)), E_NOTIMPL);
    else
        ok_hr(hr = CurrentWorkingDir->GetDirectory(Buffer, _ARRAYSIZE(Buffer)), E_UNEXPECTED);
    ok(!wcscmp(L"\x1\x1\x1\x1\x1", Buffer), "Expected %s, got %s\n", wine_dbgstr_w(L"\x1\x1\x1\x1\x1"), wine_dbgstr_w(Buffer));

    GetSystemWindowsDirectoryW(Buffer2, _ARRAYSIZE(Buffer2));
    // Windows 2k3 does not parse it without the trailing '\\'
    Buffer2[3] = '\0';
    CComHeapPtr<ITEMIDLIST> pidlDiskRoot;
    ok_hr(hr = SHParseDisplayName(Buffer2, NULL, &pidlDiskRoot, NULL, NULL), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = CurrentWorkingDir->SetDirectory(Buffer2), S_OK);
    test_at_end(EnumStr);

    Buffer[0] = '\0';
    if (g_WinVersion < WINVER_VISTA)
    {
        ok_hr(hr = CurrentWorkingDir->GetDirectory(Buffer, _ARRAYSIZE(Buffer)), E_NOTIMPL);
    }
    else
    {
        ok_hr(hr = CurrentWorkingDir->GetDirectory(Buffer, _ARRAYSIZE(Buffer)), S_OK);
        ok(!wcscmp(Buffer2, Buffer), "Expected %s, got %s\n", wine_dbgstr_w(Buffer2), wine_dbgstr_w(Buffer));
    }

    Buffer2[2] = '\0';
    ok_hr(hr = CurrentWorkingDir->SetDirectory(Buffer2), S_OK);
    test_at_end(EnumStr);

    Buffer[0] = '\0';
    Buffer2[2] = '\\';
    if (g_WinVersion < WINVER_VISTA)
    {
        ok_hr(hr = CurrentWorkingDir->GetDirectory(Buffer, _ARRAYSIZE(Buffer)), E_NOTIMPL);
    }
    else
    {
        ok_hr(hr = CurrentWorkingDir->GetDirectory(Buffer, _ARRAYSIZE(Buffer)), S_OK);
        ok(!wcscmp(Buffer2, Buffer), "Expected %s, got %s\n", wine_dbgstr_w(Buffer2), wine_dbgstr_w(Buffer));
    }

    ok_hr(hr = ACList->Expand(Buffer2), S_OK);
    // The first set of results are absolute paths, without hidden files?!
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer2, IgnoreHidden);
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer2, IgnoreHidden | IgnoreRoot | CheckLast);
}

static void
test_ACListISF_MYCOMPUTER()
{
    CComPtr<IACList2> ACList;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_ALL, IID_PPV_ARG(IACList2, &ACList));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    // Check the default
    DWORD CurrentOption = 0xdeadbeef;
    ok_hr(ACList->GetOptions(&CurrentOption), S_OK);
    ok(CurrentOption == (ACLO_CURRENTDIR|ACLO_MYCOMPUTER), "Expected the default to be %x, was %lx\n",
        (ACLO_CURRENTDIR|ACLO_MYCOMPUTER), CurrentOption);


    CComPtr<IEnumString> EnumStr;
    ok_hr(hr = ACList->QueryInterface(IID_IEnumString, (void**)&EnumStr), S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IPersistFolder> PersistFolder;
    ok_hr(hr = EnumStr->QueryInterface(IID_IPersistFolder, (void**)&PersistFolder), S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComHeapPtr<ITEMIDLIST> pidlMyComputer;
    ok_hr(hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer), S_OK);
    if (!SUCCEEDED(hr))
        return;


    hr = EnumStr->Reset();
    if (g_WinVersion < WINVER_VISTA)
        ok_hr(hr, S_FALSE);
    else
        ok_hr(hr, S_OK);
    test_ExpectDrives(EnumStr, pidlMyComputer);

    ok_hr(hr = ACList->SetOptions(ACLO_MYCOMPUTER), S_OK);
    ok_hr(EnumStr->Reset(), S_OK);
    test_ExpectDrives(EnumStr, pidlMyComputer);

    WCHAR Buffer[MAX_PATH];
    GetSystemWindowsDirectoryW(Buffer, _ARRAYSIZE(Buffer));
    // Windows 2k3 does not parse it without the trailing '\\'
    Buffer[3] = '\0';
    CComHeapPtr<ITEMIDLIST> pidlDiskRoot;
    ok_hr(hr = SHParseDisplayName(Buffer, NULL, &pidlDiskRoot, NULL, NULL), S_OK);
    if (!SUCCEEDED(hr))
        return;
    Buffer[2] = '\0';

    ok_hr(hr = ACList->Expand(Buffer), S_OK);
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer, None);
    test_ExpectDrives(EnumStr, pidlMyComputer);

    ok_hr(hr = ACList->Expand(Buffer), S_OK);
    ok_hr(EnumStr->Reset(), S_OK);
    // Pre vista does not remove the expanded data from the enumeration, it changes it to relative paths???
    if (g_WinVersion < WINVER_VISTA)
        test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer, IgnoreRoot);
    test_ExpectDrives(EnumStr, pidlMyComputer);

    ok_hr(EnumStr->Reset(), S_OK);
    ok_hr(hr = ACList->Expand(Buffer), S_OK);
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer, None);
    test_ExpectDrives(EnumStr, pidlMyComputer);
}

static void
test_ACListISF_DESKTOP()
{
    CComPtr<IEnumString> EnumStr;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_ALL, IID_PPV_ARG(IEnumString, &EnumStr));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IACList2> ACList;
    ok_hr(hr = EnumStr->QueryInterface(IID_IACList2, (void**)&ACList), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = ACList->SetOptions(ACLO_DESKTOP), S_OK);
    test_ExpectFolders(EnumStr, NULL, NULL, IgnoreRoot | CheckLast | IgnoreHidden);
}

static void
test_ACListISF_FAVORITES()
{
    CComPtr<IEnumString> EnumStr;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_ALL, IID_PPV_ARG(IEnumString, &EnumStr));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IACList2> ACList;
    ok_hr(hr = EnumStr->QueryInterface(IID_IACList2, (void**)&ACList), S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComHeapPtr<ITEMIDLIST> pidlFavorites;
    ok_hr(hr = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidlFavorites), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = ACList->SetOptions(ACLO_FAVORITES), S_OK);
    test_ExpectFolders(EnumStr, pidlFavorites, NULL, IgnoreRoot | CheckLast | IgnoreHidden);
}

static void
test_ACListISF_FILESYSONLY()
{
    CComPtr<IEnumString> EnumStr;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_ALL, IID_PPV_ARG(IEnumString, &EnumStr));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IACList2> ACList;
    ok_hr(hr = EnumStr->QueryInterface(IID_IACList2, (void**)&ACList), S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR Buffer[MAX_PATH];
    GetSystemWindowsDirectoryW(Buffer, _ARRAYSIZE(Buffer));
    // Windows 2k3 does not parse it without the trailing '\\'
    Buffer[3] = '\0';
    CComHeapPtr<ITEMIDLIST> pidlDiskRoot;
    ok_hr(hr = SHParseDisplayName(Buffer, NULL, &pidlDiskRoot, NULL, NULL), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = ACList->SetOptions(ACLO_FILESYSONLY), S_OK);
    test_at_end(EnumStr);

    ok_hr(hr = ACList->Expand(Buffer), S_OK);
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer, CheckLast | IgnoreHidden);
}

static void
test_ACListISF_FILESYSDIRS()
{
    CComPtr<IEnumString> EnumStr;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_ALL, IID_PPV_ARG(IEnumString, &EnumStr));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IACList2> ACList;
    ok_hr(hr = EnumStr->QueryInterface(IID_IACList2, (void**)&ACList), S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR Buffer[MAX_PATH];
    GetSystemWindowsDirectoryW(Buffer, _ARRAYSIZE(Buffer));
    // Windows 2k3 does not parse it without the trailing '\\'
    Buffer[3] = '\0';
    CComHeapPtr<ITEMIDLIST> pidlDiskRoot;
    ok_hr(hr = SHParseDisplayName(Buffer, NULL, &pidlDiskRoot, NULL, NULL), S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hr(hr = ACList->SetOptions(ACLO_FILESYSDIRS), S_OK);
    test_at_end(EnumStr);

    ok_hr(hr = ACList->Expand(Buffer), S_OK);
    test_ExpectFolders(EnumStr, pidlDiskRoot, Buffer, CheckLast | IgnoreFiles | IgnoreHidden);
}

static void GetEnvStatus()
{
    RTL_OSVERSIONINFOEXW rtlinfo = {0};
    void (__stdcall* pRtlGetVersion)(RTL_OSVERSIONINFOEXW*);
    pRtlGetVersion = (void (__stdcall*)(RTL_OSVERSIONINFOEXW*))GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

    rtlinfo.dwOSVersionInfoSize = sizeof(rtlinfo);
    pRtlGetVersion(&rtlinfo);
    g_WinVersion = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;

    SHELLFLAGSTATE sfs = {0};
    SHGetSettings(&sfs, SSF_SHOWALLOBJECTS);
    g_ShowHidden = !!sfs.fShowAllObjects;
    trace("Show hidden folders: %s\n", g_ShowHidden ? "yes" : "no");
}

struct CCoInit
{
    CCoInit() { hres = CoInitialize(NULL); }
    ~CCoInit() { if (SUCCEEDED(hres)) { CoUninitialize(); } }
    HRESULT hres;
};

START_TEST(ACListISF)
{
    GetEnvStatus();
    CCoInit init;
    ok_hr(init.hres, S_OK);
    if (!SUCCEEDED(init.hres))
        return;

    test_ACListISF_NONE();
    test_ACListISF_CURRENTDIR();
    test_ACListISF_MYCOMPUTER();
    test_ACListISF_DESKTOP();
    test_ACListISF_FAVORITES();
    test_ACListISF_FILESYSONLY();
    test_ACListISF_FILESYSDIRS();
}
