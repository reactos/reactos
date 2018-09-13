//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       factory.hxx
//
//  Contents:   CHTAClassFactory
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#ifndef __FACTORY_HXX__
#define __FACTORY_HXX__

// Forward declarations
class CHTMLApp;

class CHTAClassFactory : public IClassFactory
{
public:
    CHTAClassFactory();

    HRESULT Register();
    void Passivate();
    
    // IUnknown methods
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLApp, HTMLApp)

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID riid, void ** ppv);
    STDMETHOD(LockServer)(BOOL fLock);
    
private:
    DWORD _dwRegCookie;   // Class factory registration cookie
};

#endif  // __FACTORY_HXX__


