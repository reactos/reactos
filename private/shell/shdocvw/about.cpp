#include "priv.h"

#include <mluisupp.h>

//
//  The about box is now an HTML dialog. It is sent a ~ (tilde) 
//  delimited BSTR that has, in this order, version number, 
//  person software is licensed to, company software is licensed to, and 
//  whether 40, 56, or 128 bit ie is installed.
//

STDAPI_(void) IEAboutBox( HWND hWnd )
{
    TCHAR szInfo[512 + INTERNET_MAX_URL_LENGTH];  // potential for IEAK specific URL from 
                                                  // SHAboutInfo
    szInfo[0] = 0;

    SHAboutInfo(szInfo, ARRAYSIZE(szInfo));     // from shlwapi

    BSTR bstrVal = SysAllocStringT(szInfo);
    if (bstrVal)
    {
        TCHAR   szResURL[MAX_URL_STRING];
        HRESULT hr;

        hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                               HINST_THISDLL,
                               ML_CROSSCODEPAGE,
                               TEXT("about.dlg"),
                               szResURL,
                               ARRAYSIZE(szResURL),
                               TEXT("shdocvw.dll"));
        if (SUCCEEDED(hr))
        {
            VARIANT var = {0};      // variant containing version and user info
            var.vt = VT_BSTR;
            var.bstrVal = bstrVal;

            IMoniker *pmk;
            if (SUCCEEDED(CreateURLMoniker(NULL, szResURL, &pmk)))
            {
                ShowHTMLDialog(hWnd, pmk, &var, L"help: no", NULL);
                pmk->Release();
            }
            SysFreeString(bstrVal);
        }
    }
}
