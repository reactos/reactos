//+----------------------------------------------------------------------------
//
// File:        ILAYOUT.HXX
//
// Contents:    IHTMLLayout interface declaration
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_ILAYOUT_HXX_
#define I_ILAYOUT_HXX_
#pragma INCMSG("--- Beg 'ilayout.hxx'")

class CCalcInfo;

interface IHTMLLayout : IUnknown
{
     STDMETHOD(DoLayout(CCalcInfo *pci,
                        SIZE      *psize,
                        SIZE      *psizeDefault,
                        long      *pcch)) PURE;

     //HRESULT GetDisplayNode(IDispNode **pNode);
};

// {3050f433-98b5-11cf-bb82-00aa00bdce0b}
DEFINE_GUID(IID_IHTMLLayout,
    0x3050f433, 0x98b5, 0x11cf, 0xbb, 0x82, 0x0, 0xaa, 0x00, 0xbd, 0xce, 0xb);

#pragma INCMSG("--- End 'ilayout.hxx'")
#else
#pragma INCMSG("*** Dup 'ilayout.hxx'")
#endif
