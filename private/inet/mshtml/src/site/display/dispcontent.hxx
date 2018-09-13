//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontent.hxx
//
//  Contents:   An abstract interior node which represents content
//              (vs. a balancing node)
//
//----------------------------------------------------------------------------

#ifndef I_DISPCONTENT_HXX_
#define I_DISPCONTENT_HXX_
#pragma INCMSG("--- Beg 'dispcontent.hxx'")

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CDispContentNode
//
//  Contents:   An abstract interior node which represents content
//              (vs. a balancing node)
//
//----------------------------------------------------------------------------

class CDispContentNode :
    public CDispInteriorNode
{
    DECLARE_DISPNODE_ABSTRACT(CDispContentNode, CDispInteriorNode)
    
    friend class CDispNode;
    
protected:
    CDispClient*    _pDispClient;
    // 52 bytes (4 bytes + 48 bytes for CDispInteriorNode super class)
    
    // object can be created only by derived classes, and destructed only from
    // special methods
protected:
                            CDispContentNode(CDispClient* pDispClient)
                                : CDispInteriorNode()
                                    {Assert(pDispClient != NULL);
                                    _pDispClient = pDispClient;}
    virtual                 ~CDispContentNode() {}
    
public:
    // CDispNode overrides
    virtual LONG            GetZOrder() const;
    virtual CDispClient*    GetDispClient() const {return _pDispClient;}
    
protected:
    // CDispNode overrides    
#if DBG==1
    virtual void            DumpInfo(HANDLE hFile, long level, long childNumber);
#endif
};


#pragma INCMSG("--- End 'dispcontent.hxx'")
#else
#pragma INCMSG("*** Dup 'dispcontent.hxx'")
#endif

