//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menu.hxx
//
//  Contents:   Declaration of CSharesCM, implementing IContextMenu
//
//  History:    20-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __MENU_HXX__
#define __MENU_HXX__

class CSharesCM : public IContextMenu
{
public:

    CSharesCM(
        IN HWND hwnd
        );

    HRESULT
    InitInstance(
        IN PWSTR pszMachine,
        IN UINT cidl,
        IN LPCITEMIDLIST* apidl,
        IN IShellFolder* psf
        );

    ~CSharesCM();

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
        UINT        idCmd,
        UINT        uType,
        UINT      * pwReserved,
        LPSTR       pszName,
        UINT        cchMax
        );

private:

    PWSTR           m_pszMachine;
    UINT            m_cidl;
    LPITEMIDLIST*   m_apidl;
    HWND            m_hwnd;
    IShellFolder*   m_psf;
    ULONG           m_ulRefs;
};

#endif // __MENU_HXX__
