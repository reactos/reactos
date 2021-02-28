/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for IAutoComplete objects
 * PROGRAMMER:      Katayama Hirofumi MZ
 */

#define _UNICODE
#define UNICODE
#include <apitest.h>
#include <shlobj.h>
#include <atlbase.h>
#include <tchar.h>      //
#include <atlcom.h>     // These 3 includes only exist here to make gcc happy about (unused) templates..
#include <atlwin.h>     //

#define ok_wstri(x, y) \
    ok(lstrcmpiW(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

struct CCoInit
{
    CCoInit() { hr = CoInitialize(NULL); }
    ~CCoInit() { if (SUCCEEDED(hr)) { CoUninitialize(); } }
    HRESULT hr;
};

static HWND MyCreateWindow(VOID)
{
    return CreateWindowW(L"EDIT", NULL, WS_POPUPWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                         NULL, NULL, GetModuleHandleW(NULL), NULL);
}

static void MyMessageLoop(void)
{
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static LPWSTR MyCoStrDup(LPCWSTR psz)
{
    INT cch = lstrlenW(psz);
    DWORD cb = (cch + 1) * sizeof(WCHAR);
    LPWSTR ret = (LPWSTR)::CoTaskMemAlloc(cb);
    if (ret)
        CopyMemory(ret, psz, cb);
    return ret;
}

class CEnumString : public IEnumString, public IACList2
{
public:
    CEnumString() : m_cRefs(1), m_nIndex(0), m_nCount(0), m_pList(NULL)
    {
    }

    virtual ~CEnumString()
    {
        for (UINT i = 0; i < m_nCount; ++i)
        {
            CoTaskMemFree(m_pList[i]);
            m_pList[i] = NULL;
        }
        CoTaskMemFree(m_pList);
    }

    VOID SetList(UINT nCount, LPWSTR *pList)
    {
        m_nCount = nCount;
        m_pList = pList;
    }

    STDMETHODIMP QueryInterface(REFIID iid, VOID** ppv) override
    {
        if (iid == IID_IUnknown || iid == IID_IEnumString)
        {
            AddRef();
            *ppv = static_cast<IEnumString *>(this);
            return S_OK;
        }
        if (iid == IID_IACList || iid == IID_IACList2)
        {
            AddRef();
            *ppv = static_cast<IACList2 *>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        ++m_cRefs;
        return m_cRefs;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        --m_cRefs;
        if (m_cRefs == 0)
        {
            delete this;
            return 0;
        }
        return m_cRefs;
    }

    STDMETHODIMP Next(ULONG celt, LPWSTR* rgelt, ULONG* pceltFetched) override
    {
        if (rgelt)
            *rgelt = NULL;
        if (pceltFetched)
            *pceltFetched = 0;
        if (celt != 1 || !rgelt || !pceltFetched)
            return E_INVALIDARG;
        if (m_nIndex >= m_nCount)
            return S_FALSE;

        *rgelt = MyCoStrDup(m_pList[m_nIndex]);
        if (!*rgelt)
            return E_OUTOFMEMORY;
        *pceltFetched = 1;
        return S_OK;
    }
    STDMETHODIMP Skip(ULONG celt) override
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP Reset() override
    {
        m_nIndex = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumString** ppenum) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP Expand(PCWSTR pszExpand) override
    {
        return S_OK;
    }
    STDMETHODIMP GetOptions(DWORD *pdwFlag) override
    {
        return S_OK;
    }
    STDMETHODIMP SetOptions(DWORD dwFlag) override
    {
        return S_OK;
    }

protected:
    ULONG m_cRefs;
    UINT m_nIndex;
    UINT m_nCount;
    LPWSTR *m_pList;
};

static VOID DoWordBreakProc(EDITWORDBREAKPROC fn)
{
    // TODO:
}

static VOID DoTest1(VOID)
{
    HWND hwndEdit = MyCreateWindow();
    ok(hwndEdit != NULL, "hwndEdit was NULL\n");
    ShowWindow(hwndEdit, SW_SHOWNORMAL);

    UINT nCount = 2;
    LPWSTR *pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    pList[0] = MyCoStrDup(L"test\\AA");
    pList[1] = MyCoStrDup(L"test\\BBB");
    CComPtr<CEnumString> pEnum = new CEnumString();
    pEnum->SetList(nCount, pList);

    CComPtr<IAutoComplete2> pAC;
    HRESULT hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IAutoComplete2, (VOID **)&pAC);
    ok_hr(hr, S_OK);

    hr = pAC->SetOptions(ACO_AUTOSUGGEST);
    ok_hr(hr, S_OK);

    IUnknown *punk = static_cast<IUnknown *>(static_cast<IEnumString *>(pEnum));
    hr = pAC->Init(hwndEdit, punk, NULL, NULL); // IAutoComplete::Init 0x80004002
    ok_hr(hr, S_OK);

    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    PostMessageW(hwndEdit, WM_CHAR, L't', 0);
    PostMessageW(hwndEdit, WM_CHAR, L'e', 0);
    PostMessageW(hwndEdit, WM_CHAR, L's', 0);
    PostMessageW(hwndEdit, WM_CHAR, L't', 0);
    PostMessageW(hwndEdit, WM_CHAR, L'\\', 0);

    DWORD style, exstyle;
    HWND hwndDropDown;
    LONG_PTR id;
    for (INT i = 0; i < 100; ++i)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        hwndDropDown = FindWindowW(L"Auto-Suggest Dropdown", L"");
        if (hwndDropDown)
            break;
        Sleep(100);
    }
    ok(hwndDropDown != NULL, "hwndDropDown was NULL\n");

    EDITWORDBREAKPROC fn =
        (EDITWORDBREAKPROC)SendMessageW(hwndEdit, EM_GETWORDBREAKPROC, 0, 0);
    ok(fn != NULL, "fn was NULL\n");
    DoWordBreakProc(fn);

    style = (LONG)GetWindowLongPtrW(hwndDropDown, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndDropDown, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndDropDown, GWLP_ID);
    ok_long(style, 0x86800000);
    ok_long(exstyle, 0x8c);
    ok_long((LONG)id, 0);

    style = (LONG)GetClassLongPtrW(hwndDropDown, GCL_STYLE);
    ok(style == 0x20800 /* Win10 */ || style == 0 /* Win2k3 */,
       "style was 0x%08lx\n", style);

    HWND hwndChild;
    WCHAR szClass[64];

    hwndChild = GetTopWindow(hwndDropDown);
    ok(hwndChild != NULL, "hwndChild was NULL\n");
    GetClassNameW(hwndChild, szClass, _countof(szClass));
    ok_wstri(szClass, L"ScrollBar");
    style = (LONG)GetWindowLongPtrW(hwndChild, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndChild, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndDropDown, GWLP_ID);
    ok(style == 0x50000005 || style == 0x40000005, "style was 0x%08lx\n", style);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT);
    ok(hwndChild != NULL, "hwndChild was NULL\n");
    GetClassNameW(hwndChild, szClass, _countof(szClass));
    ok_wstri(szClass, L"ScrollBar");
    style = (LONG)GetWindowLongPtrW(hwndChild, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndChild, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndDropDown, GWLP_ID);
    ok_long(style, 0x5000000c);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT);
    ok(hwndChild != NULL, "hwndChild was NULL\n");
    GetClassNameW(hwndChild, szClass, _countof(szClass));
    ok_wstri(szClass, L"SysListView32");
    style = (LONG)GetWindowLongPtrW(hwndChild, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndChild, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndDropDown, GWLP_ID);
    ok_long(style, 0x54005405);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT);
    ok(hwndChild == NULL, "hwndChild was %p\n", hwndChild);

    PostQuitMessage(0);

    MyMessageLoop();
}

START_TEST(IAutoComplete)
{
    CCoInit init;
    ok_hr(init.hr, S_OK);
    if (!SUCCEEDED(init.hr))
    {
        skip("CoInitialize failed\n");
        return;
    }

    DoTest1();
}
