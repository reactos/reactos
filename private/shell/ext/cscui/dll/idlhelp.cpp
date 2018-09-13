//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       idlhelp.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <shsemip.h>    // ILFree(), etc
#include "idlhelp.h"
#include "folder.h"


HRESULT
IsOfflineFilesFolderID(
    LPCITEMIDLIST pidl
    )
{
    LPITEMIDLIST pidlOfflineFilesFolder;
    HRESULT hr = COfflineFilesFolder::CreateIDList(&pidlOfflineFilesFolder);
    if (SUCCEEDED(hr))
    {
        if (ILIsEqual(pidlOfflineFilesFolder, pidl))
            hr = S_OK;
        else
            hr = S_FALSE;

        ILFree(pidlOfflineFilesFolder);
    }
    return hr;
}


HRESULT 
BindToObject(
    IShellFolder *psf, 
    REFIID riid, 
    LPCITEMIDLIST pidl, 
    void **ppvOut
    )
{
    HRESULT hr;
    IShellFolder *psfRelease;

    if (!psf)
    {
        SHGetDesktopFolder(&psf);
        psfRelease = psf;
    }
    else
        psfRelease = NULL;

    if (!pidl || ILIsEmpty(pidl))
        hr = psf->QueryInterface(riid, ppvOut);
    else
        hr = psf->BindToObject(pidl, NULL, riid, ppvOut);

    if (psfRelease)
        psfRelease->Release();

    return hr;
}



HRESULT 
BindToIDListParent(
    LPCITEMIDLIST pidl, 
    REFIID riid, 
    void **ppv, 
    LPCITEMIDLIST *ppidlLast
    )
{
    HRESULT hr;

    LPITEMIDLIST pidlParent = ILClone(pidl);
    if (pidlParent)
    {
        ILRemoveLastID(pidlParent);
        hr = ::BindToObject(NULL, riid, (LPCITEMIDLIST)pidlParent, ppv);
        if (SUCCEEDED(hr))
        {
            if (ppidlLast)
                *ppidlLast = ILFindLastID(pidl);
        }
        ILFree(pidlParent);
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

