//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       elabel.hxx
//
//  Contents:   CLabelElement class
//
//  History:    10-Oct-1996 MohanB      Created
//
//----------------------------------------------------------------------------

#ifndef I_ELABEL_HXX_
#define I_ELABEL_HXX_
#pragma INCMSG("--- Beg 'elabel.hxx'")

#define _hxx_
#include "label.hdl"

MtExtern(CLabelElement)

class CLabelElement : public CElement
{
    DECLARE_CLASS_TYPES(CLabelElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLabelElement))

    CLabelElement(CDoc *pDoc)
      : CElement(ETAG_LABEL, pDoc) 
    {
#ifdef WIN16
	    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
            m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    }

    ~CLabelElement() {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

#ifndef NO_DATABINDING
    // databinding over-rides from CElement
    virtual const CDBindMethods *GetDBindMethods();
#endif // ndef NO_DATABINDING

    virtual HRESULT BUGCALL HandleMessage(CMessage *pmsg);
  
    virtual HRESULT ClickAction(CMessage * pMessage);

    static const CLSID * s_apclsidPages[];

    #define _CLabelElement_
    #include "label.hdl"

    virtual void Notify(CNotification * pNF);
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    RECT _rcWobbleZone;
    BOOL _fCanClick:1;

    NO_COPY(CLabelElement);
};

#pragma INCMSG("--- End 'elabel.hxx'")
#else
#pragma INCMSG("*** Dup 'elabel.hxx'")
#endif
