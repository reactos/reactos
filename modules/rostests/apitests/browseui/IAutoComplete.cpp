/*
 * PROJECT:   ReactOS api tests
 * LICENSE:   GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:   Test for IAutoComplete objects
 * COPYRIGHT: Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#define _UNICODE
#define UNICODE
#include <apitest.h>
#include <shlobj.h>
#include <atlbase.h>
#include <tchar.h>
#include <atlcom.h>
#include <atlwin.h>
#include <shlwapi.h>
#include <strsafe.h>

// compare wide strings
#define ok_wstri(x, y) \
    ok(lstrcmpiW(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

struct CCoInit
{
    CCoInit() { hr = CoInitialize(NULL); }
    ~CCoInit() { if (SUCCEEDED(hr)) { CoUninitialize(); } }
    HRESULT hr;
};

// create an EDIT control
static HWND MyCreateEditCtrl(INT x, INT y, INT cx, INT cy)
{
    return CreateWindowW(L"EDIT", NULL, WS_POPUPWINDOW, x, y, cx, cy,
                         NULL, NULL, GetModuleHandleW(NULL), NULL);
}

static BOOL s_bReset = FALSE;
static BOOL s_bExpand = FALSE;

// CEnumString class for auto-completion test
class CEnumString : public IEnumString, public IACList2
{
public:
    CEnumString() : m_cRefs(0), m_nIndex(0), m_nCount(0), m_pList(NULL)
    {
    }

    virtual ~CEnumString()
    {
        trace("CEnumString::~CEnumString\n");
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
            trace("IID_IEnumString\n");
            AddRef();
            *ppv = static_cast<IEnumString *>(this);
            return S_OK;
        }
        if (iid == IID_IACList || iid == IID_IACList2)
        {
            trace("IID_IACList2\n");
            AddRef();
            *ppv = static_cast<IACList2 *>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        //trace("CEnumString::AddRef\n");
        ++m_cRefs;
        return m_cRefs;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        //trace("CEnumString::Release\n");
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

        SHStrDupW(m_pList[m_nIndex], rgelt);
        if (!*rgelt)
            return E_OUTOFMEMORY;
        *pceltFetched = 1;
        return S_OK;
    }
    STDMETHODIMP Skip(ULONG celt) override
    {
        trace("CEnumString::Skip(%lu)\n", celt);
        return E_NOTIMPL;
    }
    STDMETHODIMP Reset() override
    {
        trace("CEnumString::Reset\n");
        m_nIndex = 0;
        s_bReset = TRUE;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumString** ppenum) override
    {
        trace("CEnumString::Clone()\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP Expand(PCWSTR pszExpand) override
    {
        trace("CEnumString::Expand(%S)\n", pszExpand);
        s_bExpand = TRUE;
        return S_OK;
    }
    STDMETHODIMP GetOptions(DWORD *pdwFlag) override
    {
        trace("CEnumString::GetOption(%p)\n", pdwFlag);
        return S_OK;
    }
    STDMETHODIMP SetOptions(DWORD dwFlag) override
    {
        trace("CEnumString::SetOption(0x%lX)\n", dwFlag);
        return S_OK;
    }

protected:
    ULONG m_cRefs;
    UINT m_nIndex, m_nCount;
    LPWSTR *m_pList;
};

// range of WCHAR (inclusive)
struct RANGE
{
    WCHAR from, to;
};

//#define OUTPUT_TABLE // generate the table to analyze

#ifndef OUTPUT_TABLE
// comparison of two ranges
static int __cdecl RangeCompare(const void *x, const void *y)
{
    const RANGE *a = (const RANGE *)x;
    const RANGE *b = (const RANGE *)y;
    if (a->to < b->from)
        return -1;
    if (b->to < a->from)
        return 1;
    return 0;
}

// is the WCHAR a word break?
static __inline BOOL IsWordBreak(WCHAR ch)
{
    static const RANGE s_ranges[] =
    {
        { 0x0009, 0x0009 }, { 0x0020, 0x002f }, { 0x003a, 0x0040 }, { 0x005b, 0x0060 },
        { 0x007b, 0x007e }, { 0x00ab, 0x00ab }, { 0x00ad, 0x00ad }, { 0x00bb, 0x00bb },
        { 0x02c7, 0x02c7 }, { 0x02c9, 0x02c9 }, { 0x055d, 0x055d }, { 0x060c, 0x060c },
        { 0x2002, 0x200b }, { 0x2013, 0x2014 }, { 0x2016, 0x2016 }, { 0x2018, 0x2018 },
        { 0x201c, 0x201d }, { 0x2022, 0x2022 }, { 0x2025, 0x2027 }, { 0x2039, 0x203a },
        { 0x2045, 0x2046 }, { 0x207d, 0x207e }, { 0x208d, 0x208e }, { 0x226a, 0x226b },
        { 0x2574, 0x2574 }, { 0x3001, 0x3003 }, { 0x3005, 0x3005 }, { 0x3008, 0x3011 },
        { 0x3014, 0x301b }, { 0x301d, 0x301e }, { 0x3041, 0x3041 }, { 0x3043, 0x3043 },
        { 0x3045, 0x3045 }, { 0x3047, 0x3047 }, { 0x3049, 0x3049 }, { 0x3063, 0x3063 },
        { 0x3083, 0x3083 }, { 0x3085, 0x3085 }, { 0x3087, 0x3087 }, { 0x308e, 0x308e },
        { 0x309b, 0x309e }, { 0x30a1, 0x30a1 }, { 0x30a3, 0x30a3 }, { 0x30a5, 0x30a5 },
        { 0x30a7, 0x30a7 }, { 0x30a9, 0x30a9 }, { 0x30c3, 0x30c3 }, { 0x30e3, 0x30e3 },
        { 0x30e5, 0x30e5 }, { 0x30e7, 0x30e7 }, { 0x30ee, 0x30ee }, { 0x30f5, 0x30f6 },
        { 0x30fc, 0x30fe }, { 0xfd3e, 0xfd3f }, { 0xfe30, 0xfe31 }, { 0xfe33, 0xfe44 },
        { 0xfe4f, 0xfe51 }, { 0xfe59, 0xfe5e }, { 0xff08, 0xff09 }, { 0xff0c, 0xff0c },
        { 0xff0e, 0xff0e }, { 0xff1c, 0xff1c }, { 0xff1e, 0xff1e }, { 0xff3b, 0xff3b },
        { 0xff3d, 0xff3d }, { 0xff40, 0xff40 }, { 0xff5b, 0xff5e }, { 0xff61, 0xff64 },
        { 0xff67, 0xff70 }, { 0xff9e, 0xff9f }, { 0xffe9, 0xffe9 }, { 0xffeb, 0xffeb },
    };
#ifndef NDEBUG
    // check the table if first time
    static BOOL s_bFirstTime = TRUE;
    if (s_bFirstTime)
    {
        s_bFirstTime = FALSE;
        for (UINT i = 0; i < _countof(s_ranges); ++i)
        {
            ATLASSERT(s_ranges[i].from <= s_ranges[i].to);
        }
        for (UINT i = 0; i + 1 < _countof(s_ranges); ++i)
        {
            ATLASSERT(s_ranges[i].to < s_ranges[i + 1].from);
        }
    }
#endif
    RANGE range = { ch, ch };
    return !!bsearch(&range, s_ranges, _countof(s_ranges), sizeof(RANGE), RangeCompare);
}
#endif

static VOID DoWordBreakProc(EDITWORDBREAKPROC fn)
{
#ifdef OUTPUT_TABLE
    // generate the table text
    WORD wType1, wType2, wType3;
    for (DWORD i = 0; i <= 0xFFFF; ++i)
    {
        WCHAR ch = (WCHAR)i;
        GetStringTypeW(CT_CTYPE1, &ch, 1, &wType1);
        GetStringTypeW(CT_CTYPE2, &ch, 1, &wType2);
        GetStringTypeW(CT_CTYPE3, &ch, 1, &wType3);
        BOOL b = fn(&ch, 0, 1, WB_ISDELIMITER);
        trace("%u\t0x%04x\t0x%04x\t0x%04x\t0x%04x\n", b, wType1, wType2, wType3, ch);
    }
#else
    // check the word break procedure
    for (DWORD i = 0; i <= 0xFFFF; ++i)
    {
        WCHAR ch = (WCHAR)i;
        BOOL b1 = fn(&ch, 0, 1, WB_ISDELIMITER);
        BOOL b2 = IsWordBreak(ch);
        ok(b1 == b2, "ch:0x%04x, b1:%d, b2:%d\n", ch, b1, b2);
    }
#endif
}

// the testcase
static VOID
DoTestCase(INT x, INT y, INT cx, INT cy, LPCWSTR pszInput,
           LPWSTR *pList, UINT nCount, BOOL bDowner)
{
    MSG msg;
    s_bExpand = s_bReset = FALSE;

    // create EDIT control
    HWND hwndEdit = MyCreateEditCtrl(x, y, cx, cy);
    ok(hwndEdit != NULL, "hwndEdit was NULL\n");
    ShowWindowAsync(hwndEdit, SW_SHOWNORMAL);

    // get word break procedure
    EDITWORDBREAKPROC fn1 =
        (EDITWORDBREAKPROC)SendMessageW(hwndEdit, EM_GETWORDBREAKPROC, 0, 0);
    ok(fn1 == NULL, "fn1 was %p\n", fn1);

    // set the list data
    CComPtr<CEnumString> pEnum(new CEnumString());
    pEnum->SetList(nCount, pList);

    // create auto-completion object
    CComPtr<IAutoComplete2> pAC;
    HRESULT hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IAutoComplete2, (VOID **)&pAC);
    ok_hr(hr, S_OK);

    // enable auto-suggest
    hr = pAC->SetOptions(ACO_AUTOSUGGEST);
    ok_hr(hr, S_OK);

    // initialize
    IUnknown *punk = static_cast<IEnumString *>(pEnum);
    hr = pAC->Init(hwndEdit, punk, NULL, NULL); // IAutoComplete::Init
    ok_hr(hr, S_OK);

    // check expansion
    ok_int(s_bExpand, FALSE);
    // check reset
    ok_int(s_bReset, FALSE);

    // input
    SetFocus(hwndEdit);
    WCHAR chLast = 0;
    for (UINT i = 0; pszInput[i]; ++i)
    {
        PostMessageW(hwndEdit, WM_CHAR, pszInput[i], 0);
        chLast = pszInput[i];
    }

    // wait for hwndDropDown
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
        if (IsWindowVisible(hwndDropDown))
            break;
        Sleep(100);
    }
    ok(hwndDropDown != NULL, "hwndDropDown was NULL\n");
    ok_int(IsWindowVisible(hwndDropDown), TRUE);

    // check word break procedure
    static BOOL s_bFirstTime = TRUE;
    if (s_bFirstTime)
    {
        s_bFirstTime = FALSE;
        EDITWORDBREAKPROC fn2 =
            (EDITWORDBREAKPROC)SendMessageW(hwndEdit, EM_GETWORDBREAKPROC, 0, 0);
        ok(fn1 != fn2, "fn1 == fn2\n");
        ok(fn2 != NULL, "fn2 was NULL\n");
        DoWordBreakProc(fn2);
    }

    // take care of the message queue
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // check reset
    ok_int(s_bReset, TRUE);

    // get sizes and positions
    RECT rcEdit, rcDropDown;
    GetWindowRect(hwndEdit, &rcEdit);
    GetWindowRect(hwndDropDown, &rcDropDown);
    trace("rcEdit: (%ld, %ld, %ld, %ld)\n", rcEdit.left, rcEdit.top, rcEdit.right, rcEdit.bottom);
    trace("rcDropDown: (%ld, %ld, %ld, %ld)\n", rcDropDown.left, rcDropDown.top, rcDropDown.right, rcDropDown.bottom);

    // is it "downer"?
    ok_int(bDowner, rcEdit.top < rcDropDown.top);

    // check window style and id
    style = (LONG)GetWindowLongPtrW(hwndDropDown, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndDropDown, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndDropDown, GWLP_ID);
    ok(style == 0x86800000 || style == 0x96800000, "style was 0x%08lx\n", style);
    ok_long(exstyle, 0x8c);
    ok_long((LONG)id, 0);

    // check class style
    style = (LONG)GetClassLongPtrW(hwndDropDown, GCL_STYLE);
    ok(style == 0x20800 /* Win10 */ || style == 0 /* Win2k3 */,
       "style was 0x%08lx\n", style);

    // get client rectangle
    RECT rcClient;
    GetClientRect(hwndDropDown, &rcClient);
    trace("rcClient: (%ld, %ld, %ld, %ld)\n",
          rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

    HWND hwndScrollBar, hwndSizeGrip, hwndList, hwndNone;
    WCHAR szClass[64];

    // scroll bar
    hwndScrollBar = GetTopWindow(hwndDropDown);
    ok(hwndScrollBar != NULL, "hwndScrollBar was NULL\n");
    GetClassNameW(hwndScrollBar, szClass, _countof(szClass));
    ok_wstri(szClass, L"ScrollBar");
    style = (LONG)GetWindowLongPtrW(hwndScrollBar, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndScrollBar, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndScrollBar, GWLP_ID);
    ok(style == 0x50000005 || style == 0x40000005, "style was 0x%08lx\n", style);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    // size-grip
    hwndSizeGrip = GetNextWindow(hwndScrollBar, GW_HWNDNEXT);
    ok(hwndSizeGrip != NULL, "hwndSizeGrip was NULL\n");
    GetClassNameW(hwndSizeGrip, szClass, _countof(szClass));
    ok_wstri(szClass, L"ScrollBar");
    style = (LONG)GetWindowLongPtrW(hwndSizeGrip, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndSizeGrip, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndSizeGrip, GWLP_ID);
    ok(style == 0x5000000c || style == 0x50000008,
       "style was 0x%08lx\n", style);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    // the list
    hwndList = GetNextWindow(hwndSizeGrip, GW_HWNDNEXT);
    ok(hwndList != NULL, "hwndList was NULL\n");
    GetClassNameW(hwndList, szClass, _countof(szClass));
    ok_wstri(szClass, L"SysListView32");
    style = (LONG)GetWindowLongPtrW(hwndList, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndList, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndList, GWLP_ID);
    ok_long(style, 0x54005405);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    // no more controls
    hwndNone = GetNextWindow(hwndList, GW_HWNDNEXT);
    ok(hwndNone == NULL, "hwndNone was %p\n", hwndNone);

    // get rectangles of controls
    RECT rcScrollBar, rcSizeGrip, rcList;
    GetWindowRect(hwndScrollBar, &rcScrollBar);
    MapWindowPoints(NULL, hwndDropDown, (LPPOINT)&rcScrollBar, 2);
    GetWindowRect(hwndSizeGrip, &rcSizeGrip);
    MapWindowPoints(NULL, hwndDropDown, (LPPOINT)&rcSizeGrip, 2);
    GetWindowRect(hwndList, &rcList);
    MapWindowPoints(NULL, hwndDropDown, (LPPOINT)&rcList, 2);
    trace("rcScrollBar: (%ld, %ld, %ld, %ld)\n", rcScrollBar.left, rcScrollBar.top,
          rcScrollBar.right, rcScrollBar.bottom);
    trace("rcSizeGrip: (%ld, %ld, %ld, %ld)\n", rcSizeGrip.left, rcSizeGrip.top,
          rcSizeGrip.right, rcSizeGrip.bottom);
    trace("rcList: (%ld, %ld, %ld, %ld)\n", rcList.left, rcList.top,
          rcList.right, rcList.bottom);

    // are they visible?
    ok_int(IsWindowVisible(hwndDropDown), TRUE);
    ok_int(IsWindowVisible(hwndEdit), TRUE);
    ok_int(IsWindowVisible(hwndSizeGrip), TRUE);
    ok_int(IsWindowVisible(hwndList), TRUE);

    // check the positions
    if (bDowner) // downer
    {
        ok_int(rcDropDown.left, rcEdit.left);
        ok_int(rcDropDown.top, rcEdit.bottom);
        ok_int(rcDropDown.right, rcEdit.right);
        //ok_int(rcDropDown.bottom, ???);
        ok_int(rcSizeGrip.left, rcClient.right - GetSystemMetrics(SM_CXVSCROLL));
        ok_int(rcSizeGrip.top, rcClient.bottom - GetSystemMetrics(SM_CYHSCROLL));
        ok_int(rcSizeGrip.right, rcClient.right);
        ok_int(rcSizeGrip.bottom, rcClient.bottom);
        ok_int(rcScrollBar.left, rcClient.right - GetSystemMetrics(SM_CXVSCROLL));
        ok_int(rcScrollBar.top, 0);
        ok_int(rcScrollBar.right, rcClient.right);
        ok_int(rcScrollBar.bottom, rcClient.bottom - GetSystemMetrics(SM_CYHSCROLL));
        ok_int(rcList.left, 0);
        ok_int(rcList.top, 0);
        //ok_int(rcList.right, 30160 or 30170???);
        ok_int(rcList.bottom, rcClient.bottom);
    }
    else // upper
    {
        ok_int(rcDropDown.left, rcEdit.left);
        //ok_int(rcDropDown.top, ???);
        ok_int(rcDropDown.right, rcEdit.right);
        ok_int(rcDropDown.bottom, rcEdit.top);
        ok_int(rcSizeGrip.left, rcClient.right - GetSystemMetrics(SM_CXVSCROLL));
        ok_int(rcSizeGrip.top, 0);
        ok_int(rcSizeGrip.right, rcClient.right);
        ok_int(rcSizeGrip.bottom, rcClient.top + GetSystemMetrics(SM_CYHSCROLL));
        ok_int(rcScrollBar.left, rcClient.right - GetSystemMetrics(SM_CXVSCROLL));
        ok_int(rcScrollBar.top, GetSystemMetrics(SM_CYHSCROLL));
        ok_int(rcScrollBar.right, rcClient.right);
        ok_int(rcScrollBar.bottom, rcClient.bottom);
        ok_int(rcList.left, 0);
        ok_int(rcList.top, 0);
        //ok_int(rcList.right, 30160 or 30170???);
        ok_int(rcList.bottom, rcClient.bottom);
    }

    // append WM_QUIT message into message queue
    PostQuitMessage(0);

    // do the messages
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        Sleep(30);
    }

    // destroy the EDIT control and drop-down window
    DestroyWindow(hwndEdit);
    DestroyWindow(hwndDropDown);

    // do the messages
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // check the expansion
    ok_int(s_bExpand, chLast == L'\\');
}

START_TEST(IAutoComplete)
{
    // initialize COM
    CCoInit init;
    ok_hr(init.hr, S_OK);
    if (!SUCCEEDED(init.hr))
    {
        skip("CoInitialize failed\n");
        return;
    }

    // get screen size
    // TODO: multimonitor
    INT cxScreen = GetSystemMetrics(SM_CXSCREEN);
    INT cyScreen = GetSystemMetrics(SM_CYSCREEN);
    trace("SM_CXSCREEN: %d, SM_CYSCREEN: %d\n", cxScreen, cyScreen);
    trace("SM_CXVSCROLL: %d, SM_CYHSCROLL: %d\n",
          GetSystemMetrics(SM_CXVSCROLL), GetSystemMetrics(SM_CYHSCROLL));

    UINT nCount;
    LPWSTR *pList;
    WCHAR szText[64];

    // Test case #1
    trace("Testcase #1 (downer) ------------------------------\n");
    nCount = 2;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    SHStrDupW(L"test\\AA", &pList[0]);
    SHStrDupW(L"test\\BBB", &pList[1]);
    DoTestCase(0, 0, 100, 16, L"test\\", pList, nCount, TRUE);

    // Test case #2
    trace("Testcase #2 (downer) ------------------------------\n");
    nCount = 300;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"test\\%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    DoTestCase(100, 20, 100, 16, L"test\\", pList, nCount, TRUE);

    // Test case #3
    trace("Testcase #3 (upper) ------------------------------\n");
    nCount = 2;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    SHStrDupW(L"test/AA", &pList[0]);
    SHStrDupW(L"test/BBB", &pList[0]);
    SHStrDupW(L"test/CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", &pList[1]);
    DoTestCase(cxScreen - 100, cyScreen - 30, 80, 18, L"test/",
               pList, nCount, FALSE);

    // Test case #4
    trace("Testcase #4 (upper) ------------------------------\n");
    nCount = 2;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    SHStrDupW(L"testtest\\AA", &pList[0]);
    SHStrDupW(L"testtest\\BBB", &pList[0]);
    SHStrDupW(L"testtest\\CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", &pList[1]);
    DoTestCase(cxScreen - 100, cyScreen - 30, 80, 18, L"testtest\\",
               pList, nCount, FALSE);

    // Test case #5
    trace("Testcase #5 (upper) ------------------------------\n");
    nCount = 300;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"testtest/%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    DoTestCase(0, cyScreen - 30, 80, 18, L"testtest/", pList, nCount, FALSE);

    // Test case #6
    trace("Testcase #6 (upper) ------------------------------\n");
    nCount = 300;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"testtest\\item-%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    DoTestCase(0, cyScreen - 30, 80, 18, L"testtest\\", pList, nCount, FALSE);
}
