/*
 * @(#)DTDState.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "dtdstate.hxx"

DEFINE_CLASS_MEMBERS(DTDState, _T("DTDState"), Base);

DTDState::~DTDState()
{
    name = null;
    ed = null;
    pt = null;
}

DTDState* DTDState::newDTDState(Name *pName, DWORD t, ElementDecl *ped, PVOID p, int nState)
{
    DTDState* s = new DTDState();
    s->name = pName;
    s->type = t;
    s->ed = ped;
    if (ped == null)
        s->matched = true;
    else
        s->matched = false;
    s->state = 0;
    s->pt = p;
    s->nState = nState;
    return s;
}