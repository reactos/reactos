//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menu.hxx
//
//  Contents:   Declaration of CSharesCMSpecial, implementing IContextMenu
//
//  History:    20-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __MENUSP_HXX__
#define __MENUSP_HXX__

#ifdef WIZARDS

class CSharesCMSpecial : public IContextMenu
{
public:

    CSharesCMSpecial(
        IN HWND hwnd
        );

    HRESULT
    InitInstance(
        IN PWSTR pszMachine,
        IN LPCITEMIDLIST pidl,
        IN IShellFolder* psf
        );

    ~CSharesCMSpecial();

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IContextMenu methods
    //

    STDMETHOD(QueryContextMenu)(
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags
        );

    STDMETHOD(InvokeCommand)(
        LPCMINVOKECOMMANDINFO lpici
        );

    STDMETHOD(GetCommandString)(
        UINT_PTR    idCmd,
        UINT        uType,
        UINT      * pwReserved,
        LPSTR       pszName,
        UINT        cchMax
        );

private:

    PWSTR           m_pszMachine;
    LPITEMIDLIST    m_pidl;
    IShellFolder*   m_psf;
    HWND            m_hwnd;
    ULONG           m_ulRefs;
};

#endif // WIZARDS

#endif // __MENUSP_HXX__
