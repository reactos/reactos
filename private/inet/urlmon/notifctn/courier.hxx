//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       courier.hxx
//
//  Contents:   the courier agent base class
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _COURIER_HXX_
#define _COURIER_HXX_

class CPackage;

class CCourierAgent : public CLifePtr
{
public:

    virtual HRESULT HandlePackage(CPackage *pCPackage) = 0;

    CCourierAgent() : CLifePtr()
    {}
    virtual ~CCourierAgent()
    {}

};
#endif // _COURIER_HXX_
