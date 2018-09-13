#include "stdafx.h"
#pragma hdrstop
#include <mshtml.h>


// This isn't a typical delay load since it's called only if wininet
// is already loaded in memory. Otherwise the call is dropped on the floor.
// Defview did it this way I assume to keep WININET out of first boot time.
BOOL MyInternetSetOption(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2)
{
    BOOL bRet = FALSE;
    HMODULE hmod = GetModuleHandle(TEXT("wininet.dll"));
    if (hmod)
    {
        typedef BOOL (*PFNINTERNETSETOPTIONA)(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2);
        PFNINTERNETSETOPTIONA fp = (PFNINTERNETSETOPTIONA)GetProcAddress(hmod, "InternetSetOptionA");
        if (fp)
        {
            bRet = fp(h, dw1, lpv, dw2);
        }
    }
    return bRet;
}

// REVIEW: maybe just check (hwnd == GetShellWindow())

STDAPI_(BOOL) IsDesktopWindow(HWND hwnd)
{
    TCHAR szName[80];

    GetClassName(hwnd, szName, ARRAYSIZE(szName));
    if (!lstrcmp(szName, TEXT(STR_DESKTOPCLASS)))
    {
        return hwnd == GetShellWindow();
    }
    return FALSE;
}

// returns:
//      S_OK                all is well, trust the enviornment we are in
//      S_FALSE or 
//      E_ACCESSDENIED      bad... don't expose local machine access
STDAPI IsSafePage(IUnknown *punkSite)
{
    // Return S_FALSE if we don't have a host site since we have no way of doing a 
    // security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
        return S_FALSE;

    HRESULT hr = E_ACCESSDENIED;
    WCHAR wszPath[MAX_PATH];
    wszPath[0] = 0;

    // ask the browser, for example we are in a .HTM doc
    IBrowserService* pbs;
    if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &pbs))))
    {
        LPITEMIDLIST pidl;

        if (SUCCEEDED(pbs->GetPidl(&pidl)))
        {
            DWORD dwAttribs = SFGAO_FOLDER;
            if (SUCCEEDED(SHGetNameAndFlagsW(pidl, SHGDN_FORPARSING, wszPath, ARRAYSIZE(wszPath), &dwAttribs))
                    && (dwAttribs & SFGAO_FOLDER))   // This is a folder. So, wszPath should be the path for it's webview template
            {
                // find the template path from webview, for example a .HTT file
                IOleCommandTarget *pct;
                if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_DefView, IID_PPV_ARG(IOleCommandTarget, &pct))))
                {
                    VARIANT vPath;
                    vPath.vt = VT_EMPTY;
                    if (pct->Exec(&CGID_DefView, DVCMDID_GETTEMPLATEDIRNAME, 0, NULL, &vPath) == S_OK)
                    {
                        if (vPath.vt == VT_BSTR && vPath.bstrVal)
                        {
                            DWORD cchPath = ARRAYSIZE(wszPath);
                            if (S_OK != PathCreateFromUrlW(vPath.bstrVal, wszPath, &cchPath, 0))
                            {
                                // it might not be an URL, in this case it is a file path
                                StrCpyNW(wszPath, vPath.bstrVal, ARRAYSIZE(wszPath));
                            }
                        }
                        VariantClear(&vPath);
                    }
                    pct->Release();
                }
            }
            ILFree(pidl);
        }
        pbs->Release();
    }
    else
    {
        ASSERT(0);      // no browser, where are we?
    }

    if (wszPath[0])
        hr = SHRegisterValidateTemplate(wszPath, SHRVT_VALIDATE | SHRVT_PROMPTUSER | SHRVT_REGISTERIFPROMPTOK);

    return hr;
}



