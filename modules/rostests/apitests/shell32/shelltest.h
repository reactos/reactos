#ifndef _SHELLTEST_H_
#define _SHELLTEST_H_

//#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <apitest.h>
#include <winreg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <atlcom.h>

// Vista's shell32 is buggy so we need to skip some tests there.
static const BOOL g_bVista = (GetNTVersion() == _WIN32_WINNT_VISTA);

VOID PathToIDList(LPCWSTR pszPath, ITEMIDLIST** ppidl);

#ifdef __cplusplus
template<class R>
HRESULT SHELL_BindToObject(IShellFolder *pSF, LPCITEMIDLIST pidl, IBindCtx *pBC, R riid, void **ppv)
{
    CComPtr<IShellFolder> psfDesktop;
    HRESULT hr;
    if (!pSF)
    {
        if (FAILED(hr = SHGetDesktopFolder(&psfDesktop)))
            return hr;
        pSF = psfDesktop;
    }
    hr = ILIsEmpty(pidl) ? pSF->QueryInterface(riid, ppv) : pSF->BindToObject(pidl, pBC, riid, ppv);
    if (SUCCEEDED(hr) && !*ppv)
        hr = E_FAIL;
    return hr;
}
#endif

#endif /* !_SHELLTEST_H_ */
