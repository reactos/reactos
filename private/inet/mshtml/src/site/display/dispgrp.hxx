//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispgrp.hxx
//
//  Contents:   Generic grouping node for display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPGRP_HXX_
#define I_DISPGRP_HXX_
#pragma INCMSG("--- Beg 'dispgrp.hxx'")

#ifndef X_DISPCONTENT_HXX_
#define X_DISPCONTENT_HXX_
#include "dispcontent.hxx"
#endif

MtExtern(CDispGroup)

//+---------------------------------------------------------------------------
//
//  Class:      CDispGroup
//
//  Synopsis:   Generic grouping node for display tree.
//
//----------------------------------------------------------------------------

class CDispGroup :
    public CDispContentNode
{
    DECLARE_DISPNODE(CDispGroup, CDispContentNode, DISPNODETYPE_GROUP)

    // object can be created only by CDispRoot, and destructed only from
    // special methods
protected:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispGroup))

                            CDispGroup(CDispClient* pDispClient)
                                : CDispContentNode(pDispClient) {}
                            ~CDispGroup() {}
};


#pragma INCMSG("--- End 'dispgrp.hxx'")
#else
#pragma INCMSG("*** Dup 'dispgrp.hxx'")
#endif
