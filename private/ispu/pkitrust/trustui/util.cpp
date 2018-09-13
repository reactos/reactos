//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       util.cpp
//
//  Contents:   Miscellaneous utility functions
//
//  History:    12-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <stdpch.h>

#include <urlmon.h>
#include <hlink.h>

#include    "unicode.h"

//
// The following are stolen from SOFTPUB
//
void TUIGoLink(HWND hwndParent, WCHAR *pszWhere)
{
    HCURSOR hcursPrev;
    HMODULE hURLMon;


    //
    //  since we're a model dialog box, we want to go behind IE once it comes up!!!
    //
    SetWindowPos(hwndParent, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    hcursPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hURLMon = (HMODULE)LoadLibraryU(L"urlmon.dll");

    if (!(hURLMon))
    {
        //
        // The hyperlink module is unavailable, go to fallback plan
        //
        //
        // This works in test cases, but causes deadlock problems when used from withing
        // the Internet Explorer itself. The dialog box is up (that is, IE is in a modal
        // dialog loop) and in comes this DDE request...).
        //
        DWORD   cb;
        LPSTR   psz;

        cb = WideCharToMultiByte(
                        0, 
                        0, 
                        pszWhere, 
                        -1,
                        NULL, 
                        0, 
                        NULL, 
                        NULL);

            if (NULL == (psz = new char[cb]))
            {
                return;
            }

            WideCharToMultiByte(
                        0, 
                        0, 
                        pszWhere, 
                        -1,
                        psz, 
                        cb, 
                        NULL, 
                        NULL);

        ShellExecute(hwndParent, "open", psz, NULL, NULL, SW_SHOWNORMAL);

        delete[] psz;
    } 
    else 
    {
        //
        // The hyperlink module is there. Use it
        //
        if (SUCCEEDED(CoInitialize(NULL)))       // Init OLE if no one else has
        {
            //
            //  allow com to fully init...
            //
            MSG     msg;

            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE); // peek but not remove

            typedef void (WINAPI *pfnHlinkSimpleNavigateToString)(LPCWSTR, LPCWSTR, LPCWSTR, IUnknown *,
                                                                  IBindCtx *, IBindStatusCallback *,
                                                                  DWORD, DWORD);

            pfnHlinkSimpleNavigateToString      pProcAddr;

            pProcAddr = (pfnHlinkSimpleNavigateToString)GetProcAddress(hURLMon, TEXT("HlinkSimpleNavigateToString"));

            if (pProcAddr)
            {
                IBindCtx    *pbc;  

                pbc = NULL;

                CreateBindCtx( 0, &pbc ); 

                (*pProcAddr)(pszWhere, NULL, NULL, NULL, pbc, NULL, HLNF_OPENINNEWWINDOW, NULL);

                if (pbc)
                {
                    pbc->Release();
                }
            }
        
            CoUninitialize();
        }

        FreeLibrary(hURLMon);
    }

    SetCursor(hcursPrev);
}

WCHAR *GetGoLink(SPC_LINK *psLink)
{
    if (!(psLink))
    {
        return(NULL);
    }

    switch (psLink->dwLinkChoice)
    {
        case SPC_URL_LINK_CHOICE:       return(psLink->pwszUrl);
        case SPC_FILE_LINK_CHOICE:      return(psLink->pwszFile);
        case SPC_MONIKER_LINK_CHOICE:   return(NULL); // TBDTBD!!!
    }

    return(NULL);
}

