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

#ifndef __PEERFACT_HXX__
#define __PEERFACT_HXX__

// Forward declarations
class CHTMLApp;

class CBehaviorFactory : public IElementBehaviorFactory
{
public:
    CBehaviorFactory();

    void Passivate();
    
    // IUnknown methods
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLApp, HTMLApp)

    // IElementBehaviorFactory methods
    STDMETHOD(FindBehavior)(BSTR bstrName, BSTR bstrUrl, IElementBehaviorSite * pSite, IElementBehavior ** ppPeer);
};

#endif  // __FACTORY_HXX__


