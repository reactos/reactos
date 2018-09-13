//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispbalance.hxx
//
//  Contents:   Synthetic node used to group similar nodes for tree efficiency.
//
//----------------------------------------------------------------------------

#ifndef I_DISPBALANCE_HXX_
#define I_DISPBALANCE_HXX_
#pragma INCMSG("--- Beg 'dispbalance.hxx'")


#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif

MtExtern(CDispBalanceNode)

//+---------------------------------------------------------------------------
//
//  Class:      CDispBalanceNode
//
//  Synopsis:   Synthetic node used to group similar nodes for tree efficiency.
//
//----------------------------------------------------------------------------

class CDispBalanceNode :
    public CDispInteriorNode
{
    DECLARE_DISPNODE(CDispBalanceNode, CDispInteriorNode, DISPNODETYPE_BALANCE)
    
    // object can be created only by CDispInteriorNode, and destructed only from
    // special methods
protected:
    friend class CDispInteriorNode;
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispBalanceNode))

                            CDispBalanceNode()
                                : CDispInteriorNode()
                                    {SetFlag(CDispFlags::s_balanceGroupInit);}
                            ~CDispBalanceNode() {}
                            
public:
    // CDispNode overrides
    virtual LONG            GetZOrder() const
                                    {Assert(FALSE); // shouldn't be here!
                                     return 0;}
    
};


#pragma INCMSG("--- End 'dispbalance.hxx'")
#else
#pragma INCMSG("*** Dup 'dispbalance.hxx'")
#endif

