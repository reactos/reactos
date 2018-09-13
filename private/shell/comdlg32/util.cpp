/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    util.cpp

Abstract:

    This module implements utility functions for the common dialog.

Author :
    Arul Kumaravel              (arulk@microsoft.com)
--*/


#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <commctrl.h>
#include <ole2.h>
#include "cdids.h"
#include "fileopen.h"
#include "filenew.h"

#include <coguid.h>
#include <shlguid.h>
#include <shguidp.h>
#include <oleguid.h>
#include <shldisp.h>
#include <inetreg.h>

#include <commdlg.h>

#include "util.h"

#ifndef ASSERT
#define ASSERT Assert
#endif

#define EVAL(x)     x

#define USE_AUTOCOMPETE_DEFAULT         TRUE
#define SZ_REGKEY_USEAUTOCOMPLETE       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define SZ_REGVALUE_FILEDLGAUTOCOMPLETE TEXT("AutoComplete In File Dialog")
#define BOOL_NOT_SET                        0x00000005
#define SZ_REGVALUE_AUTOCOMPLETE_TAB        TEXT("Always Use Tab")

/****************************************************\
    FUNCTION: AutoComplete

    DESCRIPTION:
        This function will have AutoComplete take over
    an editbox to help autocomplete DOS paths.
\****************************************************/
HRESULT AutoComplete(HWND hwndEdit, ICurrentWorkingDirectory ** ppcwd, DWORD dwFlags)
{
    HRESULT hr;
    IUnknown * punkACLISF;
    static BOOL fUseAutoComplete = -10; // Not inited.
    
    if (-10 == fUseAutoComplete)
        fUseAutoComplete = (SHRegGetBoolUSValue(SZ_REGKEY_USEAUTOCOMPLETE, SZ_REGVALUE_FILEDLGAUTOCOMPLETE, FALSE, USE_AUTOCOMPETE_DEFAULT));

    // WARNING: If you want to disable AutoComplete by default, 
    //          turn USE_AUTOCOMPETE_DEFAULT to FALSE
    if (fUseAutoComplete)
    {
        Assert(!dwFlags);	// Not yet used.
        hr = SHCoCreateInstance(NULL, &CLSID_ACListISF, NULL, IID_IUnknown, (void **)&punkACLISF);

        Assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            IAutoComplete2 * pac;

            // Create the AutoComplete Object
            hr = SHCoCreateInstance(NULL, &CLSID_AutoComplete, NULL, IID_IAutoComplete2, (void **)&pac);

            Assert(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                DWORD dwOptions = 0;

                hr = pac->Init(hwndEdit, punkACLISF, NULL, NULL);

                // Set the autocomplete options
                if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE))
                {
                    dwOptions |= ACO_AUTOAPPEND;
                }

                if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE))
                {
                    dwOptions |= ACO_AUTOSUGGEST;
                }

                // Windows uses the TAB key to move between controls in a dialog.  UNIX and other
                // operating systems that use AutoComplete have traditionally used the TAB key to
                // iterate thru the AutoComplete possibilities.  We need to default to disable the
                // TAB key (ACO_USETAB) unless the caller specifically wants it.  We will also
                // turn it on 
                static BOOL s_fAlwaysUseTab = BOOL_NOT_SET;
                if (BOOL_NOT_SET == s_fAlwaysUseTab)
                    s_fAlwaysUseTab = SHRegGetBoolUSValue(SZ_REGKEY_USEAUTOCOMPLETE, SZ_REGVALUE_AUTOCOMPLETE_TAB, FALSE, FALSE);
                    
                if (s_fAlwaysUseTab)
                    dwOptions |= ACO_USETAB;
                    
                EVAL(SUCCEEDED(pac->SetOptions(dwOptions)));

                pac->Release();
            }

            if (ppcwd)
            {
                punkACLISF->QueryInterface(IID_ICurrentWorkingDirectory, (void **)ppcwd);
            }

            punkACLISF->Release();
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: SetAutoCompleteCWD

    DESCRIPTION:
\****************************************************/
HRESULT SetAutoCompleteCWD(LPCTSTR pszDir, ICurrentWorkingDirectory * pcwd)
{
    WCHAR wsDir[MAX_PATH];

    if (!pcwd)
       return S_OK;

    SHTCharToUnicode(pszDir, wsDir, ARRAYSIZE(wsDir));
    return pcwd->SetDirectory(wsDir);
}


////////////////////////////////////////////////////////////////////////////
//
//  Overloaded allocation operators.
//
////////////////////////////////////////////////////////////////////////////
void * __cdecl operator new(
    size_t size)
{
    return ((void *)LocalAlloc(LPTR, size));
}

void __cdecl operator delete(
    void *ptr)
{
    LocalFree(ptr);
}

__cdecl _purecall(void)
{
    return (0);
}





////////////////////////////////////////////////////////////////////////////
// 
//  Common Dialog Administrator Restrictions
//
////////////////////////////////////////////////////////////////////////////

const SHRESTRICTIONITEMS c_rgRestItems[] =
{
    {REST_NOBACKBUTTON,            L"Comdlg32", L"NoBackButton"},
    {REST_NOFILEMRU ,              L"Comdlg32", L"NoFileMru"},
    {REST_NOPLACESBAR,             L"Comdlg32", L"NoPlacesBar"},
    {0, NULL, NULL},
};

#define NUMRESTRICTIONS  ARRAYSIZE(c_rgRestItems)


DWORD g_rgRestItemValues[NUMRESTRICTIONS - 1 ] = { -1 };

DWORD IsRestricted(COMMDLG_RESTRICTIONS rest)
{   static BOOL bFirstTime = TRUE;

    if (bFirstTime)
    {
       memset((LPBYTE)g_rgRestItemValues,(BYTE)-1, SIZEOF(g_rgRestItemValues));
       bFirstTime = FALSE;
    }
    return SHRestrictionLookup(rest, NULL, c_rgRestItems, g_rgRestItemValues);
}


STDAPI CDBindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut)
{
    HRESULT hres;
    IShellFolder *psfRelease;

    *ppvOut = NULL;

    
    if (!psf)
    {
       SHGetDesktopFolder(&psf);
       psfRelease = psf;
    }
    else
    {
        psfRelease = NULL;
    }

    if (!pidl || ILIsEmpty(pidl))
    {
        hres = psf->QueryInterface(riid, ppvOut);
    }
    else
    {
        hres = psf->BindToObject(pidl, NULL, riid, ppvOut);
    }

    if (psfRelease)
    {
        psfRelease->Release();
    }

    return hres;
}


STDAPI CDGetUIObjectFromFullPIDL(LPCITEMIDLIST pidl, HWND hwnd, REFIID riid, void** ppv)
{
    *ppv = NULL;

    LPCITEMIDLIST pidlChild;
    IShellFolder* psf;
    HRESULT hr = CDBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlChild);
    if (SUCCEEDED(hr))
    {
        hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, NULL, ppv);
        psf->Release();
    }

    return hr;
}


STDAPI CDBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    HRESULT hres;
    
    LPITEMIDLIST pidlParent = ILClone(pidl);
    if (pidlParent) 
    {
        ILRemoveLastID(pidlParent);
        hres = CDBindToObject(NULL, riid, pidlParent, ppv);
        ILFree(pidlParent);
    }
    else
        hres = E_OUTOFMEMORY;

    if (ppidlLast)
        *ppidlLast = ILFindLastID(pidl);

    return hres;
}


STDAPI CDGetNameAndFlags(LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName, UINT cchName, DWORD *pdwAttribs)
{
    if (pszName)
    {
        *pszName = 0;
    }

    IShellFolder *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hres = CDBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlLast);
    if (SUCCEEDED(hres))
    {
        if (pszName)
        {
            STRRET str;
            hres = psf->GetDisplayNameOf(pidlLast, dwFlags, &str);
            if (SUCCEEDED(hres))
                StrRetToStrN(pszName, cchName, &str, pidlLast);
        }

        if (pdwAttribs)
        {
            ASSERT(*pdwAttribs);    // this is an in-out param
            hres = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlLast, pdwAttribs);
        }
        psf->Release();
    }
    return hres;
}


STDAPI CDGetAttributesOf(LPCITEMIDLIST pidl, ULONG* prgfInOut)
{
    return CDGetNameAndFlags(pidl, 0, NULL, 0, prgfInOut);
}


#define MODULE_NAME_SIZE    128
#define MODULE_VERSION_SIZE  15

typedef struct tagAPPCOMPAT
{
    LPCTSTR pszModule;
    LPCTSTR pszVersion;
    DWORD  dwFlags;
} APPCOMPAT, FAR* LPAPPCOMPAT;
    
DWORD CDGetAppCompatFlags()
{
    static BOOL  bInitialized = FALSE;
    static DWORD dwCachedFlags = 0;
    static const APPCOMPAT aAppCompat[] = 
    {   //Mathcad
        {TEXT("MCAD.EXE"), TEXT("6.00b"), CDACF_MATHCAD},
        //Picture Publisher
        {TEXT("PP70.EXE"),NULL, CDACF_NT40TOOLBAR},
        {TEXT("PP80.EXE"),NULL, CDACF_NT40TOOLBAR},
        //Code Wright
        {TEXT("CW32.exe"),TEXT("5.1"), CDACF_NT40TOOLBAR},
        //Designer.exe
        {TEXT("ds70.exe"),NULL, CDACF_FILETITLE}
    };
    
    if (!bInitialized)
    {    
        TCHAR  szModulePath[MODULE_NAME_SIZE];
        TCHAR* pszModuleName;
        DWORD  dwHandle;
        int i;

        GetModuleFileName(GetModuleHandle(NULL), szModulePath, ARRAYSIZE(szModulePath));
        pszModuleName = PathFindFileName(szModulePath);

        if (pszModuleName)
        {
            for (i=0; i < ARRAYSIZE(aAppCompat); i++)
            {
                if (lstrcmpi(aAppCompat[i].pszModule, pszModuleName) == 0)
                {
                    if (aAppCompat[i].pszVersion == NULL)
                    {
                        dwCachedFlags = aAppCompat[i].dwFlags;
                    }
                    else
                    {
                        CHAR  chBuffer[3072]; // hopefully this is enough... lotus smart center needs 3000
                        TCHAR* pszVersion = NULL;
                        UINT  cb;

                        // get module version here!
                        cb = GetFileVersionInfoSize(szModulePath, &dwHandle); 
                        if (cb <= ARRAYSIZE(chBuffer) &&
                            GetFileVersionInfo(szModulePath, dwHandle, ARRAYSIZE(chBuffer), (LPVOID)chBuffer) &&
                            VerQueryValue((LPVOID)chBuffer, TEXT("\\StringFileInfo\\040904E4\\ProductVersion"), (void **) &pszVersion, &cb))
                        {   
                            DebugMsg(0x0004, TEXT("product: %s\n version: %s"), pszModuleName, pszVersion);
                            if (lstrcmpi(pszVersion, aAppCompat[i].pszVersion) == 0)
                            {
                                dwCachedFlags = aAppCompat[i].dwFlags;
                                break;
                            }
                        }
                    }
                }
            }
        }
        bInitialized = TRUE;
    }
    
    return dwCachedFlags; 
}


BOOL ILIsFTP(LPCITEMIDLIST pidl)
{
    IShellFolder * psf;
    BOOL fIsFTPFolder = FALSE;

    if (SUCCEEDED(CDBindToObject(NULL, IID_IShellFolder, pidl, (void **) &psf)))
    {
        CLSID clsid;

        if (SUCCEEDED(IUnknown_GetClassID(psf, &clsid)) &&
            (IsEqualIID(clsid, CLSID_FtpFolder)))
        {
            fIsFTPFolder = TRUE;
        }

        psf->Release();
    }

    return fIsFTPFolder;
}

