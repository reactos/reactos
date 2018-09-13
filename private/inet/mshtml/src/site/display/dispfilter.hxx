//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispfilter.hxx
//
//  Contents:   Abstract base class for display tree filter objects which
//              perform filtering for arbitrary display nodes.
//
//----------------------------------------------------------------------------

#ifndef I_DISPFILTER_HXX_
#define I_DISPFILTER_HXX_
#pragma INCMSG("--- Beg 'dispfilter.hxx'")

class CDispNode;
class CDispContext;
class CDispDrawContext;
class CDispLayerContext;

//+---------------------------------------------------------------------------
//
//  Class:      CDispFilter
//
//  Synopsis:   Abstract base class for display tree filter objects which
//              perform filtering for arbitrary display nodes.
//
//----------------------------------------------------------------------------

class CDispFilter
{
public:
    virtual void            SetSize(
                                const SIZE& size) = 0;
    
    virtual void            DrawFiltered(CDispDrawContext* pContext) = 0;

    virtual void            Invalidate(
                                const RECT& rc,
                                BOOL fSynchronousRedraw) = 0;

    virtual void            Invalidate(
                                HRGN hrgn,
                                BOOL fSynchronousRedraw) = 0;

    virtual void            SetOpaque(
                                BOOL fOpaque) = 0;
};



#pragma INCMSG("--- End 'dispfilter.hxx'")
#else
#pragma INCMSG("*** Dup 'dispfilter.hxx'")
#endif

