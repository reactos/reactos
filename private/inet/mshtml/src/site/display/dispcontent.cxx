//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontent.cxx
//
//  Contents:   An abstract interior node which represents content
//              (vs. a balancing node)
//
//  Classes:    CDispContentNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPCONTENT_HXX_
#define X_DISPCONTENT_HXX_
#include "dispcontent.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CDispContentNode::GetZOrder
//              
//  Synopsis:   Return the z order of this node.
//              
//  Arguments:  none
//              
//  Returns:    Z order of this node.
//              
//  Notes:      This method shouldn't be called unless the node is
//              in the negative or positive Z layers.
//              
//----------------------------------------------------------------------------

LONG
CDispContentNode::GetZOrder() const
{
    Assert(GetLayerType() == DISPNODELAYER_NEGATIVEZ ||
           GetLayerType() == DISPNODELAYER_POSITIVEZ);
    
    return _pDispClient->GetZOrderForSelf();
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispContentNode::DumpInfo
//              
//  Synopsis:   Dump custom information for this node.
//              
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispContentNode::DumpInfo(HANDLE hFile, long level, long childNumber)
{
#if 0
    IDispClientDebug* pIDebug;
    if (SUCCEEDED(
        _pDispClient->QueryInterface(IID_IDispClientDebug,(void**)&pIDebug)))
    {
        pIDebug->DumpDebugInfo(hFile, level, childNumber, this, 0);
        pIDebug->Release();
    }
#else
    _pDispClient->DumpDebugInfo(hFile, level, childNumber, this, 0);
#endif
}
#endif
