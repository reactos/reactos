//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispflags.cxx
//
//  Contents:   Definition of bit flags used by display tree.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPFLAGS_HXX_
#define X_DISPFLAGS_HXX_
#include "dispflags.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

// NOTE: we define all flag values here to avoid any ordering problems with
// static initialization

//
// Here is the general allocation scheme for these bits.
//

// GENERAL FLAGS, used by all node types
#define DISPFLAGS_GENERALMASK           0x0003FFFF
// SHARED FLAGS, overlapping usage by interior nodes and leaf nodes
#define DISPFLAGS_SHAREDMASK            0x001C0000
// PROPAGATED FLAGS, which are copied up the tree when they are set
#define DISPFLAGS_PROPAGATEDMASK        0xFF000000
// UNUSED FLAGS
#define DISPFLAGS_UNUSEDMASK            0x00E00000

//
// === GENERAL flags ===
// 

// layer type
#define DISPFLAGS_LAYERTYPE             (0x07 << 0)
#define DISPFLAGS_LAYERTYPE_SHIFT       0

// set if this node is visible (CSS visibility)
#define DISPFLAGS_VISIBLENODE           (1 << 3)

// right-to-left coordinate system
#define DISPFLAGS_RIGHTTOLEFT           (1 << 4)


// lifetime flag set when the client maintains pointers to this node
#define DISPFLAGS_OWNED                 (1 << 5)

// mark this node for destruction by Recalc or parent destructor
#define DISPFLAGS_DESTRUCT              (1 << 6)

// mark interior nodes (descendants of CDispInteriorNode which may have children)
#define DISPFLAGS_INTERIORNODE          (1 << 7)

// mark nodes which display a background
#define DISPFLAGS_HASBACKGROUND         (1 << 8)

// mark nodes which have information stored in lookaside cache
#define DISPFLAGS_HASLOOKASIDE          (1 << 9)

// flag a buffered node that has a partially invalid buffer
#define DISPFLAGS_BUFFERINVALID         (1 << 10)

// flag to force invalidation of this node during recalc (suppresses
// invalidation of all children)
#define DISPFLAGS_INVAL                 (1 << 11)

// flag to force recalc of this node's children
#define DISPFLAGS_RECALCCHILDREN        (1 << 12)

// flag node which opaquely draws every pixel in its bounds (_rcVisBounds for
// CDispLeafNodes, _rcContainer for CDispContainer nodes) 
#define DISPFLAGS_OPAQUENODE            (1 << 13)

// this node does not affect calculation of content bounds for scrollbars
#define DISPFLAGS_NOSCROLLBOUNDS        (1 << 14)

// this node is filtered through CDispFilter
#define DISPFLAGS_FILTERED              (1 << 15)

// this node has user clip rect in its extras
#define DISPFLAGS_HASUSERCLIP           (1 << 16)

//
// BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
//
#define DISPFLAGS_FATHITTEST            (1 << 17)


//
// === LEAF NODE flags === (overlap with interior node flags)
// 

// mark a leaf node whose position has changed (s_positionChange nodes only)
#define DISPFLAGS_POSITIONHASCHANGED    (1 << 18)

// mark a leaf node that needs just inserted notifications
#define DISPFLAGS_NEEDSJUSTINSERTED     (1 << 19)

// mark a leaf node which just inserted into the tree
#define DISPFLAGS_JUSTINSERTED          (1 << 20)


//
// === INTERIOR NODE flags === (overlap with leaf node flags)
// 

// mark balance nodes, which don't appear in client-directed tree traversals
#define DISPFLAGS_BALANCENODE           (1 << 18)

// this node has children that it must destruct during Recalc
#define DISPFLAGS_DESTRUCTCHILDREN      (1 << 19)

// flag fixed background
#define DISPFLAGS_FIXEDBACKGROUND       (1 << 20)


#define DISPFLAGS_UNUSED1               (0x0E << 21)


//
// === PROPAGATED FLAGS ===
//

// flag a branch which contains a BLOCKSDRAWING item that has saved
// a redraw region
#define DISPFLAGS_SAVEDREDRAWREGION     (1 << 24)

// branch which needs recalc
#define DISPFLAGS_RECALC                (1 << 25)

// node in view (not completely clipped by container)
#define DISPFLAGS_INVIEW                (1 << 26)

// set if any node in this branch is visible
#define DISPFLAGS_VISIBLEBRANCH         (1 << 27)

// branch contains opaquely rendered content
#define DISPFLAGS_OPAQUEBRANCH          (1 << 28)

// item which has a back buffer containing pixels underneath it
#define DISPFLAGS_BUFFERED              (1 << 29)

// node which needs position change notification
#define DISPFLAGS_POSITIONCHANGE        (1 << 30)

// node which needs in view change notification
#define DISPFLAGS_INVIEWCHANGE          (1 << 31)


#define DEFINE_FLAG(FLAGNAME,FLAGVALUE) \
    const CDispFlags CDispFlags::FLAGNAME (FLAGVALUE);
#define DEFINE_SHIFT(SHIFTNAME,SHIFTVALUE) \
    const int CDispFlags::SHIFTNAME (SHIFTVALUE);
    
// initialize static values
DEFINE_FLAG(    s_generalMask,          DISPFLAGS_GENERALMASK)
DEFINE_FLAG(    s_sharedMask,           DISPFLAGS_SHAREDMASK)
DEFINE_FLAG(    s_propagatedMask,       DISPFLAGS_PROPAGATEDMASK)
// === GENERAL flags ===
DEFINE_FLAG(    s_layerType,            DISPFLAGS_LAYERTYPE)
DEFINE_SHIFT(   s_layerTypeShift,       DISPFLAGS_LAYERTYPE_SHIFT)
DEFINE_FLAG(    s_visibleNode,          DISPFLAGS_VISIBLENODE)
DEFINE_FLAG(    s_rightToLeft,          DISPFLAGS_RIGHTTOLEFT)
DEFINE_FLAG(    s_owned,                DISPFLAGS_OWNED)
DEFINE_FLAG(    s_destruct,             DISPFLAGS_DESTRUCT)
DEFINE_FLAG(    s_interiorNode,         DISPFLAGS_INTERIORNODE)
DEFINE_FLAG(    s_hasBackground,        DISPFLAGS_HASBACKGROUND)
DEFINE_FLAG(    s_hasLookaside,         DISPFLAGS_HASLOOKASIDE)
DEFINE_FLAG(    s_bufferInvalid,        DISPFLAGS_BUFFERINVALID)
DEFINE_FLAG(    s_inval,                DISPFLAGS_INVAL)
DEFINE_FLAG(    s_recalcChildren,       DISPFLAGS_RECALCCHILDREN)
DEFINE_FLAG(    s_opaqueNode,           DISPFLAGS_OPAQUENODE)
DEFINE_FLAG(    s_noScrollBounds,       DISPFLAGS_NOSCROLLBOUNDS)
DEFINE_FLAG(    s_filtered,             DISPFLAGS_FILTERED)
DEFINE_FLAG(    s_hasUserClip,          DISPFLAGS_HASUSERCLIP)

//
// BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
//
DEFINE_FLAG(    s_fatHitTest,           DISPFLAGS_FATHITTEST)

// === LEAF NODE flags === (overlap with interior node flags)
DEFINE_FLAG(    s_positionHasChanged,   DISPFLAGS_POSITIONHASCHANGED)
DEFINE_FLAG(    s_needsJustInserted,    DISPFLAGS_NEEDSJUSTINSERTED)
DEFINE_FLAG(    s_justInserted,         DISPFLAGS_JUSTINSERTED)
// === INTERIOR NODE flags === (overlap with leaf node flags)
DEFINE_FLAG(    s_balanceNode,          DISPFLAGS_BALANCENODE)
DEFINE_FLAG(    s_destructChildren,     DISPFLAGS_DESTRUCTCHILDREN)
DEFINE_FLAG(    s_fixedBackground,      DISPFLAGS_FIXEDBACKGROUND)
// === PROPAGATED FLAGS ===
DEFINE_FLAG(    s_savedRedrawRegion,    DISPFLAGS_SAVEDREDRAWREGION)
DEFINE_FLAG(    s_recalc,               DISPFLAGS_RECALC)
DEFINE_FLAG(    s_inView,               DISPFLAGS_INVIEW)
DEFINE_FLAG(    s_visibleBranch,        DISPFLAGS_VISIBLEBRANCH)
DEFINE_FLAG(    s_opaqueBranch,         DISPFLAGS_OPAQUEBRANCH)
DEFINE_FLAG(    s_buffered,             DISPFLAGS_BUFFERED)
DEFINE_FLAG(    s_positionChange,       DISPFLAGS_POSITIONCHANGE)
DEFINE_FLAG(    s_inViewChange,         DISPFLAGS_INVIEWCHANGE)


//
// === COMBINATION flags ===
// 
// Using these combinations is faster than creating your own, even if
// you make them static const.
// 

#if DBG==1
// special flag pattern to verify that CDispNode destructor is being
// called from the right place
DEFINE_FLAG(s_debugDestruct, 0xdeadbeef)
#endif

    
DEFINE_FLAG(s_none, 0)

DEFINE_FLAG(s_invalAndRecalcChildren,
            DISPFLAGS_INVAL |
            DISPFLAGS_RECALC |
            DISPFLAGS_RECALCCHILDREN)

DEFINE_FLAG(s_propagatedAndRecalcAndInval,
            DISPFLAGS_PROPAGATEDMASK |
            DISPFLAGS_INVAL |
            DISPFLAGS_RECALCCHILDREN)

DEFINE_FLAG(s_containerConstructMask,
            DISPFLAGS_LAYERTYPE |
            DISPFLAGS_RIGHTTOLEFT |
            DISPFLAGS_VISIBLENODE |
            DISPFLAGS_OWNED |
            DISPFLAGS_HASBACKGROUND |
            DISPFLAGS_NOSCROLLBOUNDS |
            DISPFLAGS_FILTERED |
            DISPFLAGS_HASUSERCLIP)

DEFINE_FLAG(s_positionHasChangedAndInval,
            DISPFLAGS_POSITIONHASCHANGED |
            DISPFLAGS_INVAL)
    
DEFINE_FLAG(s_balanceGroupSelector,
            DISPFLAGS_LAYERTYPE |
            DISPFLAGS_INVIEWCHANGE)

DEFINE_FLAG(s_balanceGroupInit,
            DISPFLAGS_BALANCENODE)

DEFINE_FLAG(s_visibleBranchAndInView,
            DISPFLAGS_VISIBLEBRANCH |
            DISPFLAGS_INVIEW)

DEFINE_FLAG(s_visibleBranchAndInval,
            DISPFLAGS_VISIBLEBRANCH |
            DISPFLAGS_INVAL)

DEFINE_FLAG(s_inViewChangeAndVisibleBranch,
            DISPFLAGS_INVIEWCHANGE |
            DISPFLAGS_VISIBLEBRANCH);
    
DEFINE_FLAG(s_inViewChangeOrNeedsInsertedNotify,
            DISPFLAGS_INVIEWCHANGE |
            DISPFLAGS_NEEDSJUSTINSERTED);
    
DEFINE_FLAG(s_visibleNodeAndBranch,
            DISPFLAGS_VISIBLENODE |
            DISPFLAGS_VISIBLEBRANCH);
    
DEFINE_FLAG(s_inViewAndInViewChange,
            DISPFLAGS_INVIEW |
            DISPFLAGS_INVIEWCHANGE);
    
DEFINE_FLAG(s_positionHasChangedAndRecalc,
            DISPFLAGS_POSITIONHASCHANGED |
            DISPFLAGS_RECALC)

DEFINE_FLAG(s_destructOrOwned,
            DISPFLAGS_DESTRUCT |
            DISPFLAGS_OWNED)

DEFINE_FLAG(s_destructAndInval,
            DISPFLAGS_DESTRUCT |
            DISPFLAGS_INVAL)

DEFINE_FLAG(s_preDrawSelector,
            DISPFLAGS_VISIBLEBRANCH |
            DISPFLAGS_INVIEW |
            DISPFLAGS_OPAQUEBRANCH)

DEFINE_FLAG(s_drawSelector,
            DISPFLAGS_VISIBLEBRANCH |
            DISPFLAGS_INVIEW)

DEFINE_FLAG(s_visible,
            DISPFLAGS_VISIBLENODE |
            DISPFLAGS_INVIEW)

DEFINE_FLAG(s_invalAndVisible,
            DISPFLAGS_INVAL |
            DISPFLAGS_VISIBLENODE |
            DISPFLAGS_INVIEW)

DEFINE_FLAG(s_subtractOpaqueSelector,
            DISPFLAGS_OPAQUEBRANCH |
            DISPFLAGS_VISIBLEBRANCH |
            DISPFLAGS_INVIEW)

DEFINE_FLAG(s_opaqueNodeAndBranch,
            DISPFLAGS_OPAQUENODE |
            DISPFLAGS_OPAQUEBRANCH)

DEFINE_FLAG(s_clearInRecalc,
            DISPFLAGS_INVIEW |
            DISPFLAGS_RECALCCHILDREN |
            DISPFLAGS_RECALC |
            DISPFLAGS_INVAL |
            DISPFLAGS_OPAQUEBRANCH)

DEFINE_FLAG(s_interiorAndBalanceNode,
            DISPFLAGS_INTERIORNODE |
            DISPFLAGS_BALANCENODE)

DEFINE_FLAG(s_generalFlagsNotSetInDraw,
            DISPFLAGS_DESTRUCT |
            DISPFLAGS_RECALCCHILDREN |
            DISPFLAGS_RECALC)

DEFINE_FLAG(s_interiorFlagsNotSetInDraw,
            DISPFLAGS_DESTRUCTCHILDREN)


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispFlags::CheckFlagIntegrity
//              
//  Synopsis:   Make sure we didn't mess up our flag allocations.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispFlags::CheckFlagIntegrity()
{
    //
    // NOTE:  The following assertions will only fire if the definition of
    //        CDispFlags is inconsistent in some way.
    //        
    
    // masks are mutually exclusive
    Assert(!CDispFlags::s_generalMask.IsSet(CDispFlags::s_sharedMask));
    Assert(!CDispFlags::s_generalMask.IsSet(CDispFlags::s_propagatedMask));
    Assert(!(DISPFLAGS_GENERALMASK & DISPFLAGS_UNUSEDMASK));
    Assert(!CDispFlags::s_sharedMask.IsSet(CDispFlags::s_propagatedMask));
    Assert(!(DISPFLAGS_SHAREDMASK & DISPFLAGS_UNUSEDMASK));
    Assert(!(DISPFLAGS_PROPAGATEDMASK & DISPFLAGS_UNUSEDMASK));
    
    // masks cover all bits
    Assert((DISPFLAGS_GENERALMASK | DISPFLAGS_SHAREDMASK |
           DISPFLAGS_PROPAGATEDMASK | DISPFLAGS_UNUSEDMASK) == 0xffffffff);
    
    // check leaf shared flags
    Assert(CDispFlags::s_positionHasChanged.IsSet(CDispFlags::s_sharedMask));
    
    // check interior shared flags
    Assert(CDispFlags::s_balanceNode.IsSet(CDispFlags::s_sharedMask));
    Assert(CDispFlags::s_destructChildren.IsSet(CDispFlags::s_sharedMask));
    Assert(CDispFlags::s_fixedBackground.IsSet(CDispFlags::s_sharedMask));
}
#endif



