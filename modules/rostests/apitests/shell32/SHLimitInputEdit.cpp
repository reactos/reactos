/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SHLimitInputEdit
 * PROGRAMMER:      Katayama Hirofumi MZ
 */

#include "shelltest.h"
#include <shobjidl.h>
#include <shlwapi.h>

class CShellFolder
    : public IShellFolder
    , public IItemNameLimits
{
public:
    LONG m_cRefs;

    CShellFolder(INT iMaxNameLen = 0, BOOL bDisabled = FALSE)
        : m_cRefs(1)
        , m_iMaxNameLen(iMaxNameLen)
        , m_bDisabled(bDisabled)
    {
        trace("CShellFolder\n");
    }

    virtual ~CShellFolder()
    {
        trace("~CShellFolder\n");
    }

    /*** IUnknown methods ***/

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /*** IShellFolder methods ***/

    STDMETHODIMP ParseDisplayName(
        HWND hwnd,
        IBindCtx *pbc,
        LPWSTR pszDisplayName,
        ULONG *pchEaten,
        PIDLIST_RELATIVE *ppidl,
        ULONG *pdwAttributes)
    {
        trace("ParseDisplayName\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP EnumObjects(
        HWND hwnd,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList)
    {
        trace("EnumObjects\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP BindToObject(
        PCUIDLIST_RELATIVE pidl,
        IBindCtx *pbc,
        REFIID riid,
        void **ppv)
    {
        trace("BindToObject\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP BindToStorage(
        PCUIDLIST_RELATIVE pidl,
        IBindCtx *pbc,
        REFIID riid,
        void **ppv)
    {
        trace("BindToStorage\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP CompareIDs(
        LPARAM lParam,
        PCUIDLIST_RELATIVE pidl1,
        PCUIDLIST_RELATIVE pidl2)
    {
        trace("CompareIDs\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP CreateViewObject(
        HWND hwndOwner,
        REFIID riid,
        void **ppv)
    {
        trace("CreateViewObject\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP GetAttributesOf(
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        SFGAOF *rgfInOut)
    {
        trace("GetAttributesOf\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP GetUIObjectOf(
        HWND hwndOwner,
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid,
        UINT *rgfReserved,
        void **ppv)
    {
        trace("GetUIObjectOf\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP GetDisplayNameOf(
        PCUITEMID_CHILD pidl,
        SHGDNF uFlags,
        STRRET *pName)
    {
        trace("GetDisplayNameOf\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP SetNameOf(
        HWND hwnd,
        PCUITEMID_CHILD pidl,
        LPCWSTR pszName,
        SHGDNF uFlags,
        PITEMID_CHILD *ppidlOut)
    {
        trace("SetNameOf\n");
        return E_NOTIMPL;
    }

    /*** IItemNameLimits methods ***/

    STDMETHODIMP GetMaxLength(
        LPCWSTR pszName,
        int *piMaxNameLen);

    STDMETHODIMP GetValidCharacters(
        LPWSTR *ppwszValidChars,
        LPWSTR *ppwszInvalidChars);

protected:
    INT m_iMaxNameLen;
    BOOL m_bDisabled;
};

STDMETHODIMP CShellFolder::QueryInterface(REFIID riid, void **ppvObject)
{
    trace("QueryInterface\n");

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IUnknown))
    {
        trace("IID_IShellFolder\n");
        *ppvObject = static_cast<IShellFolder *>(this);
        AddRef();
        return S_OK;
    }
    if (IsEqualIID(riid, IID_IItemNameLimits))
    {
        trace("IID_IItemNameLimits\n");
        *ppvObject = static_cast<IItemNameLimits *>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellFolder::AddRef()
{
    trace("AddRef\n");
    return ++m_cRefs;
}

STDMETHODIMP_(ULONG) CShellFolder::Release()
{
    trace("Release\n");

    if (--m_cRefs == 0)
    {
        //delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP CShellFolder::GetMaxLength(
    LPCWSTR pszName,
    int *piMaxNameLen)
{
    trace("GetMaxLength('%S', %p (%d))\n", pszName, piMaxNameLen, *piMaxNameLen);

    if (!piMaxNameLen)
        return E_POINTER;

    *piMaxNameLen = m_iMaxNameLen;
    return S_OK;
}

STDMETHODIMP CShellFolder::GetValidCharacters(
    LPWSTR *ppwszValidChars,
    LPWSTR *ppwszInvalidChars)
{
    trace("GetValidCharacters(%p, %p)\n", ppwszValidChars, ppwszInvalidChars);

    if (m_bDisabled)
        return E_NOTIMPL;

    if (ppwszInvalidChars)
    {
        SHStrDupW(L"/:*?\"<>|", ppwszInvalidChars);
    }
    if (ppwszValidChars)
    {
        *ppwszValidChars = NULL;
    }
    return S_OK;
}

static BOOL CALLBACK
PropEnumProc(
    HWND hwnd,
    LPCWSTR lpszString,
    HANDLE hData)
{
    trace("PropEnumProc: '%S' --> %p\n", lpszString, hData);
    return TRUE;
}

static void
DoNullFolderTest(HWND hEdt1)
{
    HRESULT hr;

    _SEH2_TRY
    {
        hr = SHLimitInputEdit(hEdt1, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = 0xDEAD;
    }
    _SEH2_END;

    ok_int(hr, 0xDEAD);
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg != WM_INITDIALOG)
        return FALSE;

    HWND hEdt1 = GetDlgItem(hwnd, edt1);
    INT n;
    HRESULT hr;
    WCHAR szText[64];

    DoNullFolderTest(hEdt1);

    {
        CShellFolder sf(123, FALSE);

        ok_long(sf.m_cRefs, 1);

        SendMessageW(hEdt1, EM_LIMITTEXT, 234, 0);

        trace("GWLP_WNDPROC: %p\n", (void *)GetWindowLongPtr(hEdt1, GWLP_WNDPROC));

        hr = SHLimitInputEdit(hEdt1, &sf);
        ok_int(hr, S_OK);

        ok_long(sf.m_cRefs, 1);

        trace("GWLP_WNDPROC: %p\n", (void *)GetWindowLongPtr(hEdt1, GWLP_WNDPROC));

        EnumPropsW(hEdt1, PropEnumProc);

        n = (INT)SendMessageW(hEdt1, EM_GETLIMITTEXT, 0, 0);
        ok_int(n, 234);
    }

    {
        CShellFolder sf(345, TRUE);

        hr = SHLimitInputEdit(hEdt1, &sf);
        ok_int(hr, E_NOTIMPL);

        n = (INT)SendMessageW(hEdt1, EM_GETLIMITTEXT, 0, 0);
        ok_int(n, 234);
    }

    {
        CShellFolder sf(999, FALSE);

        ok_long(sf.m_cRefs, 1);

        SetWindowTextW(hEdt1, L"TEST/TEST");
        hr = SHLimitInputEdit(hEdt1, &sf);
        ok_int(hr, S_OK);
        GetWindowTextW(hEdt1, szText, _countof(szText));
        ok_wstr(szText, L"TEST/TEST");

        ok_long(sf.m_cRefs, 1);

        n = (INT)SendMessageW(hEdt1, EM_GETLIMITTEXT, 0, 0);
        ok_int(n, 234);

        SetWindowTextW(hEdt1, L"");
        SendMessageW(hEdt1, WM_CHAR, L'A', 1);
        GetWindowTextW(hEdt1, szText, _countof(szText));
        ok_wstr(szText, L"A");

        SendMessageW(hEdt1, WM_CHAR, L'/', 1);
        GetWindowTextW(hEdt1, szText, _countof(szText));
        ok_wstr(szText, L"A");

        SendMessageW(hEdt1, WM_CHAR, L'/', 1);
        GetWindowTextW(hEdt1, szText, _countof(szText));
        ok_wstr(szText, L"A");

        SendMessageW(hEdt1, WM_CHAR, L'A', 1);
        GetWindowTextW(hEdt1, szText, _countof(szText));
        ok_wstr(szText, L"AA");
    }

    {
        CShellFolder sf(4, FALSE);

        ok_long(sf.m_cRefs, 1);

        SetWindowTextW(hEdt1, L"ABC");
        hr = SHLimitInputEdit(hEdt1, &sf);

        SendMessageW(hEdt1, WM_CHAR, L'D', 1);
        GetWindowTextW(hEdt1, szText, _countof(szText));
        ok_wstr(szText, L"DABC");

        SendMessageW(hEdt1, WM_CHAR, L'E', 1);
        GetWindowTextW(hEdt1, szText, _countof(szText));
        ok_wstr(szText, L"DEABC");

        ok_long(sf.m_cRefs, 1);
    }

    EndDialog(hwnd, IDABORT);
    return TRUE;
}

START_TEST(SHLimitInputEdit)
{
    HRESULT hr;

    _SEH2_TRY
    {
        hr = SHLimitInputEdit(NULL, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = 0xDEAD;
    }
    _SEH2_END;
    ok_int(hr, 0xDEAD);

    _SEH2_TRY
    {
        hr = SHLimitInputEdit((HWND)0xDEAD, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = 0xDEAD;
    }
    _SEH2_END;
    ok_int(hr, 0xDEAD);

    DialogBoxW(GetModuleHandleW(NULL), L"SHLIMIT", NULL, DialogProc);
}
