/*
 * @(#)_rawstack.cxx 1.0 3/30/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "_rawstack.hxx"

//===========================================================================
RawStack::RawStack(long entrySize, long growth)
{
    _lEntrySize = entrySize;
    _pStack = NULL;
    _ncUsed = _ncSize = 0;
    _lGrowth = growth;
}
 
RawStack::~RawStack()
{
    delete _pStack;
}

char* 
RawStack::__push()
{
    // No magic object construction -- user has to do this.
    char* newStack = new_ne char[_lEntrySize * ( _ncSize + _lGrowth) ];
    if (newStack == NULL)
    {
        return NULL;
    }
    ::memset(newStack, 0, _lEntrySize * (_ncSize + _lGrowth));
    if (_ncUsed > 0)
    {
        ::memcpy(newStack, _pStack, _lEntrySize * _ncUsed);
    }
    _ncSize += _lGrowth;
    delete _pStack;
    _pStack = newStack;

    return &_pStack[_lEntrySize * _ncUsed++];
}
