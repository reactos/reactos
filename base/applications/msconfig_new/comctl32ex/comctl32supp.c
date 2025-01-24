/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/comctl32ex/comctl32supp.c
 * PURPOSE:     Common Controls helper functions.
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"

HRESULT GetComCtl32Version(OUT PDWORD pdwMajor, OUT PDWORD pdwMinor, OUT PDWORD pdwBuild)
{
    HRESULT   hr = S_OK;
    HINSTANCE hDll;
    DLLGETVERSIONPROC pDllGetVersion;
    DLLVERSIONINFO dvi;

    *pdwMajor = 0;
    *pdwMinor = 0;
    *pdwBuild = 0;

    /*
     * WARNING! DISCLAIMER!
     *
     * This method of retrieving a handle to an already loaded comctl32.dll
     * is known to not be reliable in case the program is using SxS, or if
     * this code is used from inside a DLL.
     */

    /*
     * See: https://learn.microsoft.com/en-us/windows/win32/controls/common-control-versions
     * and: http://www.geoffchappell.com/studies/windows/shell/comctl32/history/
     * for the possible version values to be returned.
     */

    /* Get a handle to comctl32.dll that must already be loaded */
    hDll = GetModuleHandleW(L"comctl32.dll"); // NOTE: We must not call FreeLibrary on the returned handle!
    if (!hDll) return E_FAIL;

    pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hDll, "DllGetVersion");
    if (pDllGetVersion)
    {
        ZeroMemory(&dvi, sizeof(dvi));
        dvi.cbSize = sizeof(dvi);

        hr = (*pDllGetVersion)(&dvi);
        if (SUCCEEDED(hr))
        {
            *pdwMajor = dvi.dwMajorVersion;
            *pdwMinor = dvi.dwMinorVersion;
            *pdwBuild = dvi.dwBuildNumber;

#if 0
// #include "stringutils.h"

            LPWSTR strVersion =
                FormatString(L"ComCtl32 version %d.%d, Build %d, Platform %d",
                             dvi.dwMajorVersion, dvi.dwMinorVersion, dvi.dwBuildNumber, dvi.dwPlatformID);
            MessageBoxW(NULL, strVersion, L"ComCtl32 version", MB_OK);
            MemFree(strVersion);
#endif
        }
    }
    else
    {
        /*
         * If GetProcAddress failed, the DLL is a version
         * previous to the one shipped with IE 3.x.
         */
        *pdwMajor = 4;
        *pdwMinor = 0;
        *pdwBuild = 0;
    }

    return hr;
}
