/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    icon.cpp

Abstract:

    This module implements icon handling code for the protected storage
    explorer.

    The shell uses these interfaces to retrieve icons associated with
    folders in the protected storage namespace.

Author:

    Scott Field (sfield)    11-Mar-97

--*/

#include <windows.h>
#include <shlobj.h>

#include "pstore.h"

#include "enumid.h"
#include "utility.h"

#include "icon.h"
#include "resource.h"

extern HINSTANCE    g_hInst;
extern LONG         g_DllRefCount;


CExtractIcon::CExtractIcon(
    LPCITEMIDLIST pidl
    )
{

    //
    // squirrel away the type value and key type associated with the specified
    // pidl
    //

    m_dwType = GetPidlType(pidl);
    m_KeyType = GetPidlKeyType(pidl);

    m_ObjRefCount = 1;
}


CExtractIcon::~CExtractIcon()
{
}


STDMETHODIMP
CExtractIcon::QueryInterface(
    REFIID riid,
    LPVOID *ppReturn
    )
{
    *ppReturn = NULL;

    if(IsEqualIID(riid, IID_IUnknown))
        *ppReturn = (IUnknown*)(IExtractIcon*)this;
    else if(IsEqualIID(riid, IID_IExtractIcon))
        *ppReturn = (IUnknown*)(IExtractIcon*)this;

    if(*ppReturn == NULL)
        return E_NOINTERFACE;

    (*(LPUNKNOWN*)ppReturn)->AddRef();
    return S_OK;
}


STDMETHODIMP_(DWORD)
CExtractIcon::AddRef()
{
    return InterlockedIncrement(&m_ObjRefCount);
}


STDMETHODIMP_(DWORD)
CExtractIcon::Release()
{
    LONG lDecremented = InterlockedDecrement(&m_ObjRefCount);

    if(lDecremented == 0)
        delete this;

    return lDecremented;
}


STDMETHODIMP
CExtractIcon::GetIconLocation(
    UINT uFlags,
    LPTSTR szIconFile,
    UINT cchMax,
    LPINT piIndex,
    LPUINT puFlags
    )
{
    //
    // tell the shell to always call Extract
    //

    *puFlags = GIL_NOTFILENAME;

    if(uFlags & GIL_OPENICON) {
        *piIndex = 1;  // tell Extract to return the open icon
    } else {
        *piIndex = 0;

        //
        // if the icon request is associated with the "global" local machine,
        // and is at the Type or Subtype level, display a different icon.
        //

        if( m_KeyType == PST_KEY_LOCAL_MACHINE &&
            (m_dwType == PIDL_TYPE_TYPE || m_dwType == PIDL_TYPE_SUBTYPE) )
            *piIndex = 2;
    }

    return NOERROR;
}


STDMETHODIMP
CExtractIcon::Extract(
    LPCTSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize
    )
{
    LPTSTR Resource;

    UINT nIconSizeLarge = (UINT)LOWORD(nIconSize);
    UINT nIconSizeSmall = (UINT)HIWORD(nIconSize);

    //
    // note icons are cached for performance reasons.
    //

    switch (nIconIndex) {
    case 0:
        static UINT nIconSizeSmallFolder;
        static UINT nIconSizeLargeFolder;
        static HICON hIconSmallFolder;
        static HICON hIconLargeFolder;

        Resource = MAKEINTRESOURCE(IDI_FOLDER);

        if(nIconSizeSmall != nIconSizeSmallFolder) {
            hIconSmallFolder = (HICON)LoadImage(g_hInst, Resource, IMAGE_ICON, nIconSizeSmall, nIconSizeSmall, LR_DEFAULTCOLOR | LR_SHARED);
            nIconSizeSmallFolder = nIconSizeSmall;
        }
        *phiconSmall = hIconSmallFolder;

        if(nIconSizeLarge != nIconSizeLargeFolder) {
            hIconLargeFolder = (HICON)LoadImage(g_hInst, Resource, IMAGE_ICON, nIconSizeLarge, nIconSizeLarge, LR_DEFAULTCOLOR | LR_SHARED);
            nIconSizeLargeFolder = nIconSizeLarge;
        }
        *phiconLarge = hIconLargeFolder;
        return S_OK;
    case 1:
        static UINT nIconSizeSmallFolderOpen;
        static UINT nIconSizeLargeFolderOpen;
        static HICON hIconSmallFolderOpen;
        static HICON hIconLargeFolderOpen;

        Resource = MAKEINTRESOURCE(IDI_FOLDEROPEN);

        if(nIconSizeSmall != nIconSizeSmallFolderOpen) {
            hIconSmallFolderOpen = (HICON)LoadImage(g_hInst, Resource, IMAGE_ICON, nIconSizeSmall, nIconSizeSmall, LR_DEFAULTCOLOR | LR_SHARED);
            nIconSizeSmallFolderOpen = nIconSizeSmall;
        }
        *phiconSmall = hIconSmallFolderOpen;

        if(nIconSizeLarge != nIconSizeLargeFolderOpen) {
            hIconLargeFolderOpen = (HICON)LoadImage(g_hInst, Resource, IMAGE_ICON, nIconSizeLarge, nIconSizeLarge, LR_DEFAULTCOLOR | LR_SHARED);
            nIconSizeLargeFolderOpen = nIconSizeLarge;
        }
        *phiconLarge = hIconLargeFolderOpen;
        return S_OK;
    case 2:
        static UINT nIconSizeSmallGlobal;
        static UINT nIconSizeLargeGlobal;
        static HICON hIconSmallGlobal;
        static HICON hIconLargeGlobal;

        Resource = MAKEINTRESOURCE(IDI_GLOBAL);

        if(nIconSizeSmall != nIconSizeSmallGlobal) {
            hIconSmallGlobal = (HICON)LoadImage(g_hInst, Resource, IMAGE_ICON, nIconSizeSmall, nIconSizeSmall, LR_DEFAULTCOLOR | LR_SHARED);
            nIconSizeSmallGlobal = nIconSizeSmall;
        }
        *phiconSmall = hIconSmallGlobal;

        if(nIconSizeLarge != nIconSizeLargeGlobal) {
            hIconLargeGlobal = (HICON)LoadImage(g_hInst, Resource, IMAGE_ICON, nIconSizeLarge, nIconSizeLarge, LR_DEFAULTCOLOR | LR_SHARED);
            nIconSizeLargeGlobal = nIconSizeLarge;
        }
        *phiconLarge = hIconLargeGlobal;
        return S_OK;
    default:
        return S_FALSE;
    }
}

