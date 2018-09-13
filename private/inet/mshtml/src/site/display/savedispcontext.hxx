//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       savedispcontext.hxx
//
//  Contents:   Utility class to save interesting parts of the display context
//              during tree traversal.
//
//----------------------------------------------------------------------------

#ifndef I_SAVEDISPCONTEXT_HXX_
#define I_SAVEDISPCONTEXT_HXX_
#pragma INCMSG("--- Beg 'saveddispcontext.hxx'")

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

class CSaveDispContext
{
public:
    CSaveDispContext(CDispContext* pContext)
            {_pOriginalContext = pContext; _saveContext = *pContext;}
    ~CSaveDispContext()
            {*_pOriginalContext = _saveContext;}
    
    CDispContext*               _pOriginalContext;
    CDispContext                _saveContext;
};

#pragma INCMSG("--- End 'saveddispcontext.hxx'")
#else
#pragma INCMSG("*** Dup 'saveddispcontext.hxx'")
#endif
