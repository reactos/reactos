// walk.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "idlist.h"

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))


HRESULT RefreshFolder(LPCTSTR pszFolder, UINT uFlushFlags)
{
    IShellFolder *psf;

    HRESULT hr = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hr))
    {
        ULONG ulCount;
        LPITEMIDLIST pidl;
        WCHAR wszPath[MAX_PATH];

        SHAnsiToUnicode(pszFolder, wszPath, ARRAYSIZE(wszPath));

        hr = psf->ParseDisplayName(NULL, 0, wszPath, &ulCount, &pidl, NULL);
        if (SUCCEEDED(hr))
        {
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST | uFlushFlags, pidl, NULL);
            CoTaskMemFree(pidl);
        }
        psf->Release();
    }
    return hr;
}


STDAPI SHGetIDListFromUnk(IUnknown *punk, LPITEMIDLIST *ppidl)
{
    HRESULT hres = E_NOINTERFACE;

    *ppidl = NULL;

    IPersistFolder2 *ppf;
    if (punk && SUCCEEDED(punk->QueryInterface(IID_IPersistFolder2, (void **)&ppf)))
    {
        hres = ppf->GetCurFolder(ppidl);
        ppf->Release();
    }
    return hres;
}


void DumpItemInfo(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwAttribs, UINT cDepth)
{
    HRESULT hres;
    STRRET strret;
    WCHAR szName[MAX_PATH];

    for (UINT i = 0; i < cDepth; i++)
        printf("  ");

    // try a name -> parse -> name round trip of the in folder name
    hres = psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, &strret);
    if (SUCCEEDED(hres) && SUCCEEDED(StrRetToBufW(&strret, pidl, szName, ARRAYSIZE(szName))))
    {
        LPITEMIDLIST pidl;
        hres = psf->ParseDisplayName(NULL, NULL, szName, NULL, &pidl, NULL);
        if (SUCCEEDED(hres))
        {
            hres = psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, &strret);
            if (SUCCEEDED(hres) && SUCCEEDED(StrRetToBufW(&strret, pidl, szName, ARRAYSIZE(szName))))
            {

            }
            CoTaskMemFree(pidl);
        }
    }

    hres = psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret);
    if (SUCCEEDED(hres) && SUCCEEDED(StrRetToBufW(&strret, pidl, szName, ARRAYSIZE(szName))))
    {
        LPITEMIDLIST pidl;
        IShellFolder *psfDesktop;
        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            hres = psfDesktop->ParseDisplayName(NULL, NULL, szName, NULL, &pidl, NULL);
            if (SUCCEEDED(hres))
            {
                hres = psfDesktop->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret);
                if (SUCCEEDED(hres) && SUCCEEDED(StrRetToBufW(&strret, pidl, szName, ARRAYSIZE(szName))))
                {

                }
                CoTaskMemFree(pidl);
            }
            psfDesktop->Release();
        }
    }

    hres = psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &strret);
    if (SUCCEEDED(hres))
    {
        TCHAR szName[MAX_PATH];
        StrRetToBuf(&strret, pidl, szName, ARRAYSIZE(szName));
        printf("name:'%s'", szName);

    }

    LPITEMIDLIST pidlFolder;
    if (SUCCEEDED(SHGetIDListFromUnk(psf, &pidlFolder)))
    {
        LPITEMIDLIST pidlFull = ILCombine(pidlFolder, pidl);
        if (pidlFull)
        {
            SHFILEINFO sfi;
            SHGetFileInfo((LPCTSTR)pidlFull, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_TYPENAME | SHGFI_ATTRIBUTES | SHGFI_DISPLAYNAME);
            printf("type:'%s'", sfi.szTypeName);
            CoTaskMemFree(pidlFull);
        }
        CoTaskMemFree(pidlFolder);
    }


    if (dwAttribs)
        printf(TEXT("attribs: "));

    if (dwAttribs & SFGAO_FILESYSTEM)
        printf(TEXT("SFGAO_FILESYSTEM "));
    if (dwAttribs & SFGAO_FILESYSANCESTOR)
        printf(TEXT("SFGAO_FILESYSANCESTOR "));
    if (dwAttribs & SFGAO_FOLDER)
        printf(TEXT("SFGAO_FOLDER "));

    printf(TEXT("\r\n"));
}

void DumpFullPath(LPCITEMIDLIST pidlFull, DWORD dwAttribs)
{
    TCHAR szPath[MAX_PATH];
    if (SHGetPathFromIDList(pidlFull, szPath))
    {
        if (!(dwAttribs & SFGAO_FILESYSTEM))
            printf(TEXT("non file system object returned from SHGetPathFromIDList() %s\r\n"), szPath);
    }
    else
    {
        if (dwAttribs & SFGAO_FILESYSTEM)
            printf(TEXT("file system object not returned from SHGetPathFromIDList() %s\r\n"), szPath);
    }
}

// STDAPI_(LPITEMIDLIST) SHLogILFromFSIL(LPCITEMIDLIST pidlFS);

HRESULT WalkFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, UINT cDepth, UINT cDepthLimit)
{
    if (cDepth == cDepthLimit)
        return S_FALSE;     // done

    IEnumIDList *penum;
    HRESULT hres = psf->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &penum);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidl;
        ULONG c;

        while (penum->Next(1, &pidl, &c) == NOERROR)
        {
            DWORD dwAttribs = SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER;

            hres = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidl, &dwAttribs);

            DumpItemInfo(psf, pidl, dwAttribs, cDepth);

            CoTaskMemFree(pidl);
        }
        penum->Release();
    }

    hres = psf->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &penum);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidl;
        ULONG c;

        while (penum->Next(1, &pidl, &c) == NOERROR)
        {
            DWORD dwAttribs = SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER;
            hres = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidl, &dwAttribs);

            IShellFolder *psfNew;
            hres = psf->BindToObject(pidl, NULL, IID_IShellFolder, (void **)&psfNew);
            if (SUCCEEDED(hres) && psfNew)  // work around Web Fodlers bug, success but NULL
            {
                LPITEMIDLIST pidlFull = ILCombine(pidlFolder, pidl);
                if (pidlFull)
                {
                    DumpFullPath(pidlFull, dwAttribs);

//                    LPITEMIDLIST pidlT = SHLogILFromFSIL(pidlFull);
//                    if (pidlT)
//                        CoTaskMemFree(pidlT);

                    hres = WalkFolder(psfNew, pidlFull, cDepth + 1, cDepthLimit);
                    CoTaskMemFree(pidlFull);
                }
                psfNew->Release();
            }
            CoTaskMemFree(pidl);
        }
        penum->Release();
    }

    return hres;
}

HRESULT SetStartFolder(int argc, char* argv[], IShellFolder **ppsf, LPITEMIDLIST *ppidl)
{
    *ppsf = NULL;
    *ppidl = NULL;

    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        if (argc <= 1)
        {
            hres = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, ppidl);
            if (SUCCEEDED(hres))
            {
                *ppsf = psfDesktop;
                return hres;
            }
        }
        else 
        {
            int csidl = atoi(argv[1]);
            if (csidl)
            {
                hres = SHGetSpecialFolderLocation(NULL, csidl, ppidl);
                if (SUCCEEDED(hres))
                {
                    hres = psfDesktop->BindToObject(*ppidl, NULL, IID_IShellFolder, (void **)ppsf);
                    if (SUCCEEDED(hres))
                        return hres;
                    CoTaskMemFree(*ppidl);
                    *ppidl = NULL;
                }
            }
            else
            {
                WCHAR wszPath[MAX_PATH];
                SHAnsiToUnicode(argv[1], wszPath, ARRAYSIZE(wszPath));

                hres = psfDesktop->ParseDisplayName(NULL, NULL, wszPath, NULL, ppidl, NULL);
                if (SUCCEEDED(hres))
                {
                    hres = psfDesktop->BindToObject(*ppidl, NULL, IID_IShellFolder, (void **)ppsf);
                    if (FAILED(hres))
                    {
                        CoTaskMemFree(*ppidl);
                        *ppidl = NULL;
                    }
                }
            }
        }
    }
    return hres;
}

// use these as cmd line args

// CLSID_MyComputer\CLSID_ControlPanel
// "notify.exe ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"

// CLSID_MyComputer\CLSID_ControlPanel\CLSID_ConnectionsFolder
// "notify.exe ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\::{7007acc7-3202-11d1-aad2-00805fc1270e}"

// CLSID_MyComputer\CLSID_ControlPanel\CLSID_Printers
// "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\::{2227A280-3AEA-1069-A2DE-08002B30309D}";
// 


int _cdecl main(int argc, char* argv[])
{
    CoInitialize(0);

#if 0

    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidl;
        psfDesktop->ParseDisplayName(NULL, NULL, L"F:\\Can't Enum\\Can Enum", NULL, &pidl, NULL);
//        psfDesktop->ParseDisplayName(NULL, NULL, L"\\\\chrisg06\\public\\non existant", NULL, &pidl, NULL);
//        psfDesktop->ParseDisplayName(NULL, NULL, L"\\\\chrisg06\\public\\no access", NULL, &pidl, NULL);
        psfDesktop->ParseDisplayName(NULL, NULL, L"\\desktop", NULL, &pidl, NULL);
        psfDesktop->ParseDisplayName(NULL, NULL, L"\\desktop", NULL, &pidl, NULL);

        STRRET strret;
        psfDesktop->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret);
    }
#endif

    IShellFolder *psfStart;
    LPITEMIDLIST pidlStart;

    if (SUCCEEDED(SetStartFolder(argc, argv, &psfStart, &pidlStart)))
    {
        WalkFolder(psfStart, pidlStart, 0, 60);
        psfStart->Release();
        CoTaskMemFree(pidlStart);
    }
    CoUninitialize();
    return 0;
}

