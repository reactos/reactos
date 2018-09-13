/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    about.h

Abstract:

    header file defines CDevMgrAbout class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef __ABOUT_H_
#define __ABOUT_H_


class CDevMgrAbout : public ISnapinAbout
{
public:
    CDevMgrAbout() :m_Ref(1)
    {}
// IUNKNOWN interface
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
// ISnapinAbout interface
    STDMETHOD(GetSnapinDescription)(LPOLESTR *ppDescription);
    STDMETHOD(GetProvider)(LPOLESTR* ppProvider);
    STDMETHOD(GetSnapinVersion)(LPOLESTR *ppVersion);
    STDMETHOD(GetSnapinImage)(HICON *phIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* phSmall,
                    HBITMAP* phSmallOpen,
                    HBITMAP* phLarge,
                    COLORREF* pcrMask);
private:
    HRESULT LoadResourceOleString(int StringId, LPOLESTR* ppString);
    ULONG           m_Ref;
};

#endif
