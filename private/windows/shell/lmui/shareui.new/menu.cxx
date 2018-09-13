//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menu.cxx
//
//  Contents:   Implementation of IContextMenu
//
//  History:    20-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "util.hxx"
#include "dutil.hxx"
#include "menu.hxx"
#include "menuutil.hxx"
#include "shares.h"
#include "resource.h"


CSharesCM::CSharesCM(
    IN HWND hwnd
    )
    :
    m_ulRefs(0),
    m_hwnd(hwnd),
    m_cidl(0),
    m_apidl(NULL),
    m_psf(NULL)
{
    AddRef();
}


HRESULT
CSharesCM::InitInstance(
    IN PWSTR pszMachine,
    IN UINT cidl,
    IN LPCITEMIDLIST* apidl,
    IN IShellFolder* psf
    )
{
    m_pszMachine = pszMachine;

    m_cidl = cidl;
    m_apidl = ILA_Clone(cidl, apidl);
    if (NULL == m_apidl)
    {
        return E_OUTOFMEMORY;
    }

    appAssert(NULL != psf);
    m_psf = psf;
    m_psf->AddRef();

    return S_OK;
}


CSharesCM::~CSharesCM()
{
    ILA_Free(m_cidl, m_apidl);
    m_cidl = 0;
    m_apidl = NULL;

    appAssert(NULL != m_psf);
    m_psf->Release();
    m_psf = NULL;
}


STDMETHODIMP
CSharesCM::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
    )
{
    if (uFlags & CMF_DVFILE)
    {
        // This IContextMenu was created to add items to the DefView "File"
        // menu. This menu already has 4 standard items: Create Shortcut,
        // Delete, Rename, and Properties. Since we only want Delete and
        // Properties, and don't need to add anything, we simply do nothing
        // and let DefView do the work.
        return ResultFromShort(0);
    }
    else
    {
        // Got an IContextMenu because of a double-click on the item (to get
        // the default item) or a right-click on the item (to display the
        // entire context menu). So, put it up.

        QCMINFO qcm = { hmenu, indexMenu, idCmdFirst, idCmdLast };
        MyMergeMenu(g_hInstance, POPUP_SHARE, 0, &qcm);

        ULONG dwInOut = SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_CANRENAME;
        HRESULT hr = m_psf->GetAttributesOf(m_cidl, (LPCITEMIDLIST*)m_apidl, &dwInOut);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            return hr;
        }

        if (!(dwInOut & SFGAO_CANDELETE))
        {
            EnableMenuItem(hmenu, idCmdFirst + SHARED_FILE_DELETE, MF_GRAYED | MF_BYCOMMAND);
        }
        if (!(dwInOut & SFGAO_CANRENAME))
        {
            EnableMenuItem(hmenu, idCmdFirst + SHARED_FILE_RENAME, MF_GRAYED | MF_BYCOMMAND);
        }
        if (!(dwInOut & SFGAO_HASPROPSHEET))
        {
            EnableMenuItem(hmenu, idCmdFirst + SHARED_FILE_PROPERTIES, MF_GRAYED | MF_BYCOMMAND);
        }

        SetMenuDefaultItem(hmenu, idCmdFirst + SHARED_FILE_PROPERTIES, FALSE);
        return ResultFromShort(qcm.idCmdFirst - idCmdFirst);
    }
}

STDMETHODIMP
CSharesCM::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
    )
{
    HRESULT hr = S_OK;
    LPIDSHARE pids;
    UINT i;
    UINT idCmd = (UINT)LOWORD((DWORD)lpici->lpVerb);

    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        PSTR pszCmd = (PSTR)lpici->lpVerb;

        // Check for "delete" and "properties" that come from the toolbar or
        // the shell view.
        if (0 == lstrcmpA(pszCmd, "delete"))
        {
            idCmd = SHARED_FILE_DELETE;
        }
        else if (0 == lstrcmpA(pszCmd, "rename"))
        {
            idCmd = SHARED_FILE_RENAME;
        }
        else if (0 == lstrcmpA(pszCmd, "properties"))
        {
            idCmd = SHARED_FILE_PROPERTIES;
        }
        else
        {
            return E_INVALIDARG;
        }
    }

    switch(idCmd)
    {
    case SHARED_FILE_DELETE:
    {
        for (UINT i = 0; i < m_cidl; i++)
        {
            pids = (LPIDSHARE)m_apidl[i];
            hr = ShareDoDelete(m_hwnd, m_pszMachine, Share_GetName(pids));
            CHECK_HRESULT(hr);
            // even if failure, keep going
        }
        break;
    }

    case SHARED_FILE_RENAME:
    {
        appAssert(!"Shouldn't get rename command here!");
        hr = E_FAIL;
        break;
    }

    case SHARED_FILE_PROPERTIES:
    {
        // should we do multiple-select properties?
        appAssert(m_cidl == 1);
        pids = (LPIDSHARE)m_apidl[0];

        // pass in a pointer to our own IUnknown
        IUnknown* punk;
        hr = QueryInterface(IID_IUnknown, (LPVOID*)&punk);
        if (SUCCEEDED(hr))
        {
            hr = ShareDoProperties(punk, m_pszMachine, Share_GetName(pids));
            punk->Release();
        }
        break;
    }

    } // switch(wParam)

    return hr;
}

STDMETHODIMP
CSharesCM::GetCommandString(
    UINT        idCmd,
    UINT        uType,
    UINT      * pwReserved,
    LPSTR       pszName,
    UINT        cchMax
    )
{
    HRESULT hr = E_FAIL;

    *((LPWSTR)pszName) = TEXT('\0');
    if (uType == GCS_HELPTEXT)
    {
        LoadStringW(g_hInstance, idCmd + IDS_MH_FSIDM_FIRST, (LPWSTR)pszName, cchMax);
        hr = S_OK;
    }
    else if (uType == GCS_VERB)
    {
        switch(idCmd)
        {
        case SHARED_FILE_DELETE:
        case SHARED_FILE_PROPERTIES:
            break;

        case SHARED_FILE_RENAME:
            StrNCopy((LPWSTR)pszName, c_szRename, cchMax);
            hr = S_OK;
            break;
        }
    }

    return hr;
}
