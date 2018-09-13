//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispflags.hxx
//
//  Contents:   Definition of bit flags used by display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPFLAGS_HXX_
#define I_DISPFLAGS_HXX_
#pragma INCMSG("--- Beg 'dispflags.hxx'")

#ifndef X_FLAGS_HXX_
#define X_FLAGS_HXX_
#include "flags.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

class CDispInfo;


//+---------------------------------------------------------------------------
//
//  Class:      CDispFlags
//
//  Synopsis:   Bit flags used by display tree.
//
//----------------------------------------------------------------------------

class CDispFlags :
    public CFlags32
{
public:
                            CDispFlags() {}
                            CDispFlags(const CDispFlags& flags)
                                : CFlags32(flags) {}
                            CDispFlags(LONG flagValue)
                                : CFlags32(flagValue) {}
                            ~CDispFlags() {}
    
    
public:
    //
    // Masks to categorize usage of flag bits.
    //
    
    // GENERAL FLAGS, used by all node types
    static const CDispFlags s_generalMask;
    // SHARED FLAGS, overlapping usage by interior nodes and leaf nodes
    static const CDispFlags s_sharedMask;
    // PROPAGATED FLAGS, which are copied up the tree when they are set
    static const CDispFlags s_propagatedMask;
    
    
    //
    // === GENERAL flags ===
    // 
    
    // layer type
    static const CDispFlags s_layerType;
    static const int s_layerTypeShift;
    
    // set if this node is visible (CSS visibility)
    static const CDispFlags s_visibleNode;
    
    // right-to-left coordinate system    
    static const CDispFlags s_rightToLeft;
    
    // the client maintains no pointers to this node, so destruct 
    // it when parent is destructed
    static const CDispFlags s_owned;

    // mark this node for destruction by Recalc or parent destructor
    static const CDispFlags s_destruct;
    
    // mark interior nodes (descendants of CDispInteriorNode which may have children)
    static const CDispFlags s_interiorNode;
        
    // mark nodes which display a background
    static const CDispFlags s_hasBackground;
    
    // mark nodes which have information stored in lookaside cache
    static const CDispFlags s_hasLookaside;
    
    // flag a buffered node that has a partially invalid buffer
    static const CDispFlags s_bufferInvalid;
        
    // flag to force invalidation of this node during recalc (suppresses
    // invalidation of all children)
    static const CDispFlags s_inval;
    
    // flag to force recalc of this node's children
    static const CDispFlags s_recalcChildren;
            
    // flag node which opaquely draws every pixel in its bounds (_rcVisBounds for
    // CDispLeafNodes, _rcContainer for CDispContainer nodes) 
    static const CDispFlags s_opaqueNode;
    
    // this node does not affect calculation of content bounds for scrollbars
    static const CDispFlags s_noScrollBounds;
    
    // this node is filtered through CDispFilter
    static const CDispFlags s_filtered;
    
    // this node has user clip in its extras
    static const CDispFlags s_hasUserClip;
    
    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    //
    static const CDispFlags s_fatHitTest;
    
    
    //
    // === LEAF NODE flags === (overlap with interior node flags)
    // 
    
    // mark a leaf node whose position has changed (s_positionChange nodes only)
    static const CDispFlags s_positionHasChanged;

    // mark a leaf node that needs just inserted notifications
    static const CDispFlags s_needsJustInserted;

    // mark a leaf node which just inserted into the tree
    static const CDispFlags s_justInserted;
    
    
    //
    // === INTERIOR NODE flags === (overlap with leaf node flags)
    // 
    
    // mark balance nodes, which don't appear in client-directed tree traversals
    static const CDispFlags s_balanceNode;
    
    // this node has children that it must destruct during Recalc
    static const CDispFlags s_destructChildren;
    
    // flag fixed background
    static const CDispFlags s_fixedBackground;
    
    
    //
    // === PROPAGATED FLAGS ===
    //

    // flag branch which contains something opaque that saved the redraw region
    static const CDispFlags s_savedRedrawRegion;
    
    // branch which needs recalc
    static const CDispFlags s_recalc;
        
    // node in view (not completely clipped by container)
    static const CDispFlags s_inView;

    // set if any node in this branch is visible
    static const CDispFlags s_visibleBranch;

    // branch contains opaquely rendered content
    static const CDispFlags s_opaqueBranch;

    // item which has a back buffer containing pixels underneath it
    static const CDispFlags s_buffered;

    // node which needs position change notification
    static const CDispFlags s_positionChange;
    
    // node which needs in view change notification
    static const CDispFlags s_inViewChange;
    
    
    //
    // === COMBINATION flags ===
    // 
    // Using these combinations is faster than creating your own, even if
    // you make them static const.
    // 
    
    // no flags set
    static const CDispFlags s_none;
    
    // s_inval + s_recalc + s_recalcChildren
    static const CDispFlags s_invalAndRecalcChildren;
    
    // s_propagatedMask + s_inval + s_recalcChildren
    static const CDispFlags s_propagatedAndRecalcAndInval;
    
    // s_layerType + s_rightToLeft + s_visibleNode + s_owned + s_hasBackground +
    //  s_noScrollBounds + s_filtered + s_hasUserClip
    static const CDispFlags s_containerConstructMask;
    
    // s_positionHasChanged + s_inval
    static const CDispFlags s_positionHasChangedAndInval;
    
    // s_layerType + s_inViewChange
    static const CDispFlags s_balanceGroupSelector;
    
    // s_balanceNode
    static const CDispFlags s_balanceGroupInit;
    
    // s_visibleBranch + s_inView
    static const CDispFlags s_visibleBranchAndInView;
    
    // s_visibleBranch + s_inval
    static const CDispFlags s_visibleBranchAndInval;
    
    // s_inViewChange + s_visibleBranch
    static const CDispFlags s_inViewChangeAndVisibleBranch;
    
    // s_inViewChange + s_needsJustInserted
    static const CDispFlags s_inViewChangeOrNeedsInsertedNotify;
    
    // s_visibleNode + s_visibleBranch
    static const CDispFlags s_visibleNodeAndBranch;
    
    // s_inView + s_inViewChange
    static const CDispFlags s_inViewAndInViewChange;
    
    // s_positionHasChanged + s_recalc
    static const CDispFlags s_positionHasChangedAndRecalc;
    
    // s_destruct + s_owned
    static const CDispFlags s_destructOrOwned;
    
    // s_destruct + s_inval
    static const CDispFlags s_destructAndInval;
    
    // s_visibleBranch + s_inView + s_opaqueBranch
    static const CDispFlags s_preDrawSelector;
    
    // s_visibleBranch + s_inView
    static const CDispFlags s_drawSelector;
    
    // s_visibleNode + s_inView
    static const CDispFlags s_visible;
    
    // s_visibleNode + s_inView + s_inval
    static const CDispFlags s_invalAndVisible;
    
    // s_opaqueBranch + s_visibleBranch + s_inView
    static const CDispFlags s_subtractOpaqueSelector;
    
    // s_opaqueNode + s_opaqueBranch
    static const CDispFlags s_opaqueNodeAndBranch;
    
    // s_inView + s_recalcChildren + s_recalc + s_inval + s_opaqueBranch
    static const CDispFlags s_clearInRecalc;
    
    // s_interiorNode + s_balance
    static const CDispFlags s_interiorAndBalanceNode;
    
    // s_destruct + s_balance
    static const CDispFlags s_destructOrBalanceNode;
    
    // s_destruct + s_recalcChildren + s_recalc
    static const CDispFlags s_generalFlagsNotSetInDraw;
    
    // s_destructChildren
    static const CDispFlags s_interiorFlagsNotSetInDraw;
    
#if DBG==1
    // special flag pattern to verify that CDispNode destructor is being
    // called from the right place
    static const CDispFlags s_debugDestruct;
#endif

    // fast node type checks
    BOOL                    IsLeafNode() const
                                    {return !IsSet(s_interiorNode);}
    BOOL                    IsInteriorNode() const
                                    {return IsSet(s_interiorNode);}
    BOOL                    IsBalanceNode() const
                                    {return AllSet(s_interiorAndBalanceNode);}
    
    // layer type
    DISPNODELAYER           GetLayerType() const
                                    // for speed:
                                    {return (DISPNODELAYER)
                                        (_flags & s_layerType._flags);}
                                    //{return (DISPNODELAYER)
                                    //    GetValue(s_layerType, s_layerTypeShift);}
    void                    SetLayerType(DISPNODELAYER layerType)
                                    // for speed:
                                    {_flags = 
                                        (_flags&~s_layerType._flags)|layerType;}
                                    //{SetValue((LONG)layerType,
                                    //          s_layerType, s_layerTypeShift);}
    
    // background
    BOOL                    HasBackground() const
                                    {return IsSet(s_hasBackground);}
    void                    SetBackground(BOOL fHasBackground)
                                    {SetBoolean(s_hasBackground, fHasBackground);}
    BOOL                    HasFixedBackground() const
                                    {return IsSet(s_fixedBackground);}
    void                    SetFixedBackground(BOOL fFixedBackground)
                                    {SetBoolean(s_fixedBackground, fFixedBackground);}
    
    // convenience method for ownership
    BOOL                    IsOwned() const
                                    {return IsSet(s_owned);}
    void                    SetOwned(BOOL fOwned)
                                    {SetBoolean(s_owned, fOwned);}
    
#if DBG==1
    static void             CheckFlagIntegrity();
#endif
};


#endif _DISPFLAGS_HXX_


