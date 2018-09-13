//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispitem.hxx
//
//  Contents:   CDispItem, the leaf node that invokes client drawing.
//
//----------------------------------------------------------------------------

#ifdef NEVER
//
// NOTE: This file is obsolete!  CDispItemPlus is now the only leaf node class
// used by the Display Tree.  This change was necessary due to coordinate
// system adjustments that were needed to accommodate GDI's 16-bit coordinate
// precision on Win 9x.
// 

#ifndef I_DISPITEM_HXX_
#define I_DISPITEM_HXX_
#pragma INCMSG("--- Beg 'dispitem.hxx'")

#ifndef X_DISPLEAF_HXX_
#define X_DISPLEAF_HXX_
#include "displeaf.hxx"
#endif

MtExtern(CDispItem)

//+---------------------------------------------------------------------------
//
//  Class:      CDispItem
//
//  Synopsis:   A leaf node in the display tree that invokes client drawing.
//
//----------------------------------------------------------------------------

class CDispItem :
    public CDispLeafNode
{
    DECLARE_DISPNODE(CDispItem, CDispLeafNode, DISPNODETYPE_ITEM)
    
    friend class CDispNode;
    friend class CDispInteriorNode;
    
protected:
    void*           _cookie;
    // 40 bytes (4 bytes + 36 bytes for CDispLeafNode)
    
protected:
    // object can be created only by CDispRoot, and destructed only from
    // special methods
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispItem))

                            CDispItem()
                                : CDispLeafNode() {}
                            ~CDispItem() {}

public:
    void                    SetRightToLeft(BOOL fRightToLeft)
                                    {SetBoolean(CDispFlags::s_rightToLeft,fRightToLeft);}

    void                    SetCookie(void* cookie) {_cookie = cookie;}
    virtual void*           GetCookie() const {return _cookie;}
    void*                   _GetCookie() const {return _cookie;}

    // CDispNode overrides
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const;
    virtual LONG            GetZOrder() const;

protected:
    // CDispNode overrides
    virtual BOOL            CalculateInView(
                                CDispContext* pContext,
                                BOOL fPositionChanged,
                                BOOL fNoRedraw);
    virtual void            Draw(
                                CDispDrawContext* pContext,
                                CDispNode* pChild);
    virtual BOOL            HitTestPoint(CDispHitContext* pContext) const;
    virtual void            GetNodeTransform(
                                CSize* pOffset,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            GetNodeTransform(
                                CDispContext* pContext,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;

    // CDispLeafNode overrides
    virtual void            NotifyInViewChange(
                                CDispContext* pContext,
                                BOOL fResolvedVisible,
                                BOOL fWasResolvedVisible,
                                BOOL fNoRedraw);
    
#if DBG==1
protected:
    virtual void            DumpInfo(HANDLE hFile, long level, long childNumber);
#endif
};

#pragma INCMSG("--- End 'dispitem.hxx'")
#else
#pragma INCMSG("*** Dup 'dispitem.hxx'")
#endif

#endif

