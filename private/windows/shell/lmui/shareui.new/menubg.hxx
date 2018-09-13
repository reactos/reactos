//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       menubg.hxx
//
//  Contents:   Declaration of CSharesCMBG, implementing IContextMenu for the
//              background
//
//  History:    20-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __MENUBG_HXX__
#define __MENUBG_HXX__

class CSharesCMBG : public IContextMenu
{
public:
    CSharesCMBG(
        IN HWND hwnd,
        IN PWSTR pszMachine,
        IN ULONG level
        )
        :
        m_ulRefs(0),
        m_hwnd(hwnd),
        m_pszMachine(pszMachine),
        m_level(level)
    {
        AddRef();
    }

    ~CSharesCMBG() {}

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

    HWND  m_hwnd;
    PWSTR m_pszMachine;
    ULONG m_level;
    ULONG m_ulRefs;
};

#endif // __MENUBG_HXX__
