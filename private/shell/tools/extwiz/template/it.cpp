// $$ClassType$$IT.cpp : Implementation of C$$ClassType$$IT
#include "stdafx.h"
#include "InfoTipTest.h"
#include "$$ClassType$$IT.h"
#include "shlobj.h"

/////////////////////////////////////////////////////////////////////////////
// C$$ClassType$$IT

// IPersist methods

STDMETHODIMP C$$ClassType$$IT::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_$$ClassType$$IT;
    return NOERROR;
}

// IPersistFile methods

STDMETHODIMP C$$ClassType$$IT::Load(LPCOLESTR pcwszFileName, DWORD dwMode)
{
    lstrcpyW(_wszFileName, pcwszFileName);
    return NOERROR;
}

// IQueryInfo methods

STDMETHODIMP C$$ClassType$$IT::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    IShellLink* psl;
    HRESULT hres;
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, 
        (void **)&psl);

    if(SUCCEEDED(hres))
    {
        IPersistFile* ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hres))
        {
            hres = ppf->Load(_wszFileName, 0);
            if (SUCCEEDED(hres))
            {
                TCHAR szTarget[MAX_PATH];

                hres = psl->GetPath(szTarget, MAX_PATH, NULL, 0);
                if (SUCCEEDED(hres))
                {
                    WCHAR* pwszOut = new WCHAR[MAX_PATH];
                    if (!pwszOut)
                        hres = E_OUTOFMEMORY;
                    else
                    {
                        MultiByteToWideChar(CP_ACP, 0, szTarget, -1, pwszOut, MAX_PATH);
                        *ppwszTip = pwszOut;

                        // Caller will free pszOut
                    }
                }
            }
            ppf->Release();
        }
        psl->Release();
    }

    return hres;
}

STDMETHODIMP C$$ClassType$$IT::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;
    return NOERROR;
}

