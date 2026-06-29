/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for SHInvokeCommandsOnContextMenu
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <windows.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <shobjidl.h>

typedef HRESULT (WINAPI *FN_SHInvokeCommandsOnContextMenu)(HWND, IUnknown *, IContextMenu *,
                                                           DWORD, PCSTR *, UINT);

static FN_SHInvokeCommandsOnContextMenu g_fnSHInvokeCommandsOnContextMenu = NULL;

struct MockContextMenu : public IContextMenu
{
    HRESULT queryResult   = S_OK;
    HRESULT invokeResult  = S_OK;
    UINT    defaultItemId = (UINT)-1;

    INT queryCalled  = 0;
    INT invokeCalled = 0;
    DWORD lastQueryFlags = 0;

    CHAR  lastVerb[MAX_PATH]  = {};
    WCHAR lastVerbW[MAX_PATH] = {};
    DWORD lastFMask      = 0;
    BOOL lastHadUnicode = FALSE;

    STDMETHODIMP_(ULONG) AddRef()  override { return 1; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }
    STDMETHODIMP QueryInterface(REFIID, void**) override
    {
        return E_NOINTERFACE;
    }

    STDMETHODIMP
    QueryContextMenu(HMENU hmenu, UINT, UINT idCmdFirst, UINT, UINT uFlags) override
    {
        ++queryCalled;
        lastQueryFlags = uFlags;

        if (FAILED(queryResult))
            return queryResult;

        if (defaultItemId != (UINT)-1)
        {
            AppendMenuA(hmenu, MF_STRING, idCmdFirst + defaultItemId - 1, "Item");
            SetMenuDefaultItem(hmenu, idCmdFirst + defaultItemId - 1, FALSE);
        }
        return queryResult;
    }

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici) override
    {
        ++invokeCalled;
        const CMINVOKECOMMANDINFOEX* iciex = reinterpret_cast<const CMINVOKECOMMANDINFOEX*>(pici);

        lastFMask = iciex->fMask;
        lastHadUnicode = !!(iciex->fMask & CMIC_MASK_UNICODE);

        if (HIWORD(iciex->lpVerb))
            lstrcpyA(lastVerb, iciex->lpVerb);
        else
            lastVerb[0] = ANSI_NULL;

        if (lastHadUnicode && iciex->lpVerbW && HIWORD(iciex->lpVerbW))
            lstrcpyW(lastVerbW, iciex->lpVerbW);
        else
            lastVerbW[0] = UNICODE_NULL;

        return invokeResult;
    }

    STDMETHODIMP GetCommandString(UINT_PTR, UINT, UINT*, CHAR*, UINT) override
    {
        return E_NOTIMPL;
    }
};

struct MockSite : public IUnknown
{
    STDMETHODIMP_(ULONG) AddRef()  override { return 1; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }
    STDMETHODIMP QueryInterface(REFIID, void**) override
    {
        return E_NOINTERFACE;
    }
};

class SHInvokeCommandsOnContextMenuTest : public MockContextMenu
{
protected:
    MockSite        site;

public:
    SHInvokeCommandsOnContextMenuTest()
    {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
    ~SHInvokeCommandsOnContextMenuTest()
    {
        CoUninitialize();
    }
};

static void ZeroVerbs_NoDefaultItem_ReturnsEFail(void)
{
    SHInvokeCommandsOnContextMenuTest cm;
    HRESULT hr = g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, NULL, 0);
    ok_hr(hr, E_FAIL);
    ok_int(cm.queryCalled, 1);
    ok_int(cm.invokeCalled, 0);
}

static void ZeroVerbs_WithDefaultItem_InvokesOnce(void)
{
    SHInvokeCommandsOnContextMenuTest cm;
    cm.defaultItemId = 1;
    cm.invokeResult  = S_OK;

    HRESULT hr = g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, NULL, 0);
    ok_hr(hr, S_OK);
    ok_int(cm.queryCalled,  1);
    ok_int(cm.invokeCalled, 1);
}

static void ZeroVerbs_QueryReceivesCMF_DEFAULTONLY(void)
{
    SHInvokeCommandsOnContextMenuTest cm;
    cm.defaultItemId = 1;
    g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, NULL, 0);
    ok_int(!!(cm.lastQueryFlags & CMF_DEFAULTONLY), TRUE);
}

static void NonZeroVerbs_QueryDoesNotReceiveCMF_DEFAULTONLY(void)
{
    const char* verbs[] = { "open" };
    SHInvokeCommandsOnContextMenuTest cm;
    g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, verbs, 1);
    ok_int(!!(cm.lastQueryFlags & CMF_DEFAULTONLY), FALSE);
}

static void SingleVerb_Success(void)
{
    const char* verbs[] = { "open" };
    SHInvokeCommandsOnContextMenuTest cm;
    cm.invokeResult = S_OK;

    HRESULT hr = g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, verbs, 1);
    ok_hr(hr, S_OK);
    ok_int(cm.invokeCalled, 1);
    ok_str(cm.lastVerb, "open");
}

static void MultipleVerbs_StopsOnFirstSuccess(void)
{
    const char* verbs[] = { "open", "print" };
    SHInvokeCommandsOnContextMenuTest cm;
    cm.invokeResult = S_OK;

    HRESULT hr = g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, verbs, 2);
    ok_hr(hr, S_OK);
    ok_int(cm.invokeCalled, 1);
    ok_str(cm.lastVerb, "open");
}

static void Cancelled_BreaksLoop(void)
{
    const char* verbs[] = { "open", "print" };
    SHInvokeCommandsOnContextMenuTest cm;
    cm.invokeResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    HRESULT hr = g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, verbs, 2);
    ok_hr(hr, HRESULT_FROM_WIN32(ERROR_CANCELLED));
    ok_int(cm.invokeCalled, 1);
}

static void AsciiVerb_SetsUnicodeMaskAndVerbW(void)
{
    const char* verbs[] = { "open" };
    SHInvokeCommandsOnContextMenuTest cm;
    cm.invokeResult = S_OK;

    g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, verbs, 1);
    ok_int(!!cm.lastHadUnicode, TRUE);
    ok_wstr(cm.lastVerbW, L"open");
}

static void FMask_IsPassedToInvokeCommand(void)
{
    const DWORD kMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    const char* verbs[] = { "open" };
    SHInvokeCommandsOnContextMenuTest cm;
    cm.invokeResult = S_OK;

    g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, kMask, verbs, 1);
    ok_int(!!(cm.lastFMask & kMask), TRUE);
}

struct MockSiteTarget : public IContextMenu, public IObjectWithSite
{
    IUnknown* site = NULL;
    BOOL siteSet  = FALSE;
    BOOL siteClear= FALSE;
    INT queryCnt = 0;

    // IUnknown
    STDMETHODIMP_(ULONG) AddRef()  override { return 1; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IObjectWithSite) { *ppv = static_cast<IObjectWithSite*>(this); return S_OK; }
        return E_NOINTERFACE;
    }

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown* pSite) override
    {
        if (pSite) siteSet  = true;
        else       siteClear= true;
        site = pSite;
        return S_OK;
    }
    STDMETHODIMP GetSite(REFIID, void**) override { return E_NOTIMPL; }

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU, UINT, UINT, UINT, UINT) override
    {
        ++queryCnt;
        return S_OK;
    }
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO) override { return E_FAIL; }
    STDMETHODIMP GetCommandString(UINT_PTR, UINT, UINT*, CHAR*, UINT) override { return E_NOTIMPL; }
};

static void PunkSite_SetAndClearedAroundCall(void)
{
    MockSiteTarget target;
    MockSite       mySite;

    const char* verbs[] = { "open" };
    g_fnSHInvokeCommandsOnContextMenu(NULL, &mySite, &target, 0, verbs, 1);

    ok_int(target.siteSet, TRUE);
    ok_int(target.siteClear, TRUE);
}

static void QueryFails_InvokeNotCalled(void)
{
    SHInvokeCommandsOnContextMenuTest cm;
    cm.queryResult  = E_FAIL;
    cm.invokeResult = S_OK;
    const char* verbs[] = { "open" };

    HRESULT hr = g_fnSHInvokeCommandsOnContextMenu(NULL, NULL, &cm, 0, verbs, 1);
    ok_int(FAILED(hr), TRUE);
    ok_int(cm.invokeCalled, 0);
}

struct HwndCaptureMock : public MockContextMenu
{
    HWND capturedHwnd = NULL;
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici) override
    {
        capturedHwnd = pici->hwnd;
        return S_OK;
    }
};

static void Hwnd_IsPassedToInvokeCommand(void)
{
    HwndCaptureMock mock;
    const char* verbs[] = { "open" };
    HWND fakeHwnd = reinterpret_cast<HWND>(static_cast<ULONG_PTR>(0xDEADBEEF));

    g_fnSHInvokeCommandsOnContextMenu(fakeHwnd, NULL, &mock, 0, verbs, 1);
    ok_ptr(mock.capturedHwnd, fakeHwnd);
}

START_TEST(SHInvokeCommandsOnContextMenu)
{
    HINSTANCE hSHLWAPI = LoadLibraryW(L"shlwapi");
    if (!hSHLWAPI)
    {
        skip("shlwapi not found\n");
        return;
    }

    g_fnSHInvokeCommandsOnContextMenu =
        (FN_SHInvokeCommandsOnContextMenu)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(541));
    if (!g_fnSHInvokeCommandsOnContextMenu)
    {
        skip("SHInvokeCommandsOnContextMenu not found\n");
        FreeLibrary(hSHLWAPI);
        return;
    }

    ZeroVerbs_NoDefaultItem_ReturnsEFail();
    ZeroVerbs_WithDefaultItem_InvokesOnce();
    ZeroVerbs_QueryReceivesCMF_DEFAULTONLY();
    NonZeroVerbs_QueryDoesNotReceiveCMF_DEFAULTONLY();
    SingleVerb_Success();
    MultipleVerbs_StopsOnFirstSuccess();
    Cancelled_BreaksLoop();
    AsciiVerb_SetsUnicodeMaskAndVerbW();
    FMask_IsPassedToInvokeCommand();
    PunkSite_SetAndClearedAroundCall();
    QueryFails_InvokeNotCalled();
    Hwnd_IsPassedToInvokeCommand();

    FreeLibrary(hSHLWAPI);
}
