/*
 * @(#)DTDState.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _DTDSTATE_HXX
#define _DTDSTATE_HXX

///////////////////////////////////////////////////////////////////////
// DTDState class
//
DEFINE_CLASS(DTDState);
DEFINE_CLASS(Name);
DEFINE_CLASS(ElementDecl);

class DTDState : public Base
{
    DECLARE_CLASS_MEMBERS(DTDState, Base);

protected:

    DTDState() {};

public:
    ~DTDState();

    static DTDState * newDTDState(Name *pName, DWORD t, ElementDecl *ped, PVOID p, int nState);

public:

    RName name;                // name 
    DWORD type;                // type
    int  state;                // state of the content model checking 
    bool matched;              // whether the element has been verified correctly
    RElementDecl ed;           // ElementDecl
    PVOID pt;   // pointer to the node that has this context 
    int  nState;               // The validation state  

    virtual void finalize()
    {
        name = null;
        ed = null;
        pt = null;
    }
};

#endif _DTDSTATE_HXX