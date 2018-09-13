//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       shares.hxx
//
//  Contents:   Declaration of COM object CShares
//
//  History:    13-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __SHARES_HXX__
#define __SHARES_HXX__

#include "sfolder.hxx"
#include "pfolder.hxx"
#include "rcomp.hxx"

//////////////////////////////////////////////////////////////////////////////

class CShares : public IUnknown
{
    friend class CSharesSF;
    friend class CSharesPF;
    friend class CSharesRC;

public:

    CShares()
        :
        m_ulRefs(0),
        m_pszMachine(NULL),
        m_pidl(NULL),
        m_level(0),
        m_pMenuBg(NULL)
    {
        AddRef();
    }

    ~CShares()
    {
        delete[] m_pszMachine;

        if (NULL != m_pidl)
        {
            ILFree(m_pidl);
        }
    }

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

private:

    // interface implementations
    CSharesSF       m_ShellFolder;
    CSharesPF       m_PersistFolder;
    CSharesRC       m_RemoteComputer;

    // data
    ULONG           m_ulRefs;
    PWSTR           m_pszMachine;   // machine to work on
    LPITEMIDLIST    m_pidl;
    ULONG           m_level;        // share info level: 1 or 2
    IContextMenu*   m_pMenuBg;
};

#endif // __SHARES_HXX__
