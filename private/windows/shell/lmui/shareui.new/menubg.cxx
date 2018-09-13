//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menubg.cxx
//
//  Contents:   Implementation of IContextMenu
//
//  History:    20-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "util.hxx"
#include "menubg.hxx"
#include "menuutil.hxx"
#include "resource.h"


STDMETHODIMP
CSharesCMBG::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
    )
{
    UINT idMainMerge;
    if (uFlags & CMF_DVFILE)
    {
        // This IContextMenu was created to add items to the DefView menu.
        // We only want to add a "new" item to the "File" menu, so set don't
        // do any "main" merge.
        idMainMerge = 0;
    }
    else
    {
        // In this case, it's actually a background right-click context menu,
        // so merge in the "New" menu.
        idMainMerge = POPUP_SHARESBG_POPUPMERGE2;
    }

    QCMINFO qcm = { hmenu, indexMenu, idCmdFirst, idCmdLast };

    switch (m_level)
    {
    case 1:
        MyMergeMenu(
                g_hInstance,
                0,
                POPUP_SHARESBG_POPUPMERGE1,
                &qcm);
        break;

    case 2:
        MyMergeMenu(
                g_hInstance,
                idMainMerge,
                POPUP_SHARESBG_POPUPMERGE2,
                &qcm);
        break;

    default: appAssert(FALSE);
    }

    return ResultFromShort(qcm.idCmdFirst - idCmdFirst);
}

STDMETHODIMP
CSharesCMBG::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
    )
{
    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        return E_INVALIDARG;
    }

    appAssert(ICOL1_NAME    == ICOL2_NAME);
    appAssert(ICOL1_COMMENT == ICOL2_COMMENT);

    UINT idCmd = (UINT)LOWORD((DWORD)lpici->lpVerb);
    switch (idCmd)
    {
    case FSIDM_SORTBYNAME:
        ShellFolderView_ReArrange(m_hwnd, ICOL2_NAME);
        return NOERROR;

    case FSIDM_SORTBYCOMMENT:
        ShellFolderView_ReArrange(m_hwnd, ICOL2_COMMENT);
        return NOERROR;

    case FSIDM_SORTBYPATH:
        appAssert(m_level >= 2);
        ShellFolderView_ReArrange(m_hwnd, ICOL2_PATH);
        return NOERROR;

    case FSIDM_SORTBYMAXUSES:
        appAssert(m_level >= 2);
        ShellFolderView_ReArrange(m_hwnd, ICOL2_MAXUSES);
        return NOERROR;

    case FSIDM_NEWSHARE:
    {
        appAssert(m_level >= 2);
        // pass in a pointer to our own IUnknown
        IUnknown* punk;
        HRESULT hr = QueryInterface(IID_IUnknown, (LPVOID*)&punk);
        if (SUCCEEDED(hr))
        {
            hr = ShareDoNew(punk, m_pszMachine);
            punk->Release();
        }
        return hr;
    }

    default:
        return E_INVALIDARG;
    }
}

STDMETHODIMP
CSharesCMBG::GetCommandString(
    UINT        idCmd,
    UINT        uType,
    UINT      * pwReserved,
    LPSTR       pszName,
    UINT        cchMax
    )
{
    *((LPWSTR)pszName) = TEXT('\0');
    if (uType == GCS_HELPTEXT)
    {
        LoadStringW(g_hInstance, idCmd + IDS_MH_FSIDM_FIRST, (LPWSTR)pszName, cchMax);
        return NOERROR;
    }

    return E_FAIL;
}
