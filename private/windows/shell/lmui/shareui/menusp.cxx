//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menusp.cxx
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
#include "menusp.hxx"
#include "menuutil.hxx"
#include "shares.h"
#include "resource.h"


#ifdef WIZARDS

CSharesCMSpecial::CSharesCMSpecial(
    IN HWND hwnd
    )
    :
    m_ulRefs(0),
    m_hwnd(hwnd),
    m_pidl(NULL),
    m_psf(NULL)
{
    AddRef();
}


HRESULT
CSharesCMSpecial::InitInstance(
    IN PWSTR pszMachine,
    IN LPCITEMIDLIST pidl,
    IN IShellFolder* psf
    )
{
    m_pszMachine = pszMachine;

    m_pidl = ILClone(pidl);
    if (NULL == m_pidl)
    {
        return E_OUTOFMEMORY;
    }

    m_psf = psf;
    m_psf->AddRef();

    return S_OK;
}


CSharesCMSpecial::~CSharesCMSpecial()
{
    ILFree(m_pidl);
    m_pidl = NULL;
    m_psf->Release();
    m_psf = NULL;
}


STDMETHODIMP
CSharesCMSpecial::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
    )
{
    UINT idMerge;
    if (uFlags & CMF_DVFILE)
    {
        idMerge = POPUP_SPECIAL_FILE;
    }
    else
    {
        idMerge = POPUP_SPECIAL;
    }

    QCMINFO qcm = { hmenu, indexMenu, idCmdFirst, idCmdLast };
    MyMergeMenu(g_hInstance, idMerge, 0, &qcm);
    SetMenuDefaultItem(hmenu, idCmdFirst + FSIDM_OPENSPECIAL, FALSE);
    return ResultFromShort(qcm.idCmdFirst - idCmdFirst);
}

STDMETHODIMP
CSharesCMSpecial::InvokeCommand(
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

        // Check for "link" that comes from the toolbar or the shell view.
        if (0 == lstrcmpA(pszCmd, "link"))
        {
            idCmd = SHARED_FILE_LINK;
        }
        else
        {
            return E_INVALIDARG;
        }
    }

    switch(idCmd)
    {
    case FSIDM_OPENSPECIAL:
    {
        pids = (LPIDSHARE)m_pidl;
        hr = ShareDoSpecial(m_hwnd, m_pszMachine, Share_GetFlags(pids));
        CHECK_HRESULT(hr);
        break;
    }

    case SHARED_FILE_LINK:
    {
        UINT dwfInOut = 0;
        IDataObject* pDataObject;
        hr = m_psf->GetUIObjectOf(
                        lpici->hwnd,
                        1,
                        (LPCITEMIDLIST*)&m_pidl,
                        IID_IDataObject,
                        &dwfInOut,
                        (LPVOID*)&pDataObject);
        if (SUCCEEDED(hr))
        {
            SHCreateLinks(
                lpici->hwnd,
                NULL,
                pDataObject,
                SHCL_USETEMPLATE,
                NULL);
            pDataObject->Release();
        }
        break;
    }

    } // switch(wParam)

    return hr;
}

STDMETHODIMP
CSharesCMSpecial::GetCommandString(
    UINT_PTR    idCmd,
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

#endif // WIZARDS
