//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontainerplus.hxx
//
//  Contents:   A container node with optional border and user clip.
//
//----------------------------------------------------------------------------

#ifndef I_DISPCONTAINERPLUS_HXX_
#define I_DISPCONTAINERPLUS_HXX_
#pragma INCMSG("--- Beg 'dispcontainerplus.hxx'")

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

MtExtern(CDispContainerPlus)


//+---------------------------------------------------------------------------
//
//  Class:      CDispContainerPlus
//
//  Synopsis:   A container node with optional border and user clip.
//
//----------------------------------------------------------------------------

class CDispContainerPlus :
    public CDispContainer
{
    DECLARE_DISPNODE(CDispContainerPlus, CDispContainer, DISPNODETYPE_CONTAINERPLUS)

protected:
    LONG            _extra[UNSIZED_ARRAY];  // variable length, must be last
    // 72-112 bytes (4-44 bytes + 68 bytes for CDispContainer)
    
    // object can be created only by CDispRoot, and destructed only from
    // special methods
protected:
    void* operator new(size_t cb) {Assert(FALSE); return NULL;}    // do not use
    void* operator new(size_t cb, size_t cbExtra)
            {return(MemAllocClear(Mt(CDispContainerPlus), cb + cbExtra));}
    void  operator delete(void* pv) {MemFree(pv);}

                            CDispContainerPlus(CDispClient* pDispClient)
                                : CDispContainer(pDispClient) {}
                            CDispContainerPlus(const CDispItemPlus* pItemPlus);
                            ~CDispContainerPlus() {}

public:
    // CDispNode overrides
    virtual void            SetSize(const SIZE& size, BOOL fInvalidateAll);
            
protected:
    // CDispNode overrides
    virtual void            GetNodeTransform(
                                CSize* pOffset,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            GetNodeTransform(
                                CDispContext* pContext,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            CalcDispInfo(
                                const CRect& rcClip,
                                CDispInfo* pdi) const;
    virtual CDispExtras*    GetExtras() const {return (CDispExtras*) _extra;}

    // CDispInterior overrides
    virtual BOOL            ComputeVisibleBounds();
};



#pragma INCMSG("--- End 'dispcontainerplus.hxx'")
#else
#pragma INCMSG("*** Dup 'dispcontainerplus.hxx'")
#endif

