/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for shell menu objects
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "shelltest.h"

#include <shlwapi.h>
#include <unknownbase.h>
#include <shlguid_undoc.h>

#define test_S_OK(hres, message) ok(hres == S_OK, "%s (0x%lx instead of S_OK)\n",message, hResult);
#define test_HRES(hres, hresExpected, message) ok(hres == hresExpected, "%s (0x%lx instead of 0x%lx)\n",message, hResult,hresExpected);

BOOL CheckWindowClass(HWND hwnd, PCWSTR className)
{
    ULONG size = (lstrlenW(className) + 1)* sizeof(WCHAR);
    PWCHAR buffer = (PWCHAR)malloc(size);
    if (GetClassNameW(hwnd, buffer, size ) == 0)
    {
        free(buffer);
        return FALSE;
    }
    int res = wcscmp(buffer, className);
    free(buffer);
    return res == 0;
}

class CDummyWindow : public CUnknownBase<IOleWindow>
{
protected:
    HWND m_hwnd;

    const QITAB* GetQITab()
    {
        static const QITAB tab[] = {{ &IID_IOleWindow, OFFSETOFCLASS(IOleWindow, CDummyWindow) }, {0}};
        return tab;
    }

public:
    CDummyWindow(HWND hwnd)
        :CUnknownBase( true, 0 )
    {
        m_hwnd = hwnd;
    }

   HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd)
   {
       *phwnd = m_hwnd;
       return S_OK;
   }

   HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode)
   {
       return S_OK;
   }
};

BOOL CreateCShellMenu(IShellMenu** shellMenu, IDockingWindow** dockingMenu, IObjectWithSite **menuWithSite)
{
    HRESULT hResult;
    hResult = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_IShellMenu, reinterpret_cast<void **>(shellMenu));
    test_S_OK(hResult, "Failed to instantiate CLSID_MenuBand");
    if (!shellMenu) return FALSE;

    hResult = (*shellMenu)->QueryInterface(IID_IDockingWindow, reinterpret_cast<void **>(dockingMenu));
    test_S_OK(hResult, "Failed to query IID_IDockingWindow");
    hResult = (*shellMenu)->QueryInterface(IID_IObjectWithSite, reinterpret_cast<void **>(menuWithSite));
    test_S_OK(hResult, "Failed to query IID_IObjectWithSite");
    if (!dockingMenu || !menuWithSite) return FALSE;
    return TRUE;
}


void test_CShellMenu_params()
{
    HRESULT hResult;
    IShellMenu* shellMenu;
    IDockingWindow* dockingMenu;
    IObjectWithSite* menuWithSite;

    IShellMenuCallback *psmc;
    UINT uId;
    UINT uIdAncestor;
    DWORD dwFlags;
    HWND hwndToolbar;
    HMENU hmenu;
    HWND hwndOwner;
    DWORD menuFlagss;
    IShellFolder *shellFolder;

    if (!CreateCShellMenu(&shellMenu, &dockingMenu, &menuWithSite))
    {
        skip("failed to create CShellMenuObject\n");
        return;
    }

    hResult = shellMenu->Initialize(NULL, 11, 22, 0xdeadbeef);
    test_S_OK(hResult, "Initialize failed");

    hResult = shellMenu->GetMenuInfo(&psmc, &uId, &uIdAncestor, &dwFlags);
    test_S_OK(hResult, "GetMenuInfo failed");
    ok (psmc == NULL, "wrong psmc\n");
    ok (uId == 11, "wrong uid\n");
    ok (uIdAncestor == 22, "wrong uIdAncestor\n");
    ok (dwFlags == 0xdeadbeef, "wrong dwFlags\n");

    hResult = shellMenu->Initialize(NULL, 0, ANCESTORDEFAULT, SMINIT_TOPLEVEL|SMINIT_VERTICAL);
    test_S_OK(hResult, "Initialize failed");

    hResult = dockingMenu->GetWindow(&hwndToolbar);
    test_HRES(hResult, E_FAIL, "GetWindow should fail");

    hResult = shellMenu->GetMenu(&hmenu, &hwndOwner, &menuFlagss);
    test_HRES(hResult, E_FAIL, "GetMenu should fail");

    hmenu = CreatePopupMenu();
    hResult = shellMenu->SetMenu(hmenu, NULL, 0);
    test_S_OK(hResult, "SetMenu failed");

    hwndToolbar = (HWND)UlongToPtr(0xdeadbeef);
    hResult = dockingMenu->GetWindow(&hwndToolbar);
    test_S_OK(hResult, "GetWindow failed");
    ok (hwndToolbar == NULL, "Expected NULL window\n");

    hResult = shellMenu->SetMenu(NULL, NULL, 0);
    test_S_OK(hResult, "SetMenu failed");

    hResult = shellMenu->GetMenu(&hmenu, &hwndOwner, &menuFlagss);
    test_S_OK(hResult, "GetMenu failed");
    ok (hmenu == NULL, "Got a menu\n");

    hResult = dockingMenu->GetWindow(&hwndToolbar);
    test_S_OK(hResult, "GetWindow failed");

    hResult = SHGetDesktopFolder(&shellFolder);
    test_S_OK(hResult, "SHGetDesktopFolder failed");

    hResult = shellMenu->SetShellFolder(shellFolder, NULL, 0, 0);
    test_S_OK(hResult, "SetShellFolder failed");

    hResult = shellMenu->SetShellFolder(NULL, NULL, 0, 0);
    test_HRES(hResult, E_INVALIDARG, "SetShellFolder should fail");

    hwndToolbar = (HWND)UlongToHandle(0xdeadbeef);
    hResult = dockingMenu->GetWindow(&hwndToolbar);
    test_S_OK(hResult, "GetWindow failed");
    ok (hwndToolbar == NULL, "Expected NULL window\n");

    hResult = dockingMenu->ShowDW(TRUE);
    test_HRES(hResult, S_FALSE, "ShowDW should fail");

    menuWithSite->Release();
    dockingMenu->Release();
    shellMenu->Release();
}

void test_CShellMenu()
{
    HRESULT hResult;
    IShellMenu* shellMenu;
    IDockingWindow* dockingMenu;
    IShellFolder *shellFolder;
    IObjectWithSite *menuWithSite;
    HWND hwndToolbar;

    HWND hWndParent = CreateWindowExW(0, L"EDIT", L"miau", 0, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    CDummyWindow* dummyWindow = new CDummyWindow(hWndParent);

    if (!CreateCShellMenu(&shellMenu, &dockingMenu, &menuWithSite))
    {
        skip("failed to create CShellMenuObject\n");
        delete dummyWindow;
        return;
    }

    hResult = SHGetDesktopFolder(&shellFolder);
    test_S_OK(hResult, "SHGetDesktopFolder failed");

    hResult = shellMenu->Initialize(NULL, 0, ANCESTORDEFAULT, SMINIT_TOPLEVEL|SMINIT_VERTICAL);
    test_S_OK(hResult, "Initialize failed");

    hResult = shellMenu->SetShellFolder(shellFolder, NULL, NULL, 0);
    test_S_OK(hResult, "SetShellFolder failed");

    hResult = menuWithSite->SetSite(dummyWindow);
    test_S_OK(hResult, "SetSite failed");

    hResult = dockingMenu->GetWindow(&hwndToolbar);
    test_S_OK(hResult, "GetWindow failed");
    ok(hwndToolbar != NULL, "GetWindow should return a window\n");

    HWND hwndRealParent = GetParent(hwndToolbar);
    ok(GetParent(hwndRealParent) == hWndParent, "Wrong parent\n");
    ok(CheckWindowClass(hwndToolbar, L"ToolbarWindow32"), "Wrong class\n");
    ok(CheckWindowClass(hwndRealParent, L"SysPager"), "Wrong class\n");

    menuWithSite->Release();
    dockingMenu->Release();
    shellMenu->Release();
    ok(!IsWindow(hwndToolbar), "The toolbar window should not exist\n");

    DestroyWindow(hWndParent);
}

/* The folowing struct holds info about the order callbacks are called */
/* By passing different arrays of results to CMenuCallback, we can test different sequenses of callbacks */
   struct _test_info{
       int iTest;
       UINT uMsg;};

class CMenuCallback : public CUnknownBase<IShellMenuCallback>
{
protected:
    int m_iTest;
    int m_iCallback;
    struct _test_info *m_results;
    int m_testsCount;

    const QITAB* GetQITab()
    {
        static const QITAB tab[] = {{ &IID_IShellMenuCallback, OFFSETOFCLASS(IShellMenuCallback, CMenuCallback) }, {0}};
        return tab;
    }

public:
    CMenuCallback(struct _test_info *testResults, int testsCount)
        :CUnknownBase( true, 0 )
    {
        m_iTest = 0;
        m_iCallback = 0;
        m_results = testResults;
        m_testsCount = testsCount;
    }

   void SetTest(int i)
   {
       m_iTest = i;
   }

   HRESULT STDMETHODCALLTYPE CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
   {
       /*trace ("callback type %d\n", uMsg);*/

       /*
        * it seems callback 0x10000000 is called for every item added so
        * we will ignore consecutive callbacks of this type
        * Note: this callback is invoked by shell32.dll!CMenuSFToolbar::_FilterPidl
        */
       if (uMsg == 0x10000000 && m_results[m_iCallback-1].uMsg == 0x10000000)
       {
           return S_OK;
       }

       m_iCallback++;
       if (m_iCallback > m_testsCount)
       {
           ok(0, "Got more callbacks than expected! (%d not %d). uMsg: %d\n", m_iCallback, m_testsCount, uMsg);
           return S_OK;
       }

       struct _test_info *result = &m_results[m_iCallback-1];

       ok(psmd != NULL, "Got NULL psmd\n");
       ok(m_iTest == result->iTest, "Wrong test number (%d not %d)\n", m_iTest, result->iTest);
       ok(result->uMsg == uMsg, "%d: Got wrong uMsg (%d instead of %d)\n", m_iCallback, uMsg, result->uMsg);

       if(uMsg == SMC_CREATE)
       {
           ok(psmd->dwFlags == 0, "wrong dwFlags\n");
           ok(psmd->dwMask == 0, "wrong dwMask\n");
           ok(psmd->hmenu == 0, "wrong hmenu\n");
           ok(psmd->hwnd == 0, "wrong hwnd\n");
           ok(psmd->punk != NULL, "punk is null\n");
       }

       if (uMsg == SMC_GETSFOBJECT)
       {
           ok(psmd->psf != 0, "wrong dwFlags\n");
       }

       return S_FALSE;
   }
};

void test_CShellMenu_callbacks(IShellFolder *shellFolder, HMENU hmenu)
{
    HRESULT hResult;
    IShellMenu* shellMenu;
    IDockingWindow* dockingMenu;
    IObjectWithSite *menuWithSite;
    CMenuCallback *callback;

    HWND hWndParent = CreateWindowExW(0, L"EDIT", L"miau", 0, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    CDummyWindow* dummyWindow = new CDummyWindow(hWndParent);
    ShowWindow(hWndParent, SW_SHOW);

    if (!CreateCShellMenu(&shellMenu, &dockingMenu, &menuWithSite))
    {
        skip("failed to create CShellMenuObject\n");
        delete dummyWindow;
        return;
    }

    struct _test_info cbtest_info[] =  { {1, SMC_CREATE},
                                         {2, SMC_GETSFOBJECT},
                                         {3, 0x31},
                                         {4, SMC_INITMENU},
                                         {4, 53},
                                         {4, 19},
                                         {4, 0x10000000},
                                         {4, SMC_NEWITEM},
                                         {4, 20},
                                         {4, 19},
                                         {4, 6},
                                         {4, 20},
                                         {4, 8},
                                         {4, 24},
                                         {4, 5},
                                         {4, 5},
                                         {4, 5}};

    callback = new CMenuCallback(cbtest_info,18);

    callback->SetTest(1);
    hResult = shellMenu->Initialize(callback, 0,ANCESTORDEFAULT, SMINIT_TOPLEVEL|SMINIT_VERTICAL);
    test_S_OK(hResult, "Initialize failed");

    callback->SetTest(2);
    hResult = shellMenu->SetShellFolder(shellFolder, NULL, NULL, 0);
    test_S_OK(hResult, "SetShellFolder failed");

    callback->SetTest(3);
    hResult = shellMenu->SetMenu(hmenu, hWndParent, SMSET_TOP);
    test_S_OK(hResult, "SetMenu failed");

    hResult = menuWithSite->SetSite(dummyWindow);
    test_S_OK(hResult, "SetSite failed");

    callback->SetTest(4);
    hResult = dockingMenu->ShowDW(TRUE);
    test_HRES(hResult, S_FALSE, "ShowDW failed");
}

void test_CShellMenu_with_DeskBar(IShellFolder *shellFolder, HMENU hmenu)
{
    HRESULT hResult;
    IShellMenu* shellMenu;
    IDockingWindow* dockingMenu;
    IObjectWithSite *menuWithSite;
    IMenuPopup* menuPopup;
    IBandSite* bandSite;

    /* Create the tree objects and query the nescesary interfaces */
    BOOL bCreated = CreateCShellMenu(&shellMenu, &dockingMenu, &menuWithSite);
    hResult = CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_IMenuPopup, reinterpret_cast<void **>(&menuPopup));
    test_S_OK(hResult, "Failed to instantiate CLSID_MenuDeskBar");
    hResult = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_IBandSite, reinterpret_cast<void **>(&bandSite));
    test_S_OK(hResult, "Failed to instantiate CLSID_MenuBandSite");
    if (!bCreated || !menuPopup || !bandSite)
    {
        skip("failed to create MenuBandSite object\n");
        return;
    }

    /* Create the popup menu */
    hResult = shellMenu->Initialize(NULL, 0, ANCESTORDEFAULT, SMINIT_TOPLEVEL|SMINIT_VERTICAL);
    test_S_OK(hResult, "Initialize failed");
    hResult = shellMenu->SetMenu( hmenu, NULL, SMSET_TOP);
    test_S_OK(hResult, "SetMenu failed");
    hResult = menuPopup->SetClient(bandSite);
    test_S_OK(hResult, "SetClient failed");
    hResult = bandSite->AddBand(shellMenu);
    test_S_OK(hResult, "AddBand failed");

    /* Show the popum menu */
    POINTL p = {10,10};
    hResult = menuPopup->Popup(&p, NULL, 0);
    test_HRES(hResult, S_FALSE, "Popup failed");

    HWND hWndToolbar, hWndToplevel;

    /* Ensure that the created windows are correct */
    hResult = dockingMenu->GetWindow(&hWndToolbar);
    test_S_OK(hResult, "GetWindow failed");
    ok(hWndToolbar != NULL, "GetWindow should return a window\n");

    hResult = menuPopup->GetWindow(&hWndToplevel);
    test_S_OK(hResult, "GetWindow failed");
    ok(hWndToolbar != NULL, "GetWindow should return a window\n");

    HWND hwndRealParent = GetParent(hWndToolbar);
    ok(GetParent(hwndRealParent) == hWndToplevel, "Wrong parent\n");
    ok(CheckWindowClass(hWndToolbar, L"ToolbarWindow32"), "Wrong class\n");
    ok(CheckWindowClass(hwndRealParent, L"MenuSite"), "Wrong class\n");
    ok(CheckWindowClass(hWndToplevel, L"BaseBar"), "Wrong class\n");

    ok(GetAncestor (hWndToplevel, GA_PARENT) == GetDesktopWindow(), "Expected the BaseBar window to be top level\n");
}

START_TEST(menu)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    IShellFolder *shellFolder;
    HRESULT hResult;
    hResult = SHGetDesktopFolder(&shellFolder);
    test_S_OK(hResult, "SHGetDesktopFolder failed");

    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, 0,0, L"Submenu item1");
    AppendMenuW(hSubMenu, 0,0, L"Submenu item2");
    HMENU hmenu = CreatePopupMenu();
    AppendMenuW(hmenu, 0,0, L"test");
    AppendMenuW(hmenu, 0,1, L"test1");
    MENUITEMINFOW iteminfo = {0};
    iteminfo.cbSize = sizeof(iteminfo);
    iteminfo.hSubMenu = hSubMenu;
    iteminfo.fMask = MIIM_STRING | MIIM_SUBMENU;
    iteminfo.dwTypeData = const_cast<LPWSTR>(L"submenu");
    iteminfo.cch = 7;
    InsertMenuItemW(hmenu, 0, TRUE, &iteminfo);

    test_CShellMenu_params();
    test_CShellMenu();
    test_CShellMenu_callbacks(shellFolder, hmenu);
    test_CShellMenu_with_DeskBar(shellFolder, hmenu);
}

