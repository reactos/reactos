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
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <shellutils.h>

//#define MANUAL_DEBUGGING

// compare wide strings
#define ok_wstri(x, y) \
    ok(lstrcmpiW(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

// create an EDIT control
static HWND MyCreateEditCtrl(INT x, INT y, INT cx, INT cy)
{
    DWORD style = WS_POPUPWINDOW | WS_BORDER;
    DWORD exstyle = WS_EX_CLIENTEDGE;
    return CreateWindowExW(exstyle, L"EDIT", NULL, style, x, y, cx, cy,
                           NULL, NULL, GetModuleHandleW(NULL), NULL);
}

static BOOL s_bReset = FALSE;
static BOOL s_bExpand = FALSE;
static CStringW s_strExpand;

// CEnumString class for auto-completion test
class CEnumString : public IEnumString, public IACList2
{
public:
    CEnumString() : m_cRefs(0), m_nIndex(0), m_nCount(0), m_pList(NULL)
    {
        trace("CEnumString::CEnumString(%p)\n", this);
    }

    virtual ~CEnumString()
    {
        trace("CEnumString::~CEnumString(%p)\n", this);
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
        ++m_nIndex;
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
        s_strExpand = pszExpand;
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

// the testcase A
static VOID
DoTestCaseA(INT x, INT y, INT cx, INT cy, LPCWSTR pszInput,
            LPWSTR *pList, UINT nCount, BOOL bDowner, BOOL bLong)
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

#ifdef MANUAL_DEBUGGING
    trace("enter MANUAL_DEBUGGING...\n");
    trace("NOTE: You can quit EDIT control by Alt+F4.\n");
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindow(hwndEdit))
            break;
    }
    trace("leave MANUAL_DEBUGGING...\n");
    return;
#endif

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
        if (fn2)
            DoWordBreakProc(fn2);
        else
            skip("fn2 == NULL\n");
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
#define DROPDOWN_STYLE (WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | \
                        WS_CLIPCHILDREN | WS_BORDER) // 0x96800000
    ok(style == DROPDOWN_STYLE, "style was 0x%08lx\n", style);
#define DROPDOWN_EX_STYLE (WS_EX_TOOLWINDOW | WS_EX_TOPMOST | \
                           WS_EX_NOPARENTNOTIFY) // 0x8c
    ok_long(exstyle, DROPDOWN_EX_STYLE);
    ok_long((LONG)id, 0);

    // check class style
    style = (LONG)GetClassLongPtrW(hwndDropDown, GCL_STYLE);
#define DROPDOWN_CLASS_STYLE_1 (CS_DROPSHADOW | CS_SAVEBITS)
#define DROPDOWN_CLASS_STYLE_2 0
    ok(style == DROPDOWN_CLASS_STYLE_1 /* Win10 */ ||
       style == DROPDOWN_CLASS_STYLE_2 /* WinXP/Win2k3 */,
       "style was 0x%08lx\n", style);

    // get client rectangle
    RECT rcClient;
    GetClientRect(hwndDropDown, &rcClient);
    trace("rcClient: (%ld, %ld, %ld, %ld)\n",
          rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

    HWND hwndScrollBar, hwndSizeBox, hwndList, hwndNone;
    WCHAR szClass[64];

    // scroll bar
    hwndScrollBar = GetTopWindow(hwndDropDown);
    ok(hwndScrollBar != NULL, "hwndScrollBar was NULL\n");
    GetClassNameW(hwndScrollBar, szClass, _countof(szClass));
    ok_wstri(szClass, L"ScrollBar");
    style = (LONG)GetWindowLongPtrW(hwndScrollBar, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndScrollBar, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndScrollBar, GWLP_ID);
#define SCROLLBAR_STYLE_1 (WS_CHILD | WS_VISIBLE | SBS_BOTTOMALIGN | SBS_VERT) // 0x50000005
#define SCROLLBAR_STYLE_2 (WS_CHILD | SBS_BOTTOMALIGN | SBS_VERT) // 0x40000005
    if (bLong)
        ok(style == SCROLLBAR_STYLE_1, "style was 0x%08lx\n", style);
    else
        ok(style == SCROLLBAR_STYLE_2, "style was 0x%08lx\n", style);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    // size-box
    hwndSizeBox = GetNextWindow(hwndScrollBar, GW_HWNDNEXT);
    ok(hwndSizeBox != NULL, "hwndSizeBox was NULL\n");
    GetClassNameW(hwndSizeBox, szClass, _countof(szClass));
    ok_wstri(szClass, L"ScrollBar");
    style = (LONG)GetWindowLongPtrW(hwndSizeBox, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndSizeBox, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndSizeBox, GWLP_ID);
#define SIZEBOX_STYLE_1 \
    (WS_CHILD | WS_VISIBLE | SBS_SIZEBOX | SBS_SIZEBOXBOTTOMRIGHTALIGN) // 0x5000000c
#define SIZEBOX_STYLE_2 \
    (WS_CHILD | WS_VISIBLE | SBS_SIZEBOX) // 0x50000008
    ok(style == SIZEBOX_STYLE_1 /* Win10 */ ||
       style == SIZEBOX_STYLE_2 /* Win2k3/WinXP */, "style was 0x%08lx\n", style);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);

    // the list
    hwndList = GetNextWindow(hwndSizeBox, GW_HWNDNEXT);
    ok(hwndList != NULL, "hwndList was NULL\n");
    GetClassNameW(hwndList, szClass, _countof(szClass));
    ok_wstri(szClass, WC_LISTVIEWW); // L"SysListView32"
    style = (LONG)GetWindowLongPtrW(hwndList, GWL_STYLE);
    exstyle = (LONG)GetWindowLongPtrW(hwndList, GWL_EXSTYLE);
    id = GetWindowLongPtrW(hwndList, GWLP_ID);
#define LIST_STYLE_1 \
    (WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS | \
     LVS_NOCOLUMNHEADER | LVS_OWNERDATA | LVS_OWNERDRAWFIXED | \
     LVS_SINGLESEL | LVS_REPORT) // 0x54205405
#define LIST_STYLE_2 \
    (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_NOCOLUMNHEADER | \
     LVS_OWNERDATA | LVS_OWNERDRAWFIXED | LVS_SINGLESEL | LVS_REPORT) // 0x54005405
    if (bLong)
        (void)0; // ok(style == LIST_STYLE_1, "style was 0x%08lx\n", style); broken on Windows
    else
        ok(style == LIST_STYLE_2, "style was 0x%08lx\n", style);
    ok_long(exstyle, 0);
    ok_long((LONG)id, 0);
#define LIST_EXTENDED_LV_STYLE_1 \
    (LVS_EX_DOUBLEBUFFER | LVS_EX_ONECLICKACTIVATE | \
     LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT) // 0x10068
#define LIST_EXTENDED_LV_STYLE_2 \
    (LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT | \
     LVS_EX_TRACKSELECT) // 0x68
    exstyle = ListView_GetExtendedListViewStyle(hwndList);
    ok(exstyle == LIST_EXTENDED_LV_STYLE_1 /* Win10 */ ||
       exstyle == LIST_EXTENDED_LV_STYLE_2 /* WinXP/Win2k3 */,
       "exstyle was 0x%08lx\n", exstyle);

    // no more controls
    hwndNone = GetNextWindow(hwndList, GW_HWNDNEXT);
    ok(hwndNone == NULL, "hwndNone was %p\n", hwndNone);

    // get rectangles of controls
    RECT rcScrollBar, rcSizeBox, rcList;
    GetWindowRect(hwndScrollBar, &rcScrollBar);
    MapWindowPoints(NULL, hwndDropDown, (LPPOINT)&rcScrollBar, 2);
    GetWindowRect(hwndSizeBox, &rcSizeBox);
    MapWindowPoints(NULL, hwndDropDown, (LPPOINT)&rcSizeBox, 2);
    GetWindowRect(hwndList, &rcList);
    MapWindowPoints(NULL, hwndDropDown, (LPPOINT)&rcList, 2);
    trace("rcScrollBar: (%ld, %ld, %ld, %ld)\n", rcScrollBar.left, rcScrollBar.top,
          rcScrollBar.right, rcScrollBar.bottom);
    trace("rcSizeBox: (%ld, %ld, %ld, %ld)\n", rcSizeBox.left, rcSizeBox.top,
          rcSizeBox.right, rcSizeBox.bottom);
    trace("rcList: (%ld, %ld, %ld, %ld)\n", rcList.left, rcList.top,
          rcList.right, rcList.bottom);

    // are they visible?
    ok_int(IsWindowVisible(hwndDropDown), TRUE);
    ok_int(IsWindowVisible(hwndEdit), TRUE);
    ok_int(IsWindowVisible(hwndSizeBox), TRUE);
    ok_int(IsWindowVisible(hwndList), TRUE);

    // check item count
    INT nListCount = ListView_GetItemCount(hwndList);
    if (nListCount < 1000)
        (void)0; // ok_int(nListCount, nCount); broken on Windows
    else
        ok_int(nListCount, 1000);

    // check the positions
    if (bDowner) // downer
    {
        ok_int(rcDropDown.left, rcEdit.left);
        ok_int(rcDropDown.top, rcEdit.bottom);
        ok_int(rcDropDown.right, rcEdit.right);
        //ok_int(rcDropDown.bottom, ???);
        ok_int(rcSizeBox.left, rcClient.right - GetSystemMetrics(SM_CXVSCROLL));
        ok_int(rcSizeBox.top, rcClient.bottom - GetSystemMetrics(SM_CYHSCROLL));
        ok_int(rcSizeBox.right, rcClient.right);
        ok_int(rcSizeBox.bottom, rcClient.bottom);
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
        ok_int(rcSizeBox.left, rcClient.right - GetSystemMetrics(SM_CXVSCROLL));
        ok_int(rcSizeBox.top, 0);
        ok_int(rcSizeBox.right, rcClient.right);
        ok_int(rcSizeBox.bottom, rcClient.top + GetSystemMetrics(SM_CYHSCROLL));
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
        Sleep(30); // another thread is working...
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

struct TEST_B_ENTRY
{
    INT m_line;
    UINT m_uMsg;
    WPARAM m_wParam;
    LPARAM m_lParam;
    LPCWSTR m_text;
    BOOL m_bVisible;
    INT m_ich0, m_ich1;
    INT m_iItem;
    BOOL m_bReset, m_bExpand;
    LPCWSTR m_expand;
};

static BOOL
DoesMatch(LPWSTR *pList, UINT nCount, LPCWSTR psz)
{
    INT cch = lstrlenW(psz);
    if (cch == 0)
        return FALSE;
    for (UINT i = 0; i < nCount; ++i)
    {
        if (::StrCmpNIW(pList[i], psz, cch) == 0)
            return TRUE;
    }
    return FALSE;
}

// the testcase B
static VOID
DoTestCaseB(INT x, INT y, INT cx, INT cy, LPWSTR *pList, UINT nCount,
            const TEST_B_ENTRY *pEntries, UINT cEntries)
{
    MSG msg;
    s_bExpand = s_bReset = FALSE;

    // create EDIT control
    HWND hwndEdit = MyCreateEditCtrl(x, y, cx, cy);
    ok(hwndEdit != NULL, "hwndEdit was NULL\n");
    ShowWindowAsync(hwndEdit, SW_SHOWNORMAL);

    // do messages
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ok_int(IsWindowVisible(hwndEdit), TRUE);

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

    HWND hwndDropDown = NULL, hwndScrollBar = NULL, hwndSizeBox = NULL, hwndList = NULL;
    // input
    SetFocus(hwndEdit);
    Sleep(100);

    for (UINT i = 0; i < cEntries; ++i)
    {
        const TEST_B_ENTRY& entry = pEntries[i];
        s_bReset = s_bExpand = FALSE;
        s_strExpand.Empty();

        UINT uMsg = entry.m_uMsg;
        WPARAM wParam = entry.m_wParam;
        LPARAM lParam = entry.m_lParam;
        PostMessageW(hwndEdit, uMsg, wParam, lParam);

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);

            if (msg.message == WM_CHAR || msg.message == WM_KEYDOWN)
                Sleep(100); // another thread is working
        }

        if (!IsWindow(hwndDropDown))
        {
            hwndDropDown = FindWindowW(L"Auto-Suggest Dropdown", L"");
            hwndScrollBar = hwndSizeBox = hwndList = NULL;
        }
        if (IsWindowVisible(hwndDropDown))
        {
            hwndScrollBar = GetTopWindow(hwndDropDown);
            hwndSizeBox = GetNextWindow(hwndScrollBar, GW_HWNDNEXT);
            hwndList = GetNextWindow(hwndSizeBox, GW_HWNDNEXT);
        }

        WCHAR szText[64];
        GetWindowTextW(hwndEdit, szText, _countof(szText));
        CStringW strText = szText;
        INT ich0, ich1;
        SendMessageW(hwndEdit, EM_GETSEL, (WPARAM)&ich0, (LPARAM)&ich1);

        BOOL bVisible = IsWindowVisible(hwndDropDown);
        ok(bVisible == entry.m_bVisible, "Line %d: bVisible was %d\n", entry.m_line, bVisible);
        INT iItem = -1;
        if (IsWindowVisible(hwndList))
        {
            iItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
        }
        BOOL bDidMatch = DoesMatch(pList, nCount, strText);
        ok(bVisible == bDidMatch, "Line %d: bVisible != bDidMatch\n", entry.m_line);

        if (entry.m_text)
            ok(strText == entry.m_text, "Line %d: szText was %S\n", entry.m_line, (LPCWSTR)strText);
        else
            ok(strText == L"", "Line %d: szText was %S\n", entry.m_line, (LPCWSTR)strText);
        ok(ich0 == entry.m_ich0, "Line %d: ich0 was %d\n", entry.m_line, ich0);
        ok(ich1 == entry.m_ich1, "Line %d: ich1 was %d\n", entry.m_line, ich1);
        ok(iItem == entry.m_iItem, "Line %d: iItem was %d\n", entry.m_line, iItem);
        ok(s_bReset == entry.m_bReset, "Line %d: s_bReset was %d\n", entry.m_line, s_bReset);
        ok(s_bExpand == entry.m_bExpand, "Line %d: s_bExpand was %d\n", entry.m_line, s_bExpand);
        if (entry.m_expand)
            ok(s_strExpand == entry.m_expand, "Line %d: s_strExpand was %S\n", entry.m_line, (LPCWSTR)s_strExpand);
        else
            ok(s_strExpand == L"", "Line %d: s_strExpand was %S\n", entry.m_line, (LPCWSTR)s_strExpand);
    }

    DestroyWindow(hwndEdit);
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
    HMONITOR hMon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);
    const RECT& rcWork = mi.rcWork;
    trace("rcWork: (%ld, %ld, %ld, %ld)\n",
          rcWork.left, rcWork.top, rcWork.right, rcWork.bottom);
    trace("SM_CXVSCROLL: %d, SM_CYHSCROLL: %d\n",
          GetSystemMetrics(SM_CXVSCROLL), GetSystemMetrics(SM_CYHSCROLL));

    UINT nCount;
    LPWSTR *pList;
    WCHAR szText[64];

    // Test case #1 (A)
    trace("Testcase #1 (downer, short) ------------------------------\n");
    nCount = 3;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    SHStrDupW(L"test\\AA", &pList[0]);
    SHStrDupW(L"test\\BBB", &pList[1]);
    SHStrDupW(L"test\\CCC", &pList[2]);
    DoTestCaseA(0, 0, 100, 30, L"test\\", pList, nCount, TRUE, FALSE);

    // Test case #2 (A)
    trace("Testcase #2 (downer, long) ------------------------------\n");
    nCount = 300;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"test\\%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    DoTestCaseA(100, 20, 100, 30, L"test\\", pList, nCount, TRUE, TRUE);

    // Test case #3 (A)
    trace("Testcase #3 (upper, short) ------------------------------\n");
    nCount = 2;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    SHStrDupW(L"test/AA", &pList[0]);
    SHStrDupW(L"test/BBB", &pList[0]);
    SHStrDupW(L"test/CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", &pList[1]);
    DoTestCaseA(rcWork.right - 100, rcWork.bottom - 30, 80, 40, L"test/",
                pList, nCount, FALSE, FALSE);

    // Test case #4 (A)
    trace("Testcase #4 (upper, short) ------------------------------\n");
    nCount = 2;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    SHStrDupW(L"testtest\\AA", &pList[0]);
    SHStrDupW(L"testtest\\BBB", &pList[0]);
    SHStrDupW(L"testtest\\CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", &pList[1]);
    DoTestCaseA(rcWork.right - 100, rcWork.bottom - 30, 80, 40, L"testtest\\",
                pList, nCount, FALSE, FALSE);

#ifdef MANUAL_DEBUGGING
    return;
#endif

    // Test case #5 (A)
    trace("Testcase #5 (upper, long) ------------------------------\n");
    nCount = 300;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"testtest/%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    DoTestCaseA(0, rcWork.bottom - 30, 80, 30, L"testtest/", pList, nCount, FALSE, TRUE);

    // Test case #6 (A)
    trace("Testcase #6 (upper, long) ------------------------------\n");
    nCount = 2000;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"testtest\\item-%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    DoTestCaseA(0, rcWork.bottom - 30, 80, 40, L"testtest\\", pList, nCount, FALSE, TRUE);

    // Test case #7 (B)
    trace("Testcase #7 ------------------------------\n");
    nCount = 16;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"test\\item-%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    static const TEST_B_ENTRY testcase7_entries[] =
    {
        { __LINE__, WM_CHAR, L't', 0, L"t", TRUE, 1, 1, -1, TRUE },
        { __LINE__, WM_CHAR, L'e', 0, L"te", TRUE, 2, 2, -1 },
        { __LINE__, WM_CHAR, L's', 0, L"tes", TRUE, 3, 3, -1 },
        { __LINE__, WM_CHAR, L'T', 0, L"tesT", TRUE, 4, 4, -1 },
        { __LINE__, WM_CHAR, L'\\', 0, L"tesT\\", TRUE, 5, 5, -1, TRUE, TRUE, L"tesT\\" },
        { __LINE__, WM_CHAR, L't', 0, L"tesT\\t", FALSE, 6, 6, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"tesT\\", TRUE, 5, 5, -1 },
        { __LINE__, WM_CHAR, L'i', 0, L"tesT\\i", TRUE, 6, 6, -1 },
        { __LINE__, WM_KEYDOWN, VK_DOWN, 0, L"test\\item-0", TRUE, 11, 11, 0 },
        { __LINE__, WM_KEYDOWN, VK_DOWN, 0, L"test\\item-1", TRUE, 11, 11, 1 },
        { __LINE__, WM_KEYDOWN, VK_UP, 0, L"test\\item-0", TRUE, 11, 11, 0 },
        { __LINE__, WM_KEYDOWN, VK_UP, 0, L"tesT\\i", TRUE, 6, 6, -1 },
        { __LINE__, WM_KEYDOWN, VK_UP, 0, L"test\\item-9", TRUE, 11, 11, 15 },
        { __LINE__, WM_KEYDOWN, VK_DOWN, 0, L"tesT\\i", TRUE, 6, 6, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"tesT\\", TRUE, 5, 5, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"tesT", TRUE, 4, 4, -1, TRUE },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"tes", TRUE, 3, 3, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"te", TRUE, 2, 2, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"t", TRUE, 1, 1, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"", FALSE, 0, 0, -1 },
        { __LINE__, WM_CHAR, L't', 0, L"t", TRUE, 1, 1, -1, TRUE },
        { __LINE__, WM_KEYDOWN, VK_LEFT, 0, L"t", TRUE, 0, 0, -1 },
        { __LINE__, WM_KEYDOWN, VK_RIGHT, 0, L"t", TRUE, 1, 1, -1 },
    };
    DoTestCaseB(0, 0, 100, 30, pList, nCount, testcase7_entries, _countof(testcase7_entries));

    // Test case #8 (B)
    trace("Testcase #8 ------------------------------\n");
    nCount = 10;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"test\\item-%u", i);
        SHStrDupW(szText, &pList[i]);
    }
    static const TEST_B_ENTRY testcase8_entries[] =
    {
        { __LINE__, WM_CHAR, L'a', 0, L"a", FALSE, 1, 1, -1, TRUE },
        { __LINE__, WM_CHAR, L'b', 0, L"ab", FALSE, 2, 2, -1 },
        { __LINE__, WM_KEYDOWN, VK_LEFT, 0, L"ab", FALSE, 1, 1, -1 },
        { __LINE__, WM_KEYDOWN, VK_LEFT, 0, L"ab", FALSE, 0, 0, -1 },
        { __LINE__, WM_KEYDOWN, VK_DELETE, 0, L"b", FALSE, 0, 0, -1, TRUE },
        { __LINE__, WM_CHAR, L't', 0, L"tb", FALSE, 1, 1, -1, TRUE },
        { __LINE__, WM_CHAR, L'e', 0, L"teb", FALSE, 2, 2, -1 },
        { __LINE__, WM_CHAR, L's', 0, L"tesb", FALSE, 3, 3, -1 },
        { __LINE__, WM_CHAR, L't', 0, L"testb", FALSE, 4, 4, -1 },
        { __LINE__, WM_KEYDOWN, VK_DELETE, 0, L"test", TRUE, 4, 4, -1 },
        { __LINE__, WM_CHAR, L'\\', 0, L"test\\", TRUE, 5, 5, -1, TRUE, TRUE, L"test\\" },
        { __LINE__, WM_CHAR, L'i', 0, L"test\\i", TRUE, 6, 6, -1 },
        { __LINE__, WM_KEYDOWN, VK_DELETE, 0, L"test\\i", TRUE, 6, 6, -1 },
        { __LINE__, WM_CHAR, L'z', 0, L"test\\iz", FALSE, 7, 7, -1 },
        { __LINE__, WM_KEYDOWN, VK_LEFT, 0, L"test\\iz", FALSE, 6, 6, -1 },
        { __LINE__, WM_KEYDOWN, VK_LEFT, 0, L"test\\iz", FALSE, 5, 5, -1 },
        { __LINE__, WM_KEYDOWN, VK_DELETE, 0, L"test\\z", FALSE, 5, 5, -1 },
        { __LINE__, WM_KEYDOWN, VK_DELETE, 0, L"test\\", TRUE, 5, 5, -1 },
        { __LINE__, WM_KEYDOWN, VK_LEFT, 0, L"test\\", TRUE, 4, 4, -1 },
        { __LINE__, WM_KEYDOWN, VK_LEFT, 0, L"test\\", TRUE, 3, 3, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"tet\\", FALSE, 2, 2, -1, TRUE, TRUE, L"tet\\" },
        { __LINE__, WM_KEYDOWN, VK_DELETE, 0, L"te\\", FALSE, 2, 2, -1, TRUE, TRUE, L"te\\" },
        { __LINE__, WM_KEYDOWN, VK_DELETE, 0, L"te", TRUE, 2, 2, -1, TRUE },
    };
    DoTestCaseB(rcWork.right - 100, rcWork.bottom - 30, 80, 40, pList, nCount, testcase8_entries, _countof(testcase8_entries));

    // Test case #9 (B)
    trace("Testcase #9 ------------------------------\n");
    nCount = 32;
    pList = (LPWSTR *)CoTaskMemAlloc(nCount * sizeof(LPWSTR));
    for (UINT i = 0; i < nCount; ++i)
    {
        StringCbPrintfW(szText, sizeof(szText), L"a%u\\b%u\\c%u", i % 2, (i / 2) % 2, i % 4);
        SHStrDupW(szText, &pList[i]);
    }
    static const TEST_B_ENTRY testcase9_entries[] =
    {
        { __LINE__, WM_CHAR, L'a', 0, L"a", TRUE, 1, 1, -1, TRUE },
        { __LINE__, WM_CHAR, L'0', 0, L"a0", TRUE, 2, 2, -1 },
        { __LINE__, WM_CHAR, L'\\', 0, L"a0\\", TRUE, 3, 3, -1, TRUE, TRUE, L"a0\\" },
        { __LINE__, WM_CHAR, L'b', 0, L"a0\\b", TRUE, 4, 4, -1 },
        { __LINE__, WM_CHAR, L'1', 0, L"a0\\b1", TRUE, 5, 5, -1 },
        { __LINE__, WM_CHAR, L'\\', 0, L"a0\\b1\\", TRUE, 6, 6, -1, TRUE, TRUE, L"a0\\b1\\" },
        { __LINE__, WM_CHAR, L'c', 0, L"a0\\b1\\c", TRUE, 7, 7, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"a0\\b1\\", TRUE, 6, 6, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"a0\\b1", TRUE, 5, 5, -1, TRUE, TRUE, L"a0\\" },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"a0\\b", TRUE, 4, 4, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"a0\\", TRUE, 3, 3, -1 },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"a0", TRUE, 2, 2, -1, TRUE },
        { __LINE__, WM_KEYDOWN, VK_BACK, 0, L"a", TRUE, 1, 1, -1 },
        { __LINE__, WM_KEYDOWN, VK_DOWN, 0, L"a0\\b0\\c0", TRUE, 8, 8, 0 },
        { __LINE__, WM_KEYDOWN, VK_UP, 0, L"a", TRUE, 1, 1, -1 },
    };
    DoTestCaseB(0, 0, 100, 30, pList, nCount, testcase9_entries, _countof(testcase9_entries));
}
