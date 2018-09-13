//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       generic.hxx
//
//  Contents:   CGenericElement
//
//----------------------------------------------------------------------------

#ifndef I_GENERIC_HXX_
#define I_GENERIC_HXX_
#pragma INCMSG("--- Beg 'generic.hxx'")

#define _hxx_
#include "generic.hdl"

MtExtern(CGenericElement)

//+---------------------------------------------------------------------------
//
//  CGenericElement
//
//----------------------------------------------------------------------------

class CGenericElement : public CElement
{
    DECLARE_CLASS_TYPES(CGenericElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CGenericElement))

    //
    // consruction / destruction / init
    //

    static HRESULT CreateElement(
        CHtmTag * pht, CDoc * pDoc, CElement ** ppElement);
    
    CGenericElement (CHtmTag * pht, CDoc *pDoc);

    virtual HRESULT Init2(CInit2Context * pContext);
    virtual void    Notify (CNotification * pnf);

    //
    // misc
    //

    HRESULT Save (CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd);

    //
    // traditional wiring
    //

    #define _CGenericElement_
    #include "generic.hdl"

    DECLARE_CLASSDESC_MEMBERS;

    //
    // misc
    //

#ifndef VSTUDIO7
    virtual const TCHAR * TagName()     { return _cstrTagName;   }
    virtual const TCHAR * Namespace()   { return _cstrNamespace; }
#endif //VSTUDIO7

    //
    // data
    //

#ifndef VSTUDIO7
    CStr _cstrTagName;
    CStr _cstrNamespace;
#endif //VSTUDIO7
    CStr _cstrContents;

    NO_COPY(CGenericElement);
};

#pragma INCMSG("--- End 'generic.hxx'")
#else
#pragma INCMSG("*** Dup 'generic.hxx'")
#endif
