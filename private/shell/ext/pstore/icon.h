/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    icon.h

Abstract:

    This module implements icon handling code for the protected storage
    explorer.

    The shell uses these interfaces to retrieve icons associated with
    folders in the protected storage namespace.

Author:

    Scott Field (sfield)    11-Mar-97

--*/

#ifndef ICON_H
#define ICON_H

class CExtractIcon : public IExtractIcon
{

protected:
    LONG m_ObjRefCount;

public:
    CExtractIcon(LPCITEMIDLIST pidl);
    ~CExtractIcon();

    //
    // IUnknown methods
    //

    STDMETHOD (QueryInterface) (REFIID riid, LPVOID * ppvObj);
    STDMETHOD_ (ULONG, AddRef) (void);
    STDMETHOD_ (ULONG, Release) (void);

    //
    // IExtractIcon methods
    //

    STDMETHOD (GetIconLocation) (UINT, LPTSTR, UINT, LPINT, LPUINT);
    STDMETHOD (Extract) (LPCTSTR, UINT, HICON*, HICON*, UINT);

private:
	DWORD m_dwType;
	PST_KEY m_KeyType;
};

#endif   // ICON_H
