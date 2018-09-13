/*
 * @(#)ElementStack.cxx 1.0 6/14/97
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "elementstack.hxx" 

ElementFrame *
ElementStack::push(Element * eParent, bool fIsAttribute, bool fAddRef)
{
    ElementFrame * frame;
    int c;
    
    if (!_astkframe)
    {
        _astkframe = new (INITIAL_STACK_SIZE) AElementFrame;
    }
    else
    {
        c = _astkframe->length();

        if (_sp >= c)
        {
            // Resize the stack

            _astkframe = _astkframe->resize(2 * c);
        }
    }

    frame = &(*_astkframe)[_sp++];
    frame->init(eParent, fIsAttribute, fAddRef);
    return frame;
}


void
ElementStack::pop()
{
    Assert(_sp > 0);
    _sp--;
}


ElementFrame *
ElementStack::tos()
{
    Assert(_sp > 0);

    return &(*_astkframe)[_sp - 1];
}



